/*===========================================================================
    quaternion.cpp
    22-Oct-98 Created by Neall, Nenad
    20-Nov-99 Put into RadMath

    Copyright (c)1999, 2000 Radical Entertainment, Inc.
    All rights reserved.

    Based on:
     - Ken Shoemake "Animating Rotation with Quaternions Curves"; SIGRAPH 1985, p.245
     - A. Watt & M. Watt "Advanced Animation and Rendering Techniques"; p.360
     - Graphics Gems IV p.175

    IMPORTANT - IT IS ASSUMED THAT ALL THE OPERATIONS ARE PERFORMED ON UNIT QUATERNIONS
===========================================================================*/

#include <radmath/quaternion.hpp>
#include <radmath/matrix.hpp>

namespace RadicalMathLibrary
{

const float quatEpsilon = (1.0f/360.0f); // one degree of slop allowed


void Quaternion::Identity(void)
{
    w = 1.0f;
    x = y = z = 0;
}

void Quaternion::Add(const Quaternion& q)
{
    w += q.w;
    x += q.x;
    y += q.y;
    z += q.z;
}

void Quaternion::Add(const Quaternion& q1, const Quaternion& q2)
{
    w = q1.w + q2.w;
    x = q1.x + q2.x;
    y = q1.y + q2.y;
    z = q1.z + q2.z;
}

void Quaternion::Sub(const Quaternion& q)
{
    w -= q.w;
    x -= q.x;
    y -= q.y;
    z -= q.z;

}

void Quaternion::Sub(const Quaternion& q1, const Quaternion& q2)
{
    w = q1.w - q2.w;
    x = q1.x - q2.x;
    y = q1.y - q2.y;
    z = q1.z - q2.z;

}

void Quaternion::Mult(const Quaternion& q)
{
    Quaternion d;
    // scalar part = sL * sR - dotProduct(vL, vR)
    d.w = w*q.w - x*q.x - y*q.y - z*q.z;
    // vector part = sL * vR + sR * vL + crossProduct(vL, vR)
    d.x = w*q.x + x*q.w + y*q.z - z*q.y;
    d.y = w*q.y + y*q.w + z*q.x - x*q.z;
    d.z = w*q.z + z*q.w + x*q.y - y*q.x;
    w = d.w;  x = d.x;  y = d.y;  z = d.z;

}

void Quaternion::Mult(const Quaternion& q1, const Quaternion &q2)
{
    // scalar part = sL * sR - dotProduct(vL, vR)
    w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z;
    // vector part = sL * vR + sR * vL + crossProduct(vL, vR)
    x = q1.w*q2.x + q1.x*q2.w + q1.y*q2.z - q1.z*q2.y;
    y = q1.w*q2.y + q1.y*q2.w + q1.z*q2.x - q1.x*q2.z;
    z = q1.w*q2.z + q1.z*q2.w + q1.x*q2.y - q1.y*q2.x;

}

float Quaternion::DotProduct(const Quaternion& q) const
{
    return w*q.w + x*q.x + y*q.y + z*q.z;
}

// ASSUMES q is a unit quaternion
void Quaternion::Inverse()
{
    x = -x;
    y = -y;
    z = -z;

}

void Quaternion::Normalize()
{
    float ooMag = ISqrt(w*w + x*x + y*y + z*z);
    w = w * ooMag;
    x = x * ooMag;
    y = y * ooMag;
    z = z * ooMag;

}

// q = [cos(a/2), (sin(a/2), 0, 0)]
void Quaternion::BuildFromAngleX(const float a)
{
    SinCos(a*0.5f, &x, &w);
    y = 0.0f;
    z = 0.0f;

}

// q = [cos(a/2), (0, sin(a/2), 0)]
void Quaternion::BuildFromAngleY(const float a)
{
    SinCos(a*0.5f, &y, &w);
    x = 0.0f;
    z = 0.0f;

}

// q = [cos(a/2), (0, 0, sin(a/2))]
void Quaternion::BuildFromAngleZ(const float a)
{
    SinCos(a*0.5f, &z, &w);
    x = 0.0f;
    y = 0.0f;

}

// q = [cos(a/2), (x*sin(a/2), y*sin(a/2), z*sin(a/2))]
void Quaternion::BuildFromAngleAxis(const float a, const Vector& axis)
{
    float sa;
    SinCos(a*0.5f, &sa, &w);
    x = sa*axis.x;
    y = sa*axis.y;
    z = sa*axis.z;

}

void Quaternion::BuildFromAngleXYZ(const float ax, const float ay, const float az)
{
    float cx, sx, cy, sy, cz, sz;

    SinCos(ax*0.5f, &sx, &cx);
    SinCos(ay*0.5f, &sy, &cy);
    SinCos(az*0.5f, &sz, &cz);

    // compute x*y
    float cxsy = cx*sy;
    float sxsy = sx*sy;
    float sxcy = sx*cy;
    float cxcy = cx*cy;
    // compute (x*y)*z
    w = cxcy*cz + sxsy*sz;
    x = sxcy*cz - cxsy*sz;
    y = cxsy*cz + sxcy*sz;
    z = cxcy*sz - sxsy*cz;

}

void Quaternion::BuildFromAngleYZX(const float ax, const float ay, const float az)
{
    float cx, sx, cy, sy, cz, sz;

    SinCos(ax*0.5f, &sx, &cx);
    SinCos(ay*0.5f, &sy, &cy);
    SinCos(az*0.5f, &sz, &cz);
    // compute y*z
    float cycz = cy*cz;
    float sysz = sy*sz;
    float sycz = sy*cz;
    float cysz = cy*sz;
    // compute (y*z)*x
    w = cycz*cx + sysz*sx;
    x = cycz*sx - sysz*cx;
    y = sycz*cx - cysz*sx;
    z = cysz*cx + sycz*sx;

}

//-------------------------------------------------------------------
// The books on which this function is based give rotation matrix for right-handed
// rotation in right-handed coordinate system.
// Well this is nice but we actually need Transpose of this matrix, so this function is
// a bit different from one you'll find in Watt&Watt.

void Quaternion::ConvertToMatrix(Matrix* m) const
{
    //assumes unit quaternion!!
    float xs = x + x;
    float ys = y + y;
    float zs = z + z;

    float wx = w * xs;
    float wy = w * ys;
    float wz = w * zs;

    float xx = x * xs;
    float xy = x * ys;
    float xz = x * zs;

    float yy = y * ys;
    float yz = z * ys;
    float zz = z * zs;

    m->m[0][0] = 1.0f - (yy +zz);
    m->m[1][0] = xy - wz;
    m->m[2][0] = xz + wy;

    m->m[0][1] = xy + wz;
    m->m[1][1] = 1.0f - (xx +zz);
    m->m[2][1] = yz - wx;

    m->m[0][2] = xz - wy;
    m->m[1][2] = yz + wx;
    m->m[2][2] = 1.0f - (xx + yy);
}

void Quaternion::BuildFromMatrix(const Matrix& m)
{
    int nxt[3] = {1,2,0};
    float q[4];
    float tr,s;
    int i,j,k;
    tr = m.m[0][0] + m.m[1][1] + m.m[2][2];
    if (tr > 0.0f)
    {
        s = sqrtf(tr+1.0f);
        w = -s * 0.5f;
        if (s!=0.0f)
        {
            s = 0.5f / s;
        }
        x = (m.m[2][1] - m.m[1][2]) * s;
        y = (m.m[0][2] - m.m[2][0]) * s;
        z = (m.m[1][0] - m.m[0][1]) * s;
    } 
    else 
    {
        i = 0;
        if (m.m[1][1] > m.m[0][0]) i = 1;
        if (m.m[2][2] > m.m[i][i]) i = 2;
        j = nxt[i];
        k = nxt[j];
        s = sqrtf( (m.m[i][i] - (m.m[j][j]+m.m[k][k])) + 1.0f );      

        q[i] = s * 0.5f;
        if (s!=0.0f)
        {
            s = 0.5f / s;
        }
        q[3] = (m.m[k][j] - m.m[j][k]) * s;
        q[j] = (m.m[j][i] + m.m[i][j]) * s;
        q[k] = (m.m[k][i] + m.m[i][k]) * s;

        w = -q[3];
        x = q[0];
        y = q[1];
        z = q[2];
    }  

}

void Quaternion::MakeClosest(const Quaternion& q)
{
    Quaternion qdiff1;
    qdiff1.Sub(q, *this);
    float diff1 = qdiff1.DotProduct(qdiff1);

    Quaternion qdiff2;
    qdiff2.Sub(q, Quaternion(-this->x, -this->y, -this->z, -this->w));
    float diff2 = qdiff2.DotProduct(qdiff2);

    if (diff2 < diff1)
    {
        this->x = -this->x;
        this->y = -this->y;
        this->z = -this->z;
        this->w = -this->w;
    }

}

// ASSUMES qS and qE are unit quaternions.
// code is based on listing 15.3 on page 364 in Watt & Watt
void Quaternion::Slerp(const Quaternion& q1, const Quaternion& q2, const float t)
{
    float scale_qS;
    float scale_qE = 1.0f;
    
    // determime which arc is shorter (qS, qE) or arc(qS, -qE)
    float cos_omega = q1.w*q2.w + q1.x*q2.x + q1.y*q2.y + q1.z*q2.z;
    if(cos_omega < 0.0f)
    {
        scale_qE = -1.0f;
        cos_omega = -cos_omega;
    }

    // if angle between qS and qE is small enough
    // then do just linear interpolation
    if((1.0f - cos_omega) < quatEpsilon)
    {
        // angle between qS and qE is small enough, just do linear interpolation
        // calculate scaling factors
        scale_qS = 1.0f - t;
        scale_qE *= t;
    }
    else
    {
        // angle between qS and qE is NOT small enough - do spherical interpolation
        float omega = ACos(cos_omega);  //  angle between qS and qE
        float one_over_sin_omega = 1.0f / Sin(omega);
        scale_qS = Sin((1.0f - t) * omega) * one_over_sin_omega;
        scale_qE *= Sin(t*omega) * one_over_sin_omega;
    }
    w = scale_qS*q1.w + scale_qE*q2.w;
    x = scale_qS*q1.x + scale_qE*q2.x;
    y = scale_qS*q1.y + scale_qE*q2.y;
    z = scale_qS*q1.z + scale_qE*q2.z;

}

} // namespace RadicalMathLibrary

