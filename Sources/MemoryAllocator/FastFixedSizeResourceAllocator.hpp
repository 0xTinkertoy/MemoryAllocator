//
//  FastFixedSizeResourceAllocator.hpp
//  MemoryAllocator
//
//  Created by FireWolf on 2020-9-16.
//  Copyright © 2020 FireWolf. All rights reserved.
//

#ifndef MemoryAllocator_FastFixedSizeResourceAllocator_hpp
#define MemoryAllocator_FastFixedSizeResourceAllocator_hpp

#include "MemoryAllocator.hpp"
#include "MemoryAligners.hpp"
#include <LinkedList.hpp>

///
/// Represents a resource block that conforms to `MemoryBlock` protocol
///
struct ResourceBlock: Listable<ResourceBlock>
{
    /// MemoryBlock IMP
    void* operator()()
    {
        return this;
    }
};

///
/// The fast fixed size resource allocator divides a chunk of memory into multiple small fixed size regions,
/// where each region can hold exactly one instance of the given `Resource` type.
/// This allocator maintains a linked list of free resources, so that both allocation and free can be done in O(1).
/// The caller may use this allocator to maintain a pool of objects that are frequently used by other components.
/// The caller may choose to declare a static memory region or allocate a chunk of memory from other allocators.
/// Note that this allocator does not maintain additional information of each `resource`,
/// so it does not work with "paging" if memory is insufficient and the system must swap resources out.
///
/// @tparam Resource Specify the type of the resource.
///
template <typename Resource>
class FastFixedSizeResourceAllocator: public MemoryAllocatorImp<ResourceBlock, NullAligner>
{
    // We have two pointers stored in a resource block to maintain the list.
    // on 64-bit system, the block is 16 bytes long, while on 32-bit system is 8 bytes long.
    // As a result, if the size of the actual resource is less than 8 bytes, the linked list would be corrupted.
    // Besides, it is really rare to maintain a pool of uint64_t or other types that is smaller than 8 bytes.
    // Therefore, this allocator requires that the resource size must be greater than the size of the resource block.
    static_assert(sizeof(Resource) >= sizeof(ResourceBlock), "Size of the resource must be greater than that of the block.");

#ifdef DEBUG
    friend class FastFixedSizeResourceAllocatorTest;
#endif

private:
    /// A list of free resources
    LinkedList<ResourceBlock> freeList;

protected:
    ///
    /// Find a free block that can hold the requested amount of memory
    ///
    /// @param size The number of bytes to allocate in accordance with alignment
    /// @return The free block on success, `NULL` if insufficient memory.
    ///
    ResourceBlock* getFreeBlock(size_t size) override
    {
        // Guard: The given size must be identical to the resource size
        if (size != sizeof(Resource))
        {
            perr("Inconsistent size. The given size %lu is not identical to the resource size %lu.", size, sizeof(Resource));

            return nullptr;
        }

        return this->freeList.dequeue();
    }

    ///
    /// Put back the given free block so that it can be used for other allocations
    ///
    /// @param block The block that is free to use
    /// @note Upon return, the given block can be selected by subsequent invocations of `getFreeBlock()`.
    /// @note Allocators might choose to coalesce the given block with other blocks to avoid external fragmentation.
    ///
    void putFreeBlock(ResourceBlock* block) override
    {
        this->freeList.enqueue(block);
    }

    ///
    /// Mark a block free
    ///
    /// @param block The block that is in use
    ///
    void markBlockFree([[maybe_unused]] ResourceBlock* block) override
    {
        // Nothing to do
    }

    ///
    /// Mark a block used
    ///
    /// @param block The block that is free to use
    /// @note Upon return, the given block cannot be allocated to satisfy other requests until freed.
    ///
    void markBlockUsed([[maybe_unused]] ResourceBlock* block) override
    {
        // Nothing to do
    }

    ///
    /// Locate the block that is associated with the given pointer
    ///
    /// @param pointer A non-null pointer to allocated memory
    /// @return The block that is associated with the given pointer, `NULL` on error.
    ///
    ResourceBlock* pointer2Block(void* pointer) override
    {
        return reinterpret_cast<ResourceBlock*>(pointer);
    }

public:
    ///
    /// Initialize the fixed size resource allocator with the given contiguous free memory region
    ///
    /// @param start The start address of the free memory region
    /// @param limit The size of the free memory region in bytes
    /// @return `true` on success, `false` if the size of the memory region is not a multiple of the resource size.
    ///
    bool init(void* start, size_t limit)
    {
        // Guard: Check the size of the memory region
        if (limit % sizeof(Resource) != 0)
        {
            perr("Memory region size %lu is not a multiple of the resource size %lu.", limit, sizeof(Resource));

            return false;
        }

        // Add all free resources to the list
        auto current = reinterpret_cast<uint8_t*>(start);

        auto end = current + limit;

        while (current < end)
        {
            this->freeList.enqueue(reinterpret_cast<ResourceBlock*>(current));

            current += sizeof(Resource);
        }

        return true;
    }

    ///
    /// [Convenient, Recommended] Allocate resource from the pool
    ///
    /// @return The allocated resource on success, `NULL` if no space left.
    ///
    Resource* allocate()
    {
        return reinterpret_cast<Resource*>(MemoryAllocatorImp<ResourceBlock, NullAligner>::allocate(sizeof(Resource)));
    }

    using MemoryAllocatorImp<ResourceBlock, NullAligner>::allocate;
};

#endif /* MemoryAllocator_FastFixedSizeResourceAllocator_hpp */
