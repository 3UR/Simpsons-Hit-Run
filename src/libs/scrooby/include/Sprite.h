//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This contains the Scrooby::Sprite class.
//
// Authors:     Ian Gipson
//
//===========================================================================

// Recompilation protection flag.
#ifndef SCROOBY_SPRITE_H
#define SCROOBY_SPRITE_H

// Enable/disable Bitmap Text Support w/ Scrooby Multi-sprites.
//
#define BITMAP_TEXT_SUPPORT

//===========================================================================
// Nested Includes
//===========================================================================
#include <p3d/p3dtypes.hpp> // for tColour
#include "BoundedDrawable.h"

#ifdef BITMAP_TEXT_SUPPORT
    #include <strings/unicodestring.h>
#endif

class tSprite;

namespace Scrooby
{
//===========================================================================
// Interface Definitions
//===========================================================================

enum eSpriteMode
{
    SPRITE_NORMAL,
    SPRITE_BITMAP_TEXT,

    NUM_SPRITE_MODES
};

//===========================================================================
//
// Description: This class is used to represent a front-end page
//
// Constraints: None
//
//===========================================================================

class Sprite 
: 
    virtual public BoundedDrawable
{
    public:
        virtual void SetImage( int index, const char* alias ) = 0;
        virtual void SetIndex( int n ) = 0;
        virtual int  GetIndex() = 0;
        virtual int  GetNumOfImages() = 0;
        virtual const tSprite* GetRawSprite() = 0;
        virtual void ResizeToBoundingBox() = 0;
        virtual void SetRawSprite( tSprite* sprite, bool updateDrawable = false ) = 0;
    
#ifdef BITMAP_TEXT_SUPPORT
        // set current sprite mode
        //
        virtual void SetSpriteMode( eSpriteMode mode ) = 0;

        // set current bitmap text string
        //
        virtual void CreateBitmapTextBuffer( unsigned int size ) = 0;
        virtual void SetBitmapText( const UnicodeChar* text ) = 0;
        virtual void SetBitmapText( const char* text ) = 0;

        // set character spacing
        //
        virtual void SetBitmapTextSpacing( short numPixels ) = 0;

        // set line spacing (in y-direction)
        //
        virtual void SetBitmapTextLineSpacing( short numPixels ) = 0;
#endif

    protected:
    private:
};


} // End namespace

#endif // End conditional inclusion
