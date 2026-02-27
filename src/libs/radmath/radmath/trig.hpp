//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _RADMATH_TRIG_HPP
#define _RADMATH_TRIG_HPP

#include <radmath/buildconfig.hpp>
#include <math.h>
#include <radmath/util.hpp>

namespace RadicalMathLibrary
{

// trig stuff
const float PI     = 3.14159265358979323846f;
const float PI_2   = PI * 2.0f;
const float PI_BY2 = PI / 2.0f;
const float PI_BY4 = PI / 4.0f;
const float PI_BY8 = PI / 8.0f;
const float OO_PI  = 1.0f / PI;

// convert to and from old style unsigned short angles (P3DANGLE)
template <class T> inline float AngleToRadian(T a)
{
    return float(a) * (PI_2 / 65536.0f);
}

inline unsigned short RadianToAngle(const float a)
{
    return (unsigned short)(a * (65536.0f / PI_2));
}

// convert from/to radians and degrees
inline float RadianToDeg(const float a)
{
    return a * (360.0f / PI_2);
}

inline float DegToRadian(const float a)
{
    return a * (PI_2 / 360.0f);
}

    inline float Sin(const float angle)  {  return sinf(angle); }
    inline float Cos(const float angle)  {  return cosf(angle); }
    inline void  SinCos(const float angle, float* s, float* c)
    {
        *s = sinf(angle);
        *c = cosf(angle);
    }

inline float Tan(const float angle)  {  return tanf(angle); }
inline float ASin(const float angle) {  return asinf(angle); }
inline float ACos(const float angle) {  return acosf(angle); }
inline float ATan(const float angle) {  return atanf(angle); }
inline float ATan2(const float y, const float x )  {  return atan2f(y, x); }

// This  arctan function basically takes an X/Y pair  and gives you the angle 
// between the positive X axis and the vector from the origin to that point
float ATanCoords(float x, float y);

// Polar<->Cartesian coordinate support.
void CartesianToPolar(float x, const float y, float* magnitude, float* angle );
void PolarToCartesian(float magnitude, float angle,  float* x, float* y );

// Spherical<->Cartesian support. Same details as the Polar stuff
// angledelta is rotation around Z axis (on XY plane)
// anglephi is elevation (above/below the XY plane),
void CartesianToSpherical(float x, float y, float z, float* magnitude, float* angletheta, float* anglephi );
void SphericalToCartesian(float magnitude, float angletheta, float anglephi, float* x, float* y, float* z );

}  // RadicalMathLibrary namespace

namespace rmt = RadicalMathLibrary;

#endif  //TRIG_HPP