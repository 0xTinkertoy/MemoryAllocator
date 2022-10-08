//
//  FixedSizeResourceAllocator.hpp
//  MemoryAllocator
//
//  Created by FireWolf on 2020-9-17.
//  Copyright © 2020 FireWolf. All rights reserved.
//

#ifndef MemoryAllocator_FixedSizeResourceAllocator_hpp
#define MemoryAllocator_FixedSizeResourceAllocator_hpp

#include "MemoryAllocator.hpp"
#include "MemoryAligners.hpp"
#include <cstdint>
#include <StaticBitVector.hpp>

///
/// Represents a resource block that conforms to `MemoryBlock` protocol
///
struct ResourceBlock
{
    /// MemoryBlock IMP
    void* operator()()
    {
        return this;
    }
};

///
/// The fixed size resource allocator divides a chunk of memory into multiple small fixed size regions,
/// where each region can hold exactly one instance of the given `Resource` type.
/// This allocator maintains a bitmap to keep track of which block is allocated, so allocate() takes O(n) while free() takes O(1)
/// The caller may use this allocator to maintain a pool of objects that are frequently used by other components.
/// The caller may choose to declare a static memory region or allocate a chunk of memory from other allocators.
/// Note that this allocator can be extended to work with "paging" if memory is insufficient.
///
/// @tparam Resource Specify the type of the resource.
/// @tparam NumResources Specify the number of resources.
/// @tparam BitVectorStorageUnit Specify the storage unit of the bit map
///
template <typename Resource, size_t NumResources, typename BitVectorStorageUnit = uint32_t>
class FixedSizeResourceAllocator: public MemoryAllocatorImp<ResourceBlock, NullAligner>
{
#ifdef DEBUG
    friend class FixedSizeResourceAllocatorTest;
#endif

private:
    /// A bitmap to keep track of free blocks: Bit is set if it is free
    StaticBitVector<NumResources, BitVectorStorageUnit> bitmap;

    /// The start address of the memory
    Resource* resources;

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

        // Find a free block
        int index = this->bitmap.findLeastSignificantBitIndex();

        if (index < 0)
        {
            // No more free blocks
            return nullptr;
        }

        return reinterpret_cast<ResourceBlock*>(&this->resources[index]);
    }

    ///
    /// Put back the given free block so that it can be used for other allocations
    ///
    /// @param block The block that is free to use
    /// @note Upon return, the given block can be selected by subsequent invocations of `getFreeBlock()`.
    /// @note Allocators might choose to coalesce the given block with other blocks to avoid external fragmentation.
    ///
    void putFreeBlock([[maybe_unused]] ResourceBlock* block) override
    {
        // Nothing to do
    }

    ///
    /// Mark a block free
    ///
    /// @param block The block that is in use
    ///
    void markBlockFree(ResourceBlock* block) override
    {
        this->bitmap.setBit(reinterpret_cast<Resource*>(block) - this->resources);
    }

    ///
    /// Mark a block used
    ///
    /// @param block The block that is free to use
    /// @note Upon return, the given block cannot be allocated to satisfy other requests until freed.
    ///
    void markBlockUsed(ResourceBlock* block) override
    {
        this->bitmap.clearBit(reinterpret_cast<Resource*>(block) - this->resources);
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

        // Guard: Ensure the given amount of memory can hold `NumResources` resources
        if (limit / sizeof(Resource) < NumResources)
        {
            perr("Memory region size %lu is too small to hold %lu resources.", limit, NumResources);

            return false;
        }

        // Warning if there are wasted memory
        if (limit / sizeof(Resource) > NumResources)
        {
            pwarning("The given region is larger than the actual amount of memory required to store %lu resources. Wasted %lu bytes.",
                     NumResources, limit - NumResources * sizeof(Resource));
        }

        // Initially all blocks are free
        this->bitmap.initWithOnes();

        this->resources = reinterpret_cast<Resource*>(start);

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


#endif /* MemoryAllocator_FixedSizeResourceAllocator_hpp */
