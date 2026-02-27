#include "simflexible/flexiblejointobject.hpp"

namespace sim
{

FlexibleJointObject::FlexibleJointObject(FlexJointModifier* inFlexJointModifier, PhysicsProperties* inProperties)
    : FlexibleObject(inProperties),
    mFlexJointModifier(inFlexJointModifier)
{
    rAssert(mFlexJointModifier);
    mFlexJointModifier->AddRef();
    
    SetUID(mFlexJointModifier->GetUID());
    
    mPsyst = mFlexJointModifier->GetParticleSystem();
    //mPsyst->mPhysicsProperties = mPhysicsProperties; to set in simstate
}

FlexibleJointObject::~FlexibleJointObject()
{
    mFlexJointModifier->Release();
    mPsyst = NULL;
}

void FlexibleJointObject::SetSimState(SimState *inSimState)
{
    SimulatedObject::SetSimState(inSimState);
}

} // sim
