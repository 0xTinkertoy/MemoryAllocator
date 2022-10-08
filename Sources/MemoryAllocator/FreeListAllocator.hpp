//
//  FreeListAllocator.hpp
//  MemoryAllocator
//
//  Created by FireWolf on 9/15/20.
//  Copyright © 2020 FireWolf. All rights reserved.
//

#ifndef MemoryAllocator_FreeListAllocator_hpp
#define MemoryAllocator_FreeListAllocator_hpp

#include "MemoryAllocator.hpp"
#include "MemoryAligners.hpp"
#include <LinkedList.hpp>

/// A private header that describes a memory region managed by the free list allocator
struct MemoryHeader: Listable<MemoryHeader>
{
    /// The length of this memory region excluding the header
    uint32_t size;

    /// A 4-byte magic value used to perform a simple sanity check
    uint32_t magic;

    /// A convenient pointer to the start of the memory region
    uint8_t start[0];

    // MARK: MemoryBlock IMP

    /// Return the start address of the region
    void* operator()()
    {
        return this->start;
    }

    // MARK: Comparable IMP

    friend bool operator<(const MemoryHeader& lhs, const MemoryHeader& rhs)
    {
        return reinterpret_cast<const uint8_t*>(lhs.start) < reinterpret_cast<const uint8_t*>(rhs.start);
    }

    friend bool operator>(const MemoryHeader& lhs, const MemoryHeader& rhs)
    {
        return rhs < lhs;
    }

    friend bool operator<=(const MemoryHeader& lhs, const MemoryHeader& rhs)
    {
        return !(lhs > rhs);
    }

    friend bool operator>=(const MemoryHeader& lhs, const MemoryHeader& rhs)
    {
        return !(lhs < rhs);
    }

    // MARK: Convenient Helpers

    ///
    /// Write a free header to the given address
    ///
    /// @param address The address to store the new header
    /// @param size The length of the free memory in bytes
    /// @return The header that is free to use. The address is the same as the given one.
    ///
    static inline MemoryHeader* writeFreeHeader(void* address, uint32_t size)
    {
        auto* header = reinterpret_cast<MemoryHeader*>(address);

        header->size = size;

        header->magic = MemoryHeader::kMagicValueFree;

        header->prev = nullptr;

        header->next = nullptr;

        return header;
    }

    ///
    /// Read a used header from the given address
    ///
    /// @param address The start address of allocated memory region
    /// @return A non-null header that stores control information about the allocated memory region.
    ///         `NULL` if the header is corrupted. e.g. It's magic value mismatched.
    ///
    static inline MemoryHeader* readUsedHeader(void* address)
    {
        auto* header = reinterpret_cast<MemoryHeader*>(reinterpret_cast<uint8_t*>(address) - sizeof(MemoryHeader));

        // Guard: The magic value must be `USED`
        if (header->magic != MemoryHeader::kMagicValueUsed)
        {
            return nullptr;
        }

        // Guard: The `prev` and `next` fields must be valid
        if (reinterpret_cast<uintptr_t>(header->prev) != MemoryHeader::kMagicValueFire)
        {
            return nullptr;
        }

        if (reinterpret_cast<uintptr_t>(header->next) != MemoryHeader::kMagicValueWolf)
        {
            return nullptr;
        }

        // All sanity checks passed
        // Hope that the header is not corrupted
        return header;
    }

    /// Mark this header free
    inline void setFree()
    {
        this->magic = MemoryHeader::kMagicValueFree;

        this->prev = nullptr;

        this->next = nullptr;
    }

    /// Mark this header used
    inline void setUsed()
    {
        this->magic = MemoryHeader::kMagicValueUsed;

        this->prev = reinterpret_cast<MemoryHeader*>(MemoryHeader::kMagicValueFire);

        this->next = reinterpret_cast<MemoryHeader*>(MemoryHeader::kMagicValueWolf);
    }

    ///
    /// Check whether the memory region described by this header is adjacent to the other one
    ///
    /// @param other A non-null memory header that describes another memory region
    /// @return `true` if both regions are adjacent to each other, `false` otherwise.
    ///
    inline bool isAdjacentTo(MemoryHeader* other)
    {
        return this->start + this->size == reinterpret_cast<uint8_t*>(other);
    }

private:
    // MARK: Magic Values
    static constexpr uint32_t kMagicValueUsed = 0x55534544; // 'USED';
    static constexpr uint32_t kMagicValueFree = 0x46524545; // 'FREE';
    static constexpr uint32_t kMagicValueFire = 0x46495245; // 'FIRE';
    static constexpr uint32_t kMagicValueWolf = 0x574F4C46; // 'WOLF';
};

/// A special linked list of memory regions
class FreeList: public LinkedList<MemoryHeader>
{
public:
    ///
    /// Insert the given block and merge adjacent blocks if necessary
    ///
    /// @param block A non-null block to be inserted
    ///
    void insertWithMerge(MemoryHeader* block)
    {
        // Add the block to the free list
        this->insert(block, true);

        // Merge Adjacent Blocks
        // Check the previous block
        if (block->prev != nullptr && block->prev->isAdjacentTo(block))
        {
            // Merge `block` into its previous one
            block->prev->size += sizeof(MemoryHeader) + block->size;

            block->prev->next = block->next;

            // Check whether the current block is the tail
            if (block->next != nullptr)
            {
                block->next->prev = block->prev;
            }
            else
            {
                this->tail = block->prev;
            }

            // Merged: Now `block` points to merged block
            block = block->prev;

            this->count -= 1;
        }

        // Check the next block
        if (block->next != nullptr && block->isAdjacentTo(block->next))
        {
            // Merge the next one into `block`
            block->size += sizeof(MemoryHeader) + block->next->size;

            // Check whether the next block is the tail
            if (block->next->next != nullptr)
            {
                block->next->next->prev = block;
            }
            else
            {
                this->tail = block;
            }

            // Merged: Now `block`'s next is part of `block`
            block->next = block->next->next;

            this->count -= 1;
        }
    }

    ///
    /// Remove the given block and update the free list to preserve unused memory left in the given block
    ///
    /// @param block A non-null block to be removed and contains the actual required amount of memory
    ///
    void removeWithUpdate(MemoryHeader* block)
    {
        // Remove the given block from the free list
        this->remove(block);

        // Retrieve the actual amount of required memory from the header
        uint32_t actualSize = block->magic;

        uint32_t leftSize = block->size - actualSize;

        // Adjust the header if the left amount of memory is large enough
        if (leftSize > sizeof(MemoryHeader))
        {
            // Shrink the allocated block
            block->size = actualSize;

            // Write a new header for the rest amount of memory
            auto newBlock = MemoryHeader::writeFreeHeader(block->start + actualSize, leftSize - sizeof(MemoryHeader));

            // Add it to the free list
            this->insert(newBlock, true);
        }
    }
};

///
/// A free list allocator maintains a linked list to keep track of free regions
///
/// It uses a small header to store control information in front of the allocated memory block.
/// By default, it does not align the memory and finds the first free block that satisfies the allocation request.
/// The caller could specify a custom aligner to specify how memory should be aligned.
/// This allocator sorts free regions by their starting address, so that it can coalesce adjacent regions if necessary.
///
/// @tparam Aligner Specify how to align memory
///
template <typename Aligner = NullAligner>
class FreeListAllocator: public MemoryAllocatorImp<MemoryHeader, Aligner>
{
    /// The caller must specify an aligner that also works with the actual size of the memory header.
    /// On a 32-bit machine, Memory Header is 16 bytes long, so an aligner that aligns on a 2/4/8/16-byte boundary is recommended.
    /// If we choose to align memory on a 32-byte boundary, then we will waste 16 bytes right after the header.
    static_assert(Aligner{}(sizeof(MemoryHeader)) == sizeof(MemoryHeader),
                  "Choose an aligner that also works with the size of the memory header,"
                  "so that there won't be wasted memory between the header and allocated memory returned back to the caller.");

#ifdef DEBUG
    friend class FreeListAllocatorTest;
#endif

private:
    /// A list of free regions
    FreeList freeList;

protected:
    ///
    /// Find a free block that can hold the requested amount of memory
    ///
    /// @param size The number of bytes to allocate in accordance with alignment
    /// @return The free block on success, `NULL` if insufficient memory.
    ///
    MemoryHeader* getFreeBlock(size_t size) override
    {
        // Guard: Find a free block that is large enough to hold requested amount of memory
        auto predicate = [&](const MemoryHeader* header) -> bool
        {
            return header->size >= size;
        };

        MemoryHeader* header = this->freeList.first(predicate);

        // Hack: Save the actual amount of required memory into the block for later use
        //const_cast<MemoryHeader*>(header)->magic = size;
        if (header != nullptr)
        {
            header->magic = size;
        }

        return header;
    }

    ///
    /// Put back the given free block so that it can be used for other allocations
    ///
    /// @param block The block that is free to use
    /// @note Upon return, the given block can be selected by subsequent invocations of `getFreeBlock()`.
    /// @note Allocators might choose to coalesce the given block with other blocks to avoid external fragmentation.
    ///
    void putFreeBlock(MemoryHeader* block) override
    {
        // Add the block to the free list and merge adjacent blocks if necessary
        this->freeList.insertWithMerge(block);
    }

    ///
    /// Mark a block free
    ///
    /// @param block The block that is in use
    ///
    void markBlockFree(MemoryHeader* block) override
    {
        // Mark the block free
        block->setFree();
    }

    ///
    /// Mark a block used
    ///
    /// @param block The block that is free to use
    /// @note Upon return, the given block cannot be allocated to satisfy other requests until freed.
    ///
    void markBlockUsed(MemoryHeader* block) override
    {
        // Remove the given block from the free list
        // Preserve the memory left after the allocation if necessary
        this->freeList.removeWithUpdate(block);

        // Write magic values to the header
        block->setUsed();
    }

    ///
    /// Locate the block that is associated with the given pointer
    ///
    /// @param pointer A non-null pointer to allocated memory
    /// @return The block that is associated with the given pointer, `NULL` on error.
    ///
    MemoryHeader* pointer2Block(void* pointer) override
    {
        return MemoryHeader::readUsedHeader(pointer);
    }

public:
    ///
    /// Initialize the free list allocator with the given contiguous free memory region
    ///
    /// @param start The start address of the free memory region
    /// @param limit The size of the free memory region in bytes
    /// @return `true` on success, `false` otherwise.
    /// @note This function currently always returns `true`.
    ///
    bool init(void* start, size_t limit)
    {
        // Write a header at the given `start` address
        // Align it if the given `start` is not aligned properly
        auto aligned = reinterpret_cast<void*>(Aligner()(reinterpret_cast<size_t>(start)));

        if (aligned != start)
        {
            pwarning("The given start address %p is not aligned.", start);
        }

        auto block = MemoryHeader::writeFreeHeader(aligned, limit - sizeof(MemoryHeader));

        // Add to the free list
        this->freeList.enqueue(block);

        return true;
    }
};

#endif /* MemoryAllocator_FreeListAllocator_hpp */
