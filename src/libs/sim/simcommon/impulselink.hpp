
#ifndef _IMPULSELINK_HPP_
#define _IMPULSELINK_HPP_

#include <raddebug.hpp>
#include <p3d/refcounted.hpp>
#include <radmath/radmath.hpp>

namespace sim
{

class SimTarget;
class SimState;


class ImpulseLink
    : public tRefCounted
{
public:
    ImpulseLink(SimState* inObject, int inIndex, SimTarget* inTarget, float inIntensity);
    
    void SetSimStateOffset(const rmt::Vector& inOffset) { mSimStateOffset = inOffset; }
    
    virtual void ReplaceSimTarget(SimTarget* inTarget);
    SimTarget* GetSimTarget()              { return mSimTarget; }
    void SetIntensity(float inIntensity)   { mIntensity = inIntensity; }
    float GetIntensity()             const { return mIntensity; }
    
    virtual void Update(float inDt);
    
    void SetEnabled(bool inEnabled)        { mEnabled = inEnabled; }
    bool Enabled()                   const { return mEnabled; }
    void SetMinDist(float inMinDist)       { mMinDist = inMinDist; }
    
protected:
    ImpulseLink();
    virtual ~ImpulseLink();
    
    SimState* mSimState;
    int mIndex;
    rmt::Vector mSimStateOffset;
    SimTarget* mSimTarget;
    float mIntensity;
    float mMinDist;
    bool mEnabled;
};


class ImpulseLinkDual
    : public ImpulseLink
{
public:
    ImpulseLinkDual(SimState* inObject, int inIndex1, SimState* inObject2, int inIndex2, float inIntensity);
    
    void SetSimStateOffset(const rmt::Vector& inOffset);
    virtual void ReplaceSimTarget(SimTarget* inTarget) { rAssert(false); }
    
    virtual void Update(float inDt);
    
protected:
    virtual ~ImpulseLinkDual();
    
    SimState* mSimState2;
    int mIndex2;
    rmt::Vector mSimStateOffset2;
};



} // sim


#endif // _IMPLSE_LINK_HPP_