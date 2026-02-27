/*===========================================================================
    animate.cpp
    27-Mar-96 Created by Neall
    30-Nov-99 version 12

    Copyright (c)1996-1999 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#include <radmath/radmath.hpp>
#include <p3d/anim/animate.hpp>
#include <p3d/error.hpp>
#include <constants/chunks.h>
#include <constants/chunkids.hpp>
#include <p3d/chunkfile.hpp>
#include <p3d/file.hpp>
#include <p3d/anim/animatedobject.hpp>
#include <p3d/anim/billboardobjectanimation.hpp>
#include <p3d/anim/cameraanimation.hpp>
#include <p3d/anim/lightanimation.hpp>
#include <p3d/anim/poseanimation.hpp>
#include <p3d/anim/pose.hpp>
#include <p3d/anim/skeleton.hpp>
#include <p3d/anim/compositedrawable.hpp>
#include <p3d/anim/visibilityanimation.hpp>
#include <p3d/anim/textureanimation.hpp>
#include <p3d/anim/polyskin.hpp>
#include <p3d/anim/channel.hpp>
#include <p3d/anim/expressionanimation.hpp>
#include <p3d/anim/vertexanimobject.hpp>
#include <p3d/anim/vertexanimcontroller.hpp>
#include <p3d/anim/expression.hpp>
#include <p3d/anim/shaderanimation.hpp>
#include <p3d/scenegraph/nodeanimation.hpp>
#include <p3d/effects/effect.hpp>
#include <p3d/utility.hpp>
#include <p3d/vectorcamera.hpp>
#include <p3d/light.hpp>
#include <p3d/shader.hpp>
#include <p3d/billboardobject.hpp>
#include <p3d/geometry.hpp>

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <new>

static const unsigned ANIMATION_VERSION = 0;
static const unsigned ANIMATION_GROUP_VERSION = 0;
static const unsigned ANIMATION_GROUP_LIST_VERSION = 0;
static const unsigned ANIMATION_SIZE_VERSION = 1;
static const unsigned FRAME_CONTROLLER_VERSION = 0;

static const int MAX_FC_HANDLERS = 4;

using namespace RadicalMathLibrary;

int AnimationGroupCompareByGroupID( const void* arg1, const void* arg2 )
{
    tAnimationGroup* a= *((tAnimationGroup**)arg1);
    tAnimationGroup* b= *((tAnimationGroup**)arg2);
    if (a->GetGroupId()<b->GetGroupId())
        return -1;
    if (a->GetGroupId()>b->GetGroupId())
        return 1;
    return 0;
}

int AnimationGroupCompareByUID( const void* arg1, const void* arg2 )
{
    tAnimationGroup* a= *((tAnimationGroup**)arg1);
    tAnimationGroup* b= *((tAnimationGroup**)arg2);
    if (a->GetUID()<b->GetUID())
        return -1;
    if (a->GetUID()>b->GetUID())
        return 1;
    return 0;
}

//**************************************************************
// Class tFrameController
//**************************************************************
tFrameController::tFrameController() :
    tEntity()
{
}

//-------------------------------------------------------------------
tFrameController::tFrameController(tFrameController* c) :
    tEntity()
{
    CopyName(c);
}

//-------------------------------------------------------------------
tFrameController::~tFrameController()
{
}

//**************************************************************
// Class tAnimationFrameController
//**************************************************************
tAnimationFrameController::tAnimationFrameController() : 
    cycleMode(DEFAULT_CYCLE_MODE),
    minFrame(-1.0f), 
    maxFrame(-1.0f),
    relativeSpeed(1.0f)
{ 
}

//-------------------------------------------------------------------
tAnimationFrameController::tAnimationFrameController(tAnimationFrameController* c) :
    tFrameController(c)
{
    SetCycleMode(c->cycleMode);
    SetFrameRange(c->minFrame, c->maxFrame);   
    SetRelativeSpeed(c->relativeSpeed);
}

//-------------------------------------------------------------------
tAnimationFrameController::~tAnimationFrameController()
{
}

//-------------------------------------------------------------------
void tAnimationFrameController::Reset(void)
{
    cycleMode = DEFAULT_CYCLE_MODE;
    relativeSpeed = 1.0F;
    minFrame = -1.0F;
    maxFrame = -1.0F;
}

//**************************************************************
// Class tSimpleFrameController
//**************************************************************
tSimpleFrameController::tSimpleFrameController() :
    frame(0.0f),
    numCycles(0),
    animation(NULL)
{
}

//-------------------------------------------------------------------
tSimpleFrameController::tSimpleFrameController(tSimpleFrameController* c) :
    tAnimationFrameController(c),
    frame(0.0f),
    numCycles(0),
    animation(NULL)
{
    frame = c->frame;
    tRefCounted::Assign(animation,c->animation);
}

//-------------------------------------------------------------------
tSimpleFrameController::~tSimpleFrameController()
{
    tRefCounted::Release(animation);
}

//-------------------------------------------------------------------
void tSimpleFrameController::SetAnimation(tAnimation* anim)
{
    P3DASSERT(ValidateAnimation(anim));
    tRefCounted::Assign(animation,anim);
    SetFrame(0.0f);
    numCycles = 0;
}

//-------------------------------------------------------------------
void tSimpleFrameController::SetAnimation(tAnimation* anim, float startFrame, float blendFrames)
{
    P3DASSERT(ValidateAnimation(anim));
    tRefCounted::Assign(animation,anim);
    SetFrame(startFrame);
    numCycles = 0;
}

//-------------------------------------------------------------------
tAnimation* tSimpleFrameController::GetAnimation()
{
    return animation;
}

//-------------------------------------------------------------------
float tSimpleFrameController::GetNumFrames()
{
    if (animation)
    {
        return animation->GetNumFrames();
    }
    return 0.0f;
}

//-------------------------------------------------------------------
void tSimpleFrameController::SetFrame(float f)
{
    frame = f;
    numCycles = 0;
    if(animation)
    {
        frame = animation->MakeValidFrame(f, minFrame, maxFrame, cycleMode);
    }
}

//-------------------------------------------------------------------
float tSimpleFrameController::GetFrame()
{
    return frame;
}

//-------------------------------------------------------------------
int tSimpleFrameController::LastFrameReached()
{ 
    if (animation)
    {
        float endFrame = maxFrame<0.0f ? animation->GetNumFrames() : maxFrame;
        if ((cycleMode==FORCE_NON_CYCLIC)||((cycleMode==DEFAULT_CYCLE_MODE)&&(!animation->GetCyclic())))
        {
            return (frame>=endFrame) ? 1 : 0;
        }
        else if ((minFrame>=0.0f)&&(minFrame==maxFrame))
        {
            return 1;
        }
        else
        {
            return numCycles; 
        }
    }
    return 0;
}

//-------------------------------------------------------------------
void tSimpleFrameController::Advance(float deltaTime, bool update)
{
    if(animation)
    {
        float deltaFrame = (deltaTime * 0.001f) * animation->GetSpeed() * relativeSpeed;
        float endFrame = maxFrame<0.0f ? animation->GetNumFrames() : maxFrame;
        float newFrame = frame+deltaFrame;
        if (((cycleMode==FORCE_CYCLIC)||((cycleMode==DEFAULT_CYCLE_MODE)&&(animation->GetCyclic())))&&(newFrame>=endFrame))
        {
            numCycles++;
        }

        frame = animation->MakeValidFrame(newFrame, minFrame, maxFrame, cycleMode);

        if(update)
        {
            Update();
        }
    }
}

//-------------------------------------------------------------------
void tSimpleFrameController::Reset(void)
{
    frame = 0.0f;
    numCycles = 0;
    tAnimationFrameController::Reset();
}

//**************************************************************
// Class tBlendFrameController
//**************************************************************
tBlendFrameController::tBlendFrameController() :
    tAnimationFrameController(),
    nAnim(0)
{
    memset(playInfo, 0, sizeof(PlayInfo) * P3D_MAX_BLEND_ANIMS);
}

//-------------------------------------------------------------------
tBlendFrameController::tBlendFrameController(tBlendFrameController* c) :
    tAnimationFrameController(c),
    nAnim(c->nAnim)
{
    memset(playInfo, 0, sizeof(PlayInfo) * P3D_MAX_BLEND_ANIMS);
    for (int i = 0; i < nAnim; i++)
    {
        tRefCounted::Assign(playInfo[i].anim,c->playInfo[i].anim);
        playInfo[i].currentFrame = c->playInfo[i].currentFrame;
        playInfo[i].numCycles = 0;
        playInfo[i].blendFrames = c->playInfo[i].blendFrames;
        playInfo[i].ooBlendFrames = c->playInfo[i].ooBlendFrames;
        playInfo[i].accumDelta = c->playInfo[i].accumDelta;
        playInfo[i].t = c->playInfo[i].t;
    }
}

//-------------------------------------------------------------------
tBlendFrameController::~tBlendFrameController()
{
}

//-------------------------------------------------------------------
// no blending
void tBlendFrameController::SetAnimation(tAnimation* anim)
{
    P3DASSERT(anim);
    P3DASSERTMSG(ValidateAnimation(anim),"Unknown animation group.", 
                 "tBlendFrameController::SetAnimation");

    playInfo[0].SetAnimation(anim);
    playInfo[0].currentFrame = 0.0f;
    playInfo[0].numCycles = 0;
    playInfo[0].blendFrames = 0.0f;
    playInfo[0].ooBlendFrames = 0.0f;
    playInfo[0].accumDelta = 0.0f;
    playInfo[0].t = 1.0f;

    // release any currly playing animations
    for (int i = 1; i < nAnim; i++)
    {
        playInfo[i].SetAnimation(NULL);
    }

    nAnim = 1;
}

//-------------------------------------------------------------------
// no blending with start frame
void tBlendFrameController::SetAnimation(tAnimation* anim, float startframe)
{
    P3DASSERT(anim);
    P3DASSERT(ValidateAnimation(anim));

    playInfo[0].SetAnimation(anim);
    playInfo[0].currentFrame = anim->MakeValidFrame(startframe, minFrame, maxFrame, cycleMode);
    playInfo[0].numCycles = 0;
    playInfo[0].blendFrames = 0.0f;
    playInfo[0].ooBlendFrames = 0.0f;
    playInfo[0].accumDelta = 0.0f;
    playInfo[0].t = 1.0f;

    // release any currly playing animations
    for (int i = 1; i < nAnim; i++)
    {
        playInfo[i].SetAnimation(NULL);
    }

    nAnim = 1;
}

//-------------------------------------------------------------------
// blending
void tBlendFrameController::SetAnimation(tAnimation* anim, float startFrame, float blendFrames)
{
    P3DASSERT(anim);
    P3DASSERT(ValidateAnimation(anim));
    if (blendFrames <= 0.0f)
    {
        SetAnimation(anim, startFrame); // revert to non-blending case
        return;
    }

    if (nAnim == P3D_MAX_BLEND_ANIMS)
    {
        RemoveOldestAnim(); // could cause a pop
    }

    P3DASSERT(nAnim < P3D_MAX_BLEND_ANIMS);

    playInfo[nAnim].SetAnimation(anim);
    playInfo[nAnim].currentFrame = anim->MakeValidFrame(startFrame, minFrame, maxFrame, cycleMode);
    playInfo[nAnim].numCycles = 0;
    playInfo[nAnim].blendFrames = blendFrames;
    playInfo[nAnim].ooBlendFrames = 1.0f / blendFrames;
    playInfo[nAnim].accumDelta = 0.0f;
    playInfo[nAnim].t = 0.0f;  // initial blend weight
    nAnim++;
}

//-------------------------------------------------------------------
tAnimation* tBlendFrameController::GetAnimation()
{
    // return most recent animation
    return nAnim ? playInfo[nAnim-1].GetAnimation() : NULL;
}

//-------------------------------------------------------------------
float tBlendFrameController::GetNumFrames()
{
    if (nAnim != 0)
    {
        tBlendFrameController::PlayInfo *pi = &playInfo[nAnim - 1];
        return playInfo[nAnim - 1].GetAnimation()->GetNumFrames();
    }
    return 0.0f;
}

//-------------------------------------------------------------------
void tBlendFrameController::SetFrame(float f)
{
    if (nAnim != 0)
    {
        tBlendFrameController::PlayInfo *pi = &playInfo[nAnim - 1];
        pi->currentFrame = pi->GetAnimation()->MakeValidFrame(f, minFrame, maxFrame, cycleMode);
        pi->numCycles = 0;
    }
}

//-------------------------------------------------------------------
float tBlendFrameController::GetFrame()
{
    // return frame of most recent animation
    return nAnim ? playInfo[nAnim - 1].currentFrame : 0.0f;
}

//-------------------------------------------------------------------
void tBlendFrameController::Advance(float deltaTime, bool update)
{
    // If there are no animations set don't bother updating.
    if (nAnim == 0) return;
    if (playInfo[0].GetAnimation() == NULL) return;

    int activeAnims = 0;

    if(playInfo[0].GetAnimation())
    {
        tAnimation* anim = playInfo[0].GetAnimation();
        float deltaFrame = (deltaTime * 0.001f) * anim->GetSpeed() * relativeSpeed;
        float endFrame = maxFrame<0.0f ? (anim->GetNumFrames()-1) : maxFrame;
        float newFrame = playInfo[0].currentFrame + deltaFrame;
        if (((cycleMode==FORCE_CYCLIC)||((cycleMode==DEFAULT_CYCLE_MODE)&&(anim->GetCyclic())))&&(newFrame>=endFrame))
        {
            playInfo[0].numCycles++;
        }
        playInfo[0].currentFrame = anim->MakeValidFrame(newFrame, minFrame, maxFrame, cycleMode);
        activeAnims++;
    }

    // handle other animations that will be blended in
    for(int i=1; i < nAnim; i++)
    {
        tAnimation* anim = playInfo[i].GetAnimation();
        if(anim)
        {
            // compute new frame
            float deltaFrame = (deltaTime * 0.001f) * anim->GetSpeed() * relativeSpeed;
            playInfo[i].accumDelta += deltaFrame;
            if(playInfo[i].accumDelta < playInfo[i].blendFrames)
            {
                // this animation is still active
                // compute new frame and blend weight (t = accumDelta / blendFrames)
                float endFrame = maxFrame<0.0f ? (anim->GetNumFrames()-1) : maxFrame;
                float newFrame = playInfo[i].currentFrame + deltaFrame;
                if (((cycleMode==FORCE_CYCLIC)||((cycleMode==DEFAULT_CYCLE_MODE)&&(anim->GetCyclic())))&&(newFrame>=endFrame))
                {
                    playInfo[i].numCycles++;
                }
                playInfo[i].currentFrame = anim->MakeValidFrame(newFrame, minFrame, maxFrame, cycleMode);
                playInfo[i].t = SmoothClamp(playInfo[i].accumDelta * playInfo[i].ooBlendFrames);
                activeAnims++;
            }
        }
    }

    while(activeAnims != nAnim)
    {
        RemoveOldestAnim();  // some animations have finished their blend -- remove them
    }

    if(update)
    {
        Update();
    }
}

//-------------------------------------------------------------------
void tBlendFrameController::Update()
{
    if (nAnim < 2)
    {
        if (playInfo[0].GetAnimation())
        {
            UpdateNoBlending();
        }
    }
    else
    {
        UpdateWithBlending();
    }
}

//-------------------------------------------------------------------
void tBlendFrameController::RemoveOldestAnim()
{
    // shift remaining active anims to start of list
    playInfo[0].SetAnimation(NULL);
    for(int i=1; i < nAnim; i++)
    {
        playInfo[i-1] = playInfo[i];
    }
    if(nAnim > 0)
    {
        // This has to be done to prevent a double Release(), since
        // the above operation duplicates the anim pointer without
        // increasing the reference count.
        playInfo[nAnim-1].NullAnimation();
    }
    nAnim--;
}

//-------------------------------------------------------------------
tBlendFrameController::PlayInfo::PlayInfo() :
    currentFrame(0.0f),
    blendFrames(0.0f),
    ooBlendFrames(0.0f),
    accumDelta(0.0f),
    t(1.0f),
    anim(NULL)
{
}

//-------------------------------------------------------------------
tBlendFrameController::PlayInfo::~PlayInfo()
{
    tRefCounted::Release(anim);
}

//-------------------------------------------------------------------
void tBlendFrameController::PlayInfo::SetAnimation(tAnimation* newAnim)
{
    tRefCounted::Assign(anim, newAnim);
}

//-------------------------------------------------------------------
void tBlendFrameController::PlayInfo::Reset(void)
{
    currentFrame = 0.0F;
    numCycles = 0;
    blendFrames = 0.0F;
    ooBlendFrames = 0.0F;
    accumDelta = 0.0F;
    t = 0.0F;
}

//-------------------------------------------------------------------
// Sets an animation pointer to null, ignoring reference counting.
void tBlendFrameController::PlayInfo::NullAnimation()
{
    anim = NULL;
}

//-------------------------------------------------------------------
void tBlendFrameController::Reset(void)
{
    if (nAnim>0)
    {
        nAnim = 1;
        playInfo[0].Reset();

        int a;
        for(a = 1; a < nAnim; a++)
        {
            tRefCounted::Release(playInfo[a].anim);
        }
    }
    tAnimationFrameController::Reset();
}

//-------------------------------------------------------------------
int tBlendFrameController::LastFrameReached()
{
    if(nAnim > 0)
    {
        if (playInfo[nAnim-1].anim)
        {
            float endFrame = maxFrame<0.0f ? playInfo[nAnim-1].anim->GetNumFrames() : maxFrame;
            if ((cycleMode==FORCE_NON_CYCLIC)||((cycleMode==DEFAULT_CYCLE_MODE)&&(!playInfo[nAnim-1].anim->GetCyclic())))
            {
                return (playInfo[nAnim-1].currentFrame>=endFrame) ? 1 : 0;
            }
            else if ((minFrame>=0.0f)&&(minFrame==maxFrame))
            {
                return 1;
            }
            else
            {
                return playInfo[nAnim-1].LastFrameReached(); 
            }
        }
    }
    return 0;
}

//**************************************************************
// Class tFrameControllerLoader
//**************************************************************
tFrameControllerLoader::tFrameControllerLoader(void) : tSimpleChunkHandler(Pure3D::Animation::FrameControllerData::FRAME_CONTROLLER)
{
    handlers = new Handler* [MAX_FC_HANDLERS];
    for(int i = 0; i < MAX_FC_HANDLERS; i++) handlers[i] = NULL;
}

//-------------------------------------------------------------------
tFrameControllerLoader::~tFrameControllerLoader()
{
    for(int i = 0; i < MAX_FC_HANDLERS; i++) 
        tRefCounted::Release(handlers[i]);

    delete [] handlers;
}

//-------------------------------------------------------------------
void tFrameControllerLoader::AddHandler(Handler* h)
{
    for(int i = 0; i < MAX_FC_HANDLERS; i++) 
    {
        if(handlers[i] == NULL)
        {
            tRefCounted::Assign(handlers[i], h);
            return;
        }
    }
    P3DASSERT(0);
}

//-------------------------------------------------------------------
tEntity* tFrameControllerLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    unsigned version = f->GetUInt();
    P3DASSERT(version == FRAME_CONTROLLER_VERSION);

    char buffer[256];
    f->GetString(buffer);
    unsigned type = f->GetLong();

    if(CheckFC(type))
    {
        return LoadFC(buffer, type, f, store);
    }
    else
    {
        for(int i = 0; i < nHandlers; i++)
        {
            if(handlers[i]->CheckFC(type))
            {
                tEntity* e = handlers[i]->LoadFC(buffer, type, f, store);
                status = (e == NULL) ? LOAD_ERROR : LOAD_OK;
                return e;
            }
        }
    }

    p3d::printf("framecontroller (%s) could not be loaded : unrecognized animation type\n", buffer);
    return NULL;
}

//-------------------------------------------------------------------
bool tFrameControllerLoader::CheckFC(unsigned t)
{
    switch(t)
    {
        case Pure3DAnimationTypes::ANIMATED_OBJECT_AOBJ:
        case Pure3DAnimationTypes::CAMERA_CAM:
        case Pure3DAnimationTypes::EXPRESSION_EXP:
        case Pure3DAnimationTypes::LIGHT_LITE:
        case Pure3DAnimationTypes::POSE_TRANSFORM_PTRN:
        case Pure3DAnimationTypes::POSE_VISIBILITY_PVIS:
        case Pure3DAnimationTypes::SCENEGRAPH_TRANSFORM_STRN:
        case Pure3DAnimationTypes::SCENEGRAPH_VISIBILITY_SVIS:
        case Pure3DAnimationTypes::TEXTURE_TEX:
        case Pure3DAnimationTypes::BILLBOARD_QUAD_GROUP_BQG:
        case Pure3DAnimationTypes::EFFECT_EFX:
        case Pure3DAnimationTypes::VERTEX_VRTX:                 //vertex animation added
        case Pure3DAnimationTypes::SHADER_SHAD:
            return true;
            break;
        default:
            return false;
            break;
    }
}

//-------------------------------------------------------------------
tFrameController* tFrameControllerLoader::LoadFC(char* fcname, unsigned t, tChunkFile* f, tEntityStore* store)
{
    char hname[256];
    char aname[256];
    unsigned type;
    float frameOffset;

    type = t;                              // Controller type
    frameOffset = f->GetFloat();           // Frame offset
    f->GetPString(hname);                  // Hierarchy name
    f->GetPString(aname);                  // Anim name

    tAnimation* animation = NULL;
    if ((type!=Pure3DAnimationTypes::ANIMATED_OBJECT_AOBJ)&&(type!=Pure3DAnimationTypes::EFFECT_EFX))
    {
        animation = p3d::find<tAnimation>(store, aname);
        if (!animation)
        {
            p3d::printf("framecontroller (%s) has invalid animation (%s), will not be loaded\n", fcname, aname);
            return NULL;
        }
    }

    tFrameController* fc = NULL;

    switch (type)
    {
        case Pure3DAnimationTypes::ANIMATED_OBJECT_AOBJ:
            {
                tAnimatedObject* object = p3d::find<tAnimatedObject>(store, hname);
                if (object)
                {
                    tAnimatedObjectFrameController* controller = new tAnimatedObjectFrameController;
                    controller->SetName(fcname);
                    controller->SetAnimatedObject(object);
                    controller->SetCurrentAnimation(0);
                    controller->SetFrame(frameOffset);
                    fc = controller;
                }
            }
            break;

        case Pure3DAnimationTypes::CAMERA_CAM:
            {
                tVectorCamera* camera = p3d::find<tVectorCamera>(store, hname);
                if (camera)
                {
                    tCameraAnimationController* controller = new tCameraAnimationController;
                    controller->SetName(fcname);
                    controller->SetCamera(camera);
                    controller->SetAnimation(animation);
                    controller->SetFrame(frameOffset);
                    fc = controller;
                }
            }
            break;

        case Pure3DAnimationTypes::EXPRESSION_EXP:
            {
                tExpressionMixer* mixer = p3d::find<tExpressionMixer>(store, hname);
                if (mixer)
                {
                    tExpressionAnimationController* controller = new tExpressionAnimationController;
                    controller->SetName(fcname);
                    controller->SetTargetMixer(mixer);
                    controller->SetAnimation(animation);
                    controller->SetFrame(frameOffset);
                    fc = controller;
                }
            }
            break;

        case Pure3DAnimationTypes::LIGHT_LITE:
            {
                tLight* light = p3d::find<tLight>(store, hname);
                if (light)
                {
                    tLightAnimationController* controller = new tLightAnimationController;
                    controller->SetName(fcname);
                    controller->SetLight(light);
                    controller->SetAnimation(animation);
                    controller->SetFrame(frameOffset);
                    fc = controller;
                }
            }
            break;

        case Pure3DAnimationTypes::POSE_VISIBILITY_PVIS:
            {
                tCompositeDrawable* drawable = p3d::find<tCompositeDrawable>(store, hname);
                if (drawable)
                {               
                    tCompDrawVisibilityAnimationController* controller = new tCompDrawVisibilityAnimationController;
                    controller->SetName(fcname);
                    controller->SetDrawable(drawable);  
                    controller->SetAnimation(animation);
                    controller->SetFrame(frameOffset);
                    fc = controller;
                }
            }
            break;

        case Pure3DAnimationTypes::POSE_TRANSFORM_PTRN:
            {
                tCompositeDrawable* drawable = p3d::find<tCompositeDrawable>(store, hname);
                if (drawable)
                {
                    tPoseAnimationController* controller = new tPoseAnimationController;
                    controller->SetName(fcname);
                    controller->SetPose(drawable->GetPose());
                    controller->SetAnimation(animation);
                    controller->SetFrame(frameOffset);
                    fc = controller;
                }
            }
            break;

        case Pure3DAnimationTypes::SCENEGRAPH_VISIBILITY_SVIS:
            {
                Scenegraph::Scenegraph* scene = p3d::find<Scenegraph::Scenegraph>(store, hname);
                if (scene)
                {
                    tSceneGraphVisibilityAnimationController* controller = new tSceneGraphVisibilityAnimationController;
                    controller->SetName(fcname);
                    controller->SetDrawable(scene);  
                    controller->SetAnimation(animation);
                    controller->SetFrame(frameOffset);
                    fc = controller;
                }
            }
            break;

        case Pure3DAnimationTypes::SCENEGRAPH_TRANSFORM_STRN:
            {
                Scenegraph::Scenegraph* scene= p3d::find<Scenegraph::Scenegraph>(store, hname);
                if (scene)
                {
                    const tAnimationGroup* animGroup = animation->GetGroupByIndex(0);
                    Scenegraph::Transform* transform = dynamic_cast<Scenegraph::Transform*>(scene->Find(animGroup->GetUID()));
                    
                    Scenegraph::TransformAnimationController *controller = new Scenegraph::TransformAnimationController ;
                    controller->SetName(fcname);
                    controller->SetTransformNode(transform);
                    controller->SetAnimation(animation);
                    controller->SetFrame(frameOffset);
                    fc = controller;
                }
            }
            break;

        case Pure3DAnimationTypes::TEXTURE_TEX:
            {
                tShader* shader = p3d::find<tShader>(store, hname);
                if (shader)
                {
                    tTextureAnimationController* controller = new tTextureAnimationController;
                    controller->SetName(fcname);
                    controller->SetShader(shader);
                    controller->SetAnimation(animation);
                    controller->SetFrame(frameOffset);
                    fc = controller;
                }
            }
            break;

        case Pure3DAnimationTypes::BILLBOARD_QUAD_GROUP_BQG:
            {
                tBillboardQuadGroup* group = p3d::find<tBillboardQuadGroup>(store,hname);

                if (group)
                {
                    tBillboardQuadGroupAnimationController* controller = new tBillboardQuadGroupAnimationController;
                    controller->SetName(fcname);
                    controller->SetBillboardQuadGroup(group);
                    controller->SetAnimation(animation);
                    controller->SetFrame(frameOffset);
                    fc = controller;
                }
            }
            break;

        case Pure3DAnimationTypes::EFFECT_EFX:
            {
                tEffect* effect = p3d::find<tEffect>(store,hname);

                if (effect)
                {
                    tEffectController *ec = new tEffectController;
                    ec->SetName(fcname);
                    ec->SetEffect(effect);
                    fc = ec;
                }
            }
            break;

        //create a vertex-anim object and attached to the new frame controller
        case Pure3DAnimationTypes::VERTEX_VRTX:
            {
                //create a new tVertexAnimObject
                tVertexAnimObject  *pObject = new tVertexAnimObject;

                //new a vertex anim controller
                tVertexAnimController *pController = new tVertexAnimController;
                pController->SetName( fcname );

                //find the target geometry for vertex animation
                tGeometry *geometry = p3d::find<tGeometry>( store, hname );
                P3DASSERT( geometry );

                //attach the geometry to a vertex animation object
                pObject->SetActiveObject( geometry );

                //set the vertexanimobject as the target of the controller
                pController->SetTarget( pObject );

                //Add the vertex animation object to entity store
                store->Store( pObject );

                //attach animation to controller
                pController->SetAnimation( animation );

                fc = pController;
            }

        case Pure3DAnimationTypes::SHADER_SHAD:
            {
                tShader* shader = p3d::find<tShader>(store, hname);
                if (shader)
                {
                    tShaderAnimationController *controller = new tShaderAnimationController;
                    controller->SetName(fcname);
                    controller->SetShader(shader);
                    controller->SetAnimation(animation);
                    controller->SetFrame(frameOffset);
                    fc = controller;
                }
            }

            break;

        default:
            P3DASSERT(0);
            break;
    }
    return fc;
}

//**************************************************************
// Class tAnimationMemoryBlock
//**************************************************************
tAnimationMemoryBlock::tAnimationMemoryBlock() : 
    tRefCounted(),
    blockSize(0),
    usedBlocks(0),
    block(NULL)
{
}

//-------------------------------------------------------------------
tAnimationMemoryBlock::tAnimationMemoryBlock(unsigned int size) : 
    tRefCounted(),
    blockSize(size),
    usedBlocks(0),
    block(NULL)
{
    P3DASSERT(size>0);
    block = new char[size];

}

//-------------------------------------------------------------------
tAnimationMemoryBlock::~tAnimationMemoryBlock()
{
    delete [] block;
}
    
//-------------------------------------------------------------------
char* tAnimationMemoryBlock::Allocate(unsigned int size, int alignment)
{
    int alignmentSize = 3;
    switch (alignment)
    {
        case EIGHT_BYTE:
        {
            alignmentSize = 7;
            break;
        }
        case FOUR_BYTE:
        default:
        {
            alignmentSize = 3;
            break;
        }
    }
    usedBlocks = ((usedBlocks+alignmentSize) & ~alignmentSize);    
    char* newBlock = &(block[usedBlocks]);
    usedBlocks += size;
    P3DASSERT(usedBlocks<=blockSize);
    return newBlock;
}

//**************************************************************
// Class tAnimationGroup
//**************************************************************
tAnimationGroup::tAnimationGroup() : 
    groupId(0), 
    numChannels(0), 
    channels(NULL),
    memoryBlock(NULL)
{
}

//-------------------------------------------------------------------
tAnimationGroup::~tAnimationGroup()
{
    CleanUp();
}
    
//-------------------------------------------------------------------
void tAnimationGroup::CleanUp()
{    
    if (!memoryBlock)
    {
        for (int i = 0; i < numChannels; i++)
        {
            delete channels[i];
        }
        delete [] channels;
    }
    else
    {
        name.~tName();
        for (int i = 0; i < numChannels; i++)
        {
            channels[i]->CleanUp();
        }
    }
}

//-------------------------------------------------------------------
const tIntChannel* tAnimationGroup::GetIntChannel( unsigned channelCode ) const
{
    const tChannel* channel = FindChannel(channelCode);

    if(channel && channel->GetDataType() == tChannel::INT)
    {
        return static_cast<const tIntChannel*>(channel);
    }
    return NULL;
}

//-------------------------------------------------------------------
const tFloat1Channel* tAnimationGroup::GetFloat1Channel( unsigned channelCode ) const
{
    const tChannel* channel = FindChannel(channelCode);

    if(channel && channel->GetDataType() == tChannel::FLOAT1)
    {
        return static_cast<const tFloat1Channel*>(channel);
    }
    return NULL;
}

//-------------------------------------------------------------------
const tFloat2Channel* tAnimationGroup::GetFloat2Channel( unsigned channelCode ) const
{
    const tChannel* channel = FindChannel(channelCode);

    if(channel && channel->GetDataType() == tChannel::FLOAT2)
    {
        return static_cast<const tFloat2Channel*>(channel);
    }
    return NULL;
}

//-------------------------------------------------------------------
const tVectorChannel* tAnimationGroup::GetVectorChannel( unsigned channelCode ) const
{
    const tChannel* channel = FindChannel(channelCode);

    if(channel && channel->GetDataType() == tChannel::VECTOR)
    {
        return static_cast<const tVectorChannel*>(channel);
    }
    return NULL;
}

//-------------------------------------------------------------------
const tRotationChannel* tAnimationGroup::GetRotationChannel(unsigned channelCode) const
{
    const tChannel* channel = FindChannel(channelCode);

    if(channel && channel->GetDataType() == tChannel::ROTATION)
    {
        return static_cast<const tRotationChannel*>(channel);
    }
    return NULL;
}

//-------------------------------------------------------------------
const tStringChannel* tAnimationGroup::GetStringChannel(unsigned channelCode) const
{
    const tChannel* channel = FindChannel(channelCode);

    if(channel && channel->GetDataType() == tChannel::STRING)
    {
        return static_cast<const tStringChannel*>(channel);
    }
    return NULL;
}

//-------------------------------------------------------------------
const tEntityChannel* tAnimationGroup::GetEntityChannel(unsigned channelCode) const
{
    const tChannel* channel = FindChannel(channelCode);

    if(channel && channel->GetDataType() == tChannel::ENTITY)
    {
        return static_cast<const tEntityChannel*>(channel);
    }
    return NULL;
}

//-------------------------------------------------------------------
const tBoolChannel* tAnimationGroup::GetBoolChannel(unsigned channelCode) const
{
    const tChannel* channel = FindChannel(channelCode);

    if(channel && channel->GetDataType() == tChannel::BOOL)
    {
        return static_cast<const tBoolChannel*>(channel);
    }
    return NULL;
}

//-------------------------------------------------------------------
const tColourChannel* tAnimationGroup::GetColourChannel(unsigned channelCode) const
{
    const tChannel* channel = FindChannel(channelCode);

    if(channel && channel->GetDataType() == tChannel::COLOUR)
    {
        return static_cast<const tColourChannel*>(channel);
    }
    return NULL;
}

//-------------------------------------------------------------------
const tEventChannel* tAnimationGroup::GetEventChannel(unsigned channelCode) const
{
    const tChannel* channel = FindChannel(channelCode);

    if(channel && channel->GetDataType() == tChannel::EVENT)
    {
        return static_cast<const tEventChannel*>(channel);
    }
    return NULL;
}

//-------------------------------------------------------------------
const tChannel* tAnimationGroup::GetChannel(int index) const
{
    if ((index >= 0) && (index < GetChannelCount())) return channels[index];
    return NULL;
}


//-------------------------------------------------------------------
const tChannel* tAnimationGroup::FindChannel(unsigned channelCode) const
{
    if(channels)
    {
        for(int i=0;i<numChannels;i++)
        {
            if(channelCode == channels[i]->GetChannelCode())
            {
                return channels[i];
            }
        }
    }
    return NULL;
}

//**************************************************************
// Class tAnimation
//**************************************************************
tAnimation::tAnimation() :
    tEntity(),
    numFrames(0.0f),
    speed(30.0f),
    cyclic(false),
    animType(0),
    numGroups(0),
    groups(NULL),
    memoryBlock(NULL),
    sortOrder(1)
{ 
}

//-------------------------------------------------------------------------
tAnimation::~tAnimation()
{
    if (!memoryBlock)
    {
        for (int i = 0; i < numGroups; i++)
        {
            delete groups[i];
        }
    }
    else
    {
        for (int i = 0; i < numGroups; i++)
        {
            groups[i]->CleanUp();
        }
    }
    delete [] groups;
    tRefCounted::Release(memoryBlock);
}

//-------------------------------------------------------------------------
const tAnimationGroup* tAnimation::GetGroupByIndex(int index) const
{
    if ((index>=0)&&(index<numGroups))
    {
        P3DASSERT(groups[index]);
        return groups[index];
    }
    return NULL;
}

//-------------------------------------------------------------------------
const tAnimationGroup* tAnimation::GetGroupById(int id) const
{
    if (numGroups == 0)
    { 
        return NULL;
    }
    else if (sortOrder == 2)
    {
        int left = 0;
        int right = numGroups-1;
        int center = right/2;
    
        while (left<right)
        {
            if (groups[center]->GetGroupId()>id)
            {
                right = center-1;
                center = ((right-left)/2)+left;
            }
            else if (groups[center]->GetGroupId()<id)
            {
                left = center+1;
                center = ((right-left)/2)+left;
            }
            else
            {
                return groups[center];
            }
        }

        if (groups[center]->GetGroupId()==id)
        {
            return groups[center];
        }
        return NULL;
    }
    else
    {
        for(int i=0;i<numGroups;i++)
        {
            P3DASSERT(groups[i]);
            if(groups[i]->GetGroupId() == id)
            {
                return(groups[i]);
            }
        }
        return NULL;
    }
}

//-------------------------------------------------------------------------
const tAnimationGroup* tAnimation::GetGroupByName(const char* name) const
{
    return GetGroupByUID(tName::MakeUID(name));
}

//-------------------------------------------------------------------------
const tAnimationGroup* tAnimation::GetGroupByUID(tUID uid) const
{
    if (numGroups == 0)
    { 
        return NULL;
    }
    else if (sortOrder == 1)
    {
        int left = 0;
        int right = numGroups-1;
        int center = right/2;
    
        while (left<right)
        {
            if (groups[center]->GetUID()>uid)
            {
                right = center-1;
                center = ((right-left)/2)+left;
            }
            else if (groups[center]->GetUID()<uid)
            {
                left = center+1;
                center = ((right-left)/2)+left;
            }
            else
            {
                return groups[center];
            }
        }

        if (groups[center]->GetUID()==uid)
        {
            return groups[center];
        }
        return NULL;
    }
    else
    {
        for(int i=0;i<numGroups;i++)
        {
            P3DASSERT(groups[i]);
            if(groups[i]->GetUID() == uid)
            {
                return(groups[i]);
            }
        }
        return NULL;
    }
}

//-------------------------------------------------------------------------
int tAnimation::FindGroupIndexById(int id) const
{
    for(int i=0;i<numGroups;i++)
    {
        P3DASSERT(groups[i]);
        if(groups[i]->GetGroupId() == id)
        {
            return i;
        }
    }
    return -1;
}

//-------------------------------------------------------------------------
int tAnimation::FindGroupIndexByName(const char* name) const
{
    return FindGroupIndexByUID(tName::MakeUID(name));
}

//-------------------------------------------------------------------------
int tAnimation::FindGroupIndexByUID(tUID uid) const
{
    for (int i = 0; i < numGroups; i++)
    {
        P3DASSERT(groups[i]);
        if(groups[i]->GetUID() == uid)
        {
            return i;
        }
    }
    return -1;
}

//-------------------------------------------------------------------------
float tAnimation::MakeValidFrame(float frame, float rangeMin, float rangeMax, p3dCycleMode mode) const
{
    frame = frame>0.0f ? frame : 0.0f;

    rangeMin = rangeMin>=0.0f ? rangeMin : 0.0f;
    rangeMax = rangeMax>=0.0f ? rangeMax : numFrames;

    if ((rangeMin <= frame)&&(frame <= rangeMax))
    {
        return frame;
    }
    else if (frame > rangeMax)
    {
        switch (mode)
        {
            case FORCE_CYCLIC:
            {
                return fmodf(frame, rangeMax)+rangeMin;
                break;
            }
            case FORCE_NON_CYCLIC:
            {
                return rangeMax;
                break;
            }
            case DEFAULT_CYCLE_MODE:
            default:
            {
                return (cyclic ? fmodf(frame, rangeMax)+rangeMin : rangeMax);
                break;
            }
        }
    }
    else
    {
        return rangeMin;
    }
}

//-------------------------------------------------------------------------
void tAnimation::SortAnimationGroups()
{
    switch (sortOrder)
    {
        case 1:
        {
            qsort(groups, numGroups, sizeof(tAnimationGroup*), AnimationGroupCompareByUID);
            break;
        }
        case 2:
        {
            qsort(groups, numGroups, sizeof(tAnimationGroup*), AnimationGroupCompareByGroupID);
            break;
        }
        default:
            break;
    }
}

//**************************************************************
// Class tAnimationLoader
//**************************************************************
tAnimationLoader::tAnimationLoader() : 
    tSimpleChunkHandler(Pure3D::Animation::AnimationData::ANIMATION),
    sortOrder(1)
{
}

//-------------------------------------------------------------------------
tEntity* tAnimationLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    unsigned int animVersion = f->GetLong();
    P3DASSERT(animVersion == ANIMATION_VERSION);
    
    char buf[256];
    f->GetPString(buf);

    tAnimation* anim = new tAnimation;
    anim->SetName(buf);
    anim->animType = f->GetLong();
    anim->numFrames = f->GetFloat();
    anim->speed = f->GetFloat();
    anim->cyclic = f->GetLong() == 1;
    anim->sortOrder = sortOrder;

    while(f->ChunksRemaining())
    {
        switch(f->BeginChunk())
        {
            case Pure3D::Animation::AnimationData::SIZE:
            {
                unsigned int animSizeVersion = f->GetLong();
                if (animSizeVersion != ANIMATION_SIZE_VERSION)
                {
                #ifndef RAD_RELEASE && defined(def P3D_ALLOW_ENTITY_GETNAME)
                    char buffer[1024];
                    sprintf(buffer,"WARNING : animation %s has an old animation size chunk re-export or use p3danim\n",anim->GetName());
                    p3d::print(buffer);
                #endif
                }
                else
                {
                    unsigned int pcSize = f->GetLong();
                    unsigned int ps2Size = f->GetLong();
                    unsigned int xboxSize = f->GetLong();
                    unsigned int gcSize = f->GetLong();

                    // TODO(3UR): uwp use pc size? as we technically use pc assets (idk maybe it should be done)
                    #if defined(RAD_WIN32)
                    unsigned int animSize = pcSize;
                    #elif defined(RAD_UWP)
                    unsigned int animSize = xboxSize;
                    #endif
                    if ((anim->numGroups==0)&&(animSize>0))
                    {
                        unsigned int factor = sizeof(void*) / 4;
                        anim->memoryBlock = new tAnimationMemoryBlock(animSize * factor);
                        anim->memoryBlock->AddRef();
                    }
                }
                break;
            }
            case Pure3D::Animation::AnimationData::GROUP_LIST:
            {
                unsigned int groupListVersion = f->GetLong();
                P3DASSERT(groupListVersion == ANIMATION_GROUP_LIST_VERSION);
                                                             
                anim->numGroups = f->GetLong();
                if (anim->numGroups>0)
                {
                    anim->groups = new tAnimationGroup*[anim->numGroups];
                    memset(anim->groups, 0, sizeof(tAnimationGroup*)*(anim->numGroups));

                    int currGroup = 0;
                    while((f->ChunksRemaining())&&(currGroup<anim->numGroups))
                    {
                        f->BeginChunk();
                        anim->groups[currGroup] = LoadAnimationGroup(f, store, anim->memoryBlock);
                        P3DASSERT(anim->groups[currGroup]);
                        f->EndChunk();
                        currGroup++;
                    }
                }
                break;
            }

            default:
                break;

        } // switch chunkID
        f->EndChunk();
    }

#ifndef RAD_RELEASE
#ifdef P3D_ALLOW_ENTITY_GETNAME
    if (anim->memoryBlock)
    {
        if (anim->memoryBlock->usedBlocks!=anim->memoryBlock->blockSize)
        {
            //char buffer[1024];
            //sprintf(buffer,"WARNING : animation %s has mismatched block sizes (used %d, allocated %d), re-export or use p3danim\n",anim->GetName(),anim->memoryBlock->usedBlocks,anim->memoryBlock->blockSize);
            //p3d::print(buffer);
        }       
    }
    else
    {
        char buffer[1024];
        sprintf(buffer,"WARNING : animation %s doesn't have precomputed memory sizes, re-export or use p3danim\n",anim->GetName());
        p3d::print(buffer);
    }
#endif
#endif

    anim->SortAnimationGroups();

    return anim;
}

//-------------------------------------------------------------------------
tAnimationGroup* tAnimationLoader::LoadAnimationGroup(tChunkFile* f, tEntityStore* store, tAnimationMemoryBlock* block)
{
    P3DASSERT(f->GetCurrentID() == Pure3D::Animation::AnimationData::GROUP);

    unsigned int groupVersion = f->GetLong();
    P3DASSERT(groupVersion == ANIMATION_GROUP_VERSION);

    char buf[256];
    f->GetPString(buf);

    tAnimationGroup* animGroup = NULL;
    if (!block)
    {
        animGroup = new tAnimationGroup;
    }
    else
    {
        animGroup = (tAnimationGroup*)new(block->Allocate(sizeof(tAnimationGroup),tAnimationMemoryBlock::EIGHT_BYTE))tAnimationGroup;
    }
    animGroup->SetName(buf);
    animGroup->groupId = f->GetLong();
    animGroup->numChannels = f->GetLong();
    animGroup->memoryBlock = block;
    if (animGroup->numChannels>0)
    {
        if (!block)
        {
            animGroup->channels = new tChannel*[animGroup->numChannels];             
        }
        else
        {
            animGroup->channels = (tChannel**)(block->Allocate(sizeof(tChannel*)*animGroup->numChannels));
        }
        memset(animGroup->channels, 0, sizeof(tChannel*)*(animGroup->numChannels));

        tChannelLoader channelLoader;
        
        int currChannel = 0;
        while((f->ChunksRemaining())&&(currChannel<animGroup->numChannels))
        {
            f->BeginChunk();
            animGroup->channels[currChannel] = channelLoader.LoadChannel(f,store,animGroup->memoryBlock);
            f->EndChunk();
            currChannel++;
        }
    }   
    return animGroup;
}

