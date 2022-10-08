//
//  MemoryAligners.hpp
//  MemoryAllocator
//
//  Created by FireWolf on 9/14/20.
//  Copyright © 2020 FireWolf. All rights reserved.
//

#ifndef MemoryAllocator_MemoryAligners_hpp
#define MemoryAllocator_MemoryAligners_hpp

#include <cstdint>

/// An aligner that always aligns allocation size to a constant
template <size_t Alignment>
struct ConstantAligner
{
    constexpr size_t operator()(size_t size)
    {
        size_t result = size / Alignment;

        result += (size % Alignment) ? 1 : 0;

        result *= Alignment;

        return result;
    }
};

/// A null aligner that does not align allocation
/// This is equivalent to ConstantAligner<1>
struct NullAligner
{
    constexpr size_t operator()(size_t size)
    {
        return size;
    }
};

/// An aligner that aligns allocation size to the next power of 2
struct NextPowerOfTwoAligner
{
    constexpr size_t operator()(size_t size)
    {
        // Reference:
        // https://jameshfisher.com/2018/03/30/round-up-power-2/
        // https://stackoverflow.com/questions/466204/rounding-up-to-next-power-of-2
        return size == 1 ? 1 : 1 << (32 - __builtin_clzl(size - 1));
    }
};

#endif /* MemoryAllocator_MemoryAligners_hpp */
