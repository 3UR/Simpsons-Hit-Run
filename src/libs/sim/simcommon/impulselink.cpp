

#include "simcommon/impulselink.hpp"
#include "simcommon/simulatedobject.hpp"
#include "simcommon/simmath.hpp"
#include "simcollision/collisionobject.hpp"
#include "simcollision/collisionvolume.hpp"

namespace sim
{

ImpulseLink::ImpulseLink(SimState* inSimState, int inIndex, SimTarget* inTarget, float inIntensity)
    : mSimState(inSimState),
    mIndex(inIndex),
    mSimStateOffset(0.0f, 0.0f, 0.0f),
    mSimTarget(NULL),
    mIntensity(inIntensity),
    mMinDist(VERY_SMALL),
    mEnabled(true)
{
    rAssert(mSimState);
    mSimState->AddRef();
    
    if (mSimState->GetCollisionObject())
    {
        CollisionVolume* cv = mSimState->GetCollisionObject()->GetCollisionVolume()->GetSubCollisionVolume(mIndex, true);
        if (cv)
        {
            cv->UpdateAll();
            mMinDist = 2.0f * cv->mSphereRadius;
        }
    }
    
    ReplaceSimTarget(inTarget);
}

ImpulseLink::ImpulseLink()
    : mSimState(NULL),
    mIndex(-1),
    mSimStateOffset(0.0f, 0.0f, 0.0f),
    mSimTarget(NULL),
    mIntensity(0),
    mMinDist(VERY_SMALL),
    mEnabled(true)
{
}

ImpulseLink::~ImpulseLink()
{
    if (mSimTarget)
        mSimTarget->Release();
    
    mSimState->Release();
}

void ImpulseLink::ReplaceSimTarget(SimTarget* inTarget)
{
    if (mSimTarget)
        mSimTarget->Release();
    
    mSimTarget = inTarget;
    
    if (mSimTarget)
        mSimTarget->AddRef();
}

void ImpulseLink::Update(float inDt)
{
    if (!mEnabled || inDt == 0)
        return;
    
    if (mSimState->GetControl() != simSimulationCtrl && mIndex < 0)
        return;
    
    SimulatedObject* simObject = mSimState->GetSimulatedObject(mIndex);
    rAssert(simObject);
    
    rmt::Vector normal, objPos;
    mSimTarget->GetPos(normal);
    mSimState->GetTransform(mIndex).Transform(mSimStateOffset, &objPos);
    
    normal.Sub(objPos);
    float dist = normal.NormalizeSafe();
    
    if (dist > mMinDist)
    {
        dist -= mMinDist;
        static float ApproachModeScale = 10.0f;
        float ratio = rmt::Min(1.0f, dist / mMinDist / ApproachModeScale);
        
        simObject->StartCollision(objPos, normal, NULL, mIndex);
        
        SymMatrix invMass;
        simObject->GetMassMatrix(objPos, invMass, NULL, mIndex);
        
        rmt::Vector impulse;
        
        impulse.Scale(mIntensity*ratio, normal); // full intensity
        
        if (ratio < 1.0f) // just try to immobilize
        {
            rmt::Vector tnormal, vObj, vTarget;
            mSimState->GetVelocity(objPos, vObj, mIndex);
            mSimTarget->GetSpeed(vTarget);
            vObj.Sub(vTarget);
            float dvn = vObj.DotProduct(vTarget);
            invMass.TransformVector( normal, tnormal );
            float nMn = normal.DotProduct(tnormal);
            impulse.ScaleAdd(-dvn/nMn*(1.0f-ratio), normal);
        }
        
        while (!simObject->AddImpulse(objPos, impulse, invMass, 0, mIndex))
        {
            simObject->GetMassMatrix(objPos, invMass, 0, mIndex);
        }
        simObject->EndCollision(false, mIndex);
    }
}

//
//
//

ImpulseLinkDual::ImpulseLinkDual(SimState* inSimState, int inIndex, SimState* inSimState2, int inIndex2, float inIntensity)
{
    mSimState = inSimState;
    mSimState2 = inSimState2;
    mIndex = inIndex;
    mIndex2 = inIndex2;
    mSimStateOffset.Clear();
    mSimStateOffset2.Clear();
    mIntensity = inIntensity;
    
    rAssert(mSimState && mSimState2);
    
    if (mSimState->GetCollisionObject())
    {
        CollisionVolume* cv = mSimState->GetCollisionObject()->GetCollisionVolume()->GetSubCollisionVolume(mIndex, true);
        if (cv)
        {
            cv->UpdateAll();
            mMinDist += cv->mSphereRadius;
        }
    }
    if (mSimState2->GetCollisionObject())
    {
        CollisionVolume* cv = mSimState2->GetCollisionObject()->GetCollisionVolume()->GetSubCollisionVolume(mIndex2, true);
        if (cv)
        {
            cv->UpdateAll();
            mMinDist += cv->mSphereRadius;
        }
    }
}

ImpulseLinkDual::~ImpulseLinkDual()
{
    mSimState2->Release();
}

void ImpulseLinkDual::Update(float inDt)
{
    if (!mEnabled || inDt == 0)
        return;
    
    if (mSimState->GetControl() != simSimulationCtrl && mIndex < 0)
    {
        if (mSimState2->GetControl() != simSimulationCtrl && mIndex2 < 0)
            return;
    }
    
    SimulatedObject* simObject = mSimState->GetSimulatedObject(mIndex);
    rAssert(simObject);
    SimulatedObject* simObject2 = mSimState2->GetSimulatedObject(mIndex2);
    rAssert(simObject2);
    
    rmt::Vector objPos, objPos2, n, n2;
    mSimState->GetTransform(mIndex).Transform(mSimStateOffset, &objPos);
    mSimState2->GetTransform(mIndex2).Transform(mSimStateOffset2, &objPos2);
    
    n.Sub(objPos2, objPos);
    float dist = n.NormalizeSafe();
    n.Scale(mIntensity);
    n2.Scale(-1.0f, n);
    
    if (dist > VERY_SMALL)
    {
        simObject->StartCollision(objPos, n, NULL, mIndex);
        simObject2->StartCollision(objPos2, n, NULL, mIndex2);
        
        SymMatrix m, m2, mt;
        simObject->GetMassMatrix(objPos, m, simObject2, mIndex, mIndex2);
        simObject2->GetMassMatrix(objPos2, m2, simObject, mIndex2, mIndex);
        
        bool ok = false, ok2 = false;
        
        do
        {
            mt.Sum(m, m2);
            
            ok = simObject->AddImpulse(objPos, n, mt, simObject2, mIndex, mIndex2);
            if (!ok)
            {
                simObject->GetMassMatrix(objPos, m, simObject2, mIndex, mIndex2);
                continue;
            }
            ok2 = simObject2->AddImpulse(objPos2, n2, mt, 0, mIndex);
            if (!ok)
            {
                simObject->GetMassMatrix(objPos, m, simObject2, mIndex, mIndex2);
                continue;
            }
        } while (!ok || !ok2);
        
        simObject->EndCollision(false, mIndex);
        simObject2->EndCollision(false, mIndex2);
    }
}

} // sim