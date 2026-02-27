
#ifndef _WIND_HPP_
#define _WIND_HPP_

#include <radmath/radmath.hpp>
#include <simcommon/simenvironment.hpp>

#include <p3d/refcounted.hpp>

namespace sim
{

// SimWind class

class SimWind
    : public tRefCounted
{
public:
    SimWind() : mWindseed(0, 0, 0), mWind(0, 0, 0) {}
    
    virtual inline void UpdateWind(float /*time*/)      { mWind = mWindseed; }
    inline void SetWindSeedCGS(const rmt::Vector& v) // cm/sec
    {
        SimSpeed s;
        s.SetValueCGS(v.x);
        mWindseed.x = s.Value();
        s.SetValueCGS(v.y);
        mWindseed.y = s.Value();
        s.SetValueCGS(v.z);
        mWindseed.z = s.Value();
    }
    const rmt::Vector& GetWind() { return mWind; } // internal value
    
protected:
    virtual ~SimWind() {}
    
    rmt::Vector mWindseed;
    rmt::Vector mWind;
    // float time; not needed here but could be defined in derived class 
    //             if dt is required for updating the mWind
};

// mWind evolves as the product of the mWindseed multiplied by 
// two sinusoidal function of different frequencies
class SimWindSinus
    : public SimWind
{
public:
    SimWindSinus() : mFreq0(0.5f), mFreq1(0.15f) { mWindseed.Set(1.0f, 0, 0); }
    
    inline void UpdateWind(float time)
    {
        mWind.Scale(rmt::Fabs(rmt::Sin(mFreq0*time)*rmt::Sin(mFreq1*time)), mWindseed);
    }
    inline void SetFrequencies(float f0, float f1) { mFreq0 = f0; mFreq1 = f1; }
    
protected:
    virtual ~SimWindSinus() {}
    float mFreq0;
    float mFreq1;
};

} // sim

#endif // _WIND_HPP_