/*===========================================================================
   File:            spline.hpp

   Description:     Spline evaluators

   Owner:           Jean-Luc Duprat
   Creation:        Mar 18, 2002

   Revisions:       Mar 18, 2002
                    Created the Spline Class to evaluate a single segment spline
                    It can evaluate BSpline, Bezier and Hermite curves

                    May 06, 2002
                    Added the SplineCurve Class for multi-segment curves
                    
                    June 04, 2002
                    Added support for evaluating 1st and 2nd derivative

                    Aug 28 2002
                    Added support for computing the ArcLength of a curve, and converting from ArcLength 
                    to implicit parameterization of the curve

                    Sep 05 2002
                    Introduced a second evaluator in SplineCurve, used exclusively for fwd differencing
                    This simplifies class usage as you can now call Evaluate() in the middle of the Forward() loop

   Copyright (c)2002 Radical Entertainment, Ltd. All rights reserved.
===========================================================================*/

#ifndef _RADMATH_SPLINE_HPP
#define _RADMATH_SPLINE_HPP

#include <radmath/matrix.hpp>

namespace RadicalMathLibrary
{

// This class evaluates a single spline segment using several basis functions.
// It does not attempt to manage control vertices for curves with more than 
// one segment, see SplineCurve below...

// There are 2 ways to evaluate a curve:
//
// SetCntrlMesh()               // cost: one 4x4 matrix multiply
// Evaluate(t1)                 // cost: one Vector4 transform
// Evaluate(t2)

// SetCntrlMesh()               // cost: one 4x4 matrix multiply
// InitForwardDifferencing()    // cost: one Vector4 transform
// Forward()                    // cost: 3 Vector adds

// The first calling pattern is to be used to evaluate the spline at possibly several positions 
// within the interval [0.0, 1.0]; while the second calling pattern is to be used to evaluate the
// spline starting at 0.0 and step alomg the curve.  Note that there are numerical error propagation 
// issues with forward differencing...  Look at the comments in the code.

// SplineCurve::GetLength() is very expensive!  It computes an integral which requires calling Evaluate() 
// about 5 times per curve segment.  GetLength() caches the length of each segment independently; so 
// if it is called repeatedly on the knots of a curve it is still quite cheap after the initial overhead.
// SplineCurve::ArcConvert() is the worst since it uses Newton iteration to search for the right position 
// by repeatedly calling GetLength().


class Spline // this class evaluates a single uniform cubic non-rational spline segment
{
public:
    Spline()  { /**/ }
    Spline(const Spline&);
    ~Spline() { /**/ }

    Spline& operator=(const Spline&);
    
    // Spline basis to use for curve evaluation
    enum Basis { 
        // basis functions
        BSpline, 
        Bezier,
        Hermite,  // Order is P0, D0 (derivative at P0), P1, D1 (derivative at P1)

        // first derivative
        DBSpline, 
        DBezier,
        DHermite,  // Order is P0, D0 (derivative at P0), P1, D1 (derivative at P1)

        // second derivative
        DDBSpline, 
        DDBezier,
        DDHermite  // Order is P0, D0 (derivative at P0), P1, D1 (derivative at P1)
    };

    // initialization
    void SetCntrlMesh(const Basis basis, const Vector&  cv0, const Vector&  cv1, const Vector&  cv2, const Vector&  cv3);
    void SetCntrlMesh(const Basis basis, const Vector4& cv0, const Vector4& cv1, const Vector4& cv2, const Vector4& cv3);
    void SetCntrlMesh(const Basis basis, const Matrix&  cv);
    
    // evaluation
    Vector Evaluate(const float t);  // t in [0, 1]
    Vector InitForwardDifferencing(unsigned int numSteps);
    Vector Forward();

private:
    // basis functions
    static const Matrix MBSpline;
    static const Matrix MBezier;
    static const Matrix MHermite;
    
    // first derivative of basis functions
    static const Matrix MBSplineD1;
    static const Matrix MBezierD1;
    static const Matrix MHermiteD1;

    // second derivative of basis functions
    static const Matrix MBSplineD2;
    static const Matrix MBezierD2;
    static const Matrix MHermiteD2;

    Matrix pm;           // cache for Basis * CVs

    Vector4 Q;           // Value of previous evaluation
    Vector4 Dx, Dy, Dz;  // Delta vectors for fwd differencing
}; // Spline




// This class is similar to the one above, except that it handles curves with multiple segments
// and manages the state of the spline evaluator automatically to avoid reloading it unecessarily
class SplineCurve
{
public:
    SplineCurve(int numCVs=0, Spline::Basis basis=Spline::BSpline, bool closed=false);
    SplineCurve(const SplineCurve&);
    ~SplineCurve();

    SplineCurve& operator=(const SplineCurve&);

    // initialization
    void SetBasis(Spline::Basis b);
    void SetClosed(bool c);             // whether the spline is open or closed 
    void SetNumVertices(unsigned int numCVs);
    void SetCntrlVertex(unsigned int i, const Vector& cv);
    void SetCntrlVertex(unsigned int i, const Vector4& cv);

    const Vector& GetCntrlVertex(unsigned int i) const;

    // curve properties
    Spline::Basis GetBasis() const { return basis; }
    bool          GetClosed() const { return closed; }
    unsigned int  GetNumVertices() const { return numCVs; }
    float         GetEndParam() const;    // max value of t at which curve is defined (different for open and closed curves)
    unsigned int  GetNumSegments() const; // number of segments along the curve
    const Vector& GetCntrlVertex(unsigned int i);

    // evaluation
    Vector GetKnot(unsigned int i);  // i in [0, GetNumSegments()]
    Vector Evaluate(const float t);  // t in [0, GetCurveEndParam()]
    Vector InitForwardDifferencing(unsigned int numSteps, unsigned int startKnot = 0);
    Vector Forward();

    // arc length (these functions are very expensive -- see comments above)
    float  GetLength();                     // arc length of the whole curve
    float  GetLength(const float t);        // arc length up to an arbitrary position along the curve
    float  GetSegmentLength(unsigned int i);// arc length of the ith segment
    float  ArcConvert(const float s);       // converts arc parameter s to natural parameter t

private:  

    // there are two evaluators for SplineCurve: one that is used for Evaluate() -- and hence GetLength() and ArcConvert() --
    // and another one dedicated exclusively to forward differencing.  The reason is that the first three calls only need their
    // state until the call returns, while forward differencing needs its state preserved until the end of the Forward() loop.
    // To have only one evaluator would preclude calling any of the above functions inside the Forward() loop, a common source of errors.
    typedef enum { CURVE, FORWARD_DIFFERENCING } Evaluator;
    bool  LoadEvaluator(Evaluator idx, const float t);
    int     start[2];    // first vertex (of 4) stored in evaluator
    Spline  Q[2];        // segment evaluator

    bool closed;         // whether to loop at the end of the segment   
    Spline::Basis basis; // spline basis to be used
    unsigned int numCVs; // number of control vertices
    Vector4 *CVs;        // array of control vertices
    float *segLengths;   // segment lengths

    int steps;           // steps taken by forward differencing
    int numSteps;        // steps per segment in forward differencing
}; // SplineCurve


}

namespace rmt = RadicalMathLibrary;

#endif  // _RADMATH_SPLINE_HPP