
#include "simflexible/flexjointmodifier.hpp"
#include "p3d/anim/skeleton.hpp"
#include "p3d/utility.hpp" // for p3d::MallocTemp
#include "simcommon/dline2.hpp"
#include "simcommon/simstatearticulated.hpp"
#include "simcommon/simutility.hpp"
#include "poser/pose.hpp"

#include <p3d/chunkfile.hpp>
#include <constants/chunkids.hpp>

using namespace RadicalMathLibrary;
static float myangle = .001f;
namespace sim
{
    
float FlexJointModifier::sFilterRootSpeedValue = 0.00f;
bool  FlexJointModifier::sUseRootFilteredSpeed = true;

void OptimiseDamping( ConditionParams &io_params, float mass )
{
    //This is temporary code. Explicit dependency on units have to be found.
    //This is probably not the best place also. ConditionParams should probably
    //derives from SimUnits?
    
    SimLength l(1.0f);
    float f = l.Value();
    //io_params.Stretch1Dkf   = f*io_params.Stretch1Dkf;
    //io_params.Bend1Dkf      = f*io_params.Bend1Dkf;
    //io_params.Stretch2Dkf   = f*io_params.Stretch2Dkf;
    //io_params.Shear2Dkf     = f*io_params.Shear2Dkf;
    //io_params.Bend2Dkf      = f*io_params.Bend2Dkf;
#if 0
    // Hi Martin, this is the code I suggest to let people tuning damping parameters from
    // the optimised values. But, for now, i don't want to scrap the current behvior.
    io_params.Stretch1Dkd   = 2.0f * io_params.Stretch1Dkd * Sqrt(mass*io_params.Stretch1Dkf);
    io_params.Bend1Dkd      = 2.0f * io_params.Bend1Dkd * Sqrt(mass*io_params.Bend1Dkf);
    io_params.Stretch2Dkd   = 2.0f * io_params.Stretch2Dkd * Sqrt(mass*io_params.Stretch2Dkf);
    io_params.Shear2Dkd     = 2.0f * io_params.Shear2Dkd * Sqrt(mass*io_params.Shear2Dkf);
    io_params.Bend2Dkd      = 2.0f * io_params.Bend2Dkd * Sqrt(mass*io_params.Bend2Dkf);
#else
    io_params.Stretch1Dkd   = 2.0f * Sqrt(mass*io_params.Stretch1Dkf);
    io_params.Bend1Dkd      = 2.0f * Sqrt(mass*io_params.Bend1Dkf);
    io_params.Stretch2Dkd   = 2.0f * Sqrt(mass*io_params.Stretch2Dkf);
    io_params.Shear2Dkd     = 2.0f * Sqrt(mass*io_params.Shear2Dkf);
    io_params.Bend2Dkd      = 2.0f * Sqrt(mass*io_params.Bend2Dkf);
#endif
}

//
//
//

FlexJointModifier::FlexJointModifier(poser::Pose* inPose, FlexibleJointDefinition* inDef, int item)
    : mParticleSystem(NULL),
    mPose(inPose),
    mAdvanceDt(0),
    mNumFlexJointParticle(0),
    mNumFlexJointRestParticle(0),
    mNumFixParticle(0),
    mFlexJointParticle(NULL),
    mFlexJointRestParticle(NULL),
    mPoseToParticleIndex(NULL),
    mRootIndex(-1),
    mFirstPoseIndex(-1),
    mLastPoseIndex(-1),
    mAveLen(0),
    mSystemScale(1.0f),
    mRestPositionMethod(RELATIVE4),
    mUpdateJointsMethod(SIMPLEDEF1),
    mSimState(NULL),
    mBlendingFactor(1.0f),
    mMaxdef(0.1f),
    mSpeedDampingFactor(0.0f),
    mNumSubIteration(1)
{
    
    rAssert(mPose);
    mPose->AddRef();
    mDefinition = inDef;
    mDefinition->AddRef();
    mDefinitionItem = &mDefinition->mDefinitions[item];
    
    int i;
    
    for (i = 0; i<mDefinition->mParameters.GetSize(); i++)
    {
        if (mDefinition->mParameters[i].mUID == mDefinitionItem->mParametersName)
            mParameters = mDefinition->mParameters[i];
        if (mDefinition->mParameters[i].mUID == mDefinitionItem->mRestingPosParametersName)
            mRestingPosParameters = mDefinition->mParameters[i];
        if (mDefinition->mParameters[i].mUID == mDefinitionItem->mConnectionParametersName)
            mConnectionParameters = mDefinition->mParameters[i];
    }
    
    mUseRestingPos = mDefinitionItem->mUseRestingPos;
    mUseGravity = mDefinitionItem->mGravity;
    mUseVirtualJoint = mDefinitionItem->mUseVirtualJoint;
    mSolver = mDefinitionItem->mSolver;
    mExternalDensityFactor = mDefinitionItem->mExternalDensityFactor;
    SetSimMethod(SimMethodEnum(mDefinitionItem->mSimMethod));
    SetRestMethod(RestPositionMethodEnum(mDefinitionItem->mRestMethod));
    SetUpdateJointsMethod(UpdateJointsMethodEnum(mDefinitionItem->mUpdateMethod));
    
    mNumFlexJointParticle = mDefinitionItem->mFlexibleJointIndex.GetSize();
    mFirstPoseIndex = mDefinitionItem->mFlexibleJointIndex[0];
    mLastPoseIndex = mDefinitionItem->mFlexibleJointIndex[mNumFlexJointParticle-1];
    
    // if use virtual, a virtual particle will be added at the end of each branch
    if (mUseVirtualJoint && mDefinitionItem->mEndOfBranchIndex.GetSize() > 0)
        mNumFlexJointParticle += mDefinitionItem->mEndOfBranchIndex.GetSize();
    
    // numParticles will be the number of particles in the particle system
    int numParticles = mNumFlexJointParticle;
    
    mFlexJointParticle = new FlexJointParticle[mNumFlexJointParticle];
    mPoseToParticleIndex = new short[mPose->GetJointCount()];
    for (i = 0; i<mPose->GetJointCount(); i++)
        mPoseToParticleIndex[i] = -1;
    
    // if use resting pose, we need to add particles in the particle system
    // for every non fixed mFlexJointParticle
    if (mUseRestingPos)
    {
        mNumFlexJointRestParticle = mNumFlexJointParticle - mDefinitionItem->mFlexibleFixJointIndex.GetSize();
        mFlexJointRestParticle = new FlexJointRestParticle[mNumFlexJointRestParticle];
        
        numParticles += mNumFlexJointRestParticle;
    }
    
    int currentRestParticle = 0;
    mRootIndex = -1; // mRootIndex should be the first particle but this is not enforced for the moment
    
    for (i = 0; i<mDefinitionItem->mFlexibleJointIndex.GetSize(); i++)
    {
        short currentPoseIndex = mDefinitionItem->mFlexibleJointIndex[i];
        mFlexJointParticle[i].mPoseIndex = currentPoseIndex;
        int aParent;
        aParent = mPose->GetSkeleton()->GetJoint(currentPoseIndex)->parentIndex;
        if(mPoseToParticleIndex[aParent] == -1)  
            mFlexJointParticle[i].mFlexParentIndex = i;
        else
            mFlexJointParticle[i].mFlexParentIndex = mPoseToParticleIndex[aParent];
        mFlexJointParticle[i].mParticleIndex = i;
        mFlexJointParticle[i].mFix = mDefinitionItem->mFlexibleFixJointIndex.Contains(currentPoseIndex);
        mFlexJointParticle[i].mVirtual = false;
        mFlexJointParticle[i].mNonVirtualEndOfBranch = false;
        
        mFlexJointParticle[i].mRestParticle = NULL;
        mPoseToParticleIndex[currentPoseIndex] = i;
        
        if (mUseRestingPos && !mFlexJointParticle[i].mFix)
        {
            rAssert(currentRestParticle < mNumFlexJointRestParticle);
            mFlexJointParticle[i].mRestParticle = &mFlexJointRestParticle[currentRestParticle++];
            mFlexJointParticle[i].mRestParticle->mPoseIndex = mFlexJointParticle[i].mPoseIndex;
            mFlexJointParticle[i].mRestParticle->mParticleIndex = mNumFlexJointParticle + currentRestParticle - 1;
            mFlexJointParticle[i].mRestParticle->mVirtual = false;
        }
        if (mRootIndex == -1 && mFlexJointParticle[i].mFix)
        {
            mRootIndex = i;
        }
        // if we aren't using virtual particles we need to flag the ends of branches
        if (!mUseVirtualJoint && mDefinitionItem->mEndOfBranchIndex.GetSize() > 0)
        {
            for(int j=0; j<mDefinitionItem->mEndOfBranchIndex.GetSize(); j++)
            {
                // is the current joint the end of a branch?
                int k = mDefinitionItem->mEndOfBranchIndex[j];
                if( PoseToParticleIndex(k) == i )
                    mFlexJointParticle[i].mNonVirtualEndOfBranch = true;
            }
        }
    }
    
    
    // the virtual particles
    for (int currentBranch = 0; i<mNumFlexJointParticle; i++, currentBranch++)
    {
        mFlexJointParticle[i].mPoseIndex = mDefinitionItem->mEndOfBranchIndex[currentBranch];
        mFlexJointParticle[i].mFlexParentIndex = mPoseToParticleIndex[mFlexJointParticle[i].mPoseIndex] ; 
        mFlexJointParticle[i].mParticleIndex = i;
        mFlexJointParticle[i].mFix = false;
        mFlexJointParticle[i].mVirtual = true;
        mFlexJointParticle[i].mNonVirtualEndOfBranch = false;
        mFlexJointParticle[i].mRestParticle = NULL;
        //mPoseToParticleIndex[mFlexJointParticle[i].mPoseIndex] = i;
        if (mUseRestingPos)
        {
            rAssert(currentRestParticle < mNumFlexJointRestParticle);
            mFlexJointParticle[i].mRestParticle = &mFlexJointRestParticle[currentRestParticle++];
            mFlexJointParticle[i].mRestParticle->mPoseIndex = mFlexJointParticle[i].mPoseIndex;
            mFlexJointParticle[i].mRestParticle->mParticleIndex = mNumFlexJointParticle + currentRestParticle - 1;
            mFlexJointParticle[i].mRestParticle->mVirtual = true;
        }
    }
    
    // set the connections, those will be used for extracting the matrix from the particle position
    for (i = 0; i<mDefinitionItem->mFlexibleJointConnection.GetSize(); i++)
    {
        short i1 = mPoseToParticleIndex[mDefinitionItem->mFlexibleJointConnection[i].mIndex1];
        short i2 = mPoseToParticleIndex[mDefinitionItem->mFlexibleJointConnection[i].mIndex2];
        float length = mDefinitionItem->mFlexibleJointConnection[i].mLength;
        
        rAssert(i1 >= 0);
        rAssert(i2 >= 0);
        
        FlexJointModifier::Connection* c = &mFlexibleJointConnection.NewElem();
        c->mFlexJointParticleIndex1 = i1;
        c->mFlexJointParticleIndex2 = i2;
        c->mLength = length;
        mFlexJointParticle[i1].mFlexibleJointConnection.Add(c);
        mFlexJointParticle[i2].mFlexibleJointConnection.Add(c);
    }
    
    // create the position of the particle
    Vector* particlepos = (Vector*)p3d::MallocTemp(sizeof(Vector)*numParticles);
    
    for (i = 0; i<mNumFlexJointParticle; i++)
    {
        mFlexJointParticle[i].mFinalMat.Identity();
        mFlexJointParticle[i].mInitialMat.Identity();
        /// don't need to set non-rest particle positions here?
        GetPosePosition(i, particlepos[i]);
        if (mFlexJointParticle[i].mRestParticle) // rest particle initially at the same position
        {
            particlepos[mFlexJointParticle[i].mRestParticle->mParticleIndex] = particlepos[i];
        }
    }
    
    // the particle system
    mParticleSystem = new ParticleSystemManual(numParticles, particlepos);
    mParticleSystem->SetParticleSystemSolver(eParticleSystemSolverType(mSolver));
    if(!mUseGravity)
    {
        SimEnvironment* zeroGravSimEnv = new SimEnvironment;
        zeroGravSimEnv->SetGravityCGS(0.0f, 0.0f, 0.0f);
        mParticleSystem->SetSimEnvironment(zeroGravSimEnv);
    }
    else
    {
        mParticleSystem->SetSimEnvironment(SimEnvironment::GetDefaultSimEnvironment());
    }
    
    p3d::FreeTemp(particlepos);
    
    *mParticleSystem->mCparams = mParameters;
    
    float jointLength=0;
    mParticleSystem->mDeli = 0;
    for (i = 0; i<mDefinitionItem->mFlexibleJointConnection.GetSize(); i++)
    {
        short i1 = mPoseToParticleIndex[mDefinitionItem->mFlexibleJointConnection[i].mIndex1];
        short i2 = mPoseToParticleIndex[mDefinitionItem->mFlexibleJointConnection[i].mIndex2];
        float length = mDefinitionItem->mFlexibleJointConnection[i].mLength;
        mParticleSystem->mDeli += length;
        
        if (mDefinitionItem->mFlexibleJointConnection[i].mOnBranch)
        {
            jointLength += length;
        }        
        ConditionParams* c = (mDefinitionItem->mFlexibleJointConnection[i].mOnBranch ? &mParameters : &mConnectionParameters);
        ((ParticleSystemManual*)mParticleSystem)->AddLine(i1, i2, c);
    }
    mParticleSystem->mDeli /= mDefinitionItem->mFlexibleJointConnection.GetSize();
    mAveLen = mParticleSystem->mDeli;
    
    //OptimiseDamping( mParameters, mParticleSystem->mMass[0]);
    //OptimiseDamping( mConnectionParameters, mParticleSystem->mMass[0]);
    //OptimiseDamping( mRestingPosParameters, mParticleSystem->mMass[0]);
    
    for(i=0; i<mNumFlexJointParticle; i++)
    {
        if (mFlexJointParticle[i].mFix)
        {
            mNumFixParticle += 1;
            mParticleSystem->FixParticle(mFlexJointParticle[i].mParticleIndex);
        }
        if (mFlexJointParticle[i].mRestParticle)
        {
            mParticleSystem->FixParticle(mFlexJointParticle[i].mRestParticle->mParticleIndex);
            
            ((ParticleSystemManual*)mParticleSystem)->AddLine( 
                mFlexJointParticle[i].mParticleIndex, 
                mFlexJointParticle[i].mRestParticle->mParticleIndex, 
                &mRestingPosParameters);
        }
        if (mFlexJointParticle[i].mVirtual)
        {
            Connection* c = &mFlexibleJointConnection.NewElem();
            c->mFlexJointParticleIndex1 = mFlexJointParticle[i].mParticleIndex;
            c->mFlexJointParticleIndex2 = mPoseToParticleIndex[mFlexJointParticle[i].mPoseIndex];
            Vector tmpVec1, tmpVec2;
            mParticleSystem->GetPosParticle(c->mFlexJointParticleIndex1, tmpVec1);
            mParticleSystem->GetPosParticle(c->mFlexJointParticleIndex2, tmpVec2);
            tmpVec1.Sub(tmpVec2);
            c->mLength = tmpVec1.Magnitude();
            ((ParticleSystemManual*)mParticleSystem)->AddLine(
                c->mFlexJointParticleIndex1, 
                c->mFlexJointParticleIndex2, 
                &mParameters);
            jointLength += c->mLength;
        }
    }
    mTeleportVelocity       = new rmt::Vector[mNumFixParticle];
    mPreviousFilteredSpeed  = new rmt::Vector[mNumFixParticle];
    mRootFilteredSpeed      = new rmt::Vector[mNumFixParticle];
    mPreviousRootSpeed      = new rmt::Vector[mNumFixParticle];
    mNewRootSpeed           = new rmt::Vector[mNumFixParticle];
    
    for (i = 0; i<mNumFixParticle; i++)
    {
        mTeleportVelocity[i].Clear();
        mPreviousFilteredSpeed[i].Clear();
        mRootFilteredSpeed[i].Clear();
        mPreviousRootSpeed[i].Clear();
        mNewRootSpeed[i].Clear();
    }
    for (i = 0; i<mDefinitionItem->mFlexibleJointHinge.GetSize(); i++)
    {
        short i1 = mPoseToParticleIndex[mDefinitionItem->mFlexibleJointHinge[i].mIndex1];
        short i2 = mPoseToParticleIndex[mDefinitionItem->mFlexibleJointHinge[i].mIndex2];
        short i3 = mPoseToParticleIndex[mDefinitionItem->mFlexibleJointHinge[i].mIndex3];
        ((ParticleSystemManual*)mParticleSystem)->AddBending1D(i1, i2, i3,false, &mParameters);
    }
    
    if (!mParameters.GetParamSet())
    {
        i = mNumFlexJointParticle - mDefinitionItem->mFlexibleFixJointIndex.GetSize();
        jointLength /= Max(1,i);
        i = mNumFlexJointParticle / Max(1,mDefinitionItem->mFlexibleFixJointIndex.GetSize());
        ((ParticleSystemManual*)mParticleSystem)->SetLengthEstimate( jointLength, i, &mParameters );
        ((ParticleSystemManual*)mParticleSystem)->SetLengthEstimate( jointLength, 2, &mConnectionParameters );
        ((ParticleSystemManual*)mParticleSystem)->SetLengthEstimate( 0,           2, &mRestingPosParameters );
        ((ParticleSystemManual*)mParticleSystem)->SetMasterSubPart(&mParameters);
    
        ((ParticleSystemManual*)mParticleSystem)->UpdateCparamsForLambda();
        ((ParticleSystemManual*)mParticleSystem)->UpdateCparamsForKappa ();
        ((ParticleSystemManual*)mParticleSystem)->UpdateCparamsForIota  ();
    }
}

FlexJointModifier::~FlexJointModifier()
{
    mDefinition->Release();
    mPose->Release();
    delete[] mFlexJointParticle;
    delete[] mFlexJointRestParticle;
    delete[] mPoseToParticleIndex;
    delete mParticleSystem;
}

Vector& FlexJointModifier::GetPosePosition(int flexParticleIndex, rmt::Vector& pos)
{
    poser::Joint* joint = mPose->GetJoint(mFlexJointParticle[flexParticleIndex].mPoseIndex);
    pos = joint->GetWorldMatrix().Row(3);
    
    if (mFlexJointParticle[flexParticleIndex].mVirtual)
    {
        // pos_virtual = pos_index + (pos_index - pos_index_parent) = 2*pos_index - pos_index_parent
        Vector v = joint->GetParentJoint()->GetWorldMatrix().Row(3);
        pos.Scale(2.0f);
        pos.Sub(v);
    }
    return pos;
}

Matrix& FlexJointModifier::GetPoseTransform(int flexParticleIndex, rmt::Matrix& m)
{
    poser::Joint* joint = mPose->GetJoint(mFlexJointParticle[flexParticleIndex].mPoseIndex);
    m = joint->GetWorldMatrix();
    
    if (mFlexJointParticle[flexParticleIndex].mVirtual)
    {
        // pos_virtual = pos_index + (pos_index - pos_index_parent) = 2*pos_index - pos_index_parent
        Vector v = joint->GetParentJoint()->GetWorldMatrix().Row(3);
        m.Row(3).Scale(2.0f);
        m.Row(3).Sub(v);
    }
    return m;
}

void FlexJointModifier::SetPoseTransform(int flexParticleIndex, const rmt::Matrix& inTransform)
{
    rAssert(!mFlexJointParticle[flexParticleIndex].mVirtual);
    poser::Joint* j = mPose->GetJoint(mFlexJointParticle[flexParticleIndex].mPoseIndex);
    j->SetWorldMatrix(inTransform);
}

void FlexJointModifier::SetPoseTranslation(int flexParticleIndex, const rmt::Vector& inTranslation)
{
    rAssert(!mFlexJointParticle[flexParticleIndex].mVirtual);
    poser::Joint* j = mPose->GetJoint(mFlexJointParticle[flexParticleIndex].mPoseIndex);
    j->SetWorldTranslation(inTranslation);
}

Vector& FlexJointModifier::GetBonePose(int flexIndex, rmt::Vector& v)
{
    Vector v1, v2;
    int parent;
    parent = mFlexJointParticle[flexIndex].mFlexParentIndex;
    GetPosePosition(mFlexJointParticle[flexIndex].mParticleIndex, v1);
    GetPosePosition(mFlexJointParticle[parent].mParticleIndex, v2);
    v.Sub(v1, v2);
    return v;
}

void FlexJointModifier::SetSimStateArticulated(SimStateArticulated* inSimState)
{
    mSimState = inSimState;
}

/*
void FlexJointModifier::SetScale(float f)
{
rAssert(f > 0);

  float ratio = f/mSystemScale;
  mSystemScale = f;
  
    mParticleSystem->Scale(ratio);
    
      for(int i=0; i<mNbJoints-1; i++)
      {
      mLen[i] *= ratio;
      }
      mAveLen *= ratio;
      }
*/

void FlexJointModifier::GetRelAxisAngle(const int i, Vector& axis, float& angle)
{
    // Finds the world space axis and angle required to rotate pose position i 
    // into the particle position.
    int parent;
    parent = mFlexJointParticle[i].mFlexParentIndex;
    rAssert(parent != i);
    
    Vector particlepos, previouspos, deltaparticle, deltaprevious;
    mParticleSystem->GetPosParticle(mFlexJointParticle[parent].mParticleIndex, particlepos);
    mParticleSystem->GetPosParticle(mFlexJointParticle[i].mParticleIndex, deltaparticle);
    deltaparticle.Sub(particlepos);
    
    previouspos = mFlexJointParticle[parent].mInitialMat.Row(3);   
    deltaprevious = mFlexJointParticle[i].mInitialMat.Row(3);
    deltaprevious.Sub(previouspos);
    AxisAngle(deltaprevious, deltaparticle, axis, angle);
}

void FlexJointModifier::GetRelRotation(const int i, Matrix& m)
{               
    // Finds the rotation matrix m that takes the local pose position i and rotates
    // it into the local particle position.  This is in the parent frame.
    
    int parent;
    parent = mFlexJointParticle[i].mFlexParentIndex;
    rAssert(parent != i);
    
    
    Vector particlepos, previouspos, deltaparticle, deltaprevious, axis;
    float angle;
    mParticleSystem->GetPosParticle(mFlexJointParticle[parent].mParticleIndex, particlepos);
    mParticleSystem->GetPosParticle(mFlexJointParticle[i].mParticleIndex, deltaparticle);
    deltaparticle.Sub(particlepos);
    
    previouspos = mFlexJointParticle[parent].mInitialMat.Row(3);   
    deltaprevious = mFlexJointParticle[i].mInitialMat.Row(3);
    deltaprevious.Sub(previouspos);
    AxisAngle(deltaprevious, deltaparticle, axis, angle);
    
    Matrix old;
    GetPoseTransform(parent, old);
    // put rotation axis into joint's reference frame since we are 
    // doing a vector-side multiplication  ( v . Mnew . Mold )
    old.Invert();
    axis.Rotate(old);
    
    m.Identity();
    if(!Epsilon(angle, 0.0f, 0.001f))
        m.FillRotation(axis, angle);
    
    //#define DEBUGDISPLAY
#ifdef DEBUGDISPLAY
    Matrix tmpm;
    GetPoseTransform(i, tmpm);
    
    dLineRel(particlepos, axis.Scale(5.0f), tColour(0, 200, 30));
    dLineRel(particlepos, deltaprevious, tColour(40, 40, 200));
    //dLineRel(particlepos, deltaparticle, tColour(244, 244, 244));
    deltaprevious = Vector(0.0f, 0.2f, 0.0f);
    dLineRel(particlepos, deltaprevious.Rotate(tmpm), tColour(0, 0, 0));
#endif
}

void FlexJointModifier::UpdateJoints()
{
    // compute the joint matrix so that the pose reflects the simulated particles position
    // The orientation of a joint cannot be computed without knowing the child's
    // position.  This forces the algorithm to proceed ass-backwards.
    
    // watch mBlendingFactor?!? which varies from 0 to 1, 0:pure animation
    
    for (int i = 0; i<mNumFlexJointParticle; i++)
    {
        int parentIndex = mFlexJointParticle[i].mFlexParentIndex;
        
        switch (mFlexJointParticle[i].mFlexibleJointConnection.GetSize())
        {
        case 0:
        default:
            {
                // Set the parent's rotation matrix
                if(parentIndex != i)
                {
                    Matrix mupdated, minput, mrel;
                    GetRelRotation(i, mrel);
                    GetPoseTransform(parentIndex, minput);
                    
                    // concatenate the object space mupdated with the parent world matrix minput
                    mupdated = mrel;
                    mupdated.Mult(minput);
                    CopyRotation(mupdated, mFlexJointParticle[parentIndex].mFinalMat);
                    
                    // if there is no virtual joint, we set the final joint's orientation
                    // to align to its parent's.  
                    if(mFlexJointParticle[i].mNonVirtualEndOfBranch)
                    {
                        // this time, apply the relative rotation to the current woorld matrix
                        GetPoseTransform(i, minput );
                        mupdated.Mult(mrel, minput );
                        // we need to copy mupdated to the pose but we can't overwrite joint i
                        // because we need it to calculate the child's relative rotation.
                        CopyRotation(mupdated, mFlexJointParticle[i].mFinalMat);
                    }
                } 
                // set the current joint's position (must come after rotation calculation above)             
                Vector particlePos;
                mParticleSystem->GetPosParticle(mFlexJointParticle[i].mParticleIndex, particlePos);
                mFlexJointParticle[i].mFinalMat.Row(3) = particlePos;
                break;
            }
            //case 1:
            //   break;
            //case 2:
            //   break;
            //default: // n particles
            //   break;
        }
    }
    
    if( sUseRootFilteredSpeed )
    {
        int fixcount = -1;
        rmt::Vector freeDeltaSpeed;
        
        for (int j = 0; j<mNumFlexJointParticle; j++)
        {
            if(mFlexJointParticle[j].mFix)
            {
                fixcount += 1;
                //freeDeltaSpeed.Sub(mNewRootSpeed[fixcount], mPreviousFilteredSpeed[fixcount]);
                freeDeltaSpeed.Sub(mNewRootSpeed[fixcount], mRootFilteredSpeed[fixcount]);
                mPreviousFilteredSpeed[fixcount] = mRootFilteredSpeed[fixcount];
                mPreviousRootSpeed[fixcount] = mNewRootSpeed[fixcount];
            }
            else
            {
                mParticleSystem->AddSpeedParticle(j, freeDeltaSpeed);
            }
            
            if (!mFlexJointParticle[j].mVirtual)
            {
                SetPoseTransform(j, mFlexJointParticle[j].mFinalMat);
            }
            if (mFlexJointParticle[j].mRestParticle && mSpeedDampingFactor > 0.0f)
            {
                rmt::Vector v;
                mParticleSystem->GetVelParticle(
                    mFlexJointParticle[j].mParticleIndex, v.x, v.y, v.z);

				if(Fabs(v.x) < mSpeedDampingFactor)
					v.x = 0.0f;
				if(Fabs(v.y) < mSpeedDampingFactor)
					v.y = 0.0f;
				if(Fabs(v.z) < mSpeedDampingFactor)
					v.z = 0.0f;

                mParticleSystem->SetSpeedParticle(
                    mFlexJointParticle[j].mParticleIndex, v.x, v.y, v.z);
            }
        }
    }
}

void FlexJointModifier::CacheCurrentPose()
{
    // mInitialMat stores the particle system state at the beginning of the frame
    // It is needed for calculating the relative rotation since it proceeds ass-backwards
    for (int i=0; i<mNumFlexJointParticle; i++)
    {
        GetPoseTransform(mFlexJointParticle[i].mParticleIndex, 
            mFlexJointParticle[i].mInitialMat);
    }
}

bool FlexJointModifier::UpdateRestingPosition()
{
    float idt = rmt::Inverse(mAdvanceDt);
    
    if (sUseRootFilteredSpeed && mRootIndex >= 0)
    {
        // in order to prevent artifacts when the pose movement is chunky or pops, we 
        // filter the speed of the root particle (the first fix particle of the system)
        // and translate the whole system so that it behaves as it it has moved at the
        // filtered speed but from the actual position indicated by the current pose.
        
        Vector rootpos, previousrootpos, shiftedrootpos, teleportDisplacement(0.0f, 0.0f, 0.0f), freeDeltaSpeed;
        int fixcount = -1;
        for(int k=0; k<mNumFlexJointParticle; k++)
        {
            if(mFlexJointParticle[k].mFix )
            {
                fixcount += 1;
                
                // Calculate the linear rootspeed
                GetPosePosition(k, rootpos);        
                mParticleSystem->GetPosParticle(k, previousrootpos);        
                mNewRootSpeed[fixcount].Sub(rootpos, previousrootpos);
                
                // If particle has been violently relocated, start from scratch
                if ( mNewRootSpeed[fixcount].x > 10.0f ||  mNewRootSpeed[fixcount].y > 10.0f || mNewRootSpeed[fixcount].z > 10.0f )
                {
                    mRootFilteredSpeed[fixcount].ScaleAdd(sFilterRootSpeedValue, mNewRootSpeed[fixcount]);
                    ResetParticleSystem(&mNewRootSpeed[fixcount]);
                    mTeleportVelocity[fixcount].Clear();
                    mPreviousFilteredSpeed[fixcount].Clear();
                    mRootFilteredSpeed[fixcount].Clear();
                    mPreviousRootSpeed[fixcount].Clear();
                    mNewRootSpeed[fixcount].Clear();
                    
                    return true;
                }
                else
                {
                    // Calculate the new filtered root speed.
                    mNewRootSpeed[fixcount].Scale(idt);
                    
                    // This is the change in speed.         
                    mRootFilteredSpeed[fixcount].Sub(mNewRootSpeed[fixcount], mPreviousFilteredSpeed[fixcount]);    
                    
                    // new speed = old speed + kappa * delta speed
                    mRootFilteredSpeed[fixcount].ScaleAdd(mPreviousFilteredSpeed[fixcount], sFilterRootSpeedValue, mRootFilteredSpeed[fixcount]);
                    
                    
                    // Now we magically teleport the whole system to the point where the root particle 
                    // can complete its journey to rootpos at the new filtered rootspeed.
                    //    <-     teleportDisplacement       -><-  RootFilteredSpeed * t  ->
                    //    |----------------------------------|----------------------------|
                    // previous                            shifted                     rootpos
                    shiftedrootpos.ScaleAdd(rootpos, -mAdvanceDt, mRootFilteredSpeed[fixcount]);
                    teleportDisplacement.Sub(shiftedrootpos, previousrootpos); 
                    
                    // set position
                    SetVector(mParticleSystem->mP[k]->m_x, rootpos.x, 
                        rootpos.y, 
                        rootpos.z);
                    // set velocity
                    SetVector(mParticleSystem->mP[k]->m_v, 
                        mRootFilteredSpeed[fixcount].x, 
                        mRootFilteredSpeed[fixcount].y, 
                        mRootFilteredSpeed[fixcount].z);
                    
                    freeDeltaSpeed.Sub(mPreviousFilteredSpeed[fixcount], mPreviousRootSpeed[fixcount]);
                }
            } // end fixed particle code
            
            else // free particle code
            {
                // The particle system accumulates velocities for the free particles.  If we don't
                // subtract the previous teleport velocity, then a system moving at constant speed
                // and filtering half the speed each frame would cause the free particles to increase
                // in speed over time.
                
                // Subtract the previous teleport velocity
                mParticleSystem->AddSpeedParticle(k, freeDeltaSpeed );
                //mParticleSystem->AddSpeedParticle(k, mTeleportVelocity[fixcount]);
                // Translate particles by teleportDisplacement
                mParticleSystem->MoveParticle(k, teleportDisplacement.x, teleportDisplacement.y, teleportDisplacement.z);
            }
            
            if( mFlexJointParticle[k].mRestParticle)
            {
                // Shift rest particles into position ready for dynamic move to current pose
                mParticleSystem->MoveParticle(mFlexJointParticle[k].mRestParticle->mParticleIndex,
                    teleportDisplacement.x,
                    teleportDisplacement.y,
                    teleportDisplacement.z);
            }
            
        }   // end for loop     
    }   // end filtering
    
    CacheCurrentPose();
    
    if(mUseRestingPos)
    {
        
        switch (mRestPositionMethod)
        {
        case RELATIVE1:
            {
                for (int i = 0; i<mNumFlexJointParticle; i++)
                {
                    if (mFlexJointParticle[i].mRestParticle)
                    {
                        Matrix mat;
                        mat.Identity();
                        Vector particlepos, bone, axis;
                        float angle;
                        int parentIndex, grandparentIndex;
                        parentIndex = mFlexJointParticle[i].mFlexParentIndex;
                        if(i != parentIndex )
                        {
                            grandparentIndex = mFlexJointParticle[parentIndex].mFlexParentIndex;
                            if(grandparentIndex != parentIndex)
                            {
                                GetRelAxisAngle(parentIndex, axis, angle); 
                                if(!Epsilon(angle, 0.0f, myangle))
                                    mat.FillRotation(axis, angle);
                                GetBonePose(i, bone);
                                bone.Rotate(mat);
                            }
                            else
                            {
                                GetBonePose(i, bone);
                            }
                            mParticleSystem->GetPosParticle(
                                mFlexJointParticle[parentIndex].mParticleIndex, 
                                particlepos);           
                            particlepos.Add(bone);
                        }
                        // Calculate the velocity to get from the last restParticle
                        // position to the current restParticle position in time idt
                        mParticleSystem->DynMoveParticle(
                            mFlexJointParticle[i].mRestParticle->mParticleIndex, 
                            particlepos, idt);
                    }
                }  // end for
            } // end switchCase
            break;
        case RELATIVE2:
            {
                
                
            }
        case RELATIVE3:
        case RELATIVE4:
        case RELATIVE5:
        case RELATIVE6:
        default: // default method is non relative
            {
                for (int i = 0; i<mNumFlexJointParticle; i++)
                {
                    if ( mFlexJointParticle[i].mRestParticle)
                    {
                        Vector pos;
                        GetPosePosition(i, pos);
                        //! SetPosParticle should be fine
                        mParticleSystem->DynMoveParticle(mFlexJointParticle[i].mRestParticle->mParticleIndex, pos, idt);
                        //mParticleSystem->SetPosParticle(mFlexJointParticle[i].mRestParticle->mParticleIndex, pos);
                    }
                }  // end for
            }
            break;
        }  // end switch
        
    }
    return true;
}

void FlexJointModifier::TestExtension1D()
{
    static int maxiter = 10;
    int n = 0;
    bool moved = true;
    
    while (moved && n < maxiter)
    {
        moved = false;
        for(int i=0; i<mFlexibleJointConnection.GetSize(); i++)
        {
            int p1 = mFlexibleJointConnection[i].mFlexJointParticleIndex1;
            int p2 = mFlexibleJointConnection[i].mFlexJointParticleIndex2;
            if (mParticleSystem->TestExtension1D(
                mParticleSystem->mP[p1], 
                mParticleSystem->mP[p2], 
                mFlexibleJointConnection[i].mLength, mMaxdef))
            {
                moved = true;
            }
        }
        n++;
    }
}

void FlexJointModifier::Update(poser::Pose* inPose)
{
    SimStats::StartFlexSimulation();

    if (mPose != inPose)
    {
        //reset system...
        mPose->Release();
        mPose = inPose;
        mPose->AddRef();
    }
    
    
    if (IsEnabled() && mBlendingFactor > 0)
    {
        if (mAdvanceDt > 0)
        {
            if (!UpdateRestingPosition())
                return; // problem
            
            mParticleSystem->SetDt(mAdvanceDt/float(mNumSubIteration));
            mParticleSystem->StartUpdate();
            mParticleSystem->ApplyConstraints();
            
            for (int i=0; i<mNumSubIteration; i++)
            {
                mParticleSystem->ApplyForces();
                mParticleSystem->AdvanceSystem();
                TestExtension1D();
            }

            mParticleSystem->EndUpdate();
        }
        UpdateJoints();
        
        //There is no SetTransform to do as in the case of rigid bodies. 
        //But we need to inform the collision objects that its collision volumes
        //have been moved.
        mSimState->SetHasMoved(mParticleSystem->GetHasMoved());
        
        mAdvanceDt = 0;
    }
    SimStats::EndFlexSimulation();
}

void FlexJointModifier::ResetParticleSystem(Vector* newSpeed)
{
    for (int i = 0; i<mNumFlexJointParticle; i++)
    {
        Vector particlepos;
        GetPosePosition(mFlexJointParticle[i].mParticleIndex, particlepos);
        mParticleSystem->SetPosParticle(mFlexJointParticle[i].mParticleIndex, particlepos); 
        if (mFlexJointParticle[i].mRestParticle) // rest particle initially at the same position
        {
            mParticleSystem->SetPosParticle(mFlexJointParticle[i].mRestParticle->mParticleIndex, particlepos); 
            SetVector(mParticleSystem->mP[i]->m_v, 0.0f, 0.0f, 0.0f);
        }
    }
    if (newSpeed)
        mParticleSystem->SetSystemSpeed(*newSpeed);  
}
/*
ComputeInternalSpeed is to be used to compute initial particle speed from 
two subsequent animation frame using the sequence of command similar to:
mFlexJointModifier->ResetParticleSystem();   // set the particle position == anim pose
mAnimController->Advance(dt);                // advance the anim pose
mFlexJointModifier->ComputeInternalSpeed(dt);// compute new speed between the two poses
This can be usefull when blending from animation to simulation
*/
void FlexJointModifier::ComputeInternalSpeed(float dt, poser::Pose* inPose)
{
    if (mPose != inPose)
    {
        //reset system...
        mPose->Release();
        mPose = inPose;
        mPose->AddRef();
    }
    
    if (dt == 0)
    {
        Vector speed(0, 0, 0);
        mParticleSystem->SetSystemSpeed(speed);
    }
    else
    {
        float idt = rmt::Inverse(dt);
        
        for (int i = 0; i<mNumFlexJointParticle; i++)
        {
            Vector speed, tmp;
            mParticleSystem->GetPosParticle(mFlexJointParticle[i].mParticleIndex, speed); // the old pos
            // sub the old pos from the new pos
            speed.Sub(GetPosePosition(mFlexJointParticle[i].mParticleIndex, tmp));
            speed.Scale(idt);
            mParticleSystem->SetSpeedParticle(i, speed.x, speed.y, speed.z);
            if (mFlexJointParticle[i].mRestParticle)
            {
                mParticleSystem->GetPosParticle(mFlexJointParticle[i].mRestParticle->mParticleIndex, speed); // the old pos
                speed.Sub(tmp);
                speed.Scale(idt);
                mParticleSystem->SetSpeedParticle(mFlexJointParticle[i].mRestParticle->mParticleIndex, speed.x, speed.y, speed.z);
            }
        }
    }
}

void FlexJointModifier::DebugDisplay()
{
    DrawLineToggler toggler;

    tColour red(200, 0, 0);
    tColour gray(100, 100, 100);
    tColour c3(0, 200, 0);
    Vector pos1;
    Vector pos2;
    int i;
    
    for(i=0; i<mFlexibleJointConnection.GetSize(); i++)
    {
        mParticleSystem->GetPosParticle(mFlexibleJointConnection[i].mFlexJointParticleIndex1, pos1); 
        mParticleSystem->GetPosParticle(mFlexibleJointConnection[i].mFlexJointParticleIndex2, pos2); 
        dStreamLine(pos1, pos2, red, red);
    }
    if (mUseRestingPos)
    {
        for(i=0; i<mNumFlexJointParticle; i++)
        {
            if (mFlexJointParticle[i].mRestParticle)
            {
                mParticleSystem->GetPosParticle(mFlexJointParticle[i].mParticleIndex, pos1); 
                mParticleSystem->GetPosParticle(mFlexJointParticle[i].mRestParticle->mParticleIndex, pos2); 
                dStreamLine(pos1, pos2, gray, gray);
            }
        }
    }

#ifdef RAD_DEBUG
    static bool drawForce = false;
    if (drawForce)
        mParticleSystem->DrawForce();
#endif
}

/*
two frames are necessary to start the simulation: a first frame to update the particle
position and a second frame to give them their initial speed. SetBlendingFactor 
has to be called on two successive frames in order to provide a proper start. 

  It is the responsability of the user to call ComputeInternalSpeed before the next
  frame
*/
void FlexJointModifier::SetBlendingFactor(float inBlendingFactor) 
{
    rAssert(inBlendingFactor >= 0 && inBlendingFactor <= 1.0f);
    
    if (mBlendingFactor == 0 && inBlendingFactor > 0)
    {
        ResetParticleSystem();
    }
    
    mBlendingFactor = inBlendingFactor;
    
    if (mBlendingFactor < 0.01f) // in order to avoid insignificant value in the blending
    {
        mBlendingFactor  = 0;
    }
}

//
//
//


static unsigned FLEXIBLEJOINT_VERSION = 1;

FlexibleJointDefinitionLoader::FlexibleJointDefinitionLoader() : tSimpleChunkHandler(Simulation::Flexible::JOINT) {};

tEntity* FlexibleJointDefinitionLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    char name[128];
    f->GetPString(name);
    
    unsigned version = f->GetLong();
    rAssert(version == FLEXIBLEJOINT_VERSION);
    
    FlexibleJointDefinition* newDefinition = new FlexibleJointDefinition;
    newDefinition->SetName(name);
    
    while(f->ChunksRemaining())
    {
        f->BeginChunk();
        switch(f->GetCurrentID())
        {
        case Simulation::Flexible::JOINT_PARAMETERS:
            {
                ConditionParams* param = &newDefinition->mParameters.NewElem();
                f->GetPString(name);
                param->mUID = tEntity::MakeUID(name);
                param->Stretch1Dkf = f->GetFloat();
                param->Stretch1Dkd = f->GetFloat();
                param->Bend1Dkf = f->GetFloat();
                param->Bend1Dkd = f->GetFloat();
                param->Wind1DKf = f->GetFloat();
                param->SetParamSet(true);
                break;
            }
        case Simulation::Flexible::JOINT_LAMBDA:
            {
                ConditionParams* param = &newDefinition->mParameters.NewElem();
                f->GetPString(name);
                param->mUID = tEntity::MakeUID(name);
                param->LambdaF = f->GetFloat();
                param->LambdaD = f->GetFloat();
                param->KappaF = f->GetFloat();
                param->KappaD = f->GetFloat();
                param->Wind1DKf = f->GetFloat();
                param->SetParamSet(false);
                param->SetUseLongSide(true);
                break;
            }
        case Simulation::Flexible::JOINT_DEFINITION:
            {
                FlexibleJointDefinition::FlexibleJointDefinitionItem* definitionItem = &newDefinition->mDefinitions.NewElem();
                f->GetPString(name);
                definitionItem->mName = tEntity::MakeUID(name);
                f->GetPString(name);
                definitionItem->mParametersName = tEntity::MakeUID(name);
                f->GetPString(name);
                definitionItem->mRestingPosParametersName = tEntity::MakeUID(name);
                f->GetPString(name);
                definitionItem->mConnectionParametersName = tEntity::MakeUID(name);
                definitionItem->mGravity = ShortToBool(f->GetUWord());
                definitionItem->mSimMethod = f->GetUWord();
                definitionItem->mUseRestingPos = ShortToBool(f->GetUWord());
                definitionItem->mRestMethod = f->GetUWord();
                definitionItem->mUpdateMethod = f->GetUWord();
                definitionItem->mUseVirtualJoint= ShortToBool(f->GetUWord());
                definitionItem->mSolver = f->GetUWord();
                definitionItem->mExternalDensityFactor = f->GetFloat();
                
                int numIndex = f->GetLong();
                int i;
                for (i=0; i<numIndex; i++)
                {
                    definitionItem->mFlexibleJointIndex.NewElem() = f->GetLong();
                }
                numIndex = f->GetLong();
                for (i=0; i<numIndex; i++)
                {
                    definitionItem->mFlexibleFixJointIndex.NewElem() = f->GetLong();
                }
                numIndex = f->GetLong();
                for (i=0; i<numIndex; i++)
                {
                    definitionItem->mEndOfBranchIndex.NewElem() = f->GetLong();
                }
                int numConnection = f->GetLong();
                for (i=0; i<numConnection; i++)
                {
                    FlexibleJointDefinition::FlexibleJointConnection* connection = &definitionItem->mFlexibleJointConnection.NewElem();
                    connection->mIndex1 = f->GetLong();
                    connection->mIndex2 = f->GetLong();
                    connection->mLength = f->GetFloat();
                    connection->mOnBranch = ShortToBool(f->GetUWord());
                }
                int numHinge = f->GetLong();
                for (i=0; i<numHinge; i++)
                {
                    FlexibleJointDefinition::FlexibleJointHinge* hinge = &definitionItem->mFlexibleJointHinge.NewElem();
                    hinge->mIndex1 = f->GetLong();
                    hinge->mIndex2 = f->GetLong();
                    hinge->mIndex3 = f->GetLong();
                }
                
                break;
            }
        }
        f->EndChunk();
    }
    
    return newDefinition;
}

} // sim
