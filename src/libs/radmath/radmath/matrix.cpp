//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <radmath/matrix.hpp>
#include <radmath/trig.hpp>
#include <assert.h>

namespace RadicalMathLibrary
{

// a template for an identity matrix

Matrix::Matrix(float a, float b, float c, float d, 
             float e, float f, float g, float h, 
             float i, float j, float k, float l, 
             float m2, float n, float o, float p)
{
    m[0][0] = a; m[0][1] = b; m[0][2] = c; m[0][3] = d; 
    m[1][0] = e; m[1][1] = f; m[1][2] = g; m[1][3] = h; 
    m[2][0] = i; m[2][1] = j; m[2][2] = k; m[2][3] = l; 
    m[3][0] = m2; m[3][1] = n; m[3][2] = o; m[3][3] = p; 

}

void Matrix::Identity(void)
{
    m[0][0] = 1.0f; m[0][1] = 0.0f; m[0][2] = 0.0f; m[0][3] = 0.0f; 
    m[1][0] = 0.0f; m[1][1] = 1.0f; m[1][2] = 0.0f; m[1][3] = 0.0f; 
    m[2][0] = 0.0f; m[2][1] = 0.0f; m[2][2] = 1.0f; m[2][3] = 0.0f; 
    m[3][0] = 0.0f; m[3][1] = 0.0f; m[3][2] = 0.0f; m[3][3] = 1.0f; 
}

void Matrix::IdentityRotation(void)
{
    m[0][0] = m[1][1] = m[2][2] = 1;
    m[0][1] = m[0][2] = 
    m[1][0] = m[1][2] = 
    m[2][0] = m[2][1] = 0;

}

void Matrix::IdentityTranslation(void)
{
    m[3][0] = m[3][1] = m[3][2] = 0;

}

void Matrix::IdentityProjection(void)
{
    m[0][3] = m[1][3] = m[2][3] = 0;
    m[3][3] = 1;

}

void Matrix::FillRotation(const Quaternion& quat)
{ 
    quat.ConvertToMatrix(this); 
}

void Matrix::FillRotateX(float angle)
{
    m[0][0] = 1.0f;
    m[0][1] = m[0][2] = m[1][0] = m[2][0] = 0;
    float sinX, cosX;
    SinCos(angle, &sinX, &cosX);
    m[1][1] = m[2][2] = cosX;
    m[2][1] = -sinX;
    m[1][2] = sinX;

}

void Matrix::FillRotateY(float angle)
{
    m[1][1] = 1.0f;
    m[0][1] = m[1][0] = m[1][2] = m[2][1] = 0;
    float sinY, cosY;
    SinCos(angle, &sinY, &cosY);
    m[0][0] = m[2][2] = cosY;
    m[0][2] = -sinY;
    m[2][0] = sinY;

}

void Matrix::FillRotateZ(float angle)
{
    m[2][2] = 1.0f;
    m[0][2] = m[1][2] = m[2][0] = m[2][1] = 0;
    float sinZ, cosZ;
    SinCos(angle, &sinZ, &cosZ);
    m[0][0] = m[1][1] = cosZ;
    m[1][0] = -sinZ;
    m[0][1] = sinZ;

}

void Matrix::FillRotation(const Vector& vector, float theta)
{
    Vector axis;
    axis.Normalize(vector);

    float sinHalfTheta, cosHalfTheta;
    float halfTheta = theta * 0.5f;
    SinCos(halfTheta, &sinHalfTheta, &cosHalfTheta);
    float xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;
    float qx = axis.x * sinHalfTheta;
    float qy = axis.y * sinHalfTheta;
    float qz = axis.z * sinHalfTheta;
    float qw = cosHalfTheta;

    xs = qx * 2;  ys = qy * 2;  zs = qz * 2;
    wx = (qw * xs); wy = (qw * ys); wz = (qw * zs);
    xx = (qx * xs); xy = (qx * ys); xz = (qx * zs);
    yy = (qy * ys); yz = (qy * zs); zz = (qz * zs);

    m[0][0] = 1.0f - (yy + zz); m[0][1] = xy - wz; m[0][2] = xz + wy;
    m[1][0] = xy + wz; m[1][1] = 1.0f - (xx + zz); m[1][2] = yz - wx;
    m[2][0] = xz - wy; m[2][1] = yz + wx; m[2][2] = 1.0f - (xx + yy);

}

void Matrix::FillRotateXYZ(float anglex, float angley, float anglez)
{
    float sx, cx, sy, cy, sz, cz;
    SinCos(anglex, &sx, &cx);
    SinCos(angley, &sy, &cy);
    SinCos(anglez, &sz, &cz);
    float sxsy = sx*sy;
    float cxsy = cx*sy;
    m[0][0] = cy*cz;
    m[0][1] = cy*sz;
    m[0][2] = -sy;
    m[1][0] = sxsy*cz - cx*sz;
    m[1][1] = sxsy*sz + cx*cz;
    m[1][2] = sx*cy;
    m[2][0] = cxsy*cz + sx*sz;
    m[2][1] = cxsy*sz - sx*cz;
    m[2][2] = cx*cy;

}

void Matrix::FillRotateYZX(float anglex, float angley, float anglez)
{
    float cx, sx, cy, sy, cz, sz;
    SinCos(anglex, &sx, &cx);
    SinCos(angley, &sy, &cy);
    SinCos(anglez, &sz, &cz);
    float cxcy = cx*cy;
    float sxsy = sx*sy;
    float sxcy = sx*cy;
    float cxsy = cx*sy;
    m[0][0] = cy*cz;
    m[0][1] = cxcy*sz + sxsy;
    m[0][2] = sxcy*sz - cxsy;
    m[1][0] = -sz;
    m[1][1] = cx*cz;
    m[1][2] = sx*cz;
    m[2][0] = sy*cz;
    m[2][1] = cxsy*sz - sxcy;
    m[2][2] = sxsy*sz + cxcy;

}

void Matrix::FillTranslate(const Vector& vector)
{
    m[3][0] = vector.x;
    m[3][1] = vector.y;
    m[3][2] = vector.z;

}

void Matrix::FillScale(float xScale, float yScale, float zScale)
{
    // set non-diagonals to zero
    m[0][1] = m[0][2] = m[1][0] = m[1][2] = m[2][0] = m[2][1] = 0;

    m[0][0] = xScale;
    m[1][1] = yScale;
    m[2][2] = zScale;

}

//------------------------------------------------------------------
// Heading matrix functions
// Given a heading vector, these functions will build a matrix that
// is aligned to that vector (optionally ignoring certain axes). 
// useful for billboard type operations
void Matrix::FillHeadingXZ(const Vector& heading)
{
    IdentityRotation();
    Vector v(heading.x, 0.0f, heading.z);
    float length = v.Magnitude();

    float rl = 1.0f / length;
    v.x *= rl;
    v.z *= rl;
    m[0][0] = v.z;
    m[0][2] = -v.x;
    m[2][0] = v.x;
    m[2][2] = v.z;

}

void Matrix::FillHeadingYZ(const Vector& heading)
{
    IdentityRotation();
    Vector v(0.0f, heading.y, heading.z);
    float length = v.Magnitude();
    float rl = 1.0f / length;
    v.y *= rl;
    v.z *= rl;
    m[1][1] = v.z;
    m[1][2] = -v.y;
    m[2][1] = v.y;
    m[2][2] = v.z;

}

void Matrix::FillHeadingXY(const Vector& heading)
{
    IdentityRotation();
    Vector v(heading.x, heading.y, 0.0f);
    float length = v.Magnitude();
    float rl = 1.0f / length;
    v.x *= rl;
    v.y *= rl;
    m[0][0] = v.x;
    m[0][1] = v.y;
    m[1][0] = -v.y;
    m[1][1] = v.x;

}

void Matrix::FillHeading(const Vector& heading, const Vector& vup)
{
    Row(2) = heading;
    Row(2).Normalize();
    Row(0).CrossProduct(vup, heading);
    Row(1).CrossProduct(heading, Row(0));
    Row(0).Normalize();
    Row(1).Normalize();

}

void Matrix::FillReflection(const Vector& point, const Vector& normal)
{
    Vector n;
    n.Normalize(normal);
    float dot2 = 2.0f * (point.x*n.x + point.y*n.y + point.z*n.z);

    m[0][0] = 1 - 2.0f * n.x * n.x;
    m[1][0] =   - 2.0f * n.x * n.y;
    m[2][0] =   - 2.0f * n.x * n.z;
    m[3][0] = dot2 * n.x;

    m[0][1] =   - 2.0f * n.y * n.x;
    m[1][1] = 1 - 2.0f * n.y * n.y;
    m[2][1] =   - 2.0f * n.y * n.z;
    m[3][1] = dot2 * n.y;

    m[0][2] =   - 2.0f * n.z * n.x;
    m[1][2] =   - 2.0f * n.z * n.y;
    m[2][2] = 1 - 2.0f * n.z * n.z;
    m[3][2] = dot2 * n.z;

    m[0][3] = 0;
    m[1][3] = 0;
    m[2][3] = 0;
    m[3][3] = 1;


}


void Matrix::SetPerspective(const float fov, const float aspect, const float n, const float f)
{
    float halfx = Tan(fov*0.5f)*n;
    float halfy = halfx/aspect;

    float a = f/(f-n);
    float b = -a * n;
    float c = n/halfx;
    float d = n/halfy;

    Row4(0).Set(  c,  0, 0, 0);
    Row4(1).Set(  0,  d, 0, 0);
    Row4(2).Set(  0,  0, a, 1);
    Row4(3).Set(  0,  0, b, 0);

}

void Matrix::Transpose(void)
{
    float t1 = m[0][1];
    float t2 = m[0][2];
    float t3 = m[0][3];
    float t4 = m[1][2];
    float t5 = m[1][3];
    float t6 = m[2][3];

    m[0][1] = m[1][0];
    m[0][2] = m[2][0];
    m[0][3] = m[3][0];
    m[1][2] = m[2][1];
    m[1][3] = m[3][1];
    m[2][3] = m[3][2];

    m[1][0] = t1;
    m[2][0] = t2;
    m[3][0] = t3;
    m[2][1] = t4;
    m[3][1] = t5;
    m[3][2] = t6;

}

void Matrix::InvertOrtho(void)
{
    float t0 = -m[3][0];
    float t1 = -m[3][1];
    float t2 = -m[3][2];

    Transpose();
    IdentityProjection();
    m[3][0] = (t0*m[0][0]) + (t1*m[1][0]) + (t2*m[2][0]);
    m[3][1] = (t0*m[0][1]) + (t1*m[1][1]) + (t2*m[2][1]);
    m[3][2] = (t0*m[0][2]) + (t1*m[1][2]) + (t2*m[2][2]);

}

void Matrix::Invert(void)
{
    float det;
    det  = (  m[0][0] * m[1][1] * m[2][2] );
    det += (  m[0][1] * m[1][2] * m[2][0] );
    det += (  m[0][2] * m[1][0] * m[2][1] );
    det += ( -m[0][2] * m[1][1] * m[2][0] );
    det += ( -m[0][1] * m[1][0] * m[2][2] );
    det += ( -m[0][0] * m[1][2] * m[2][1] );

    if(Epsilon(det, 1.0f))
    {
        InvertOrtho();
    }
    else
    {
        Matrix tmp;
        float inverseDet = 1.0f / det;
        tmp.m[0][0] =   ( ( (m[1][1] * m[2][2]) - (m[1][2] * m[2][1]) ) * inverseDet );
        tmp.m[1][0] = - ( ( (m[1][0] * m[2][2]) - (m[1][2] * m[2][0]) ) * inverseDet );
        tmp.m[2][0] =   ( ( (m[1][0] * m[2][1]) - (m[1][1] * m[2][0]) ) * inverseDet );
        tmp.m[3][0] = m[3][0];
        tmp.m[0][1] = - ( ( (m[0][1] * m[2][2]) - (m[0][2] * m[2][1]) ) * inverseDet );
        tmp.m[1][1] =   ( ( (m[0][0] * m[2][2]) - (m[0][2] * m[2][0]) ) * inverseDet );
        tmp.m[2][1] = - ( ( (m[0][0] * m[2][1]) - (m[0][1] * m[2][0]) ) * inverseDet );
        tmp.m[3][1] = m[3][1];
        tmp.m[0][2] =   ( ( (m[0][1] * m[1][2]) - (m[0][2] * m[1][1]) ) * inverseDet );
        tmp.m[1][2] = - ( ( (m[0][0] * m[1][2]) - (m[0][2] * m[1][0]) ) * inverseDet );
        tmp.m[2][2] =   ( ( (m[0][0] * m[1][1]) - (m[0][1] * m[1][0]) ) * inverseDet );    
        tmp.m[3][2] = m[3][2];
        // Calculate -C * inverse(A)
        tmp.m[3][0] = - ( (m[3][0] * tmp.m[0][0]) + (m[3][1] * tmp.m[1][0]) + (m[3][2] * tmp.m[2][0]) );
        tmp.m[3][1] = - ( (m[3][0] * tmp.m[0][1]) + (m[3][1] * tmp.m[1][1]) + (m[3][2] * tmp.m[2][1]) );
        tmp.m[3][2] = - ( (m[3][0] * tmp.m[0][2]) + (m[3][1] * tmp.m[1][2]) + (m[3][2] * tmp.m[2][2]) );
        tmp.m[3][3] = m[3][3];
        *this = tmp;

        IdentityProjection();
    
    }
}

void Matrix::OrthoNormal(void)
{
    Vector r1new, r2new, r3new, r2norm;
    r1new = Row(0);
    r2norm = Row(1);

    r1new.Normalize();
    r3new.CrossProduct(r1new,r2norm);
    r3new.Normalize();
    r2new.CrossProduct(r3new,r1new);

    Row(0) = r1new;
    Row(1) = r2new;
    Row(2) = r3new;

}

bool Matrix::IsOrthoNormal()
{
    float m0 = Row(0).Magnitude();
    float m1 = Row(1).Magnitude();
    float m2 = Row(2).Magnitude();
    float d0 = Row(0).DotProduct(Row(1));
    float d1 = Row(1).DotProduct(Row(2));
    float d2 = Row(0).DotProduct(Row(2));

    return Epsilon(m0, 1.0f) &&
             Epsilon(m1, 1.0f) &&
             Epsilon(m2, 1.0f) &&
             Epsilon(d0, 0.0f) &&
             Epsilon(d1, 0.0f) &&
             Epsilon(d2, 0.0f);
}

void Matrix::Transform(const Vector& src, Vector* dest) const
{
    float x,y,z;
    x = m[0][0]*src.x + m[1][0]*src.y + m[2][0]*src.z + m[3][0];
    y = m[0][1]*src.x + m[1][1]*src.y + m[2][1]*src.z + m[3][1];
    z = m[0][2]*src.x + m[1][2]*src.y + m[2][2]*src.z + m[3][2];
    dest->Set(x,y,z);
}

void Matrix::Transform(const Vector4& src, Vector4* dest) const
{
    float x,y,z,w;
    x = m[0][0]*src.x + m[1][0]*src.y + m[2][0]*src.z + m[3][0]*src.w;
    y = m[0][1]*src.x + m[1][1]*src.y + m[2][1]*src.z + m[3][1]*src.w;
    z = m[0][2]*src.x + m[1][2]*src.y + m[2][2]*src.z + m[3][2]*src.w;
    w = m[0][3]*src.x + m[1][3]*src.y + m[2][3]*src.z + m[3][3]*src.w;
    dest->Set(x,y,z,w);
}

void Matrix::RotateVector(const Vector& src, Vector* dest) const
{
    float x,y,z;;
    x = m[0][0]*src.x + m[1][0]*src.y + m[2][0]*src.z;
    y = m[0][1]*src.x + m[1][1]*src.y + m[2][1]*src.z;
    z = m[0][2]*src.x + m[1][2]*src.y + m[2][2]*src.z;
    dest->Set(x,y,z);
}

void Matrix::Transform(int n, const Vector* src, Vector* dest) const
{
    for(int i = 0; i < n; i++)
        Transform(*src++, dest++);
}

void Matrix::Transform(int n, const Vector4* src, Vector4* dest) const
{
    for(int i = 0; i < n; i++)
        Transform(*src++, dest++);
}

void Matrix::Mult(const Matrix& a, const Matrix& b)
{
    assert(a.m[0][3] == 0.0f);
    assert(a.m[1][3] == 0.0f);
    assert(a.m[2][3] == 0.0f);
    assert(a.m[3][3] == 1.0f);
    assert(b.m[0][3] == 0.0f);
    assert(b.m[1][3] == 0.0f);
    assert(b.m[2][3] == 0.0f);
    assert(b.m[3][3] == 1.0f);

    m[0][0] = (a.m[0][0] * b.m[0][0]) + (a.m[0][1] * b.m[1][0]) + (a.m[0][2] * b.m[2][0]);
    m[0][1] = (a.m[0][0] * b.m[0][1]) + (a.m[0][1] * b.m[1][1]) + (a.m[0][2] * b.m[2][1]);
    m[0][2] = (a.m[0][0] * b.m[0][2]) + (a.m[0][1] * b.m[1][2]) + (a.m[0][2] * b.m[2][2]);
    m[0][3] = 0.0f;

    m[1][0] = (a.m[1][0] * b.m[0][0]) + (a.m[1][1] * b.m[1][0]) + (a.m[1][2] * b.m[2][0]);
    m[1][1] = (a.m[1][0] * b.m[0][1]) + (a.m[1][1] * b.m[1][1]) + (a.m[1][2] * b.m[2][1]);
    m[1][2] = (a.m[1][0] * b.m[0][2]) + (a.m[1][1] * b.m[1][2]) + (a.m[1][2] * b.m[2][2]);
    m[1][3] = 0.0f;

    m[2][0] = (a.m[2][0] * b.m[0][0]) + (a.m[2][1] * b.m[1][0]) + (a.m[2][2] * b.m[2][0]);
    m[2][1] = (a.m[2][0] * b.m[0][1]) + (a.m[2][1] * b.m[1][1]) + (a.m[2][2] * b.m[2][1]);
    m[2][2] = (a.m[2][0] * b.m[0][2]) + (a.m[2][1] * b.m[1][2]) + (a.m[2][2] * b.m[2][2]);
    m[2][3] = 0.0f;

    m[3][0] = (a.m[3][0] * b.m[0][0]) + (a.m[3][1] * b.m[1][0]) + (a.m[3][2] * b.m[2][0]) + b.m[3][0];
    m[3][1] = (a.m[3][0] * b.m[0][1]) + (a.m[3][1] * b.m[1][1]) + (a.m[3][2] * b.m[2][1]) + b.m[3][1];
    m[3][2] = (a.m[3][0] * b.m[0][2]) + (a.m[3][1] * b.m[1][2]) + (a.m[3][2] * b.m[2][2]) + b.m[3][2];
    m[3][3] = 1.0f;


}

void Matrix::MultFull(const Matrix& a, const Matrix& b)
{
    for(int i = 0; i < 4; i++)
    {
        for(int j = 0; j < 4; j++)
        {
            m[i][j] = 0;
            for(int k = 0; k < 4; k++)
            {
                m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }
    }

}
}

