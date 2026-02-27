//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        SuperCamController.cpp
//
// Description: Implement SuperCamController
//
// History:     14/05/2002 + Created -- NAME
//
//=============================================================================

//========================================
// System Includes
//========================================
// Foundation Tech
#include <raddebug.hpp>

//========================================
// Project Includes
//========================================
#ifndef WORLD_BUILDER
#include <camera/animatedcam.h>
#include <camera/supercammanager.h>
#endif

#include <camera/SuperCamController.h>

//******************************************************************************
//
// Global Data, Local Data, Local Classes
//
//******************************************************************************

//******************************************************************************
//
// Public Member Functions
//
//******************************************************************************

//==============================================================================
// SuperCamController::SuperCamController
//==============================================================================
// Description: Constructor.
//
// Parameters:  None.
//
// Return:      N/A.
//
//==============================================================================
SuperCamController::SuperCamController() :
    Mappable( Input::ACTIVE_GAMEPLAY     | 
              Input::ACTIVE_FIRST_PERSON | 
              Input::ACTIVE_SS_GAME      |
              Input::ACTIVE_ANIM_CAM ),
    mIsWheel( false ) 
{
}

//==============================================================================
// SuperCamController::~SuperCamController
//==============================================================================
// Description: Destructor.
//
// Parameters:  None.
//
// Return:      N/A.
//
//==============================================================================
SuperCamController::~SuperCamController()
{
}

//=============================================================================
// SuperCamController::LoadControllerMappings
//=============================================================================
// Description: Comment
//
// Parameters:  ( unsigned int controllerId )
//
// Return:      void 
//
//=============================================================================
void SuperCamController::LoadControllerMappings( unsigned int controllerId )
{
    #ifdef RAD_UWP
    ClearMap(0);
    Map( "RightStickX",     stickX,             0, controllerId );
    Map( "RightStickY",     stickY,             0, controllerId );
    Map( "LeftTrigger",     zToggle,            0, controllerId );
    Map( "RightTrigger",    lookToggle,         0, controllerId );
    Map( "A",               A,                  0, controllerId );
    Map( "RightThumb",      toggleFirstPerson,  0, controllerId );
    Map( "LeftStickY",      leftStickY,         0, controllerId );
    Map( "Start",           start,              0, controllerId );
    Map( "Black",           cameraToggle,       0, controllerId );
    #endif

    #ifdef RAD_WIN32
    ClearMap(0);
    Map( "CameraLeft", stickXdown, 0, controllerId );
    Map( "CameraRight", stickXup, 0, controllerId );
    Map( "CameraMoveIn", stickYup, 0, controllerId );
    Map( "CameraMoveOut", stickYdown, 0, controllerId );
    Map( "CameraZoom", zToggle, 0, controllerId );
    Map( "CameraLookUp", lookToggle, 0, controllerId );
    Map( "CameraCarLeft", carLookLeft, 0, controllerId );
    Map( "CameraCarRight", carLookRight, 0, controllerId );
    Map( "CameraCarLookUp", carLookUp, 0, controllerId );
    Map( "CameraCarLookBack", lookBack, 0, controllerId );
    Map( "CameraToggle", cameraToggle, 0, controllerId );    
    Map( "feSelect", A, 0, controllerId );
    Map( "feMouseUp", mouseLookUp, 0, controllerId );
    Map( "feMouseDown", mouseLookDown, 0, controllerId );
    #endif
}

//=============================================================================
// SuperCamController::OnButtonDown
//=============================================================================
// Description: Comment
//
// Parameters:  ( int controllerId, int buttonId, const IButton* pButton )
//
// Return:      void 
//
//=============================================================================
void SuperCamController::OnButtonDown( int controllerId, int buttonId, const IButton* pButton )
{
#ifndef WORLD_BUILDER
    //
    // Tell the animated camera to skip
    //
    if( ( buttonId == A ) || ( buttonId == start ) )
    {
        AnimatedCam::Skip();
    }

    if ( buttonId == toggleFirstPerson )
    {
        //This is ugly.
        GetSuperCamManager()->ToggleFirstPerson( controllerId );
    }
#endif
}

//******************************************************************************
//
// Private Member Functions
//
//******************************************************************************
