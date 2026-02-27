//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime
//
// Description: This contains the exported Scrooby Enumerations
//
// Authors:     Darwin Chau
//
//===========================================================================

// Recompilation protection flag.
#ifndef SCROOBY_ENUMS_H
#define SCROOBY_ENUMS_H

//===========================================================================
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//===========================================================================
namespace Scrooby
{

enum JustificationEnum
{
    Left,
    Right,
    Top,
    Bottom,
    Centre,
    Center = Centre
};

enum XLLanguage
{
    XL_ENGLISH          = 0,
    XL_FRENCH           = 1,
    XL_GERMAN           = 2,
    XL_UKENGLISH        = 3,
    XL_SWEDISH          = 4,
    XL_FINNISH          = 5,
    XL_ITALIAN          = 6,
    XL_SPANISH          = 7,
    XL_NORWEGIAN        = 8,
    XL_JAPANESE         = 9,
    XL_LAST_LANGUAGE    = 10        // number of languages present
};

const char language_ID[XL_LAST_LANGUAGE]=
{
    'E',      // English
    'F',      // French
    'G',      // German
    'U',      // Uk english
    'D',      // sweDish
    'N',      // fiNnish
    'I',      // Italian
    'S',      // Spanish
    'R',      // noRwegian
    'J',      // Japanese
};

enum ErrorLevel
{
    LVL_DEBUG   = 0,  // Log anything that might be handy to this level
    LVL_INFO    = 1,  // This should represent messages when something of relative importance is occuring
    LVL_WARNING = 2,  // Use this level when it's something which is not critical, but important
    LVL_ERROR   = 3,  // Only critical messages should be on this level
    LVL_SILENT  = 4   // Do NOT log on this level
};

enum TextMode
{
    TEXT_OVERLAP,
    TEXT_CLIP,
    TEXT_WRAP
};


} // End namespace

#endif  // End conditional inclusion
