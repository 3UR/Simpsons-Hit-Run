//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/anim/shaderanimation.hpp>
#include <p3d/anim/channel.hpp>
#include <p3d/shader.hpp>

using namespace RadicalMathLibrary;

//*********************************************************
// Class tShaderAnimationController
//*********************************************************
tShaderAnimationController::tShaderAnimationController() : 
    tBlendFrameController(),
    mShader(NULL) 
{
}

//---------------------------------------------------------
tShaderAnimationController::tShaderAnimationController(tShaderAnimationController *c) :
    tBlendFrameController(c),
    mShader(NULL)
{
    SetShader(c->Shader());
}

//---------------------------------------------------------
tShaderAnimationController::~tShaderAnimationController()
{
    tRefCounted::Release(mShader);
}
 
//---------------------------------------------------------
tFrameController* tShaderAnimationController::Clone(void)
{
    return new tShaderAnimationController(this);
}

//---------------------------------------------------------
void tShaderAnimationController::SetShader(tShader *shader)
{
    tRefCounted::Assign(mShader, shader);
}

//---------------------------------------------------------
bool tShaderAnimationController::ValidateAnimation(tAnimation* anim)
{
    return (anim->GetAnimationType() == Pure3DAnimationTypes::SHADER_SHAD);
}

//---------------------------------------------------------
void tShaderAnimationController::UpdateNoBlending()
{
    if (mShader == NULL) return;

    tAnimation* anim = playInfo[0].GetAnimation();
    float frame = playInfo[0].GetCurrentFrame();

    int group;
    for (group = 0; group < anim->GetNumGroups(); group++)
    {

        const tAnimationGroup *animGroup = anim->GetGroupByIndex(group);

        int index;
        for (index = 0; index < animGroup->GetChannelCount(); index++)
        {
            const tChannel *channel = animGroup->GetChannel(index);
            if (channel == NULL) continue;


            switch (channel->GetDataType())
            {
                // Nothing to do with these for shaders
                case tChannel::UNASSIGNED:
                case tChannel::STRING:
                case tChannel::FLOAT2:
                case tChannel::EVENT:
                    break;


                case tChannel::INT:
                {
                    const tIntChannel *c = (tIntChannel *)channel;
                    int val;
                    c->GetValue(frame, &val);
                    mShader->SetInt(c->GetChannelCode(), val);
                    break;
                }

                case tChannel::FLOAT1:
                {
                    const tFloat1Channel *c = (tFloat1Channel *)channel;
                    float val;
                    c->GetValue(frame, &val);
                    mShader->SetFloat(c->GetChannelCode(), val);
                    break;
                }

                case tChannel::VECTOR:
                {
                    const tVectorChannel *c = (tVectorChannel *)channel;
                    rmt::Vector val;
                    c->GetValue(frame, &val);
                    mShader->SetVector(c->GetChannelCode(), val);
                    break;
                }

                case tChannel::ROTATION:
                {
                    const tRotationChannel *c = (tRotationChannel *)channel;
                    rmt::Matrix val;
                    c->GetMatrix(frame, &val);
                    mShader->SetMatrix(c->GetChannelCode(), val);
                    break;
                }

                case tChannel::ENTITY:
                {
                    const tEntityChannel *c = (tEntityChannel *)channel;
                    tEntity *val;
                    c->GetValue(frame, &val);
                    tTexture *tex = dynamic_cast<tTexture *>(val);
                    if (tex != NULL)
                    {
                        mShader->SetTexture(c->GetChannelCode(), tex);
                    }
                    break;
                }

                case tChannel::BOOL:
                {
                    const tBoolChannel *c = (tBoolChannel *)channel;
                    bool val;
                    c->GetValue(frame, &val);
                    mShader->SetInt(c->GetChannelCode(), val);
                    break;
                }

                case tChannel::COLOUR:
                {
                    const tColourChannel *c = (tColourChannel *)channel;
                    tColour val;
                    c->GetValue(frame, &val);
                    mShader->SetColour(c->GetChannelCode(), val);
                    break;
                }
            }
        }
    }
}

//---------------------------------------------------------
void tShaderAnimationController::UpdateWithBlending()
{
    UpdateNoBlending();
}

