/*===========================================================================
    drawablepose.cpp
    9-Dec-99 Created by Neall

    Copyright (c)1999 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#include <p3d/anim/drawablepose.hpp>
#include <p3d/anim/pose.hpp>
#include <p3d/anim/skeleton.hpp>
#include <p3d/error.hpp>

tDrawablePose::~tDrawablePose()
{
    if(pose)
    {
        pose->Release();
    }
}

void tDrawablePose::Display()
{
    if(!pose)
    {
        GetPose();
    }

    if(pose)
    {
        Display(pose);
    }
}

void tDrawablePose::SetPose(tPose* p)
{
    tRefCounted::Assign(pose, p);
}

tPose* tDrawablePose::GetPose() 
{
    if(!pose)
    {
        P3DASSERT(skeleton);
        pose = skeleton->NewPose();
        pose->AddRef();
    }

    return pose;
}

