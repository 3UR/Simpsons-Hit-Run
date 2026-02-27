/*===========================================================================
    drawablepose.hpp
    9-Dec-99 Created by Neall

    Copyright (c)1999 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _DRAWABLEPOSE_HPP
#define _DRAWABLEPOSE_HPP

#include <p3d/drawable.hpp>
#include <p3d/anim/pose.hpp>

class tDrawablePose : public tDrawable, public tPosable
{
public:
    tDrawablePose() : skeleton(NULL), pose(NULL) { /**/ }

    virtual void Display();
    virtual void Display(tPose* p) = 0;

    virtual void SetPose(tPose* p);
    virtual tPose* GetPose();

    virtual tSkeleton* GetSkeleton() { return skeleton; }

protected:
    virtual ~tDrawablePose();

    tSkeleton* skeleton;
    tPose* pose;
};

#endif /* _DRAWABLEPOSE_HPP */

