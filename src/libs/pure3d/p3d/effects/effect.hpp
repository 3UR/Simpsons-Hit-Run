/*===========================================================================
    File:: effect.hpp

    Special effects base class

    Stan Jang

    Copyright (c)1995 - 2000 Radical Entertainment, Inc.  All rights reserved.

===========================================================================*/
#ifndef _EFFECT_H
#define _EFFECT_H

#include <p3d/anim/animate.hpp>
#include <p3d/drawable.hpp>
#include <p3d/refcounted.hpp>

class tEffect;
class tEffectController;

//**********************************************************
//
// Class tEffectFactory
//
//**********************************************************
class tEffectFactory : public tEntity
{
public:
    virtual tEffect* CreateEffect(tEffectController* controller = NULL) = 0;
};

//**********************************************************
//
// Class tEffect
//
//**********************************************************
class tEffect : public tDrawable
{
public:
    virtual tEffectFactory* GetFactory() const = 0;
    virtual float           GetNumFrames() = 0;
    virtual void            SetFrame(float f) = 0;
    virtual float           GetFrame() = 0;
    virtual void            SetFrameRange(float minframe, float maxframe) = 0;
    virtual void            SetCycleMode(p3dCycleMode cyclemode) = 0;
    virtual p3dCycleMode    GetCycleMode() = 0;
    virtual void            Advance(float deltaTime) = 0;
    virtual void            Reset() = 0;
    virtual void            Display() = 0;
    virtual void            SetVisibility( bool visible ) = 0;
    virtual bool            GetVisibility() = 0;
    virtual int             LastFrameReached() = 0;
};

//**********************************************************
//
// Class tEffectController
//
//**********************************************************
class tEffectController : public tFrameController
{
public:
    tEffectController();
    tEffectController(tEffectController *c);
      
    virtual void  Advance(float deltaTime, bool update = true);   
    virtual void  Reset()                                       { effect->Reset(); accumulatedTime = 0; }
    virtual int   LastFrameReached()                            { return effect->LastFrameReached(); }
    virtual void  SetRelativeSpeed(float rs)                    { if (rs>=0.0f) relativeSpeed = rs; }
    virtual float GetRelativeSpeed()                            { return relativeSpeed; }
    virtual float GetNumFrames()                                { return effect->GetNumFrames(); }
    virtual void  SetFrame(float f)                             { effect->SetFrame(f); }
    virtual float GetFrame()                                    { return effect->GetFrame(); }
    virtual void  SetFrameRange(float minframe, float maxframe) { effect->SetFrameRange(minframe,maxframe); }
    virtual void  SetCycleMode(p3dCycleMode cyclemode)          { effect->SetCycleMode(cyclemode); }
    virtual p3dCycleMode GetCycleMode()                         { return effect->GetCycleMode(); }

    void SetEffect(tEffect* e)                                  { tRefCounted::Assign(effect,e); }
    tEffect* GetEffect() const                                  { return effect; } 
      
    virtual tFrameController *Clone(void);
    
protected:
    virtual ~tEffectController();
    virtual void Update();
    
    tEffect*        effect;
    float           relativeSpeed;
    float           accumulatedTime;
};

#endif

