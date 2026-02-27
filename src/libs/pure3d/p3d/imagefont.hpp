//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _IMAGEFONT_HPP_
#define _IMAGEFONT_HPP_

#include <p3d/loadmanager.hpp>
#include <p3d/entity.hpp>
#include <p3d/font.hpp>

class tImageFontLoader;
class tImage;

//**************************************************************
// Class tImageGlyph
//**************************************************************
class tImageGlyph
{
public:
    tImageGlyph();
    ~tImageGlyph();

    unsigned short  xOrigin;
    unsigned short  leftBearing;
    unsigned short  rightBearing;
    unsigned short  width;
    unsigned short  advance;
    P3D_UNICODE     code;
};

//**************************************************************
// Class tImageFont
//**************************************************************
class tImageFont : public tEntity
{
public:
    // gets the actual font size
    virtual unsigned int GetFontSize() const                            { return fontSize; }

    // gets the width of the largest glyph in the font
    virtual unsigned int GetFontWidth() const                           { return fontWidth; }

    // gets the height of the font
    virtual unsigned int GetFontHeight() const                          { return fontHeight; }

    // gets the base line of the font from top of font
    virtual unsigned int GetFontBaseLine() const                        { return fontBaseLine; }
    
    // gets the height of single or multiline text 
    virtual unsigned int GetTextHeight(const P3D_UNICODE* text) const;

    // gets the maximum width of single or multiline text
    virtual unsigned int GetTextWidth(const P3D_UNICODE* text) const;

    // display single or multiline text
    virtual void DisplayText(tImage* image, int x, int y, const P3D_UNICODE* text, int formatting = 0) const; 

    // get the spacing between lines of multiline text
    virtual unsigned int GetLineSpacing() const                         { return lineSpacing; } 

    // set the spacing between lines of multiline text
    virtual void SetLineSpacing(unsigned int space)                     { lineSpacing = space>0 ? space : 0; }

    // get the colour used to draw the font
    virtual tColour GetColour() const                                   { return colour; }

    // set the colour used to draw the font
    virtual void SetColour(tColour c)                                   { colour = c; }

    // get the glyph to display if an unknown glyph is encountered in text
    P3D_UNICODE GetMissingLetter() const                                { return glyphs[missingGlyph].code; }       

    // set the glyph to display if an unknown glyph is encountered in text
    // returns false if glyph doesn't exist
    bool SetMissingLetter(P3D_UNICODE letter);

    friend class tImageFontLoader;

protected:
    tImageFont();
    ~tImageFont();

    // get the width of one line of text
    virtual unsigned int GetLineWidth(const P3D_UNICODE* text) const;

    // display a single line of text
    virtual void DisplayLine(tImage* image, int x, int y, const P3D_UNICODE* text, int formatting = 0) const;

    // find the glyph that corresponds to a character code
    int FindGlyph(P3D_UNICODE code) const;

    unsigned int    fontSize;
    unsigned int    fontWidth;
    unsigned int    fontHeight;
    unsigned int    fontBaseLine;
    unsigned int    lineSpacing;
    tColour         colour;
    tImage*         image;
    unsigned int    missingGlyph;
    unsigned int    numGlyphs;
    tImageGlyph*    glyphs;
};

//**************************************************************
// Class tImageFontLoader
//**************************************************************
class tImageFontLoader : public tSimpleChunkHandler
{
public:
    tImageFontLoader();

protected:
    ~tImageFontLoader() {}

    tEntity* LoadObject(tChunkFile*, tEntityStore* store);
};

#endif

