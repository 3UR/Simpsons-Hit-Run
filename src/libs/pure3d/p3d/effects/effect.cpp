/*===========================================================================
    effect.cpp
    20-Sept-2000 Created by Kevin Voon

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#include <p3d/effects/effect.hpp>

//**********************************************************
//
// Class tEffectController
//
//**********************************************************
tEffectController::tEffectController():
    effect(NULL),
    relativeSpeed(1.0f),
    accumulatedTime(0.0f)
{
}

tEffectController::tEffectController(tEffectController *c):
    effect(NULL),
    relativeSpeed(c->relativeSpeed),
    accumulatedTime(c->accumulatedTime)
{
    CopyName(c);   
    SetEffect(c->GetEffect());
}

tFrameController *tEffectController::Clone(void)
{
    return new tEffectController(this);
}

void tEffectController::Update()
{
    effect->Advance(accumulatedTime);
    accumulatedTime = 0.0f;
}

void tEffectController::Advance(float deltaTime, bool update)
{
    accumulatedTime += (deltaTime * relativeSpeed);
    
    if (update)
    {
        Update();
    }
}

tEffectController::~tEffectController()
{
    tRefCounted::Release(effect);
}

