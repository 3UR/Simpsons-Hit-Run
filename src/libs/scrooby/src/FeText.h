//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This contains the FeText class.
//
// Authors:     Ian Gipson
//
//===========================================================================
#ifndef __FeText__
#define __FeText__

//===========================================================================
// Includes
//===========================================================================
class PascalCString;

#include "Text.h"
#include "FeBoundedDrawable.h"
#include "FeParent.h"
#include "strings/unicodeString.h"

// TODO(3UR): if text is still scaled weirdly remove all uwp references from this file otherwise remove the todo here and in other places
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
#pragma warning( disable : 4250 )
#endif

#if defined( RAD_WIN32 ) || defined( RAD_UWP )
struct ShortRectExtents
{
    short xMin,
          xMax,
          yMin,
          yMax;
};
#endif

class tTextString;
class tFont;


class FeText
    : 
        public FeBoundedDrawable, 
        public FeParent,
        virtual public Scrooby::Text // for the exported client interface
{
    public:

        FeText( const tName& name, int x, int y );
        virtual void SetAlpha( float a );
        virtual void SetColour( tColour c );
        virtual void SetColour( int red, int green, int blue ); // Javascript testbed interface

        virtual void RestoreDefaultColour();

        virtual int GetIndex();
        virtual void SetIndex( int n );
        virtual int GetNumOfStrings();

        //allows access to the current string in the text object 
        virtual UnicodeString GetString();
        UnicodeChar* GetStringBuffer();

        // be careful when using this function!!!
        //
        void SetStringBuffer( UnicodeChar* buffer );

        //allows access to any of the individual strings in the text object
        virtual UnicodeString GetString( const int index );
        UnicodeChar* GetStringBuffer( const int index );

        // this will default to setting it as a hard coded string
        virtual void SetString( int index, const char* string, const char* textBibleName = NULL );
        virtual void SetString( int index, const char* string, unsigned int textBibleID = 0 );
        virtual void SetString( int index, UnicodeString& string, const char* textBibleName = NULL );
        virtual void SetString( int index, UnicodeString& string, unsigned int textBibleID = 0 );

        virtual void Reset();
        virtual void Display();

        void SetTextStyle( const char* textStyleName );
        void SetTextStyle( unsigned int resourceID );
        tFont* GetTextStyle() const;

        void AddHardCodedString( const char* string );
        void AddTextBibleString( unsigned int textBibleResourceID, const char* stringID );
        void AddTextBibleString( const char* textBibleName, const char* stringID );

        Scrooby::TextMode GetTextMode();
        void SetTextMode( Scrooby::TextMode mode );

        int GetTextWidth();
        int GetTextHeight();

#if defined( RAD_WIN32 ) || defined( RAD_UWP )
        virtual bool IsPointInBoundingRect( float x, float y );
#endif
        // Whether or not to show text with a drop shadow
        void SetDisplayShadow( bool show );
        bool IsDisplayingShadow();
        
        // Properties for text shadows
        void SetShadowOffset( int x, int y );
        int GetXShadowOffset();
        int GetYShadowOffset();
        void SetShadowColour( tColour c );
        tColour& GetShadowColour();

        // TC: Added text outline support
        //
        void SetDisplayOutline( bool show ) { mDisplayOutline = show; }
        bool IsDisplayingOutline() const { return mDisplayOutline; }
        void SetOutlineColour( tColour c ) { mOutlineColour = c; }
        tColour GetOutlineColour() const { return mOutlineColour; }

        virtual tFont* GetFont() const { return mFont; }

    protected:
        virtual ~FeText();

        //-------------------------------------------------------------------------
        // Private member functions.
        //-------------------------------------------------------------------------

        void ResetText();       
        void ReCalculateAlignment();
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
        void RecalculateRectExtents();
#endif

        //-------------------------------------------------------------------------
        // Private member data.
        //-------------------------------------------------------------------------
 
        int mIndex;
        unsigned int mTextStyle;
        float mXShadowOffset;
        float mYShadowOffset;
        tColour mOriginalColour;
        tColour mShadowColour;
        tFont* mFont;
        Scrooby::TextMode mTextMode;
        UnicodeChar* mBuffer;

        bool mOverrideStringBuffer : 1;
        bool mDisplayShadow : 1;
        bool mDisplayOutline : 1;
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
        bool mIsBoundingBoxStretched : 1;
#endif
        // TC: Added text outline support
        //
        tColour mOutlineColour;

#if defined( RAD_WIN32 ) || defined( RAD_UWP )
        ShortRectExtents m_rectExtents;     //stores the bounding rect info in windows coordinate system.
#endif

};



class FeTextChildString : public FeEntity
{
    public:
    
        virtual UnicodeString GetString();
        virtual UnicodeChar* GetStringBuffer() = 0;
        virtual bool IsValid() = 0;

        virtual bool IsDrawable() { return false; }

};


class FeTextChildHardCodedString : public FeTextChildString
{
    public:
        
        FeTextChildHardCodedString( UnicodeString string );
        virtual ~FeTextChildHardCodedString();

        virtual UnicodeChar* GetStringBuffer();
        void SetString( UnicodeString string );
        bool IsValid();
    
    protected:
        
        UnicodeString mUnicodeString;
};


class FeTextChildTextBibleString : public FeTextChildString
{
    public:
        
        FeTextChildTextBibleString( unsigned int textBibleResourceID,
                                    const char* stringID );
        
        virtual ~FeTextChildTextBibleString();

        virtual UnicodeChar* GetStringBuffer();

        virtual unsigned int GetTextBibleResourceId() const;

        bool IsValid();

    protected:
        
        unsigned int mTextBibleResourceID;
        char* mStringID;
        unsigned int mLanguage;
        bool mLoaded;
};

#endif
