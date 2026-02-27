//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <radmath/geometry.hpp>
#include <radmath/matrix.hpp>

namespace RadicalMathLibrary
{

void Plane::Set(const Vector& p1, const Vector& p2, const Vector& p3)
{
   Vector v1, v2;
   v1.Sub(p2,p1);
   v2.Sub(p3,p1);
   v1.NormalizeSafe();
   v2.NormalizeSafe();
   normal.CrossProduct(v1,v2);
   normal.NormalizeSafe();
   Set(normal, p1);
}

bool Plane::Inside (const Box3D& box) const
{
    //this function takes the AABB algorithms from Real Time Rendering book( 2n Edition, p587 )
    //and make a minor change
    Vector v;

    v.x = normal.x >= 0.f ? box.low.x : box.high.x;
    v.y = normal.y >= 0.f ? box.low.y : box.high.y;
    v.z = normal.z >= 0.f ? box.low.z : box.high.z;

    return Inside( v );
}

bool Plane::Contains (const Box3D& box) const
{
    // Increase the number of points we check for a box.

    //
    // this code contains alot of branch instructions, thus it will mess with
    // any branch predictions in the processor.  it would be faster to cull
    // by bounding the box with a sphere, and testing that first.
    //
    const Vector& low = box.low;
    const Vector& high = box.high;
    Vector p0( low.x,  low.y,  low.z );
    Vector p1( high.x, low.y,  low.z );
    Vector p2( high.x, high.y, low.z );
    Vector p3( low.x,  high.y, low.z );

    Vector p4( low.x,  low.y,  high.z );
    Vector p5( high.x, low.y,  high.z );
    Vector p6( high.x, high.y, high.z );
    Vector p7( low.x,  high.y, high.z );

    if( Contains(p0) && Contains(p1) && Contains(p2) && Contains(p3) && Contains(p4) && Contains(p5) && Contains(p6) && Contains(p7) )
    {
        return true;
    }
    return false;
}

bool Plane::Intersect(const Vector& start, const Vector& end, float * dist )
{
    float len_s = normal.Dot(start) + D;
    float len_e = normal.Dot(end) + D;
    float l = len_s - len_e;

    if( l != 0.f ){
        *dist = len_s / l;
        return true;
    }
    else                //line is parallel to the plane, no intersection
        return false;
}

bool Plane::Intersect(const Vector& start, const Vector& dir, Vector* ints )
{
    float a = normal.Dot(dir);

    if( a == 0.f )
        return false;       //no solution

    float b = normal.Dot(start) + D;
    
    float m = -b/a;

    ints->Add(start, m*dir);

    return true;
}

void Plane::Transform(const Matrix& mat)
{
    Transform(mat, *this);
}

void Plane::Transform(const Matrix& mat, const Plane& plane)
{
    float scale = D / -plane.normal.Dot(plane.normal);

    Vector pointInPlane;
    pointInPlane.Scale(scale, normal);

    mat.RotateVector(plane.normal, &normal);
    mat.Transform(pointInPlane, &pointInPlane);

    D = -normal.Dot(pointInPlane);
}

} // end of namespace
