//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _LIGHTANIMATION_HPP_
#define _LIGHTANIMATION_HPP_

#include <p3d/anim/animate.hpp>

class tLight;

//*********************************************************
// Class tLightAnimationController
//*********************************************************
class tLightAnimationController : public tBlendFrameController
{
public:
    tLightAnimationController();
    tLightAnimationController(tLightAnimationController* c);

    void SetLight(tLight* light);
    tLight* Light(void) { return mLight; }

    virtual bool ValidateAnimation(tAnimation* anim);

    virtual tFrameController* Clone(void);

protected:
    virtual ~tLightAnimationController();

    void UpdateNoBlending();
    void UpdateWithBlending();

    // "Spot" enum entry Added by Bryan Ewert on 19 Dec 2001
    enum tLightType 
    {
        Invalid = -1,  
        Ambient, 
        Point,
        Directional, 
        Spot
    };

    tLight*     mLight;
    tLightType  mLightType;
};

#endif // _LIGHTANIMATION_HPP_

