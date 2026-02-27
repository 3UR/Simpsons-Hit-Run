//=============================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All Rights Reserved.
//
// Component   : Scrooby
//
// Description : Container class for font attributes
//
// Authors     : Mike Perzel
//
//=============================================================================

//=============================================================================
// Includes
//=============================================================================
#include "FeTextStyle.h"
#include "strings/pcstring.h"
#include <p3d/p3dtypes.hpp>

//=============================================================================
// Local constants, typedefs, macros
//=============================================================================

//=============================================================================
// Global data, local data, local classes
//=============================================================================

//=============================================================================
// Member functions
//=============================================================================

//=============================================================================
// Inline Defined Functions (See FeTextStyle.h)
//=============================================================================
// int   FeTextStyle::GetSize();
// void  FeTextStyle::SetSize( int size );
// const FeTextStyle::PascalCString& GetFontName();
// const FeTextStyle::PascalCString& GetFontFilename();
// const FeTextStyle::PascalCString& GetFontStyle();
// const FeTextStyle::tColour& GetColour();

//=============================================================================
// FeTextStyle::FeTextStyle
//=============================================================================
// Description : constructor
//
//=============================================================================
FeTextStyle::FeTextStyle() :
m_Size( 0 )
{
    m_FontName = new PascalCString();
    m_FontData = new PascalCString();
    m_FontStyle = new PascalCString();
    m_Colour = new tColour( 255, 255, 255, 255 );
}

//=============================================================================
// FeTextStyle::~FeTextStyle
//=============================================================================
// Description : deconstructor
//
//=============================================================================
FeTextStyle::~FeTextStyle()
{
    if( m_FontName )
    {
        delete m_FontName;
    }
    if( m_FontData )
    {
        delete m_FontData;
    }
    if( m_FontStyle )
    {
        delete m_FontStyle;
    }
    if( m_Colour )
    {
        delete m_Colour;
    }
}

//=============================================================================
// FeTextStyle::SetFontName
//=============================================================================
// Description : Sets the symbolic name for the font
//
// Parameters  : name - a PascalCString object which will be copied
//
//=============================================================================
void FeTextStyle::SetFontName( PascalCString name )
{
    *m_FontName = name;
}

//=============================================================================
// FeTextStyle::SetFontFilename
//=============================================================================
// Description : Sets the path to the actual font file
//
// Parameters  : name - a PascalCString object which will be copied
//
//=============================================================================
void FeTextStyle::SetFontFilename( PascalCString name )
{
    *m_FontData = name;
}

//=============================================================================
// FeTextStyle::SetFontStyle
//=============================================================================
// Description : Sets the style specifier string (currently not used)
//
// Parameters  : style - a PascalCString object which will be copied
//
//=============================================================================
void FeTextStyle::SetFontStyle( PascalCString style )
{
    *m_FontStyle = style;
}

//=============================================================================
// FeTextStyle::SetColour
//=============================================================================
// Description : Sets the starting colour for the font
//
// Constraints : This only defines the default (starting) value for the text
//               objects.  This can be overridden through FeText
//
// Parameters  : colour - a tColour object
//
//=============================================================================
void FeTextStyle::SetColour( tColour* colour )
{
    if( m_Colour )
    {
        delete m_Colour;
    }
    m_Colour = colour;
}

//=============================================================================
// FeTextStyle::SetColour
//=============================================================================
// Description : Sets the starting colour for the font
//
// Constraints : This only defines the default (starting) value for the text
//               objects.  This can be overridden through FeText
//
// Parameters  : r,g,b,a - the individual color components of the color
//
//=============================================================================
void FeTextStyle::SetColour( int r, int g, int b, int a )
{
    SetColour( new tColour( r, g, b, a ) );
}
