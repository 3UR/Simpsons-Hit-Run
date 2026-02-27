//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        language.cpp
//
// Description: contains functions for dealing with language
//
// History:     21/1/2002 + Created -- Ian Gipson
//
//=============================================================================

//========================================
// Project Includes
//========================================

#include <presentation/language.h>

//*****************************************************************************
//
// Public Member Functions
//
//*****************************************************************************
namespace Language{

//=============================================================================
// Language::GetHardwareLanguage()
//=============================================================================
// Description: returns the currently set language for the console.
//
// Parameters: None.
//
// Return:      Language enum specifying the current language
//
//=============================================================================
Language GetHardwareLanguage()
{
    // TODO(3ur): Keeping this here for ref. UWP should support this?
    #if 0
    ////////////////////////////////////////////////////////////
    // XBOX
    ////////////////////////////////////////////////////////////
    switch ( XGetLanguage() )
    {
        case XC_LANGUAGE_ENGLISH : 
        {
            return ENGLISH;
        }
        case XC_LANGUAGE_FRENCH : 
        {
            return FRENCH;
        }
        case XC_LANGUAGE_GERMAN : 
        {
            return GERMAN;
        }
        case XC_LANGUAGE_ITALIAN : 
        {
            return ITALIAN;
        }
        case XC_LANGUAGE_JAPANESE : 
        {
            return JAPANESE;
        }
        case XC_LANGUAGE_SPANISH : 
        {
            return SPANISH;
        }
        default : 
        {
            return UNKNOWN;
        }
    }
    #endif
    
    // TODO(3ur): above. adding uwp here for temp
    #if defined(RAD_WIN32) || defined(RAD_UWP)
    ////////////////////////////////////////////////////////////
    // WIN32
    ////////////////////////////////////////////////////////////
    return ENGLISH;     // to be implemented.
    #endif
}

} //namespace Language