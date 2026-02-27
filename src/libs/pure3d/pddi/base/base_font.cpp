/*===========================================================================
    font.cpp
    28-Feb-98 Created by Neall
    06-Nov-99 Ported to v12, converted to stroke fonts

    Copyright (c)1998, 1999 Radical Entertainment Inc.
    All rights reserved.
===========================================================================*/

#include <pddi/base/base_font.hpp>
#include <pddi/pddi.hpp>

#include <stdlib.h>
#include <pddi/base/romans.h>  // glyphs

float pddiBaseFont::xscale = 1.0f;
float pddiBaseFont::yscale = 1.0f;

pddiBaseFont::pddiBaseFont(pddiRenderContext* ctx)
{
    context = ctx; // weak reference, don't addref
}

pddiBaseFont::~pddiBaseFont()
{
}

void pddiBaseFont::Print(const char* str, int x, int y, float z, pddiColour colour)
{
    // tweak these values to adjust the screen size of the font
    const float baseline = 10.0f * yscale;
    const float descender = baseline + (5.0F * yscale);  // inter-line spacing
    const float xsize = 0.5f * xscale;
    const float ysize = 0.5f * yscale;

    const char PEN_UP = -50;

    unsigned char* s = (unsigned char*)str;
    float basex = (float)x;
    float basey = (float)((y * yscale) + baseline);
    float penz = (float)z;

    // Count verts to send
    int nVerts = 0;
    while(*s)
    {
        if(*s < 32 || *s > 127)
        {
            ++s;
            continue;
        }

        int glyphIndex = *s - 32;
        nVerts += (int)GlyphTable[glyphIndex].nVerts;
        ++s;
    }

    pddiPrimStream* stream = NULL;
    if(nVerts)
    {
        stream = context->BeginPrims(NULL, PDDI_PRIM_LINES, PDDI_V_C, nVerts);
    }

    s = (unsigned char*)str;
    while(*s)
    {
        if(*s == '\n')
        {
            basex = (float)x;
            basey += descender;
            s++;
            continue;
        }
        if(*s == '\t')
        {
            basex = (float)(((((int) basex) / 40) + 1) * 40);
            s++;
            continue;
        }
        if(*s < 32 || *s > 127)
        {
            s++;
            continue;
        }

        // the glyph table starts at 32 (space)
        int glyphIndex = *s - 32;
        Glyph* glyph = &GlyphTable[glyphIndex];
        Stroke* stroke = &StrokeTable[glyph->firstStroke];
        // move pen to initial position
        // the first stroke x coord is the leftmost position
        float maxX = 0.5f + (float)abs(stroke[0].x) * xsize;
        basex += maxX;
        pddiVector v;

        for(int i=1; i < glyph->nStroke-1; i++)
        {
            if(stroke[i+1].x != PEN_UP)
            {
                v.x = basex + xsize*(float)stroke[i].x;
                v.y = basey + ysize*(float)stroke[i].y;
                v.z = penz;
                stream->Vertex(&v, colour);

                v.x = basex + xsize*(float)stroke[i+1].x;
                v.y = basey + ysize*(float)stroke[i+1].y;
                stream->Vertex(&v, colour);
            }
            else
            {
                i++;
            }
        }

        // next character
        basex += maxX;
        s++;
    }

    if(nVerts)
    {
        context->EndPrims(stream);
    }
}

void pddiBaseFont::SetScale(float xScale, float yScale)
{
    xscale = xScale;
    yscale = yScale;
}


