//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime
//
// Description: This contains the enumeration <-> string conversion functions
//
// Authors:     Eric Tong
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================

#include <assert.h>
//#include <string.h>
#include "strings/stricmp.h"
#include "utility/EnumConversion.h"

//===========================================================================
// Local Constants, Typedefs, and Macros
//===========================================================================

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================

//===========================================================================
// Member Functions
//===========================================================================

//===========================================================================
// StringToEnums
//===========================================================================
// Description: These functions convert string to enums
//
// Constraints:    None
//
// Parameters:    str - the input string
//
// Return:      enum - the appropriate enum
//
//===========================================================================
Scrooby::JustificationEnum StringToJustificationEnum( const char* str )
{
    if( ::rstricmp( str, g_JustificationLeft ) == 0 )
    {
        return Scrooby::Left;
    }
    else if( ::rstricmp( str, g_JustificationRight ) == 0 )
    {
        return Scrooby::Right;
    }
    else if( ::rstricmp( str, g_JustificationTop ) == 0 )
    {
        return Scrooby::Top;
    }
    else if( ::rstricmp( str, g_JustificationBottom ) == 0 )
    {
        return Scrooby::Bottom;
    }
    else if( ::rstricmp( str, g_JustificationCenter ) == 0 )
    {
        return Scrooby::Centre;
    }
    else
    {
        assert( 0 );
        return Scrooby::Left;
    }
}

