//
//  BinaryBuddyAllocatorTest.cpp
//  MemoryAllocator
//
//  Created by FireWolf on 9/22/20.
//  Copyright © 2020 FireWolf. All rights reserved.
//

#include "BinaryBuddyAllocatorTest.hpp"
#include <MemoryAllocator/BinaryBuddyAllocator.hpp>

void BinaryBuddyAllocatorTest::run()
{
    printf("==== TEST BINARY BUDDY ALLOCATOR STARTED ====\n");

    // Setup: 128 bytes memory, Max Order = 3, Basic Block Size = 16: 16, 32, 64, 128 bytes block
    BinaryBuddyAllocator<3, 16> allocator;

    passert(allocator.init(this->memory, 128), "Initialization.");

    // Basic Tree Properties
    passert(allocator.MaxNumNodes == 15, "Max number of nodes.");

    passert(allocator.Order2Depth(3) == 0, "Order 3 block resides at depth 0.");

    passert(allocator.Order2Depth(2) == 1, "Order 3 block resides at depth 1.");

    passert(allocator.Order2Depth(1) == 2, "Order 3 block resides at depth 2.");

    passert(allocator.Order2Depth(0) == 3, "Order 3 block resides at depth 3.");

    passert(allocator.Depth2Order(0) == 3, "Blocks at depth 0 have an order of 3.");

    passert(allocator.Depth2Order(1) == 2, "Blocks at depth 1 have an order of 2.");

    passert(allocator.Depth2Order(2) == 1, "Blocks at depth 2 have an order of 1.");

    passert(allocator.Depth2Order(3) == 0, "Blocks at depth 3 have an order of 0.");

    passert(allocator.Order2Size(3) == 128, "Order 3 blocks are 128 bytes long.");

    passert(allocator.Order2Size(2) == 64, "Order 2 blocks are 64 bytes long.");

    passert(allocator.Order2Size(1) == 32, "Order 1 blocks are 32 bytes long.");

    passert(allocator.Order2Size(0) == 16, "Order 0 blocks are 16 bytes long.");

    passert(allocator.Index2Depth(0) == 0, "Block at index 0 resides at depth 0.");

    passert(allocator.Index2Depth(1) == 1, "Block at index 1 resides at depth 1.");

    passert(allocator.Index2Depth(2) == 1, "Block at index 2 resides at depth 1.");

    passert(allocator.Index2Depth(3) == 2, "Block at index 2 resides at depth 2.");

    passert(allocator.Index2Depth(6) == 2, "Block at index 2 resides at depth 2.");

    passert(allocator.Index2Depth(7) == 3, "Block at index 7 resides at depth 3.");

    passert(allocator.Index2Depth(11) == 3, "Block at index 11 resides at depth 3.");

    passert(allocator.Index2Depth(14) == 3, "Block at index 14 resides at depth 3.");

    passert(allocator.getLeftChild(0) == 1, "Left(0) is 1.");

    passert(allocator.getLeftChild(5) == 11, "Left(5) is 11.");

    passert(allocator.getRightChild(0) == 2, "Right(0) is 2.");

    passert(allocator.getRightChild(5) == 12, "Right(5) is 12.");

    passert(allocator.getParent(5) == 2, "Parent(5) is 2.");

    passert(allocator.getParent(7) == 3, "Parent(7) is 3.");

    passert(allocator.isRoot(0), "IsRoot(0) = YES.");

    passert(!allocator.isRoot(7), "IsRoot(0) = NO.");

    passert(!allocator.isLeaf(5), "IsLeaf(5) = NO.");

    passert(allocator.isLeaf(13), "IsLeaf(13) = YES.");

    passert(allocator.isLeftChild(1), "IsLeftChild(1) = YES.");

    passert(allocator.isLeftChild(11), "IsLeftChild(11) = YES.");

    passert(!allocator.isLeftChild(2), "IsLeftChild(2) = NO.");

    passert(!allocator.isLeftChild(4), "IsLeftChild(4) = NO.");

    passert(allocator.getBuddyBlock(1) == 2, "Buddy(1) = 2.");

    passert(allocator.getBuddyBlock(6) == 5, "Buddy(6) = 5.");

    pinfo("Basic Tree Properties: Test Passed.");

    passert(allocator.size2Order(10) == 0, "10 bytes -> Order 0 block.");

    passert(allocator.size2Order(15) == 0, "15 bytes -> Order 0 block.");

    passert(allocator.size2Order(24) == 1, "24 bytes -> Order 1 block.");

    passert(allocator.size2Order(30) == 1, "30 bytes -> Order 1 block.");

    passert(allocator.size2Order(45) == 2, "45 bytes -> Order 2 block.");

    passert(allocator.size2Order(65) == 3, "65 bytes -> Order 3 block.");

    passert(allocator.size2Order(192) == 4, "192 bytes -> Order 4 block.");

    pinfo("Size -> Order: Test Passed.");

    // Allocate 10 bytes: An order 0 block is required
    pinfo("Prepare to allocate 10 bytes.");

    pinfo("Tree: [BEFORE]");

    allocator.printTree();

    void* block161 = allocator.allocate(10);

    passert(block161 == this->memory, "Should be able to allocate 10 bytes.");

    pinfo("Tree: [AFTER]");

    allocator.printTree();

    passert(allocator.isAllocated(7), "Block at index 7 should be allocated.");

    passert(allocator.isFree(8), "Block at index 8 should be free.");

    passert(allocator.isSplit(3), "Block at index 3 should be split.");

    passert(allocator.isFree(4), "Block at index 4 should be free.");

    passert(allocator.isSplit(1), "Block at index 1 should be split.");

    passert(allocator.isFree(2), "Block at index 2 should be free.");

    passert(allocator.isSplit(0), "Block at index 0 should be split.");

    // Allocate 12 bytes: An order 0 block is required.
    pinfo("Prepare to allocate 12 bytes.");

    pinfo("Tree: [BEFORE]");

    allocator.printTree();

    void* block162 = allocator.allocate(12);

    passert(block162 == &this->memory[16], "Should be able to allocate 12 bytes.");

    pinfo("Tree: [AFTER]");

    allocator.printTree();

    passert(allocator.isAllocated(8), "Block at index 8 should be allocated.");

    passert(allocator.isSplit(3), "Block at index 3 should remain split.");

    passert(allocator.isFree(4), "Block at index 4 should remain free.");

    // Allocate 24 bytes: An order 1 block is required.
    pinfo("Prepare to allocate 24 bytes.");

    pinfo("Tree: [BEFORE]");

    allocator.printTree();

    void* block321 = allocator.allocate(24);

    passert(block321 == &this->memory[32], "Should be able to allocate 24 bytes.");

    pinfo("Tree: [AFTER]");

    allocator.printTree();

    passert(allocator.isAllocated(4), "Block at index 4 should be allocated.");

    passert(allocator.isSplit(1), "Block at index 1 should remain split.");

    passert(allocator.isFree(2), "Block at index 2 should remain free.");

    // At this moment, the left 64 bytes are allocated.
    // Allocate 13 bytes: An order 0 block is required.
    pinfo("Prepare to allocate 13 bytes.");

    pinfo("Tree: [BEFORE]");

    allocator.printTree();

    void* block163 = allocator.allocate(13);

    passert(block163 == &this->memory[64], "Should be able to allocate 13 bytes.");

    pinfo("Tree: [AFTER]");

    allocator.printTree();

    passert(allocator.isAllocated(11), "Block at index 11 should be allocated.");

    passert(allocator.isFree(12), "Block at index 12 should be free.");

    passert(allocator.isSplit(5), "Block at index 5 should be split.");

    passert(allocator.isFree(6), "Block at index 6 should be free.");

    passert(allocator.isSplit(2), "Block at index 2 should be split.");

    // Allocate 64 bytes: An order 2 block is required.
    // Should fail because of no memory
    passert(allocator.allocate(64) == nullptr, "Should not be able to allocate 64 bytes.");

    // Allocate 16 bytes: An order 0 block is required.
    pinfo("Tree: [BEFORE]");

    allocator.printTree();

    void* block164 = allocator.allocate(16);

    passert(block164 == &this->memory[80], "Should be able to allocate 16 bytes.");

    pinfo("Tree: [AFTER]");

    allocator.printTree();

    passert(allocator.isAllocated(12), "Block at index 12 should be allocated.");

    passert(allocator.isSplit(5), "Block at index 5 should remain split.");

    passert(allocator.isFree(6), "Block at index 6 should remain free.");

    passert(allocator.isSplit(2), "Block at index 2 should remain split.");

    // At this moment, we only have 32 bytes free.
    // Start to deallocate memory
    // Release Block 11 and 12, so these two blocks are merged and Block 5 becomes free.
    // Block 5's buddy block, Block 6, is also free, so they are merged and Block 2 becomes free.
    pinfo("Prepare to release Block 11.");

    pinfo("Tree: [BEFORE]");

    allocator.printTree();

    passert(allocator.free(block163), "Should be able to release Block 11.");

    pinfo("Tree: [AFTER]");

    allocator.printTree();

    passert(allocator.isFree(11), "Block 11 should be free now.");

    passert(allocator.isAllocated(12), "Block 12 should still be allocated.");

    passert(allocator.isSplit(5), "Block 5 should still be split.");

    passert(allocator.isFree(6), "Block 6 should still be free.");

    // Release Block 12 and they will be merged.
    pinfo("Prepare to release Block 12.");

    pinfo("Tree: [BEFORE]");

    allocator.printTree();

    passert(allocator.free(block164), "Should be able to release Block 12.");

    pinfo("Tree: [AFTER]");

    allocator.printTree();

    passert(allocator.isFree(2), "Block 2 should be free now.");

    // Block 2 is now free: 64 bytes free memory
    // Release Block 8 and Block 7 and they will be merged.
    pinfo("Prepare to release Block 7.");

    pinfo("Tree: [BEFORE]");

    allocator.printTree();

    passert(allocator.free(block161), "Should be able to release Block 7.");

    pinfo("Tree: [AFTER]");

    allocator.printTree();

    passert(allocator.isFree(7), "Block 7 should be free.");

    passert(allocator.isAllocated(8), "Block 8 should still be allocated.");

    passert(allocator.isSplit(3), "Block 3 should remain split.");

    passert(allocator.isAllocated(4), "Block 4 should still be allocated.");

    // Release Block 8
    pinfo("Prepare to release Block 8.");

    pinfo("Tree: [BEFORE]");

    allocator.printTree();

    passert(allocator.free(block162), "Should be able to release Block 8.");

    pinfo("Tree: [AFTER]");

    allocator.printTree();

    passert(allocator.isFree(3), "Block 3 should be free.");

    passert(allocator.isAllocated(4), "Block 4 should remain allocated.");

    passert(allocator.isSplit(1), "Block 1 should remain split.");

    // Release Block 4: Block 3 and 4 will be merged.
    // As a result, Block 1 is free and will be merged with its buddy block Block 2.
    pinfo("Prepare to release Block 4.");

    pinfo("Tree: [BEFORE]");

    allocator.printTree();

    passert(allocator.free(block321), "Should be able to release Block 4.");

    pinfo("Tree: [AFTER]");

    allocator.printTree();

    passert(allocator.isFree(0), "Block 0 should be free.");

    printf("==== TEST BINARY BUDDY ALLOCATOR FINISHED ====\n");
}
