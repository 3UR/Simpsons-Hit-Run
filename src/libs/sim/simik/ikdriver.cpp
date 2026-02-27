

#include "simik/ikdriver.hpp"
#include "simik/ikconsts.hpp"
#include "simik/srs.hpp"
#include "simcommon/simtarget.hpp"
#include "simcommon/simmath.hpp"
#include "simcommon/dline2.hpp"
#include "p3d/anim/skeleton.hpp" 
#include "poser/constants.hpp"
#include "poser/joint.hpp"
#include "poser/transform.hpp"
#include "poser/pose.hpp"



using namespace RadicalMathLibrary;



namespace sim
{

const Vector nX(-1.0f, 0.0f, 0.0f);
const Vector nY(0.0f, -1.0f, 0.0f);
const Vector nZ(0.0f, 0.0f, -1.0f);


IKDriver::IKDriver(  poser::Pose* inPose,
    int inStartJoint, 
    int inMidJoint, 
    int inEndJoint, 
    IKLimbType inIKLimbType,                                 
    SimTarget* inEffectorTarget,
    SimTarget* inConstraintTarget
    )
    : mStartIndex(inStartJoint),
    mMidIndex(inMidJoint), 
    mEndIndex(inEndJoint),
    mPose(inPose),
    mEffectorTarget(inEffectorTarget),
    mConstraintTarget(inConstraintTarget),
    mSwivelAngle(0),
    mIKLimbType(inIKLimbType)
{
    rAssert(mPose);
    mPose->AddRef();
    rAssert(mEffectorTarget);
    mEffectorTarget->AddRef();
    
    if (mConstraintTarget == NULL)
    {
        mModifierType = MParented;
    }
    else
    {
        mModifierType = MPoleAndGoalFromMaya;
        mConstraintTarget->AddRef();
    }
    
    Init();
}

IKDriver::~IKDriver()
{
    mPose->Release();
    mEffectorTarget->Release();
    
    if (mConstraintTarget)
        mConstraintTarget->Release();
    
    delete mIKSolver;
}

void IKDriver::Init()
{
    mGoalType = GPosition;
    
    
    // Note that the IK solver expects all vector and matrix data to be relative to the 
    // shoulder joint; more exactly, if uses the parent orientation frame shifted to the 
    // shoulder position.  Call this the shifted-parent frame.
    
    // the Character doesn't need to have a PhObj associated with it
    // but if there is none, we won't be able to have collision detection 
    // between the particle system and the rest of the Character.
    
    mIKSolver = new SRS();  
    
    mOptimize = false;  // if true, skips the calculation for identical input
    
    // local coordinates of characters can have arbitrary orientation
    // IKAN expects all bones to be x-aligned and the y-axis to point up.
    // we must calculate the twist matrices.
    
    tSkeleton::Joint* sk_j1     = mPose->GetSkeleton()->GetJoint(mStartIndex); // shoulder or upper leg
    tSkeleton::Joint* sk_jmid   = mPose->GetSkeleton()->GetJoint(mMidIndex);   // elbow or knee
    tSkeleton::Joint* sk_j2     = mPose->GetSkeleton()->GetJoint(mEndIndex);   // wrist or ankle
    tSkeleton::Joint* parentJnt = mPose->GetSkeleton()->GetJoint(sk_j1->parentIndex);
    
    mR1.Identity(); mRmid.Identity(); mR2.Identity();  
    mE.Identity();  mT.Identity();   mS.Identity();
    
    // IKAN does calculations in parent frame shifted to the startJoint position
    // use invShiftedParent to transform from character-root to shoulder coordinates
    SetShiftedParentSkel();  
    
    // calculate T (shoulder to elbow) and S (elbow to wrist) transforms
    // being careful about possible extra joints
    tSkeleton::Joint* sk_ptr = mPose->GetSkeleton()->GetJoint(sk_jmid->parentIndex);
    mT.Row(3) = sk_jmid->restPose.Row(3);  
    
    while(sk_ptr != sk_j1)
    {
        mT.Row(3).Transform(sk_ptr->restPose);
        sk_ptr = mPose->GetSkeleton()->GetJoint(sk_ptr->parentIndex);
    }   
    
    sk_ptr = mPose->GetSkeleton()->GetJoint(sk_j2->parentIndex);
    mS.Row(3) = sk_j2->restPose.Row(3);
    while(sk_ptr != sk_jmid)
    {
        mS.Row(3).Transform(sk_ptr->restPose);
        sk_ptr = mPose->GetSkeleton()->GetJoint(sk_ptr->parentIndex);
    }
    
    Vector v1, v2;
    v1.Sub(sk_j2->worldMatrix.Row(3), sk_jmid->worldMatrix.Row(3) );
    v2.Sub(sk_jmid->worldMatrix.Row(3), sk_j1->worldMatrix.Row(3) );
    mRotateAxis0.CrossProduct(v1, v2);
    mRotateAxis0.Rotate(mInvShiftedParent);
    
    //Vector v;
    //v = sk_jmid->worldMatrix.Row(3);
    //v.Rotate(mInvShiftedParent);
    
    // Calculate the twist matrices needed to correct local orientation of joints
    // All twist matrices should be x-axis rotations for x-aligned bones
    
    Vector goal0 = sk_j2->worldMatrix.Row(3);
    SetLocalGoal(goal0);    
    mLocalGoal0 = mLocalGoal;   
    mProjAxis = GetProjFromSkel();
    mPosAxis.CrossProduct(mProjAxis, mLocalGoal);   
    mProjAxis0 = mProjAxis; mPosAxis0 = mPosAxis;
    mIKSolver->init(mT.m, mS.m, (float*)&mProjAxis, (float*)&mPosAxis);
    
    mIKSolver->mS = mS;
    mIKSolver->mT = mT;
    mIKSolver->mInvS = mS;
    mIKSolver->mInvS.Invert(); 
    
    
    // Twist matrices are computed by comparing the IKAN solution when the goal
    // is the wrist position to the rotation matrices of skeleton->joint->restPose
    // The initial poleVector is calculated from the elbow position.
    // There must be a bend in the elbow.
    // Tw = Ro . R^(-1) ;  Tw = Twist matrix, Ro = restPose, R = IKAN solution (R1, Rmid or R2)
    // Ra, Rb are for non-x-aligned bones, not implemented yet.
    
    mOldVersion = true;
    
    ComputeTwistMatrices();
}

void IKDriver::SetSolverAxes()
{
    float proj[3], pos[3];
    proj[0] = mProjAxis.x; proj[1] = mProjAxis.y; proj[2] = mProjAxis.z;
    pos[0] = mPosAxis.x; pos[1] = mPosAxis.y; pos[2] = mPosAxis.z;
    mIKSolver->SetAxes( proj, pos );
}

void IKDriver::SetProjAxis(const Vector& v)
{
    mProjAxis = v;
    mPosAxis.CrossProduct(mProjAxis, mLocalGoal);
    SetSolverAxes();
}

void IKDriver::SetPoleVector(Vector v)
    // converts world space vector to shiftedParent coordinates and sets projAxis
{
    v.Sub(mPose->GetJoint(mStartIndex)->GetWorldTranslation());
    v.Rotate(mInvShiftedParent);
    SetProjAxis(v);
}

void IKDriver::SetPointConstraint(const Vector& v)
{
    mConstraintType = CPosition;
    mPointConstraint.Sub(v,mPose->GetJoint(mStartIndex)->GetWorldTranslation());
}

void IKDriver::SetEndEffectorOffset(const Vector& v)
{
    mE.Identity();
    mE.Row(3) = v;
}

void IKDriver::SetEndEffectorOffset(const Matrix& m)
{
    mE = m;
}

void IKDriver::SetShiftedParentPose()
    // use invShiftedParent to re-orient vectors in the local frame (no translation), 
    // invShiftedParent to transform from world to shoulder coordinates
{
    poser::Joint* j1 = mPose->GetJoint(mStartIndex);
    poser::Joint* parentJnt = j1->GetParentJoint();
    Matrix parent; 
    parent = parentJnt->GetWorldTransform().GetMatrix();
    mShiftedParent = parent;
    mShiftedParent.FillTranslate(j1->GetWorldTransform().GetTranslation());
    mInvShiftedParent = mShiftedParent;  
    mInvShiftedParent.Invert();
}

void IKDriver::SetShiftedParentSkel()
    // use invShiftedParentRot to re-orient vectors in the local frame (no translation), 
    // invShiftedParent to transform from world to shoulder coordinates
{
    tSkeleton::Joint* sk_j1 = mPose->GetSkeleton()->GetJoint(mStartIndex);
    tSkeleton::Joint* parentJnt = mPose->GetSkeleton()->GetJoint(sk_j1->parentIndex); 
    Matrix parent; 
    parent = parentJnt->worldMatrix; 
    mShiftedParent = parent;
    mShiftedParent.FillTranslate(sk_j1->worldMatrix.Row(3));
    mInvShiftedParent = mShiftedParent;  
    mInvShiftedParent.Invert();
}

void IKDriver::SetLocalGoal(const Vector& newGoal)
{
    Vector v;
    v = newGoal;
    v.Transform(mInvShiftedParent);  // put goal into shifted-parent frame   
    mLocalGoal = v;
}

void IKDriver::SetLocalGoal(const Matrix& newGoal)
{
    Matrix m; 
    m = newGoal;
    m.MultFull(mInvShiftedParent);  // put goal into shifted-parent frame   
    mLocalGoalMat = m;
    mLocalGoal = m.Row(3);
}

void IKDriver::SetProjDefault()
    // Assumes Y is up, -Z is going forward in the character frame
{
    if(mIKLimbType == 0)  //elbow
    {
        // goal in front, elbow points forward
        mProjAxis.Rotate(Z, mInvShiftedParent);
    }
    else if (mIKLimbType == 1)  //knee
    {
        // leg below the waist, knee points forward
        mProjAxis.Rotate(nZ, mInvShiftedParent);
        
    }
    mPosAxis.Rotate(Y, mInvShiftedParent);
    SetSolverAxes(); // sets iksolver axes
}

void IKDriver::SetProjParented()
{
    float angle;
    Vector axis;
    Matrix m;
    m.Identity();
    AxisAngle(mLocalGoal0, mLocalGoal, axis, angle);
    if( angle == PI )
    {
        m.FillRotation(mProjAxis0, PI);  // an axis perpendicular to mLocalGoal0
    }
    else
    {
        m.FillRotation(axis, angle);
    }
    mProjAxis.Rotate(mProjAxis0, m); mPosAxis.Rotate(mPosAxis0, m);
    SetSolverAxes();
}

void IKDriver::SetRotateAxis()
{
    float angle;
    Vector axis;
    Matrix m;
    m.Identity();
    AxisAngle(mLocalGoal0, mLocalGoal, axis, angle);
    
    if( angle == PI )
    {
        m.FillRotation(mProjAxis0, PI);  // an axis perpendicular to mLocalGoal0
    }
    else if (Abs(angle) > 0.00001f)
    {
        m.FillRotation(axis, angle);
    }
    // Find rotation axis in midjoint frame
    mRotateAxis.Rotate(mRotateAxis0, m);
}

bool IKDriver::IsSolveNeeded(const Vector& inGoal )
    // sets localGoal if different from newGoal 
    // Postcondition: localGoal is (within epsilon of) newGoal in shiftedParent frame
{
    if( inGoal.Equals(mLocalGoal))
        return false;
    else
        return true;
}

bool IKDriver::IsSolveNeeded(const Matrix& inGoal )
    // sets localGoal if different from newGoal 
    // not quite right for orientation yet... need .Equals for matrices
    // Postcondition: localGoal is (within epsilon) newGoal in shiftedParent frame
{
    Matrix m = inGoal;
    if( m.Row(3).Equals(mLocalGoalMat.Row(3)))
        return false;
    else
        return true;
}

Vector IKDriver::GetGoalFromPose()
    //! Need to handle 3 aligned joints case
    // Gets end joint position in CharacterRoot space from pose
{
    poser::Joint* j1 = mPose->GetJoint(mStartIndex); 
    poser::Joint* j2 = mPose->GetJoint(mEndIndex);  
    poser::Joint* jmid = mPose->GetJoint(mMidIndex); 
    
    
    Matrix cumulativeRot, temp;
    Vector endPos, addvec;
    cumulativeRot = j1->GetObjectTransform().GetMatrix();
    endPos.Rotate(jmid->GetObjectTransform().GetTranslation(), cumulativeRot);
    temp = cumulativeRot;
    cumulativeRot.Mult(jmid->GetObjectTransform().GetMatrix(), temp);
    addvec.Rotate(j2->GetObjectTransform().GetTranslation(), cumulativeRot);
    endPos.Add(addvec);
    
    return endPos;
}

Vector IKDriver::GetMidJointPose()
    //! Need to handle 3 aligned joints case
    // Gets mid joint position in ShiftedParent space from pose
{
    poser::Joint* j1 = mPose->GetJoint(mStartIndex); 
    poser::Joint* jmid = mPose->GetJoint(mMidIndex);  
    
    Vector midPos;
    midPos = jmid->GetObjectTransform().GetTranslation();
    midPos.Rotate(j1->GetObjectTransform().GetMatrix());
    return midPos;
}

Vector IKDriver::GetMidJointSkel()
    // Gets mid joint position in ShiftedParent space from skeleton
{
    tSkeleton::Joint* j1 = mPose->GetSkeleton()->GetJoint(mStartIndex);       
    tSkeleton::Joint* jmid = mPose->GetSkeleton()->GetJoint(mMidIndex);     
    
    Vector midPos;
    midPos = jmid->restPose.Row(3);
    midPos.Rotate(j1->restPose);
    return midPos;
}

Vector IKDriver::GetProjFromPose()
    // Precondition: SetShiftedParent() has been called for the current pose
    //               LocalGoal has been updated
{
    Vector midPos = GetMidJointPose();
    Vector norm;
    norm = mLocalGoal;
    norm.Normalize();
    norm = PerpComponent(midPos, norm);
    norm.Normalize();
    return norm;
}

Vector IKDriver::GetProjFromSkel()
{
    Vector midPos = GetMidJointSkel();
    Vector norm;
    norm = mLocalGoal;
    norm.Normalize();
    norm = PerpComponent(midPos, norm);
    norm.Normalize();
    return norm;
}

bool IKDriver::ComputeTwistMatrices()
{
    tSkeleton::Joint* sk_j1 = mPose->GetSkeleton()->GetJoint(mStartIndex);       // shoulder or upper leg
    tSkeleton::Joint* sk_jmid = mPose->GetSkeleton()->GetJoint(mMidIndex);   // elbow or knee
    tSkeleton::Joint* sk_j2 = mPose->GetSkeleton()->GetJoint(mEndIndex);       // wrist or ankle
    tSkeleton::Joint* parentJnt = mPose->GetSkeleton()->GetJoint(sk_j1->parentIndex);
    
    Vector goal0 = sk_j2->worldMatrix.Row(3);
    SetLocalGoal(goal0); 
    
    SolveForGoal(goal0);
    
    Matrix invR1, invRmid, invR2, invS;
    invR1 = mR1; invRmid = mRmid, invR2 = mR2;
    invR1.Invert();  invRmid.Invert(); invR2.Invert();
    
    mTw1.Identity();  
    mTw1.Mult( sk_j1->restPose, invR1);
    mTw1.IdentityTranslation();
    mTw1.OrthoNormal();
    mInvTw1.Identity();
    mInvTw1 = mTw1;
    mInvTw1.Invert();
    mInvTw1.OrthoNormal();
    
    mTwmid.Identity();
    mTwmid.Mult( sk_jmid->restPose, mTw1);
    mTwmid.IdentityTranslation();   
    mTwmid.Mult(invRmid );
    mTwmid.OrthoNormal();
    
    Matrix  A, B, Tb, skR1, skRmid, skR2;
    A.Identity(); B.Identity(); mTa.Identity(); Tb.Identity(); 
    skR1.Identity(); skRmid.Identity(); skR2.Identity();
    
    
    CopyRotation(sk_j1->restPose, skR1);
    A.Mult(skR1, invR1);
    mTa.FillTranslate(sk_jmid->restPose.Row(3));
    
    CopyRotation(sk_jmid->restPose, skRmid);
    B.Mult(skRmid, invRmid);
    Tb.FillTranslate(sk_j2->restPose.Row(3));
    
    mT.Mult(mTa, A);
    mS.Mult(Tb, B);
    
    mIKSolver->mS = mS;
    mIKSolver->mT = mT;
    
    mIKSolver->mInvS = mS;
    mIKSolver->mInvS.Invert(); 
    
    if(!mOldVersion)
        mIKSolver->init(mT.m, mS.m, (float*)&mProjAxis, (float*)&mPosAxis);
    
    // put mRotateAxis0 into A frame (elbow frame)
    Matrix invT;
    invT = mT;
    invT.Invert();
    mRotateAxis0.Rotate( invT );
    
    return true;
}


void IKDriver::ReplacePose(poser::Pose* inPose)
{
    rAssert(inPose);
    
    mPose->Release();
    mPose = inPose;
    mPose->AddRef();
}

void IKDriver::ReplaceEffectorTarget(SimTarget* inTarget)
{
    rAssert(inTarget);
    
    mEffectorTarget->Release();
    mEffectorTarget = inTarget;
    mEffectorTarget->AddRef();
}

void IKDriver::ProgressiveGoal(Matrix& goal)
    // Finds an interpolated goal between current and final endjoints, polevectors and orientations
{
    Vector distance;
}

bool IKDriver::SolveForGoal(const Vector& goal)
    // Precondition: goal is in character-root coordinates
    // SetShiftedParentPose() or SetShiftedParentSkel() should be called first
{
    
    SetLocalGoal(goal);
    if(mOptimize)
    {
        if(!IsSolveNeeded(mLastGoal))
            return false;
        mLastGoal = mLocalGoal;
    }
    
    float rangle = 0;
    
    int success = 0;
    if(mOldVersion)
        success = mIKSolver->SetGoalPos((float*)&mLocalGoal, mE.m, rangle);  
    else
    {  
        SetRotateAxis();
        success = mIKSolver->SetGoalPos((float*)&mLocalGoal, mE.m, mRotateAxis, mRmid); 
    }
    
    if (success)
    {
        if(mConstraintType == CPosition)
        {
            Vector pole;
            float fcenter[3];
            mIKSolver->GetCircleCenter(fcenter);
            pole.Sub(mPointConstraint, float2Vec(fcenter));
            SetPoleVector(pole);
            mIKSolver->SolveR1(mSwivelAngle, mR1.m);
        }
        else
        {
            mIKSolver->SolveR1(mSwivelAngle, mR1.m);
        }
        
        Vector axis;
        axis.CrossProduct( mS.Row(3), mTa.Row(3) );
        if(mOldVersion)
            mRmid.FillRotateY(rangle);  
    }
    
    return success == 0 ? false : true;
}

bool IKDriver::SolveForGoal(const Matrix& inGoal)
{
    Matrix temp = mLocalGoalMat;
    SetLocalGoal(inGoal);
    
    if(mOptimize)
    {
        if(IsSolveNeeded(temp))
            return false;
    }
    
    float rangle = 0;
    int success = mIKSolver->SetGoal(mLocalGoalMat.m, rangle);
    
    if (success)
    {
        
        if (mConstraintType == CPosition)
        { 
            Vector pole;
            float fcenter[3];
            mIKSolver->GetCircleCenter(fcenter);
            pole.Sub(mPointConstraint, float2Vec(fcenter));
            SetPoleVector(pole);
            mIKSolver->SolveR1R2(mSwivelAngle, mR1.m, mR2.m);
            
        }
        else
        {
            mIKSolver->SolveR1R2(mSwivelAngle, mR1.m, mR2.m);
        }
        
        mRmid.FillRotateY(rangle);
        
    }
    return success == 0 ? false : true;
}

void IKDriver::SolveAim(const Vector& inGoal)
{
    
    Vector temp; 
    temp = inGoal;
    SetLocalGoal(inGoal);
    if(mOptimize)
    {
        if(IsSolveNeeded(temp))
            return;
    }
    
    float rangle = 0.2f;// = swivelangle;
    //in this case, rangle is given, not solved for; third argument should be const
    mIKSolver->SetAimGoal((float*)&mLocalGoal, (float*)&mProjAxis, rangle);
    
    
    
    mIKSolver->SolveAim(mSwivelAngle, mR1.m);
    
    mRmid.FillRotateY(rangle);
    
    //return success == 0 ? false : true;
}

void IKDriver::Advance(float dt)
{
    SetShiftedParentPose();
    Matrix goal;
    goal.Identity();
    //dynamic_cast<SimMatrixProgressiveTarget*>(mEffectorTarget)->GetTransform(goal, dt);
    //dynamic_cast<SimMatrixTarget*>(mEffectorTarget)->GetTransform(goal, dt);
    mEffectorTarget->GetTransform(goal, dt);
    
    
    if(mModifierType == MParented)
    {                   
        SetLocalGoal(goal.Row(3));
        SetProjParented();
    }
    else if (mModifierType == MPoleFromPose)
    {
        SetLocalGoal(goal.Row(3));
        SetProjAxis(GetProjFromPose());
    }
    else if (mModifierType == MPoleAndGoalFromPose)
    {         
        goal.Row(3) = GetGoalFromPose();
        goal.Row(3).Transform(mShiftedParent);
        SetLocalGoal(goal.Row(3));
        SetProjAxis(GetProjFromPose());
    }
    else if (mModifierType == MPoleAndGoalFromMaya)
    {
        Vector constraint;
        mConstraintTarget->GetPos(constraint, dt);
        
        // uncomment if values are in local joint space coordinates
        //goal.Row(3).Transform(mShiftedParent);
        SetLocalGoal(goal);
        SetPoleVector(constraint);
        // characterRoot coordinates fed into objectMatrix by exporter
        // IK constraint joints are parented to joint(0)
        //SetPointConstraint(mPose->GetJoint(62)->worldMatrix.Row(3));
        //goal.Ro   w(3) = mPose->GetJoint(61)->worldMatrix.Row(3);       
    }
    
    if(mGoalType == GPosition)
        SolveForGoal(goal.Row(3));
    else
        SolveForGoal(goal);
    
    UpdateJoints();  
}

void IKDriver::UpdateJoints()
{  
    poser::Joint* j1 = mPose->GetJoint(mStartIndex);      // shoulder or upper leg
    poser::Joint* jmid = mPose->GetJoint(mMidIndex);      // elbow or knee
    poser::Joint* j2 = mPose->GetJoint(mEndIndex);        // wrist or ankle 
    
    Matrix updateR1, updateRmid, updateR2, temp;
    updateR1.Identity(); updateRmid.Identity(), updateR2.Identity(); temp.Identity();
    
    if(mOldVersion)
    {
        updateR1.Mult(mTw1, mR1);
        updateR1.OrthoNormal(); 
        
        updateRmid.Mult(mTwmid, mRmid);
        updateRmid.Mult( mInvTw1);
        updateRmid.OrthoNormal();   
    }
    else
    {
        CopyRotation(mT, temp);
        updateR1.Mult(temp, mR1);
        
        CopyRotation(mS, temp);
        updateRmid.Mult(temp, mRmid);  
    }
    
    updateR1.Row(3) = j1->GetObjectTransform().GetTranslation();
    j1->SetTransform(poser::SPACE_OBJECT, poser::Transform(updateR1));
    //j1->SetTransform(poser::SPACE_OBJECT, poser::Transform(mR1));        // IKAN solution without untwisting
    
    updateRmid.Row(3) = jmid->GetObjectTransform().GetTranslation();
    jmid->SetTransform(poser::SPACE_OBJECT, poser::Transform(updateRmid));
    //jmid->SetTransform(poser::SPACE_OBJECT, poser::Transform(mRmid));    // IKAN solution without untwisting
    
    
    if(mGoalType != GPosition)
    {
        mEffectorTarget->GetTransform(updateR2);
        temp = jmid->GetWorldTransform().GetMatrix();
        temp.Invert();
        updateR2.Mult(temp);
        updateR2.Row(3) = j2->GetObjectTransform().GetTranslation();
        j2->SetTransform(poser::SPACE_OBJECT, poser::Transform(updateR2));
    }
}

void IKDriver::DebugDisplay()
{
    DrawLineToggler toggler;

    // draw the target
    Vector smallBox(0.1f, 0.1f, 0.1f);
    //Vector goal;
    //mEffectorTarget->GetPos(goal);
    //dBox3(goal, smallBox, tColour(0, 0, 0) );
    

    Vector lgoal(mLocalGoal);
    lgoal.Transform(mShiftedParent);
    dBox3(lgoal, Vector(.01f, .01f, .01f), tColour(0, 0, 0) );
    Matrix worldTarget;
	worldTarget.Mult(mLocalGoalMat, mShiftedParent);
	dAxes(0.1f, worldTarget);

    Vector temp, origin;
    poser::Joint* j = mPose->GetJoint(mEndIndex); 
    
    // draw local axes of joints in chain + parent joint
    for(int i=0; i<4; i++)
    {
        origin = j->GetWorldTransform().GetTranslation();
        temp.Transform(LOCAL_X, j->GetWorldTransform().GetMatrix());
        dLine2(origin, temp, RED);
        temp.Transform(LOCAL_Y, j->GetWorldTransform().GetMatrix());
        dLine2(origin, temp, GREEN);
        temp.Transform(LOCAL_Z, j->GetWorldTransform().GetMatrix());
        dLine2(origin, temp, BLUE);
        
        // draw bones white
        if(i<3)
        {
            temp = j->GetParentJoint()->GetWorldTranslation();
            dLine2(temp, origin, tColour(255, 255, 255));
        }
        j = j->GetParentJoint();
    }
    
    // draw circle equation parameters for shoulder
    j = mPose->GetJoint(mStartIndex);
    origin = j->GetWorldTranslation();
    
    float r;
    float fc[3]; float fu[3]; float fv[3];
    Vector c, u, v, g;
    mIKSolver->GetCircleVars(fc,fu,fv,r);
    c = float2Vec(fc); u = float2Vec(fu); v = float2Vec(fv);
    g = mLocalGoal;
    
    
    //Put all debug vectors into characterRoot space
    u.Normalize();
    u.Scale(mIKSolver->radius);
    u.Rotate(mShiftedParent);
    
    v.Normalize();
    v.Scale(0.3f);
    v.Transform(mShiftedParent);
    
    c.Transform(mShiftedParent);
    g.Transform(mShiftedParent);
    
    Vector tmp;
    
    
    // Vectors starting at the centre of the circle equation
    tmp = u;   
    dLineRel(c, tmp, GREEN);           // u vector (poleVector) from circle equation green
    tmp=nZ;
    tmp.Rotate(mShiftedParent);
    //dLineRel(c, tmp, tColour(10, 150, 255));        // nZ (in local space) light blue
    
    if(mConstraintType == CPosition)
    {
        dLine2(c, mPointConstraint, tColour(20, 220, 40));  // point constraint  off green
    }
    
    // Vectors starting at startJoint
    
    dLine2(origin, g, BLUE);                        // shoulder to goal  blue
    tmp = mLocalGoal0;
    tmp.Rotate(mShiftedParent);
    //dLineRel(origin, tmp, tColour(180, 180, 180));   // localGoal0 light grey
    tmp=mProjAxis0;
    tmp.Rotate(mShiftedParent);    
    //dLineRel(origin, tmp, tColour(100, 100, 100));    // projAxis0 dark grey
    tmp=mProjAxis;
    tmp.Rotate(mShiftedParent);
    dLineRel(mPose->GetJoint(mEndIndex)->GetWorldTranslation(), tmp, tColour(220, 100, 40));   // projAxis redish
    tmp=mPosAxis;
    tmp.Rotate(mShiftedParent);  
    //dLineRel(origin, tmp, tColour(70, 60, 250) );    // posAxis greenish
    
    tmp = mRotateAxis;
    tmp.Normalize();
    tmp.Rotate(mT);
    tmp.Rotate(mShiftedParent);                        // yellow
    //dLineRel(mPose->GetJoint(mMidIndex)->worldMatrix.Row(3), tmp, tColour(255,255,0) );
    
    tmp = mRotateAxis0;
    tmp.Normalize();
    tmp.Rotate(mT);
    tmp.Rotate(mShiftedParent);                        // light green
    //dLineRel(mPose->GetJoint(mMidIndex)->worldMatrix.Row(3), tmp, tColour(84,255,159) );
    
}

}  // namespace sim
