#ifndef _SIMSTATETARGET_HPP_
#define _SIMSTATETARGET_HPP_


#include <simcommon/simtarget.hpp>
#include <simcommon/simstate.hpp>

namespace sim
{


class SimStateTarget
    :public SimTarget
{
public:
    
    SimStateTarget(SimState* inSimState);
    
    virtual void ReplaceTarget(SimState* inSimState);
    virtual void GetPos(rmt::Vector& v, float dt=0) const { mSimState->GetTransform().Transform(offset, &v); }
    virtual void GetTransform(rmt::Matrix& m, float dt=0) const { m = mSimState->GetTransform(); GetPos(*(rmt::Vector*)m.m[3]); }
    virtual void GetSpeed(rmt::Vector& v) const; 
    virtual void GetAngularVelocity(rmt::Vector& v) const;
    
protected:
    SimStateTarget() {} // locked
    virtual ~SimStateTarget() {}
    SimState* mSimState;
};



// This class creates intermediate targets given the initial position and the target 
// position.  It handles situations where the endEffector of an IK chain cannot move 
// fast enough to reach the target or would collide with an object while moving to the goal
/*
class SimStateProgressiveTarget
:public SimTarget
{
SimStateProgressiveTarget();
~SimStateProgressiveTarget();

  
    
      };
*/
class SimVectorProgressiveTarget
    :public SimVectorTarget
{
public:
    SimVectorProgressiveTarget(const rmt::Vector* inCurrent, 
        const rmt::Vector* inTarget, 
        float inMAXSPEED);
    
    ~SimVectorProgressiveTarget() {}
    void GetPos(rmt::Vector& v, float dt=0) const;
    
private:
    const rmt::Vector* mCurrentPosition;
    const float mMAXSPEED;
};


class SimMatrixProgressiveTarget
    :public SimMatrixTarget
{
public:
    SimMatrixProgressiveTarget(const rmt::Matrix* inCurrent,
        const rmt::Matrix* inTarget,
        float inMAXSPEED,
        float inMAXANGSPEED = 1.0f);
    
    SimMatrixProgressiveTarget(const rmt::Matrix* inCurrent,
        const SimMatrixTarget* inTarget,
        float inMAXSPEED,
        float inMAXANGSPEED = 1.0f);
    
    void GetPos(rmt::Vector& v, float dt=0) const;
    void GetTransform(rmt::Matrix& v, float dt=0) const;
    
protected:
    ~SimMatrixProgressiveTarget() {}
    const rmt::Matrix* mCurrentOrientation;
    const float mMAXSPEED;
    const float mMAXANGSPEED;
};

} // sim

#endif