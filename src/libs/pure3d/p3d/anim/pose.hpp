/*===========================================================================
    pose.hpp
    24-Nov-99 Created by Neall

    Copyright (c)1999 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _POSE_HPP
#define _POSE_HPP

#include <p3d/entity.hpp>
#include <radmath/radmath.hpp>

class tPose;

class tPosable
{
public:
    virtual tPose* GetPose() = 0;
};



class tSkeletonLoader;
class tSkeleton;

class tPose : public tRefCounted
{
public:
    tPose(tSkeleton* skeleton);
    tPose(int jointCount);

    //----------------------------------------------------------------
    class Joint
    {
    public:
        rmt::Matrix objectMatrix;
        rmt::Matrix worldMatrix;
        Joint* parent;
        tUID uid;
		int pad;

        void ConcatMatrix(const rmt::Matrix* m) { worldMatrix.Mult(objectMatrix, *m); }

        friend class tPose;
	};

    //----------------------------------------------------------------

    void SetSkeleton(tSkeleton* skeleton);
    tSkeleton* GetSkeleton() const  { return skeleton; }
    int GetNumJoint() const         { return nJoint; }
    
    Joint* GetJoint(const int i) const             { if ((i>=0)&&(i<nJoint)) return &joint[i]; return NULL; }
    Joint* FindJoint(const tUID uid) const;
    Joint* FindJoint(const char* name) const       { return FindJoint(tEntity::MakeUID(name)); }
    Joint* FindJoint(const tName& name) const      { return FindJoint(name.GetUID()); }
    int    FindJointIndex(const tUID uid) const;
    int    FindJointIndex(const char* name) const  { return FindJointIndex(tEntity::MakeUID(name)); }
    int    FindJointIndex(const tName& name) const { return FindJointIndex(name.GetUID()); }

    void Evaluate(const rmt::Matrix* root = NULL);
    void EvaluateJointMatrix(const tUID uid, const rmt::Matrix* root, rmt::Matrix* matrix);
    void EvaluateJointPosition(const tUID uid, const rmt::Matrix* root, rmt::Vector* position);

    // hack to update unanimated joints for DA
    void EvaluateJointMatrix(unsigned joint, const rmt::Matrix* root);

    void SetPoseReady(bool ready)                  { poseReady = ready; }
    bool IsPoseReady()                             { return poseReady; }

    void ResetToRestPose();
    unsigned int UpdateCount(void)                 { return updateCount; }

protected:
    virtual ~tPose();

    int nJoint;
    int nRealJoint;
    tSkeleton* skeleton;
    Joint* joint;
    bool poseReady;
    unsigned int updateCount;
};

#endif /* _POSE_HPP */

