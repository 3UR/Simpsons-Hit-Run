

#ifndef _MISCDRIVER_HPP_
#define _MISCDRIVER_HPP_

#include <radmath/radmath.hpp>
#include "p3d/anim/skeleton.hpp"
#include "poser/pose.hpp"


namespace sim
{


// sets joint 0 matrix to a given external rootmatrix
class RootMatrixDriver : public poser::PoseDriver
{
public:
    RootMatrixDriver(rmt::Matrix* inRootMatrix) : mRootMatrix(inRootMatrix) {}
    virtual int GetMinimumJointIndex() const { return 0; }
    virtual int GetPriority() const { return 0; }
    virtual void Advance(float dt) {}
    virtual void Update(poser::Pose* pose) 
    {
        poser::Joint* j = pose->GetJoint(0);
        rmt::Matrix m = j->GetObjectMatrix();
        m.Mult(*mRootMatrix);
        j->SetWorldMatrix(m);
    }
private:
    rmt::Matrix* mRootMatrix;
};


// applies simple oscillation transform to a given joint
class OscillatorDriver : public poser::PoseDriver
{
public:
    OscillatorDriver(int minJoint=0, float inMax=5.0f) : mMinJoint(minJoint) , mMaxTranslation(inMax)
    { 
        mSpeed.Set(0.0f, 50.0f, 0.0f);
        mTranslation.Clear();
    }
    virtual int GetMinimumJointIndex() const { return mMinJoint; }
    virtual int GetPriority() const { return 1; }
    virtual void Advance(float dt) 
    { 
        mTranslation.ScaleAdd(dt, mSpeed);
        if (mTranslation.MagnitudeSqr() > rmt::Sqr(mMaxTranslation))
            mSpeed.Scale(-1.0f);
    }
    virtual void Update(poser::Pose* pose) 
    {
        poser::Joint* j = pose->GetJoint(mMinJoint);
        rmt::Matrix m = j->GetObjectMatrix();
        m.Row(3).Add(mTranslation);
        j->SetObjectMatrix(m);

        if (mMinJoint == 0)
        {
            j->SetWorldMatrix(m);
        }
    }

    void SetMaxTranslation(float inMaxTranslation) { mMaxTranslation = inMaxTranslation; }
    void SetSpeed(const rmt::Vector& inSpeed) { mSpeed = inSpeed; }

private:
    int mMinJoint;
    rmt::Vector mSpeed;
    rmt::Vector mTranslation;
    float mMaxTranslation;
};

class RotationDriver : public poser::PoseDriver
{
public:
    RotationDriver(int minJoint=0.0f, int inAxis=0, float inSpeed=5.0f) : mMinJoint(minJoint)
    { 
        mSpeedAngle = inSpeed;
        if (inAxis==0)
            mRotationAxis.Set(1,0,0);
        else if(inAxis==1)
            mRotationAxis.Set(0,1,0);
        else
            mRotationAxis.Set(0,0,1);
        mAngle = 0.0f;
    }
    virtual int GetMinimumJointIndex() const { return mMinJoint; }
    virtual int GetPriority() const { return 1; }
    virtual void Advance(float dt) 
    { 
        mAngle += mSpeedAngle*dt;
    }
    virtual void Update(poser::Pose* pose) 
    {
        poser::Joint* j = pose->GetJoint(mMinJoint);
        rmt::Matrix m = j->GetObjectMatrix();
        rmt::Matrix m2;
        m2.Identity();
        m2.FillRotation(mRotationAxis, mAngle);
        static float dirAngle=.25f;
        if (dirAngle!=0.0f)
        {
            rmt::Matrix m3;
            m3.Identity();
            rmt::Vector dir(0,1,0);
            m3.FillRotation(dir,dirAngle);
            m2.Mult(m3);
        }
        m2.Mult(m);
        j->SetObjectMatrix(m2);

        if (mMinJoint == 0)
        {
            j->SetWorldMatrix(m2);
        }
    }

    void SetParams( float inAngleSpeed, float inAxis) { mAngle = inAngleSpeed; mSpeedAngle=inAxis; }

private:
    int mMinJoint;
    float mAngle;
    float mSpeedAngle;
    rmt::Vector mRotationAxis;
};

class OrbitDriver : public poser::PoseDriver
{
public:
    OrbitDriver(int minJoint=0.0f, int inAxis=0, float inSpeed=5.0f) : mMinJoint(minJoint)
    { 
        mSpeedAngle = inSpeed;
        if (inAxis==0)
            mRotationAxis.Set(1,0,0);
        else if(inAxis==1)
            mRotationAxis.Set(0,1,0);
        else
            mRotationAxis.Set(0,0,1);
        mAngle = 0.0f;

        mRadius.Set(5.0f, 0, 0);
    }
    
    virtual int GetMinimumJointIndex() const { return mMinJoint; }
    virtual int GetPriority() const { return 1; }
    virtual void Advance(float dt) 
    { 
        mAngle += mSpeedAngle*dt;
    }
    virtual void Update(poser::Pose* pose) 
    {
        poser::Joint* j = pose->GetJoint(mMinJoint);
        rmt::Matrix m = j->GetWorldMatrix();
        rmt::Matrix m2;
        m2.Identity();
        m2.FillRotation(mRotationAxis, mAngle);

        static float dirAngle=.25f;
        if (dirAngle!=0.0f)
        {
            rmt::Matrix m3;
            m3.Identity();
            rmt::Vector dir(0,1,0);
            m3.FillRotation(dir,dirAngle);
            m2.Mult(m3);
        }
        m2.Mult(m);

        rmt::Vector trans = mRadius;
        trans.Rotate(m2);
        m2.Row(3).Add(trans);
        j->SetObjectMatrix(m2);
        j->SetWorldMatrix(m2);
    }

    void SetParams(float inAngleSpeed, float inAxis) { mAngle = inAngleSpeed; mSpeedAngle=inAxis; }
    void SetOrbitCenter(const rmt::Vector& radius) 
    {
        mRadius = radius;
    }

private:
    int mMinJoint;
    float mAngle;
    float mSpeedAngle;
    rmt::Vector mRotationAxis;
    rmt::Vector mRadius;
};


class CarWheelDriver : public poser::PoseDriver
{
public:
    CarWheelDriver(float inSpeed, float inDirectionAngle)
    { 
        mMinJoint[0] = 1;
        mMinJoint[1] = 2;
        mMinJoint[2] = 3;
        mMinJoint[3] = 4;

        mSpeedAngle = inSpeed;
        mAngle = 0.0f;
        mDirectionAngle = inDirectionAngle;
        static int wheelAxis=0, directionAxis=1;
        if (wheelAxis==0)
            mRotationAxis.Set(1,0,0);
        else if(wheelAxis==1)
            mRotationAxis.Set(0,1,0);
        else
            mRotationAxis.Set(0,0,1);

        if (directionAxis==0)
            mDirectionAxis.Set(1,0,0);
        else if(directionAxis==1)
            mDirectionAxis.Set(0,1,0);
        else
            mDirectionAxis.Set(0,0,1);
    }
    virtual int GetMinimumJointIndex() const { return mMinJoint[0]; }
    virtual int GetPriority() const { return 1; }
    void SetSpeed( float inSpeed ) { mSpeedAngle = inSpeed;}
    void SetDirection( float inDirection ) { mDirectionAngle = inDirection; }
    virtual void Advance(float dt) 
    { 
        mAngle += -mSpeedAngle*dt;
    }
    virtual void Update(poser::Pose* pose) 
    {
        rmt::Matrix m, m2, m3;
        for (int i = 0 ; i<4 ; i++ )
        {
            m2.Identity();
            m3.Identity();
            poser::Joint* j = pose->GetJoint(mMinJoint[i]);
            m = j->GetObjectMatrix();
            m2.FillRotation(mRotationAxis, mAngle);
            if(i>1)
            {
                m3.FillRotation(mDirectionAxis,mDirectionAngle);
                m2.Mult(m3);
            }
            m2.Mult(m);
            j->SetObjectMatrix(m2);
        }
    }

    void SetParams( float inAngleSpeed, float inAxis) 
    { 
        SetSpeed(inAngleSpeed);
        SetDirection(inAxis);
    }

private:
    int mMinJoint[4];

    float mAngle;
    float mSpeedAngle;
    float mDirectionAngle;
    rmt::Vector mRotationAxis;
    rmt::Vector mDirectionAxis;
};

// this driver mimics the anim expression in Maya that copies the rotation from
// joint to another joint minus the twist around joint axis. 
// CopyRotationMinusTwistDriver assumes that the 
// joint is aligned along the axis(0, 1, 2) axis.
// this should be the last thing done, after any IK or physics etc drivers

static const int CopyRotationNoTwistDriverPriority = 20; 
static const int CopyRotationDriverPriority = 20; 

class CopyRotationMinusTwistDriver : public poser::PoseDriver
{
public:
    CopyRotationMinusTwistDriver(int srcJntIndex, int dstJntIndex, int axis=0) 
        : mSrcJntIndex(srcJntIndex), mDstJntIndex(dstJntIndex), mAxis(axis)
    {
        rAssert(mSrcJntIndex < mDstJntIndex);
        rAssert(mAxis >= 0 && mAxis < 3);
    }
    virtual int GetMinimumJointIndex() const { return mDstJntIndex; }
    virtual int GetPriority() const { return CopyRotationNoTwistDriverPriority; }

    virtual void Advance(float dt) {}
    virtual void Update(poser::Pose* pose) 
    {
        rmt::Matrix rest = pose->GetSkeleton()->GetJoint(mSrcJntIndex)->restPose;
        rmt::Matrix m = pose->GetJoint(mSrcJntIndex)->GetObjectMatrix();

        rmt::Vector axis;
        axis.CrossProduct(m.Row(mAxis), rest.Row(mAxis));           

        float norm = axis.NormalizeSafe();

        if (norm > 0.0f)
        {
            float angle = ASinSafe(norm);

            if (m.Row(mAxis).x * rest.Row(mAxis).x < 0.0f)
                angle = rmt::PI - angle;

            rmt::Matrix dm;
            dm.Identity();
            dm.FillRotation(axis, angle);
            m.Mult(pose->GetSkeleton()->GetJoint(mDstJntIndex)->restPose, dm);
        }
        pose->GetJoint(mDstJntIndex)->SetObjectMatrixRotation(m);
    }

private:
    int mSrcJntIndex;
    int mDstJntIndex;
    int mAxis;
};

class CopyRotationDriver : public poser::PoseDriver
{
public:
    CopyRotationDriver(int srcJntIndex, int dstJntIndex) 
        : mSrcJntIndex(srcJntIndex), mDstJntIndex(dstJntIndex)
    {
        rAssert(mSrcJntIndex < mDstJntIndex);
    }
    virtual int GetMinimumJointIndex() const { return mDstJntIndex; }
    virtual int GetPriority() const { return CopyRotationDriverPriority; }

    virtual void Advance(float dt) {}
    virtual void Update(poser::Pose* pose) 
    {
        pose->GetJoint(mDstJntIndex)->SetObjectMatrixRotation(pose->GetJoint(mSrcJntIndex)->GetObjectMatrix());
    }

private:
    int mSrcJntIndex;
    int mDstJntIndex;
};


} // sim


#endif // _MISCDRIVER_HPP_
