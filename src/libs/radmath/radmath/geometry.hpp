//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _RADMATH_GEOMETRY_HPP
#define _RADMATH_GEOMETRY_HPP

#include <radmath/buildconfig.hpp>
#include <radmath/vector.hpp>
#include <radmath/util.hpp>

namespace RadicalMathLibrary
{

//-------------------------------------------------------
//  Spheres
//-------------------------------------------------------
class Sphere
{
public:
   Sphere() :                  centre(Vector(0,0,0)), radius(1)   { /**/ }
   Sphere(Vector c, float r) : centre(c), radius(r)               { /**/ }
   Sphere(const Sphere& s) :   centre(s.centre), radius(s.radius) { /**/ }

   void Set(Vector c, float r) { centre = c; radius = r; }

   //can this sphere intersect the other one?
   inline bool Intersects( const Sphere& right ) const
   {
        Vector diff(centre);
        diff.Sub(right.centre);
        if( diff.Magnitude() < ( right.radius + radius ) )
        {
            return true;
        }
        else
        {
            return false;
        }
   }

   //distance from the sphere to a point
   float Distance( const Vector& point ) const
   {
        float dist = ( centre - point ).Magnitude();
        return( dist - radius );
   }
   
   //does the sphere contain the point?
   bool Contains( const Vector& point ) const
   {
        float distSqr = ( centre - point ).MagnitudeSqr();
        return( distSqr <= radius * radius );
   }

   Vector centre;
   float radius;
};

//-------------------------------------------------------
// p3d box
//-------------------------------------------------------

class Box3D
{
public:
    Box3D() :                       low(Vector(1e10f, 1e10f, 1e10f)), high(Vector(-1e10f, -1e10f, -1e10f)) { /**/ }
    Box3D(Vector min, Vector max) : low(min), high(max)      { /**/ }
    Box3D(const Box3D& b) :         low(b.low), high(b.high) { /**/ }

    void Set(Vector min, Vector max) { low = min; high = max; }

    inline float Height() const { return high.z - low.z; }
    inline float Length() const { return high.x - low.x; }
    inline float Width() const  { return high.y - low.y; }

    Sphere GetBoundingSphere() const
    {
        Vector temp; 
        Vector centre;
        float diagonal;

        temp.Sub(high,low); 
        diagonal = temp.Magnitude(); 
        centre.ScaleAdd(low,0.5f,temp);
        return Sphere(centre,diagonal);
    }

    void GetBoundingSphere(Sphere* ipSphere) const
    {
        Vector temp; 

        temp.Sub(high,low); 
        ipSphere->radius = temp.Magnitude(); 
        ipSphere->centre.ScaleAdd(low,0.5f,temp);
    }

    //does the box contain the point?
    bool Contains( const Vector& point ) const
    {
        if ((point.x>=low.x)&&(point.x<=high.x))
        {
            if ((point.y>=low.y)&&(point.y<=high.y))
            {
                if ((point.z>=low.z)&&(point.z<=high.z))
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool Intersects( const Sphere& sphere ) const
    {
        if ((sphere.centre.x>=(low.x-sphere.radius))&&(sphere.centre.x<=(high.x+sphere.radius)))
        {
            if ((sphere.centre.y>=(low.y-sphere.radius))&&(sphere.centre.y<=(high.y+sphere.radius)))
            {
                if ((sphere.centre.z>=(low.z-sphere.radius))&&(sphere.centre.z<=(high.z+sphere.radius)))
                {
                    return true;
                }
            }
        }
        return false;   
    }

    bool Intersects( const Box3D& box ) const
    {
        if ((box.high.x>=low.x)&&(box.low.x<=high.x))
        {
            if ((box.high.y>=low.y)&&(box.low.y<=high.y))
            {
                if ((box.high.z>=low.z)&&(box.low.z<=high.z))
                {
                    return true;
                }
            }
        }
        return false;   
    }
    
    void Expand( const Vector& point )
    {
        low.x = Min(low.x,point.x);
        low.y = Min(low.y,point.y);
        low.z = Min(low.z,point.z);
        high.x = Max(high.x,point.x);
        high.y = Max(high.y,point.y);
        high.z = Max(high.z,point.z);
    }

    Vector Mid() const
    {
        Vector mid;
        mid.Add(high, low);
        mid.Scale(0.5f);
        return mid;
    }

    Vector low;
    Vector high;
};

//-------------------------------------------------------
// Planes
// 
// A simple plane in space, defined by plane equation 
//    Ax + By + Cz + D = 0;
//    Where (A,B,C) are the surface normal and D is the constant 
//-------------------------------------------------------

class Plane
{
public:
    enum Side { Front, Back, Contained };
    Plane() : normal(0,0,1), D(0) {};

    // build a plane from plane equation
    Plane(const Vector& n, float d) : normal(n), D(d) {}; 
    void Set(const Vector& n, float d) { normal = n; D = d;}

    // build a plane from another plane
    Plane(const Plane& plane) : normal(plane.normal), D(plane.D) {} 

    // build a plane from a normal and a point in the plane
    Plane(const Vector& norm, const Vector& pointInPlane) : normal(norm), D(-normal.DotProduct(pointInPlane)) {}
    void Set(const Vector& norm, const Vector& pointInPlane) { normal= norm; D =  -normal.DotProduct(pointInPlane); }

    // build a plane from three points in the plane
    Plane(const Vector& pointInPlane1, const Vector& pointInPlane2, const Vector& pointInPlane3) { Set(pointInPlane1, pointInPlane2, pointInPlane3); }
    void Set(const Vector& pointInPlane1, const Vector& pointInPlane2, const Vector& pointInPlane3);

    // invert the plane
    void Invert(void) { normal.Scale(-1); D *= -1; }

    // signed distance to plane 
    // (positive vales are on same side as normal, negative are on other side)
    float Distance(const Vector& point) const { return point.DotProduct( normal ) + D; }

    // Is a point inside the plane, inside being defined as the side away from the surface normal
    bool Inside (const Vector& point) const { return point.DotProduct( normal ) < -D; }

    // Is a sphere(any part of it) inside the plane
    bool Inside (const Sphere& sphere) const { return sphere.centre.DotProduct(normal) < (-D + (sphere.radius)); }

    // Is a box (any part of it) inside the plane
    bool Inside (const Box3D& box) const;
 
    // Is a point completely inside the plane, inside being defined as the side away from the surface normal
    bool Contains (const Vector& point) const { return Inside(point); }

    // Is a sphere completely inside the plane, inside being defined as the side away from the surface normal
    bool Contains (const Sphere& sphere) const { return sphere.centre.DotProduct(normal) < (-D - (sphere.radius)); }

    // Is a box completely inside the plane, inside being defined as the side away from the surface normal
    bool Contains (const Box3D& box) const;

    Side CheckSide(const Vector& point, float epsilon = 0.00001f) const 
    {
        float distance = Distance(point);
        if(Epsilon(distance, 0.0f, epsilon))
        {
            return Contained;
        }
        return (distance > 0) ? Front : Back;
    }

    // Is point on the plane, infront or behind
    Side Test(const Vector& point) const 
    {
        return CheckSide(point,0.0f);
    }
   
    // Is sphere on the plane, infront or behind
    Side Test(const Sphere& sphere) const 
    {
        float distance = Distance(sphere.centre);
        if(distance<=sphere.radius)
        {
            if(distance>=-sphere.radius)
            {
                return Contained;
            }
            return Back;
        }
        else
        {
            return Front;
        }
    }

    // Is box on the plane, infront or behind
    Side Test(const Box3D& box) const 
    {
        const Vector& low = box.low;
        const Vector& high = box.high;
        Vector p[8];

        p[0].Set( low.x,  low.y,  low.z );
        p[1].Set( high.x, low.y,  low.z );
        p[2].Set( high.x, high.y, low.z );
        p[3].Set( low.x,  high.y, low.z );

        p[4].Set( low.x,  low.y,  high.z );
        p[5].Set( high.x, low.y,  high.z );
        p[6].Set( high.x, high.y, high.z );
        p[7].Set( low.x,  high.y, high.z );
    
        int front = 0;
        int back = 0;

        for (int i = 0; i < 8; i++)
        {
            switch (Test(p[i]))
            {
                case Front:
                {
                    front++;
                    if (back>0)
                    {
                        return Contained;
                    }
                    break;
                }
                case Back:
                {
                    back++;
                    if (front>0)
                    {
                        return Contained;
                    }
                    break;
                }
                case Contained:
                {
                    return Contained;
                    break;
                }
            }
        }
        
        return (front>0) ? Front : Back;
    }

    // line segment intersection tests 
    bool Intersect(const Vector&, const Vector&, float *dist ); 
    bool Intersect(const Vector& start, const Vector& dir, Vector* ints );

    // transform by a matrix
    void Transform(const Matrix& mat);
    void Transform(const Matrix& mat, const Plane& plane);

    // the plane equation itself
    Vector normal;
    float D;
};

//--------------------------------------
// triangles
//--------------------------------------

class Triangle 
{
public:
    //constructor
    Triangle
    ( 
        const Vector& p0, 
        const Vector& p1, 
        const Vector& p2 
    );

    //distance from a point to a triangle
    float Distance( const Vector& p ) const;

    //measures the distance between two triangles
    float Distance( const Triangle& right ) const;

    //the squared distance to this triangle
    float DistanceSquared( const Vector& p ) const;

    //test if two triangles intersect
    bool Intersect( const Triangle& right ) const;

    //does a rudimentary check whether or not the triangles can intersect
    bool IsIntersectionPossible( const Triangle& right ) const;

    Vector v[ 3 ];
protected:
};

}

namespace rmt = RadicalMathLibrary;

#endif
