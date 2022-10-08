//
//  BinaryBuddyAllocatorTest.hpp
//  MemoryAllocator
//
//  Created by FireWolf on 9/22/20.
//  Copyright © 2020 FireWolf. All rights reserved.
//

#ifndef BinaryBuddyAllocatorTest_hpp
#define BinaryBuddyAllocatorTest_hpp

#include "TestSuite.hpp"
#include <cstdint>

class BinaryBuddyAllocatorTest: public TestSuite
{
private:
    uint8_t memory[128] = {0};

public:
    void run() override;
};

#endif /* BinaryBuddyAllocatorTest_hpp */
