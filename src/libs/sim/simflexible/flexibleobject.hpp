#ifndef _FLEXIBLEOBJECT_HPP_
#define _FLEXIBLEOBJECT_HPP_

#include <radmath/radmath.hpp>
#include "simflexible/condition.hpp"
#include "simcommon/tarray.hpp"
#include "simcommon/simulatedobject.hpp"
#include <p3d/p3dtypes.hpp>

namespace sim
{
    
class ParticleSystem;
class FlexParticle;
class SimTarget;
class PhysicsProperties;

class FlexibleObject
    : public SimulatedObject
{
public:
    FlexibleObject(PhysicsProperties* inProperties, ParticleSystem* ps);
    
    virtual void UpdateForPhysicsProperties();
    virtual void UpdateForSimEnvironment();
    virtual SimulatedObject* Clone( SimState *inSimState);
    virtual bool IsAtRest() const;
    virtual bool TestResting(float dt);
    virtual void WakeUp();
    virtual void SetSimState( SimState *inSimState);
    
    // Methods in the process of solving collisions
    // StartCollision clear the cache buffer
    // AddImpulse store in the cache buffer the effect caused by the collision, returns false if the impulse can be added in the buffer.
    // End Collision adds the cache buffer into the object's state.
    // ResolveInterpenetration is called when the collision solver find an
    // interpenetration between a flexible object and a rigid body.
    virtual void  StartCollision(const rmt::Vector& inPos, const rmt::Vector& inDir, const SimulatedObject* inObj = NULL, int index = -1, int otherIndex = -1);
    virtual bool  AddImpulse(const rmt::Vector& inPos, const rmt::Vector& inImp, const SymMatrix& inSymMatrix, SimulatedObject* otherObj = NULL, int index = -1, int otherIndex = -1);
    virtual void  EndCollision(bool fdyn = true, int index = -1, int otherIndex = -1){}
    virtual void  GetMassMatrix(const rmt::Vector& inPos, SymMatrix& oM, SimulatedObject* o = NULL, int index = -1, int otherIndex = -1);
    virtual float InvMass() const { return 1.0f; }
    virtual void  ResolveInterpenetration( int in_index, float &io_dist, const rmt::Vector &inN, rmt::Vector &o_imp, bool addconstrain, const SimulatedObject &otherObj);
    virtual void  AddSolverConstraint( const rmt::Vector& inDir, int in_index );
    
    virtual void SyncSimObj(bool inReset=true) {}
    
    void SetScale(float f);
    void SynchronizeParticleSystem();
    void  AddFixParticleTarget(int i, SimTarget* t);
    void FreeParticle(int p);
    bool NeighborFixParticle(int p);
    void UpdateAttachedParticles(float dt);
    
    virtual const rmt::Vector& ParticlePosition(int p) const;
    virtual const rmt::Vector& ParticleVelocity(int p) const;
    
    void ResetPos();
    
    virtual void SetInternalParams(const ConditionParams& p);
    
    virtual void DebugDisplay();
    
    virtual bool UpdateConstraintTarget( const LocalTarget &in_target, rmt::Vector *o_VelocityTarget, SimTarget &io_target) const;
    
    class ParticleTarget
    {
    public:
        ParticleTarget() : mSimTarget(NULL) {}
        ~ParticleTarget();
        
        ParticleTarget &operator= ( const ParticleTarget &inParticleTarget )
        {
            Set(inParticleTarget.mParticleIndex, inParticleTarget.mSimTarget);
            return *this;
        }
        
        void Set(int inParticleIndex, SimTarget* inSimTarget);
        
        SimTarget* mSimTarget;
        int mParticleIndex;
    };
    
    bool IsActivated() const { return mIsActivated; }
    void SetActicated(bool b) { mIsActivated = b; }
    virtual int GetNumSubObj() const;
    
    ParticleSystem         *mPsyst;
    TArray<ParticleTarget>  mParticleTargets;
    
    virtual void Update(float inDt);
    
protected:
    friend class SimStateFlexible;
    
    virtual ~FlexibleObject();
    
    
    FlexibleObject(PhysicsProperties* inProperties);
    bool AddCollisionConstraint(const rmt::Vector& inDir, int inIndex);
    
    float mSystemScale;
    float mImpScale;
    bool  mIsActivated;
    
private:
    FlexibleObject& operator=(FlexibleObject& inObj) { rAssert(false); return *this; }
    FlexibleObject(const FlexibleObject& inObject) { rAssert(false); }

};
    
} // sim

#endif // _FlexibleObject_HPP_
