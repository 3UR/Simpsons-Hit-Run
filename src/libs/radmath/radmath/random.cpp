//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <radmath/random.hpp>
#include <radmath/util.hpp>

namespace RadicalMathLibrary
{

static inline unsigned int nextRandomValue(unsigned int &u, unsigned int &l)
{
    return ((u = 36969 * (u & 65535) + (u >> 16)) << 16) + 
             ((l = 18000 * (l & 65535) + (l >> 16)) & 65535);
}

Randomizer::Randomizer(unsigned seed)
{
    Seed(seed);
}

Randomizer::Randomizer(unsigned seed1, unsigned seed2)
{
    Seed(seed1, seed2);
}

Randomizer::~Randomizer()
{
}

void Randomizer::Seed(unsigned seed)
{
    if(seed == 0)
        seed = 0xfc7d2ba2;

    upperBits = seed & 0x55555555;
    lowerBits = seed & 0xaaaaaaaa;

    Int();
    Int();
}


void Randomizer::Seed(unsigned seed1, unsigned seed2)
{
    if(seed1 == 0)
        seed1 = 0xfc7d2ba2;

    if(seed2 == 0)
        seed2 = 0x534ffd12;

    upperBits = seed1;
    lowerBits = seed2;
}

unsigned int Randomizer::Int(void)
{
    return nextRandomValue(upperBits, lowerBits);
};

unsigned int Randomizer::IntRanged(unsigned int range)
{
    return FtoL(Float() * (range + 1));
};

unsigned Randomizer::IntRanged(unsigned low, unsigned high)
{
    return low + IntRanged(high - low);
}

float Randomizer::Float(void)
{
#if 1 // bit packing version
    float result;

    // stuff low 23 bits into mantissa - exponent = 127
    *(unsigned*)(&result) = 0x3f800000 | (nextRandomValue(upperBits, lowerBits) & 0x7fffff);
    return result - 1.0f;

#else // "safe" version
    return float(nextRandomValue(upperBits, lowerBits) * 2.328306e-10);
#endif
}

float Randomizer::FloatSigned(void)
{
#if 1 // bit packing version
    float result;
    unsigned rnd = nextRandomValue(upperBits, lowerBits);

    // stuff low 23 bits into mantissa and bit 31 into sign field - exponent = 127
    *(unsigned*)(&result) = 0x3f800000 | (rnd & 0x7fffff);
    result -= 1.0f;
    *(unsigned*)(&result) |= (0x80000000 & rnd);

    return result;

#else // "safe" version
    return float(nextRandomValue(upperBits, lowerBits) * 4.656613e-10);
#endif
}

}

