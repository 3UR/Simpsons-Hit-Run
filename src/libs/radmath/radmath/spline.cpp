/*===========================================================================
   File:            spline.cpp

   Description:     Spline evaluators

   Owner:           Jean-Luc Duprat
   Creation:        Mar 18, 2002.

   Revisions:       May 06, 2002
                    Added the SplineCurve Class for multi-segment curves
                    
                    June 04, 2002
                    Added support for evaluating 1st and 2nd derivative

   Copyright (c)2002 Radical Entertainment, Ltd. All rights reserved.
===========================================================================*/
 
#include <math.h>
#include <assert.h>
#include <radmath/spline.hpp>
#include <radmath/numerical.hpp>

using namespace RadicalMathLibrary;


/////////////////////////////////////////////////////////////////////////////////
// class Spline
/////////////////////////////////////////////////////////////////////////////////

// Note: these matrices are different from those in Foley-VanDam because 
//       the curve is parametrized from [0, 1].  This gives us a slightly more
//       efficient evaluator... Details in Bartels-Beatty-Barsky 87.

// Curve
const Matrix Spline::MBSpline( 1.0f/6.0f, 4.0f/6.0f, 1.0f/6.0f, 0.0f/6.0f,
                              -3.0f/6.0f, 0.0f/6.0f, 3.0f/6.0f, 0.0f/6.0f,
                               3.0f/6.0f,-6.0f/6.0f, 3.0f/6.0f, 0.0f/6.0f,
                              -1.0f/6.0f, 3.0f/6.0f,-3.0f/6.0f, 1.0f/6.0f);

const Matrix Spline::MBezier ( 1.0f, 0.0f, 0.0f, 0.0f,
                              -3.0f, 3.0f, 0.0f, 0.0f,
                               3.0f,-6.0f, 3.0f, 0.0f,
                              -1.0f, 3.0f,-3.0f, 1.0f);

const Matrix Spline::MHermite( 1.0f, 0.0f, 0.0f, 0.0f,
                               0.0f, 1.0f, 0.0f, 0.0f,
                              -3.0f,-2.0f, 3.0f,-1.0f,
                               2.0f, 1.0f,-2.0f, 1.0f);

// First Derivative
const Matrix Spline::MBSplineD1(-3.0f/6.0f,  0.0f/6.0f, 3.0f/6.0f, 0.0f/6.0f,
                                 6.0f/6.0f,-12.0f/6.0f, 6.0f/6.0f, 0.0f/6.0f,
                                -3.0f/6.0f,  9.0f/6.0f,-9.0f/6.0f, 3.0f/6.0f,
                                 0.0f/6.0f,  0.0f/6.0f, 0.0f/6.0f, 0.0f/6.0f);

const Matrix Spline::MBezierD1 (-3.0f,  3.0f, 0.0f, 0.0f,
                                 6.0f,-12.0f, 6.0f, 0.0f,
                                -3.0f,  9.0f,-9.0f, 3.0f,
                                 0.0f,  0.0f, 0.0f, 0.0f);

const Matrix Spline::MHermiteD1( 0.0f, 1.0f, 0.0f, 0.0f,
                                -6.0f,-4.0f, 6.0f,-2.0f,
                                 6.0f, 3.0f,-6.0f, 3.0f,
                                 0.0f, 0.0f, 0.0f, 0.0f);

// Second Derivative
const Matrix Spline::MBSplineD2( 6.0f/6.0f,-12.0f/6.0f,  6.0f/6.0f, 0.0f/6.0f,
                                 6.0f/6.0f, 18.0f/6.0f,-18.0f/6.0f, 6.0f/6.0f,
                                 0.0f/6.0f,  0.0f/6.0f,  0.0f/6.0f, 0.0f/6.0f,
                                 0.0f/6.0f,  0.0f/6.0f,  0.0f/6.0f, 0.0f/6.0f);

const Matrix Spline::MBezierD2 ( 6.0f,-12.0f,  6.0f, 0.0f,
                                -6.0f, 18.0f,-18.0f, 6.0f,
                                 0.0f,  0.0f,  0.0f, 0.0f,
                                 0.0f,  0.0f,  0.0f, 0.0f);

const Matrix Spline::MHermiteD2(-6.0f,-4.0f,  6.0f,-2.0f,
                                12.0f, 6.0f,-12.0f, 6.0f,
                                 0.0f, 0.0f,  0.0f, 0.0f,
                                 0.0f, 0.0f,  0.0f, 0.0f);


Spline::Spline(const Spline&s)
{
    pm=s.pm;
    Q=s.Q;

    Dx=s.Dx;
    Dy=s.Dy;
    Dz=s.Dz;
}


Spline& 
Spline::operator=(const Spline& s)
{
    pm=s.pm;
    Q=s.Q;

    Dx=s.Dx;
    Dy=s.Dy;
    Dz=s.Dz;

    return *this;
}


void
Spline::SetCntrlMesh(const Basis basis, const Vector&  cv0, const Vector&  cv1, const Vector&  cv2, const Vector&  cv3)
{
    Matrix CVs;
    CVs.Row(0) = cv0;
    CVs.m[0][3] = 1.0f;
    CVs.Row(1) = cv1;
    CVs.m[1][3] = 1.0f;
    CVs.Row(2) = cv2;
    CVs.m[2][3] = 1.0f;
    CVs.Row(3) = cv3;
    CVs.m[3][3] = 1.0f;
    
    switch(basis)
    {
    case BSpline:
        pm.MultFull(MBSpline, CVs);
        break;
    case Bezier:
        pm.MultFull(MBezier, CVs);
        break;
    case Hermite:
        pm.MultFull(MHermite, CVs);
        break;
    case DBSpline:
        pm.MultFull(MBSplineD1, CVs);
        break;
    case DBezier:
        pm.MultFull(MBezierD1, CVs);
        break;
    case DHermite:
        pm.MultFull(MHermiteD1, CVs);
        break;
    case DDBSpline:
        pm.MultFull(MBSplineD2, CVs);
        break;
    case DDBezier:
        pm.MultFull(MBezierD2, CVs);
        break;
    case DDHermite:
        pm.MultFull(MHermiteD2, CVs);
        break;
    }
}


void 
Spline::SetCntrlMesh(const Basis basis, const Vector4& cv0, const Vector4& cv1, const Vector4& cv2, const Vector4& cv3)
{
    Matrix CVs;
    CVs.Row4(0) = cv0;
    CVs.Row4(1) = cv1;
    CVs.Row4(2) = cv2;
    CVs.Row4(3) = cv3;

    switch(basis)
    {
    case BSpline:
        pm.MultFull(MBSpline, CVs);
        break;
    case Bezier:
        pm.MultFull(MBezier, CVs);
        break;
    case Hermite:
        pm.MultFull(MHermite, CVs);
        break;
    case DBSpline:
        pm.MultFull(MBSplineD1, CVs);
        break;
    case DBezier:
        pm.MultFull(MBezierD1, CVs);
        break;
    case DHermite:
        pm.MultFull(MHermiteD1, CVs);
        break;
    case DDBSpline:
        pm.MultFull(MBSplineD2, CVs);
        break;
    case DDBezier:
        pm.MultFull(MBezierD2, CVs);
        break;
    case DDHermite:
        pm.MultFull(MHermiteD2, CVs);
        break;
    }
}


void 
Spline::SetCntrlMesh(const Basis basis, const Matrix&  cv)
{
    switch(basis)
    {
    case BSpline:
        pm.MultFull(MBSpline, cv);
        break;
    case Bezier:
        pm.MultFull(MBezier, cv);
        break;
    case Hermite:
        pm.MultFull(MHermite, cv);
        break;
    case DBSpline:
        pm.MultFull(MBSplineD1, cv);
        break;
    case DBezier:
        pm.MultFull(MBezierD1, cv);
        break;
    case DHermite:
        pm.MultFull(MHermiteD1, cv);
        break;
    case DDBSpline:
        pm.MultFull(MBSplineD2, cv);
        break;
    case DDBezier:
        pm.MultFull(MBezierD2, cv);
        break;
    case DDHermite:
        pm.MultFull(MHermiteD2, cv);
        break;
    }
}
    

Vector
Spline::Evaluate(const float t)
{
    float tt = t;
#ifdef RAD_RELEASE
    // in release, make sure you can't step out of interval
    if ( tt < 0.0f ) tt = 0.0f;
    if ( tt > 1.0f ) tt = 1.0f;
#endif

    assert(t>=0.0f && t<=1.0f);

    Q = Vector4(1, tt, tt*tt, tt*tt*tt);
    Q.Transform(pm);
    
    return Q;
}


// Watch out as error accumulates every time this is called.
// It might become significant for small enough dt's.  This implementation
// is ok to float precision up to about dt = 1/1024.f
Vector
Spline::InitForwardDifferencing(unsigned int numSteps)
{
    float dt = 1.0f/numSteps;
    Matrix delta(0.0f, dt*dt*dt, 6*dt*dt*dt, 6*dt*dt*dt,
                 0.0f, dt*dt,   2*dt*dt,     0.0f,
                 0.0f, dt,      0.0f,        0.0f,
                 1.0f, 0.0f,    0.0f,        0.0f);

    // Extract the columns of the matrix
    Dx = Vector4(pm.m[3][0], pm.m[2][0], pm.m[1][0], pm.m[0][0]);  // Col 0
    Dy = Vector4(pm.m[3][1], pm.m[2][1], pm.m[1][1], pm.m[0][1]);  // Col 1
    Dz = Vector4(pm.m[3][2], pm.m[2][2], pm.m[1][2], pm.m[0][2]);  // Col 2

    Q = Vector4(Dx.w, Dy.w, Dz.w, 1.0f);

    Dx.Transform(delta);
    Dy.Transform(delta);
    Dz.Transform(delta);

    return Q;
}


Vector
Spline::Forward()
{
    // Note: this lets you step out of the [0,1] range where the spline is defined.  Careful
    Q.Add(Vector(Dx.y, Dy.y, Dz.y));
    Dx.y+=Dx.z; Dy.y+=Dy.z; Dz.y+=Dz.z;
    Dx.z+=Dx.w; Dy.z+=Dy.w; Dz.z+=Dz.w;

    return Q;
}


/////////////////////////////////////////////////////////////////////////////////
// class SplineCurve
/////////////////////////////////////////////////////////////////////////////////


SplineCurve::SplineCurve(int n, Spline::Basis b, bool c)
{
    assert(n==0 || n>=4 || c == true); // it doesn't make sense to evaluate an open cubic spline if you have less than 4 CVs

    numCVs = n;
    basis = b;
    closed = c;
    start[0] = -1;
    start[1] = -1;
    CVs = 0;
    segLengths = 0;

    if(numCVs>0)
        CVs = new Vector4[numCVs];
}


SplineCurve::SplineCurve(const SplineCurve& s)
{
    start[0] = s.start[0];
    start[1] = s.start[1];
    closed= s.closed;
    Q[0] = s.Q[0];
    Q[1] = s.Q[1];
    basis = s.basis;
    segLengths = 0;

    delete[] CVs;

    unsigned int i;
    numCVs = s.numCVs;
    if(numCVs>0)
        CVs = new Vector4[numCVs];
    for(i=0; i<numCVs; i++)
        CVs[i] = s.CVs[i];

    unsigned int numSeg = GetNumSegments()+1; // last entry holds total for the curve
    if(s.segLengths)
    {
        segLengths = new float[numSeg];
        for(i=0; i<numSeg; i++)
            segLengths[i]=s.segLengths[i];
    }

    steps = s.steps;
    numSteps = s.numSteps;
}


SplineCurve::~SplineCurve()
{
    delete[] CVs;
    delete[] segLengths;
}


SplineCurve& SplineCurve::operator=(const SplineCurve& s)
{
    start[0] = s.start[0];
    start[1] = s.start[1];
    closed= s.closed;
    Q[0] = s.Q[0];
    Q[1] = s.Q[1];
    basis = s.basis;

    delete[] CVs;
    delete[] segLengths;
    
    unsigned int i;
    numCVs = s.numCVs;
    if(numCVs>0)
        CVs = new Vector4[numCVs];
    for(i=0; i<numCVs; i++)
        CVs[i] = s.CVs[i];

    unsigned int numSeg = GetNumSegments()+1; // last entry holds total for the curve
    if(s.segLengths)
    {
        segLengths = new float[numSeg];
        for(i=0; i<numSeg; i++)
            segLengths[i]=s.segLengths[i];
    }

    steps = s.steps;
    numSteps = s.numSteps;
    
    return *this;
}


void 
SplineCurve::SetBasis(Spline::Basis b)
{
    basis = b;
    start[0] = -1;  // invalidate state of evaluators
    start[1] = -1;
    delete[] segLengths;
    segLengths = 0; // invalidate computed lengths
}


void 
SplineCurve::SetClosed(bool c)
{
    if(c == closed)
        return;

    // invalidate computed lengths
    if(segLengths)
    {
        if(closed)
        {
            // was closed, now open
            float *newSegLengths = new float[numCVs-2];

            // previous segLengths are still valid, only total length isn't
            unsigned int i;
            for(i=0; i<numCVs-3; i++)
                newSegLengths[i] = segLengths[i];
            newSegLengths[numCVs-3] = -1.0f;

            delete[] segLengths;
            segLengths = newSegLengths;
        }
        else
        {
            // was open, now closed
            float *newSegLengths = new float[numCVs+1];

            // previous segLengths are still valid
            unsigned int i;
            for(i=0; i<numCVs-3; i++)
                newSegLengths[i] = segLengths[i];
            for(i=numCVs-3; i<numCVs+1; i++)
                newSegLengths[i] = -1.0f;

            delete[] segLengths;
            segLengths = newSegLengths;
        }
    }

    closed = c;
}


void 
SplineCurve::SetNumVertices(unsigned int n)
{
    start[0] = -1;  // invalidate state of evaluators
    start[1] = -1;

    if(numCVs==n)
        return;
    assert(n==0 || n>=4 || closed==true); // it doesn't make sense to evaluate an open cubic spline if you have less than 4 CVs

    delete[] CVs; 
    delete[] segLengths; 
    CVs = 0;
    segLengths = 0;

    numCVs = n;
    if(numCVs>0)
        CVs = new Vector4[numCVs];
}


const Vector&
SplineCurve::GetCntrlVertex(unsigned int idx) const
{
    assert(idx>=0 && idx<numCVs);
    return CVs[idx];
}


void 
SplineCurve::SetCntrlVertex(unsigned int idx, const Vector& cv)
{
    assert(idx>=0 && idx<numCVs);
    CVs[idx] = cv;
    start[0] = -1;  // invalidate state of evaluators
    start[1] = -1;

    if(segLengths)
    {
        // moving one vertex invalidates the lengths computed for 4 segments
        unsigned int min = Min(0u, idx-3);
        for(unsigned int i=min; i<idx; i++)
            segLengths[i]=-1.0f;
        segLengths[GetNumSegments()]=-1.0f; // total length needs to be recomputed
    }

}


void 
SplineCurve::SetCntrlVertex(unsigned int idx, const Vector4& cv)
{
    assert(idx>=0 && idx<numCVs);
    CVs[idx] = cv;
    start[0] = -1;  // invalidate state of evaluators
    start[1] = -1;
}


float
SplineCurve::GetEndParam() const 
{ 
    assert(numCVs>=4 || closed==true);
    return float(closed?numCVs:numCVs-3); 
}


unsigned int
SplineCurve::GetNumSegments() const 
{ 
    if(numCVs<4 && closed == false)
        return 0;

    return closed?numCVs:numCVs-3; 
}


const Vector& 
SplineCurve::GetCntrlVertex(unsigned int i)
{
    assert(i>=0 && i<numCVs);
    return CVs[i];
}


Vector 
SplineCurve::GetKnot(unsigned int i)
{
    assert(i>=0 && i<=GetNumSegments());  // stay inside area where curve is defined

    return Evaluate(float(i));   
}


bool
SplineCurve::LoadEvaluator(Evaluator idx, const float t)
{
    assert(idx==0 || idx==1);
    
    int s = int(t);
    if(s==start[idx])
        return false;  // evaluator's state is fine

    // moved to a new segment, update state
    Q[idx].SetCntrlMesh(basis, CVs[s%numCVs], CVs[(s+1)%numCVs], CVs[(s+2)%numCVs], CVs[(s+3)%numCVs]);
    start[idx] = s;
    return true;
}


Vector
SplineCurve::Evaluate(const float t)
{
    float tt = t;
#ifdef RAD_RELEASE
    // in release, make sure you can't step out of interval
    if ( tt < 0.0f ) tt = 0.0f;
    if ( tt > GetEndParam() ) tt = GetEndParam();
#endif

    assert(numCVs>=4 || closed==true);
    assert(t>=0.0f && t<=GetEndParam());  // stay inside area where curve is defined

    LoadEvaluator(CURVE, tt);
    return Q[CURVE].Evaluate(tt-start[CURVE]); // Q is parametrized from [0,1]
}


Vector 
SplineCurve::InitForwardDifferencing(unsigned int ns, unsigned int sk)
{
#ifdef RAD_RELEASE
    // in release, make sure you can't step out of interval
    if ( sk > GetEndParam() ) sk = unsigned(GetEndParam());
#endif

    assert(numCVs>=4 || closed==true);
    assert(sk>=0 && sk<=GetEndParam());

    steps=ns*sk;
    numSteps = ns;
    LoadEvaluator(FORWARD_DIFFERENCING, float(sk));
    return Q[FORWARD_DIFFERENCING].InitForwardDifferencing(numSteps);
}


Vector 
SplineCurve::Forward()
{
    steps++;
    float t = steps/float(numSteps);
    if(closed && t==GetEndParam())
    {
        // wrap a closed curve
        steps = 0;
        t = 0.0f;
    }

#ifdef RAD_RELEASE
    // in release, make sure you can't step out of interval
    if ( t < 0.0f ) t = 0.0f;
    if ( t > GetEndParam() ) t = GetEndParam();
#endif

    assert(t>=0.0f && t<=GetEndParam());  // avoid stepping outside of area where curve is not defined

    if(LoadEvaluator(FORWARD_DIFFERENCING, t)) // we may need to reload evaluator if we cross segment boundary
        return Q[FORWARD_DIFFERENCING].InitForwardDifferencing(numSteps); // if so we need to restart the forward differencing
    else 
        return Q[FORWARD_DIFFERENCING].Forward();
}


float DoEvalSpeed(float t, void* o)
{
    Spline *s=static_cast<Spline*>(o); // this is the derivative of the curve being evaluated
    Vector velocity = s->Evaluate(t);
    float speed = velocity.Magnitude();
    return speed;
}


float 
IntegrateSpeed(const float t, Spline& derivative)
{
    assert(t>=0.0f && t<=1.0f);
    
    // integrate speed along curve over time to get length
    Integrate ir;
    return ir.GaussianQuadrature(0.0f, t, DoEvalSpeed, &derivative);
}

float 
SplineCurve::GetSegmentLength(unsigned int seg)
{
    assert(numCVs>=4 || closed==true);
    unsigned int numSeg = GetNumSegments();
    assert(seg>=0 && seg<=numSeg);
    
    if(segLengths && segLengths[seg]>=0.0f)
        return segLengths[seg];

    if(!segLengths)
    {
        segLengths = new float[numSeg+1]; // last entry holds total for the curve
        for(unsigned int i=0; i<numSeg+1; i++)
            segLengths[i]=-1.0f;
    }

    LoadEvaluator(CURVE, float(seg));
    Spline derivative;
    derivative.SetCntrlMesh(Spline::Basis(int(basis)+3), CVs[seg%numCVs], CVs[(seg+1)%numCVs], CVs[(seg+2)%numCVs], CVs[(seg+3)%numCVs]);
    segLengths[seg] = IntegrateSpeed(1.0f, derivative);
    return segLengths[seg];
}


float 
SplineCurve::GetLength()
{
    assert(numCVs>=4 || closed==true);
    unsigned int numSeg = GetNumSegments();
    if(segLengths && segLengths[numSeg]>=0.0f)
        return segLengths[numSeg];

    unsigned int i;
    if(!segLengths)
    {
        segLengths = new float[numSeg+1]; // last entry holds total for the curve
        for(i=0; i<numSeg+1; i++)
            segLengths[i]=-1.0f;
    }

    float len=0.0f;
    for(i=0; i<numSeg; i++)
        len+=GetSegmentLength(i);
    segLengths[numSeg] = len;
    return len;
}


float 
SplineCurve::GetLength(const float t)
{
    float tt = t;
#ifdef RAD_RELEASE
    // in release, make sure you can't step out of interval
    if ( tt < 0.0f ) tt = 0.0f;
    if ( tt > GetEndParam() ) tt = GetEndParam();
#endif

    assert(numCVs>=4 || closed==true);
    assert(t>=0.0f && t<=GetEndParam());  // stay inside area where curve is defined

    float len = 0.0f;
    unsigned int min = int(tt);
    for(unsigned int i=0; i<min; i++)
        len += GetSegmentLength(i);
    
    LoadEvaluator(CURVE, tt);
    Spline derivative;
    derivative.SetCntrlMesh(Spline::Basis(int(basis)+3), CVs[min%numCVs], CVs[(min+1)%numCVs], CVs[(min+2)%numCVs], CVs[(min+3)%numCVs]);
    len += IntegrateSpeed(tt-min, derivative);
    return len;
}


float
SplineCurve::ArcConvert(float s)
{
#ifdef RAD_RELEASE
    // in release, make sure you can't step out of interval
    if ( s < 0.0f ) s = 0.0f;
    if ( s > GetLength() ) s = GetLength();
#endif

    assert(numCVs>=4 || closed==true);
    assert(s>=0.0f && s<=GetLength());

    float L = GetLength();
    float r = s/L;
    
    // find which segment to search for the root in
    float len=0.0f;
    unsigned int i;
    for(i=0; i<GetNumSegments(); i++)
    {
        len += segLengths[i];
        if(len>=s)
            break;
    }
    
    float interval = GetSegmentLength(i);
    float minT = float(i);
    float maxT = minT + 1.0f;
    float t = i + (s - len + interval)/ interval; // inital guess

    assert(basis>=0 && basis<3);
    SplineCurve derivative(*this);
    derivative.SetBasis(Spline::Basis(int(basis)+3));

    // use Newton iteration to converge on the solution
    const unsigned int MAX_ITER = 32;
    for(i=0; i<MAX_ITER; i++)
    {
        t = Clamp(t, minT, maxT);  // sanity check: do not step out of bounds
        float diff = GetLength(t) - s;
        if(Epsilon(diff, 0.0f))
            return t; // converged, we are done

        Vector velocity = derivative.Evaluate(t);
        float speed = velocity.Magnitude();
        if(speed!=0)
            t -= diff/speed;
        else
            t -= diff; // as good as anything else really
    }

    t = Clamp(t, minT, maxT);    // sanity check: do not step out of bounds
    return t; // MAX_ITER reached, we are done. Answer sucks, too bad...
}
