//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        
//
// Description: Implement Locator
//
// History:     04/04/2002 + Created -- Cary Brisebois
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
#include <meta/Locator.h>
#else
#include "Locator.h"
#endif

#include <meta/locatorevents.h>
#include <meta/locatortypes.h>

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
// Locator::Locator
//==============================================================================
// Description: Constructor.
//
// Parameters:	None.
//
// Return:      N/A.
//
//==============================================================================
Locator::Locator() :
    mID( ~0 ),
    mData( 0 ),
    mUserData( nullptr ),
    mFlags( 0 ),
    mLocation( rmt::Vector( 0.0f, 0.0f, 0.0f ) )
{
    SetFlag( ACTIVE, true );
}

//==============================================================================
// Locator::~Locator
//==============================================================================
// Description: Destructor.
//
// Parameters:	None.
//
// Return:      N/A.
//
//==============================================================================
Locator::~Locator()
{
}

//=============================================================================
// Locator::GetPosition
//=============================================================================
// Description: Comment
//
// Parameters:  ( rmt::Vector* currentLoc )
//
// Return:      void 
//
//=============================================================================
void Locator::GetPosition( rmt::Vector* currentLoc )
{
    GetLocation( currentLoc );
}

//=============================================================================
// Locator::GetHeading
//=============================================================================
// Description: Comment
//
// Parameters:  ( rmt::Vector* heading )
//
// Return:      void 
//
//=============================================================================
void Locator::GetHeading( rmt::Vector* heading )
{
    //Locators have no heading.
    heading->Set( 0.0f, 0.0f, 0.0f );
}

const char* const LocatorEvent::Name[] =
{
    "Flag",
    "Camera Cut",
    "Check Point",
    "Base",
    "Death",
    "Interior Exit",
    "Bounce Pad",
    "Ambient Sound - City",
    "Ambient Sound - Seaside",
    "Ambient Sound - Suburbs",
    "Ambient Sound - Forest",
    "Ambient Sound - KEM Rooftop",
    "Ambient Sound - Farm",
    "Ambient Sound - Barn",
    "Ambient Sound - PP - Interior",
    "Ambient Sound - PP - Exterior",
    "Ambient Sound - River",
    "Ambient Sound - Business",
    "Ambient Sound - Construction",
    "Ambient Sound - Stadium",
    "Ambient Sound - Stadium Tunnel",
    "Ambient Sound - Expressway",
    "Ambient Sound - Slum",
    "Ambient Sound - Railyard",
    "Ambient Sound - Hospital",
    "Ambient Sound - Light City",
    "Ambient Sound - Shipyard",
    "Ambient Sound - Quay",
    "Ambient Sound - Lighthouse",
    "Ambient Sound - Country Highway",
    "Ambient Sound - Krustylu",
    "Ambient Sound - Dam",

    "Ambient Sound - Forest Highway",
    "Ambient Sound - Retaining Wall",
    "Ambient Sound - Krustylu Ext.",
    "Ambient Sound - Duff Exterior",
    "Ambient Sound - Duff Interior",
    "Ambient Sound - Stonecutter Tunnel",
    "Ambient Sound - stonecutter Hall",
    "Ambient Sound - Sewers",
    "Ambient Sound - Burns Tunnel",
    "Ambient Sound - PP Room 1",
    "Ambient Sound - PP Room 2",
    "Ambient Sound - PP Room 3",
    "Ambient Sound - PP Tunnel 1",
    "Ambient Sound - PP Tunnel 2",
    "Ambient Sound - Mansion Interior",
    "Park Birds",
    "Whacky Gravity",
    "Far Plane Change",

    "Ambient Sound - Country Night",
    "Ambient Sound - Suburbs Night",
    "Ambient Sound - Forest Night",
    "Ambient Sound - Halloween1",
    "Ambient Sound - Halloween2",
    "Ambient Sound - Halloween3",
    "Ambient Sound - Placeholder3",
    "Ambient Sound - Placeholder4",
    "Ambient Sound - Placeholder5",
    "Ambient Sound - Placeholder6",
    "Ambient Sound - Placeholder7",
    "Ambient Sound - Placeholder8",
    "Ambient Sound - Placeholder9",
    "Goo Damage",
    "Coin Zone",        //Not used, just loaded.
    "Light Change",
    "Trap",
    "Ambient Sound - Seaside Night",
    "Ambient Sound - Lighthouse Night",
    "Ambient Sound - Brewery Night",
    "Ambient Sound - Placeholder10",
    "Ambient Sound - Placeholder11",
    "Ambient Sound - Placeholder12",
    "Ambient Sound - Placeholder13",
    "Ambient Sound - Placeholder14",
    "Ambient Sound - Placeholder15",
    "Ambient Sound - Placeholder16",
    //This and below not used in any offline tool!
    "Dynamic Zone",
    "Occlusion Zone",
    "Car Door",
    "Action Button",
    "Interior Entrance",
    "Generic Button Handler Event",
    "Jump on Fountain",
    "Load Pedestrian Model Group",
    "Gag"
};
const char* const LocatorType::Name[] =
{
    "Event",
    "Script",
    "Generic",
    "Car Start",
    "Spline",
    "Dynamic Zone",
    "Occlusion",
    "Interior Entrance",
    "Directional",
    "Action",
    "FOV",
    "Breakable Camera",
    "Static Camera",
    "Ped Group",
    "Coin",
    "Spawn Point"
};