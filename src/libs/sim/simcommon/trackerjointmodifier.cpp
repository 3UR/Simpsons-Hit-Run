
#include <simcommon/TrackerJointModifier.hpp>
#include <simcommon/simtarget.hpp>
#include <simcommon/simmath.hpp>
#include <simcommon/dline2.hpp>
#include <poser/pose.hpp>
#include <poser/joint.hpp>
#include <p3d/anim/skeleton.hpp>

using namespace RadicalMathLibrary;

namespace sim
{
    
TrackerJointModifier::TrackerJointModifier(poser::Pose* inPose, int inIndex, SimTarget* t)
    : mLookAt(t),
    mTrackerJointDef(0),
    mLookAtControl(LookAtEmpty),
    mLookAtDir0(1.0f, 0, 0),
    mAxis0(0, 1.0f, 0),
    mTrackVertical(true),
    mTrackerJointDefVertical(0),
    mSeeTarget(false),
    mJointIndex(inIndex),
    mCurrentDt(0)
{
    rAssert(mLookAt);
    mLookAt->AddRef();
    
    mSpeedH = 4.0f;           // default trackerJoint angular speed in rad/sec
    mSpeedV = 4.0f;           // default trackerJoint angular speed in rad/sec
    mGoodEnough = 0.999f;     // default acceptable deviation
    mMaxangleH = 1.0f;        // default max deformation
    mMaxangleV = 1.0f;        // default max deformation
    
    mUseAnimJointLocalMat = true;
    
    mJoint = inPose->GetJoint(mJointIndex);
    mJointLocalMat = inPose->GetSkeleton()->GetJoint(mJointIndex)->restPose;
    mStoredAnimMat = mJointLocalMat;
    mRestPose = mJointLocalMat;
}

TrackerJointModifier::~TrackerJointModifier()
{
    mLookAt->Release();
}

void TrackerJointModifier::LookAt(SimTarget* t, bool resetdef)
{
    mLookAt->Release();
    mLookAt = t;
    mLookAt->AddRef();
    
    if (resetdef)
    {
        mTrackerJointDef = 0;
    }
    SetLookAtControl(mLookAt == NULL ? LookAtEmpty : LookAtActivated);
}

void TrackerJointModifier::SetLookAtControl(LookAtControlEnum c)
{
    if (mLookAtControl == c)
        return;
    
    // switching from LookAtActivated to LookAtEmpty will pop if there is any deformation
    
    /*
    The following code make sure that there is no pop when we activate/deactivate the tracker.
    This will work only for horizontal deformation for the moment.
    */
    
    // switching from LookAtActivated to LookAtDissipating: dissipate toward animation pose
    if (mLookAtControl == LookAtActivated && c == LookAtDissipating)
    {
        rAssert(mUseAnimJointLocalMat == false);
        
        Matrix curHeadTransform = mJointLocalMat; // the rest pose
        AddDefToMatrix(curHeadTransform);
        
        Vector w;
        WFromRotMatrix(w, mStoredAnimMat, curHeadTransform, 1.0f);
        mTrackerJointDef = w.DotProduct(mAxis0);
        
        //Vector axis1;
        //axis1.CrossProduct(mLookAtDir0, mAxis0);
        //mTrackerJointDefVertical = w.DotProduct(axis1);
        
        mUseAnimJointLocalMat = true;
    }
    // switching toward LookAtActivated: add deformation over the restpose
    else if (c == LookAtActivated)
    {
        rAssert(mUseAnimJointLocalMat == true);
        mUseAnimJointLocalMat = false;
        mJointLocalMat = mRestPose;
        
        Matrix curHeadTransform = mStoredAnimMat;
        AddDefToMatrix(curHeadTransform);
        
        Vector w;
        WFromRotMatrix(w, mJointLocalMat, curHeadTransform, 1.0f);
        mTrackerJointDef = w.DotProduct(mAxis0);
        
        //Vector axis1;
        //axis1.CrossProduct(mLookAtDir0, mAxis0);
        //mTrackerJointDefVertical = w.DotProduct(axis1);
    }
    
    mLookAtControl = c;
}

void TrackerJointModifier::SetAnimJointLocalMat()
{
    // the pose has just been updated by the animation
    //rAssert(pose->IsPoseReady() == false);
    
    mStoredAnimMat = mJoint->GetObjectTransform().GetMatrix();
}

void TrackerJointModifier::AddDefToMatrix(Matrix& inMatrix) const
{
    Matrix m;
    m.Identity();
    
    Vector axis1;
    axis1.CrossProduct(mLookAtDir0, mAxis0);
    
    if (Fabs(mTrackerJointDef) > 0)
    {
        m.Identity();
        m.FillRotation(mAxis0, -mTrackerJointDef);
        axis1.Rotate(m);
    }
    if (Fabs(mTrackerJointDefVertical) > 0)
    {
        Matrix tmp;
        tmp.Identity();
        tmp.FillRotation(axis1, mTrackerJointDefVertical);
        m.Mult(tmp);
    }
    m.Mult(inMatrix);
    inMatrix = m;
}
    
void TrackerJointModifier::Update(poser::Pose* inPose)
{
    if (!IsEnabled())
        return;
    
    mJoint = inPose->GetJoint(mJointIndex);
    
    mSeeTarget = false;
    
    SetAnimJointLocalMat();
    
    if (mUseAnimJointLocalMat)
    {
        mJointLocalMat = mStoredAnimMat;
    }
    
    if (mLookAtControl != LookAtEmpty && mCurrentDt > 0)
    {
        float speedHdt = mSpeedH*mCurrentDt;
        float speedVdt = mSpeedV*mCurrentDt;
        
        poser::Transform pt = mJoint->GetObjectTransform();
        pt.SetMatrix(mJointLocalMat);
        
        if (Fabs(mTrackerJointDef) > 0 || Fabs(mTrackerJointDefVertical) > 0)
        {
            Matrix m = mJointLocalMat;
            AddDefToMatrix(m);
            pt.SetMatrix(m);
        }
        mJoint->SetObjectTransform(pt);
        
        // vector from the object to the trackerJoint, use the previous worldMatrix
        Vector dr;
        float dot = 0;
        float drDotAxis0 = 0;
        Vector drv, drh;
        
        if (mLookAtControl != LookAtDissipating)
        {
            Matrix m = mJoint->GetWorldTransform().GetMatrix();
            mLookAt->GetPos(dr);
            dr.Sub(m.Row(3));
            
            m.IdentityTranslation();
            m.InvertOrtho();
            dr.Rotate(m); // dr in joint space
            
            if (dr.NormalizeSafe() < VERY_SMALL)
            {
                return; // no can do
            }
            
            // mAxis0 is the main rotation axis of the joint. For the head it would be the
            // axis around which the head turns to look left and right. drh the projection 
            // of dr in the plane perp to mAxis0 and drv to other component
            drDotAxis0 = dr.DotProduct(mAxis0);
            drv.Scale(drDotAxis0, mAxis0);
            drh.Sub(dr, drv);
            drh.Normalize();
            
            dot = drh.DotProduct(mLookAtDir0);
            if (dot > 0)
                mSeeTarget = true;
        }
        
        if (!mSeeTarget) // if obj is behind relax the current mTrackerJointDef
        {
            if (Fabs(mTrackerJointDef) < speedHdt)
            {
                mTrackerJointDef = 0;
                
                if (mLookAtControl == LookAtDissipating)
                    SetLookAtControl(LookAtEmpty);
            }
            else
            {
                mTrackerJointDef -= (mTrackerJointDef > 0 ? speedHdt : -speedHdt);
            }
        }
        else if (dot < mGoodEnough) // do nothing if trackerJoint already look at the right place
        {
            float ang = ACos(dot);
            rAssert(ang > 0);
            
            if (ang > speedHdt)
                ang = speedHdt;
            
            Vector tmp;
            tmp.CrossProduct(mLookAtDir0, drh);
            if (tmp.DotProduct(mAxis0) < 0)
                ang = -ang;
            
            mTrackerJointDef += ang;
            
            if (Fabs(mTrackerJointDef) > mMaxangleH)
                mTrackerJointDef = (mTrackerJointDef > 0 ? mMaxangleH : -mMaxangleH);
        }
        
        if (mTrackVertical)
        {
            if (!mSeeTarget) // if obj is behind relax the current mTrackerJointDef
            {
                if (Fabs(mTrackerJointDefVertical) < speedVdt)
                {
                    mTrackerJointDefVertical = 0;
                }
                else
                {
                    mTrackerJointDefVertical -= (mTrackerJointDefVertical > 0 ? speedVdt : -speedVdt);
                }
            }
            else
            {
                dr.Normalize();
                float dot = dr.DotProduct(drh);
                
                if (dot < mGoodEnough) // do nothing if trackerJoint already look at the right place
                {
                    float ang = ACos(dot);
                    rAssert(ang > 0);
                    
                    if (ang > speedVdt)
                        ang = speedVdt;
                    
                    if (drDotAxis0 > 0)
                        ang = -ang;
                    
                    mTrackerJointDefVertical += ang;
                    
                    if (Fabs(mTrackerJointDefVertical) > mMaxangleV)
                    {
                        mTrackerJointDefVertical = (mTrackerJointDefVertical > 0 ? mMaxangleV : -mMaxangleV);
                    }
                }
            }
        }
        
        pt = mJoint->GetObjectTransform();
        Matrix m = mJointLocalMat;
        AddDefToMatrix(m);
        pt.SetMatrix(m);
        //pt.SetMatrix(mJointLocalMat);
        mJoint->SetObjectTransform(pt);
    }
    else
    {
        if (!mUseAnimJointLocalMat)
        {
            poser::Transform pt = mJoint->GetObjectTransform();
            pt.SetMatrix(mJointLocalMat);
            mJoint->SetObjectTransform(pt);
        }
    }
    mCurrentDt = 0;
}

void TrackerJointModifier::DebugDisplay()
{
    DrawLineToggler toggler;

    Matrix m = mJoint->GetWorldTransform().GetMatrix();
    Vector p = m.Row(3);
    Vector v;
    static float scale = 1.0f;
    
    m.IdentityTranslation();
    m.OrthoNormal();
    
    tColour c(100, 50, 50);
    
    m.RotateVector(mLookAtDir0, &v); // the direction currently looked at
    v.ScaleAdd(p, scale, v);
    dStreamLine(p, v, c, c);
    
    m.RotateVector(mAxis0, &v); // the axis of rotation
    v.ScaleAdd(p, scale/2.0f, v);
    dStreamLine(p, v, c, c);
}

} // sim