//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include<p3d/anim/instancedgeometry.hpp>
#include<p3d/anim/polyskin.hpp>
#include<p3d/anim/pose.hpp>
#include<p3d/anim/skeleton.hpp>
#include<p3d/primgroup.hpp>
#include<p3d/utility.hpp>
#include<p3d/matrixstack.hpp>


static tInstancedGeometry* list = NULL;

tInstancedGeometry::tInstancedGeometry(tPolySkin* s)
{
    skin = s;
    pose = s->GetSkeleton()->NewPose();

    skin->AddRef();
    pose->AddRef();

    curCount = 0;
    maxCount = s->GetPrimGroupSkinned(0)->GetInstanceCount();

    next = list;
    prev = NULL;

    if(next)
        next->prev = this;

    list = this;
}

tInstancedGeometry::~tInstancedGeometry()
{
    skin->Release();
    pose->Release();

    if(next)
        next->prev = prev;

    if(prev) 
        prev->next = next;
    else
        list = next;
}

void tInstancedGeometry::Display(void)
{
    if((curCount + 1) == maxCount)
    {
        Flush();
    }

    pose->GetJoint(curCount)->worldMatrix = *p3d::stack->GetMatrix();
    curCount++;
}

void tInstancedGeometry::Flush(void)
{
    if(curCount == 0)
        return;

    pose->SetPoseReady(true);
    p3d::stack->PushIdentity();

    skin->DisplayInstanced(pose, curCount);

    p3d::stack->Pop();
    curCount = 0;
}

void tInstancedGeometry::FlushAll(void)
{
    tInstancedGeometry* walk = list;

    while(walk)
    {
        walk->Flush();
        walk = walk->next;
    }
}

void tInstancedGeometry::ProcessShaders(ShaderCallback& callback)
{
    skin->ProcessShaders(callback);
}
