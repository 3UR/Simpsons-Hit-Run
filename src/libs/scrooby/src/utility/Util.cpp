//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Global utility functions
//
// Description: None
//
// Authors:     Eric Tong
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================
#include "strings/pcstring.h"
#include <string.h>

//===========================================================================
// Local Constants, Typedefs, and Macros
//===========================================================================

namespace Scrooby
{

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================

PascalCString StripFileExtension( const char* string, const char* ext )
{
    int numChars = strlen( string ) - strlen( ext );
    PascalCString completeString = string;
    PascalCString extString = completeString.SubString( numChars, strlen( ext ) );
    if( extString.EqualsInsensitive( ext ) )
    {
        return completeString.SubString( 0, numChars );
    }
    return string;
}

//===========================================================================
// Member Functions
//===========================================================================

}    //end namespace
