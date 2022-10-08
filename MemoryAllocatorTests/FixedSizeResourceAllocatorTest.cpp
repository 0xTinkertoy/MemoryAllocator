//
//  FixedSizeResourceAllocatorTest.cpp
//  MemoryAllocator
//
//  Created by FireWolf on 9/21/20.
//  Copyright © 2020 FireWolf. All rights reserved.
//

#include "FixedSizeResourceAllocatorTest.hpp"
#include <MemoryAllocator/FixedSizeResourceAllocator.hpp>
#include "Debug.hpp"
#include <cstdio>

void FixedSizeResourceAllocatorTest::run()
{
    printf("==== TEST FIXED SIZE RESOURCE ALLOCATOR STARTED ====\n");

    printf("Resource Block Size = %lu.\n", sizeof(ResourceBlock));

    printf("Resource Size = %lu.\n", sizeof(Point));

    // Setup
    FixedSizeResourceAllocator<Point, 12, uint8_t> allocator;

    passert(allocator.init(this->memory, sizeof(this->memory)), "Allocation Initialization.");

    for (int index = 0; index < 12; index += 1)
    {
        passert(allocator.bitmap.containsBit(index), "Bit %d should be set.", index);
    }

    for (int index = 12; index < 16; index += 1)
    {
        passert(!allocator.bitmap.containsBit(index), "Bit %d should be clear.", index);
    }

    // Allocate
    Point* points[12];

    for (int index = 0; index < 12; index += 1)
    {
        points[index] = allocator.allocate();

        passert(points[index] != nullptr, "Point %d can be allocated.", index);

        passert(!allocator.bitmap.containsBit(index), "Bit %d should be clear.", index);

        pinfo("Point %02d allocated at %p.", index, points[index]);
    }

    // Subsequent allocation should fail due to no memory
    passert(allocator.allocate() == nullptr, "No memory.");

    pinfo("Allocation Test Passed.");

    // Deallocate
    allocator.free(points[5]);

    passert(allocator.bitmap.containsBit(5), "Bit %d should be set.", 5);

    pinfo("Deallocation Test Passed.");

    // Allocate again
    points[5] = allocator.allocate();

    passert(points[5] != nullptr, "Should be able to allocate again.");

    passert(!allocator.bitmap.containsBit(5), "Bit %d should be clear.", 5);

    pinfo("Next Allocation Test Passed.");

    // Deallocate
    allocator.free(points[5]);

    allocator.free(points[3]);

    passert(allocator.bitmap.containsBit(5), "Bit %d should be set.", 5);

    passert(allocator.bitmap.containsBit(3), "Bit %d should be set.", 3);

    pinfo("Multiple Deallocations Test Passed.");

    // Allocate again
    passert(allocator.allocate() != nullptr, "Should be able to allocate.");

    passert(allocator.bitmap.containsBit(5), "Bit %d should be set.", 5);

    passert(!allocator.bitmap.containsBit(3), "Bit %d should be clear.", 3);

    pinfo("Multiple Aellocations Test Passed.");

    printf("==== TEST FIXED SIZE RESOURCE ALLOCATOR FINISHED ====\n");
}
