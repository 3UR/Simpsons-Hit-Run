//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _NODEANIMATION_HPP
#define _NODEANIMATION_HPP

#include <p3d/anim/animate.hpp>

namespace Scenegraph
{

class Transform;

//*********************************************************
// Class TransformAnimationController
//*********************************************************
class TransformAnimationController : public tBlendFrameController
{
public:
    TransformAnimationController();
    TransformAnimationController(TransformAnimationController* c);

    virtual tFrameController* Clone(void);

    void SetTransformNode(Transform* transform);
    Transform* GetTransformNode() { return transform; }

    virtual bool ValidateAnimation(tAnimation* anim);

protected:
    virtual ~TransformAnimationController();

    void UpdateNoBlending();
    void UpdateWithBlending();

    Transform* transform;
};

}

#endif 

