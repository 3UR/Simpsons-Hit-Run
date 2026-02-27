/*===========================================================================
    pose.cpp
    30-Nov-99 Created by Neall

    Copyright (c)1999, 2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#include <p3d/anim/pose.hpp>
#include <p3d/anim/skeleton.hpp>
#include <p3d/error.hpp>
 
using namespace RadicalMathLibrary;

//-------------------------------------------------------------------
void ComputeJointMatrix(const Matrix* rootMatrix, tPose::Joint* j)
{
    if(j->parent == NULL)
    {
        j->ConcatMatrix(rootMatrix);
    }
    else
    {
        ComputeJointMatrix(rootMatrix, j->parent);
        j->ConcatMatrix(&j->parent->objectMatrix);
    }
}

//-------------------------------------------------------------------
tPose::tPose(tSkeleton* s) : skeleton(s)
{
    skeleton->AddRef();
    nRealJoint = nJoint = skeleton->GetNumJoint();
    joint = new Joint[nJoint];

    ResetToRestPose();
    poseReady = false;
    updateCount = 0;
}

tPose::tPose(int jointCount) 
{
    skeleton = NULL;
    nJoint = 0;
    nRealJoint = jointCount;
    joint = new Joint[nRealJoint];

    poseReady = false;
    updateCount = 0;
}

tPose::~tPose()
{
    tRefCounted::Release(skeleton);
    delete[] joint;
}

void tPose::SetSkeleton(tSkeleton* s)
{
    tRefCounted::Assign(skeleton, s);
    if(skeleton)
    {
        nJoint = skeleton->GetNumJoint();
        P3DASSERT(nJoint <= nRealJoint);
    }
    else
    {
        nJoint = 0;
    }
}

tPose::Joint* tPose::FindJoint(const tUID uid) const
{
    P3DASSERT(skeleton);
    for(int i=0; i < nJoint; i++)
    {
        if(joint[i].uid == uid)
        {
            return &joint[i];
        }
    }
    return NULL;
}

int tPose::FindJointIndex(const tUID uid) const
{
    P3DASSERT(skeleton);
    for(int i=0; i < nJoint; i++)
    {
        if(joint[i].uid == uid)
        {
            return i;
        }
    }
    return -1;
}

void tPose::Evaluate(const Matrix* root)
{
    P3DASSERT(nJoint);
    if(!poseReady)
    {
        if(root)
        {
            joint[0].ConcatMatrix(root);
        }
        else
        {
            joint[0].worldMatrix = joint[0].objectMatrix;
        }

        for(int i=1; i < nJoint; i++)
        {
            joint[i].ConcatMatrix(&joint[i].parent->worldMatrix);
        }
        ++updateCount;
    }
    poseReady = true;
}

void tPose::EvaluateJointMatrix(const tUID uid, const Matrix* root, Matrix* matrix)
{
    Joint* leaf = FindJoint(uid);
    P3DASSERT(leaf);

    if(!poseReady)
    {
        if(root)
        {
            ComputeJointMatrix(root, leaf);
        }
        else
        {
            Matrix rootMatrix;
            rootMatrix.Identity();
            ComputeJointMatrix(&rootMatrix, leaf);
        }
        ++updateCount;
    }
    *matrix = leaf->worldMatrix;
}

void tPose::EvaluateJointPosition(const tUID uid, const Matrix* root, Vector* position)
{
    Matrix matrix;
    EvaluateJointMatrix(uid, root, &matrix);
    *position = matrix.Row(3);
}


void tPose::EvaluateJointMatrix(unsigned index, const Matrix* root)
{
   Joint* j = &joint[index];
   if(j->parent == NULL)
   {
      joint[index].ConcatMatrix(root);
   }
   else
   {
      joint[index].ConcatMatrix(&j->parent->worldMatrix);
   }
}

void tPose::ResetToRestPose()
{
    P3DASSERT(skeleton != NULL);
    P3DASSERT(nJoint > 0);
    for(int i=0; i < nJoint; i++)
    {
        tSkeleton::Joint* sj = skeleton->GetJoint(i);
        joint[i].uid = sj->uid;
        joint[i].parent = &joint[sj->parentIndex];
        joint[i].objectMatrix = sj->restPose;
        joint[i].worldMatrix.Identity();
    }
    joint[0].parent = NULL;
    ++updateCount;
 }

