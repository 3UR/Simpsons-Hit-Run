//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/scenegraph/nodeanimation.hpp>
#include <p3d/scenegraph/scenegraph.hpp>
#include <p3d/anim/channel.hpp>
#include <p3d/utility.hpp>
#include <radmath/radmath.hpp>

#include <string.h>

using namespace RadicalMathLibrary;

namespace Scenegraph
{

//*********************************************************
// Class TransformAnimationController
//*********************************************************
TransformAnimationController::TransformAnimationController() : 
    tBlendFrameController(),
    transform(NULL)
{
}

//-------------------------------------------------------------------
TransformAnimationController::TransformAnimationController(TransformAnimationController* c) :
    tBlendFrameController(c),
    transform(NULL)
{
    SetTransformNode(c->GetTransformNode());   
}

//-------------------------------------------------------------------
TransformAnimationController::~TransformAnimationController()
{
    tRefCounted::Release(transform);
}

//-------------------------------------------------------------------
void TransformAnimationController::SetTransformNode(Transform* t)
{
    tRefCounted::Assign(transform, t);
}

//-------------------------------------------------------------------
tFrameController* TransformAnimationController::Clone(void)
{
    return new TransformAnimationController(this);
}

//-------------------------------------------------------------------
bool TransformAnimationController::ValidateAnimation(tAnimation* anim)
{
    return (anim->GetAnimationType() == Pure3DAnimationTypes::SCENEGRAPH_TRANSFORM_STRN);
}

//-------------------------------------------------------------------
void TransformAnimationController::UpdateNoBlending()
{
    if (transform == NULL)
    {
        return;
    }

    tAnimation* anim = playInfo[0].GetAnimation();
    const tAnimationGroup* animGroup = anim->GetGroupByIndex(0);
    
    P3DASSERT(animGroup);

    float frame = playInfo[0].GetCurrentFrame();

    const tVectorChannel* translation = animGroup->GetVectorChannel(Pure3DAnimationChannels::SceneGraph::TRANSLATION_TRAN);
    const tRotationChannel* rotation = animGroup->GetRotationChannel(Pure3DAnimationChannels::SceneGraph::ROTATION_ROT);

    if(translation)
    {
        translation->GetValue(frame, &(transform->ModifyTransform()->Row(3)));
    }
    if(rotation)
    {
        rotation->GetMatrix(frame, transform->ModifyTransform());
    }
}

//-------------------------------------------------------------------
void TransformAnimationController::UpdateWithBlending()
{   
    if (transform == NULL)
    {
        return;
    }

    tAnimation *anim = playInfo[0].GetAnimation();
    const tAnimationGroup* animGroup = anim->GetGroupByIndex(0);

    P3DASSERT(animGroup);

    float frame = playInfo[0].GetCurrentFrame();

    Vector vBlend(0.0f, 0.0f, 0.0f);
    Quaternion qBlend(0.0f, 0.0f, 0.0f, 1.0f);

    const tVectorChannel* translation = animGroup->GetVectorChannel(Pure3DAnimationChannels::SceneGraph::TRANSLATION_TRAN);
    const tRotationChannel* rotation = animGroup->GetRotationChannel(Pure3DAnimationChannels::SceneGraph::ROTATION_ROT);

    // prime the accumulation buffer by evaluating the first animation in the buffer
    if (translation)
    {
        translation->GetValue(frame, &vBlend);
    }

    if (rotation)
    {
        rotation->GetQuaternion(frame, &qBlend);
    }
    
    // blend in remaining animations using the blend weight
    for(int j=1; j < nAnim; j++)
    {
        anim = playInfo[j].GetAnimation();
        animGroup = anim->GetGroupByIndex(0);

        P3DASSERT(animGroup);

        float frame = playInfo[j].GetCurrentFrame();

        translation = animGroup->GetVectorChannel(Pure3DAnimationChannels::SceneGraph::TRANSLATION_TRAN);
        rotation = animGroup->GetRotationChannel(Pure3DAnimationChannels::SceneGraph::ROTATION_ROT);

        if (translation)
        {
            Vector v;
            vBlend.Scale(1.0f - playInfo[j].GetWeight());
            translation->GetValue(frame, &v);
            vBlend.ScaleAdd(playInfo[j].GetWeight(), v);
        }

        if (rotation)
        {
            Quaternion q;
            rotation->GetQuaternion(frame, &q);
            qBlend.Slerp(q, playInfo[j].GetWeight());
        }
    }

    // stuff the resulting blended translation vector into the pose
    transform->ModifyTransform()->Row(3) = vBlend;

    // stuff the blended rotation quaternion into the pose
    qBlend.ConvertToMatrix(transform->ModifyTransform());
}

}
