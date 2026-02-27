//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _RADMATH_VECTOR_HPP
#define _RADMATH_VECTOR_HPP

#include <radmath/buildconfig.hpp>
#include <radmath/util.hpp>
#include <radmath/trig.hpp>
#include <math.h>
#include <assert.h>

namespace RadicalMathLibrary
{
// classes defined in this header
class Vector2;
class Vector;
class Vector4;

// referenced classes from elsewhere
class Matrix;

// 3 element float vector (aka, a 3D posiotion)
class Vector
{
public:
    Vector() { /**/ }
    Vector(float nx, float ny, float nz ) { x=nx; y=ny; z=nz; }

    // Access operators (using ugly hack)
    float& operator[](int i) { assert((i<3)&&(i>=0)); return (&x)[i]; }     
    const float& operator[](int i) const { assert((i<3)&&(i>=0)); return (&x)[i]; }  

    // float cast
    operator float*() { return(&x); }

    // sets to [0 0 0]
    void Clear(void) { x=y=z=0.0f; }

    // assignment is handled by the default assignment operator.
    // We overload = once more so we can assign 3-vectors to fours
    Vector& operator=(const Vector4& vect);

    bool Equals(const Vector& a, float epsilon = 0.00001f) const;
    bool Equal(const Vector& a, float epsilon = 0.00001f) const { return Equals(a,epsilon); }

    // retrieving and setting the components
    void Get(float* C1, float* C2, float* C3) { *C1 = x; *C2 = y; *C3 = z; }
    void Set(float C1, float C2, float C3)    { x = C1; y = C2; z = C3;}

    // arithmetic operations.
    // Calling:
    // a.Add(b) -> a = (a+b)
    // a.Add(b,c) -> a = (b+c)
    // and likewise for the Sub

    inline void Add(const Vector& vect)
    {
        x += vect.x;
        y += vect.y;
        z += vect.z;
    }

    inline void Add(const Vector& vect1, const Vector& vect2)
    {
        x = vect1.x + vect2.x;
        y = vect1.y + vect2.y;
        z = vect1.z + vect2.z;
    }

    inline void Sub(const Vector& vect)
    {
        x -= vect.x;
        y -= vect.y;
        z -= vect.z;
    }

    inline void Sub(const Vector& vect1, const Vector& vect2)
    {
        x = vect1.x - vect2.x;
        y = vect1.y - vect2.y;
        z = vect1.z - vect2.z;
    }

#if 0
    // vector scaling
    inline void Scale(float scaleFactor)
    {
        asm __volatile__(R"(
            mul.s %0, %0, %3
            mul.s %1, %1, %3
            mul.s %2, %2, %3
            )"
            : "+f"(x),"+f"(y),"+f"(z)
            : "f"(scaleFactor));
    }

    inline void Scale(float scaleFactorX, float scaleFactorY, float scaleFactorZ)
    {
        asm __volatile__(R"(
            mul.s %0, %0, %3
            mul.s %1, %1, %4
            mul.s %2, %2, %5
            )"
            : "+f"(x),"+f"(y),"+f"(z)
            : "f"(scaleFactorX),"f"(scaleFactorY),"f"(scaleFactorZ) );
    }

#else
    // vector scaling
    void Scale(float scaleFactor)
    {
        x *= scaleFactor;
        y *= scaleFactor;
        z *= scaleFactor;
    }

    void Scale(float scaleFactorX, float scaleFactorY, float scaleFactorZ)
    {
        x *= scaleFactorX;
        y *= scaleFactorY;
        z *= scaleFactorZ;
    }

#endif

    void Scale(float scaleFactor, const Vector& scaleMe)
    {
        x = scaleMe.x * scaleFactor;
        y = scaleMe.y * scaleFactor;
        z = scaleMe.z * scaleFactor;
    }

    void ScaleAdd(float scaleFactor, const Vector& scaleMe)
    {
        x += scaleMe.x * scaleFactor;
        y += scaleMe.y * scaleFactor;
        z += scaleMe.z * scaleFactor;
    }

    void ScaleAdd(const Vector& vect, float scaleFactor, const Vector& scaleMe)
    {
        x = vect.x + scaleMe.x * scaleFactor;
        y = vect.y + scaleMe.y * scaleFactor;
        z = vect.z + scaleMe.z * scaleFactor;
    }

    // dot product
    float DotProduct(const Vector& vect) const
    {
        return ((x * vect.x) + (y * vect.y) + (z * vect.z));
    }

    float Dot( const Vector& vect ) const
    {
        return DotProduct( vect );
    }

    // cross products
    void CrossProduct(const Vector& vect);
    void CrossProduct(const Vector& vect1, const Vector& vect2);
    
    // create a unit vector
    void Normalize(void);
    void Normalize(const Vector& vect);

    // test for a zero length vector, normalize the vector and return the original length
    float NormalizeSafe(void)
    {
        const float smallValue = 0.0000001f;
        float mag = DotProduct(*this);
        if (mag > smallValue)
        {
            mag = Sqrt(mag);
            Scale(1.0f/mag);
        }
        else
        {
            mag = x = y = z = 0;
        }
        return mag;
    }

    // magnitude of the vector
    float Magnitude(void) const
    {
        return Sqrt(x*x+y*y+z*z);
    }
    float Length(void) const {  return Magnitude();}

    // squared magnitude of the vector
    float MagnitudeSqr(void) const
    {
        return (x*x) + (y*y) + (z*z);
    }

    // transform a vector by a matrix
    void Transform(const Matrix& matrix);
    void Transform(const Vector&, const Matrix& matrix);
    
    void Rotate(const Matrix& matrix);
    void Rotate(const Vector&, const Matrix& matrix);

    // make an YZX Euler angle Vector from a matrix
    void ConvertToEulerYZX(const Matrix& m);

    void Interpolate(const Vector& b, float delta)
    {
        float invDelta = 1.0f - delta;
        x = x*invDelta + b.x*delta;
        y = y*invDelta + b.y*delta;
        z = z*invDelta + b.z*delta;
    }

    void Interpolate(const Vector& a, const Vector& b, float delta) 
    {
        float invDelta = 1.0f - delta;
        x = a.x*invDelta + b.x*delta;
        y = a.y*invDelta + b.y*delta;
        z = a.z*invDelta + b.z*delta;
    }

    // Unary operators
    Vector operator-() const { return(Vector(-x,-y,-z)); } 
    Vector operator+() const { return *this; } 

    Vector operator+( const Vector& right ) const
    {
        return Vector( x + right.x, y + right.y, z + right.z );
    }

    Vector& operator+=( const Vector& right )
    {
        x += right.x;
        y += right.y;
        z += right.z;
        return *this;
    }

    Vector& operator-=( const Vector& right )
    {
        x -= right.x;
        y -= right.y;
        z -= right.z;
        return *this;
    }

    Vector operator-( const Vector& right ) const
    {
        return Vector( x - right.x, y - right.y, z - right.z );
    }

    bool operator==( const Vector& right ) const
    {
        return Equals( right );
    }

    bool operator!=( const Vector& right ) const
    {
        return !( Equals( right ) );
    }

    Vector& operator *=( const float scaleFactor )
    {
        Scale( scaleFactor );
        return *this;
    }

    Vector& operator /=( const float scaleFactor )
    {
        Scale( 1.0f / scaleFactor );
        return *this;
    }


    float x,y,z;
}; // Vector


// homogeneous 4 element vector
// Note!  The w component is assumed to be 1.0 for all operations except for:
// Equals(), DivideByW(), Transform()
class Vector4 : public Vector
{
public:
    Vector4() { /* */ }
    Vector4(float nx, float ny, float nz, float nw = 1.0f) { x=nx; y=ny; z=nz; w=nw; }

    void Clear(void)    { x=y=z=0.0f; w=1.0f; }

    const Vector4& operator=(const Vector& vector) { x = vector.x; y = vector.y; z = vector.z; w = 1.0f; return *this; }
    bool Equals(const Vector4& a, float epsilon = 0.00001f) const;

    // retrie and set the components
    void Get(float* C1, float* C2, float* C3, float* C4) const { *C1 = x; *C2 = y; *C3 = z; *C4 = w; }
    void Set(float C1, float C2, float C3, float C4 = 1.0f)    { x = C1; y = C2; z = C3; w = C4; } 

    void DivideByW()
    {
        float oow = 1.0f / w;
        x *= oow;
        y *= oow;
        z *= oow;
        w = 1.0f;
    }
    
    // transform a vector by a matrix
    void Transform(const Matrix& matrix);
    void Transform(const Vector4&, const Matrix& matrix);

    void Interpolate(const Vector4& a, const Vector4& b, float delta)
    {
        float invDelta = 1.0f - delta;
        x = a.x*invDelta + b.x*delta;
        y = a.y*invDelta + b.y*delta;
        z = a.z*invDelta + b.z*delta;
        w = a.w*invDelta + b.w*delta;
    }

    float w;
}; // Vector4


// 2 element vector
class Vector2
{
public:
    Vector2()                    { /* */ }
    Vector2(float U, float V)    { u=U; v=V; }
    void Clear(void)             { u=0.0f, v=0.0f;}
    void Set(float U, float V)   { u=U; v=V; }
    void Get(float* U, float* V) { *U=u; *V=v; }

    union
    {
        float u;
        float x;
    };

    union
    {
        float v;
        float y;
    };
};

// a bulk vector normalization function
inline void Normalize(int n, const Vector* src, Vector* dest)
{
    for(int i = 0; i < n; i++)
    {
        dest[n] = src[n];
        dest[n].Normalize();
    }
}

}

namespace rmt = RadicalMathLibrary;

inline rmt::Vector operator*(float f, const rmt::Vector &a) { return rmt::Vector(f*a.x, f*a.y, f*a.z); }
inline rmt::Vector operator*(const rmt::Vector& a, float f) { return rmt::Vector(f*a.x, f*a.y, f*a.z); }
inline rmt::Vector operator/(float f, const rmt::Vector& a) { return rmt::Vector(f/a.x, f/a.y, f/a.z); }
inline rmt::Vector operator/(const rmt::Vector& a, float f) { return rmt::Vector(a.x/f, a.y/f, a.z/f); }

inline rmt::Vector operator*(const rmt::Vector& a, const rmt::Vector& b) { return rmt::Vector(a.x*b.x, a.y*b.y, a.z*b.z); }
inline rmt::Vector operator/(const rmt::Vector& a, const rmt::Vector& b) { return rmt::Vector(a.x/b.x, a.y/b.y, a.z/b.z); }

#endif