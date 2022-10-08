//
//  FreeListAllocatorTest.cpp
//  MemoryAllocator
//
//  Created by FireWolf on 2020-9-16.
//  Copyright © 2020 FireWolf. All rights reserved.
//

#include "FreeListAllocatorTest.hpp"
#include <MemoryAllocator/FreeListAllocator.hpp>
#include <cstdio>

static constexpr uint32_t kMagicValueUsed = 0x55534544;
static constexpr uint32_t kMagicValueFree = 0x46524545; 
static constexpr uint32_t kMagicValueFire = 0x46495245;
static constexpr uint32_t kMagicValueWolf = 0x574F4C46;

void FreeListAllocatorTest::run()
{
    printf("==== TEST FREE LIST ALLOCATOR STARTED ====\n");

    printf("Memory Header Size = %lu.\n", sizeof(MemoryHeader));

    printf("Managed Memory Region Size = %lu.\n", sizeof(this->memory));

    // Setup
    FreeListAllocator<ConstantAligner<8>> allocator;

    allocator.init(this->memory, sizeof(this->memory));

    // Test Initial Setup
    passert(allocator.freeList.getCount() == 1, "There should be only 1 header in the free list.");

    const MemoryHeader* initial = allocator.freeList.peekHead();

    passert(allocator.freeList.peekTail() == allocator.freeList.peekHead(), "Tail header in the free list.");

    passert(reinterpret_cast<const uint8_t*>(initial) == reinterpret_cast<uint8_t*>(this->memory), "Initial header address.");

    passert(initial->prev == nullptr, "Initial header prev.");

    passert(initial->next == nullptr, "Initial header next.");

    passert(initial->size == sizeof(this->memory) - sizeof(MemoryHeader), "Initial header size.");

    passert(initial->magic == kMagicValueFree, "Initial header magic.");

    printf("Current Free List (after initial setup): ");

    allocator.freeList.forEach([](const MemoryHeader* header) { printf("%d -> ", header->size); });

    printf("NULL\n");

    // ------------------------- 1024 -------------------------
    //      256         256                 256         256
    // --------------------------------------------------------
    // Allocate 250 bytes, aligned to 256 bytes
    void* b1 = allocator.allocate(250);

    auto* b1header = MemoryHeader::readUsedHeader(b1);

    passert(b1header != nullptr, "B1 header should be valid.");

    passert(b1header->size == 256, "B1 header size should be 256.");

    passert(b1header->magic == kMagicValueUsed, "B1 header magic.");

    passert(reinterpret_cast<uintptr_t>(b1header->prev) == kMagicValueFire, "B1 header prev.");

    passert(reinterpret_cast<uintptr_t>(b1header->next) == kMagicValueWolf, "B1 header next.");

    passert(b1header->start == b1, "B1 header start address.");

    passert(allocator.freeList.getCount() == 1, "Should still be 1 header in the free list.");

    passert(allocator.freeList.peekHead()->size == (sizeof(this->memory) - sizeof(MemoryHeader) * 2 - 256), "Free space header size.");

    passert(allocator.freeList.peekHead()->magic == kMagicValueFree, "Free space header magic.");

    passert(allocator.freeList.peekHead()->prev == nullptr, "Free space header prev.");

    passert(allocator.freeList.peekHead()->next == nullptr, "Free space header next.");

    printf("Current Free List (after B1 is allocated): ");

    allocator.freeList.forEach([](const MemoryHeader* header) { printf("%d -> ", header->size); });

    printf("NULL\n");

    // Allocate another 256 block
    void* b2 = allocator.allocate(251);

    auto* b2header = MemoryHeader::readUsedHeader(b2);

    passert(b2header != nullptr, "B2 header should be valid.");

    passert(b2header->size == 256, "B2 header size should be 256.");

    passert(b2header->magic == kMagicValueUsed, "B2 header magic.");

    passert(reinterpret_cast<uintptr_t>(b2header->prev) == kMagicValueFire, "B2 header prev.");

    passert(reinterpret_cast<uintptr_t>(b2header->next) == kMagicValueWolf, "B2 header next.");

    passert(b2header->start == b2, "B2 header start address.");

    passert(allocator.freeList.getCount() == 1, "Should still be 1 header in the free list.");

    passert(allocator.freeList.peekHead()->size == (sizeof(this->memory) - sizeof(MemoryHeader) * 3 - 256 * 2), "Free space header size.");

    passert(allocator.freeList.peekHead()->magic == kMagicValueFree, "Free space header magic.");

    passert(allocator.freeList.peekHead()->prev == nullptr, "Free space header prev.");

    passert(allocator.freeList.peekHead()->next == nullptr, "Free space header next.");

    printf("Current Free List (after B2 is allocated): ");

    allocator.freeList.forEach([](const MemoryHeader* header) { printf("%d -> ", header->size); });

    printf("NULL\n");

    // Allocate the third 256 block
    void* b3 = allocator.allocate(252);

    auto* b3header = MemoryHeader::readUsedHeader(b3);

    passert(b3header != nullptr, "B3 header should be valid.");

    passert(b3header->size == 256, "B3 header size should be 256.");

    passert(b3header->magic == kMagicValueUsed, "B3 header magic.");

    passert(reinterpret_cast<uintptr_t>(b3header->prev) == kMagicValueFire, "B3 header prev.");

    passert(reinterpret_cast<uintptr_t>(b3header->next) == kMagicValueWolf, "B3 header next.");

    passert(b3header->start == b3, "B3 header start address.");

    passert(allocator.freeList.getCount() == 1, "Should still be 1 header in the free list.");

    passert(allocator.freeList.peekHead()->size == (sizeof(this->memory) - sizeof(MemoryHeader) * 4 - 256 * 3), "Free space header size.");

    passert(allocator.freeList.peekHead()->magic == kMagicValueFree, "Free space header magic.");

    passert(allocator.freeList.peekHead()->prev == nullptr, "Free space header prev.");

    passert(allocator.freeList.peekHead()->next == nullptr, "Free space header next.");

    printf("Current Free List (after B3 is allocated): ");

    allocator.freeList.forEach([](const MemoryHeader* header) { printf("%d -> ", header->size); });

    printf("NULL\n");

    // Attempt to allocate another 256 bytes
    passert(allocator.allocate(256) == nullptr, "Should fail because of no memory.");

    // Allocate 128 bytes
    void* b4 = allocator.allocate(128);

    auto* b4header = MemoryHeader::readUsedHeader(b4);

    passert(b4header != nullptr, "B4 header should be valid.");

    passert(b4header->size == 128, "B4 header size.");

    passert(allocator.freeList.getCount() == 1, "Should still be 1 header in the free list.");

    passert(allocator.freeList.peekHead()->size == (sizeof(this->memory) - sizeof(MemoryHeader) * 5 - 256 * 3 - 128), "Free space header size.");

    uint32_t available = allocator.freeList.peekHead()->size;

    printf("Current Free List (after B4 is allocated): ");

    allocator.freeList.forEach([](const MemoryHeader* header) { printf("%d -> ", header->size); });

    printf("NULL\n");

    // Current Status:
    // ------------------------- 1024 -------------------------
    //  H + 256  |  H + 256  |  H + 256  |  H + 128  | H + FREE
    // --------------------------------------------------------
    //    USED        USED        USED        USED       FREE
    //
    // Start to free blocks
    // Free b3: No merge operations
    passert(allocator.free(b3), "Should be able to free b3.");

    passert(allocator.freeList.getCount() == 2, "Should be 2 blocks in the free list.");

    passert(allocator.freeList.peekHead() == b3header, "First block is the b3 header.");

    passert(b3header->size == 256, "B3 size should still be 256 bytes.");

    passert(b3header->magic == kMagicValueFree, "B3 header magic should be FREE.");

    passert(b3header->prev == nullptr, "B3 header prev should be NULL.");

    passert(b3header->next == allocator.freeList.peekTail(), "B3 header next should be the tail.");

    printf("Current Free List (after B3 is freed): ");

    allocator.freeList.forEach([](const MemoryHeader* header) { printf("%d -> ", header->size); });

    printf("NULL\n");

    // Current Status:
    // ------------------------- 1024 -------------------------
    //  H + 256  |  H + 256  |  H + 256  |  H + 128  | H + FREE
    // --------------------------------------------------------
    //    USED        USED        FREE        USED       FREE
    //
    // Free b4: Merge with b3 and b5
    passert(allocator.free(b4), "Should be able to free b4.");

    passert(allocator.freeList.getCount() == 1, "Should be only 1 block in the free list.");

    passert(allocator.freeList.peekHead() == b3header, "Head block is the b3 header.");

    passert(allocator.freeList.peekTail() == b3header, "Tail block is the b3 header.");

    passert(b3header->size == 256 + 128 + available + sizeof(MemoryHeader) * 2, "Merged block size.");

    passert(b3header->magic == kMagicValueFree, "Merged block magic.");

    passert(b3header->prev == nullptr, "Merged block prev.");

    passert(b3header->next == nullptr, "Merged block next.");

    printf("Current Free List (after B4 is freed): ");

    allocator.freeList.forEach([](const MemoryHeader* header) { printf("%d -> ", header->size); });

    printf("NULL\n");

    // Current Status:
    // ------------------------- 1024 -------------------------
    //  H + 256  |  H + 256  |  H + 256 + 128 + FREE + 2*H
    // --------------------------------------------------------
    //    USED        USED        FREE
    //     b1          b2          b3 (merged)
    //
    // Free b1: No merge
    passert(allocator.free(b1), "Should be able to free b1.");

    passert(allocator.freeList.getCount() == 2, "Should be two blocks in the free list.");

    passert(allocator.freeList.peekHead() == b1header, "Head block is the b1 header.");

    passert(allocator.freeList.peekTail() == b3header, "Tail block is the b3 header.");

    passert(b1header->size == 256, "B1 size should still be 256.");

    passert(b1header->magic == kMagicValueFree, "B1 magic should be FREE.");

    passert(b1header->prev == nullptr, "B1 prev should be NULL.");

    passert(b1header->next == b3header, "B1 next should be B3.");

    passert(b3header->size == 256 + 128 + available + sizeof(MemoryHeader) * 2, "B3 size should remain unchanged.");

    passert(b3header->prev == b1header, "B3 prev should be B1.");

    passert(b3header->next == nullptr, "B3 next should be NULL.");

    printf("Current Free List (after B1 is freed): ");

    allocator.freeList.forEach([](const MemoryHeader* header) { printf("%d -> ", header->size); });

    printf("NULL\n");

    // Current Status:
    // ------------------------- 1024 -------------------------
    //  H + 256  |  H + 256  |  H + 256 + 128 + FREE + 2*H
    // --------------------------------------------------------
    //    FREE        USED        FREE
    //     b1          b2          b3 (merged)
    //
    // Free b2: Merge with b1 and b3
    passert(allocator.free(b2), "Should be able to free b2.");

    passert(allocator.freeList.getCount() == 1, "Should be only 1 block in the free list.");

    passert(allocator.freeList.peekHead() == b1header, "Head block is the b1 header.");

    passert(allocator.freeList.peekTail() == b1header, "Tail block is the b1 header.");

    passert(b1header->size == sizeof(this->memory) - sizeof(MemoryHeader), "B1 header size.");

    passert(b1header->magic == kMagicValueFree, "B1 header magic.");

    passert(b1header->prev == nullptr, "B1 header prev should be NULL.");

    passert(b1header->next == nullptr, "B1 header next should be NULL.");

    printf("Current Free List (after B2 is freed): ");

    allocator.freeList.forEach([](const MemoryHeader* header) { printf("%d -> ", header->size); });

    printf("NULL\n");

    // Miscellaneous
    passert(allocator.free(nullptr), "Should be fine to free a NULL pointer.");

    passert(allocator.allocate(0) == nullptr, "Allocating 0 is not allowed.");

    void* ptr = allocator.allocate(512);
    
    auto header = reinterpret_cast<MemoryHeader*>(reinterpret_cast<uint8_t*>(ptr) - sizeof(MemoryHeader));

    header->magic = 0;

    passert(!allocator.free(ptr), "Free memory with invalid magic should fail.");

    header->magic = kMagicValueUsed;

    header->prev = nullptr;

    passert(!allocator.free(ptr), "Free memory with invalid magic should fail.");

    header->prev = reinterpret_cast<MemoryHeader*>(kMagicValueFire);

    header->next = nullptr;

    passert(!allocator.free(ptr), "Free memory with invalid magic should fail.");

    printf("==== TEST FREE LIST ALLOCATOR FINISHED ====\n");
}
