//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime
//
// Description: This contains the FeText object
//
// Authors:     Wilkin Ng, Ian Gipson
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================

#include <p3d/entity.hpp>
#include <p3d/matrixstack.hpp>
#include <p3d/textstring.hpp>
#include <p3d/texturefont.hpp>
#include <p3d/unicode.hpp>
#include <p3d/utility.hpp>
#include <p3d/shader.hpp>
#include "FeApp.h"
#include "FePage.h"
#include "FeText.h"
#include "FeTextBible.h"
#include "ResourceManager/FeResourceManager.h"
#include <raddebug.hpp>
#include <string.h>
#include "utility/debugMessages.h"

//===========================================================================
// Local Constants, Typedefs, and Macros
//===========================================================================

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================

//===========================================================================
// Member Functions
//===========================================================================

//===========================================================================
// FeText::FeText(char *name, int x, int y) 
//===========================================================================
// Description: FeText's normal constructor
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
FeText::FeText( const tName& name, int x, int y ) 
:
    FeBoundedDrawable( name ),
    mIndex( 0 ),
    mTextStyle( 0 ),
    mXShadowOffset( -1.0f ),
    mYShadowOffset( -2.0f ),
    mFont( NULL ),
    mTextMode( Scrooby::TEXT_OVERLAP ),
    mBuffer( NULL ),
    mOverrideStringBuffer( false ),
    mDisplayShadow( false ),
    mDisplayOutline( false ),
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
    mIsBoundingBoxStretched( false ),
#endif
    mOutlineColour( 0, 0, 0, 192 )
{
    FeDrawable::NormalizeCoord( x, y, mPosX, mPosY );
    FeText::Reset();
    mOriginalColour.Set( 0xFF, 0xFF, 0xFF, 0xFF );
    mShadowColour.Set( 0x00, 0x00, 0x00, 0x80 );
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
    memset( &m_rectExtents, 0, sizeof( m_rectExtents ) );
#endif
}


FeText::~FeText()
{
    P3D_UNICODE* buffer = FeApp::GetInstance()->GetTextBuffer();
    if( buffer != NULL )
    {
        delete [] buffer;
        buffer = NULL;

        FeApp::GetInstance()->SetTextBuffer( NULL );
    }

    if( mFont )
    {
        mFont->Release();
    }
}

void FeText::AddHardCodedString( const char* string )
{
    // Convert from ASCII to Unicode.
    UnicodeString stringToSet;
    stringToSet.ReadAscii( string );

    FeTextChildHardCodedString* pHardCoded;
    
    pHardCoded = new FeTextChildHardCodedString( stringToSet );
    
    AddChild( pHardCoded );
    ResetText();
}

void FeText::AddTextBibleString( unsigned int textBibleResourceID, const char* stringID )
{
    FeTextChildTextBibleString* pTextBible;
    
    pTextBible = new FeTextChildTextBibleString( textBibleResourceID, stringID );
    
    AddChild( pTextBible );
    ResetText();
}

void FeText::AddTextBibleString( const char* textBibleName, const char* stringID )
{
    FeTextChildTextBibleString* pTextBible;
    
    unsigned int textBibleResourceID = FeApp::GetInstance()->GetFeResourceManager().GetIndex( textBibleName, true );
    pTextBible = new FeTextChildTextBibleString( textBibleResourceID, stringID );
    
    AddChild( pTextBible );
    ResetText();
}


void FeText::Display() //Override
{
    if( mFont )
    {
        FeTextChildString* text = dynamic_cast < FeTextChildString* >( FeParent::GetChildIndex( mIndex ) );
        p3d::stack->Push();

        // fonts are in world space, so they need to be scaled down into screen space
        float fontSize = mFont->GetFontSize();
        float scale = 1.0f / FeApp::GetInstance()->GetScreenHeight();
        p3d::stack->Scale( scale, scale, 1.0f );
        ResetText();

        // TODO(3UR): if text is still scaled weirdly remove all uwp references from this file otherwise remove the todo here and in other places
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
        // TC: for PC sku, the source fonts are actually twice as big (for higher resolution display),
        //     so we need to scale them back down to the intended size
        //
        p3d::stack->Scale( 0.5f, 0.5f, 1.0f );
#endif // RAD_WIN32

        int formatting = m_horizontalJustification == Scrooby::Centre ? CENTRE_X : 0;

        // render drop shadow
        //
        if( mDisplayShadow )
        {
            // TC [HACK]: Never allow shadow alpha to be greater than text alpha.
            //
            tColour shadowColour = mShadowColour;
            if( mShadowColour.Alpha() > GetColour().Alpha() )
            {
                shadowColour.SetAlpha( GetColour().Alpha() );
            }
/*
            // multiply shadow alpha by text drawable alpha
            float textAlpha = GetColour().Alpha() / 255.0f;
            tColour shadowColour = mShadowColour;
            shadowColour.SetAlpha( static_cast<int>( textAlpha * mShadowColour.Alpha() ) );
*/
            mFont->SetColour( shadowColour );
            p3d::stack->Push();
            p3d::stack->Translate( mXShadowOffset, mYShadowOffset, 0 );
            mFont->DisplayText( FeApp::GetInstance()->GetTextBuffer(), formatting );
            p3d::stack->Pop();
        }

        // render text outline
        //
        if( mDisplayOutline )
        {
            // TC: Text Pseudo-Outlining (just rendering 4 drop shadows around text)
            //
            static float outlinePercentage = 0.05f;
            float outlineThickness = outlinePercentage * mFont->GetFontSize();

            // TC [HACK]: Never allow outline alpha to be greater than text alpha.
            //
            tColour outlineColour = mOutlineColour;
            if( mOutlineColour.Alpha() > GetColour().Alpha() )
            {
                outlineColour.SetAlpha( GetColour().Alpha() );
            }

            mFont->SetColour( outlineColour );

            for( int i = -1; i < 2; i++ )
            {
                for( int j = -1; j < 2; j++ )
                {
                    if( i != 0 && j != 0 )
                    {
                        p3d::stack->Push();
                        p3d::stack->Translate( i * outlineThickness, j * outlineThickness, 0 );
                        mFont->DisplayText( FeApp::GetInstance()->GetTextBuffer(), formatting );
                        p3d::stack->Pop();
                    }
                }
            }
        }

        // render the actual text
        //
        mFont->SetColour( GetColour() );
        mFont->DisplayText( FeApp::GetInstance()->GetTextBuffer(), formatting );

        p3d::stack->Pop();
#ifdef RAD_DEBUG
        if( FeApp::GetInstance()->IsShowingBoundingBoxes() )
        {
            DisplayBoundingBox();
        }
#endif
    }
}

//===========================================================================
// FeText::GetIndex
//===========================================================================
// Description: gets the index number of the current string in a multi-string
//                text
//
// Constraints:    None
//
// Parameters:    NONE
//
// Return:      the value of mIndex
//
//===========================================================================
int FeText::GetIndex()
{
    return mIndex;
}

//===========================================================================
// FeText::GetNumOfString
//===========================================================================
// Description: Returns the number of strings which are in a Multi-Text object
//
// Constraints:    None
//
// Parameters:    NONE
//
// Return:      Number of internal strings
//
//===========================================================================
int FeText::GetNumOfStrings()
{
    return GetChildrenCount();
}

//===========================================================================
// GetString
//===========================================================================
// Description: allows access to the current string being displayed.
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      the string currently being displayed
//
//===========================================================================
UnicodeString FeText::GetString()
{
    return GetString( mIndex );
}
UnicodeChar* FeText::GetStringBuffer()
{
    return GetStringBuffer( mIndex );
}
void FeText::SetStringBuffer( UnicodeChar* buffer )
{
    mBuffer = buffer;
    mOverrideStringBuffer = (mBuffer != GetStringBuffer());
}
//===========================================================================
// GetString
//===========================================================================
// Description: allows access to any of the individual strings in the text object
//
// Constraints:    None
//
// Parameters:    index - index of which of the multi-text's strings we want
//
// Return:      the string stored at that particular index
//
//===========================================================================
UnicodeChar* FeText::GetStringBuffer( const int index )
{
    if ( index < FeParent::GetChildrenCount() )
    {
        FeTextChildString* text = dynamic_cast < FeTextChildString* >( FeParent::GetChildIndex( index ) );
        if( text )
        {
            return text->GetStringBuffer();
        }
        else
        {
            //this occurs if there was no valid string in that index
            //Scrooby::Log::Message( Scrooby::LVL_ERROR, "FeText::GetString - Object at index %d is not a text object", index );
        }
    }
    else
    {
        //we're asking for an index that is out of range if we got to here
        //Scrooby::Log::Message( Scrooby::LVL_ERROR, "FeText::GetString - Requested a string index too high (%d, max %d)", index, GetChildrenCount() );
    }
    return NULL;
}
UnicodeString FeText::GetString( const int index )
{
    UnicodeString rValue;
    UnicodeChar* buffer = GetStringBuffer( index );
    if( buffer )
    {
        rValue.ReadUnicode( buffer );
    }
    else
    {
        rValue.ReadAscii( "No Valid Text" );
    }
    return rValue;
}

int FeText::GetTextHeight()
{
/*
    if( mFont )
    {
        //float scale = 1.0f / FeApp::GetInstance()->GetScreenHeight();
        return static_cast<int>(mFont->GetTextHeight( FeApp::GetInstance()->GetTextBuffer() ));
    }
    return 0;
*/
    int textHeight = 0;

    if( mFont != NULL )
    {
        textHeight = static_cast<int>( mFont->GetTextHeight( (P3D_UNICODE*)mBuffer ) );
    }

    return textHeight;
}


Scrooby::TextMode FeText::GetTextMode()
{
    return mTextMode;
}

//===========================================================================
// GetTextStyle
//===========================================================================
// Description: returns the resource ID of the text style
//
// Constraints:    None
//
// Parameters:    NONE
//
// Return:      the resource ID of the text style
//
//===========================================================================
tFont* FeText::GetTextStyle() const
{
    return mFont;
}

int FeText::GetTextWidth()
{
/*
    if( mFont )
    {
        return static_cast<int>(mFont->GetTextWidth( FeApp::GetInstance()->GetTextBuffer() ));
    }
    return 0;
*/
    int textWidth = 0;

    if( mFont != NULL )
    {
        textWidth = static_cast<int>( mFont->GetTextWidth( (P3D_UNICODE*)mBuffer ) );
    }

    return textWidth;
}
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
bool FeText::IsPointInBoundingRect( float x, float y )
{
    float height = FeApp::GetInstance()->GetScreenHeight();
    float width = FeApp::GetInstance()->GetScreenWidth();

    const float ASPECT = width/height; // The screen aspect ratio.

    int ix, iy;

    // convert from p3d point to scrooby screen point.
    x = (x/2.0f) + 0.5f;
    y = (((y*ASPECT)/2.0f) + 0.5f)/ASPECT;
    InverseNormalizeCoord( x, y, ix, iy );
    return ((ix <= m_rectExtents.xMax) && (ix >= m_rectExtents.xMin) && (iy <= m_rectExtents.yMax) && (iy >= m_rectExtents.yMin));
}
#endif

void FeText::Reset()
{
    FeBoundedDrawable::Reset();
    mIndex = 0;
    if( mFont )
    {
        SetColour( mOriginalColour );
    }
}

void FeText::ReCalculateAlignment()
{
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
    if( !mIsBoundingBoxStretched )
    {
        // TC: for PC sku, the source fonts are actually twice as big (for higher resolution display),
        //     so we need to stretch the bounding box accordingly for proper text wrapping
        //
        mIsBoundingBoxStretched = true;
        FeBoundedDrawable::StretchBoundingBox( 2.0f, 1.0f );
    }
#endif // RAD_WIN32

    m_offsetX = 0;
    m_offsetY = 0;
    if( !mFont )
    {
        if( mTextStyle )
        {
            mFont = dynamic_cast<tFont*>( FeApp::GetInstance()->GetFeResourceManager().GetResource( mTextStyle ) );
            if( mFont )
            {
                mFont->AddRef();
            }
        }
    }
    if( mFont )
    {
        ResetText();
        float height = 0.0f;
        float width = 0.0f;
        float scale = 1.0f;
        float boxWidth = 0.0f;
        float boxHeight = 0.0f;
        scale = 1.0f / FeApp::GetInstance()->GetScreenHeight();
        width = (mFont->GetTextWidth( FeApp::GetInstance()->GetTextBuffer() )) * scale;
        height = (mFont->GetTextHeight( FeApp::GetInstance()->GetTextBuffer() )) * scale;
        NormalizeCoord( m_width, m_height, boxWidth, boxHeight );

#if defined( RAD_WIN32 ) || defined( RAD_UWP )
        // TC: for PC sku, the source fonts are actually twice as big (for higher resolution display),
        //     so the width of the current text string is only half as wide; this is needed for proper
        //     right alignment
        //
        width /= 2.0f;
#endif

        switch( m_horizontalJustification )
        {
        case Scrooby::Right :
            m_offsetX = boxWidth - width;
            break;
        case Scrooby::Centre :
            m_offsetX = boxWidth / 2.0f; // let tTextureFont::DisplayLine do the centering;
                                         // we just need to place the offset in the center
                                         // of the bounding box
            break;
        case Scrooby::Left :
        default :
            break;
        }

        switch( m_verticalJustification )
        {
        case Scrooby::Top :
            // TC: This doesn't seem to work properly; make it the same as Bottom.
            //
//            m_offsetY = (boxHeight - height);
            break;
        case Scrooby::Centre :
            m_offsetY = (boxHeight - height) / 2.0f;
            break;
        case Scrooby::Bottom :
        default :
            break;
        }
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
        RecalculateRectExtents();
#endif
    }
    else
    {
        m_isDirty = true;
    }

}

void FeText::ResetText()
{
    float boxWidth = 0.0f;
    float boxHeight = 0.0f;
    float scale = 1.0f / FeApp::GetInstance()->GetScreenHeight();
    NormalizeCoord( m_width, m_height, boxWidth, boxHeight );
    P3D_UNICODE* buffer = FeApp::GetInstance()->GetTextBuffer();
    int i = 0;
    int lastSpace = 0;

    if( !buffer )
    {
        buffer = new P3D_UNICODE[SCROOBY_TEXT_BUFFER_SIZE];
        FeApp::GetInstance()->SetTextBuffer( buffer );
    }
    
    UnicodeChar* string = GetStringBuffer();

    // TC: Nasty Hack!!
    //
    if( mOverrideStringBuffer )
    {
        string = mBuffer;
    }

    if( mBuffer != string )
    {
        m_isDirty = true;
        mBuffer = string;
    }

    int length = 0;
    while( string && string[length] != 0 )
    {
        length++;
    }

    for( ; i < length; i++ )
    {
        buffer[i] = static_cast<P3D_UNICODE>(string[i]);
        buffer[i+1] = 0;
        if( string[i] == ' ' )
        {
            lastSpace = i;
        }
        if( mTextMode != Scrooby::TEXT_OVERLAP )
        {
            if( scale * mFont->GetTextWidth( buffer ) > boxWidth )
            {
                if( mTextMode == Scrooby::TEXT_CLIP )
                {
                    // clip off the last character copied
                    //
                    buffer[ i ] = '\0';

                    // Exit the loop, using only the text we've already copied
                    break;
                }
                else if( mTextMode == Scrooby::TEXT_WRAP )
                {
                    if( lastSpace > 0 )
                    {
                        buffer[lastSpace] = (P3D_UNICODE)'\n';
                        lastSpace = 0;
                    }
                }
                else
                {
                    // this can't happen
                    rAssert(false);
                }
            }
        }
    }
}

#if defined( RAD_WIN32 ) || defined( RAD_UWP )
void FeText::RecalculateRectExtents()
{
    float height = FeApp::GetInstance()->GetScreenHeight();
    float width = FeApp::GetInstance()->GetScreenWidth();

    int textWidth = GetTextWidth()/2;
    int textHeight = GetTextHeight()/2;

    if( textWidth == 0 || textHeight == 0 ) return;

    const float ASPECT = width/height; // The screen aspect ratio.

    // convert from p3d point to scrooby screen point.

    int xMin = 0, yMin = 0, xMax = 0, yMax = 0;

    float xTextOffset = (textWidth/1.5f);
    switch( m_horizontalJustification )
    {
    case Scrooby::Left :
        GetOriginPosition( xMin, yMin );
        break;

    case Scrooby::Center :
        //we have to findout the difference between the bounding width and the text width, and offset it from XMin.
        GetBoundingBoxCenter( xMin, yMin );

        xMin -= (int)xTextOffset;
        yMin -= textHeight/2;

        break;
    default :
        break;
    }

    xMax = xMin + textWidth + (int)(xTextOffset/2); 
    yMax = yMin + textHeight;   

    m_rectExtents.xMin = xMin;
    m_rectExtents.xMax = xMax;
    m_rectExtents.yMin = yMin;
    m_rectExtents.yMax = yMax;
}
#endif

void FeText::RestoreDefaultColour()
{
    SetColour( mOriginalColour );
}

//===========================================================================
// SetAlpha
//===========================================================================
// Description: Sets the alpha value for the text object
//
// Constraints:    a must be on the interval [0,1]
//
// Parameters:    a - the alpha value that we're setting
//
// Return:      None
//
//===========================================================================
void FeText::SetAlpha( float a )
{
    FeDrawable::SetAlpha( a );
}

void FeText::SetColour( int red, int green, int blue )
{
    tColour c( red, green, blue, static_cast<int>(GetAlpha()) * 255 );
    SetColour( c );
}

void FeText::SetColour(tColour c)
{
    FeDrawable::SetColour( c );
}

//===========================================================================
// FeText::SetHardCodedString
//===========================================================================
// Description: Set the given string object to the new string (as a hardcoded string)
//
// Constraints:    None
//
// Parameters:    index - the index of the string object
//              string - the new hardcoded string text
//
// Return:      None
//
//===========================================================================
void FeText::SetString( int index, const char* string, const char* textBibleName )
{
    SetString( index, string, FeApp::GetInstance()->GetFeResourceManager().GetIndex( textBibleName, true ) );
}
void FeText::SetString( int index, const char* string, unsigned int textBibleID )
{
    UnicodeString unicodeString;
    unicodeString.ReadAscii( string );
    SetString( index, unicodeString, textBibleID );
}
void FeText::SetString( int index, UnicodeString& string, const char* textBibleName )
{
    SetString( index, string, FeApp::GetInstance()->GetFeResourceManager().GetIndex( textBibleName, true ) );
}
void FeText::SetString( int index, UnicodeString& string, unsigned int textBibleID )
{
    //IAN IMPROVE: this function is thrashing memory by replacing strings continuously
    unsigned int childrenCount = GetChildrenCount();
    if( index < GetChildrenCount() )
    {
        FeTextChildString* text = dynamic_cast < FeTextChildString* >( GetChildIndex( index ) );
        rAssert( text );
        if( text == NULL )
        {
            return;
        }
        FeTextChildString* newText = NULL;
        if( textBibleID )
        {
            char* temp = static_cast<char*>(p3d::MallocTemp( string.Length() + 1 ));
            string.MakeAscii( temp, string.Length() + 1 );
            newText = new FeTextChildTextBibleString( textBibleID, temp );
            p3d::FreeTemp( (void*)temp );
        }
        else
        {
            newText = new FeTextChildHardCodedString( string );
        }
        ReplaceChild( text, newText );
    }
    else
    {
        rAssert( 0 );
    }
    m_isDirty = true;
}

void FeText::SetIndex(int n)
{
    if (n < GetChildrenCount())
    {
        //no need to redisplay if nothing has changed
        if( mIndex == n )
        {
        }
        else
        {
            mIndex = n;
        }
    }
    else
    {
        //Scrooby::Log::Message( Scrooby::LVL_ERROR, "FeText::SetIndex - Index was set too high" );
        rAssert( false );
    }
    m_isDirty = true;
}

//===========================================================================
// FeText::SetTextBibleString
//===========================================================================
// Description: Set the given string object to the new string (as a hardcoded string)
//
// Constraints:    None
//
// Parameters:    index - the index of the string object
//              name - the hash name of the text bible string
//
// Return:      None
//
//===========================================================================
/*void FeText::SetTextBibleString( int index, const char* name, unsigned int bibleID )
{
    if( index < GetChildrenCount() )
    {
        FeTextChildString* text = dynamic_cast < FeTextChildString* >( GetChildIndex( index ) );
        FeTextChildTextBibleString* bibleText = new FeTextChildTextBibleString( bibleID, name );
        ReplaceChild( text, bibleText );
    }
}*/

void FeText::SetTextMode( Scrooby::TextMode mode )
{
    mTextMode = mode;
    m_isDirty = true;
}

void FeText::SetTextStyle( const char* textStyleName )
{
    SetTextStyle( FeApp::GetInstance()->GetFeResourceManager().GetIndex( textStyleName ) );
}

void FeText::SetTextStyle( unsigned int textStyleID )
{
    mTextStyle = textStyleID;
    mFont = dynamic_cast<tFont*>(FeApp::GetInstance()->GetFeResourceManager().GetResource( mTextStyle ));
    if( mFont )
    {
        mFont->AddRef();
    }
    m_isDirty = true;
}

void FeText::SetDisplayShadow( bool show )
{
    mDisplayShadow = show;
}

bool FeText::IsDisplayingShadow()
{
    return mDisplayShadow;
}

void FeText::SetShadowOffset( int x, int y )
{
    // TC: Added text outline support (hacked in with the Scrooby
    //     Builder's drop shadow support). Here, I'm using drop shadow
    //     offsets of (0,0) to mean that a text outline is desired.
    //
    if( x == 0 && y == 0 )
    {
        SetDisplayOutline( mDisplayShadow );
        SetOutlineColour( mShadowColour );

        SetDisplayShadow( false );
    }
    else
    {
        mXShadowOffset = static_cast<float>(x);
        mYShadowOffset = static_cast<float>(y);
    }
}

int FeText::GetXShadowOffset()
{
    return static_cast<int>(mXShadowOffset);
}

int FeText::GetYShadowOffset()
{
    return static_cast<int>(mYShadowOffset);
}

void FeText::SetShadowColour( tColour c )
{
    mShadowColour = c;
}

tColour& FeText::GetShadowColour()
{
    return mShadowColour;
}




UnicodeString FeTextChildString::GetString()
{
    UnicodeString rValue;
    rValue.ReadUnicode( GetStringBuffer() );
    return rValue;
}


FeTextChildHardCodedString::FeTextChildHardCodedString( UnicodeString string ) : mUnicodeString( string )
{
}

FeTextChildHardCodedString::~FeTextChildHardCodedString()
{
}

UnicodeChar* FeTextChildHardCodedString::GetStringBuffer()
{
    return( mUnicodeString.GetBuffer() );
}


void FeTextChildHardCodedString::SetString( UnicodeString string )
{
    mUnicodeString = string;
}

bool FeTextChildHardCodedString::IsValid()
{
    return true;
}



FeTextChildTextBibleString::FeTextChildTextBibleString( unsigned int textBibleResourceID, const char* stringID ) : mTextBibleResourceID( textBibleResourceID ), mLoaded( true )
{
    mStringID = new char[strlen( stringID ) + 1];
    strcpy( mStringID, stringID );
}


FeTextChildTextBibleString::~FeTextChildTextBibleString()
{
    delete [] mStringID;
}


UnicodeChar* FeTextChildTextBibleString::GetStringBuffer()
{
    int bibleHandle = mTextBibleResourceID;
    FeTextBible* theBible = dynamic_cast<FeTextBible*>(FeApp::GetInstance()->GetFeResourceManager().GetResource( bibleHandle ));

    //UnicodeString unicodeString;
    if( theBible == NULL )
    {
        //the bible is not yet loaded
        mLoaded = false;
        return NULL;
    }
    mLanguage = theBible->GetLanguageIndex();
    mLoaded = true;
    UnicodeChar* ret = theBible->GetWChar( mStringID );
	rAssert(ret);
	return ret;
}

unsigned int FeTextChildTextBibleString::GetTextBibleResourceId() const
{
    return mTextBibleResourceID;
}

bool FeTextChildTextBibleString::IsValid()
{
    FeTextBible* theBible = dynamic_cast<FeTextBible*>(FeApp::GetInstance()->GetFeResourceManager().GetResource( mTextBibleResourceID ));
    if( !mLoaded )
    {
        return false;
    }
    if( theBible )
    {
        return (mLanguage == theBible->GetLanguageIndex());
    }
    return false;
}
