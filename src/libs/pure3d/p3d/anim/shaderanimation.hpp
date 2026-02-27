//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _SHADERANIMATION_HPP_
#define _SHADERANIMATION_HPP_

#include <p3d/anim/animate.hpp>

class tShader;

//*********************************************************
// Class tShaderAnimationController
//*********************************************************
class tShaderAnimationController : public tBlendFrameController
{
public:
    tShaderAnimationController();
    tShaderAnimationController(tShaderAnimationController* c);

    void SetShader(tShader *shader);
    tShader *Shader(void) { return mShader; }

    virtual bool ValidateAnimation(tAnimation *anim);

    virtual tFrameController *Clone(void);

protected:
    virtual ~tShaderAnimationController();

    void UpdateNoBlending();
    void UpdateWithBlending();

    tShader *mShader;
};

#endif // _SHADERANIMATION_HPP_

