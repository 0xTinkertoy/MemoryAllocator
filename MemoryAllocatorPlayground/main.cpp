//
//  main.cpp
//  MemoryAllocatorPlayground
//
//  Created by FireWolf on 10/8/22.
//  Copyright © 2022 FireWolf. All rights reserved.
//

#include <iostream>
#include <MemoryAllocator/BinaryBuddyAllocator.hpp>

int main(int argc, const char * argv[])
{
    BinaryBuddyAllocator<2, 16> allocator;

    static constexpr size_t kSize = 64;

    auto memory = new uint8_t[kSize];

    allocator.init(memory, kSize);

    allocator.printTree();

    allocator.allocate(12);

    allocator.printTree();

    delete [] memory;

    return 0;
}
