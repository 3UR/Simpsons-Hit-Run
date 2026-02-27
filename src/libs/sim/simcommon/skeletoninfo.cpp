
#include "simcommon/skeletoninfo.hpp"
#include "simcommon/dline2.hpp"
#include "p3d/anim/skeleton.hpp"
#include "p3d/utility.hpp"
#include <string.h>

using namespace RadicalMathLibrary;

namespace sim
{

SkeletonInfo::SkeletonInfo(tSkeleton* inSkeleton)
{
    CLASSTRACKER_CREATE( SkeletonInfo );
    mNumJoints = inSkeleton->GetNumJoint();
    mJointInfo = new JointInfo[mNumJoints];

    SetNameObject(inSkeleton->GetNameObject());
    
    int* numChild;
    numChild = (int*)p3d::MallocTemp(mNumJoints*sizeof(int));
    memset(numChild, 0, mNumJoints*sizeof(int));
    
    int i;
    for (i=1; i<mNumJoints; i++) // add the contribution of the childs to the axis
    {
        int parent = inSkeleton->GetJoint(i)->parentIndex;
        
        if (numChild[parent] == 0) // addis all the contribution doesn't work because of some weird joints in the skeleton
        {
            mJointInfo[parent].mAxis.Add(inSkeleton->GetJoint(i)->worldMatrix.Row(3));
            mJointInfo[parent].mAxis.Sub(inSkeleton->GetJoint(parent)->worldMatrix.Row(3));
            numChild[parent]++;
        }
    }
    for (i=0; i<mNumJoints; i++) // normalize the axis
    {
        if (numChild[i] > 0)
        {
            mJointInfo[i].mLength = mJointInfo[i].mAxis.NormalizeSafe()/float(numChild[i]);
        }
    }
    for (i=0; i<mNumJoints; i++) // get the rot axis, reset it if not ortho to the axis
    {
        int parentIndex = inSkeleton->GetJoint(i)->parentIndex;
        mJointInfo[i].mRotAxis.CrossProduct(mJointInfo[parentIndex].mAxis, mJointInfo[i].mAxis);
        mJointInfo[i].mRotAxis.NormalizeSafe();
        
        rmt::Vector tmp;
        tmp.CrossProduct(mJointInfo[i].mRotAxis, mJointInfo[i].mAxis);
        if (tmp.Magnitude() < 0.8f)
            mJointInfo[i].mRotAxis.Clear();
    }
    for (i=0; i<mNumJoints; i++) // transform axis in joint space
    {
        rmt::Vector tmp;
        if (numChild[i] > 0)
        {
            tmp = mJointInfo[i].mAxis;
            inSkeleton->GetJoint(i)->inverseWorldMatrix.RotateVector(tmp, &mJointInfo[i].mAxis);
        }
        else // childless joint get their parent axis and get no default x aligned rotaxis
        {
            mJointInfo[i] = mJointInfo[inSkeleton->GetJoint(i)->parentIndex];
            mJointInfo[i].mRotAxis.Set(1.0f, 0, 0);
        }
    }
    for (i=1; i<mNumJoints; i++) // transform rotaxis in joint space
    {
        int parentIndex = inSkeleton->GetJoint(i)->parentIndex;
        rmt::Vector tmp = mJointInfo[i].mRotAxis;
        inSkeleton->GetJoint(parentIndex)->inverseWorldMatrix.RotateVector(tmp, &mJointInfo[i].mRotAxis);
    }
    p3d::FreeTemp(numChild);
}

SkeletonInfo::~SkeletonInfo() 
{ 
    CLASSTRACKER_DESTROY( SkeletonInfo );
    delete[] mJointInfo; 
}

void SkeletonInfo::SetJointAxis(int index, const rmt::Vector& axis, float len)
{
    rAssert(index >= 0 && index < mNumJoints);
    mJointInfo[index].mAxis = axis;
    mJointInfo[index].mAxis.Normalize();
    mJointInfo[index].mLength = len;
}

void SkeletonInfo::SetJointRotAxis(int index, const rmt::Vector& axis)
{
    rAssert(index >= 0 && index < mNumJoints);
    mJointInfo[index].mRotAxis = axis;
    mJointInfo[index].mRotAxis.Normalize();
}

bool SkeletonInfo::ValidSkeletonInfo()
{
    for (int i=0; i<mNumJoints; i++)
    {
        // first test: axis needs to be perpendicular to rotaxis
        if (mJointInfo[i].mLength > 0)
        {
            rmt::Vector tmp;
            tmp.CrossProduct(mJointInfo[i].mAxis, mJointInfo[i].mRotAxis);
            float test = tmp.MagnitudeSqr();
            if (rmt::Fabs(test -1.0f) > 0.01f)
                return false;
        }
    }
    return true;
}

void SkeletonInfo::DebugDisplay(tPose* inPose)
{
    DrawLineToggler toggler;

    tColour c1(0, 100, 150);
    tColour c2(0, 200, 100);
    
    for (int i=1; i<mNumJoints; i++)
    {
        rmt::Vector pos = inPose->GetJoint(i)->worldMatrix.Row(3);
        rmt::Vector axis;
        GetWorldJointAxis(i, inPose, axis);
        axis.ScaleAdd(pos, mJointInfo[i].mLength, axis);
        dStreamLine(pos, axis, c1, c1);
        
        GetWorldRotAxis(i, inPose, axis);
        axis.ScaleAdd(pos, mJointInfo[i].mLength, axis);
        dStreamLine(pos, axis, c2, c2);
    }
}

} // sim