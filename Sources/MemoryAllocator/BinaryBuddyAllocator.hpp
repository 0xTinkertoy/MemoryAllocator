//
//  BinaryBuddyAllocator.hpp
//  MemoryAllocator
//
//  Created by FireWolf on 9/22/20.
//  Copyright © 2020 FireWolf. All rights reserved.
//

#ifndef MemoryAllocator_BinaryBuddyAllocator_hpp
#define MemoryAllocator_BinaryBuddyAllocator_hpp

#include "MemoryAllocator.hpp"
#include "MemoryAligners.hpp"
#include <StaticBitVector.hpp>
#include <SignificantBit.hpp>

/// A dummy resource block that conforms to the `MemoryBlock` protocol
struct ResourceBlock
{
    /// The block index in the binary tree
    size_t index;

    void* operator()()
    {
        return this;
    }
};

///
/// The binary buddy allocator divides a chunk of memory into partitions and
/// tries to find one that is best suited for the requested amount of memory.
/// This allocator assigns each memory block an order, ranging from 0 to `MaxOrder`.
/// A block of order k has a size of `BasicBlockSize * 2^k`.
/// As such, the size of the largest block is determined by the parameter `MaxOrder`.
/// The caller should choose appropriate values for both `MaxOrder` and `BasicBlockSize` to avoid memory waste.
///
/// @tparam MaxOrder Specify the maximum order of a memory block
/// @tparam BasicBlockSize Specify the size of the smallest memory block
///
template <size_t MaxOrder, size_t BasicBlockSize>
class BinaryBuddyAllocator: public MemoryAllocatorImp<ResourceBlock, NullAligner>
{
private:
#ifdef DEBUG
    friend class BinaryBuddyAllocatorTest;
#endif
    /// Internally, this allocator maintains a binary tree to keep track of the status of each block.
    /// For example, if we have 128 bytes memory available and the basic block is 16 bytes long,
    /// then the maximum order is 3, because 2 ^ 3 * 16 bytes = 128 bytes.
    /// Initially, we have a block of order 3 that spans the entire free memory.
    /// Now we want to allocate 12 bytes, the allocator needs to find a block of order 3 to fulfill the request.
    /// It starts by splitting the order 3 block into two order 2 blocks.
    /// Similarly, it needs to split an order 2 block into two order 1 blocks and eventually into two order 0 blocks.
    /// As such, the binary tree becomes
    ///
    /// 0                                       127
    /// ----------------- 128 B -----------------
    /// -------- 64 B -------|------- 64 B ------
    /// -- 32 B --|-- 32 B --|
    /// -16B-|-16B-
    /// ALLOC FREE
    ///
    /// If we have 8 requests to allocate 16 bytes, the allocator divides the free memory area into eight 16-byte blocks.
    /// In this case, we have a perfect binary tree, and hence the maximum number of nodes is 2 ^ Depth - 1,
    /// where `Depth` is the tree depth and equals to `MaxOrder + 1`.
    static constexpr size_t MaxNumNodes = PowerOf2()(MaxOrder + 1) - 1;

    /// Get the depth of a block of order K in the binary tree
    static constexpr size_t Order2Depth(size_t order) { return MaxOrder - order; }

    /// Get the order of a block at depth D in the binary tree
    static constexpr size_t Depth2Order(size_t depth) { return MaxOrder - depth; }

    /// Get the size of a block of order K
    static constexpr size_t Order2Size(size_t order) { return PowerOf2()(order) * BasicBlockSize; }

    /// Get the depth of a block at the given index
    static constexpr size_t Index2Depth(size_t index)
    {
        // Given a block order `K`, the range of index is [2^D - 1, 2^(D+1) - 2] (Length = 2^D).
        // Therefore, for a block at index `i`, its depth in the tree is Floor(log2(i+1)).
        // This is equivalent to finding the position of the MSB in `i + 1`.
        return MSBFinder<size_t>()(index + 1);
    }

    /// Get the order of a block at the given index
    static constexpr size_t Index2Order(size_t index)
    {
        return Depth2Order(Index2Depth(index));
    }

    /// The maximum block size is the size of the block that has the maximum order
    static constexpr size_t MaxBlockSize = Order2Size(MaxOrder);

    /// The start address of the free memory region
    void* start = nullptr;

    /// We could use a struct `left, right` to represent a tree node, but the memory overhead is too high.
    /// In the worst case, we need at least `MaxNumNodes * 8` bytes to store the perfect binary tree on 32-bit system.
    /// In the above case, we have at most 15 nodes and hence we need 120 bytes to store the tree.
    /// Instead, we use a flat bit array to represent the tree, where each bit indicates whether a block is free or not.
    /// As a result, the memory consumption is reduced to `RoundUp(MaxNumNodes / 8)` bytes.
    StaticBitVector<MaxNumNodes> tree;

    /// However, since it is possible that a memory block of order K has been split into two blocks of order K - 1,
    /// it is not sufficient to use just one single bit to determine the current status of a memory block.
    /// We can avoid pre-allocating memory to store other control information by carefully defining the status as follows.
    ///
    /// For a block `B` of order K, where K is greater than 0 (i.e. it is not the leaf node in the tree)
    ///
    /// 1. It is **free** if its free bit is set and its children blocks all have free bit clear.
    ///     In this case, when the allocator tries to find a free block of order `K - 1`, it won't return one of B's children as the result.
    ///     Consequently, the allocator avoids splitting a higher order block unnecessarily when a free block of order `K - 1` exists.
    ///
    /// 2. It is **allocated** if its free bit is clear and its children blocks all have free bit set.
    ///     In this case, when the allocator tries to find a free block of order `K - 1`, it must also ensure that the parent block is not allocated.
    ///     When children blocks are all free, it implicitly means that they are merged and become parts of their parent block.
    ///
    /// 3. It is **split** if its free bit is clear and at least one of its children has free bit clear.
    ///     This is based on the fact that the allocator splits a block of order K only when there is no free block of order K - 1.
    ///     As a result, the allocator will use one of its children to fulfill the allocation request and mark the other one as free.
    ///
    /// As such, we translate above definitions into the following helper functions.

    // MARK: Tree-related Properties

    ///
    /// Get the left child of the node at the given index
    ///
    /// @param index The index of the node in the binary tree
    /// @return The index of its left child block.
    ///
    inline size_t getLeftChild(size_t index)
    {
        return index * 2 + 1;
    }

    ///
    /// Get the right child of the node at the given index
    ///
    /// @param index The index of the node in the binary tree
    /// @return The index of its right child block.
    ///
    inline size_t getRightChild(size_t index)
    {
        return index * 2 + 2;
    }

    ///
    /// Get the parent of the node at the given index
    ///
    /// @param index The index of the node in the binary tree
    /// @return The index of its parent block.
    /// @precondition This function assumes the the given block is not the root block.
    ///
    inline size_t getParent(size_t index)
    {
        passert(!this->isRoot(index), "The given block cannot be the root node in the tree.");

        return (index - 1) / 2;
    }

    ///
    /// Check whether the node at the given index is the root node
    ///
    /// @param index The index of the node in the binary tree
    /// @return `true` if it is the root node of the tree, `false` otherwise.
    ///
    inline bool isRoot(size_t index)
    {
        return index == 0;
    }

    ///
    /// Check whether a block at the given index is the leaf node in the tree
    ///
    /// @param index The index of the node in the binary tree
    /// @return `true` if it is the leaf node (i.e. has an order of 0), `false` otherwise.
    ///
    inline bool isLeaf(size_t index)
    {
        return this->getLeftChild(index) >= MaxNumNodes;
    }

    ///
    /// Check whether a block at the given index is the left child of some other block in the tree
    ///
    /// @param index The index of the node in the binary tree
    /// @return `true` if it is the left child, `false` otherwise.
    /// @precondition This function assumes the the given block is not the root block.
    ///
    inline bool isLeftChild(size_t index)
    {
        passert(!this->isRoot(index), "The given block cannot be the root node in the tree.");

        // Left child has an odd index
        return (index & 0x1U) == 0x1;
    }

    // MARK: Examine Block Status

    ///
    /// Check whether a block at the given index is free
    ///
    /// @param index The index of the node in the binary tree
    /// @return `true` if it is free, `false` otherwise.
    ///
    bool isFree(size_t index)
    {
        // Guard: Check whether the block has an order of 0
        if (this->isLeaf(index))
        {
            // The free bit directly reflects the status of a leaf block
            return this->tree.containsBit(index);
        }

        return this->tree.containsBit(index) &&
               !this->tree.containsBit(this->getLeftChild(index)) &&
               !this->tree.containsBit(this->getRightChild(index));
    }

    ///
    /// Check whether a block at the given index is allocated
    ///
    /// @param index The index of the node in the binary tree
    /// @return `true` if it is allocated, `false` otherwise.
    ///
    bool isAllocated(size_t index)
    {
        // Guard: Check whether the block has an order of 0
        if (this->isLeaf(index))
        {
            // The free bit directly reflects the status of a leaf block
            return !this->tree.containsBit(index);
        }

        return !this->tree.containsBit(index) &&
                this->tree.containsBit(this->getLeftChild(index)) &&
                this->tree.containsBit(this->getRightChild(index));
    }

    ///
    /// Check whether a block at the given index is split
    ///
    /// @param index The index of the node in the binary tree
    /// @return `true` if it is split, `false` otherwise.
    ///
    bool isSplit(size_t index)
    {
        // Guard: Check whether the block has an order of 0
        if (this->isLeaf(index))
        {
            // A leaf block cannot be split
            return false;
        }

        return !this->tree.containsBit(index) &&
                (this->tree.getBit(this->getLeftChild(index)) + this->tree.getBit(this->getRightChild(index)) < 2);
    }

    ///
    /// Get the string description of the status of a block at the given index
    ///
    /// @param index The index of the node in the binary tree
    /// @return A non-null status string.
    ///
    const char* getBlockStatus(size_t index)
    {
        if (this->isFree(index))
        {
            return "Free";
        }

        if (this->isAllocated(index))
        {
            return "Alloc";
        }

        if (this->isSplit(index))
        {
            return "Split";
        }

        return "Error";
    }

    // MARK: Manage Buddy Blocks

    ///
    /// Get the buddy block of the block at the given index
    ///
    /// @param index The index of the node in the binary tree
    /// @return The index of its buddy block.
    /// @precondition This function assumes that the given block is not the root block.
    ///
    size_t getBuddyBlock(size_t index)
    {
        passert(!this->isRoot(index), "The given block cannot be the root block.");

        if (this->isLeftChild(index))
        {
            // The given block is the left child
            return index + 1;
        }
        else
        {
            // The given block is the right child
            return index - 1;
        }
    }

    ///
    /// Split the block at the given index into two smaller blocks
    ///
    /// @param index The index of the node in the binary tree
    /// @return The index of its left child.
    /// @note Upon return, the given block is marked split and children blocks are marked free.
    /// @precondition This function assumes that the given block is free and is not the leaf node in tree.
    ///
    size_t splitBlock(size_t index)
    {
        passert(!this->isLeaf(index), "Cannot split a block that is the leaf node in the tree.");

        passert(this->isFree(index), "Attempt to split a non-free block.");

        // Clear the free bit of the given block
        this->tree.clearBit(index);

        // Set the free bit of children blocks
        this->tree.setBit(this->getLeftChild(index));

        this->tree.setBit(this->getRightChild(index));

        // Return the left child
        return this->getLeftChild(index);
    }

    ///
    /// Merge the block at the given index with its buddy block
    ///
    /// @param index The index of the node in the binary tree
    /// @return The index of its parent block.
    /// @note Upon return, the given block and its buddy block have free bit clear,
    ///       while their parent block is marked free to satisfy the above definition.
    /// @precondition This function assumes that the buddy block is also free.
    ///
    size_t mergeBlock(size_t index)
    {
        size_t buddy = this->getBuddyBlock(index);

        size_t parent = this->getParent(index);

        passert(this->isFree(buddy), "The buddy block of the given block at index %lu should be free.", index);

        passert(this->getParent(buddy) == parent, "The buddy block should have the same parent as the given block at index %lu.", index);

        // Clear the free bit of both children block
        this->tree.clearBit(index);

        this->tree.clearBit(buddy);

        // Set the free bit of their parent block
        this->tree.setBit(this->getParent(index));

        return parent;
    }

    // MARK: Locate Free Blocks

    ///
    /// [Helper] Get the order of a block that can hold the given number of bytes
    ///
    /// @param size The amount of memory in bytes
    /// @return The order of a block that can hold the requested amount of memory.
    ///
    size_t size2Order(size_t size)
    {
        // First round the size to the next multiple of the basic block size
        // e.g. Block Size = 100 bytes; Order 0 to 3: 100, 200, 400, 800 bytes block
        // This returns the number of Basic Blocks required to hold the requested amount of memory
        size_t numBasicBlocks = (size / BasicBlockSize) + ((size % BasicBlockSize) ? 1 : 0);

        // Then find the next power of 2 of the number of Basic Blocks
        numBasicBlocks = NextPowerOf2Finder()(numBasicBlocks);

        // Finally calculates the order of the above number of blocks
        return MSBFinder()(numBasicBlocks);
    }

    ///
    /// [Helper] Find a free block of the given order "K"
    ///
    /// @param order The order of the memory block.
    /// @return The index of the free block of order K on success, `-1` otherwise.
    /// @note This function recursively searches for and splits a higher order block if no free block of order K is available.
    /// @note This function returns `-1` if the given order K is out of bounds.
    ///
    ssize_t getFreeBlockOfOrder(size_t order)
    {
        pinfo("Called with block order %lu.", order);

        // Guard: The order should not exceed the maximum order supported by the allocator
        if (order > MaxOrder)
        {
            pinfo("The requested order [%lu] exceeds the maximum order [%lu] supported by this allocator.", order, MaxOrder);

            return -1;
        }

        // >> Step 2: Determine the range of bits in the tree to search for a free block of order `K`
        size_t depth = Order2Depth(order);

        auto range = ClosedRange<size_t>::createWithLength(PowerOf2()(depth) - 1, PowerOf2()(depth));

        // >> Step 3: Search for a free block of order K by finding a set bit
        ssize_t index = -1;

        while (range.lowerBound <= range.upperBound)
        {
            pinfo("Attempt to find a free block of order %lu within range [%lu, %lu].", order, range.lowerBound, range.upperBound);

            index = this->tree.findLeastSignificantBitIndexWithRange(range);

            // Guard: Check whether a free block of order K is available
            if (index < 0)
            {
                pinfo("Unable to find a free block of order %lu.", order);

                pinfo("Will try to find a free block of order %lu.", order + 1);

                // No free block of order K
                // The allocator needs to search for and splits a free block of order K + 1
                index = this->getFreeBlockOfOrder(order + 1);

                // Guard: Check whether a free block of order K + 1 is available
                if (index < 0)
                {
                    // Failure: Insufficient memory
                    pinfo("Unable to find a free block of order K + 1 (K = %lu). Insufficient memory.", order);
                }
                else
                {
                    // The allocator has a free block of order K + 1
                    // Split it into two blocks of order K and return one of them
                    pinfo("Found a free block of a higher order %lu at index %lu.", order + 1, index);

                    pinfo("Will split it into two blocks of order %lu.", order);

                    index = this->splitBlock(index);

                    pinfo("The left child of the split block is at index %lu.", index);
                }

                break;
            }

            // The allocator has a free block of order K
            // Guard: Check whether the free block is the root node of the tree
            if (this->isRoot(index))
            {
                // OK: The free block is the root node of the tree
                pinfo("Found a free block of order %lu at the root node.", order);

                break;
            }

            // Guard: Ensure that the parent block of this block is not allocated
            //        If the parent has already been allocated, then both children blocks are unavailable
            if (this->isAllocated(this->getParent(index)))
            {
                // The allocator cannot use this block of order K because the parent block has already been allocated
                // Adjust the lower bound of the search range and try again
                // If this block is the left child of the parent, we also skip the right child
                range.lowerBound += this->isLeftChild(index) ? 2 : 1;

                pinfo("Found a free block of order %lu at index %lu but its parent at index %lu has already been allocated.", order, index, this->getParent(index));

                pinfo("Cannot use this free block. Will keep searching with adjusted range [%lu, %lu]", range.lowerBound, range.upperBound);

                continue;
            }

            // The allocator indeed has found a free block of order K
            // Its parent block must have been split and its children blocks should all have free bit clear
            passert(this->isSplit(this->getParent(index)), "Parent of this free block at index %ld must have been split.", index);

            passert(this->isFree(index), "At least one of the children of this free block at index %ld has free bit set.", index);

            pinfo("Found a free block of order %lu at index %lu. It is the child of parent block at %lu.", order, index, this->getParent(index));

            break;
        }

        pinfo("Return with block index %ld.", index);

        return index;
    }

#ifdef DEBUG
    // MARK: Print Binary Tree Nodes

    ///
    /// [DEBUG] Print the information of a block at the given index
    ///
    /// @param index The index of the node in the binary tree
    ///
    void printBlock(size_t index)
    {
        printf("Block%02lu [Order %lu] (%s)", index, Index2Order(index), this->getBlockStatus(index));
    }

    ///
    /// [DEBUG] Traverse the binary tree in pre-order and print the information of each block
    ///
    /// @param index The index of the node in the binary tree as the current root node
    /// @param padding Specify the indentation string before printing a block
    /// @param pointer Specify the tree formatting before printing a block
    /// @ref Modified from https://www.baeldung.com/java-print-binary-tree-diagram.
    ///
    void traverseBlocks(size_t index, const char* padding, const char* pointer)
    {
        static constexpr size_t size = (MaxOrder + 2) * 4;

        char newPadding[size] = {0};

        if (index < MaxNumNodes)
        {
            printf("\n%s%s", padding, pointer);

            printBlock(index);

            snprintf(newPadding, size, "%s%s", padding, this->isLeftChild(index) ? "│   " : "    ");

            traverseBlocks(this->getLeftChild(index), newPadding, "├───");

            traverseBlocks(this->getRightChild(index), newPadding, "└───");
        }
    }
#endif

protected:
    ///
    /// Find a free block that can hold the requested amount of memory
    ///
    /// @param size The number of bytes to allocate in accordance with alignment
    /// @return The free block on success, `NULL` if insufficient memory.
    ///
    ResourceBlock* getFreeBlock(size_t size) override
    {
        // >> Step 1: Find the order `K` of a block that can hold `size` bytes
        size_t order = this->size2Order(size);

        pinfo("Called with size %lu bytes. Requires a block of order %lu.", size, order);

        // The rest of work is delegated to the helper
        ssize_t index = this->getFreeBlockOfOrder(order);

        // Guard: Verify the index of the free block
        if (index < 0)
        {
            return nullptr;
        }

        // Find the block at the index
        // Calculate the stride from the first block of order `K`
        size_t stride = index - (PowerOf2()(Order2Depth(order)) - 1);

        auto block = reinterpret_cast<ResourceBlock*>(reinterpret_cast<uint8_t*>(this->start) + stride * Order2Size(order));

        block->index = index;

        return block;
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
        // Merge the block if possible
        if (this->isRoot(block->index))
        {
            // Root block (that has the maximum order) does not have a buddy block
            return;
        }

        pinfo("Attempt to merge the block at index %lu with its buddy.", block->index);

        // Try to merge with the buddy block "recursively"
        // `index` stores the index of a block to be merged if possible
        size_t index = block->index;

        while (true)
        {
            // Guard: Check whether the current block is the root block
            if (this->isRoot(index))
            {
                pinfo("Reached the root block. Can not merge the block further. The entire memory region is free.");

                break;
            }

            // Guard: Check whether the buddy block is also free
            if (!this->isFree(this->getBuddyBlock(index)))
            {
                pinfo("Aborted merging because the buddy block of the block at index %lu is not free.", index);

                break;
            }

            // The buddy block is free
            pinfo("The buddy block of the block at index %lu is also free. Will merge them.", index);

            size_t parent = this->mergeBlock(index);

            pinfo("The block at index %lu has been merged with its buddy block at index %lu into their parent block %lu.",
                  index, this->getBuddyBlock(index), parent);

            // Update the index to try to merge the parent block and its buddy block
            index = parent;
        }
    }

    ///
    /// Mark a block free
    ///
    /// @param block The block that is in use
    ///
    void markBlockFree(ResourceBlock* block) override
    {
        // Set the free bit
        this->tree.setBit(block->index);

        // Children blocks of a free block must have free bit clear (definition above)
        // Guard: Check whether the given block has children
        if (!this->isLeaf(block->index))
        {
            this->tree.clearBit(this->getLeftChild(block->index));

            this->tree.clearBit(this->getRightChild(block->index));
        }
    }

    ///
    /// Mark a block used
    ///
    /// @param block The block that is free to use
    /// @note Upon return, the given block cannot be allocated to satisfy other requests until freed.
    ///
    void markBlockUsed(ResourceBlock* block) override
    {
        // Clear the free bit
        this->tree.clearBit(block->index);

        // Children blocks of an allocated block must have free bit set (definition above)
        // Guard: Check whether the given block has children
        if (!this->isLeaf(block->index))
        {
            this->tree.setBit(this->getLeftChild(block->index));

            this->tree.setBit(this->getRightChild(block->index));
        }
    }

    ///
    /// Locate the block that is associated with the given pointer
    ///
    /// @param pointer A non-null pointer to allocated memory
    /// @return The block that is associated with the given pointer, `NULL` on error.
    ///
    ResourceBlock* pointer2Block(void* pointer) override
    {
        // Apply the binary search:
        // The given pointer is the address of either an order K block `B` or one of B's children block.
        // `saddr` stores the start address of the current order K block
        auto saddr = reinterpret_cast<uint8_t*>(this->start);

        // `index` stores the index of the current order K block in the binary tree
        ssize_t index = 0;

        // Start from the root block of the maximum order (depth = 0)
        for (size_t order = MaxOrder; order >= 0; order -= 1)
        {
            // The current block of interest has an order of `K` and starts at `saddr`.
            pinfo("Tree Depth = %lu: Examine the block that has an order of %lu and starts at %p.", MaxOrder - order, order, saddr);

            // Check the given address against the start address of the current order K block
            if (pointer == saddr)
            {
                // Address matched:
                // The given pointer might be the address of the current order K block or one of its left (grand)child.
                // i.e. Left child that has an order K - 1 or the left child that has an order K - 2 or ...
                // Guard: The current order K block must be either allocated or split
                passert(!this->isFree(index), "The current block of order %lu at index %lu cannot be free.", order, index);

                // Guard: Check whether the current block is allocated or not
                if (this->isAllocated(index))
                {
                    // Found the block that starts at `pointer` and the order also matches
                    pinfo("Found the block that starts at %p. Order = %lu; Index = %lu.", pointer, order, index);

                    break;
                }

                // Guard: Check whether the current block is split or not
                if (this->isSplit(index))
                {
                    // Found the block that starts at `pointer` but the order should be smaller
                    // Keep searching until we found an allocated block that starts at `pointer`
                    // Adjust the index accordingly: Will examine the left child of the current order K block
                    index = this->getLeftChild(index);

                    continue;
                }

                pfatal("Should never reach at here.");
            }

            // Guard: Ensure that the current block is not the leaf node
            if (order == 0)
            {
                // A valid pointer that is the start address of an order 0 block should never reach at here.
                perr("The given pointer %p is not valid.", pointer);

                index = -1;

                break;
            }

            // Check the given address against the start address of the right child of the current order K block
            if (pointer < saddr + Order2Size(order - 1))
            {
                // The given address is part of the left child of the current order K block
                index = this->getLeftChild(index);
            }
            else
            {
                // The given address is part of the right child of the current order K block
                saddr += Order2Size(order - 1);

                index = this->getRightChild(index);
            }
        }

        // Guard: Verify the index
        if (index < 0)
        {
            return nullptr;
        }

        auto block = reinterpret_cast<ResourceBlock*>(pointer);

        block->index = index;

        return block;
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
        this->start = start;

        if (limit < MaxBlockSize)
        {
            perr("The size of the free memory (%lu bytes) is not large enough to hold the maximum block of size %lu bytes.", limit, MaxBlockSize);

            return false;
        }

        if (limit > MaxBlockSize)
        {
            pwarning("The size of the free memory (%lu bytes) is larger than the maximum block of size %lu bytes. Wasted memory.", limit, MaxBlockSize);
        }

        // Initially, the root block is free, and hence its children and children of children should all have free bit clear
        this->tree.initWithZeros();

        this->tree.setBit(0);

        pinfo("Initialized with start address %p and size %lu bytes.", start, limit);

        pinfo("Basic block size is %lu bytes; Max block size is %lu bytes.", BasicBlockSize, MaxBlockSize);

        pinfo("Max block order is %lu; Max number of nodes in the tree is %lu.", MaxOrder, MaxNumNodes);

        pinfo("Size of the binary tree is %lu bytes.", sizeof(this->tree));

        return true;
    }

#ifdef DEBUG
    ///
    /// Print the binary tree with detailed information about each block
    ///
    /// @ref Modified from https://www.baeldung.com/java-print-binary-tree-diagram.
    ///
    void printTree()
    {
        printBlock(0);

        traverseBlocks(this->getLeftChild(0), "", "├───");

        traverseBlocks(this->getRightChild(0), "", "└───");

        printf("\n");
    }
#endif
};

#endif /* MemoryAllocator_BinaryBuddyAllocator_hpp */
