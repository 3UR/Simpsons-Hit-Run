
#ifndef _FLEXIBLEJOINOBJECT_HPP_
#define _FLEXIBLEJOINOBJECT_HPP_

#include "simflexible/flexibleobject.hpp"
#include "simflexible/flexjointmodifier.hpp"

namespace sim
{
    
    class FlexibleJointObject
        : public FlexibleObject
    {
    public:
        FlexibleJointObject(FlexJointModifier* inFlexJointModifier, PhysicsProperties* inProperties);
        
        //virtual void UpdateForPhysicsProperties();
        //virtual void UpdateForSimEnvironment();
        SimulatedObject* Clone( SimState *inSimState) { rAssert(false); return NULL; }
        //virtual bool IsAtRest() const;
        //virtual bool TestResting(float dt);
        //virtual void WakeUp();
        //virtual void SetSimState( SimState *inSimState);
        
        bool IsValid()     const { return true;}//mFlexJointModifier->mJointState == NULL ? false : true; }
        bool IsActivated() const { return (IsValid() && mFlexJointModifier->IsEnabled()) ? true : false; }
        
        // those will return internal value only if the FlexJointModifier is activated and contains a jointState
        bool Contains(int inIndex)             const { return mFlexJointModifier->ContainsJoint(inIndex); }
        short PoseToParticleIndex(int inIndex) const { return mFlexJointModifier->PoseToParticleIndex(inIndex); }
        
        // Methods in the process of solving collisions
        // StartCollision clear the cache buffer
        // AddImpulse store in the cache buffer the effect caused by the collision, returns false if the impulse can be added in the buffer.
        // End Collision adds the cache buffer into the object's state.
        // ResolveInterpenetration is called when the collision solver find an
        // interpenetration between a flexible object and a rigid body.
        virtual void  StartCollision(const rmt::Vector& inPos, const rmt::Vector& inDir, const SimulatedObject* inObj = NULL, int inIndex = -1, int otherIndex = -1)
        {
            rAssert(IsActivated());
            mCurrentIndex = PoseToParticleIndex(inIndex);
            FlexibleObject::StartCollision(inPos, inDir, inObj, mCurrentIndex);
        }
        
        virtual bool  AddImpulse(const rmt::Vector& inPos, const rmt::Vector& inImp, const SymMatrix& inSymMatrix, SimulatedObject* otherObj = NULL, int inIndex = -1, int otherIndex = -1)
        {
            rAssert(mCurrentIndex == PoseToParticleIndex(inIndex) && IsActivated());
            return FlexibleObject::AddImpulse(inPos, inImp, inSymMatrix, otherObj, mCurrentIndex);
        }
        
        virtual void  GetMassMatrix(const rmt::Vector& inPos, SymMatrix& oM, SimulatedObject* o = NULL, int inIndex = -1, int otherIndex = -1)
        {
            rAssert(mCurrentIndex == PoseToParticleIndex(inIndex) && IsActivated());
            FlexibleObject::GetMassMatrix(inPos, oM, o, mCurrentIndex);
        }
        
        virtual void  ResolveInterpenetration( int inIndex, float &io_dist, const rmt::Vector &inN, rmt::Vector &o_imp, bool addconstrain, const SimulatedObject &otherObj)
        {
            rAssert(mCurrentIndex == PoseToParticleIndex(inIndex) && IsActivated());
            FlexibleObject::ResolveInterpenetration(mCurrentIndex, io_dist, inN, o_imp, addconstrain, otherObj);
        }
        virtual void  EndCollision(bool fdyn = true, int index = -1, int otherIndex = -1)
        {
            mCurrentIndex = 0;
        }
        virtual void  AddSolverConstraint( const rmt::Vector& inDir, int inIndex )
        {
            rAssert(IsActivated());
            mCurrentIndex = PoseToParticleIndex(inIndex);
            FlexibleObject::AddSolverConstraint(inDir, mCurrentIndex);
        }
        
        
        //virtual void PushSimState(bool inReset=true) {}
        
        //void SetScale(float f);
        //void SynchronizeParticleSystem();
        //void  AddFixParticleTarget(int i, SimTarget* t);
        //void FreeParticle(int p);
        //bool NeighborFixParticle(int p);
        //void UpdateAttachedParticles(float dt);
        
        void SetSimState( SimState *inSimState);
        //void ResetPos();
        
        //virtual void DebugDisplay();
        
        //virtual bool UpdateConstraintTarget( const LocalTarget &in_target, rmt::Vector *o_VelocityTarget, SimTarget &io_target) const { return false; }
        
        FlexJointModifier*   mFlexJointModifier;
        short                mCurrentIndex;

    protected:
        virtual ~FlexibleJointObject();
        
        
    };
    
} // sim

#endif // _FLEXIBLEJOINOBJECT_HPP_
