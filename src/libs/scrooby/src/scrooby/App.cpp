//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby::App
//
// Description: Implementation of the Scrooby::App class.
//
// Authors:     Darwin Chau
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================
#include "App.h"
#include "FeApp.h"
#include "FeScreen.h"

using namespace Scrooby;

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================
App* App::sInstance = 0;

//===========================================================================
// Member Functions
//===========================================================================

// Since this is an interface, leave the constructor and destructor empty
App::App() 
{
}

App::~App()
{
}

//===========================================================================
// App::SetBackslashes
//===========================================================================
// Description : tells the asyncfileloader that we are using backslashes '\'
//               or forward slashes '/'
//               '/' - linux
//               '\' - everything else
//
// Constraints : None
//
// Parameters  : None
//
// Return      : None
//
//===========================================================================
void App::SetBackslashes( bool backslashesOn ) 
{
    //AsyncFileLoader::ForceBackSlashes( backslashesOn );
}

// These static members are used in place of constructors and destructors
// for this singleton class
App* App::GetInstance()
{
    if( sInstance == 0 )
    {
        sInstance = FeApp::GetInstance();
    }

    return( sInstance );
}

void App::DeleteInstance()
{
    if( sInstance != 0 )
    {
        FeApp::DeleteInstance();
        sInstance = 0;
    }
}

void App::EnableScreenScaling( bool enable, float scaleX, float scaleY, float scaleZ )
{
#ifdef SCREEN_SCALING_HACK
    g_screenScalingEnabled = enable;

    if( scaleX > 0.0f )
    {
        g_screenScaleX = scaleX;
    }

    if( scaleY > 0.0f )
    {
        g_screenScaleY = scaleY;
    }

    if( scaleZ > 0.0f )
    {
        g_screenScaleZ = scaleZ;
    }
#endif
}

