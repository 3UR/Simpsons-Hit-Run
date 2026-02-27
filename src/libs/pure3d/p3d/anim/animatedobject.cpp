//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/anim/animatedobject.hpp>
#include <p3d/anim/compositedrawable.hpp>
#include <p3d/anim/poseanimation.hpp>
#include <p3d/anim/visibilityanimation.hpp>
#include <p3d/effects/effect.hpp>
#include <p3d/effects/particlesystem.hpp>
#include <p3d/error.hpp>
#include <p3d/chunkfile.hpp>
#include <p3d/utility.hpp>
#include <constants/chunkids.hpp>

#include <string.h>

static const unsigned ANIMATED_OBJECT_FACTORY_VERSION = 0;
static const unsigned ANIMATED_OBJECT_VERSION = 0;
static const unsigned ANIMATED_OBJECT_ANIMATION_VERSION = 0;

//*********************************************************
// todo:
// - Memory usage problem, the animated object factory keeps around
//   a template of the base object that never gets used (wastes space).
//   Perhaps implement a pool of animated objects that get allocated at load
//   time and then game teams can request an object from the factory
// - Deformers don't work
// - Particle blend ratios and bias are not easily accessible, perhaps
//   smart prop should take these into account as tweakable parameters
//*********************************************************

//*********************************************************
// Class tAnimatedObjectFactory
//*********************************************************
tAnimatedObjectFactory::tAnimatedObjectFactory() :
    tEntity(),
    baseObject(NULL),
    numAnimations(0),
    animations(NULL),
    poseControllers(NULL),
    visibilityControllers(NULL),
    numEffects(NULL),
    effects(NULL),
    effectControllers(NULL)
{
}

tAnimatedObjectFactory::~tAnimatedObjectFactory()
{
    tRefCounted::Release(baseObject);
    for (unsigned int i = 0; i < numAnimations; i++)
    {
        tRefCounted::Release(animations[i]);
    }
    delete [] animations;
    delete [] poseControllers;
    delete [] visibilityControllers;
    delete [] numEffects;
    for (unsigned int j = 0; j < numAnimations; j++)
    {
        delete effects[j];
        delete effectControllers[j];
    }
    delete [] effects;
    delete [] effectControllers;
}

tAnimatedObject*
tAnimatedObjectFactory::CreateObject(tAnimatedObjectFrameController* controller, tPose* pose)
{
    tAnimatedObject* object = new tAnimatedObject();
    object->SetNameObject(GetNameObject());
    object->baseObject = baseObject->Clone(pose);
    object->baseObject->AddRef();

    // make sure that there are animations to copy
    if (numAnimations>0)
    {
        // clone the animation object animations
        object->numAnimations = numAnimations;
        object->animations = new tAnimatedObjectAnimation*[numAnimations];
        for (unsigned int i = 0; i < numAnimations; i++)
        {
            object->animations[i] = animations[i]->Clone();
            object->animations[i]->AddRef();

            // set the correct pose for the new pose controller
            if (poseControllers[i]!=-1)
            {            
                ((tPoseAnimationController*)object->animations[i]->controllers[poseControllers[i]])->SetPose(object->baseObject->GetPose());
            }

            // set the visibility controller to point to the new base object
            if (visibilityControllers[i]!=-1)
            {
                ((tCompDrawVisibilityAnimationController*)(object->animations[i]->controllers[visibilityControllers[i]]))->SetDrawable(object->baseObject);
            }

            // make sure effects get cloned properly
            for (unsigned int j = 0; j < numEffects[i]; j++)
            {
                // only create a new effect for the first animated object animation and use it in all the controllers
                // of the subsequent animations
                if (i==0)
                {
                    // get the original effect
                    tEffect* origObj = (tEffect*)object->baseObject->GetDrawableElement(effects[i][j])->GetDrawable();
                    // create the effect (and attach it to it's new controller
                    tEffect* newObj = origObj->GetFactory()->CreateEffect((tEffectController*)(object->animations[i]->controllers[effectControllers[i][j]]));
                    // set the effect into the composite drawable
                    object->baseObject->GetDrawableElement(effects[i][j])->SetDrawable(newObj);                    
                }
                else
                {
                    // used the previously created effect and attach it to the effect controller of the current animation
                    ((tEffectController*)(object->animations[i]->controllers[effectControllers[i][j]]))->SetEffect((tEffect*)object->baseObject->GetDrawableElement(effects[i][j])->GetDrawable());
                }
            }
        }
    }

    if (controller)
    {
        tRefCounted::Assign(controller->object,object);
    }

    return object;
}

//*********************************************************
// Class tAnimatedObjectAnimation
//*********************************************************
tAnimatedObjectAnimation::tAnimatedObjectAnimation() :
    tEntity(),
    frameRate(30.0f),
    numFrames(0.0f),
    numControllers(0),
    controllers(NULL),
    activeControllers(NULL)
{
}

tAnimatedObjectAnimation::~tAnimatedObjectAnimation()
{
    for (unsigned int i = 0; i < numControllers; i++)
    {
        tRefCounted::Release(controllers[i]);
    }
    delete [] controllers;
    delete [] activeControllers;
}

tAnimatedObjectAnimation* 
tAnimatedObjectAnimation::Clone() const
{
    tAnimatedObjectAnimation* animation = new tAnimatedObjectAnimation;
    animation->frameRate = frameRate;
    animation->numFrames = numFrames;
    animation->numControllers = numControllers;
    animation->controllers = new tFrameController*[numControllers];
    animation->activeControllers = new bool[numControllers];

    for (unsigned int i = 0; i < numControllers; i++)
    {
        animation->controllers[i] = controllers[i]->Clone();
        animation->controllers[i]->AddRef();
        animation->activeControllers[i] = activeControllers[i];
    }
    return animation;
}

tFrameController* 
tAnimatedObjectAnimation::GetFrameControllerByIndex(unsigned int index) const
{
    if (index < numControllers)
    {
        return controllers[index];
    }
    return NULL;
}

tFrameController* 
tAnimatedObjectAnimation::GetFrameControllerByName(const char* fcname) const
{
    return GetFrameControllerByUID(tEntity::MakeUID(fcname));
}

tFrameController* 
tAnimatedObjectAnimation::GetFrameControllerByUID(tUID fcuid) const
{
    for (unsigned int i = 0; i < numControllers; i++)
    {
        if (controllers[i]->GetUID() == fcuid)
        {
            return controllers[i];
        }
    }
    return NULL;
}
    
void 
tAnimatedObjectAnimation::EnableFrameControllerByIndex(unsigned int index, bool enable)
{
    activeControllers[index] = enable;
}

void 
tAnimatedObjectAnimation::EnableFrameControllerByName(const char* fcname, bool enable)
{
    EnableFrameControllerByUID(tEntity::MakeUID(fcname),enable);
}

void 
tAnimatedObjectAnimation::EnableFrameControllerByUID(tUID fcuid, bool enable)
{
    for (unsigned int i = 0; i < numControllers; i++)
    {
        if (controllers[i]->GetUID() == fcuid)
        {
            activeControllers[i] = enable;
        }
    }
}

void 
tAnimatedObjectAnimation::Advance(float deltaTime)
{
    for (unsigned int i = 0; i < numControllers; i++)
    {
        if ((controllers[i])&&(activeControllers[i]))
        {
            controllers[i]->Advance(deltaTime,true);
        }
    }    
}

//*********************************************************
// Class tAnimatedObject
//*********************************************************
tAnimatedObject::tAnimatedObject() :
    tDrawable(),
    deltaTime(0.0f),
    baseObject(NULL),
    numAnimations(0),
    currentAnimation(0),
    animations(NULL)
{
}

tAnimatedObject::~tAnimatedObject()
{
    tRefCounted::Release(baseObject);
    for (unsigned int i = 0; i < numAnimations; i++)
    {
        tRefCounted::Release(animations[i]);
    }
    delete [] animations;
}

void
tAnimatedObject::Update(float dt)
{
    deltaTime += dt;
}

void
tAnimatedObject::Display()
{
    if (!baseObject)
        return;

    P3DASSERT(animations[currentAnimation]);
    animations[currentAnimation]->Advance(deltaTime);
    deltaTime = 0.0f;
    baseObject->Display();
}

void tAnimatedObject::ProcessShaders(ShaderCallback& callback)
{
    if(baseObject)
        baseObject->ProcessShaders(callback);
}

//*********************************************************
// Class tAnimatedObjectFrameController
//*********************************************************
tAnimatedObjectFrameController::tAnimatedObjectFrameController() :
    tFrameController(),
    cycleMode(DEFAULT_CYCLE_MODE),
    relativeSpeed(1.0f),
    frame(0.0f),
    numCycles(0),
    object(NULL)
{
}

tAnimatedObjectFrameController::tAnimatedObjectFrameController(tAnimatedObjectFrameController* c) :
    tFrameController(c),
    cycleMode(c->cycleMode),
    relativeSpeed(c->relativeSpeed),
    frame(c->frame),
    numCycles(0),
    object(NULL)
{
    tRefCounted::Assign(object,c->object);
}

tAnimatedObjectFrameController::~tAnimatedObjectFrameController()
{
    tRefCounted::Release(object);
}

void 
tAnimatedObjectFrameController::SetRelativeSpeed(float rs)
{
    if ((!object)||(object->numAnimations==0))
        return;

    relativeSpeed = rs;
    for (unsigned int i = 0; i < object->numAnimations; i++)
    {
        tAnimatedObjectAnimation* animation = object->animations[i];
        for (unsigned int j = 0; j < animation->numControllers; j++)
        {
            if (animation->controllers[j])
            {
                animation->controllers[j]->SetRelativeSpeed(relativeSpeed);
            }
        }
    }
}

void 
tAnimatedObjectFrameController::SetFrame(float f)
{
    if ((!object)||(object->numAnimations==0))
        return;

    frame = f<0.0f ? 0.0f : f;
    numCycles = 0;

    tAnimatedObjectAnimation* animation = object->animations[object->currentAnimation];
    P3DASSERT(animation);

    object->deltaTime += (frame * 1000.0f) / (relativeSpeed * animation->frameRate);

    for (unsigned int i = 0; i < animation->numControllers; i++)
    {
        if (animation->controllers[i])
        {
            animation->controllers[i]->SetFrame(0.0f);
        }
    }
}

void 
tAnimatedObjectFrameController::SetFrameRange(float minframe, float maxframe)
{
    if ((!object)||(object->numAnimations==0))
        return;

    tAnimatedObjectAnimation* animation = object->animations[object->currentAnimation];
    P3DASSERT(animation);
    for (unsigned int i = 0; i < animation->numControllers; i++)
    {
        if (animation->controllers[i])
        {
            animation->controllers[i]->SetFrameRange(minframe,maxframe);
        }
    }
}

void 
tAnimatedObjectFrameController::SetCycleMode(p3dCycleMode mode)
{
    if (cycleMode == mode)
        return;

    cycleMode = mode;

    if ((!object)||(object->numAnimations==0))
        return;

    tAnimatedObjectAnimation* animation = object->animations[object->currentAnimation];
    P3DASSERT(animation);
    for (unsigned int i = 0; i < animation->numControllers; i++)
    {
        if (animation->controllers[i])
        {
            animation->controllers[i]->SetCycleMode(cycleMode);
        }
    }
}

void 
tAnimatedObjectFrameController::Advance(float deltaTime, bool update)
{
    if ((!object)||(object->numAnimations==0))
        return;

    tAnimatedObjectAnimation* animation = object->animations[object->currentAnimation];
    P3DASSERT(animation);

    float deltaFrame = (deltaTime * 0.001f) * animation->frameRate * relativeSpeed;
    if ((deltaFrame+frame)>=animation->numFrames)
    {
        numCycles++;
    }
    frame += deltaFrame;
    object->deltaTime += deltaTime;
}

void 
tAnimatedObjectFrameController::Reset()
{
    SetCycleMode(DEFAULT_CYCLE_MODE);

    if ((!object)||(object->numAnimations==0))
        return;

    for (unsigned int i = 0; i < object->numAnimations; i++)
    {
        tAnimatedObjectAnimation* animation = object->animations[i];
        for (unsigned int j = 0; j < animation->numControllers; j++)
        {
            if (animation->controllers[j])
            {
                animation->controllers[j]->Reset();
            }
            animation->activeControllers[j] = true;
        }
    }
    frame = 0.0f;
    numCycles = 0;
    object->deltaTime = 0.0f;
    object->currentAnimation = 0;
}

tFrameController*
tAnimatedObjectFrameController::Clone(void)
{
    return new tAnimatedObjectFrameController(this);
}

void 
tAnimatedObjectFrameController::Update()
{
}

//**************************************************************
// Class tAnimatedObjectFactoryLoader
//**************************************************************
tAnimatedObjectFactoryLoader::tAnimatedObjectFactoryLoader() : 
    tSimpleChunkHandler(Pure3D::AnimatedObject::FACTORY)
{
}

//-------------------------------------------------------------------------
tEntity* tAnimatedObjectFactoryLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    unsigned int version = f->GetLong();
    P3DASSERT(version == ANIMATED_OBJECT_FACTORY_VERSION);
    
    tAnimatedObjectFactory* factory = new tAnimatedObjectFactory;

    char buf[256];

    f->GetPString(buf);
    factory->SetName(buf);
 
    f->GetPString(buf);   
    factory->baseObject = p3d::find<tCompositeDrawable>(store, buf);
    P3DASSERT(factory->baseObject);
    factory->baseObject->AddRef();

    factory->numAnimations = f->GetLong();
    factory->animations = new tAnimatedObjectAnimation*[factory->numAnimations];
    factory->poseControllers = new int[factory->numAnimations];
    factory->visibilityControllers = new int[factory->numAnimations];
    factory->numEffects = new unsigned int[factory->numAnimations];
    factory->effects = new int*[factory->numAnimations];
    factory->effectControllers = new int*[factory->numAnimations];

    memset(factory->animations,0,factory->numAnimations*sizeof(tAnimatedObjectAnimation*));
    memset(factory->poseControllers,-1,factory->numAnimations*sizeof(int));
    memset(factory->visibilityControllers,-1,factory->numAnimations*sizeof(int));
    memset(factory->numEffects,0,factory->numAnimations*sizeof(unsigned int));
    memset(factory->effects,0,factory->numAnimations*sizeof(int));
    memset(factory->effectControllers,0,factory->numAnimations*sizeof(int));

    tAnimatedObjectAnimationLoader loader;

    int effects[256];
    int effectControllers[256];

    unsigned int currAnim = 0;
    while(f->ChunksRemaining())
    {
        switch(f->BeginChunk())
        {
            case Pure3D::AnimatedObject::ANIMATION:
            {   
                if (currAnim<factory->numAnimations)
                {
                    tAnimatedObjectAnimation* animation = loader.LoadObject(f,store);
                    if (animation)
                    {
                        for (unsigned int i = 0; i < animation->numControllers; i++)
                        {
                            if ((factory->poseControllers[currAnim]==-1)&&(dynamic_cast<tPoseAnimationController*>(animation->controllers[i])))
                            {
                                factory->poseControllers[currAnim] = i;
                                continue;
                            }
                            if ((factory->visibilityControllers[currAnim]==-1)&&(dynamic_cast<tCompDrawVisibilityAnimationController*>(animation->controllers[i])))
                            {
                                factory->visibilityControllers[currAnim] = i;
                                continue;
                            }
                            tEffectController* ec = dynamic_cast<tEffectController*>(animation->controllers[i]);
                            if (ec)
                            {
                                effects[factory->numEffects[currAnim]]=factory->baseObject->FindNodeIndex(ec->GetEffect()->GetUID());
                                if (effects[factory->numEffects[currAnim]]!=-1)
                                {
                                    effectControllers[factory->numEffects[currAnim]]=i;
                                    factory->numEffects[currAnim]++;
                                }
                                continue;
                            }
                        }

                        factory->effects[currAnim] = new int[factory->numEffects[currAnim]];
                        factory->effectControllers[currAnim] = new int[factory->numEffects[currAnim]];
                        memcpy(factory->effects[currAnim],effects,factory->numEffects[currAnim]*sizeof(int));
                        memcpy(factory->effectControllers[currAnim],effectControllers,factory->numEffects[currAnim]*sizeof(int));

                        tRefCounted::Assign(factory->animations[currAnim],animation);
                        currAnim++;
                    }
                }
                break;
            }

            default:
                break;

        } // switch chunkID
        f->EndChunk();
    }

    P3DASSERT(currAnim==factory->numAnimations);
    return factory;
}

//**************************************************************
// Class tAnimatedObjectLoader
//**************************************************************
tAnimatedObjectLoader::tAnimatedObjectLoader() : 
    tSimpleChunkHandler(Pure3D::AnimatedObject::OBJECT)
{
}

//-------------------------------------------------------------------------
tEntity* tAnimatedObjectLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    unsigned int version = f->GetLong();
    P3DASSERT(version == ANIMATED_OBJECT_VERSION);
    
    char objectName[256];
    f->GetPString(objectName);

    char factoryName[256];
    f->GetPString(factoryName);

    unsigned int currentAnimation = (unsigned int)f->GetLong();

    tAnimatedObject* object = NULL;
    tAnimatedObjectFactory* factory = p3d::find<tAnimatedObjectFactory>(store, factoryName);
    if (factory)
    {
        object = factory->CreateObject(NULL);
        object->SetName(objectName);
        object->currentAnimation = currentAnimation;
        if (object->currentAnimation >= object->numAnimations)
        {
            object->currentAnimation = object->numAnimations-1;
        }
    }
    else
    {
        object = new tAnimatedObject;
        object->SetName(objectName);
    }

    return object;
}

//**************************************************************
// Class tAnimatedObjectAnimationLoader
//**************************************************************
bool tAnimatedObjectAnimationLoader::HandlesChunk(unsigned chunkID)
{
    return (chunkID==Pure3D::AnimatedObject::ANIMATION);
}

//-------------------------------------------------------------------------
tAnimatedObjectAnimation* tAnimatedObjectAnimationLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    unsigned int version = f->GetLong();
    P3DASSERT(version == ANIMATED_OBJECT_ANIMATION_VERSION);
    
    tAnimatedObjectAnimation* animation = new tAnimatedObjectAnimation;

    char buf[256];
    f->GetPString(buf);
    animation->SetName(buf);
    animation->frameRate = f->GetFloat();
    animation->numControllers = f->GetLong();
    animation->controllers = new tFrameController*[animation->numControllers];
    animation->activeControllers = new bool[animation->numControllers];

    memset(animation->controllers,0,animation->numControllers*sizeof(tFrameController*));
    memset(animation->activeControllers,1,animation->numControllers*sizeof(bool));

    tFrameControllerLoader* loader = dynamic_cast<tFrameControllerLoader*>(p3d::loadManager->GetP3DHandler()->GetHandler(Pure3D::Animation::FrameControllerData::FRAME_CONTROLLER));
    loader->AddRef();

    unsigned int currController=0;
    while(f->ChunksRemaining())
    {
        switch(f->BeginChunk())
        {
            case Pure3D::Animation::FrameControllerData::FRAME_CONTROLLER:
            {   
                tFrameController* controller = (tFrameController*)loader->LoadObject(f,store);
                if (controller)
                {
                    tRefCounted::Assign(animation->controllers[currController],controller);
                    currController++;
                }
                break;
            }

            default:
                break;

        } // switch chunkID
        f->EndChunk();
    }
    loader->Release();

    animation->numFrames = 0.0f;
    for (unsigned int i = 0; i < animation->numControllers; i++)
    {
        tAnimationFrameController *ac = dynamic_cast<tAnimationFrameController*>(animation->controllers[i]);
        if (ac)
        {
            tAnimation* anim = ac->GetAnimation();
            if (anim)
            {
                animation->numFrames = anim->GetNumFrames()>animation->numFrames ? anim->GetNumFrames() : animation->numFrames;
            }
        }
    }

    rWarningMsg( currController == animation->numControllers, "Warning: controllers missing for animated object" );
    animation->numControllers = currController;
    return animation;
}
