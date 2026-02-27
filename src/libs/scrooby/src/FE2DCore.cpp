// Fe2DCore.cpp
// Created by wng on Wed, Apr 05, 2000 @ 12:39 PM.
#include "stdafx.h"

#ifndef __FE2DCore__
#include "FE2DCore.h"
#endif

#include "utility/memory.h"
#include <utility.hpp>
#include <raddebug.hpp>


//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================
class Fe2DCore;
Fe2DCore* Fe2DCore::sInstance = NULL;

//===========================================================================
// Member Functions
//===========================================================================


//===========================================================================
// Fe2DCore::GetInstance
//===========================================================================
// Description: This method is the only was for clients to access this
//              singleton.
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      Pointer to the single instance of this class.
//
//===========================================================================
Fe2DCore* Fe2DCore::GetInstance()
{
    if( sInstance == NULL )
    {
        sInstance = new ( ScroobyPermPool ) Fe2DCore;
    }

    return( sInstance );
}


//===========================================================================
// Fe2DCore::DeleteInstance
//===========================================================================
// Description: Frees the single allocated instance of this class.  The
//              client must call this during shutdown.
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
void Fe2DCore::DeleteInstance()
{
    if( sInstance != NULL )
    {
        delete sInstance;
        sInstance = NULL;
    }
}



Fe2DCore::Fe2DCore() 
{
}


Fe2DCore::~Fe2DCore()
{
}

void Fe2DCore::AddSection(char *sectionname)
{
    p3d::inventory->AddSection(sectionname);
}

void Fe2DCore::RemoveSectionElement(char *name)
{
    p3d::inventory->RemoveSectionElements(name);
}

tDrawable *Fe2DCore::GetHandlePrimitive(unsigned handle)
{
    rAssert( scene[ handle ].Used() );
//    rAssert( scene[ handle ].GetTDrawable() );

    return scene[ handle ].GetTDrawable();
}

void Fe2DCore::DeleteSection(char *sectionname)
{
    p3d::inventory->DeleteSection(sectionname);
}




