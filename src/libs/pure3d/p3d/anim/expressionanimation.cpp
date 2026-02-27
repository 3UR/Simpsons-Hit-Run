/*=============================================================================
  expressionanimation.cpp
  29-Mar-2000
  Created by: Katrina

  Description: Pure3D expression classes for animation of
                    parameterize facial expressions

  Copyright (c) 1997-2000 Radical Entertainment, Inc.
  All Rights Reserved

=============================================================================*/

#include <string.h>

#include <p3d/anim/expressionanimation.hpp>
#include <p3d/anim/expression.hpp>
#include <p3d/anim/animate.hpp>
#include <p3d/anim/channel.hpp>
#include <p3d/error.hpp>

//**************************************************************
// Class tExpressionAnimationController
//**************************************************************
tExpressionAnimationController::tExpressionAnimationController() :
    tBlendFrameController(),
    mixer(NULL),
    expStateAccumulator(NULL)
{
}

//--------------------------------------------------------------
tExpressionAnimationController::tExpressionAnimationController(tExpressionAnimationController* c) :
    tBlendFrameController(c),
    mixer(NULL),
    expStateAccumulator(NULL)
{
    SetTargetMixer(c->GetTargetMixer());
}

//--------------------------------------------------------------
tExpressionAnimationController::~tExpressionAnimationController()
{
    tRefCounted::Release(mixer);

    if (expStateAccumulator != NULL)
    {
        delete [] expStateAccumulator;
    }
}

//--------------------------------------------------------------
tFrameController* tExpressionAnimationController::Clone(void)
{
    return new tExpressionAnimationController(this);
}

//--------------------------------------------------------------
bool tExpressionAnimationController::ValidateAnimation(tAnimation* anim)
{
    bool isExpAnim = (anim->GetAnimationType() == Pure3DAnimationTypes::EXPRESSION_EXP);
    bool allExpsPresent = true;

    if (isExpAnim)
    {
        // verify that all channels in the animation are defined in the mixer's expression group
        P3DASSERT(mixer != NULL);

        tExpressionGroup* eg = mixer->GetExpressionGroup();
        P3DASSERT(eg != NULL);

        for (int i = 0; i < anim->GetNumGroups(); i++)
        {
            tExpression* exp = NULL;
            int expIndex = -1;
            
            exp = eg->FindExpression(anim->GetGroupByIndex(i)->GetNameObject(), &expIndex);
            if ((exp == NULL) || (expIndex == -1))
            {
                allExpsPresent = false;
            }
        }
    }
    return (isExpAnim && allExpsPresent);
}

//--------------------------------------------------------------
void tExpressionAnimationController::SetTargetMixer(tExpressionMixer* target)
{
    delete [] expStateAccumulator;
    expStateAccumulator = NULL;

    tRefCounted::Assign(mixer, target);

    // set up the accumulator arrays for blending
    int nExp = mixer->GetNumExpression();

    P3DASSERT(nExp != 0);

    expStateAccumulator = new float[nExp];

    ResetAccumulators();
}

//--------------------------------------------------------------
tExpressionMixer* tExpressionAnimationController::GetTargetMixer() const
{
    return mixer;
}

//--------------------------------------------------------------
void tExpressionAnimationController::UpdateNoBlending()
{
    if (mixer == NULL)
    {
        return;
    }

    tAnimation* anim = playInfo[0].GetAnimation();
    float frame = playInfo[0].GetCurrentFrame();

    int nChannels = anim->GetNumGroups();
    for (int i = 0; i < nChannels; i++)
    {
        const tAnimationGroup* animGroup = anim->GetGroupByIndex(i);

        if (animGroup)
        {
            const tFloat1Channel* stateChannel = animGroup->GetFloat1Channel(Pure3DAnimationChannels::Expression::STATE_STE);

            if (stateChannel)
            {
                float value;
                stateChannel->GetValue(frame, &value);
                mixer->SetExpressionState(animGroup->GetNameObject(), value);
            }
        }
    }

    mixer->Update();
}

//--------------------------------------------------------------
void tExpressionAnimationController::UpdateWithBlending()
{
    if (mixer == NULL)
    {
        return;
    }

    ResetAccumulators();

    tAnimation* anim = playInfo[0].GetAnimation();

    tExpressionGroup* eg = mixer->GetExpressionGroup();
    P3DASSERT(eg);

    int nChannel = anim->GetNumGroups();
    int i;
    for(i = 0; i < nChannel; i++)
    {
        const tAnimationGroup* animGroup = anim->GetGroupByIndex(i);

        if (animGroup)
        {
            const tFloat1Channel* stateChannel = animGroup->GetFloat1Channel(Pure3DAnimationChannels::Expression::STATE_STE);
        
            if (stateChannel)
            {
                tExpression* tmp = NULL;
                int expIndex = -1;

                tmp = eg->FindExpression(animGroup->GetUID(), &expIndex);
                P3DASSERT(expIndex != -1);

                float v;
                stateChannel->GetValue(playInfo[0].GetCurrentFrame(), &v);

                expStateAccumulator[expIndex] = v;
            }
        }
    }

    // blend in remaining animations using the blend weight t
    for (int j = 1; j < nAnim; j++)
    {
        anim = playInfo[j].GetAnimation();

        nChannel = anim->GetNumGroups();

        for (i = 0; i < nChannel; i++)
        {
            const tAnimationGroup* animGroup = anim->GetGroupByIndex(i);

            if (animGroup)
            {
                const tFloat1Channel* stateChannel = animGroup->GetFloat1Channel(Pure3DAnimationChannels::Expression::STATE_STE);
             
                if (stateChannel)
                {
                    tExpression* tmp = NULL;
                    int expIndex = -1;

                    tmp = eg->FindExpression(animGroup->GetUID(), &expIndex);
                    P3DASSERT(expIndex != -1);

                    float vBlend;
                    stateChannel->GetValue(playInfo[j].GetCurrentFrame(), &vBlend);

                    float blendWeight = 1.0f - playInfo[j].GetWeight();
                    expStateAccumulator[expIndex] *= blendWeight;
                    expStateAccumulator[expIndex] += vBlend * playInfo[j].GetWeight();
                }
            }
        }
    }

    for (i = 0; i < mixer->GetNumExpression(); i++)
    {
        mixer->SetExpressionState(i, expStateAccumulator[i]);
    }

    mixer->Update();
}

//--------------------------------------------------------------
void tExpressionAnimationController::ResetAccumulators()
{
    for (int i = 0; i < mixer->GetNumExpression(); i++)
    {
        expStateAccumulator[i] = 0.0f;
    }
}


