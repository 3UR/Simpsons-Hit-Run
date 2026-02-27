//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _BILLBOARD_OBJECT_ANIM_HPP_
#define _BILLBOARD_OBJECT_ANIM_HPP_

#include <p3d/anim/animate.hpp>

class tBillboardQuadGroup;

//*********************************************************
// Class tBillboardQuadGroupAnimationController
//*********************************************************
class tBillboardQuadGroupAnimationController : public tSimpleFrameController
{
public:
    tBillboardQuadGroupAnimationController();
    tBillboardQuadGroupAnimationController(tBillboardQuadGroupAnimationController* c);

    void SetBillboardQuadGroup(tBillboardQuadGroup* group);
    tBillboardQuadGroup* GetBillboardQuadGroup(void);

    virtual bool ValidateAnimation(tAnimation* anim);

    virtual tFrameController* Clone(void);

protected:
    virtual ~tBillboardQuadGroupAnimationController();

    void Update();

    tBillboardQuadGroup* mQuadGroup;
};

#endif // _BILLBOARD_OBJECT_ANIM_HPP_

