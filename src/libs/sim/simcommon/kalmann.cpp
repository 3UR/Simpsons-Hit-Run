
#include "simcommon/Kalmann.hpp"
#include "simcommon/simmath.hpp"

using namespace RadicalMathLibrary;

namespace sim
{

inline void MultComp( Vector &v1, const Vector &v2 )
{
    v1.x *= v2.x;
    v1.y *= v2.y;
    v1.z *= v2.z;
}

inline void Fabsv( Vector &in_v )
{
    in_v.x = Fabs(in_v.x);
    in_v.y = Fabs(in_v.y);
    in_v.z = Fabs(in_v.z);
}

const float almost_one = 1.0f - MICRO_EPS;

inline void LocalClamp(float& val, float minVal, float maxVal)
{
    if (val <= minVal)
        val = minVal;
    else if (val > maxVal)
        val = maxVal;
}

//
//
//

SimpleFilter1D::SimpleFilter1D()
 : mDelta(0.0f),
   mDeltaScl(0.0f),
   mPrevMeasurement(0.0f),
   mDt(0.0f)
{
}

// Pass in the threshold value that we want to detect for measUncert.
bool SimpleFilter1D::Init( float firstGuess, float measUncert, float /*valueUncert*/ )
{
    rAssert( measUncert > 0.0f );

    mPrevMeasurement = firstGuess;
    if( measUncert <= 0.0f )
        return false;//Not initialised
    
    mDelta = 1.0f/measUncert;
    mDt    = 0.0f;
    return true;
}

float SimpleFilter1D::Estimate(float newMeasurement, float in_dt /* = 0*/ )
{
    rAssert(in_dt != 0.0f);

    if( in_dt != mDt )
    {
        mDeltaScl = in_dt * mDelta;

        LocalClamp(mDeltaScl, 0, almost_one);

        if (mDt == 0.0f)
        { 
            // When mDt is zero, the filter have just been reset. Here, we
            // guaranty that the first estimate will be 2 time greater than
            // the first guess given at the Init method.
            mPrevMeasurement = ( 2.0f*mPrevMeasurement - newMeasurement*mDeltaScl ) / ( 1.0f - mDeltaScl );
        }
        
        mDt = in_dt;
    }
    mPrevMeasurement += mDeltaScl * ( newMeasurement - mPrevMeasurement );
    return mPrevMeasurement;
}

//
//
//

SimpleFilter3D::SimpleFilter3D()
 : mDelta(0.0f,0.0f,0.0f),
   mDeltaScl(0.0f,0.0f,0.0f),
   mPrevMeasurement(0.0f,0.0f,0.0f),
   mDt(0.0f)
{
}

// Pass in the threshold value that we want to detect for measUncert.
bool SimpleFilter3D::Init( const Vector *firstGuess, const Vector *measUncert, const Vector * /*ValueUncert*/ )
{
    rAssert( measUncert->x > 0.0f && measUncert->y > 0.0f && measUncert->z > 0.0f );
    if( !(measUncert->x > 0.0f && measUncert->y > 0.0f && measUncert->z > 0.0f) )
        return false;
    
    mPrevMeasurement = *firstGuess;
    
    mDelta.x = 1.0f/measUncert->x;
    mDelta.y = 1.0f/measUncert->y;
    mDelta.z = 1.0f/measUncert->z;
    mDt      = 0.0f;
    return true;
    
}

const Vector & SimpleFilter3D::Estimate( const Vector *newMeasurement, float in_dt)
{
    rAssert(newMeasurement);
    rAssert(in_dt != 0.0f);

    
    if( in_dt != mDt )
    {
        mDeltaScl.Scale(in_dt, mDelta);
        
        LocalClamp(mDeltaScl.x, 0, almost_one);
        LocalClamp(mDeltaScl.y, 0, almost_one);
        LocalClamp(mDeltaScl.z, 0, almost_one);

        if (mDt == 0.0f)
        {
            // See comment in SimpleFilter1D::Estimate
            Vector l_v = *newMeasurement;
            MultComp(l_v, mDeltaScl);
            mPrevMeasurement.Scale(2.0f);
            mPrevMeasurement.Sub(l_v);
            mPrevMeasurement.x /= ( 1.0f - mDeltaScl.x );
            mPrevMeasurement.y /= ( 1.0f - mDeltaScl.y );
            mPrevMeasurement.z /= ( 1.0f - mDeltaScl.z );
        }
        mDt = in_dt;
    }
    
    mPrevMeasurement.x += mDeltaScl.x * (newMeasurement->x - mPrevMeasurement.x);
    mPrevMeasurement.y += mDeltaScl.y * (newMeasurement->y - mPrevMeasurement.y);
    mPrevMeasurement.z += mDeltaScl.z * (newMeasurement->z - mPrevMeasurement.z);
    return mPrevMeasurement;
}

bool SimpleFilterAbs3D::Init( const Vector *firstGuess, const Vector *measUncert, const Vector * ValueUncert )
{
    if( SimpleFilter3D::Init( firstGuess, measUncert, ValueUncert ) )
    {
        Fabsv(mDelta);
        return true;
    }
    return false;
}

const Vector & SimpleFilterAbs3D::Estimate( const Vector *newMeasurement, float in_dt)
{
    rAssert(newMeasurement);
    
    Vector l_v;
    l_v = *newMeasurement;
    Fabsv(l_v);
    return SimpleFilter3D::Estimate(&l_v,in_dt);
}

//
//
//

Kalmann1D::Kalmann1D()
{
    Init(0, 1.0f, 1.0f);
}

bool Kalmann1D::Init(float firstGuess, float measUncert, float valueUncert)
{
    fEstimate = firstGuess;
    fR = measUncert*measUncert;
    fQ = valueUncert*valueUncert;
    fP = (fQ > 0 ? fQ : 1.0f);
    fSpeed = 0;
    return true;
}

float Kalmann1D::Estimate(float newMeasurement, float dt)
{
    /*
    don't really need dt for now. The estimate error does not grow 
    with time. If dt is small, Estimate is called more often and
    we converge sooner.
    */
    
    fK = fP / (fP + fR); 
    UpdateP();
    fEstimate = fEstimate + fSpeed * dt + fK * (newMeasurement - fEstimate);
    
    return fEstimate;
}

//
//
//

DoubleKalmann1D::DoubleKalmann1D()
{
}

bool DoubleKalmann1D::Init(float firstGuess, float measUncert, float valueUncert, float frstGuess2, float measUncert2, float valueUncert2)
{
    Kalmann1D::Init(firstGuess, measUncert, valueUncert);
    fSpeedFilter.Init(frstGuess2, measUncert2, valueUncert2);
    mPrevMeasurement = firstGuess;
    return true;
}

bool DoubleKalmann1D::Init(float firstGuess, float measUncert, float valueUncert)
{
    Kalmann1D::Init(firstGuess, measUncert, valueUncert);
    fSpeedFilter.Init(firstGuess, measUncert, valueUncert);
    mPrevMeasurement = firstGuess;
    return true;
}

float DoubleKalmann1D::Estimate(float newMeasurement, float dt)
{
    rAssert(dt > 0);
    fSpeed = (newMeasurement - mPrevMeasurement) / dt;    // measure speed
    fSpeed = fSpeedFilter.Estimate(fSpeed);            // estimate speed
    mPrevMeasurement = newMeasurement;
    return Kalmann1D::Estimate(newMeasurement, dt);
}

} // sim
