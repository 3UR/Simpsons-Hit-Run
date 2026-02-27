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

// Recompilation protection flag.
#ifndef __SCROOBYUTIL_H
#define __SCROOBYUTIL_H

//===========================================================================
// Nested Includes
//===========================================================================

#include "strings/pcstring.h"

//===========================================================================
// Forward References
//===========================================================================

namespace Scrooby
{

//===========================================================================
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//===========================================================================

//===========================================================================
// Interface Definitions
//===========================================================================

PascalCString StripFileExtension( const char* string, const char* ext );

//===========================================================================
// Inlines
//===========================================================================

}    //end namespace

#endif                                  // End conditional inclusion
