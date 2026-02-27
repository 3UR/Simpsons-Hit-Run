//===========================================================================
// Copyright (C) 2003 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Text Bible Public Interface
//
// Authors:     Tony Chu
//
//===========================================================================

// Recompilation protection flag.
#ifndef SCROOBY_TEXTBIBLE_H
#define SCROOBY_TEXTBIBLE_H

#include <strings/unicodestring.h>

namespace Scrooby
{

//===========================================================================
// Interface Definitions
//===========================================================================

class TextBible
{
public:
    virtual UnicodeChar* GetWChar( unsigned int hash ) = 0;
    virtual UnicodeChar* GetWChar( const char* key ) = 0;

};

} // End namespace

#endif // End conditional inclusion
