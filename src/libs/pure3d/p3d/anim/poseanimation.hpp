/*===========================================================================
    poseanimation.hpp
    04-Jan-00 Created by Neall

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _POSEANIMATION_HPP
#define _POSEANIMATION_HPP

#include <p3d/anim/animate.hpp>

class tPose;

//*********************************************************
// Class tPoseAnimationController
//*********************************************************
class tPoseAnimationController : public tBlendFrameController
{
public:
    tPoseAnimationController();
    tPoseAnimationController(tPoseAnimationController* c);

    void SetPose(tPose* pose);
    tPose* GetPose() { return pose; }

    virtual bool ValidateAnimation(tAnimation* anim);

    virtual tFrameController* Clone(void);

protected:
    virtual ~tPoseAnimationController();

    void UpdateNoBlending();
    void UpdateWithBlending();

    tPose* pose;

    friend class tPoseAnimationLoader;
};

#endif /* _POSEANIMATION_HPP */

