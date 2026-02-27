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

// Recompilation protection flag.
#ifndef __ENUMCONVERSION_H
#define __ENUMCONVERSION_H


//===========================================================================
// Nested Includes
//===========================================================================
#include "Enums.h"

//===========================================================================
// Forward References
//===========================================================================

//===========================================================================
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//===========================================================================

const char* const g_JustificationLeft                   = "left";
const char* const g_JustificationRight                  = "right";
const char* const g_JustificationTop                    = "top";
const char* const g_JustificationBottom                 = "bottom";
const char* const g_JustificationCenter                 = "centre";
const char* const g_TranslucencyNormal                  = "normal";
const char* const g_TextTypeBibleString                 = "textbiblestring";
const char* const g_TextTypeHardcodedString             = "hardcodedstring";

//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: These functions will do the string <-> enumeration conversion
//
// Constraints: None
//
//===========================================================================

Scrooby::JustificationEnum StringToJustificationEnum( const char* str );

//===========================================================================
// Inlines
//===========================================================================

#endif                                  // End conditional inclusion

