
#ifndef _KALMANN_HPP_
#define _KALMANN_HPP_

#include <radmath/radmath.hpp>

using namespace RadicalMathLibrary;

namespace sim
{

class Filter1D
{
public:
    Filter1D(){};
    virtual bool Init( float firstGuess, float measUncert, float valueUncert)=0;
    virtual float Estimate(float newMeasurement, float dt = 0)=0;
    virtual float GetEstimate( )const=0;
};

class  SimpleFilter1D : public Filter1D
{
public:
    SimpleFilter1D();
    virtual bool Init( float firstGuess, float measUncert, float valueUncert);
    virtual float Estimate(float newMeasurement, float dt = 0);
    virtual float GetEstimate() const
    {
        return mPrevMeasurement;
    }
protected:
    float mDelta, mDeltaScl;
    float mPrevMeasurement;
    float mDt;
};


class  Filter3D
{
public:
    Filter3D(){};
    virtual bool Init( const Vector *firstGuess, const Vector *measUncert, const Vector *valueUncert )=0;
    virtual const Vector & Estimate( const Vector *newMeasurement, float dt = 0 )=0;
    virtual const Vector & GetEstimate() const=0;
};

class  SimpleFilter3D
    :public Filter3D
{
public:
    SimpleFilter3D();
    virtual bool Init( const Vector *firstGuess, const Vector *measUncert, const Vector *valueUncert );
    virtual const Vector & Estimate( const Vector *newMeasurement, float dt = 0 );
    virtual const Vector & GetEstimate() const { return mPrevMeasurement; }

    
protected:
    Vector mDelta,mDeltaScl;
    Vector mPrevMeasurement;
    float  mDt;
};

//Same filter as the preceeding but all values are threat in absolute values.
class SimpleFilterAbs3D 
    :public SimpleFilter3D
{
public:
    virtual bool Init( const Vector *firstGuess, const Vector *measUncert, const Vector *valueUncert );
    virtual const Vector& Estimate( const Vector *newMeasurement, float dt = 0 );
};

//

class  Kalmann1D
    :public Filter1D
{
public:
    Kalmann1D();
    virtual bool Init(float firstGuess, float measUncert, float valueUncert);
    virtual float Estimate(float newMeasurement, float dt = 0);
    virtual float GetEstimate() const { return 0; }
    
protected:
    float fSpeed;
    
private:
    inline void UpdateP() { fP = fP * (1.0f - fK) + fQ; }
    
    float fK;
    float fP;
    float fR;
    float fQ;
    float fEstimate;
};

//

class  DoubleKalmann1D
    : public Kalmann1D
{
public:
    DoubleKalmann1D();
    virtual bool Init(float firstGuess, float measUncert, float valueUncert);
    virtual float Estimate(float newMeasurement, float dt = 0);
    bool Init(float firstGuess, float measUncert, float valueUncert, float frstGuess2, float measUncert2, float valueUncert2);
    virtual float GetEstimate() const { return 0; }
    
private:
    Kalmann1D fSpeedFilter;
    float mPrevMeasurement;
};

} // sim

#endif // _Kalmann_HPP_
