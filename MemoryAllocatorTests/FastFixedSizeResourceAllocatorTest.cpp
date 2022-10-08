//
//  FastFixedSizeResourceAllocatorTest.cpp
//  MemoryAllocator
//
//  Created by FireWolf on 9/16/20.
//  Copyright © 2020 FireWolf. All rights reserved.
//

#include "FastFixedSizeResourceAllocatorTest.hpp"
#include <MemoryAllocator/FastFixedSizeResourceAllocator.hpp>
#include "Debug.hpp"
#include <cstdio>

void FastFixedSizeResourceAllocatorTest::run()
{
    printf("==== TEST FAST FIXED SIZE RESOURCE ALLOCATOR STARTED ====\n");

    printf("Resource Block Size = %lu.\n", sizeof(ResourceBlock));

    printf("Resource Size = %lu.\n", sizeof(Tuple));

    // Setup
    FastFixedSizeResourceAllocator<Tuple> allocator;

    passert(!allocator.init(nullptr, sizeof(Tuple) * 4 + 1), "Should fail if the region size %% resource size != 0");

    passert(allocator.init(this->memory, sizeof(this->memory)), "Should be able to initialize the allocator.");

    // Test Initial Setup
    passert(allocator.freeList.getCount() == 8, "Initially 8 free resources.");

    passert(reinterpret_cast<const uint8_t*>(allocator.freeList.peekHead()) == reinterpret_cast<uint8_t*>(this->memory), "Head resource is the first one.");

    passert(reinterpret_cast<const uint8_t*>(allocator.freeList.peekTail()) == reinterpret_cast<uint8_t*>(this->memory + 7), "Tail resource is the last one.");

    printf("Current Free List (after initial setup): ");

    allocator.freeList.forEach([](const ResourceBlock* block) { printf("%p -> ", block); });

    printf("NULL\n");

    // Allocate 7 objects
    Tuple* tuples[8];

    for (int index = 0; index < 7; index += 1)
    {
        tuples[index] = allocator.allocate();

        passert(tuples[index] == &this->memory[index], "Allocate object index = %d.", index);

        passert(reinterpret_cast<const uint8_t*>(allocator.freeList.peekHead()) == reinterpret_cast<uint8_t*>(&this->memory[index + 1]), "Head resource is resource[%d].", index);

        passert((reinterpret_cast<const uint8_t*>(allocator.freeList.peekTail()) == reinterpret_cast<uint8_t*>(&this->memory[7])), "Tail resource is the last one.");

        printf("Current Free List (after object index = %d is allocated): ", index);

        allocator.freeList.forEach([](const ResourceBlock* block) { printf("%p -> ", block); });

        printf("NULL\n");
    }

    // Allocate the last object
    tuples[7] = allocator.allocate();

    passert(tuples[7] == &this->memory[7], "Allocate object index = 7.");

    passert(allocator.freeList.isEmpty(), "Free list should be empty.");

    passert(allocator.freeList.peekHead() == nullptr, "Head should be NULL.");

    passert(allocator.freeList.peekTail() == nullptr, "Tail should be NULL.");

    // Subsequent allocation requests should fail
    passert(allocator.allocate() == nullptr, "Insufficient memory.");

    // Free object 0
    passert(allocator.free(tuples[0]), "Free object 0.");

    passert(allocator.freeList.getCount() == 1, "Free list should have 1 element.");

    passert(reinterpret_cast<const uint8_t*>(allocator.freeList.peekHead()) == reinterpret_cast<uint8_t*>(tuples[0]), "Head is object 0.");

    passert(reinterpret_cast<const uint8_t*>(allocator.freeList.peekTail()) == reinterpret_cast<uint8_t*>(tuples[0]), "Tail is object 0.");

    passert(allocator.freeList.peekHead()->prev == nullptr, "Head prev set to NULL.");

    passert(allocator.freeList.peekTail()->next == nullptr, "Head next set to NULL.");

    // Free object 7
    passert(allocator.free(tuples[7]), "Free object 7.");

    passert(allocator.freeList.getCount() == 2, "Free list should have 2 elements.");

    passert(reinterpret_cast<const uint8_t*>(allocator.freeList.peekHead()) == reinterpret_cast<uint8_t*>(tuples[0]), "Head is object 0.");

    passert(reinterpret_cast<const uint8_t*>(allocator.freeList.peekTail()) == reinterpret_cast<uint8_t*>(tuples[7]), "Tail is object 7.");

    // Allocate an object
    passert(allocator.allocate() == tuples[0], "Allocate an object. Returned pointer should be object 0.");

    passert(allocator.freeList.getCount() == 1, "Free list should have 1 element.");

    passert(allocator.allocate() == tuples[7], "Allocate another object. Returned pointer should be object 7.");

    passert(allocator.freeList.getCount() == 0, "Free list should be empty.");

    auto alloc = reinterpret_cast<MemoryAllocatorImp<ResourceBlock, NullAligner>*>(&allocator);

    passert(alloc->allocate(8) == nullptr, "Attempt to allocate with an invalid size.");

    printf("==== TEST FAST FIXED SIZE RESOURCE ALLOCATOR STARTED ====\n");
}
