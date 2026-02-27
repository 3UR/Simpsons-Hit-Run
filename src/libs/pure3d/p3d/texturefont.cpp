//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/texturefont.hpp>
#include <p3d/utility.hpp>
#include <p3d/chunkfile.hpp>
#include <p3d/file.hpp>
#include <constants/chunkids.hpp>
#include <p3d/shader.hpp>
#include <p3d/texture.hpp>
#include <p3d/unicode.hpp>
#include <p3d/matrixstack.hpp>

#include <string.h>
static const unsigned TEXTURE_FONT_VERSION = 0;

const unsigned int BACKGROUND_GLYPH_INDEX = 0;

//**************************************************************
// Class tTextureGlyph
//**************************************************************
tTextureGlyph::tTextureGlyph() :
    tFontGlyph(),
    textureNum(0),
    bottomLeft(0.0f,0.0f),
    topRight(1.0f,1.0f)
{
}

tTextureGlyph::~tTextureGlyph()
{
}

//**************************************************************
// Class tTextureFont
//**************************************************************
tTextureFont::tTextureFont():
    fontSize(0.0f),
    fontWidth(0.0f),
    fontHeight(0.0f),
    fontBaseLine(0.0f),
    lineSpacing(0.0f),
    colour(255,255,255,255),
    shader(NULL),
    numTextures(0),
    textures(NULL),
    missingGlyph(0), // for srr2 we define char 1 as rectangle, (note char 0 is space)
    numGlyphs(0),
    glyphs(NULL),
    mGlyphCodes(NULL)
{  
}

tTextureFont::~tTextureFont()
{
    tRefCounted::Release(shader);
    for(unsigned int i = 0; i < numTextures; i++)
    {
        tRefCounted::Release(textures[i]);
    }
    delete[] textures;
    delete[] glyphs;
    delete[] mGlyphCodes;
}

float 
tTextureFont::GetTextHeight(const P3D_UNICODE* text) const
{
    if (!text)
    {
        return 0.0f;
    }

    float height = fontHeight;
    float lineHeight = fontHeight + lineSpacing;

    unsigned int currChar = 0;
    while(text[currChar])
    {
        if (text[currChar]==(P3D_UNICODE)'\n')
        {
            height += lineHeight;
        }
        currChar++;
    }
    return height;
}

float 
tTextureFont::GetTextWidth(const P3D_UNICODE* text) const
{
    if (!text)
    {
        return 0.0f;
    }

    float maxWidth = 0.0f;
    float width = 0.0f;
    unsigned int currChar = 0;

    while(text[currChar])
    {
        width = GetLineWidth(&text[currChar]);
        if (width>maxWidth)
        {
            maxWidth = width;
        }
        
        while ((text[currChar]) && (text[currChar]!=(P3D_UNICODE)'\n'))
        {
            currChar++;
        }
        if( text[currChar] == (P3D_UNICODE)'\n' )
        {
            currChar++;
        }
    }
    return maxWidth;
}


void 
tTextureFont::SetShader(tShader* s)
{ 
    tRefCounted::Assign(shader,s); 
}


bool
tTextureFont::SetMissingLetter(P3D_UNICODE code)
{
    int newMissingGlyph = FindGlyph(code);
    if (newMissingGlyph >= 0)
    {
        missingGlyph = newMissingGlyph;
        return true;
    }
    return false;
}

float 
tTextureFont::GetLineWidth(const P3D_UNICODE* text) const
{
    if ((!text)||(!text[0])||(text[0]==(P3D_UNICODE)'\n'))
    {
        return 0.0f;
    }

    float width = 0.0f;
    unsigned int currChar = 0;
    int currGlyph;

    while ((text[currChar]) && (text[currChar] != (P3D_UNICODE)'\n'))
    {
        currGlyph = FindGlyph(text[currChar]);
        if (currGlyph >= 0)
        {
            width += glyphs[currGlyph].GetAdvance();
        }
        else
        {
            width += glyphs[missingGlyph].GetAdvance();
        }
        currChar++;
    }

    return width;
}

void 
tTextureFont::DisplayLine(const P3D_UNICODE* text, int formatting) const
{
    if ((!text)||(!text[0])||(text[0]==(P3D_UNICODE)'\n'))
    {
        return;
    }

    unsigned int currChar = 0;
    int currGlyph = 0; 
    float x = 0.0f;
    float width = 0.0f;
    int totalGlyphs = 0;


    p3d::stack->Push();
    p3d::stack->Translate((formatting & CENTRE_X) ? -GetLineWidth(text) / 2.0f : 0.0f, fontBaseLine-fontHeight, 0.0f);
       
    // Render all the letters from each texture at the same time
    for(unsigned int currTex = 0; currTex < numTextures; currTex++)
    {
        currChar = 0;
        int vertCount = 0;
        while ((text[currChar]) && (text[currChar]!=(P3D_UNICODE)'\n'))
        {
            currGlyph = FindGlyph(text[currChar]);
            if ((!currGlyph) || (currGlyph == -1))
            {
                currGlyph = missingGlyph;
            }        
            if ( glyphs[currGlyph].GetTextureNum() == currTex )
            {
                vertCount += 6;
            }
            currChar++;
        }
        totalGlyphs += vertCount;;

        if (vertCount>0)
        {
            currChar = 0;
            x = 0.0f;
            shader->SetTexture(PDDI_SP_BASETEX,textures[currTex]);
            pddiPrimStream* stream = p3d::pddi->BeginPrims(shader->GetShader(), PDDI_PRIM_TRIANGLES, PDDI_V_CT, vertCount);
            while ((text[currChar]) && (text[currChar]!=(P3D_UNICODE)'\n'))
            {
                currGlyph = FindGlyph(text[currChar]);
                if ((!currGlyph) || (currGlyph == -1))
                {
                    currGlyph = missingGlyph;
                }        
                if ( glyphs[currGlyph].GetTextureNum() == currTex )
                {
                    DrawGlyph(stream, currGlyph, x + glyphs[currGlyph].GetLeftBearing());
                }
                x += glyphs[currGlyph].GetAdvance();
                currChar++;
            }
            p3d::pddi->EndPrims(stream);
        }
    }

    // Render all the spaces between letters
    currChar = 0;
    currGlyph = 0;
    x = 0.0f;
    shader->SetTexture(PDDI_SP_BASETEX,textures[0]);
    pddiPrimStream* stream = p3d::pddi->BeginPrims(shader->GetShader(), PDDI_PRIM_TRIANGLES, PDDI_V_CT, totalGlyphs + 6);
    while ((text[currChar]) && (text[currChar]!=(P3D_UNICODE)'\n'))
    {
        currGlyph = FindGlyph(text[currChar]);
        if (!currGlyph)
        {
            currGlyph = missingGlyph;
        }                
        width += glyphs[currGlyph].GetLeftBearing();
        DrawBlank(stream, x, width);
        x += glyphs[currGlyph].GetWidth() + width;
        width = glyphs[currGlyph].GetRightBearing();
        currChar++;
    }
    DrawBlank(stream, x, width);
    p3d::pddi->EndPrims(stream);

    p3d::stack->Pop();
}

int 
tTextureFont::FindGlyph(P3D_UNICODE code) const
{
    int startPos = 0;
    int endPos = numGlyphs-1;
    int currPos = (endPos-startPos)/2;
    
    if (mGlyphCodes[startPos] == code)  return startPos;
    if (mGlyphCodes[endPos]   == code)  return endPos;

    while (((endPos - startPos) > 1)      &&
           (mGlyphCodes[startPos] < code) &&
           (mGlyphCodes[endPos]   > code))
    {
        if (mGlyphCodes[currPos] == code)
        {
            return currPos;
        }
        else
        {
            if (mGlyphCodes[currPos] < code)  startPos = currPos;
            else                              endPos   = currPos;

            currPos = startPos + ((endPos - startPos) / 2);
        }
    }

    if (mGlyphCodes[currPos] == code)  return currPos;

    return -1;
}

void 
tTextureFont::DrawGlyph(pddiPrimStream* buffer, unsigned int glyphIndex, float x) const
{
    float h = fontHeight;
    float w = glyphs[glyphIndex].GetWidth();

    float u0 = glyphs[glyphIndex].GetBottomLeft().u;
    float u1 = glyphs[glyphIndex].GetTopRight().u;
    float v0 = glyphs[glyphIndex].GetBottomLeft().v;
    float v1 = glyphs[glyphIndex].GetTopRight().v;

    tColour c=colour;

    // 0
    buffer->Colour(c);
    buffer->UV(u0, v0);
    buffer->Coord(x,0.0f,0.0f);

    // 1
    buffer->Colour(c);
    buffer->UV(u0, v1);
    buffer->Coord(x,h,0.0f);

    // 2
    buffer->Colour(c);
    buffer->UV(u1, v1);
    buffer->Coord(x + w,h,0.0f);

    // 0
    buffer->Colour(c);
    buffer->UV(u0, v0);
    buffer->Coord(x,0.0f,0.0f);

    // 2
    buffer->Colour(c);
    buffer->UV(u1, v1);
    buffer->Coord(x + w,h,0.0f);
    
    // 3
    buffer->Colour(c);
    buffer->UV(u1, v0);
    buffer->Coord(x + w,0.0f,0.0f);
}

void 
tTextureFont::DrawBlank(pddiPrimStream* buffer, float x, float width) const 
{
    float h = fontHeight;
    float w = width;

    float u0 = glyphs[BACKGROUND_GLYPH_INDEX].GetBottomLeft().u;
    float u1 = glyphs[BACKGROUND_GLYPH_INDEX].GetTopRight().u;
    float v0 = glyphs[BACKGROUND_GLYPH_INDEX].GetBottomLeft().v;
    float v1 = glyphs[BACKGROUND_GLYPH_INDEX].GetTopRight().v;

    tColour c=colour;

    // 0
    buffer->Colour(c);
    buffer->UV(u0, v0);
    buffer->Coord(x,0.0f,0.0f);

    // 1
    buffer->Colour(c);
    buffer->UV(u0, v1);
    buffer->Coord(x,h,0.0f);

    // 2
    buffer->Colour(c);
    buffer->UV(u1, v1);
    buffer->Coord(x + w,h,0.0f);

    // 0
    buffer->Colour(c);
    buffer->UV(u0, v0);
    buffer->Coord(x,0.0f,0.0f);

    // 2
    buffer->Colour(c);
    buffer->UV(u1, v1);
    buffer->Coord(x + w,h,0.0f);
    
    // 3
    buffer->Colour(c);
    buffer->UV(u1, v0);
    buffer->Coord(x + w,0.0f,0.0f);
}

//**************************************************************
// Class tTextureGlyph
//**************************************************************
tTextureFontLoader::tTextureFontLoader() : tSimpleChunkHandler(Pure3D::Font::TEXTURE_FONT)
{
}

tEntity* 
tTextureFontLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    unsigned int version = f->GetLong();
    P3DASSERT((unsigned)version == TEXTURE_FONT_VERSION);

    char buf[256];

    tTextureFont* font = new tTextureFont;

    f->GetPString(buf);
    font->SetName(buf);

    f->GetPString(buf);
    tShader* shader = p3d::find<tShader>(store, buf);
    if (!shader)
    {
        shader = new tShader;
        shader->SetInt(PDDI_SP_ISLIT,0);
        shader->SetInt(PDDI_SP_BLENDMODE,PDDI_BLEND_ALPHA);
        shader->SetInt(PDDI_SP_FILTER,PDDI_FILTER_BILINEAR);
        shader->SetInt(PDDI_SP_ALPHATEST, 0);
    }
    font->SetShader(shader);

    font->fontSize = f->GetFloat();
    font->fontWidth = f->GetFloat();
    font->fontHeight = f->GetFloat();
    font->fontBaseLine = f->GetFloat();

    font->numTextures = (unsigned int)f->GetLong();
    P3DASSERT(font->numTextures);
    font->textures = new tTexture*[font->numTextures];
    memset(font->textures,0,sizeof(tTexture*)*font->numTextures);

    tTextureLoader* loader = dynamic_cast<tTextureLoader*>(p3d::loadManager->GetP3DHandler()->GetHandler(Pure3D::Texture::TEXTURE));
    loader->AddRef();

    unsigned int currTex = 0;
    while(f->ChunksRemaining())
    {
        switch(f->BeginChunk())
        {
            case Pure3D::Texture::TEXTURE:
            {
                P3DASSERT(currTex<font->numTextures);
                tTexture* texture = loader->LoadTexture(f);
                tRefCounted::Assign(font->textures[currTex],texture);
                currTex++;
                break;
            }
            case Pure3D::Font::TEXTURE_GLYPH_LIST:
            {
                P3DASSERT(!font->glyphs);
                if (font->glyphs)
                {
                    delete[] font->glyphs;
                    delete[] font->mGlyphCodes;
                }
                font->numGlyphs = f->GetLong();
                P3DASSERT(font->numGlyphs>0);
                font->glyphs = new tTextureGlyph[font->numGlyphs];
                font->mGlyphCodes = new unsigned short[font->numGlyphs];

                for(unsigned int i = 0; i < font->numGlyphs; i++)
                {
                    font->glyphs[i].textureNum = f->GetLong();
                    font->glyphs[i].bottomLeft.u = f->GetFloat();
                    font->glyphs[i].bottomLeft.v = f->GetFloat();
                    font->glyphs[i].topRight.u = f->GetFloat();
                    font->glyphs[i].topRight.v = f->GetFloat();
                    font->glyphs[i].leftBearing = f->GetFloat();
                    font->glyphs[i].rightBearing = f->GetFloat();
                    font->glyphs[i].width = f->GetFloat();
                    font->glyphs[i].advance = f->GetFloat();
                    font->glyphs[i].code = (unsigned int)f->GetLong();
                    font->mGlyphCodes[i] = font->glyphs[i].code;
                }
                break;
            }
            default:
                break;
        }
        f->EndChunk();
    }
    loader->Release();

    font->SetMissingLetter(0x7f7f); // special character rendered as rectangle by the font for srr2 xbox tcr
    return font;
}

