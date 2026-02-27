//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   FeResourceEntry
//
// Description: Implementation of the FeResourceEntry class.
//
// Authors:     Darwin Chau
//
// Revisions        Date            Author        Revision
//                  2000/07/17        DChau        Created
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================
#include "FeResourceEntry.h"
#include <raddebug.hpp>
#include <string.h>  // DARWIN TODO: ditch this if we don't need to store string for the alias
#include "utility/memory.h"


//===========================================================================
// Public Member Functions
//===========================================================================

//===========================================================================
// FeResourceEntry::FeResourceEntry
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
FeResourceEntry::FeResourceEntry
( 
    const char* alias,
    unsigned int resourceHandle
)
:
    FeEntity(),    
    mResourceHandle( resourceHandle )
{
    rAssert( NULL != alias );
    
    //
    // Has the alias and store it.  This is how we'll look it up later.
    //
    FeEntity::SetUID( FeEntity::MakeUID( alias ) );

    //
    // DARWIN TODO:  Do we really need to store this string since we
    // can use the hash value for lookup?  Does anyone actually ever
    // need to use the string itself?
    // 
    /*mAlias = static_cast< char* >( ScroobyGetMemory( ScroobyTempPool, strlen( alias ) + 1 ) );
    rValid( mAlias ); 
    strcpy( mAlias, alias );*/
}
        

//===========================================================================
// FeResourceEntry::~FeResourceEntry
//===========================================================================
// Description: Destructor.
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
FeResourceEntry::~FeResourceEntry()
{
    /*if( NULL != mAlias )
    {
        delete [] mAlias;
    }*/
}
