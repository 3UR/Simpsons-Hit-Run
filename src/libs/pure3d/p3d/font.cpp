//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/font.hpp>
#include <p3d/utility.hpp>
#include <p3d/matrixstack.hpp>

#include <string.h>

//**************************************************************
// Class tFontGlyph
//**************************************************************
tFontGlyph::tFontGlyph() :
    leftBearing(0.0f),
    rightBearing(0.0f),
    width(0.0f),
    advance(0.0f),
    code(0)
{
}

tFontGlyph::~tFontGlyph()
{
}

//**************************************************************
// Class tFont
//**************************************************************
void tFont::DisplayText(const P3D_UNICODE* text, int formatting) const
{
    if (!text)
    {
        return;
    }

    float lineHeight = GetFontHeight()+GetLineSpacing();

    p3d::stack->Push();
    p3d::stack->Translate(0.0f,(formatting & CENTRE_Y) ? GetTextHeight(text) / 2.0f : 0.0f, 0.0f);
    unsigned int currChar = 0;
    while(text[currChar])
    {
        DisplayLine(&text[currChar], formatting);
        while ((text[currChar]) && (text[currChar]!=(P3D_UNICODE)'\n'))
        {
            currChar++;
        }
        if (text[currChar]==(P3D_UNICODE)'\n')
        {
            currChar++;
        }
        p3d::stack->Translate(0.0f, -lineHeight, 0.0f);
    }  
    p3d::stack->Pop();
}

