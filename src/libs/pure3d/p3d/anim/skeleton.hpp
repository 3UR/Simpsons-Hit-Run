/*===========================================================================
    skeleton.hpp
    24-Nov-99 Created by Neall

    Copyright (c)1999 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _SKELETON_HPP
#define _SKELETON_HPP

#include <p3d/entity.hpp>
#include <p3d/loadmanager.hpp>
#include <p3d/array.hpp>

class tPose;

class tSkeleton : public tEntity
{
public:
    tSkeleton(int nJoint);

    //----------------------------------------------------------------
    class Joint
    {
    public:
        tUID uid;
        int parentIndex;
        int dof;

        rmt::Matrix restPose;
        rmt::Matrix worldMatrix;
        rmt::Matrix inverseWorldMatrix;

        int   mappedJointIndex;
        float xAxisMap;
        float yAxisMap;
        float zAxisMap;

        // Needed for mirroring
        rmt::Vector restPoseYZXEuler;

        // Need for applying animation from skeletons of different 
        // Hierarchys
        bool preserveBoneLengths;
	};

    //----------------------------------------------------------------

    int GetNumJoint() const                     { return joint.Size(); }
    Joint* GetJoint(const int i) const          { return &joint[i]; }
    Joint* FindJoint(const tUID) const;
    Joint* FindJoint(const char* name)          { return FindJoint(tEntity::MakeUID(name)); }
    int    FindJointIndex(const tUID uid) const;
    int    FindJointIndex(const char* name) const    { return FindJointIndex(tEntity::MakeUID(name)); }

    tPose* NewPose();

    void Rebuild();

protected:
    tSkeleton();
    virtual ~tSkeleton();

    tArray<Joint> joint;

    friend class tSkeletonLoader;
};


//-------------------------------------------------------------------
class tSkeletonLoader : public tSimpleChunkHandler
{
public:
    tSkeletonLoader();
	// Michael Riegger - moved to public from protected
    tEntity* LoadObject(tChunkFile*, tEntityStore* store);
protected:
    ~tSkeletonLoader() {};

};

#endif /* _SKELETON_HPP */

