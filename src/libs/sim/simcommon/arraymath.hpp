#ifndef ARRAYMATH_HPP
#define ARRAYMATH_HPP

/*
This file contains array and math math functions on arrays of float.
These arrays are blocks of larger arrays and tables which makes use of 
rmt::Vector and RMT::Matrix class inapropriate.
*/

#include "radmath/radmath.hpp"

#include "raddebug.hpp"
#include <string.h>

namespace sim
{

#define USE_FAST_MATH4
#define ARRAY_MATH_VERY_SMALL 0.00000001f

#ifdef USE_FAST_MATH4
inline void ScaleVector4(float* res, const float* v, float s)
{
    res[0] = s * v[0];
    res[1] = s * v[1];
    res[2] = s * v[2];
    res[3] = s * v[3];
}

inline void AddVector4(float* res, const float* v_1, const float* v_2)
{
    res[0] = v_1[0] + v_2[0];
    res[1] = v_1[1] + v_2[1];
    res[2] = v_1[2] + v_2[2];
    res[3] = v_1[3] + v_2[3];
}

inline void SubVector4(float* res, const float* v_1, const float* v_2)
{
    res[0] = v_1[0] - v_2[0];
    res[1] = v_1[1] - v_2[1];
    res[2] = v_1[2] - v_2[2];
    res[3] = v_1[3] - v_2[3];
}

inline void AddScaleVector4(float* res, const float* v_1, const float* v_2, float s)
{
    res[0] = v_1[0] + s * v_2[0];
    res[1] = v_1[1] + s * v_2[1];
    res[2] = v_1[2] + s * v_2[2];
    res[3] = v_1[3] + s * v_2[3];
}

inline float DotVector4(const float* v_1, const float* v_2)
{
    return v_1[0]*v_2[0] + v_1[1]*v_2[1] + v_1[2]*v_2[2] + v_1[3]*v_2[3];
}
#endif // USE_FAST_MATH4



inline float DotVector(const float a[], const float b[]) 
{
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

inline void SetVector(float v[], const float x, const float y, const float z)
{
    v[0] = x; v[1] = y; v[2] = z;
}

void CrossVector(const float v_1[], const float v_2[], float r[]);

inline void ZeroVector(float f[]) 
{ 
    f[0] = f[1] = f[2] = 0.0f; 
}

inline void EqualVector(const float v[], float r[])
{
    r[0] = v[0]; r[1] = v[1]; r[2] = v[2];
}

inline float LenVector(const float f[]) 
{ 
    return rmt::Sqrt(DotVector(f, f)); 
}

float NormalizeVector(float f[]);

inline void SubVector(const float a[], const float b[], float r[]) 
{ 
    r[0] = a[0] - b[0];
    r[1] = a[1] - b[1];
    r[2] = a[2] - b[2];
}

inline void AddVector(const float a[], const float b[], float r[]) 
{ 
    r[0] = a[0] + b[0];
    r[1] = a[1] + b[1];
    r[2] = a[2] + b[2];
}

inline void ScaleVector(const float a[], const float s, float r[]) 
{ 
    r[0] = a[0] * s;
    r[1] = a[1] * s;
    r[2] = a[2] * s;
}

inline void AddScaleVector(const float a[], const float b[], const float s, float r[])
{
    r[0] = a[0] + b[0] * s;
    r[1] = a[1] + b[1] * s;
    r[2] = a[2] + b[2] * s;
}

inline void AddLinearVector(const float a[], const float sa, const float b[], const float sb, float r[])
{
    r[0] = a[0] * sa + b[0] * sb;
    r[1] = a[1] * sa + b[1] * sb;
    r[2] = a[2] * sa + b[2] * sb;
}

inline void AddAddLinearVector(const float a[], const float sa, const float b[], const float sb, float r[])
{
    r[0] += a[0] * sa + b[0] * sb;
    r[1] += a[1] * sa + b[1] * sb;
    r[2] += a[2] * sa + b[2] * sb;
}

void ComputeTriNormal(const float v_1[], const float v_2[], const float v3[], float n[]);

void AddMatrix(const float m1[][3], const float m2[][3], float mt[][3]);
void SubMatrix(const float m1[][3], const float m2[][3], float mt[][3]);
void AddScaleMatrix(const float m1[][3], const float m2[][3], float s, float mt[][3]);
void AddLinearMatrix(const float m1[][3], float s1, const float m2[][3], float s2, float mt[][3]);
void ScaleMatrix(const float m1[][3], const float s, float mt[][3]);
void EqualMatrix(const float m1[][3], float mt[][3]);
void ZeroMatrix(float k[][3]);
void MultMatrix( const float m1[][3], const float m2[][3], float mt[][3]);
void TransposeMatrix( float m[][3] );

void MultVectorVectorT(const float v[], const float vt[], float m[][3]);
void MultVectorVectorT(const float v[], float m[][3]);

void TransformVector(const float m[][3], float r[]);
void TransformVector(const float m[][3], const float v[], float r[]);
void TransformVectorAdd(const float k[][3], const float a[], float res[]);

float TriangleArea(const float p1[], const float p2[], const float p3[]);

inline void SetIdentity(float k[][3] ) 
{
    k[0][0] = k[1][1] = k[2][2] = 1.0f;
    k[0][1] = k[0][2] = k[1][0] = k[1][2] = k[2][0] = k[2][1] = 0.0f;
}

inline bool IsMatrixSymetric( float k[][3], float in_eps=ARRAY_MATH_VERY_SMALL ) 
{
    if( rmt::Fabs( k[0][1] - k[1][0] ) > in_eps )
        return false;
    if( rmt::Fabs( k[0][2] - k[2][0] ) > in_eps )
        return false;
    if( rmt::Fabs( k[1][2] - k[2][1] ) > in_eps )
        return false;
    return true;
}

bool AreMatricesEqual(const float j[][3], const float k[][3], float in_eps=ARRAY_MATH_VERY_SMALL);
bool AreMatricesEqualTranspose(const float j[][3], const float k[][3], float in_eps=ARRAY_MATH_VERY_SMALL);
void RemoveComponent(const float dir[], float v[]);
float DotVectorN(const float a[], const float b[], int size);


#ifdef USE_FAST_MATH4
void MakeMultipleOf4(int& vecSize);

inline void AddScaleVectorN(float* res, const float* v_1, const float* v_2, const float f, int size)
{
    rAssert(size % 4 == 0);

    for(int i=0; i<size; i+=4) 
        AddScaleVector4(&res[i], &v_1[i], &v_2[i], f);
}

inline void AddVectorN(float* r, const float* b, const float* c, int size)
{
    //rAssert(size % 4 == 0);
    for(int i = 0; i<size; i+=4)
        AddVector4(&r[i], &b[i], &c[i]);
}

inline void SubVectorN(float* r, const float* b, const float* c, int size)
{
    //rAssert(size % 4 == 0);
    for(int i = 0; i<size; i+=4)
        SubVector4(&r[i], &b[i], &c[i]);
}

#else
inline void AddScaleVectorN(float* res, const float* v_1, const float* v_2, const float f, int size)
{
    for(int i=0; i<size; i++) 
        res[i] = v_1[i] + f * v_2[i];
}

inline void AddVectorN(float* r, const float* b, const float* c, int size)
{
    for(int i = 0; i<size; i++)
        r[i] = b[i] + c[i];
}

inline void SubVectorN(float* r, const float* b, const float* c, int size)
{
    for(int i = 0; i<size; i++)
        r[i] = b[i] - c[i];
}

#endif // USE_FAST_MATH4

inline void ZeroVectorN(float v[], int size)
{
    memset(v, 0, size*sizeof(float));
}

inline void EqualVectorN(float res[], const float a[], int size)
{
    memcpy(res, a, size*sizeof(float));
}

} // sim


#endif // ARRAYMATH_HPP
