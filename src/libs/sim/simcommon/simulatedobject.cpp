
#include "simcommon/simulatedobject.hpp"

namespace sim
{

int SimulatedObject::sSimulatedObjectRefIndex = 0;

SimulatedObject::SimulatedObject(PhysicsProperties* inProperties)
    : mSimState(NULL),
    mProgressTime(0.0f),
    mPhysicsProperties(inProperties),
    mSimEnvironment(NULL),
    mUseRestingDetector(true),
    mType(MaxPhObjEnum)
{
    rAssert(mPhysicsProperties);
    mPhysicsProperties->AddRef();
    
    mRefIndex = sSimulatedObjectRefIndex ++;
}

SimulatedObject::SimulatedObject()  // for clone service
    : mSimState(NULL), 
    mPhysicsProperties(NULL), 
    mSimEnvironment(NULL),
    mUseRestingDetector(true),
    mType(MaxPhObjEnum) 
{
    mRefIndex = sSimulatedObjectRefIndex ++;
}


SimulatedObject::~SimulatedObject()
{
    if (mPhysicsProperties)
        mPhysicsProperties->Release();
    if (mSimEnvironment)
        mSimEnvironment->Release();
}

void SimulatedObject::SetPhysicsProperties(PhysicsProperties* inProperties)
{
    rAssert(inProperties);
    
    if (mPhysicsProperties != inProperties)
    {
        if (mPhysicsProperties != NULL)
            mPhysicsProperties->Release();
        
        mPhysicsProperties = inProperties;
        mPhysicsProperties->AddRef();
    }
    UpdateForPhysicsProperties();
}

void SimulatedObject::SetSimEnvironment(SimEnvironment* inSimEnvironment)
{
    rAssert(inSimEnvironment);
    
    if (mSimEnvironment != inSimEnvironment)
    {
        if (mSimEnvironment != NULL)
            mSimEnvironment->Release();
        
        mSimEnvironment = inSimEnvironment;
        mSimEnvironment->AddRef();
        
        UpdateForSimEnvironment();
    }
}

void  SimulatedObject::ResolveInterpenetration( int in_index, float &io_dist, const rmt::Vector &inN, rmt::Vector &o_imp, bool addconstrain, const SimulatedObject &otherObj)
{
}

void SimulatedObject::SetClone(SimulatedObject* clone, SimState* inSimState) //const
{
    clone->mSimState = inSimState;
    clone->mPhysicsProperties = mPhysicsProperties;
    clone->mPhysicsProperties->AddRef();
    clone->mSimEnvironment = mSimEnvironment;
    clone->mProgressTime = 0;
    if (mSimEnvironment)
        clone->mSimEnvironment->AddRef(); // keep the same SimEnvironment
    
    clone->SetNameObject(GetNameObject());
}


} // sim
