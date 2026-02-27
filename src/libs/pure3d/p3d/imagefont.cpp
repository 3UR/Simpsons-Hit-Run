//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/imagefont.hpp>
#include <p3d/utility.hpp>
#include <p3d/image.hpp>
#include <p3d/chunkfile.hpp>
#include <p3d/file.hpp>
#include <p3d/unicode.hpp>
#include <constants/chunkids.hpp>

#include <string.h>
static const unsigned IMAGE_FONT_VERSION = 0;

//**************************************************************
// Class tImageGlyph
//**************************************************************
tImageGlyph::tImageGlyph() :
    xOrigin(0),
    leftBearing(0),
    rightBearing(0),
    width(0),
    advance(0),
    code(0)
{
}

tImageGlyph::~tImageGlyph()
{
}

//**************************************************************
// Class tImageFont
//**************************************************************
tImageFont::tImageFont():
    fontSize(0),
    fontWidth(0),
    fontHeight(0),
    fontBaseLine(0),
    lineSpacing(0),
    colour(255,255,255,255),
    image(NULL),
    missingGlyph(0),
    numGlyphs(0),
    glyphs(NULL)
{
}

tImageFont::~tImageFont()
{
    tRefCounted::Release(image);
    delete [] glyphs;
}

unsigned int
tImageFont::GetTextHeight(const P3D_UNICODE* text) const
{
    if (!text)
    {
        return 0;
    }

    unsigned int height = fontHeight;
    unsigned int lineHeight = fontHeight + lineSpacing;

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

unsigned int 
tImageFont::GetTextWidth(const P3D_UNICODE* text) const
{
    if (!text)
    {
        return 0;
    }

    unsigned int maxWidth = 0;
    unsigned int width = 0;
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
    }

    return maxWidth;
}

void 
tImageFont::DisplayText(tImage* dest, int x, int y, const P3D_UNICODE* text, int formatting) const
{
    if ((!text)||(!dest))
    {
        return;
    }

    dest->AddRef();

    unsigned int lineHeight = fontHeight+lineSpacing;

    y -= (formatting & CENTRE_Y) ? GetTextHeight(text) / 2 : 0;

    unsigned int currChar = 0;
    while(text[currChar])
    {
        DisplayLine(dest,x,y,text,formatting);
        while ((text[currChar]) && (text[currChar]!=(P3D_UNICODE)'\n'))
        {
            currChar++;
        }
        if (text[currChar]==(P3D_UNICODE)'\n')
        {
            currChar++;
        }
        y += lineHeight;
    }  

    dest->Release();
}

bool
tImageFont::SetMissingLetter(P3D_UNICODE code)
{
    int newMissingGlyph = FindGlyph(code);
    if (newMissingGlyph >= 0)
    {
        missingGlyph = newMissingGlyph;
        return true;
    }
    return false;
}

unsigned int 
tImageFont::GetLineWidth(const P3D_UNICODE* text) const
{
    if ((!text)||(!text[0])||(text[0]==(P3D_UNICODE)'\n'))
    {
        return 0;
    }

    unsigned int width = 0;
    unsigned int currChar = 0;
    int currGlyph;

    while ((text[currChar]) && (text[currChar] != (P3D_UNICODE)'\n'))
    {
        currGlyph = FindGlyph(text[currChar]);
        if (currGlyph >= 0)
        {
            width += glyphs[currGlyph].advance;
        }
        else
        {
            width += glyphs[missingGlyph].advance;
        }
        currChar++;
    }

    return width;
}

void 
tImageFont::DisplayLine(tImage* dest, int x, int y, const P3D_UNICODE* text, int formatting) const
{
    if ((!text)||(!text[0])||(text[0]==(P3D_UNICODE)'\n'))
    {
        return;
    } 
    
    unsigned int currChar = 0;
    int currGlyph = 0;    

    if (dest->GetDepth()==8)
    {
        tImage8* destTmp = (tImage8*)dest;
        tImage8* glyphTmp = (tImage8*)image;
        int palCount = glyphTmp->GetUsedColours();
        for(int palIndex = 0; palIndex < palCount; palIndex++)
        {
            destTmp->SetPaletteEntry(palIndex, glyphTmp->GetPaletteEntry(palIndex));
        }
    }

    x += (formatting & CENTRE_X) ? -(int)GetLineWidth(text) / 2 : 0;
    y += (fontHeight-fontBaseLine);
    
    while ((text[currChar]) && (text[currChar]!=(P3D_UNICODE)'\n'))
    {
        currGlyph = FindGlyph(text[currChar]);
        if (!currGlyph)
        {
            currGlyph = missingGlyph;
        }        

        dest->CopyRegion(image, 0, 0, glyphs[currGlyph].leftBearing, fontHeight, x, y);
        x += glyphs[currGlyph].leftBearing;
        dest->CopyRegion(image, glyphs[currGlyph].xOrigin, 0, glyphs[currGlyph].width, fontHeight, x, y);
        x += glyphs[currGlyph].width;
        dest->CopyRegion(image, 0, 0, glyphs[currGlyph].rightBearing, fontHeight, x, y);
        x += glyphs[currGlyph].rightBearing;

        currChar++;
    }
}

int 
tImageFont::FindGlyph(P3D_UNICODE code) const
{
    int startPos = 0;
    int endPos = numGlyphs-1;
    int currPos = (endPos-startPos)/2;
    
    if (glyphs[startPos].code==code)
    {
        return startPos;
    }   
    if (glyphs[endPos].code==code)
    {
        return endPos;
    }   

    while (((endPos-startPos)>1)&&((glyphs[startPos].code<code)&&(glyphs[endPos].code>code)))
    {
        if (glyphs[currPos].code==code)
        {
            return currPos;
        }
        else
        {
            if (glyphs[currPos].code<code)
            {
                startPos=currPos;
            }
            else
            {
                endPos=currPos;
            }
            currPos=startPos+((endPos-startPos)/2);
        }
    }

    if (glyphs[currPos].code==code)
    {
        return currPos;
    }
    else
    {
        return -1;
    }
}

//**************************************************************
// Class tImageFontLoader
//**************************************************************
tImageFontLoader::tImageFontLoader() : tSimpleChunkHandler(Pure3D::Font::IMAGE_FONT)
{
}


tEntity* 
tImageFontLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    unsigned int version = f->GetLong();
    P3DASSERT((unsigned)version == IMAGE_FONT_VERSION);

    char buf[256];

    tImageFont* font = new tImageFont;

    f->GetPString(buf);
    font->SetName(buf);
    font->fontSize = (unsigned int)f->GetLong();
    font->fontWidth = (unsigned int)f->GetLong();
    font->fontHeight = (unsigned int)f->GetLong();
    font->fontBaseLine = (unsigned int)f->GetLong();

    tImageLoader* loader = dynamic_cast<tImageLoader*>(p3d::loadManager->GetP3DHandler()->GetHandler(Pure3D::Texture::IMAGE));
    loader->AddRef();

    while(f->ChunksRemaining())
    {
        switch(f->BeginChunk())
        {
            case Pure3D::Texture::IMAGE:
            {
                P3DASSERT(!font->image);
                tImage* image = loader->LoadImage(f);
                tRefCounted::Assign(font->image,image);
                break;
            }
            case Pure3D::Font::IMAGE_GLYPH_LIST:
            {
                P3DASSERT(!font->glyphs);
                if (font->glyphs)
                {
                    delete [] font->glyphs;
                }
                font->numGlyphs = f->GetLong();
                P3DASSERT(font->numGlyphs>0);
                font->glyphs = new tImageGlyph[font->numGlyphs];

                for(unsigned int i = 0; i < font->numGlyphs; i++)
                {
                    font->glyphs[i].xOrigin = f->GetUWord();
                    font->glyphs[i].leftBearing = f->GetUWord();
                    font->glyphs[i].rightBearing = f->GetUWord();
                    font->glyphs[i].width = f->GetUWord();
                    font->glyphs[i].advance = f->GetUWord();
                    font->glyphs[i].code = (unsigned int)f->GetLong();
                }
                break;
            }
            default:
                break;
        }
        f->EndChunk();
    }
    loader->Release();

    return font; 
}


