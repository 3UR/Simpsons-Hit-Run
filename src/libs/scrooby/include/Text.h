//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This contains the Srooby::Text class.
//
// Authors:     Ian Gipson
//
//===========================================================================

// Recompilation protection flag.
#ifndef SCROOBY_TEXT_H
#define SCROOBY_TEXT_H

//===========================================================================
// Nested Includes
//===========================================================================
#include "BoundedDrawable.h"
#include "strings/unicodeString.h"

class UnicodeString;
class tFont;

namespace Scrooby
{
//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: This class is used to represent a Scrooby Text element
//
// Constraints: None
//
//===========================================================================

class Text
:
    virtual public BoundedDrawable
{
    public:
        
        // Switches back to the colour defined in the text style.
        virtual void RestoreDefaultColour() = 0;        
        
        // String index for multi-string objects
        virtual int GetIndex() = 0;
        virtual void SetIndex( int n ) = 0;        

        // Allows access to the total number of strings in a multistring object
        virtual int GetNumOfStrings() = 0;

        // Access the strings of a text object.  Where no index is specified, the current index is used.
        virtual UnicodeString GetString() = 0;
        virtual UnicodeString GetString( const int index ) = 0;
        virtual UnicodeChar* GetStringBuffer() = 0;
        virtual UnicodeChar* GetStringBuffer( const int index ) = 0;

        // If a text bible is specified, then string is used as an index into the text bible.  otherwise
        // string is the new value of the string.
        virtual void SetString( int index, const char* string, const char* textBibleName ) = 0;
        virtual void SetString( int index, const char* string, unsigned int textBibleID = 0 ) = 0;
        virtual void SetString( int index, UnicodeString& string, const char* textBibleName ) = 0;
        virtual void SetString( int index, UnicodeString& string, unsigned int textBibleID = 0 ) = 0;

        // Switch how the text object handles text which is larger than the current bounding box
        virtual Scrooby::TextMode GetTextMode() = 0;
        virtual void SetTextMode( Scrooby::TextMode mode ) = 0;

        // Metrics for how much of the screen the text will take up.  These numbers will change according to
        // the text display mode.
        virtual int GetTextHeight() = 0;
        virtual int GetTextWidth() = 0;

        // Whether or not to show text with a drop shadow
        virtual void SetDisplayShadow( bool show ) = 0;
        virtual bool IsDisplayingShadow() = 0;

        // Properties for text shadows
        virtual void SetShadowOffset( int x, int y ) = 0;
        virtual int GetXShadowOffset() = 0;
        virtual int GetYShadowOffset() = 0;
        virtual void SetShadowColour( tColour c ) = 0;
        virtual tColour& GetShadowColour() = 0;

        // TC: Added text outline support
        //
        virtual void SetDisplayOutline( bool show ) = 0;
        virtual bool IsDisplayingOutline() const = 0;
        virtual void SetOutlineColour( tColour c ) = 0;
        virtual tColour GetOutlineColour() const = 0;

        virtual tFont* GetFont() const = 0;

        virtual void SetStringBuffer( UnicodeChar* buffer ) = 0;

};

} // End namespace

#endif // End conditional inclusion
