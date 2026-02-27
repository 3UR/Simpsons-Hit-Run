
#include "simcommon/simstatearticulated.hpp"
#include "simcommon/dline2.hpp"
#include "simcollision/collisionobject.hpp"
#include "simphysics/articulatedphysicsobject.hpp"

//#include "simflexible/flexiblejointobject.hpp"

#include "p3d/utility.hpp"
#include "p3d/anim/skeleton.hpp"

//#include <poser/pose.hpp>

using namespace RadicalMathLibrary;


namespace sim
{

bool SimStateArticulated::sUseInventoryObject = true;

SimStateArticulated* SimStateArticulated::CreateSimStateArticulated(poser::Pose* inPose, SimStateArticulated* inSimState, const char* inName)
{
    rAssert(inSimState->mArticulated == false || inSimState->mConvertedToRigidBody == true);

    SimulatedObject* simObj = inSimState->GetSimulatedObject();

    SimStateArticulated* simState = CreateSimStateArticulated(inPose, inSimState->GetCollisionObject(), simObj);

    if (inName)
    {
        if (simState->GetCollisionObject())
        {
            simState->GetCollisionObject()->SetName(inName);
        }
        if (simObj)
        {
            simObj->SetName(inName);
        }
    }
    return simState;
}

SimStateArticulated* SimStateArticulated::CreateSimStateArticulated(tUID inUid, poser::Pose* inPose, SimStateAttributes inAttrib, tEntityStore* inStore)
{
    CollisionObject * collObj = NULL;
    SimulatedObject * simObj  = NULL;
    SkeletonInfo* info = NULL;

    if (inAttrib & SimStateAttribute_Collision || inAttrib == SimStateAttribute_Default)
    {
        if (inStore)
            collObj = p3d::find<CollisionObject>(inStore, inUid);
        else
            collObj = p3d::find<CollisionObject>(inUid);
        
        if (!collObj)// && inAttrib != SimStateAttribute_Default)
        {
            return NULL;
        }
        if (collObj && collObj->GetSimState() == NULL && collObj->GetNumJoint() == 0)
        {
            return NULL;
        }
    }
    
    if (inAttrib & SimStateAttribute_Simulated || inAttrib == SimStateAttribute_Default)
    {
        if (inStore)
            simObj = p3d::find<ArticulatedPhysicsObject>(inStore, inUid);
        else
            simObj = p3d::find<ArticulatedPhysicsObject>(inUid);
        
        if (simObj)
        {
            // need joint axis, this is temps and should be provided by the skeleton 
            // ot other skeleton info class.
    
            if (inStore)
                info = p3d::find<SkeletonInfo>(inStore, inPose->GetSkeleton()->GetUID());
            else
                info = p3d::find<SkeletonInfo>(inPose->GetSkeleton()->GetUID());

            if (info == NULL)
            {
                info = new SkeletonInfo(inPose->GetSkeleton());
                if (inStore)
                {
                    inStore->Store(info);
                }
                else
                {
                    p3d::inventory->Store(info);
                }
            }
            rAssert(info);
        }

        if (!simObj && inAttrib != SimStateAttribute_Default)
        {
            return NULL;
            //         simObj = p3d::find<FlexibleObject>(inUid);
        }
    }
    ((ArticulatedPhysicsObject*)simObj)->SetSkeletonInfo(info);
    sUseInventoryObject = false;
    SimStateArticulated* res = CreateSimStateArticulated(inPose, collObj, simObj); 
    sUseInventoryObject = true;
    ((ArticulatedPhysicsObject*)simObj)->SetSkeletonInfo(NULL);
    return res;
}

SimStateArticulated* SimStateArticulated::CreateSimStateArticulated(const char* inName, poser::Pose* inPose, SimStateAttributes inAttrib, tEntityStore* inStore)
{
    return CreateSimStateArticulated(tEntity::MakeUID(inName), inPose, inAttrib, inStore);
}

SimStateArticulated* SimStateArticulated::CreateSimStateArticulated(poser::Pose* inPose, CollisionObject* collObj, SimulatedObject* simObj)
{
    inPose->SetRestPose(); // make sure the pose is set to something
    inPose->GetJoint(0)->SetWorldTransform(inPose->GetJoint(0)->GetObjectTransform());

    SimStateArticulated* simState = new SimStateArticulated(inPose);
    
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
                if (((SimStateArticulated*)simObj->GetSimState())->mConvertedToRigidBody)
                {
                    simState->mConvertedToRigidBody = true;
                }
                simObj = simObj->Clone(simState);
            }
            else if (!sUseInventoryObject)
            {
                simObj = simObj->Clone(simState);
            }
            else
            {
                simObj->SetSimState(simState);
            }

            simState->SetSimulatedObject(simObj);

            if (simObj->Type() == ArticulatedObjectType)
            {
                ArticulatedPhysicsObject* artObj = (ArticulatedPhysicsObject*)simObj;
                artObj->SetPose(inPose);
            }
        }
        if (collObj)
        {
            if (collObj->GetSimState())
            {
                if (((SimStateArticulated*)collObj->GetSimState())->mConvertedToRigidBody)
                {
                    simState->mConvertedToRigidBody = true;
                }
                collObj = collObj->Clone(simState);
            }
            else if (!sUseInventoryObject)
            {
                collObj = collObj->Clone(simState);
            }
            else
            {
                collObj->SetSimState(simState);
            }
            simState->SetCollisionObject(collObj);
            collObj->Update();
            collObj->mCollisionVolume->UpdateBBox();
            
            if (!collObj->IsStatic())
                collObj->Relocated();
            
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

//
//
//

SimStateArticulated::SimStateArticulated(poser::Pose* inPose, SimControlEnum inControl) 
    : SimState(inControl),
    mPose(NULL),
    mMinJointCollisionIndex(0),
    mConvertedToRigidBody(false)
{
    rAssert(inPose);

    mArticulated = true;
    
    int jointCount = inPose->GetJointCount();
    
    mJointState = new JointState[jointCount];
    
    //mFlexibleJointCache = new FlexibleJointCache[jointCount];
    
    SetPose(inPose);
}

SimStateArticulated::~SimStateArticulated()
{
    int  i;

    delete[] mJointState;

    if (mSimulatedObject)
    {
        GetArtPhysObject()->SetPose(NULL);
    }

    if ( mPose )
    {
        mPose->Release();
        mPose = NULL;
    }
    //for ( i=0; i<mFlexibleJointObjects.GetSize(); i++)
    //{
    //    mFlexibleJointObjects[i]->Release();
    //}
    //delete[] mFlexibleJointCache;

    for ( i=0 ; i<mVirtualCMList.GetSize() ; i++ )
        delete mVirtualCMList[i];
}

void SimStateArticulated::SetCollisionObject(CollisionObject* inObject)
{
    SimState::SetCollisionObject(inObject);
    
    // find the first joint that has a collisoin volume that is not a BBoxVolume.
    if (mCollisionObject->IsStatic())
    {
        mCollisionObject->SetIsStatic(false);
        mCollisionObject->GetCollisionVolume()->UpdateAll();
        mCollisionObject->SetIsStatic(true);
    }
    else
    {
        mCollisionObject->GetCollisionVolume()->UpdateAll();
    }

    mMinJointCollisionIndex = 0;
    
    for (int i=0; i<mCollisionObject->GetNumJoint(); i++)
    {
        CollisionVolume* localVolume = mCollisionObject->GetCollisionVolume()->GetSubCollisionVolume(i);
        if (localVolume)
        {
            mMinJointCollisionIndex = i;
            break;
        }
    }
}

void SimStateArticulated::SetPose(poser::Pose* inPose)
{
    rAssert(mPose == NULL); // or should make sure it works...
    
    if (mPose)
        mPose->Release();
    
    rAssert(inPose);
    
    mPose = inPose;
    mPose->AddRef();
    
    int i;
    for (i=0; i<mPose->GetJointCount(); i++)
    {
        mJointState[i].SetJoint(mPose->GetJoint(i));
    }
    if (mSimulatedObject && mSimulatedObject->Type() == ArticulatedObjectType)
    {
        GetArtPhysObject()->SetPose(inPose);
    }
}

void SimStateArticulated::SetTransform(const rmt::Matrix& inTransform, float inDt)
{
    SimState::SetTransform(inTransform, inDt);
}

void SimStateArticulated::UpdateJointState(float inDt)
{
    if (mSimulatedObject)
    {
        GetArtPhysObject()->UpdatePoseMatrix();
    }
    UpdateHasMoved();

    //Update each vcm.
    ArticulatedPhysicsObject* physobj = GetArtPhysObject();
    for( int i=0 ; i<mVirtualCMList.GetSize() ; i++ )
    {
        int index;
        index = mVirtualCMList[i]->GetIndex();

        Vector p(0,0,0), v(0,0,0);
        if (physobj)
        {
            PhysicsJoint* jnt = physobj->GetJoint(index);
            if(jnt)
            {
                jnt->CMPosition(p);
            }
        }
        else
        {
            p = GetPosition(index);
        }
        GetVelocity(p, v, index);
        mVirtualCMList[i]->Update( p, v, inDt );
    }
}

void SimStateArticulated::ConvertToRigidBody()
{
    if (mConvertedToRigidBody)
        return;

    rmt::Matrix savedm = mTransform;
    mTransform.Identity();
    mPose->Clean();
    mPose->SetRestPose();

    if (mCollisionObject)
    {
        mCollisionObject->ConvertToRigidBody();
        mCollisionObject->Relocated();
    }

    if (GetArtPhysObject())
        GetArtPhysObject()->ConvertToRigidBody();

    mTransform = savedm;
    mConvertedToRigidBody = true;
    mArticulated = false;
}

void SimStateArticulated::AddVirtualCM( int inIndex, VirtualCMMode inBits )
{
    rAssert((inIndex >= 0) && (inIndex < mPose->GetJointCount()));
    if ((inIndex < 0) || (inIndex >= mPose->GetJointCount()))
        return;

    if (inIndex >= 0 )
    {
        if (GetArtPhysObject())
        {
            if (GetArtPhysObject()->mJoints[inIndex])
            {
                mVirtualCMList.Add(new JointVirtualCM( inBits, inIndex) );
            }
        }
    }
    else
        SimState::AddVirtualCM(inBits);
}

void SimStateArticulated::InitVirtualCM( int inJointIndex )
{
    if (inJointIndex >= 0 )
    {
        for( int i=0 ; i<mVirtualCMList.GetSize() ; i++ )
        {
            if (mVirtualCMList[i]->GetIndex() == inJointIndex )
            {
                Vector p, v;
                if (GetArtPhysObject())
                {
                    PhysicsJoint* jnt = GetArtPhysObject()->GetJoint(inJointIndex);
                    if(jnt)
                    {
                        jnt->CMPosition(p);
                    }
                }
                else
                {
                    p = GetPosition(inJointIndex);
                }
                GetVelocity(p, v, inJointIndex);
                mVirtualCMList[i]->InitLinear(p, v);
                return;
            }
        }
    }
    else
        SimState::InitVirtualCM();
}

// Release the Pose and related information to save memory for objects 
// that don't require it
void SimStateArticulated::ReleasePose(void)
{
    if (mPose != NULL)
    {
        if (mSimulatedObject)
        {
            GetArtPhysObject()->SetPose(NULL);
        }
        if (mPose != NULL)
        {
            mPose->Release();
            mPose = NULL;
        }
    }
    delete[] mJointState;
    mJointState = NULL;
}

void SimStateArticulated::InitAllVirtualCM()
{
    int index;
    for( int i=0 ; i<mVirtualCMList.GetSize() ; i++ )
    {
        index = mVirtualCMList[i]->GetIndex();
        Vector p, v;
        if (GetArtPhysObject())
        {
            PhysicsJoint* jnt = GetArtPhysObject()->GetJoint(index);
            if(jnt)
            {
                jnt->CMPosition(p);
            }
        }
        else
        {
            p = GetPosition(index);
        }
        GetVelocity(p, v, index);
        mVirtualCMList[i]->InitLinear(p, v);
    }
    SimState::InitAllVirtualCM();
}

VirtualCM* SimStateArticulated::GetVirtualCM(int inJointIndex)
{
    if (inJointIndex >= 0 )
    {
        for( int i=0 ; i<mVirtualCMList.GetSize() ; i++ )
        {
            if (mVirtualCMList[i]->GetIndex() == inJointIndex)
            {
                return mVirtualCMList[i];
            }
        }
        return NULL;
    }
    else
        return SimState::GetVirtualCM();
}

void SimStateArticulated::AddCacheToVirtualCM( const rmt::Vector& inV, const rmt::Vector& inW, int inJointIndex)
{
    if (inJointIndex >= 0 )
    {
        for( int i=0 ; i<mVirtualCMList.GetSize() ; i++ )
        {
            if (mVirtualCMList[i]->GetIndex() == inJointIndex)
            {
                mVirtualCMList[i]->AddObjectCache(inV, inW );
                break;
            }
        }
    }
    else
        SimState::AddCacheToVirtualCM(inV, inW );
}


void SimStateArticulated::StartModifyingJointVisibility()
{
    if (mSimulatedObject)
        GetArtPhysObject()->StartModifyingJointVisibility();
}

void SimStateArticulated::SetJointVisibility(int inJoint, bool inVisible)
{
    if (mSimulatedObject)
        GetArtPhysObject()->SetJointVisibility(inJoint, inVisible);

    // mCollisionObject also has setvisibility by owner. We must use byjoint
    // here because the physics object doesn't have the concept of owner, all
    // we can do is to turn on/off an entire joint. If more than one props is
    // attached to a same joint, this system may not work very well.
    if (mCollisionObject)
        mCollisionObject->SetVisibility(inJoint, inVisible);
}

void SimStateArticulated::EndModifyingJointVisibility()
{
    if (mSimulatedObject)
        GetArtPhysObject()->EndModifyingJointVisibility();

    if (mCollisionObject)
        mCollisionObject->GetCollisionVolume()->UpdateBBoxVisibility();
}

SimState* SimStateArticulated::CreateJointSimState(int inJoint, const char* inName)
{
    SimState* newSimState = NULL;

    PhysicsObject* phys = NULL;
    CollisionObject* coll = NULL;

    if (mCollisionObject)
    {
        CollisionVolume* vol = mCollisionObject->GetCollisionVolume()->GetSubCollisionVolume(inJoint, false);

        // vol might contains subvolumes from sub joints and possibly more than one volume
        // associated with inJoint.
        if (vol)
        {
            int ownsub = 0;

            if (vol->HasSubVolume())
            {
                CollisionVolume* subvol = NULL;
                int i;
                int othersub = 0;
                for (i=0; i<vol->SubVolumeList()->GetSize(); i++)
                {
                    if (vol->SubVolumeList()->GetAt(i)->ObjRefIndex() != inJoint)
                    {
                        othersub++;
                    }
                    else if (subvol == NULL)
                    {
                        subvol = vol->SubVolumeList()->GetAt(i);
                    }
                }
                if (othersub > 0)
                {
                    ownsub = vol->SubVolumeList()->GetSize() - othersub;
                    rAssert(ownsub > 0 && subvol != NULL);

                    if (ownsub > 1)
                    {
                        subvol = new BBoxVolume;

                        for (i=0; i<vol->SubVolumeList()->GetSize(); i++)
                        {
                            CollisionVolume* tmp = vol->SubVolumeList()->GetAt(i);
                            if (tmp->ObjRefIndex() == inJoint)
                            {
                                subvol->AddSubVolume(tmp);
                            }
                        }
                    }
                    vol = subvol;
                }
            }
            // need to addref and release in case vol is a newly created
            // BBoxVolume from above.
            CollisionVolume* newvol = vol->Clone();
            newvol->SetObjRefIndex(0);

            if (ownsub > 1)
            {
                vol->EmptySubVolume();
                vol->Release();
            }
            coll = new CollisionObject(newvol);
            coll->SetVisibility(0, true);

            if (inName)
                coll->SetName(inName);
            else
                coll->SetName("JointObjectDefaultName");
        }
    }
    if (mSimulatedObject && GetArtPhysObject()->GetJoint(inJoint))
    {
        PhysicsJoint* joint = GetArtPhysObject()->GetJoint(inJoint);

        if (joint->LocalMass() > 0)
        {
            phys = new PhysicsObject(   mSimulatedObject->GetPhysicsProperties(), 
                                        joint->DCMPositionLocal(),
                                        joint->LocalInertiaMatrix(),
                                        joint->LocalVolume() );

            if (inName)
                phys->SetName(inName);
            else
                phys->SetName("JointObjectDefaultName");
        }
    }
    if (coll || phys)
        return SimState::CreateSimState(coll, phys);
    else
        return NULL;
}

void SimStateArticulated::StoreJointState(float inDt)
{
    for (int i=0; i<mPose->GetJointCount(); i++)
    {
        mJointState[i].StoreJointState(inDt);
    }
}

void SimStateArticulated::ResetVelocities()
{ 
    SimState::ResetVelocities();

    if ( mPose )
    {
        for (int i=0; i<mPose->GetJointCount(); i++)
        {
            mJointState[i].ResetVelocities();
        }
    }
}

void SimStateArticulated::UpdateHasMoved()
{
    if (mCollisionObject && !mCollisionObject->HasMoved())
    {
        // if no root motion has been provided to the character, it is maybe because the root motion is 
        // in the aniamtion, if any. test for translation in the joint mMinJointCollisionIndex in case.
        Vector dp;
        dp.Sub(mJointState[mMinJointCollisionIndex].GetPosition(), *((Vector*)(&mJointState[mMinJointCollisionIndex].GetPreviousTransform().m[3])));
        
        if (dp.DotProduct(dp) > 0)
        {
            mCollisionObject->Moved(dp);
        }
        else
        {
            mCollisionObject->SetHasMoved(true); // root might not be moving but animation still runs on it.
        }
    }
    if (mArticulated)
    {
        // mApproxSpeedMagnitude will be used for estimating the next collision time for this object
        mJointState[mMinJointCollisionIndex].UpdateVelocity();
        float tmp = mJointState[mMinJointCollisionIndex].mVelocityState.mLinear.MagnitudeSqr();
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
    /*
        Note that this mechanism still fail for non preserved bone 
        length articulated object. The best option in that case would be to call 
        mCollisionObject->Update() manually before calling DetectCollision.

        Being extra safe here would ruin a lot of optimisation that still works
        for most of the objects.
    */
}

const rmt::Matrix& SimStateArticulated::GetTransform(int inIndex) const
{
    if (inIndex >= 0 && mPose != NULL )
        return mPose->GetJoint(inIndex)->GetWorldMatrix();
    else
        return SimState::GetTransform();
}

const rmt::Vector& SimStateArticulated::GetPosition(int inIndex) const
{
    if (inIndex >= 0)
        return mPose->GetJoint(inIndex)->GetWorldMatrix().Row(3);
    else
        return SimState::GetPosition();
}

void SimStateArticulated::GetVelocity(const rmt::Vector& inPosition, rmt::Vector& oVelocity, int inIndex)
{
    if (inIndex >= 0 && mJointState )
    {
        mJointState[inIndex].GetVelocity(inPosition, oVelocity);
    }
    else
        SimState::GetVelocity(inPosition, oVelocity);
}

SimulatedObject* SimStateArticulated::GetSimulatedObject(int inIndex)
{
    SimulatedObject* ret = mSimulatedObject;
    
    if (inIndex > 0)
    {
        ret = NULL;
    }
    return ret; 
}

void SimStateArticulated::DebugDisplay(int debugIndex)
{
    DrawLineToggler toggler;

    //SimState::DebugDisplay(debugIndex);
    
    tColour colour(0, 255, 0);
    static float speedScale = 0.1f;
    
    if(debugIndex & 1)
    {
        for (int i=0; i<mPose->GetJointCount(); i++)
        {
            Vector speed = mJointState[i].GetLinearVelocity();
            speed.ScaleAdd(mJointState[i].GetPosition(), speedScale, speed);
            dStreamLine(mJointState[i].GetPosition(), speed, colour, colour);
        }
    }

    if(debugIndex & 4 )
    {
        for (int i=0 ; i<mVirtualCMList.GetSize() ; i++ )
        {
            mVirtualCMList[i]->DebugDisplay();
        }
    }
    
    if(debugIndex & 2)
    {
        //for (int i=0; i<mFlexibleJointObjects.GetSize(); i++)
        //{
        //    mFlexibleJointObjects[i]->mFlexJointModifier->DebugDisplay();
        //}
        
        for (int j=0; j<mPose->GetJointCount(); j++)
        {
            mJointState[j].DebugDisplay();
        }
        if (mSimulatedObject && mSimulatedObject->Type() == ArticulatedObjectType)
        {
            //GetArtPhysObject()->GetSkeletonInfo()->DebugDisplay(mPose);
            static float sizeScale = 0.1f;
            static float defScale = 10.0f;
            for (int j=0; j<GetArtPhysObject()->NumSimJoints(); j++)
            {
                tColour colOrigin(0, 200, 50);
                tColour colCM(200, 75, 0);
                tColour colDef(200, 200, 200);
                
                PhysicsJoint* jnt = GetArtPhysObject()->GetSimJoint(j);

                if (!jnt->IsVisible())
                    continue;

                float len = GetArtPhysObject()->GetSkeletonInfo()->mJointInfo[jnt->Index()].mLength;
                float size = len * sizeScale;
                Vector sizeVector(size, size, size), cm;
                dBox3(jnt->Origin(), sizeVector, colOrigin);
                jnt->CMPosition(cm);
                dBox3(cm, sizeVector, colCM);
                float def = jnt->Deformation();
                
                GetArtPhysObject()->JointAxis(jnt->Index(), sizeVector, true);
                sizeVector.ScaleAdd(jnt->Origin(), len, sizeVector);
                dLine2(jnt->Origin(), sizeVector, colOrigin, colCM);
                sizeVector.Set(1.0f, 0, 0);
                sizeVector.ScaleAdd(cm, def*defScale, sizeVector);
                dLine2(cm, sizeVector, colDef, colDef);
            }
            if (mCollisionObject)
            {
                tColour colCM(200, 75, 100);
                float size = mCollisionObject->GetCollisionVolume()->mBoxSize.x * sizeScale;
                Vector sizeVector(size, size, size), cm;
                GetArtPhysObject()->CMPosition(cm);
                dBox3(cm, sizeVector, colCM);
            }
        }
    }
}
//
// SimStateArticulated::JointState methods
//

SimStateArticulated::JointState::JointState()
{
    mVelocityUpdated = true;
}

SimStateArticulated::JointState::~JointState()
{
}

void SimStateArticulated::JointState::SetJoint(poser::Joint* inJoint)
{
    mJoint = inJoint;
}

void SimStateArticulated::JointState::DebugDisplay()
{
}

void SimStateArticulated::JointState::StoreJointState(float dt)
{
    if (dt == 0)
    {
        mVelocityState.Reset();
        mVelocityUpdated = true;
    }
    else
    {
        mPreviousDt = dt;
        mPreviousTransform = mJoint->GetWorldTransform().GetMatrix();
        mVelocityUpdated = false;
    }
}

void SimStateArticulated::JointState::GetVelocity(const rmt::Vector& inPosition, rmt::Vector& oVelocity)
{
    UpdateVelocity();
    
    Vector tmp;
    tmp.Sub(inPosition, mJoint->GetWorldTransform().GetMatrix().Row(3));
    oVelocity.CrossProduct(mVelocityState.mAngular, tmp);
    oVelocity.Add(mVelocityState.mLinear);
}

void SimStateArticulated::JointState::UpdateVelocity()
{
    if (!mVelocityUpdated)
    {
        mVelocityUpdated = true;
        float scale = 1.0f; // temp, to fix
        ExtractVelocityFromMatrix(mPreviousTransform, mJoint->GetWorldTransform().GetMatrix(), scale, mPreviousDt, mVelocityState);
    }
}

//
//
//

// the flexjointmodifier can be add directly to the joint state or can be added using that 
// method. If this method is used, the flexjointmodifier will participate to the collision response
/*void SimStateArticulated::AddFlexJointModifier(FlexJointModifier* inFlexJointModifier)
{
    inFlexJointModifier->SetSimStateArticulated(this);
    FlexibleJointObject* newObject = new FlexibleJointObject(inFlexJointModifier, GetPhysicsProperties());
    newObject->AddRef();
    newObject->SetSimState(this);
    
    mFlexibleJointObjects.Add(newObject);
    SetFlexibleJointCache();
}

FlexJointModifier* SimStateArticulated::GetFlexJointModifier(tUID inUID)
{
    FlexJointModifier* ret = NULL;
    
    for (int i=0; i<mFlexibleJointObjects.GetSize(); i++)
    {
        if (inUID == mFlexibleJointObjects[i]->GetUID())
        {
            ret = mFlexibleJointObjects[i]->mFlexJointModifier;
            break;
        }
    }
    return ret;
}

void SimStateArticulated::RemoveFlexJointModifier(tUID inUID)
{
    FlexJointModifier* ret = NULL;
    
    for (int i=0; i<mFlexibleJointObjects.GetSize(); i++)
    {
        if (inUID == mFlexibleJointObjects[i]->GetUID())
        {
            mFlexibleJointObjects[i]->Release();
            mFlexibleJointObjects.RemoveAt(i);
            break;
        }
    }
    SetFlexibleJointCache();
}

void SimStateArticulated::SynchronizeFlexJointModifier()
{
    bool removed = false;
    
    for (int i=0; i<mFlexibleJointObjects.GetSize(); i++)
    {
        if (!mFlexibleJointObjects[i]->IsValid())
        {
            mFlexibleJointObjects[i]->Release();
            mFlexibleJointObjects[i] = NULL;
            removed = true;
        }
    }
    if (removed)
        mFlexibleJointObjects.Compact();
    
    SetFlexibleJointCache();
}

void SimStateArticulated::SetFlexibleJointCache()
{
    for (int i=0; i<mPose->GetJointCount(); i++)
    {
        mFlexibleJointCache[i].mFlexibleJointObject = NULL;
        for (int j=0; j<mFlexibleJointObjects.GetSize(); j++)
        {
            short particleIndex = mFlexibleJointObjects[j]->PoseToParticleIndex(i);
            if (particleIndex > 0)
            {
                rAssert(mFlexibleJointCache[i].mFlexibleJointObject == NULL); // otherwise there is conflict
                mFlexibleJointCache[i].mFlexibleJointObject = mFlexibleJointObjects[j];
                mFlexibleJointCache[i].mParticleIndex = particleIndex;
                break;
            }
        }
    }
}
*/
void SimStateArticulated::SetMinJointCollisionIndex(int inMinJointCollisionIndex)
{ 
    rAssert(inMinJointCollisionIndex < mPose->GetJointCount()); 
    mMinJointCollisionIndex = inMinJointCollisionIndex; 
}

} // sim
