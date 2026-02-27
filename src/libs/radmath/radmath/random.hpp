//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _RADMATH_RANDOM_HPP
#define _RADMATH_RANDOM_HPP

#include <radmath/buildconfig.hpp>

namespace RadicalMathLibrary
{

// A random number gererator, based on suggestions from George Marsaglia in sci.stat.math
// Uses two 16-bit multiply with carry rng's
// It passes randomness tests invented by people who care about this stuff WAAAAAY more 
// than I do (sick bastards like George Marsaglia), and has a period of about 2^60

// "Anyone who considers arithmetic means of producing random numbers is,
//  of course, in a state of sin" - John Von Neumann
class Randomizer
{
public:
    Randomizer(unsigned seed);
    Randomizer(unsigned seed1, unsigned seed2);
    ~Randomizer(); 

    void Seed(unsigned seed);
    void Seed(unsigned seed1, unsigned seed2);

    unsigned Int(void);                              // 0 <= value <= MAX_INT
    unsigned IntRanged(unsigned high);               // 0 <= value <= high
    unsigned IntRanged(unsigned low, unsigned high); // low <= value <= high
    float Float(void);                               // 0.0f <= value < 1.0f
    float FloatSigned(void);                         // -1.0f < value < 1.0f

private:
    unsigned upperBits;
    unsigned lowerBits;
}; 

}

namespace rmt = RadicalMathLibrary;

#endif
