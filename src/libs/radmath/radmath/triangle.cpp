//-----------------------------------------------------------------------------
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// triangle.cpp
//
// Description:  defines all the methods for working with triangles
//
//-----------------------------------------------------------------------------
#include <radmath/geometry.hpp>
#include <float.h>

namespace RadicalMathLibrary
{

Triangle::Triangle
( 
    const Vector& p0, 
    const Vector& p1, 
    const Vector& p2 
)
{
    v[ 0 ] = p0;
    v[ 1 ] = p1;
    v[ 2 ] = p2;
}

//-----------------------------------------------------------------------------
// D i s t a n c e
//
// Synopsis:    distance from a point to a triangle
// Parameters:  p - the point to measure the distance to
// Returns:     double - the distance to that point
//-----------------------------------------------------------------------------
float Triangle::Distance( const Vector& p ) const
{
    return Sqrt( DistanceSquared( p ) );
}

//-----------------------------------------------------------------------------
// D i s t a n c e
//
// Synopsis:    distance from one triangle to another
// Parameters:  right - the other triangles
// Returns:     double - the distance to that point
//-----------------------------------------------------------------------------
float Triangle::Distance( const Triangle& right ) const
{
    float minimumDistance = FLT_MAX;
    Vector vx(v[1]);
    vx.Sub(v[0]);
    Vector vy(v[2]);
    vy.Sub(v[0]);

    int i;
    int j;
    static int steps = 5;
    for( i = 0; i <= steps; i++ )
    for( j = 0; j <= steps; j++ )
    {
        float x = i / float( steps );
        float y = j / float( steps );
        if( x + y > 1.0f )
        {
            continue;
        }
        Vector point(v[0]);
        Vector vxs, vys;
        vxs.Scale(x, vx);
        vys.Scale(y, vy);
        point.Add(vxs);
        point.Add(vys);
        
        float distance = right.Distance( point );
        if( distance < minimumDistance )
        {
            minimumDistance = distance;
        }
    }
    return minimumDistance;
}


//-----------------------------------------------------------------------------
// D i s t a n c e S q u a r e d
//
// Synopsis:    the squared distance to this triangle
// Parameters:  p - the point to measure the distance to
// Returns:     double - the squared distance to that point
//-----------------------------------------------------------------------------
float Triangle::DistanceSquared( const Vector& p ) const
{
    //
    // This code is blatantly ripped off of MagicSoftware
    // http://www.magic-software.com/Source/Distance3D/MgcDist3DVecTri.cpp
    // the class names have been changed to protect the innocent
    //

    Vector kDiff(v[0]);
    kDiff.Sub(p);

    Vector e[ 3 ];
    e[ 0 ] = v[ 1 ];
    e[ 0 ].Sub(v[ 0 ]);
    e[ 1 ] = v[ 2 ];
    e[ 1 ].Sub(v[ 1 ]);
    e[ 2 ] = v[ 0 ];
    e[ 2 ].Sub(v[ 2 ]);
    float fA00 = e[ 0 ].MagnitudeSqr();
    float fA01 = e[ 0 ].Dot( e[ 1 ] );
    float fA11 = e[ 1 ].MagnitudeSqr();
    float fB0 = kDiff.Dot( e[ 0 ] );
    float fB1 = kDiff.Dot( e[ 1 ] );
    float fC = kDiff.MagnitudeSqr();
    float fDet = ::fabsf(fA00*fA11-fA01*fA01);
    float fS = fA01*fB1-fA11*fB0;
    float fT = fA01*fB0-fA00*fB1;
    float fSqrDist;

    if ( fS + fT <= fDet )
    {
        if ( fS < 0.0f )
        {
            if ( fT < 0.0f )  // region 4
            {
                if ( fB0 < 0.0f )
                {
                    fT = 0.0f;
                    if ( -fB0 >= fA00 )
                    {
                        fS = 1.0f;
                        fSqrDist = fA00+2.0f*fB0+fC;
                    }
                    else
                    {
                        fS = -fB0/fA00;
                        fSqrDist = fB0*fS+fC;
                    }
                }
                else
                {
                    fS = 0.0f;
                    if ( fB1 >= 0.0f )
                    {
                        fT = 0.0f;
                        fSqrDist = fC;
                    }
                    else if ( -fB1 >= fA11 )
                    {
                        fT = 1.0f;
                        fSqrDist = fA11+2.0f*fB1+fC;
                    }
                    else
                    {
                        fT = -fB1/fA11;
                        fSqrDist = fB1*fT+fC;
                    }
                }
            }
            else  // region 3
            {
                fS = 0.0f;
                if ( fB1 >= 0.0f )
                {
                    fT = 0.0f;
                    fSqrDist = fC;
                }
                else if ( -fB1 >= fA11 )
                {
                    fT = 1.0f;
                    fSqrDist = fA11+2.0f*fB1+fC;
                }
                else
                {
                    fT = -fB1/fA11;
                    fSqrDist = fB1*fT+fC;
                }
            }
        }
        else if ( fT < 0.0f )  // region 5
        {
            fT = 0.0f;
            if ( fB0 >= 0.0f )
            {
                fS = 0.0f;
                fSqrDist = fC;
            }
            else if ( -fB0 >= fA00 )
            {
                fS = 1.0f;
                fSqrDist = fA00+2.0f*fB0+fC;
            }
            else
            {
                fS = -fB0/fA00;
                fSqrDist = fB0*fS+fC;
            }
        }
        else  // region 0
        {
            // minimum at interior point
            float fInvDet = 1.0f/fDet;
            fS *= fInvDet;
            fT *= fInvDet;
            fSqrDist = fS*(fA00*fS+fA01*fT+2.0f*fB0) +
                fT*(fA01*fS+fA11*fT+2.0f*fB1)+fC;
        }
    }
    else
    {
        float fTmp0, fTmp1, fNumer, fDenom;

        if ( fS < 0.0f )  // region 2
        {
            fTmp0 = fA01 + fB0;
            fTmp1 = fA11 + fB1;
            if ( fTmp1 > fTmp0 )
            {
                fNumer = fTmp1 - fTmp0;
                fDenom = fA00-2.0f*fA01+fA11;
                if ( fNumer >= fDenom )
                {
                    fS = 1.0f;
                    fT = 0.0f;
                    fSqrDist = fA00+2.0f*fB0+fC;
                }
                else
                {
                    fS = fNumer/fDenom;
                    fT = 1.0f - fS;
                    fSqrDist = fS*(fA00*fS+fA01*fT+2.0f*fB0) +
                        fT*(fA01*fS+fA11*fT+2.0f*fB1)+fC;
                }
            }
            else
            {
                fS = 0.0f;
                if ( fTmp1 <= 0.0f )
                {
                    fT = 1.0f;
                    fSqrDist = fA11+2.0f*fB1+fC;
                }
                else if ( fB1 >= 0.0f )
                {
                    fT = 0.0f;
                    fSqrDist = fC;
                }
                else
                {
                    fT = -fB1/fA11;
                    fSqrDist = fB1*fT+fC;
                }
            }
        }
        else if ( fT < 0.0f )  // region 6
        {
            fTmp0 = fA01 + fB1;
            fTmp1 = fA00 + fB0;
            if ( fTmp1 > fTmp0 )
            {
                fNumer = fTmp1 - fTmp0;
                fDenom = fA00-2.0f*fA01+fA11;
                if ( fNumer >= fDenom )
                {
                    fT = 1.0f;
                    fS = 0.0f;
                    fSqrDist = fA11+2.0f*fB1+fC;
                }
                else
                {
                    fT = fNumer/fDenom;
                    fS = 1.0f - fT;
                    fSqrDist = fS*(fA00*fS+fA01*fT+2.0f*fB0) +
                        fT*(fA01*fS+fA11*fT+2.0f*fB1)+fC;
                }
            }
            else
            {
                fT = 0.0f;
                if ( fTmp1 <= 0.0f )
                {
                    fS = 1.0f;
                    fSqrDist = fA00+2.0f*fB0+fC;
                }
                else if ( fB0 >= 0.0f )
                {
                    fS = 0.0f;
                    fSqrDist = fC;
                }
                else
                {
                    fS = -fB0/fA00;
                    fSqrDist = fB0*fS+fC;
                }
            }
        }
        else  // region 1
        {
            fNumer = fA11 + fB1 - fA01 - fB0;
            if ( fNumer <= 0.0f )
            {
                fS = 0.0f;
                fT = 1.0f;
                fSqrDist = fA11+2.0f*fB1+fC;
            }
            else
            {
                fDenom = fA00-2.0f*fA01+fA11;
                if ( fNumer >= fDenom )
                {
                    fS = 1.0f;
                    fT = 0.0f;
                    fSqrDist = fA00+2.0f*fB0+fC;
                }
                else
                {
                    fS = fNumer/fDenom;
                    fT = 1.0f - fS;
                    fSqrDist = fS*(fA00*fS+fA01*fT+2.0f*fB0) +
                        fT*(fA01*fS+fA11*fT+2.0f*fB1)+fC;
                }
            }
        }
    }
    return ::fabsf(fSqrDist);
}

//-----------------------------------------------------------------------------
// Projection
//
// Synopsis:    helper function for the triangle intersection code
//-----------------------------------------------------------------------------
static void Projection (const Vector& rkD, const Vector akV[3],
    float& rfMin, float& rfMax)
{
    float afDot[3] = { rkD.Dot(akV[0]), rkD.Dot(akV[1]), rkD.Dot(akV[2]) };

    rfMin = afDot[0];
    rfMax = rfMin;

    if ( afDot[1] < rfMin )
        rfMin = afDot[1];
    else if ( afDot[1] > rfMax )
        rfMax = afDot[1];

    if ( afDot[2] < rfMin )
        rfMin = afDot[2];
    else if ( afDot[2] > rfMax )
        rfMax = afDot[2];
}

//-----------------------------------------------------------------------------
// Intersect
//
// Synopsis:    test if two triangles intersect
//
// Parameters:  right - the triangle that is to be tested for intesection 
//              against. (NB- not a right triangle.
//-----------------------------------------------------------------------------
bool Triangle::Intersect( const Triangle &right ) const
{
    Vector kDir;
    float fUMin, fUMax, fVMin, fVMax;
    int i0, i1;

    // direction N
    Vector akE[3];
    akE[0].Sub(v[1], v[0]);
    akE[1].Sub(v[2], v[1]);
    akE[2].Sub(v[0], v[2]);
    Vector kN;
    kN.CrossProduct(akE[0], akE[1]);
    float fNdU0 = kN.Dot(right.v[0]);
    Projection(kN,right.v,fVMin,fVMax);
    if ( fNdU0 < fVMin || fNdU0 > fVMax )
        return false;

    // direction M
    Vector akF[3];
    akF[0].Sub(right.v[1], right.v[0]);
    akF[1].Sub(right.v[2], right.v[1]);
    akF[2].Sub(right.v[0], right.v[2]);
    Vector kM;
    kM.CrossProduct(akF[0], akF[1]);

    const float fEpsilon = 1e-06f;  // cutoff: sin(Angle(N,M)) < epsilon
    Vector kNxM;
    kNxM.CrossProduct(kN, kM);
    if ( kNxM.Dot(kNxM) >= fEpsilon*(kN.Dot(kN)*kM.Dot(kM)) )
    {
        // triangles are not parallel
        float fMdV0 = kM.Dot(right.v[0]);
        Projection(kM,v,fUMin,fUMax);
        if ( fMdV0 < fUMin || fMdV0 > fUMax )
            return false;

        // directions E[i0]xF[i1]
        for (i1 = 0; i1 < 3; i1++)
        {
            for (i0 = 0; i0 < 3; i0++)
            {
                kDir.CrossProduct(akE[i0], akF[i1]);
                Projection(kDir,v,fUMin,fUMax);
                Projection(kDir,right.v,fVMin,fVMax);
                if ( fUMax < fVMin || fVMax < fUMin )
                    return false;
            }
        }
    }
    else  // triangles are parallel (and, in fact, coplanar)
    {
        // directions NxE[i0]
        for (i0 = 0; i0 < 3; i0++)
        {
            kDir.CrossProduct(kN, akE[i0]);
            Projection(kDir,v,fUMin,fUMax);
            Projection(kDir,right.v,fVMin,fVMax);
            if ( fUMax < fVMin || fVMax < fUMin )
                return false;
        }

        // directions NxF[i1]
        for (i1 = 0; i1 < 3; i1++)
        {
            kDir.CrossProduct(kM, akF[i1]);
            Projection(kDir,v,fUMin,fUMax);
            Projection(kDir,right.v,fVMin,fVMax);
            if ( fUMax < fVMin || fVMax < fUMin )
                return false;
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
// I s I n t e r s e c t i o n P o s s i b l e
//
// Synopsis:    does a rudimentary check whether or not the triangles can intersect
// Returns:     unsigned int - the number of faces
//-----------------------------------------------------------------------------
bool Triangle::IsIntersectionPossible( const Triangle& right ) const
{
    Vector t;

    t.Sub(v[1], v[0]);
    float d0 = t.MagnitudeSqr();
    t.Sub(v[2], v[0]);
    float d1 = t.MagnitudeSqr();
    float maxDistance = Max( d0, d1 );
    Sphere s( v[ 0 ], Sqrt( maxDistance ) );

    t.Sub(right.v[ 1 ], right.v[ 0 ]);
    float rd0 = t.MagnitudeSqr();
    t.Sub(right.v[ 2 ], right.v[ 0 ]);
    float rd1 = t.MagnitudeSqr();
    float rMaxDistance = Max( rd0, rd1 );
    Sphere rs( right.v[ 0 ], Sqrt( rMaxDistance ) );

    return s.Intersects( rs );
}


#if 0

#include <assert.h>

//-----------------------------------------------------------------------------
// T e s t T r i a n g l e D i s t a n c e
//
// Synopsis:    coverage test for distance to a triangle.  Will tolerate 
//              0.1% error
//-----------------------------------------------------------------------------
static 
void TestTriangleDistance()
{
    //
    // Test suite for distance from a triangle code
    //
    Vector p0( -1.0f, 0.0f, 0.0f );
    Vector p1( 1.0f, 0.0f, 0.0f );
    Vector p2( 0.0f, 1.0f, 0.0f );
    Triangle t( p0, p1, p2 );

    int i;
    int j;
    float maxError = 0.0f;
    for( i = 1; i < 100; i++ )
    {
        float x = ( i / 100.0f );
        for( j = 1; j < 100; j++ )
        {
            float y = ( j / 100.0f );
            if( x + y > 1.0f )
            {
                continue;
            }
            float d = t.Distance( Vector( x, y, 1 ) );        
            float error = d - 1.0f;
            if( error > maxError )
            {
                maxError = error;
            }
        }
    }
    float tolerance = 0.001f;
    if( maxError < tolerance )
    {
        //test passed
    }
    else
    {
        // test failed
        assert(false && "Test failed, error is above tolerance\n");
    }
}

//-----------------------------------------------------------------------------
// T e s t T r i a n g l e I n t e r s e c t i o n
//
// Synopsis:    coverage test for intersection between two triangles
//-----------------------------------------------------------------------------
static 
void TestTriangleIntersection()
{
    //
    // Test suite for triangle intersection code
    //
    Vector v00( 0.0f, 0.0f, 0.0f );
    Vector v01( 1.0f, 0.0f, 0.0f );
    Vector v02( 0.0f, 1.0f, 0.0f );
    Triangle t0( v00, v01, v02 );

    Vector v10( 0.0f, 0.0f, 1.0f );
    Vector v11( 1.0f, 0.0f, 1.0f );
    Vector v12( 0.0f, 1.0f, 1.0f );
    Triangle t1( v10, v11, v12 );

    bool intersect = t0.Intersect( t1 );
}

#endif

} // end of namespace
