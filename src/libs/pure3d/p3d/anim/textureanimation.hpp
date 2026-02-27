//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _TEXTUREANIMATION_HPP_
#define _TEXTUREANIMATION_HPP_

#include <p3d/anim/animate.hpp>

class tShader;

//*********************************************************
// Class tTextureAnimationController
//*********************************************************
class tTextureAnimationController : public tSimpleFrameController
{
public:
    tTextureAnimationController();
    tTextureAnimationController(tTextureAnimationController* c);

    void SetShader(tShader* Shader);
    tShader* Shader(void) { return mShader; }

    virtual tFrameController* Clone(void);
    virtual bool ValidateAnimation(tAnimation* anim);

protected:
    virtual ~tTextureAnimationController();
    virtual void Update(void);

    tShader *mShader;
};

#endif // _TEXTUREANIMATION_HPP_

