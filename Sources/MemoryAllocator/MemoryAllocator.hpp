//
//  MemoryAllocator.hpp
//  MemoryAllocator
//
//  Created by FireWolf on 2020-9-14.
//  Copyright © 2020 FireWolf. All rights reserved.
//

#ifndef MemoryAllocator_MemoryAllocator_hpp
#define MemoryAllocator_MemoryAllocator_hpp

#include <concepts>
#include <Debug.hpp>
#include <cstddef>

/// Declare the kernel memory allocator
#define OSDeclareMemoryAllocator(type, name) \
static type name;

/// Declare the kernel memory allocator as well as memory management routines
#define OSDeclareMemoryAllocatorWithKernelServiceRoutines(type, name) \
OSDeclareMemoryAllocator(type, name)                                  \
extern "C" void* kmalloc(size_t size)                                 \
{                                                                     \
    return name.allocate(size);                                       \
}                                                                     \
                                                                      \
extern "C" void kfree(void* ptr)                                      \
{                                                                     \
    name.free(ptr);                                                   \
}

///
/// Public interface of a memory allocator
///
/// @note To implement a custom memory allocator, refer to the `MemoryAllocatorImp` class.
///
class MemoryAllocator
{
public:
    ///
    /// Allocate the requested amount of memory
    ///
    /// @param size Specify the amount of memory in bytes
    /// @return A pointer to newly allocated memory on success, `NULL` otherwise.
    ///
    virtual void* allocate(size_t size) = 0;

    ///
    /// Free previously allocated memory
    ///
    /// @param pointer A pointer to previously allocated memory
    /// @return `true` on success, `false` if pointer is invalid.
    /// @note No action is performed if the given pointer is `NULL`.
    ///
    virtual bool free(void* pointer) = 0;
};

///
/// `MemoryBlock` is the basic structure managed by a memory allocator
///
/// It could be a header if the allocator maintains a list of free memory regions;
/// It could be a fixed size memory block if the allocator maintains a pool of objects.
/// A memory block must implement the operator `()` to return its start address,
/// i.e. the address that is handed back to the caller of `allocate()`.
///
template <typename T>
concept MemoryBlock = requires(T entity)
{
    { entity() } -> std::same_as<void*>;
};

///
/// Defines a set of methods to be implemented by a memory allocator
///
/// @tparam Block Specify the basic structure managed by the allocator
///               Must implement the operator function `void* operator()()`.
/// @tparam Aligner Specify the behavior of memory alignment
///               Must implement the operator function `uint32_t operator()(uint32_t size)` to
///               return the actual number of bytes to be allocated in accordance with alignment.
///
template <typename Block, typename Aligner>
requires MemoryBlock<Block>
class MemoryAllocatorImp: public MemoryAllocator
{
protected:
    ///
    /// Find a free block that can hold the requested amount of memory
    ///
    /// @param size The number of bytes to allocate in accordance with alignment
    /// @return The free block on success, `NULL` if insufficient memory.
    ///
    virtual Block* getFreeBlock(size_t size) = 0;

    ///
    /// Put back the given free block so that it can be used for other allocations
    ///
    /// @param block The block that is free to use
    /// @note Upon return, the given block can be selected by subsequent invocations of `getFreeBlock()`.
    /// @note Allocators might choose to coalesce the given block with other blocks to avoid external fragmentation.
    ///
    virtual void putFreeBlock(Block* block) = 0;

    ///
    /// Mark a block free
    ///
    /// @param block The block that is in use
    ///
    virtual void markBlockFree(Block* block) = 0;

    ///
    /// Mark a block used
    ///
    /// @param block The block that is free to use
    /// @note Upon return, the given block cannot be allocated to satisfy other requests until freed.
    ///
    virtual void markBlockUsed(Block* block) = 0;

    ///
    /// Locate the block that is associated with the given pointer
    ///
    /// @param pointer A non-null pointer to allocated memory
    /// @return The block that is associated with the given pointer, `NULL` on error.
    ///
    virtual Block* pointer2Block(void* pointer) = 0;

public:
    ///
    /// Allocate the requested amount of memory
    ///
    /// @param size Specify the amount of memory in bytes
    /// @return A pointer to newly allocated memory on success, `NULL` otherwise.
    ///
    void* allocate(size_t size) override
    {
        // Guard: Return null if size is 0
        if (size == 0)
        {
            return nullptr;
        }

        // Guard: Find the block that can hold the requested size (aligned)
        Block* block = this->getFreeBlock(Aligner()(size));

        if (block == nullptr)
        {
            perr("Failed to find a free block to hold %lu bytes.", size);

            return nullptr;
        }

        // Mark the block used and return its address
        this->markBlockUsed(block);

        return (*block)();
    }

    ///
    /// Free previously allocated memory
    ///
    /// @param pointer A pointer to previously allocated memory
    /// @return `true` on success, `false` if pointer is invalid.
    /// @note No action is performed if the given pointer is `NULL`.
    ///
    bool free(void* pointer) override
    {
        // Guard: It's OK to free a NULL pointer
        if (pointer == nullptr)
        {
            return true;
        }

        // Guard: Locate the block that is associated with the given pointer
        Block* block = this->pointer2Block(pointer);

        if (block == nullptr)
        {
            perr("Failed to find the block that is associated with the pointer %p.", pointer);

            return false;
        }

        // Mark the block free and put it back
        this->markBlockFree(block);

        this->putFreeBlock(block);

        return true;
    }
};

#endif /* MemoryAllocator_MemoryAllocator_hpp */
