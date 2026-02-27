/*===========================================================================
    transitionmanager.cpp
    04-03-2002 Created by Mike Perzel

    Copyright (c)2002 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _TRANSITION_MANAGER
#define _TRANSITION_MANAGER

#include <radmath/radmath.hpp>
#include <p3d/entity.hpp>

class tTexture;
class tShader;

class tTransitionManager : public tEntity
{
public:
    tTransitionManager();
    
    void Display();
    void Advance( float delta = 16.0f );
    void StartTransition();

    void SetTransitionTime( float time ) { mTime = time; }
    float GetTransitionTime() { return mTime; }

    void SetTransitionType( unsigned int type ) { mType = type; }
    unsigned int GetTransitionType() { return mType; }

    void SetAnimCurveExponent( unsigned int exponent) { mExponent = exponent; }
    unsigned int GetAnimCurveExponent() { return mExponent; }

    bool IsDrawing() { return mDrawing; }
    bool IsGrabbing() { return mGrabbing; }

    enum TransitionType
    {
        TRANSITION_NONE=0,
        TRANSITION_FADE_TO_ALPHA,
        TRANSITION_FADE_TO_BLACK,
        TRANSITION_SLIDE_LEFT,
        TRANSITION_SLIDE_RIGHT,
        TRANSITION_SLIDE_TOP,
        TRANSITION_SLIDE_BOTTOM,
        TRANSITION_SHRINK,
        TRANSITION_SPIN_AND_SHRINK,
        TRANSITION_EXPLODE,
        TRANSITION_SHATTER,
        TRANSITION_SPLIT_SLIDE_VERTICAL,
        TRANSITION_SPLIT_SLIDE_HORIZONTAL,
        NUMBER_OF_TRANSITIONS
    };

protected:
    ~tTransitionManager();

    struct overlayPoint
    {
        pddiVector pos;
        pddiVector2 uv;
    };
    struct overlayTri
    {
        overlayPoint point[3];
    };
    struct overlayQuad
    {
        overlayTri tri[2];
    };


    tTexture* mTexture;
    tShader* mShader;
    tColour mColour;
    rmt::Vector mVector;
    rmt::Randomizer mRand;

    bool mGrabbing;
    bool mDrawing;
    unsigned int mType;
    unsigned int mExponent;
    float mTimeElapsed;
    float mTime;
    float mScale;
    float mRotation;

    // Polygon Attributes
    int mXSize;
    int mYSize;
    overlayQuad* mPositions;

private:
};

#endif
