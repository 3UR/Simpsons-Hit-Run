//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/anim/billboardobjectanimation.hpp>
#include <p3d/anim/channel.hpp>
#include <p3d/billboardobject.hpp>
#include <radmath/radmath.hpp>

#include <p3d/error.hpp>

using namespace RadicalMathLibrary;

//*********************************************************
// Class tBillboardQuadGroupAnimationController
//*********************************************************
tBillboardQuadGroupAnimationController::tBillboardQuadGroupAnimationController() : 
    tSimpleFrameController(),
    mQuadGroup(NULL)
{
}

//---------------------------------------------------------
tBillboardQuadGroupAnimationController::tBillboardQuadGroupAnimationController(tBillboardQuadGroupAnimationController* c) :
    tSimpleFrameController(c),
    mQuadGroup(NULL)
{
    SetBillboardQuadGroup(c->mQuadGroup);
}

//---------------------------------------------------------
tBillboardQuadGroupAnimationController::~tBillboardQuadGroupAnimationController()
{
    tRefCounted::Release(mQuadGroup);
}

//---------------------------------------------------------
tFrameController* tBillboardQuadGroupAnimationController::Clone(void)
{
    return new tBillboardQuadGroupAnimationController(this);
}

//---------------------------------------------------------
void tBillboardQuadGroupAnimationController::SetBillboardQuadGroup(tBillboardQuadGroup* group)
{
    tRefCounted::Assign(mQuadGroup, group);
}

//---------------------------------------------------------
tBillboardQuadGroup* tBillboardQuadGroupAnimationController::GetBillboardQuadGroup()
{
    return mQuadGroup;
}

//---------------------------------------------------------
bool tBillboardQuadGroupAnimationController::ValidateAnimation(tAnimation* anim)
{
    return (anim->GetAnimationType()==Pure3DAnimationTypes::BILLBOARD_QUAD_GROUP_BQG);
}

//---------------------------------------------------------
void tBillboardQuadGroupAnimationController::Update()
{
    // If there is no destination, why do the work?
    if (mQuadGroup == NULL) 
        return;

    float frame = animation->MakeValidFrame(GetFrame(), minFrame, maxFrame, cycleMode);

    const tBoolChannel*   visibility = NULL;
    const tVectorChannel* translation = NULL;
    const tFloat1Channel* width = NULL;
    const tFloat1Channel* height = NULL;
    const tFloat1Channel* distance = NULL;
    const tColourChannel* colour = NULL;
    const tFloat2Channel* uvOffset = NULL;
    const tFloat2Channel* uvOffsetRange = NULL;
    const tFloat1Channel* sourceRange = NULL;
    const tFloat1Channel* edgeRange = NULL;

    for (int i = 0; i < animation->GetNumGroups(); i++)
    {
        const tAnimationGroup* animationGroup = animation->GetGroupByIndex(i);
        if(animationGroup)
        {
            tBillboardQuad* quad = mQuadGroup->FindQuadByUID(animationGroup->GetUID());
            if (quad)
            {
                visibility = animationGroup->GetBoolChannel(Pure3DAnimationChannels::BillboardObjects::VISIBILITY_VIS);
                translation = animationGroup->GetVectorChannel(Pure3DAnimationChannels::BillboardObjects::TRANSLATION_TRAN);
                width = animationGroup->GetFloat1Channel(Pure3DAnimationChannels::BillboardObjects::WIDTH_WDT);
                height = animationGroup->GetFloat1Channel(Pure3DAnimationChannels::BillboardObjects::HEIGHT_HGT);
                distance = animationGroup->GetFloat1Channel(Pure3DAnimationChannels::BillboardObjects::DISTANCE_DIST);
                colour = animationGroup->GetColourChannel(Pure3DAnimationChannels::BillboardObjects::COLOUR_CLR);
                uvOffset = animationGroup->GetFloat2Channel(Pure3DAnimationChannels::BillboardObjects::UVOFFSET_OFF);
                uvOffsetRange = animationGroup->GetFloat2Channel(Pure3DAnimationChannels::BillboardObjects::UVOFFSET_RANGE_ORNG);
                sourceRange = animationGroup->GetFloat1Channel(Pure3DAnimationChannels::BillboardObjects::SOURCE_RANGE_SRNG);
                edgeRange = animationGroup->GetFloat1Channel(Pure3DAnimationChannels::BillboardObjects::EDGE_RANGE_ERNG);

                if (visibility)
                {
                    bool v;
                    visibility->GetValue(frame, &v);
                    quad->SetVisibility(v);
                }
                if (translation)
                {
                    Vector v;
                    translation->GetValue(frame, &v);
                    quad->SetTranslation(v);
                }
                if (width)
                {
                    float w;
                    width->GetValue(frame,&w);
                    quad->SetWidth(w);
                }
                if (height)
                {
                    float h;
                    height->GetValue(frame,&h);
                    quad->SetHeight(h);
                }
                if (colour)
                {
                    tColour c;
                    colour->GetValue(frame,&c);
                    quad->SetColour(c);
                }
                if (uvOffset)
                {
                    Vector2 o;
                    uvOffset->GetValue(frame,&o);
                    quad->SetUVOffset(o);
                }
                if (distance)
                {
                    float d;
                    distance->GetValue(frame,&d);
                    quad->SetDistance(d);
                }
                if (uvOffsetRange)
                {
                    Vector2 o;
                    uvOffsetRange->GetValue(frame,&o);
                    quad->SetUVOffsetRange(o);
                }
                if (sourceRange)
                {
                    float r;
                    sourceRange->GetValue(frame,&r);
                    quad->SetSourceRange(r);
                }
                if (edgeRange)
                {
                    float r;
                    edgeRange->GetValue(frame,&r);
                    quad->SetEdgeRange(r);
                }
            }
        }
    }
}




