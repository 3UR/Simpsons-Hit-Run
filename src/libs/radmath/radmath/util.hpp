//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _RADMATH_UTIL_HPP
#define _RADMATH_UTIL_HPP

#include <radmath/buildconfig.hpp>
#include <cmath>
#include <float.h>
     
namespace RadicalMathLibrary
{

//conversion functions
inline float LtoF( const int x )
{
    return (float)(x);
}

inline int FtoL(const float x)
{
    /*
        Don't use.  Result is a round of x rather than a truncate.

        int r;
        _asm fld x
        _asm fistp r;
        return r;
    */
    
    return (int)(x);
}
    
// generic absolute value
template <class T> inline T Abs(const T x)
{
    return x >= 0 ? x : -x;
}

// absolute value for floats, - faster than generic version
inline float Fabs(const float a)
{
    unsigned x = *(unsigned*)&a & 0x7fffffff;  // strip of bit 31
    return *(float*)&x;
}

// returns -1 if negative, 0 if 0, 1 if positive
template <class T> inline T Sign(const T x)
{
    return x < 0 ? T(-1) : x > 0 ? T(1) : T(0);
}

// maximum of a, b
template <class T> inline T Max(const T a, const T b)
{
    return a < b ? b : a;
}

// minimum of a, b
template <class T> inline T Min(const T a, const T b)
{
    return a < b ? a : b;
}

// square of number
template <class T> inline T Sqr(const T x)
{
    return x*x;
}

// swaps x, y
template <class T> inline void Swap(T& x, T& y)
{
    T tmp = y;
    y = x;
    x = tmp;
}

// clamps x between a, and b
template <class T> inline T Clamp(const T x, const T a, const T b)
{
    return (x>=a && x<=b) ? x : (x>b) ? b : a;
}

// clamps between 0, 1 using a cubic - nice for ease-in/ease-out
inline float SmoothClamp(const float x)
{
    if(x < 0.0f) return 0.0f;
    if(x > 1.0f) return 1.0f;
    return (x*x * (3.0f - (x+x)));
}

// is x == n within tolerance e?
inline bool Epsilon(const float x, const float n, const float e = 0.000001f)
{
    return (x >= -e+n) && (x <= e+n);
}

// return previous power of two (i.e. largest power of two smaller than or equal to i)
inline int PrevPow2(int i)
{
    int shift = 0;
    while(i != 1)
    {
        i >>= 1;
        shift++;
    }

    return 1 << shift;
}

// true if i is a power of two, false otherwise
inline bool IsPow2(const int i)
{
    return (i & (i - 1)) == 0;
}


// return next power of two (i.e. smallest power of two greater than than or equal to i)
inline int NextPow2(int i)
{

    bool isPow2 = IsPow2(i);
    if (isPow2) return i;

    int  result = 1;
    while (i > 0)
    {
        result <<= 1;
        i >>= 1;
    }

    return result;
}


// take the log2 of a number that is already a power of 2
inline int Pow2Log2(const int x)
{
    switch(x)
    {
        case 1    : return 0;
        case 2    : return 1;
        case 4    : return 2;
        case 8    : return 3;
        case 16   : return 4;
        case 32   : return 5;
        case 64   : return 6;
        case 128  : return 7;
        case 256  : return 8;
        case 512  : return 9;
        case 1024 : return 10;
        case 2048 : return 11;
        case 4096 : return 12;
        case 8192 : return 13;
        default   : return 0;
    }
}

// inverse, square root, inverse square root, ceil, floor, exponential
inline float Inverse(const float a)                {  return 1.0f/a; }
inline float Sqrt(const float a) { return sqrtf(a); }
inline float ISqrt(const float a)                  {  return 1.0f/sqrtf(a); }
inline float Ceil(const float a)                   {  return ceilf(a); }
inline float Floor(const float a)                  {  return floorf(a); }
inline float Exp(const float x)                    {  return expf(x); }


// A hashing function that uses the "hashpjw"
// algorithm.  
// Taken from P3D
// which was in turn taken from Dragon book, p436
inline unsigned Hash(const char *x)
{
    unsigned h = 0;
    unsigned g;

    while(*x != 0)
    {
        h = (h << 4) + *x++;
        if ((g = h & 0xf0000000) != 0)
        {
            h = (h ^ (g >> 24)) ^ g;
        }
    }
    return h;
}

inline int IsNan( const float n )
{
    return std::isnan(n);
}

}  // RadicalMathLibrary namespace

namespace rmt = RadicalMathLibrary;

#endif  //UTIL_HPP