
#include "simcommon/simconstraint.hpp"
#include "simcommon/simstate.hpp"
#include "simcommon/simulatedobject.hpp"

namespace sim
{

SimConstraint::SimConstraint( SimState *in_constrainingObject, LocalTarget &in_constrainingLacalTarget,
    SimState *in_constrainedObject,  LocalTarget &in_constrainedLacalTarget  ) :
mState1(in_constrainingObject),
    mLocTarget1(in_constrainingLacalTarget),
    mState2(in_constrainedObject),
    mLocTarget2(in_constrainedLacalTarget)
{
    if (mState1)
        mState1->AddRef();
    if (mState2)
        mState2->AddRef();
    
    mTarget1 = new SimTarget;
    mTarget1->AddRef();
}

SimConstraint::~SimConstraint()
{
    if (mState1)
        mState1->Release();
    if (mState2)
        mState2->Release();
    
    mTarget1->Release();
}

void SimConstraint::Update(float in_dt)
{
    rmt::Vector l_VelocyTarget;
    //First we update the target position defined by the constraining object.
    mState1->GetSimulatedObject()->UpdateConstraintTarget(mLocTarget1, &l_VelocyTarget, *mTarget1);
    
    //Ask to the constrained object to satisfied the constraint.
    mState2->GetSimulatedObject()->UpdateConstraint(mLocTarget2, *mTarget1, &l_VelocyTarget, in_dt);
    
}

} // sim