
#include <simcommon/simstate.hpp>
#include "simcommon/dline2.hpp"
#include "p3d/utility.hpp"
#include "simphysics/physicsobject.hpp"
#include "simcollision/collisionobject.hpp"

using namespace RadicalMathLibrary;

namespace sim
{

float SimState::sApproxSpeedMagnitudeFactor = 2.0f;
rmt::Matrix ManualSimState::sIdentity(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);

SimState* SimState::CreateSimState(tUID inUid, SimStateAttributes inAttrib, tEntityStore* inStore)
{
    
    CollisionObject * collObj = NULL;
    SimulatedObject * simObj  = NULL;
    if (inAttrib & SimStateAttribute_Collision || inAttrib == SimStateAttribute_Default)
    {
        if (inStore)
            collObj = p3d::find<CollisionObject>(inStore, inUid);
        else
            collObj = p3d::find<CollisionObject>(inUid);
        
        if (!collObj && inAttrib != SimStateAttribute_Default)
        {
            return NULL;
        }
        if (collObj && collObj->GetNumJoint() > 0)
        {
            return NULL;
        }
    }
    if (inAttrib & SimStateAttribute_Simulated || inAttrib == SimStateAttribute_Default)
    {
        if (inStore)
            simObj = p3d::find<PhysicsObject>(inStore, inUid);
        else
            simObj = p3d::find<PhysicsObject>(inUid);
        
        if (!simObj && inAttrib != SimStateAttribute_Default)
        {
            return NULL;
            //         simObj = p3d::find<FlexibleObject>(inUid);
        }
    }

    return CreateSimState(collObj, simObj);
}

SimState* SimState::CreateSimState(SimState* inSimState, const char* inName)
{
    rAssert(inSimState->mArticulated == false);

    SimState* simState = SimState::CreateSimState(inSimState->GetCollisionObject(), inSimState->GetSimulatedObject());

    if (inName)
    {
        if (simState->GetCollisionObject())
        {
            simState->GetCollisionObject()->SetName(inName);
        }
        if (simState->GetSimulatedObject())
        {
            simState->GetSimulatedObject()->SetName(inName);
        }
    }
    return simState;
}

SimState* SimState::CreateSimState(const char* inName, SimStateAttributes inAttrib, tEntityStore* inStore)
{
    return CreateSimState(tEntity::MakeUID(inName), inAttrib, inStore);
}

SimState* SimState::CreateStaticSimState(CollisionObject* inCollisionObject)
{
    SimState* simState = CreateSimState(inCollisionObject, NULL);
    simState->mObjectMoving = false;
    return simState;
}

SimState* SimState::CreateSimState(CollisionVolume* inCollisionVolume, const char* inName, tEntityStore* inStore)
{
    rAssert(inCollisionVolume->GetCollisionObject() == NULL);

    inCollisionVolume->DeOptimize();
    CollisionObject* newCollisionObject = new CollisionObject(inCollisionVolume);
    newCollisionObject->SetPossibleCollisionEvents(0);
    newCollisionObject->mIsStatic = false;

    if (inName)
        newCollisionObject->SetName(inName);
    else
        newCollisionObject->SetName("Not baptized yet");

    newCollisionObject->SetPhysicsProperties(PhysicsProperties::DefaultPhysicsProperties(inStore));

    SimState* simState = CreateSimState(newCollisionObject, NULL);
    return simState;
}

SimState* SimState::CreateSimState(CollisionObject* collObj, SimulatedObject* simObj)
{
    SimState* simState = new SimState;
    
    if (simState == NULL )
    {
        return NULL;
    }
    else
    {
        if (simObj)
        {
            if (simObj->GetSimState())
            {
                simObj = simObj->Clone(simState);
            }
            else
            {
                simObj->SetSimState(simState);
            }
            simState->SetSimulatedObject(simObj);
        }
        if (collObj)
        {
            bool cloned = false;
            if (collObj->GetSimState())
            {
                collObj = collObj->Clone(simState);
                cloned = true;
            }
            else
            {
                collObj->SetSimState(simState);
            }
            simState->SetCollisionObject(collObj);

            if ( !cloned )
            {
                collObj->Update();
                collObj->mCollisionVolume->UpdateBBox();
            }
            
            // temporarilly to make sure that simobject and collobject have same physics properties
            // this will be fixed when the real loaders will be created
            if (simObj && collObj)
            {
                collObj->SetPhysicsProperties(simObj->GetPhysicsProperties());
            }
        }
    }
    
    return simState;
}

SimState* SimState::CreateManualSimState(CollisionVolume* inCollisionVolume, const char* inName)
{
    rAssert(inCollisionVolume->GetCollisionObject() == NULL);

    inCollisionVolume->DeOptimize();
    CollisionObject* collObj = new CollisionObject(inCollisionVolume);
    collObj->SetPossibleCollisionEvents(0);
    collObj->mIsStatic = true;

    if (inName)
        collObj->SetName(inName);
    else
        collObj->SetName("Unnamed ManualSimState");

    collObj->SetPhysicsProperties(PhysicsProperties::DefaultPhysicsProperties());

    ManualSimState* simState = new ManualSimState;
    
    collObj->SetSimState(simState);
    simState->SetCollisionObject(collObj);
    collObj->SetManualUpdate(true);
    collObj->PostManualUpdate();

    return simState;
}

//
//
//

void ExtractVelocityFromMatrix(const rmt::Matrix& oldMatrix, const rmt::Matrix& newMatrix, float scale, float dt, SimVelocityState& velocity)
{
    velocity.mLinear.Sub(*((Vector*)newMatrix.m[3]), *((Vector*)oldMatrix.m[3]));
    velocity.mLinear.Scale(Inverse(dt));
    WFromRotMatrix(velocity.mAngular, oldMatrix, newMatrix, dt);
    if (scale != 1.0f)
    {
        velocity.mAngular.Scale(Inverse(Sqr(scale)));
    }
}

//
//
//

SimState::SimState(SimControlEnum inControl)
    : mAIRefPointer(NULL),
    mAIRefIndex(0),
    mScale(1.0f),
    mControl(inControl),
    mSimulatedObject(NULL),
    mCollisionObject(NULL),
    mVirtualCM(NULL),
    mObjectMoving(false),
    mSafeTimeBeforeCollision(0),
    mApproxSpeedMagnitude(0),
    mArticulated(false)
{
    mTransform.Identity();
}

SimState::~SimState()
{
    if (mCollisionObject)
    {
        mCollisionObject->SetSimState(NULL);
        mCollisionObject->Release();
    }
    if (mSimulatedObject)
    {
        mSimulatedObject->SetSimState(NULL);
        SimEnvironment::RemoveObject(mSimulatedObject);
        mSimulatedObject->Release();
    }
    if (mVirtualCM)
    {
        delete mVirtualCM;
    }
}

void SimState::SetControl(SimControlEnum inControl)
{
    if (mControl != inControl)
    {
        mControl = inControl;
        
        if (mControl == simSimulationCtrl)
        {
            rAssert(mSimulatedObject && mSimulatedObject->InvMass() != 0);
            if (mSimulatedObject && mSimulatedObject->InvMass() != 0)
            {
                SyncSimObj();
                mSimulatedObject->WakeUp();
            }
            else
            {
                mControl = simAICtrl;
                return; // release code won't break;
            }
        }
        InitAllVirtualCM();
    }
}

void SimState::InitAllVirtualCM()
{
    InitVirtualCM(-1);
}

void SimState::SetHasMoved(bool in_hasMoved)
{
    if (GetCollisionObject())
    {
        GetCollisionObject()->SetHasMoved(in_hasMoved);
    }
}

void SimState::SetSimulatedObject(SimulatedObject* inObject) 
{ 
    if (mSimulatedObject)
    {
        SimEnvironment::RemoveObject(mSimulatedObject);
        mSimulatedObject->Release();
    }
    
    mSimulatedObject = inObject;
    mSimulatedObject->AddRef();
    SimEnvironment::AddObject(mSimulatedObject);
}

void SimState::SetCollisionObject(CollisionObject* inObject)
{
    if (mCollisionObject)
        mCollisionObject->Release();
    
    rAssert(mCollisionObject == NULL); 
    mCollisionObject = inObject;
    mCollisionObject->AddRef();
}

float SimState::GetSphereRadius() const
{
    if (mCollisionObject)
        return mCollisionObject->GetCollisionVolume()->mSphereRadius;
    else
        return 1.0f;
}

void SimState::SetPhysicsProperties(PhysicsProperties* inProperties)
{
    if (mCollisionObject)
        mCollisionObject->SetPhysicsProperties(inProperties);
    if (mSimulatedObject && dynamic_cast<PhysicsObject*>(mSimulatedObject))
        dynamic_cast<PhysicsObject*>(mSimulatedObject)->SetPhysicsProperties(inProperties);
}

PhysicsProperties *SimState::GetPhysicsProperties() const
{
    if (mCollisionObject)
        return mCollisionObject->GetPhysicsProperties();
    if (mSimulatedObject && dynamic_cast<PhysicsObject*>(mSimulatedObject))
        return dynamic_cast<PhysicsObject*>(mSimulatedObject)->GetPhysicsProperties();
    
    return NULL;
}

void SimState::SyncSimObj()
{
    rAssert(mSimulatedObject != NULL);
    mSimulatedObject->SyncSimObj(false);
}

void SimState::SetScaleFromTransform()
{
    mScale = ComputeScaleFromMatrix(mTransform, MILLI_EPS);
}

void SimState::GetVelocity(const rmt::Vector& inPosition, rmt::Vector& oVelocity, int inIndex)
{
    Vector tmp;
    tmp.Sub(inPosition, *((Vector*)mTransform.m[3]));
    oVelocity.CrossProduct(mVelocityState.mAngular, tmp);
    oVelocity.Add(mVelocityState.mLinear);
}

void SimState::AddVirtualCM( int, VirtualCMMode inBits )
{
    rAssert(!mVirtualCM);
    if (mVirtualCM)
        return;

    mVirtualCM = new VirtualCM(inBits);
    InitVirtualCM();

}

void SimState::InitVirtualCM( int )
{
    if (mVirtualCM)
    {
        mVirtualCM->InitLinear( GetPosition(), GetLinearVelocity() );
        //mVirtualCM->InitAngular();
    }
}

void SimState::AddCacheToVirtualCM( const Vector& inV, const Vector& inW, int )
{
    if(mVirtualCM)
        mVirtualCM->AddObjectCache( inV, inW );
}

void SimState::SetTransform(const rmt::Matrix& inTransform, float dt)
{
    mObjectMoving = !SameMatrix(mTransform, inTransform);
    
    if (mControl == simAICtrl)
    {
        if (mObjectMoving)
        {
            /* this would be extra safe but would be expensive. Would also require to maintain
               a mDirtyScale so that the physics object and the collision object could be 
               update for that new scale if any.

            // check for new scale
            float newScale = ComputeScaleFromMatrix(inTransform);
            if (Fabs(mScale - newScale)/mScale > MILLI_EPS)
            {
                SetScale(newScale);
            }
            */
            //rAssert (Fabs(mScale - ComputeScaleFromMatrix(inTransform))/mScale < CENTI_EPS);
            
            if (dt != 0)
            {
                // must set the velocities for collision purposes.
                ExtractVelocityFromMatrix(mTransform, inTransform, mScale, dt, mVelocityState);
            }
            else
            {
                ResetVelocities();
                
                // object relocation: the safe next step scheme for collision detection doesn't apply anymore
                if (mCollisionObject)
                {
                    mCollisionObject->Relocated();
                    mCollisionObject->Update();
                }
            }
        }
        else
        {
            ResetVelocities();
        }
    }
    
    if (mCollisionObject && mObjectMoving)
    {
        MoveCollisionObject(mTransform, inTransform);
    }
    
    mTransform = inTransform;
    
    if (mVirtualCM)
        mVirtualCM->Update( GetPosition(), GetLinearVelocity(), dt );

    if (!mArticulated) // articulated object updates mApproxSpeedMagnitude in UpdateHasMoved
    {
        // mApproxSpeedMagnitude will be used for estimating the next collision time for this object
        float tmp = mVelocityState.mLinear.DotProduct(mVelocityState.mLinear);
        if (tmp > Sqr(UpApproxSpeedMagnitude()))
        {
            mApproxSpeedMagnitude = Sqrt(tmp);
            if (mCollisionObject)
                mCollisionObject->Relocated();
        }
        else if (tmp < DownApproxSpeedMagnitude())
        {
            mApproxSpeedMagnitude = Sqrt(tmp);
        }
    }
}

void SimState::MoveCollisionObject(const Matrix& previousTransform, const Matrix& newTransform)
{
    Matrix m;
    m.Invert(previousTransform);
    Vector p0, dp;
    m.Transform(mCollisionObject->GetCollisionVolume()->mPosition, &p0);
    newTransform.Transform(p0, &dp);
    dp.Sub(mCollisionObject->GetCollisionVolume()->mPosition);
    mCollisionObject->Moved(dp);
}

void SimState::DynamicPositionAdjustment(const rmt::Vector& inDeltaPosition, float inDt, bool adjustSpeed)
{
    if (mSimulatedObject && mControl == simSimulationCtrl)
    {
        rAssert(inDt > 0);
        rmt::Matrix currentTransform = mTransform;
        mTransform.Row(3).Add(inDeltaPosition);
        MoveCollisionObject(currentTransform, mTransform);
        if(adjustSpeed)
        {
            mVelocityState.mLinear.ScaleAdd(rmt::Inverse(inDt), inDeltaPosition);
        }
    }
}

void SimState::DebugDisplay(int debugIndex)
{
    if(debugIndex & 1)
    {
        tColour colour(0, 255, 0);
        static float speedScale = 0.1f;
        Vector speed = mVelocityState.mLinear;
        
        speed.ScaleAdd(mTransform.Row(3), speedScale, speed);
        dLine2(mTransform.Row(3), speed, colour);
    }
    if(debugIndex & 4 && mVirtualCM)
    {
        mVirtualCM->DebugDisplay();
    }
}

int FindMaxCollisionCollisionVolumeJointIndex(CollisionVolume* inCollisionVolume)
{
    int ji = inCollisionVolume->ObjRefIndex();
    if (inCollisionVolume->SubVolumeList())
    {
        for (int i=0; i<inCollisionVolume->SubVolumeList()->GetSize(); i++)
        {
            ji = rmt::Max(ji, inCollisionVolume->SubVolumeList()->GetAt(i)->ObjRefIndex());
        }
    }
    return ji;
}

void SimState::SwapCollisionVolume(CollisionVolume* inCollisionVolume)
{
    // if there is a simulated object, swapping the collisionvolume becomes 
    // a little more tricky cause the object's mass, cm, inertia matrix etc are
    // extracted from the collisionvolume and the simulated behaviour could 
    // become pretty weird. Let me know if it is neccessary and we can discuss...
    rAssert(inCollisionVolume && mCollisionObject && !mSimulatedObject);
    if (inCollisionVolume && mCollisionObject && !mSimulatedObject)
    {
        //int ji = FindMaxCollisionCollisionVolumeJointIndex(inCollisionVolume);
        // also this won't work for collision volume hierarchy.
        //rAssert(ji < mCollisionObject->GetNumJoint());
        rAssert (inCollisionVolume->Type() != BBoxVolumeType && mCollisionObject->GetCollisionVolume()->Type() != BBoxVolumeType);
        if (inCollisionVolume->Type() != BBoxVolumeType && mCollisionObject->GetCollisionVolume()->Type() != BBoxVolumeType)
        {
            mCollisionObject->SetCollisionVolume(inCollisionVolume);
        }
    }
}



} // sim
