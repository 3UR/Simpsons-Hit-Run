//=============================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All Rights Reserved
//
// Component   : Scrooby
//
// Description : Container for a font style
//
// Author(s)   : Mike Perzel
//
//=============================================================================

// Recompillation Protection
#ifndef FE_TEXT_STYLE
#define FE_TEXT_STYLE

//=============================================================================
// Nested Includes
//=============================================================================
#include "FeEntity.h"

//=============================================================================
// Forward References
//=============================================================================
class PascalCString;

//=============================================================================
// Constants, Typedefs, and Macro Definitions
//=============================================================================

//=============================================================================
// Interface Definitions
//=============================================================================

class FeTextStyle : public FeEntity
//=============================================================================
// Description : Class to contain the properties that different text types can
//               have
//
//=============================================================================
{
public:
    FeTextStyle();

    int  GetSize();
    void SetSize( int size );

    const PascalCString& GetFontName();
    void                 SetFontName( PascalCString name );

    const PascalCString& GetFontFilename();
    void                 SetFontFilename( PascalCString name );

    const PascalCString& GetFontStyle();
    void                 SetFontStyle( PascalCString style );

    const tColour& GetColour();
    void           SetColour( tColour* colour );
    void           SetColour( int r, int g, int b, int a = 0xff );

    virtual bool IsDrawable() { return false; }

protected:
    ~FeTextStyle();

    // member variables
    int m_Size;
    PascalCString* m_FontName;
    PascalCString* m_FontData;
    PascalCString* m_FontStyle;
    tColour* m_Colour;

private:
};

//=============================================================================
// Inlines
//=============================================================================

//=============================================================================
// FeTextStyle::GetSize
//=============================================================================
// Description : Retrieves the point size the font should be displayed at
//
// Return      : integer value font point size
//
//=============================================================================
inline int FeTextStyle::GetSize()
{
    return m_Size;
}

//=============================================================================
// FeTextStyle::SetSize
//=============================================================================
// Description : Sets the font display size
//
// Parameters  : size - a new integral point display size
//
//=============================================================================
inline void FeTextStyle::SetSize( int size )
{
    m_Size = size;
}

//=============================================================================
// FeTextStyle::GetFontName
//=============================================================================
// Description : Returns the symbolic name for the font
//
// Return      : const reference to a PascalCString object
//
//=============================================================================
inline const PascalCString& FeTextStyle::GetFontName()
{
    return *m_FontName;
}

//=============================================================================
// FeTextStyle::GetFontFilename
//=============================================================================
// Description : Returns the actual filename for the font
//
// Return      : const reference to a PascalCString object
//
//=============================================================================
inline const PascalCString& FeTextStyle::GetFontFilename()
{
    return *m_FontData;
}

//=============================================================================
// FeTextStyle::GetFontStyle
//=============================================================================
// Description : Returns a string describing the style of the font
//
// Return      : const reference to a PascalCString object
//
//=============================================================================
inline const PascalCString& FeTextStyle::GetFontStyle()
{
    return *m_FontStyle;
}

//=============================================================================
// FeTextStyle::GetColour
//=============================================================================
// Description : Gets the current colour of the font
//
// Constraints : This only defines the default (starting) value for the text
//               objects.  This can be overridden through FeText
//
// Return      : a const tColour object reference
//
//=============================================================================
inline const tColour& FeTextStyle::GetColour()
{
    return *m_Colour;
}

// end conditional define
#endif