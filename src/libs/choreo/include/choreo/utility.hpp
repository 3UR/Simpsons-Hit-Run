/*
 * choreo/utility.hpp
 */

#ifndef CHOREO_UTILITY_HPP
#define CHOREO_UTILITY_HPP


#include <stdio.h>
#include <stdarg.h>
#include <radmath/radmath.hpp>


namespace choreo
{


//----------------------------------------------------------------------
// Blending function prototypes
//----------------------------------------------------------------------

inline float BlendFloat(float in0,
                        float in1,
                        float alpha);
inline void BlendVector(const rmt::Vector& in0,
                        const rmt::Vector& in1,
                        float alpha,
                        rmt::Vector& out);
inline void BlendQuaternion(const rmt::Quaternion& in0,
                            const rmt::Quaternion& in1,
                            float alpha,
                            rmt::Quaternion& out);
void BlendMatrix(const rmt::Matrix& in0,
                 const rmt::Matrix& in1,
                 float alpha,
                 rmt::Matrix& out);

// linearly increments (current) towards (target) at speed of (rate),
// but does not overshoot
inline float ApproachFloat(float current,
                           float target,
                           float rate);


//----------------------------------------------------------------------
// Orientation function prototypes
//----------------------------------------------------------------------

// returns the world angle in the range [0, 2PI)
// 0 radians begins at (0,0,-1)
// pi/2 radians is at (-1,0,0)
float GetWorldAngle(float x, float z);

// returns smallest arc in range [-PI, PI)
float GetSmallestArc(float start, float stop);

// rotates vector v by angle radians about the y-axis
void RotateYVector(float angle, rmt::Vector& v);

// fills rotation part of matrix with rotation that
// will rotate vector s to t
void MatrixFillRotateVectorToVector(rmt::Matrix& m, const rmt::Vector& s, const rmt::Vector& t);


//----------------------------------------------------------------------
// Phase/frame function prototypes
//----------------------------------------------------------------------

inline float Wrap(float min, float max, float val);
inline float UnitWrap(float unit);


//----------------------------------------------------------------------
// String manip function prototypes
//----------------------------------------------------------------------

int VStrPrintf(char* str, int len, const char* fmt, va_list ap);
int StrPrintf(char* str, int len, const char* fmt, ...);
char* StrSet(char* str, int len, const char* orig);
char* StrCat(char* str, int len, const char* orig);
char* StrEscapify(char* str, int len, const char* orig);


//----------------------------------------------------------------------
// Blending function implementations
//----------------------------------------------------------------------

inline float BlendFloat(float in0,
                        float in1,
                        float alpha)
{
    return (in0 * (1.0f - alpha)) + (in1 * alpha);
}

inline void BlendVector(const rmt::Vector& in0,
                        const rmt::Vector& in1,
                        float alpha,
                        rmt::Vector& out)
{
    out = in1;
    out.Scale(alpha);
    
    rmt::Vector tmp = in0;
    tmp.Scale(1.0f - alpha);
    
    out.Add(tmp);
}

inline void BlendQuaternion(const rmt::Quaternion& in0,
                            const rmt::Quaternion& in1,
                            float alpha,
                            rmt::Quaternion& out)
{
    out.Slerp(in0, in1, alpha);
}

inline float ApproachFloat(float current,
                           float target,
                           float rate)
{
    float result;
    float diff = current - target;
    
    if (rmt::Fabs(diff) < rate)
    {
        result = target; 
    }
    else if (diff < rate)
    {
        result = current + rate;
    }
    else
    {
        result = current - rate;
    }
    
    return result;
}


//----------------------------------------------------------------------
// Phase/frame function implementations
//----------------------------------------------------------------------

inline float Wrap(float min, float max, float val)
{
    if ((val < min) || (val >= max))
    {
        if (rmt::Epsilon(min, max))
            return min;

        float range = max - min;
        float iVal = -rmt::Floor((val - min) / range);
        val += (iVal * range);
        
        if (rmt::Epsilon(val, max) ||
            rmt::Epsilon(val, min))
        {
            val = min;
        }
    }
    
    return val;
}

inline float UnitWrap(float unit)
{
    if ((unit < 0.0f) || (unit >= 1.0f))
    {
        float iUnit = -rmt::Floor(unit);
        unit += iUnit;
        
        if (rmt::Epsilon(unit, 1.0f) ||
            rmt::Epsilon(unit, 0.0f))
        {
            unit = 0.0f;
        }
    }
    
    return unit;
}


} // namespace choreo


#endif
