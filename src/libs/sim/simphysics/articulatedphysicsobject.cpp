
#include "simphysics/articulatedphysicsobject.hpp"

#include "p3d/anim/skeleton.hpp"
#include <string.h>
#include "simcommon/simutility.hpp"

using namespace RadicalMathLibrary;

namespace sim
{

// when we add an impulse to a joint, its subjoint muscle will absorb some of it if
// not totally stiff. ABSORB_CACHE_FEEDBACK = false will diseable this behavior => ~30% faster.
static bool ABSORB_CACHE_FEEDBACK = 1;
static float TIME_UPDATE_INERTIA = 0.1f; // sec

ArticulatedPhysicsObject::ArticulatedPhysicsObject(PhysicsProperties* inProperties, pPhysicsJoint* inJoints, int inNumSimJoints)
    : PhysicsObject(inProperties),
    mSimJoints(inJoints),
    mJoints(NULL),
    mNumSimJoints(inNumSimJoints),
    mNumJoints(0),
    mPose(NULL),
    mTimeSinceComputeInertiaMatrix(0),
    mCollidingJoint(NULL),
    mSelfCollisionCenter(NULL),
    mSelfCollision(false),
    mAllCacheEmpty(false),
    mSkeletonInfo(NULL),
    mImpulseReactionEnabled(false)
{
    mType = ArticulatedObjectType;
    
    SimTime t(TIME_UPDATE_INERTIA);
    
    mTimeComputeInertiaMatrix = t.Value();
    mTimeSinceComputeInertiaMatrix = mTimeComputeInertiaMatrix;
    
    for (int i=0; i<mNumSimJoints; i++)
    {
        mSimJoints[i]->SetParentObject(this);
        mVolumei += mSimJoints[i]->mLocalVolume;
    }


    mVolume = mVolumei;
    mMass = mVolume * Density();
    mInvMass = rmt::Inverse(mMass);
    mInertiaMatrix0.Identity(); // can't calculated the real inertia matrix until the joint are positionned be the pose
    mInertiaMatrix0.Scale(mMass);
    mInertiaMatrix = mInertiaMatrix0;
    
    t.SetValueCGS(0.333f);//0.167));
    mDissipationDeformationRate = 1.0f/t.Value();
    
    t.SetValueCGS(0.2f); //0.05f);//0.08));//05));
    mDissipationDeformationSpeedRate = 1.0f/t.Value();
    
    t.SetValueCGS(0.25f);
    mDissipationInternalRate = 1.0f/t.Value();
}

ArticulatedPhysicsObject::~ArticulatedPhysicsObject()
{
    tRefCounted::Release( mSkeletonInfo );
    tRefCounted::Release( mPose );
    
    int i;
    for( i = 0; i < mNumSimJoints; ++i )
    {
        delete mSimJoints[ i ]; 
    }

    delete[] mSimJoints;
    delete[] mJoints;
}

void ArticulatedPhysicsObject::SetPose(poser::Pose* inPose)
{
    if (inPose == NULL)
    {
        tRefCounted::Release( mPose );
        return;
    }
        
    if (inPose != mPose)
    {
        tRefCounted::Assign( mPose, inPose );
        
        InitFromPose();
        UpdatePoseMatrix();
        UpdateForPhysicsProperties();
    }
}

void ArticulatedPhysicsObject::SetSkeletonInfo(SkeletonInfo* inSkeletonInfo)
{
    tRefCounted::Assign(mSkeletonInfo,inSkeletonInfo);
}

void ArticulatedPhysicsObject::InitFromPose()
{
    // not all the pose joint has physicsjoint => mNumSimJoints < pose->GetNumJoint()
    // we need a quick access from poseJointIndex to physicsJoint.
    tSkeleton* skel = mPose->GetSkeleton();
    
    if (mJoints != NULL) // the articulated has been used before.
    {
        rAssert(mNumJoints == mPose->GetJointCount());

        for (int i=0; i<mNumSimJoints; i++)
        {
            int currentIndex = mSimJoints[i]->mIndex;
            mJoints[currentIndex]->SetTransform(mPose->GetJoint(currentIndex)->GetWorldMatrix());
        }
    }
    else
    {
        mNumJoints = mPose->GetJointCount();
        mJoints = new pPhysicsJoint[mNumJoints];
        memset(mJoints, 0, mNumJoints*sizeof(pPhysicsJoint));
        int i;
        for (i=0; i<mNumSimJoints; i++)
        {
            int index = mSimJoints[i]->mIndex;
            mJoints[index] = mSimJoints[i];
        }
    
        /*
        // make sure that if there is a collision on a subjoint taht has 
        // no PhysicsJoint associated, the collision is moved to the first 
        // non null parent.
        PhysicsJoint* lastJoint = mSimJoints[0];
        for (i=mSimJoints[0]->mIndex+1; i<mNumJoints; i++)
        {
        if (mJoints[i] == NULL)
        mJoints[i] = lastJoint;
        else
        lastJoint = mJoints[i];
        }
        */

        // need to setup the joint parent/child hierarchy
        for ( i = 0; i < mNumSimJoints; ++i )
        {
            int currentIndex = mSimJoints[i]->mIndex;
            int parentIndex = skel->GetJoint(currentIndex)->parentIndex;
            mSimJoints[i]->mParentIndex = parentIndex;
            do
            {
                if (mJoints[parentIndex] && currentIndex != parentIndex)
                {
                    rAssert(mSimJoints[i]->mParentSimJoint == NULL);
                    mSimJoints[i]->mParentSimJoint = mJoints[parentIndex];
                    mJoints[parentIndex]->mChildrenJoints.ResizeArray( mNumSimJoints );
                    mJoints[parentIndex]->mChildrenJoints.Add(mSimJoints[i]);
                    break;
                }
                parentIndex = skel->GetJoint(parentIndex)->parentIndex;
            } 
            while (parentIndex > 0);

            if (i > 0 && parentIndex == 0 && mJoints[parentIndex] != NULL && mSimJoints[i]->mParentSimJoint == NULL)
            {
                mSimJoints[i]->mParentSimJoint = mJoints[parentIndex];
                mJoints[parentIndex]->mChildrenJoints.ResizeArray( mNumSimJoints );
                mJoints[parentIndex]->mChildrenJoints.Add(mSimJoints[i]);
            }
        
            if (mSimJoints[i]->mInvStiffness > 0 || (mSimJoints[i]->mParentSimJoint && mSimJoints[i]->mParentSimJoint->mSimBranch == true))
                mSimJoints[i]->mSimBranch = true;
        
            mJoints[currentIndex]->SetTransform(mPose->GetJoint(currentIndex)->GetWorldMatrix());
        }
    }
    ComputeJointMasses();
}

void ArticulatedPhysicsObject::ComputeJointMasses()
{
    int i;
    PhysicsJoint* jnt;
    PhysicsJoint* subjnt;
    mVolume = 0;
    mMass = 0;

    for (i=0; i<mNumSimJoints; i++)
    {
        jnt = mSimJoints[i];
        jnt->mVolume = jnt->mLocalVolume;
        jnt->mMass = jnt->mLocalMass;
        jnt->mInertiaMatrix0 = jnt->mLocalInertiaMatrix0;
        jnt->mML2 = (jnt->mInertiaMatrix0.xx + jnt->mInertiaMatrix0.yy + jnt->mInertiaMatrix0.zz) * 0.33f;

        if (jnt->IsVisible())
        {
            mVolume += jnt->mVolume;
            mMass += jnt->mMass;
        }
    }
    if (mMass > 0)
    {
        mInvMass = Inverse(mMass);

        for (i=mNumSimJoints-1; i>=0; i--)
        {
            jnt = mSimJoints[i];
            for (int j=0; j<jnt->mChildrenJoints.GetSize(); j++)
            {
                subjnt = jnt->mChildrenJoints[j];
                if (subjnt->GetNumDOF() == 0 && subjnt->IsVisible())
                {
                    jnt->mVolume += subjnt->mVolume;
                    jnt->mMass += subjnt->mMass;
                    jnt->mML2 += subjnt->mML2;
                    jnt->mInertiaMatrix0.Add(subjnt->mInertiaMatrix0);
                }
            }
        }
    }
    else
    {
        // must set the object's control to ai before setting all the joints to invisible
        rAssert(mSimState->GetControl() == simAICtrl);
    }
    for (i=0; i<mNumSimJoints; i++)
    {
        jnt = mSimJoints[i];
        if (jnt->mMass > 0)
            jnt->mInvMass = Inverse(jnt->mMass);
    }

    // force recomputation of inertia
    mTimeSinceComputeInertiaMatrix = mTimeComputeInertiaMatrix;
    ComputeInertiaMatrix();
}

bool ArticulatedPhysicsObject::SetJointVisibility(int inJoint, bool inVisible)
{
    bool ret = false;

    if (mJoints[inJoint] != NULL && mJoints[inJoint]->IsVisible() != inVisible)
    {
        mJoints[inJoint]->SetVisible(inVisible);
    }
    return ret;
}

void ArticulatedPhysicsObject::EndModifyingJointVisibility()
{
    rAssert(mMass == 0); // must call StartModifyingJointVisibility
    ComputeJointMasses();
}

void ArticulatedPhysicsObject::JointAxis(int inIndex, rmt::Vector& axis, bool recompute) const
{
    if (recompute || mJoints[inIndex] == NULL)
    {
        mPose->GetJoint(inIndex)->GetWorldMatrix().RotateVector(JointInfo(inIndex).mAxis, &axis);
    }
    else
        axis = mJoints[inIndex]->mAxis;
}

void ArticulatedPhysicsObject::ConvertToRigidBody()
{
    mTimeSinceComputeInertiaMatrix = mTimeComputeInertiaMatrix;
    ComputeInertiaMatrix();
    mTimeComputeInertiaMatrix = VERY_LARGE; // don't need to compute it anymore.
    mRestingDetector.ComputeTypicalValues(0.016f);
    mRestingDetector.ResetRestingCondition();
}

SimulatedObject* ArticulatedPhysicsObject::Clone(SimState* inSimState)
{
    rAssert(inSimState); // SimState is unique for each object
    
    ArticulatedPhysicsObject* clone;
    clone = new ArticulatedPhysicsObject();
    SetClone(clone, inSimState);
    clone->mType = mType;
    return clone;
}

void ArticulatedPhysicsObject::SetClone(ArticulatedPhysicsObject* clone, SimState* inSimState) //const
{
    rAssert(dynamic_cast<SimStateArticulated*>(inSimState));
    
    PhysicsObject::SetClone(clone, inSimState);
    
    clone->mNumSimJoints = mNumSimJoints;

    if (mNumSimJoints > 0)
        clone->mSimJoints = new pPhysicsJoint[mNumSimJoints];

    int i;
    for (i=0; i<mNumSimJoints; i++)
    {
        clone->mSimJoints[i] = mSimJoints[i]->Clone();
        clone->mSimJoints[i]->SetParentObject(clone);
    }
    
    clone->mTimeSinceComputeInertiaMatrix = 0.0f;
    clone->mPose = static_cast<SimStateArticulated*>(inSimState)->GetPose();
    clone->mPose->AddRef();

    clone->mSkeletonInfo = mSkeletonInfo;
    if (clone->mSkeletonInfo)
        clone->mSkeletonInfo->AddRef();
    
    clone->InitFromPose();
    
    clone->mDissipationDeformationRate = mDissipationDeformationRate;
    clone->mDissipationDeformationSpeedRate = mDissipationDeformationSpeedRate;
    clone->mDissipationInternalRate = mDissipationInternalRate;
    clone->mCollidingJoint = NULL;
    clone->mSelfCollisionCenter = NULL;
    clone->mSelfCollision = false;
    clone->mAllCacheEmpty = true;
    mTimeStep = 0;
}

void ArticulatedPhysicsObject::Update(float inDt)
{
    mTimeStep = inDt;
    
    PhysicsObject::Update(mTimeStep);
    
    if (!ConvertedToRigidBody())
    {
        SimStats::StartSimulation();

        // simply dissipate the impulse into nothing for now
        float deformationRate      = Max(0.0f, 1.0f - inDt * mDissipationDeformationRate);
        float deformationSpeedRate = Max(0.0f, 1.0f - inDt * mDissipationDeformationSpeedRate);
        float deformationInternal  = Max(0.0f, 1.0f - inDt * mDissipationInternalRate);
    
        mAngularMomentum.Scale(deformationInternal);
        mSimState->GetAngularVelocity().Scale(deformationInternal);
    
        for (int i=0; i<mNumSimJoints; i++)
        {
            mSimJoints[i]->UpdateDeformation(deformationRate, deformationSpeedRate, inDt);
        }
    
        mTimeSinceComputeInertiaMatrix += mTimeStep;

        SimStats::EndSimulation();
    }
}

void ArticulatedPhysicsObject::UpdatePoseMatrix()
{
    if (!ConvertedToRigidBody())
    {
        for (int i=0; i<mNumSimJoints; i++)
        {
            mSimJoints[i]->SetTransform(mPose->GetJoint(mSimJoints[i]->mIndex)->GetWorldMatrix());
        }
        ComputeInertiaMatrix();
    }
}

void ArticulatedPhysicsObject::UpdateForPhysicsProperties()
{
    PhysicsObject::UpdateForPhysicsProperties();

    for (int i=0; i<mNumSimJoints; i++)
    {
        mSimJoints[i]->UpdateForPhysicsProperties();
    }
}

void ArticulatedPhysicsObject::ComputeInertiaMatrix()
{
    if (mMass > 0 && mTimeSinceComputeInertiaMatrix >= mTimeComputeInertiaMatrix && !ConvertedToRigidBody())
    {
        mTimeSinceComputeInertiaMatrix = 0;
    
        // compute the new center of mass
        // ignore the joint visibility for now.
        Vector v;
        mCMPosition.Clear();
        int i;
        for (i=0; i<mNumSimJoints; i++)
        {
            if (mSimJoints[i]->mLocalMass > 0 && mSimJoints[i]->IsVisible())
            {
                mSimJoints[i]->CMPosition(v);
                mCMPosition.ScaleAdd(mSimJoints[i]->mLocalMass, v);
            }
        }
        mCMPosition.Scale(mInvMass);
    
        // adapt the cm in term of object's relative coord.
        Matrix rotMatT;
        Vector tmp1;
        tmp1.Sub(mCMPosition, mSimState->GetPosition()); // tmp1 is mRotMat * mCMOffset0
        rotMatT = mRotMat;
        rotMatT.Transpose();   
        rotMatT.RotateVector(tmp1, &mCMOffset0); // untransform tmp1 to get fRcm0
        mCMOffsetEmpty = false;
    
        // we compute the inertia matrix of the rotated object (fI) instead of fI0.
        // So we'll need to untransform fI to get fI0.
        SymMatrix tmp, jtmp;
        mInertiaMatrix.Zero();
    
        for (i=0; i<mNumSimJoints; i++)
        {
            if (mSimJoints[i]->mLocalMass > 0 && mSimJoints[i]->IsVisible())
            {
                v.Add(mSimJoints[i]->mDCMPosition, mSimJoints[i]->Origin());
                v.Sub(mCMPosition);
                ComputePointMassInertia(tmp, v, mSimJoints[i]->mLocalMass);
                jtmp = mSimJoints[i]->mLocalInertiaMatrix0;
                jtmp.TransformSelf(JointTransform(i));
            
                tmp.Add(jtmp);
                mInertiaMatrix.Add(tmp);
            }
        }

        // RotateVector(transposed(mRotMat) == UnTransform(mRotMat)
        mInertiaMatrix0 = mInertiaMatrix;
        rotMatT = mRotMat;
        rotMatT.Transpose();   
        mInertiaMatrix0.TransformSelf(rotMatT);
        mInvInertiaMatrix.Invert(mInertiaMatrix); // don't need to call BodyI();

        // add external contribution to the cm
        mRotMat.RotateVector(mExternalCMOffset0, &v);
        mCMPosition.Add(v);
    
        // updating mInertiaMatrix will cause a very little jump in the angular momemtum. 
        // this is not noticeable on screen but the data is perturbed.
        // But on the other hand, updating mInertiaMatrix every step seems useless...
    }
}


//

void ArticulatedPhysicsObject::StartCollision(const rmt::Vector& inPos, const rmt::Vector& inDir, const SimulatedObject* otherObj, int index, int otherindex)
{
    if (mMass == 0)
        return;

    mCacheEmpty = true;
    mAllCacheEmpty = false;
    
    rAssert(index == -1 || mJoints[index] != NULL);
    
    if (index >= 0 && mJoints[index]->mSimBranch)
    {
        rAssert(!ConvertedToRigidBody());
        StartHandlingCollision(otherObj, index, otherindex);
        mJoints[index]->ComputeAbsorptionFactor(inPos, inDir);
        EndHandlingCollision();
    }
    else
    {
        PhysicsObject::StartCollision(inPos, inDir, otherObj);
    }
}

void ArticulatedPhysicsObject::GetMassMatrix(const rmt::Vector& inPos, SymMatrix& oSymMatrix, SimulatedObject* otherObj, int index, int otherindex)
{
    if (mMass == 0)
    {
        oSymMatrix.Zero();
        return;
    }
    PhysicsObject::GetMassMatrix(inPos, oSymMatrix, otherObj);
    
    if (index >= 0 && mJoints[index]->mSimBranch)
    {
        oSymMatrix.Invert(oSymMatrix);
        StartHandlingCollision(otherObj, index, otherindex);
        mJoints[index]->GetMassMatrix(inPos, oSymMatrix);
        oSymMatrix.Invert(oSymMatrix);
        EndHandlingCollision();
    }
}

bool ArticulatedPhysicsObject::SpeedAdjustmentFromCache(const rmt::Vector& pos, rmt::Vector& speed, int inIndex)
{
    bool ret = false;

    PhysicsJoint* jnt = mJoints[inIndex];
    while(jnt && jnt->mIndex > 0)
    {
        if (jnt->mOverFlowCache)
        {
            Vector w, dv, dr;
            dr.Sub(pos, jnt->Origin());
            jnt->GetCache(w);
            dv.CrossProduct(w, dr);
            speed.Add(dv);
            ret = true;
        }
        jnt = mJoints[jnt->mParentIndex];
    }
    return ret;
}

bool ArticulatedPhysicsObject::AddImpulse(const rmt::Vector& inPos, const rmt::Vector& inImp, const SymMatrix& inSymMatrix, SimulatedObject* otherObj, int index, int otherindex)
{
    bool ret = false;

    if (mMass > 0)
    {
        if (index >= 0 && mJoints[index]->mSimBranch)
        {
            Vector localImp = inImp;
            StartHandlingCollision(otherObj, index, otherindex);
            ret = mJoints[index]->AddImpulse(inPos, localImp, inSymMatrix);
            EndHandlingCollision();
            if (ret && localImp.DotProduct(localImp) > RELA_SMALL)
                ret = PhysicsObject::AddImpulse(inPos, localImp, inSymMatrix, otherObj);
        }
        else
        {
            ret = PhysicsObject::AddImpulse(inPos, inImp, inSymMatrix, otherObj);
        }
    }
    
    return ret;
}

void ArticulatedPhysicsObject::EndCollision(bool fdyn, int index, int otherindex)
{
    if (mAllCacheEmpty) // for self-collision, the first endcollision adds all the cache
        return;
    
    if (mSimState->GetControl() != simSimulationCtrl)
        ResetCache();
    
    if (!ConvertedToRigidBody() && index >= 0)// && mJoints[index]->mSimBranch)
    {
        if (!ABSORB_CACHE_FEEDBACK || !mImpulseReactionEnabled)
            fdyn = false;
        
        SimulatedObject* otherObj = (otherindex >= 0 ? this : NULL);
        StartHandlingCollision(otherObj, index, otherindex);
        if (mSelfCollisionCenter)
        {
            mSelfCollisionCenter->StartIncludeCache();
        }
        else
        {
            Vector cacheW(0, 0, 0);
            if (!mCacheEmpty)
            {
                mInvInertiaMatrix.TransformVector(mCacheL, cacheW);
            }

            if (!fdyn)
            {
                mSimJoints[0]->IncludeCache(mCacheV, cacheW);
            }
            else
            {
                mSimJoints[0]->IncludeCacheReaction(mCacheV, cacheW);
            }
        }
        EndHandlingCollision();
    }
    PhysicsObject::EndCollision(fdyn);
    
    mAllCacheEmpty = true;
}

// access to collision cache data
bool ArticulatedPhysicsObject::GetCollisionCache(Vector& o_cacheV, Vector& o_cacheW, int inIndex) const
{
    if (inIndex >= 0 )
    {
        rAssert(mJoints[inIndex]);
        if (mJoints[inIndex]->GetNumDOF()>0)
        {
            mJoints[inIndex]->GetCache(o_cacheW);
            o_cacheV.CrossProduct( o_cacheW, mJoints[inIndex]->mDCMPosition );
        }
        else
        {
            o_cacheV.Clear();
            o_cacheW.Clear();
        }
        return true;
    }
    else
    {
        return PhysicsObject::GetCollisionCache( o_cacheV, o_cacheW );
    }
}

bool ArticulatedPhysicsObject::IsCacheEmpty(int index) const
{
    rAssertMsg(0,"Not implemented");
    return false;
}

void ArticulatedPhysicsObject::ResetCache(int index)
{
    if (index == -1)
    {
        PhysicsObject::ResetCache();
    }
    else if (mJoints[index])
    {
        mJoints[index]->ResetCache();
    }
}

void ArticulatedPhysicsObject::StartHandlingCollision(const SimulatedObject* otherObj, int index, int otherindex)
{
    if (index >= 0)
    {
        mCollidingJoint = mJoints[index];
    
        if (otherObj == this)
        {
            rAssert(mCollidingJoint);
            mSelfCollision = true;
            PhysicsJoint* otherJoint = mJoints[otherindex];
            if (otherJoint)
            {
                if (mCollidingJoint->mIndex < otherJoint->mIndex)
                {
                    mSelfCollisionCenter = mCollidingJoint;
                }
                else
                {
                    mSelfCollisionCenter = otherJoint;
                    otherJoint = mCollidingJoint;
                }
                while (mSelfCollisionCenter && !mSelfCollisionCenter->IsParentOf(otherJoint))
                {
                    mSelfCollisionCenter = mSelfCollisionCenter->mParentSimJoint;
                }
            }
        }
    }
}

//
// the following method are to be used with the reaching system
//
/*
void ArticulatedPhysicsObject::TryIntercept(int inJoint, float inIntensity, const rmt::Vector& targetPos, const rmt::Vector& targetSpeed)
{
}

void ArticulatedPhysicsObject::ApplyImpulseForGoal(int inJoint, float inIntensity, const rmt::Vector& goal)
{
}
*/

void ArticulatedPhysicsObject::ApplyImpulse(int inJointIndex, float inIntensity, const rmt::Vector& inDir)
{
    rAssert(!ConvertedToRigidBody());
    rAssert(mJoints[inJointIndex] != NULL && inIntensity > 0);
    Vector cm, impulse;
    mJoints[inJointIndex]->CMPosition(cm);
    impulse.Scale(inIntensity, inDir);
    
    SymMatrix m;
    StartCollision(cm, inDir, NULL, inJointIndex);
    GetMassMatrix(cm, m, NULL, inJointIndex); // not sure about that, the mass should include fJoint...
    while(!AddImpulse(cm, impulse, m, NULL, inJointIndex))
    {
        GetMassMatrix(cm, m, NULL, inJointIndex);
    }
    EndCollision(false, inJointIndex);
}


} // sim
