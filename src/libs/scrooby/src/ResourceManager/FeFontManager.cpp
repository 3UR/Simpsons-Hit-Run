//=============================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All Rights Reserved.
//
// Component   : Scrooby
//
// Description : A font control system for the many different fonts and text
//               styles.
//
// Author(s)   : Mike Perzel(12/6/2001)
//
//=============================================================================

//=============================================================================
// Includes
//=============================================================================
#include "FeFontManager.h"

//=============================================================================
// Local constants, typedefs, macros
//=============================================================================
const bool DefaultFontReuse = false;
const int  DefalutFontSize  = 20;

//=============================================================================
// Global data, local data, local classes
//=============================================================================

//=============================================================================
// Member functions
//=============================================================================

//=============================================================================
// FeFontManager::FeFontManager
//=============================================================================
// Description : constructor
//
//=============================================================================
FeFontManager::FeFontManager()
:
mbReuseFonts( DefaultFontReuse ),
mDefaultFontSize( DefalutFontSize )
{

}

//=============================================================================
// FeFontManager::FeFontManager
//=============================================================================
// Description : deconstructor
//
// Constraints : should only be called by Release()
//
//=============================================================================
FeFontManager::~FeFontManager()
{

}

//=============================================================================
// FeFontManager::AddFontFilename
//=============================================================================
// Description : Adds a Freetype font to the system that can be used.
//
// Constraints : Adding filenames in here may be costly on memory
//
// Parameters  : filename - a filename for a font to be added to the manager
//
//=============================================================================
void FeFontManager::AddFontFilename( const char* filename )
{

}

//=============================================================================
// FeFontManager::SetFontsReused
//=============================================================================
// Description : Sets the font loading behavior of the manager
//
// Constraints : reusing fonts will be cheaper for memory, but may not look as
//               good
//
// Parameters  : reuse - true if the fonts will be reused, false otherwise
//
//=============================================================================
void FeFontManager::SetFontsReused( bool reuse )
{
    mbReuseFonts = reuse;
}

//=============================================================================
// FeFontManager::IsFontsReused
//=============================================================================
// Description : returns the current loading method
//
// Return      : true if fonts are going to be reused, false otherwise
//
//=============================================================================
bool FeFontManager::IsFontsReused()
{
    return mbReuseFonts;
}

//=============================================================================
// FeFontManager::SetFontsReused
//=============================================================================
// Description : Sets the default font size if fonts are going to be reused
//
// Constraints : Used only if font reuse is enabled (See IsFontsReused())
//
// Parameters  : size - new integral font height
//
//=============================================================================
void FeFontManager::SetDefaultFontSize( int size )
{
    mDefaultFontSize = size;
}

//=============================================================================
// FeFontManager::IsFontsReused
//=============================================================================
// Description : returns the default font size if font reuse is enabled
//
// Constraints : Used only if font reuse is enabled (See IsFontsReused())
//
// Return      : an integral size for the font height
//
//=============================================================================
int FeFontManager::GetDefaultFontSize()
{
    return mDefaultFontSize;
}