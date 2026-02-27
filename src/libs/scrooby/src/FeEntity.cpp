//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   FeEntity
//
// Description: Implementation of the FeEntity class.
//
// Authors:     Wilkin Ng
//
// Revisions        Date            Author        Revision
//                  2000/08/28        WNg          Created
//
//===========================================================================


//===========================================================================
// Includes
//===========================================================================
#include "FeEntity.h"
#ifdef SCROOBY_MEMORY_CHECKING
#include "utility/debugmessages.h"
#include "radtime.hpp"
#endif

//===========================================================================
// Public Member Functions
//===========================================================================

//===========================================================================
// FeEntity::FeEntity()
//===========================================================================
// Description: Constructor.
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      N/A.
//
//===========================================================================
FeEntity::FeEntity()
{
#ifdef SCROOBY_MEMORY_CHECKING
    static bool initializedTime = false;
    if( !initializedTime )
    {
        ::radTimeInitialize();
        initializedTime = true;
    }
    Scrooby::Log::Message( Scrooby::LVL_DEBUG, "FeEntity 0x%8.8x %u ****************", this, ::radTimeGetMicroseconds() );
    Scrooby::Log::Message( Scrooby::LVL_DEBUG, "FeEntity 0x%8.8x %u - created", this, ::radTimeGetMicroseconds() );
#endif
}


//===========================================================================
// FeEntity::~FeEntity()
//===========================================================================
// Description: Destructor.
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      N/A.
//
//===========================================================================
FeEntity::~FeEntity()
{
#ifdef SCROOBY_MEMORY_CHECKING
    Scrooby::Log::Message( Scrooby::LVL_DEBUG, "FeEntity 0x%8.8x %u - destroyed (%s)", this, ::radTimeGetMicroseconds(),GetName() );
#endif
}

#ifdef SCROOBY_MEMORY_CHECKING
void FeEntity::SetName( const char* n )
{
    tEntity::SetName( n );
    Scrooby::Log::Message( Scrooby::LVL_DEBUG, "FeEntity 0x%8.8x %u - given name %s", this, ::radTimeGetMicroseconds(), n );
}

void FeEntity::AddRef()
{
    tEntity::AddRef();
    Scrooby::Log::Message( Scrooby::LVL_DEBUG, "FeEntity 0x%8.8x %u - AddRef (%d)", this, ::radTimeGetMicroseconds(), refCount );
}

void FeEntity::Release()
{
    Scrooby::Log::Message( Scrooby::LVL_DEBUG, "FeEntity 0x%8.8x %u - Release (%d)", this, ::radTimeGetMicroseconds(), refCount );
    tEntity::Release();
}
#endif

//===========================================================================
// FeEntity::IsOwner
//===========================================================================
// Description: quick test to determine if this object is of a certian class
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      true or false - is this object of the class
//
//===========================================================================
bool FeEntity::IsOwner() const
{
    //by default, an object is NOT an owner
    return false;
}

