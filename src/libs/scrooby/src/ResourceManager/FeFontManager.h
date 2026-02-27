//=============================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All Rights Reserved
//
// Component   : Scrooby
//
// Description : A font control system for the many different fonts and text
//               styles.
//
// Author(s)   : Mike Perzel(12/6/2001)
//
//=============================================================================

// Recompillation Protection
#ifndef FE_FONT_MANAGER
#define FE_FONT_MANAGER

//=============================================================================
// Nested Includes
//=============================================================================
#include "FeEntity.h"

//=============================================================================
// Forward References
//=============================================================================

//=============================================================================
// Constants, Typedefs, and Macro Definitions
//=============================================================================

//=============================================================================
// Interface Definitions
//=============================================================================

class FeFontManager : public FeEntity
//=============================================================================
// Description : This class  will manage fonts.  It has been separated in order 
//               to allow different methods of font resource control, while 
//               still seeming generic to the rest of the components
//
//=============================================================================
{
public:
    FeFontManager();

    void AddFontFilename( const char* filename );

    void SetFontsReused( bool reuse );
    bool IsFontsReused();    

    void SetDefaultFontSize( int size );
    int GetDefaultFontSize();
    
protected:
    ~FeFontManager();

private:
    bool mbReuseFonts;
    int mDefaultFontSize;

};

//=============================================================================
// Inlines
//=============================================================================

//=============================================================================
// thisclass::MemberFunction
//=============================================================================
// Description :
//
// Constraints :
//
// Parameters  :
//
// Return      :
//
//=============================================================================

// end conditional define
#endif