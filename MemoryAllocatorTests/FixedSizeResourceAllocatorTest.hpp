//
//  FixedSizeResourceAllocatorTest.hpp
//  MemoryAllocator
//
//  Created by FireWolf on 9/21/20.
//  Copyright © 2020 FireWolf. All rights reserved.
//

#ifndef FixedSizeResourceAllocatorTest_hpp
#define FixedSizeResourceAllocatorTest_hpp

#include "TestSuite.hpp"
#include <cstdint>

class FixedSizeResourceAllocatorTest: public TestSuite
{
private:
    struct Point
    {
        uint32_t x, y;
    };

    Point memory[12] = {};

public:
    void run() override;
};

#endif /* FixedSizeResourceAllocatorTest_hpp */
