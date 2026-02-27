#include "simcommon/symmatrix.hpp"
#include "simcommon/simmath.hpp"

using namespace RadicalMathLibrary;

namespace sim
{

SymMatrix::SymMatrix()
{
}

SymMatrix::~SymMatrix()
{
}

SymMatrix::SymMatrix(float r1, float r2, float r3, float r4, float r5, float r6)
{
    xx = r1;
    xy = r2;
    xz = r3;
    yy = r4;
    yz = r5;
    zz = r6;
}

void SymMatrix::Add(const SymMatrix& b)
{
    xx += b.xx;
    xy += b.xy;
    xz += b.xz;
    yy += b.yy;
    yz += b.yz;
    zz += b.zz;
}

void SymMatrix::Sum(const SymMatrix& a, const SymMatrix& b)
{
    xx = a.xx + b.xx;
    xy = a.xy + b.xy;
    xz = a.xz + b.xz;
    yy = a.yy + b.yy;
    yz = a.yz + b.yz;
    zz = a.zz + b.zz;
}

void SymMatrix::Scale(const float b)
{
    xx = xx * b;
    xy = xy * b;
    xz = xz * b;
    yy = yy * b;
    yz = yz * b;
    zz = zz * b;
}

void SymMatrix::Scale(const SymMatrix& a, const float b)
{
    xx = a.xx * b;
    xy = a.xy * b;
    xz = a.xz * b;
    yy = a.yy * b;
    yz = a.yz * b;
    zz = a.zz * b;
}

void SymMatrix::Mult(const SymMatrix& a, const SymMatrix& b)
{
    SymMatrix c;
    c.xx = a.xx * b.xx + a.xy * b.xy + a.xz * b.xz;
    c.xy = a.xx * b.xy + a.xy * b.yy + a.xz * b.yz;
    c.xz = a.xx * b.xz + a.xy * b.yz + a.xz * b.zz;
    c.yy = a.xy * b.xy + a.yy * b.yy + a.yz * b.yz;
    c.yz = a.xy * b.xz + a.yy * b.yz + a.yz * b.zz;
    c.zz = a.xz * b.xz + a.yz * b.yz + a.zz * b.zz;
    *this = c;
}

void SymMatrix::FillMatrix(Matrix& m) const
{
    m.m[0][0] = xx;
    m.m[0][1] = xy;
    m.m[1][0] = xy;
    m.m[0][2] = xz;
    m.m[2][0] = xz;
    m.m[1][1] = yy;
    m.m[1][2] = yz;
    m.m[2][1] = yz;
    m.m[2][2] = zz;
    m.IdentityTranslation();
}

void SymMatrix::Set(Matrix& m)
{
    xx = m.m[0][0];
    xy = m.m[0][1];
    xz = m.m[0][2];
    yy = m.m[1][1];
    yz = m.m[1][2];
    zz = m.m[2][2];
    // rAssert(xy == m.m[1][0]);
    // rAssert(xz == m.m[2][0]);
    // rAssert(yz == m.m[2][1]);
}

/*
void SymMatrix::Transform(const Matrix& m)
{
float a, b, c, e1, e2, e3;
SymMatrix p;

  e1 = PhGetRotElement(0, 0, &m); 
  e2 = PhGetRotElement(0, 1, &m); 
  e3 = PhGetRotElement(0, 2, &m);
  a = rmMult(xx, e1) + rmMult(xy, e2) + rmMult(xz, e3);
  b = rmMult(xy, e1) + rmMult(yy, e2) + rmMult(yz, e3);
  c = rmMult(xz, e1) + rmMult(yz, e2) + rmMult(zz, e3);
  p.xx = rmMult(a, e1) + rmMult(b, e2) + rmMult(c, e3);
  
    e1 = PhGetRotElement(2, 0, &m); 
    e2 = PhGetRotElement(2, 1, &m); 
    e3 = PhGetRotElement(2, 2, &m);
    p.xz = rmMult(a, e1) + rmMult(b, e2) + rmMult(c, e3);
    
      e1 = PhGetRotElement(1, 0, &m); 
      e2 = PhGetRotElement(1, 1, &m); 
      e3 = PhGetRotElement(1, 2, &m);
      p.xy = rmMult(a, e1) + rmMult(b, e2) + rmMult(c, e3);
      
        a = rmMult(xx, e1) + rmMult(xy, e2) + rmMult(xz, e3);
        b = rmMult(xy, e1) + rmMult(yy, e2) + rmMult(yz, e3);
        c = rmMult(xz, e1) + rmMult(yz, e2) + rmMult(zz, e3);
        p.yy = rmMult(a, e1) + rmMult(b, e2) + rmMult(c, e3);
        
          e1 = PhGetRotElement(2, 0, &m); 
          e2 = PhGetRotElement(2, 1, &m); 
          e3 = PhGetRotElement(2, 2, &m);
          p.yz = rmMult(a, e1) + rmMult(b, e2) + rmMult(c, e3);
          
            a = rmMult(xx, e1) + rmMult(xy, e2) + rmMult(xz, e3);
            b = rmMult(xy, e1) + rmMult(yy, e2) + rmMult(yz, e3);
            c = rmMult(xz, e1) + rmMult(yz, e2) + rmMult(zz, e3);
            p.zz = rmMult(a, e1) + rmMult(b, e2) + rmMult(c, e3);
            *this = p;
            }
*/

void SymMatrix::TransformSelf(const Matrix& m)
{
    float a, b, c, e1, e2, e3;
    SymMatrix p;
    
    e1 = m.m[0][0]; 
    e2 = m.m[1][0]; 
    e3 = m.m[2][0];
    a = xx*e1 + xy*e2 + xz*e3;
    b = xy*e1 + yy*e2 + yz*e3;
    c = xz*e1 + yz*e2 + zz*e3;
    p.xx = a*e1 + b*e2 + c*e3;
    
    e1 = m.m[0][2]; 
    e2 = m.m[1][2]; 
    e3 = m.m[2][2];
    p.xz = a*e1 + b*e2 + c*e3;
    
    e1 = m.m[0][1]; 
    e2 = m.m[1][1]; 
    e3 = m.m[2][1];
    p.xy = a*e1 + b*e2 + c*e3;
    
    a = xx*e1 + xy*e2 + xz*e3;
    b = xy*e1 + yy*e2 + yz*e3;
    c = xz*e1 + yz*e2 + zz*e3;
    p.yy = a*e1 + b*e2 + c*e3;
    
    e1 = m.m[0][2]; 
    e2 = m.m[1][2]; 
    e3 = m.m[2][2];
    p.yz = a*e1 + b*e2 + c*e3;
    
    a = xx*e1 + xy*e2 + xz*e3;
    b = xy*e1 + yy*e2 + yz*e3;
    c = xz*e1 + yz*e2 + zz*e3;
    p.zz = a*e1 + b*e2 + c*e3;
    *this = p;
}

void SymMatrix::TransformSelf(const Vector& v)
{
    float a, b, c;
    SymMatrix p;
    
    a = -xy*v.z + xz*v.y;
    b = -yy*v.z + yz*v.y;
    c = -yz*v.z + zz*v.y;
    p.xx = -b*v.z + c*v.y;
    p.xz = -a*v.y + b*v.x;
    p.xy = a*v.z - c*v.x;
    
    a = xx*v.z - xz*v.x;
    c = xz*v.z - zz*v.x;
    p.yy = a*v.z - c*v.x;
    p.yz = -a*v.y;
    
    a = -xx*v.y + xy*v.x;
    b = -xy*v.y + yy*v.x;
    p.zz = -a*v.y + b*v.x;
    *this = p;
}

void SymMatrix::TransformVector(const Vector& v, Vector& r) const
{
    r.x = xx*v.x + xy*v.y + xz*v.z;
    r.y = xy*v.x + yy*v.y + yz*v.z;
    r.z = xz*v.x + yz*v.y + zz*v.z;  
}

void SymMatrix::Invert(const SymMatrix& a)
{
/*
This class usually describes Inertia matrix which may contains
relatively extreme numbers. Since we have to deal with power 3 of
these extreme number it is safe to normalize a at the beginning and 
multiply the result by the scale factor at the end
    */
    rAssert(a.xx != 0); // a.xx not supposed to be 0
    float scaleFactor = 1.0f / a.xx;
    SymMatrix p;
    p.Scale(a, scaleFactor);
    
    float det;
    det =  p.xx * p.yy * p.zz;
    
    if (det == 0)
    {
        Zero(); // better than nan...
        rAssertMsg(false, "Trying to invert a SymMatrix matrix with det=0");
        return;
    }
    else
    {
        *this = p;
        det += 2.0f * xy * yz * xz;
        det -= Sqr(xz) * yy;
        det -= Sqr(xy) * zz;
        det -= Sqr(yz) * xx;  
        
        // Calculate inverse(A) = adj(A) / det(A)
        float idet = scaleFactor / det; // insert the scale factor
        
        p.xx =   (yy * zz - Sqr(yz)) * idet;
        p.xy = - (xy * zz - yz * xz) * idet;
        p.xz =   (xy * yz - yy * xz) * idet;
        p.yy =   (xx * zz - Sqr(xz)) * idet;
        p.yz = - (xx * yz - xy * xz) * idet;
        p.zz =   (xx * yy - Sqr(xy)) * idet;  
        *this = p;
    }
}

void SymMatrix::Identity()
{
    xx = yy = zz = 1.0f;
    xy = xz = yz = 0;
}

void SymMatrix::Zero()
{
    xx = yy = zz = 0;
    xy = xz = yz = 0;
}

} // sim