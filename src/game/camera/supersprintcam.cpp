//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        supersprintcam.cpp
//
// Description: Implement SuperSprintCam
//
// History:     2/8/2003 + Created -- Cary Brisebois
//
//=============================================================================

//========================================
// System Includes
//========================================
// Foundation Tech
#include <raddebug.hpp>
#include <radmath/radmath.hpp>

#include <p3d/vectorcamera.hpp>

//========================================
// Project Includes
//========================================
#include <camera/supersprintcam.h>
#include <camera/supercamconstants.h>
#include <camera/supercamcontroller.h>

#include <mission/gameplaymanager.h>

//*****************************************************************************
//
// Global Data, Local Data, Local Classes
//
//*****************************************************************************
unsigned int SuperSprintCam::mSprintCamCount = 0;

#ifdef RAD_UWP
static float CAMERA_VALUES[ RenderEnums::MAX_LEVEL - RenderEnums::numLevels ][ 3 ] = 
{
//    { 49.99f, 160.21f, -233.60f },
    { 49.99f, 160.21f, -233.60f },
    { 36.79f, 213.18f, -224.51f },
    { -35.01f, 164.01f, -234.05f },
    { 37.59f, 347.682f, -316.03f },
    { 38.66f, 152.70f, -215.20f },
    { -0.729f, 149.45f, -235.30f },
    { 54.581f, 178.399f, -220.06f }
};

//Theres a 1-1 bonus to main levels.
static rmt::Vector* CAMERA_SETTINGS = (rmt::Vector*)(CAMERA_VALUES); 

static float CAMERA_FOV[ RenderEnums::MAX_LEVEL - RenderEnums::numLevels ] =
{
//    0.2618f,
    0.4f,
    0.4f,
    0.35f,
    0.26f,
    0.4f,
    0.42f,
    0.4f
};

#endif

#ifdef RAD_WIN32
//Straight copy of xbox. Will figure out later.
static float CAMERA_VALUES[ RenderEnums::MAX_LEVEL - RenderEnums::numLevels ][ 3 ] = 
{
//    {-14.268f, 322.9046f, -313.13299f},
    {-14.268f, 322.9046f, -313.13299f},
    {-10.0362f, 290.3872f, -336.8332f},
    {-14.268f, 322.9046f, -313.13299f},
    {-12.757f, 261.6806f, -341.0744f},
    {-14.268f, 322.9046f, -313.13299f},
    {-14.268f, 322.9046f, -313.13299f},
    {-14.268f, 322.9046f, -313.13299f}
};

//Theres a 1-1 bonus to main levels.
static rmt::Vector* CAMERA_SETTINGS = (rmt::Vector*)(CAMERA_VALUES); 

static float CAMERA_FOV[ RenderEnums::MAX_LEVEL - RenderEnums::numLevels ] =
{
//    0.2618f,
    0.2618f,
    0.2618f,
    0.2618f,
    0.2618f,
    0.2618f,
    0.2618f,
    0.2618f
};

#endif

//*****************************************************************************
//
// Public Member Functions
//
//*****************************************************************************

//=============================================================================
// SuperSprintCam::SuperSprintCam
//=============================================================================
// Description: Constructor.
//
// Parameters:  None.
//
// Return:      N/A.
//
//=============================================================================
SuperSprintCam::SuperSprintCam() :
    mNear( 150.0f ),
    mFar( 800.0f ),
    mFOV( rmt::PI_BY2 ),
    mAspect( 4.0f / 3.0f ),
    mLag( 0.05f ),
    mDist( 10.0f )
{
}

//=============================================================================
// SuperSprintCam::~SuperSprintCam
//=============================================================================
// Description: Destructor.
//
// Parameters:  None.
//
// Return:      N/A.
//
//=============================================================================
SuperSprintCam::~SuperSprintCam()
{
}

//*****************************************************************************
//
// Protected Member Functions
//
//*****************************************************************************

//=============================================================================
// SuperSprintCam::Update
//=============================================================================
// Description: Comment
//
// Parameters:  ( unsigned int milliseconds )
//
// Return:      void 
//
//=============================================================================
void SuperSprintCam::Update( unsigned int milliseconds )
{
    if ( GetFlag( (Flag)FIRST_TIME ) )
    {
        //Get the position of the vector cam.
        //Find and set up the camera.
        tVectorCamera* cam = p3d::find<tVectorCamera>("sprint_camShape");

//        mPos.Set( 4.847f, 240.5329f, -234.17244f );
        mPos = CAMERA_SETTINGS[ GetGameplayManager()->GetCurrentLevelIndex() - 1 ];
        mTarg.Set( 0.0f, 0.0f, 0.0f );

        mFOV = CAMERA_FOV[ GetGameplayManager()->GetCurrentLevelIndex() - 1 ];
        SetFOV( mFOV );

        mAspect = 4.0f / 3.0f;
        SetAspect( mAspect );

        mUp.Set( -0.019949f, 0.6996f, 0.714194f );

        mPosDelta.Set( 0.0f, 0.0f, 0.0f );

#ifndef RAD_RELEASE
//        InitMyController( s_secondaryControllerID );
#endif
        SetFlag( (Flag)FIRST_TIME, false );
    }
    else
    {
#ifndef RAD_RELEASE     
//        float x, y, z;
//        x = mController->GetValue( SuperCamController::stickX );
//        y = mController->GetValue( SuperCamController::leftStickY );
//        z = mController->GetValue( SuperCamController::stickY );

//        if ( rmt::Epsilon( x, 0.0f, 0.001f ) && rmt::Epsilon( y, 0.0f, 0.001f ) && rmt::Epsilon( z, 0.0f, 0.001f ) )
//        {
#endif
            GetPosition( &mPos );
            GetTarget( &mTarg );
            GetCameraUp( &mUp );
#ifndef RAD_RELEASE
//        }
//        else
//        {
//            //Use the controller values to move the camera.
//
//            rmt::Vector xMove( mDist, 0.0f, 0.0f );
//            xMove.Scale( x );
//
//            rmt::Vector yMove( 0.0f, -mDist, 0.0f );
//            yMove.Scale( y );
//
//            rmt::Vector zMove( 0.0f, 0.0f, mDist );
//            zMove.Scale( z );
//
//            GetPosition( &mPos );
//            rmt::Vector desiredPos( mPos );
//            desiredPos.Add( xMove );
//            desiredPos.Add( yMove );
//            desiredPos.Add( zMove );
//
//            float timeMod = milliseconds / 16.0f;
//
//            float lag = mLag * timeMod;
//            CLAMP_TO_ONE( lag );
//
//            MotionCubic( &mPos.x, &mPosDelta.x, desiredPos.x, lag );
//            MotionCubic( &mPos.y, &mPosDelta.y, desiredPos.y, lag );
//            MotionCubic( &mPos.z, &mPosDelta.z, desiredPos.z, lag );
//        }
#endif
    }

    SetNearPlane( mNear );
    SetFarPlane( mFar );
    SetFOV( mFOV );
    SetAspect( mAspect );
    SetCameraValues( milliseconds, mPos, mTarg );

    SetFlag( (Flag)CUT, false );
}

//=============================================================================
// SuperSprintCam::OnInit
//=============================================================================
// Description: Comment
//
// Parameters:  ()
//
// Return:      void 
//
//=============================================================================
void SuperSprintCam::OnInit()
{
}

//=============================================================================
// SuperSprintCam::OnShutdown
//=============================================================================
// Description: Comment
//
// Parameters:  ()
//
// Return:      void 
//
//=============================================================================
void SuperSprintCam::OnShutdown()
{
}

//=============================================================================
// SuperSprintCam::OnRegisterDebugControls
//=============================================================================
// Description: Comment
//
// Parameters:  ()
//
// Return:      void 
//
//=============================================================================
void SuperSprintCam::OnRegisterDebugControls()
{
#ifdef DEBUGWATCH
    if ( mSprintCamCount == 0 )
    {
        char nameSpace[256];
        sprintf( nameSpace, "SuperCam\\SuperSprint" );

        radDbgWatchAddFloat( &mNear,   "mNear",   nameSpace, NULL, NULL, 1.0, 2000.0f );
        radDbgWatchAddFloat( &mFar,    "mFar",    nameSpace, NULL, NULL, 1.0, 2000.0f );
        radDbgWatchAddFloat( &mFOV,    "mFOV",    nameSpace, NULL, NULL, 0.001f,   rmt::PI );
        radDbgWatchAddFloat( &mAspect, "mAspect", nameSpace, NULL, NULL, 0.001f,      3.0f );

        radDbgWatchAddVector(&(mPos.x),  "mPos",    nameSpace, NULL, NULL, -2000.0f, 2000.0f );
        radDbgWatchAddVector(&(mTarg.x), "mTarg",   nameSpace, NULL, NULL, -2000.0f, 2000.0f );
        radDbgWatchAddVector(&(mUp.x),   "mUp",     nameSpace, NULL, NULL, -2000.0f, 2000.0f );

        radDbgWatchAddFloat( &mDist, "Dist", nameSpace, NULL, NULL, 1.0f, 1000.0f );
    }
#endif
    mSprintCamCount++;
}

//=============================================================================
// SuperSprintCam::OnUnregisterDebugControls
//=============================================================================
// Description: Comment
//
// Parameters:  ()
//
// Return:      void 
//
//=============================================================================
void SuperSprintCam::OnUnregisterDebugControls()
{
#ifdef DEBUGWATCH
    if ( mSprintCamCount == 1 )
    {
        radDbgWatchDelete( &mNear );
        radDbgWatchDelete( &mFar );
        radDbgWatchDelete( &mFOV );
        radDbgWatchDelete( &mAspect );

        radDbgWatchDelete( &mPos.x );
        radDbgWatchDelete( &mTarg.x );
        radDbgWatchDelete( &mUp.x );

        radDbgWatchDelete( &mDist );
    }
#endif
    mSprintCamCount--;
}


//*****************************************************************************
//
// Private Member Functions
//
//*****************************************************************************
