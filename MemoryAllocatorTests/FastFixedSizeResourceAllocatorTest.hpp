//
//  FastFixedSizeResourceAllocatorTest.hpp
//  MemoryAllocator
//
//  Created by FireWolf on 9/16/20.
//  Copyright © 2020 FireWolf. All rights reserved.
//

#ifndef FastFixedSizeResourceAllocatorTest_hpp
#define FastFixedSizeResourceAllocatorTest_hpp

#include "TestSuite.hpp"
#include <cstdint>

class FastFixedSizeResourceAllocatorTest: public TestSuite
{
private:
    struct Tuple
    {
        uint32_t a, b, c, d;
    };

    Tuple memory[8] = {};

public:
    void run() override;
};

#endif /* FastFixedSizeResourceAllocatorTest_hpp */
