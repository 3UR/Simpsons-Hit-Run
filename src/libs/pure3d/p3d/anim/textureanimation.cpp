//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/anim/textureanimation.hpp>
#include <p3d/anim/channel.hpp>
#include <p3d/utility.hpp>
#include <p3d/error.hpp>
#include <p3d/shader.hpp>
#include <p3d/texture.hpp>

//*********************************************************
// Class tTextureAnimationController
//*********************************************************
tTextureAnimationController::tTextureAnimationController() : 
    tSimpleFrameController(),
    mShader(NULL)
{
}

//---------------------------------------------------------
tTextureAnimationController::tTextureAnimationController(tTextureAnimationController* c) :  
    tSimpleFrameController(c),
    mShader(NULL)
{
    SetShader(c->Shader());
}

//---------------------------------------------------------
tTextureAnimationController::~tTextureAnimationController()
{
    tRefCounted::Release(mShader);
}

//---------------------------------------------------------
tFrameController* tTextureAnimationController::Clone(void)
{
    return new tTextureAnimationController(this);
}

//---------------------------------------------------------
void tTextureAnimationController::SetShader(tShader *mat)
{
    tRefCounted::Assign(mShader, mat);
}

//---------------------------------------------------------
bool tTextureAnimationController::ValidateAnimation(tAnimation* anim)
{
    return (anim->GetAnimationType()==Pure3DAnimationTypes::TEXTURE_TEX);
}

//---------------------------------------------------------
void tTextureAnimationController::Update()
{
    if (mShader == NULL) return;
    if (animation == NULL) return;

    float pframe = animation->MakeValidFrame(frame,minFrame,maxFrame,cycleMode);

    const tAnimationGroup *animGroup = animation->GetGroupByIndex(0);

    if (animGroup)
    {
        const tEntityChannel *textures = animGroup->GetEntityChannel(Pure3DAnimationChannels::Texture::TEXTURE_TEX);

        if (textures)
        {
            tEntity *texture;
            textures->GetValue(pframe, &texture);
            mShader->SetTexture(PDDI_SP_BASETEX, (tTexture *)texture);
        }
    }
}
