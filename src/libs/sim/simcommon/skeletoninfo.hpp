
#ifndef _SKELETONINFO_HPP_
#define _SKELETONINFO_HPP_

#include "p3d/entity.hpp"
#include "radmath/radmath.hpp"
#include "p3d/anim/pose.hpp"

class tSkeleton;

namespace sim
{

class SkeletonInfo
    : public tEntity
{
public:
    SkeletonInfo(tSkeleton* inSkeleton);
    ~SkeletonInfo();
    
    const rmt::Vector& GetJointAxis(int index) const { return mJointInfo[index].mAxis; }
    const rmt::Vector& GetRotAxis(int index)   const { return mJointInfo[index].mRotAxis; }
    
    void GetWorldJointAxis(int index, tPose* inPose, rmt::Vector& axis) const 
    { 
        inPose->GetJoint(index)->worldMatrix.RotateVector(mJointInfo[index].mAxis, &axis); 
    }
    void GetWorldRotAxis(int index, tPose* inPose, rmt::Vector& axis) const 
    { 
        inPose->GetJoint(index)->parent->worldMatrix.RotateVector(mJointInfo[index].mRotAxis, &axis);  
    }

    // those allow to manually set desired axis and joint axis for a given joint
    void SetJointAxis(int index, const rmt::Vector& axis, float len);
    void SetJointRotAxis(int index, const rmt::Vector& axis);

    // return false if sone joint are irregular.
    bool ValidSkeletonInfo();
    
    void DebugDisplay(tPose* inPose);
    
    class JointInfo
    {
    public:
        JointInfo(): mAxis(0.0f, 0.0f, 0.0f), mRotAxis(0.0f, 0.0f, 0.0f), mLength(0) {  }
        ~JointInfo() {}
        
        rmt::Vector    mAxis;         // approx. joint Axis in joint space, migth not work in no parent
        rmt::Vector    mRotAxis;      // approx. cross product of mAxis and parent mAxis in joint space
        //rmt::Vector    mFacingAxis;
        float          mLength;       // approx. boneLength
    };
    
    JointInfo* mJointInfo;
    int mNumJoints;
};


} // sim

#endif // _SkeletonInfo_HPP_
