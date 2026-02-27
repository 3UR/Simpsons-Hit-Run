//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _CAMERAANIM_HPP_
#define _CAMERAANIM_HPP_

#include <p3d/anim/animate.hpp>

class tVectorCamera;

//*********************************************************
// Class tCameraAnimationController
//*********************************************************
class tCameraAnimationController : public tBlendFrameController
{
public:
    tCameraAnimationController();
    tCameraAnimationController(tCameraAnimationController* c);

    void SetCamera(tVectorCamera* camera);
    tVectorCamera* Camera(void) { return mCamera; }

    virtual bool ValidateAnimation(tAnimation* anim);

    virtual tFrameController* Clone(void);
    void SetOffsetMatrix( const rmt::Matrix& m );

protected:
    virtual ~tCameraAnimationController();

    void UpdateNoBlending();
    void UpdateWithBlending();

    tVectorCamera* mCamera;
    rmt::Matrix mOffsetMatrix;
};

#endif // _CAMERAANIM_HPP_

