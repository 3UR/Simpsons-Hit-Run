//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _RADMATH_QUATERNION_HPP
#define _RADMATH_QUATERNION_HPP

#include <radmath/buildconfig.hpp>

namespace RadicalMathLibrary
{

// forward declarations
class Matrix;
class Vector;

//-------------------------------------------------------------------
// Quaternions
//-------------------------------------------------------------------
class Quaternion
{
public:
    Quaternion() { /**/ }
    Quaternion(float W, float X, float Y, float Z) : w(W), x(X), y(Y), z(Z) {}

    void Identity(void);

    void Add(const Quaternion& q);
    void Add(const Quaternion& q1, const Quaternion& q2); 
    void Sub(const Quaternion& q);
    void Sub(const Quaternion& q1, const Quaternion& q2); 
    void Mult(const Quaternion& q);
    void Mult(const Quaternion& q1, const Quaternion& q2);

    float DotProduct(const Quaternion& q) const;
    void Inverse();
    void Normalize();

    void BuildFromAngleAxis(const float a, const Vector& axis);
    void BuildFromAngleX(const float a);
    void BuildFromAngleY(const float a);
    void BuildFromAngleZ(const float a);
    void BuildFromAngleXYZ(const float x, const float y, const float z);
    void BuildFromAngleYZX(const float x, const float y, const float z);
    void BuildFromMatrix(const Matrix& m);
    
    void ConvertToMatrix(Matrix* m) const;

    // modifies this so it is on same side of hypersphere as q
    void MakeClosest(const Quaternion& q);

    // spherical-linear interpolation
    void Slerp(const Quaternion& q1, const Quaternion& q2, const float t);
    void Slerp(const Quaternion& q, const float t) { Slerp(*this, q, t); }
    
    float w, x, y, z;
};

}

namespace rmt = RadicalMathLibrary;

#endif
