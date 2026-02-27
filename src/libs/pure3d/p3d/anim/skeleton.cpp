/*===========================================================================
    skeleton.cpp
    30-Nov-99 Created by Neall

    Copyright (c)1999 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#include <p3d/anim/skeleton.hpp>
#include <p3d/anim/pose.hpp>
#include <p3d/file.hpp>
#include <constants/chunks.h>
#include <p3d/chunkfile.hpp>
#include <p3d/inventory.hpp>
#include <p3d/error.hpp>
#include <p3d/utility.hpp>

tSkeleton::tSkeleton(int nj) 
{
    joint.SetSize( nj );
}

tSkeleton::~tSkeleton()
{
}

tSkeleton::Joint* tSkeleton::FindJoint(const tUID uid) const
{
    for(unsigned i=0; i < joint.Size(); i++)
    {
        if(joint[i].uid == uid)
        {
            return &joint[i];
        }
    }
    return NULL;
}

int tSkeleton::FindJointIndex(const tUID uid) const
{
    for(unsigned i=0; i < joint.Size(); i++)
    {
        if(joint[i].uid == uid)
        {
            return(i);
        }
    }
    return -1;
}

tPose* tSkeleton::NewPose()
{
    return new tPose(this);
}

void tSkeleton::Rebuild()
{
    // build the bone->world matrices, and their inverses
    // based upon the rest-pose local matrix
    joint[0].worldMatrix = joint[0].restPose;
    joint[0].inverseWorldMatrix.InvertOrtho(joint[0].restPose);

    for(unsigned i = 1; i < joint.Size(); i++)
    {
        joint[i].worldMatrix.Mult(joint[i].restPose, joint[joint[i].parentIndex].worldMatrix);
        joint[i].inverseWorldMatrix.InvertOrtho(joint[i].worldMatrix);
    }
}

//-------------------------------------------------------------------
static const int SKELETON_VERSION = 0;

tSkeletonLoader::tSkeletonLoader() : tSimpleChunkHandler(P3D_SKELETON)
{
}

tEntity* tSkeletonLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    char name[128];
    f->GetPString(name);
    int ver = f->GetLong();
    P3DASSERT(ver == SKELETON_VERSION);

    int nJoint = f->GetLong();

    if(nJoint < 1)
    {
        p3d::printf("tSkeletonLoader - skipping skeleton: '%s' because nJoints < 1\n", name );
        return NULL;
    }

    tSkeleton* skeleton = new tSkeleton(nJoint);
    skeleton->SetName(name);

    int index = 0;   
    
    while(f->ChunksRemaining())
    {
        f->BeginChunk();
        if(f->GetCurrentID() == P3D_SKELETON_JOINT)
        {
            P3DASSERT(index < nJoint);
            char name[128];
            f->GetPString(name);
            skeleton->joint[index].uid = tEntity::MakeUID(name);
            skeleton->joint[index].parentIndex = f->GetLong();
            skeleton->joint[index].dof = f->GetLong();
            f->GetLong(); // free zxes
            f->GetLong(); // primary axis
            f->GetLong(); // secondary axis
            f->GetLong(); // twist axis
            f->GetData(&(skeleton->joint[index].restPose), 16, tFile::DWORD);
            //
            // Now build the restPose YZX Euler angles
            //
            skeleton->joint[index].restPoseYZXEuler.ConvertToEulerYZX(skeleton->joint[index].restPose);

            //
            // New Data so setup the defaults before checking for 
            // modifying data chucks
            //
            skeleton->joint[index].mappedJointIndex = index;
            skeleton->joint[index].xAxisMap = 1.0f;
            skeleton->joint[index].yAxisMap = 1.0f;
            skeleton->joint[index].zAxisMap = 1.0f;
            skeleton->joint[index].preserveBoneLengths = false;
            //
            // Look for subchunks
            //
            while(f->ChunksRemaining())
            {
                f->BeginChunk();
                unsigned int subChunkID = f->GetCurrentID();
                switch(subChunkID)
                {
                    case P3D_SKELETON_JOINT_FIX_FLAG:
                    {
                        bool preserve = f->GetLong()?true:false;
                        
                        skeleton->joint[index].preserveBoneLengths = preserve;
                    }
                    break;
                    case P3D_SKELETON_JOINT_MIRROR_MAP:
                    {
                        skeleton->joint[index].mappedJointIndex = f->GetLong();
                        skeleton->joint[index].xAxisMap = f->GetFloat();
                        skeleton->joint[index].yAxisMap = f->GetFloat();
                        skeleton->joint[index].zAxisMap = f->GetFloat();
                    }
                    break;
                    default:
                        break;
                }
                f->EndChunk();
            }// end subchunk tests
            index++;
        }
        f->EndChunk();
    }
    skeleton->Rebuild();
    return skeleton;
}

