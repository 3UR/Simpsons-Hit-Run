/*===========================================================================
    animateobject.hpp
    Copyright (c)2002 Radical Entertainment Ltd.
    All rights reserved.
===========================================================================*/
#ifndef _ANIMATED_OBJECT_HPP
#define _ANIMATED_OBJECT_HPP

#include <p3d/entity.hpp>
#include <p3d/drawable.hpp>
#include <p3d/anim/animate.hpp>
#include <p3d/loadmanager.hpp>

class tAnimatedObjectFactory;
class tAnimatedObject;
class tAnimatedObjectAnimation;
class tAnimatedObjectFrameController;
class tCompositeDrawable;
class tPose;

//*********************************************************
// Class tAnimatedObjectFactory
//*********************************************************
class tAnimatedObjectFactory : public tEntity
{
public:
    tAnimatedObject* CreateObject(tAnimatedObjectFrameController* controller, tPose* pose = NULL );

    friend class tAnimatedObjectFactoryLoader;

protected:
    tAnimatedObjectFactory();
    ~tAnimatedObjectFactory();

    tCompositeDrawable*         baseObject;
    unsigned int                numAnimations;
    tAnimatedObjectAnimation**  animations;

    int*                        poseControllers;
    int*                        visibilityControllers;

    unsigned int*               numEffects;
    int**                       effects;              
    int**                       effectControllers;
};
    
//*********************************************************
// Class tAnimatedObjectAnimation
//*********************************************************
class tAnimatedObjectAnimation : public tEntity
{
public:     
    float GetFrameRate() const                                                      { return frameRate; }
    float GetNumFrames() const                                                      { return numFrames; }
    unsigned int GetNumFrameControllers() const                                     { return numControllers; }

    tFrameController* GetFrameControllerByIndex(unsigned int index) const;
    tFrameController* GetFrameControllerByName(const char* name) const;
    tFrameController* GetFrameControllerByUID(tUID uid) const;
    
    void EnableFrameControllerByIndex(unsigned int index, bool enable);
    void EnableFrameControllerByName(const char* name, bool enable);
    void EnableFrameControllerByUID(tUID uid, bool enable);

    friend class tAnimatedObjectFactory;
    friend class tAnimatedObject;
    friend class tAnimatedObjectFrameController;
    friend class tAnimatedObjectFactoryLoader;
    friend class tAnimatedObjectAnimationLoader;

protected:
    tAnimatedObjectAnimation();
    ~tAnimatedObjectAnimation();

    tAnimatedObjectAnimation* Clone() const;

    void Advance(float deltaTime);

    float              frameRate;
    float              numFrames;
    unsigned int       numControllers;
    tFrameController** controllers;
    bool*              activeControllers;
};

//*********************************************************
// Class tAnimatedObject
//*********************************************************
class tAnimatedObject : public tDrawable
{
public:
    void Update(float deltaTime);
    void Display();

    virtual void ProcessShaders(ShaderCallback&);

    tCompositeDrawable* GetBaseObject() const                                   { return baseObject; }

    unsigned int GetNumAnimations() const                                       { return numAnimations; }
    void SetCurrentAnimation(unsigned int anim)                                 { if (numAnimations>0) currentAnimation = anim>=numAnimations ? numAnimations-1 : anim; }

    tAnimatedObjectAnimation* GetCurrentAnimation() const                       { return animations[currentAnimation]; }
    tAnimatedObjectAnimation* GetAnimation(unsigned int index) const            { if (index<numAnimations) return animations[index]; return NULL; }

    friend class tAnimatedObjectFactory;
    friend class tAnimatedObjectFrameController;
    friend class tAnimatedObjectLoader;

protected:
    tAnimatedObject();
    ~tAnimatedObject();

    float                       deltaTime;
    tCompositeDrawable*         baseObject;
    unsigned int                numAnimations;
    unsigned int                currentAnimation;
    tAnimatedObjectAnimation**  animations;
};

//*********************************************************
// Class tAnimatedObjectFrameController
//*********************************************************
class tAnimatedObjectFrameController : public tFrameController
{
public:
    tAnimatedObjectFrameController();
    tAnimatedObjectFrameController(tAnimatedObjectFrameController* c);

    virtual tFrameController* Clone(void);

    unsigned int GetNumAnimations() const                                       { if (object) return object->GetNumAnimations(); return 0; }
    void SetCurrentAnimation(unsigned int anim)                                 { if (object) object->SetCurrentAnimation(anim); SetFrame(0.0f); }

    void SetAnimatedObject(tAnimatedObject* new_obj)                            { tRefCounted::Assign(object,new_obj); }
    tAnimatedObject* GetAnimatedObject()                                        { return object; }

    // adjust animation speed multiplier
    virtual void SetRelativeSpeed(float rs);

    // return animation speed multiplier
    virtual float GetRelativeSpeed()                                            { return relativeSpeed; }

    // gets the number of frames used by this frame controller
    virtual float GetNumFrames()                                                { if (object) return object->GetCurrentAnimation()->GetNumFrames(); return 0.0f; }

    // raw frame control - this overrides Advance(), and SetRelativeSpeed()
    virtual void SetFrame(float f);

    // returns the current frame of the controller
    virtual float GetFrame()                                                    { return frame; }

    // set the valid frame range -1.0f means no frame range, play all frames
    virtual void SetFrameRange(float minframe, float maxframe);

    // set the cycling mode
    virtual void SetCycleMode(p3dCycleMode cyclemode);

    // set the cycling mode
    virtual p3dCycleMode GetCycleMode()                                         { return cycleMode; }

    // Advance animation by deltaTime milliseconds
    virtual void Advance(float deltaTime, bool update = true);

    // returns true if a non-cyclic animation has reached its final frame
    virtual int LastFrameReached()                                              { return numCycles; }

    // resets the internals
    virtual void Reset(void);

    friend class tAnimatedObjectFactory;

protected:
    virtual ~tAnimatedObjectFrameController();
   
    // update object that is being animated
    void Update();

    p3dCycleMode        cycleMode;
    float               relativeSpeed;
    float               frame;
    int                 numCycles;
    tAnimatedObject*    object;
};
 
//*********************************************************
// Class tAnimatedObjectFactoryLoader
//*********************************************************
class tAnimatedObjectFactoryLoader : public tSimpleChunkHandler
{
public:
    tAnimatedObjectFactoryLoader();
    tEntity* LoadObject(tChunkFile* file, tEntityStore* store);

protected:
    ~tAnimatedObjectFactoryLoader() {};
};

//*********************************************************
// Class tAnimatedObjectLoader
//*********************************************************
class tAnimatedObjectLoader : public tSimpleChunkHandler
{
public:
    tAnimatedObjectLoader();
    tEntity* LoadObject(tChunkFile* file, tEntityStore* store);

protected:
    ~tAnimatedObjectLoader() {};
};

//*********************************************************
// Class tAnimatedObjectAnimationLoader
//*********************************************************
class tAnimatedObjectAnimationLoader
{
public:
    bool HandlesChunk(unsigned chunkID);
    tAnimatedObjectAnimation* LoadObject(tChunkFile* file, tEntityStore* store);
};

#endif //_ANIMATED_OBJECT_HPP
