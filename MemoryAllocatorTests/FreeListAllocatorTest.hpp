//
//  FreeListAllocatorTest.hpp
//  MemoryAllocator
//
//  Created by FireWolf on 2020-9-16.
//  Copyright © 2020 FireWolf. All rights reserved.
//

#ifndef FreeListAllocatorTest_hpp
#define FreeListAllocatorTest_hpp

#include "TestSuite.hpp"
#include <cstdint>

class FreeListAllocatorTest: public TestSuite
{
private:
    uint8_t memory[1024] = {};

public:
    void run() override;
};

#endif /* FreeListAllocatorTest_hpp */
