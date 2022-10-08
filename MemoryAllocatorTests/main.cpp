//
//  main.cpp
//  MemoryAllocator
//
//  Created by FireWolf on 9/14/20.
//  Copyright © 2020 FireWolf. All rights reserved.
//

// Memory Allocator Tests
#include "MemoryAllocatorTests.hpp"

static FreeListAllocatorTest freeListAllocatorTest;
static FastFixedSizeResourceAllocatorTest fastFixedSizeResourceAllocatorTest;
static FixedSizeResourceAllocatorTest fixedSizeResourceAllocatorTest;
static BinaryBuddyAllocatorTest binaryBuddyAllocatorTest;

static TestSuite* tests[] =
{
    &freeListAllocatorTest,
    &fastFixedSizeResourceAllocatorTest,
    &fixedSizeResourceAllocatorTest,
    &binaryBuddyAllocatorTest
};

int main(int argc, const char * argv[])
{
    for (auto test : tests)
    {
        test->run();
    }

    return 0;
}
