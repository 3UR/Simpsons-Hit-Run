//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/textstring.hpp>
#include <p3d/font.hpp>
#include <p3d/utility.hpp>
#include <p3d/matrixstack.hpp>
#include <p3d/unicode.hpp>
#include <p3d/error.hpp>

#include <string.h>

tTextString::tTextString(tFont* f, P3D_UNICODE* s, float sc, bool d)
{
    colour.Set(255,255,255);
    font = f;
    font->AddRef();
    string = (char*)s;
    scale = sc;
    del = d;
    P3DASSERT(font);
}

tTextString::~tTextString()
{
    if(del)
        delete [] string;
    font->Release();
    font = NULL;
}
  
void tTextString::Display(void)
{
    if(scale != 1.0f)
    {
        p3d::stack->Push();
        p3d::stack->Scale(scale,scale,1.0f);
    }

    tColour c = font->GetColour();
    font->SetColour(colour);

    font->DisplayText((P3D_UNICODE*)string);

    font->SetColour(c);

    if(scale != 1.0f)
    {
        p3d::stack->Pop();
    }
}

void tTextString::GetBoundingBox(rmt::Box3D* box)
{
    box->low.Set(0,0,0);
    box->high.Set(font->GetTextWidth((P3D_UNICODE*)string) * scale, font->GetTextHeight((P3D_UNICODE*)string) * scale, 0);
}

void tTextString::SetFont(tFont* f)
{
    font->Release();
    font = f;
    font->AddRef();
    P3DASSERT(font);
}

tFont* tTextString::GetFont(void)
{
    return font;
}

void tTextString::SetString(P3D_UNICODE* text, bool d)
{
    if(del)
        delete [] string;

    string = (char*)text;
    del = d;
}

P3D_UNICODE* tTextString::GetString(void)
{
    return (P3D_UNICODE*)string;
}

void tTextString::SetColour(tColour c)
{
    colour = c;
}

tColour tTextString::GetColour(void)
{
    return colour;
}

