//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        string.cpp
//
// Subsystem:    Foundation Technologies - string object
//
// Description:    This file contains the implementation of the string object
//
// Revisions:   June 20, 2000     Creation     TEH
//
//=============================================================================

//============================================================================
// Include Files
//============================================================================

#include "pch.hpp"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <radstring.hpp>
#include <raddebug.hpp>
#include <radobject.hpp>
#include <radmemory.hpp>
#include <radmemorymonitor.hpp>

#define IS_NULL_MASK 0x80000000
#define SIZE_MASK ( ~IS_NULL_MASK )

//============================================================================
// Component: radString
//============================================================================

class radString
    :
    public IRadString,
    public radRefCount
{
    public:

    IMPLEMENT_REFCOUNTED_NOSIZE( "radString" )

    //========================================================================
    // radString::GetObjectSize
    //========================================================================

    #ifdef RAD_DEBUG

        virtual size_t GetObjectSize( void )
        {
            return sizeof(*this) + ( m_pBuffer ? GetSize( ) : 0 );
        }

    #endif

    //========================================================================
    // radString::IsNull
    //========================================================================

    bool IsNull( void ) const
    {
        return ( m_IsNull_Size & IS_NULL_MASK ) != 0;
    }

    //========================================================================
    // radString::SetNull
    //========================================================================

    void SetNull( bool null )
    {
        if ( null )
        {
            m_IsNull_Size |= IS_NULL_MASK;
        }
        else
        {
            m_IsNull_Size &= ~IS_NULL_MASK;
        }
    }

    //========================================================================
    // radString::Equals
    //========================================================================

    virtual bool Equals( const char * pBuffer )
    {
        if ( pBuffer != NULL )
        {
            if ( IsNull( ) == false )
            {
                return ( strcmp( m_pBuffer, pBuffer ) == 0 );
            }
        }
        else if ( IsNull( ) == true )
        {
            return true; // both null
        }

        return false; // one null, one not
    }

    //========================================================================
    // radString::Equals
    //========================================================================

    virtual bool Equals( IRadString * pIString )
    {
        rAssert( pIString != NULL );

        if ( pIString != NULL )
        {
            return Equals( pIString->GetChars( ) );
        }

        return false;
    }

    //========================================================================
    // radString::GetSize
    //========================================================================

    virtual unsigned int GetSize( void )
    {
        return m_IsNull_Size & SIZE_MASK;
    }

    //========================================================================
    // radString::SetSize
    //========================================================================

    void SetSizeBits( unsigned int reserved )
    {
        m_IsNull_Size &= IS_NULL_MASK;
        m_IsNull_Size |= ( reserved & (~IS_NULL_MASK) );
    }

    virtual void SetSize( unsigned int reserved )
    {
        if ( reserved > GetSize( ) )
        {
            if ( m_pBuffer != NULL )
            {
                rDebugString( "radString: realloc!\n" );

                char * pNewBuffer = (char*) ::radMemoryAlloc( GetThisAllocator( ), reserved );

                strcpy( pNewBuffer, m_pBuffer );

                ::radMemoryFree( GetThisAllocator( ), m_pBuffer );

                m_pBuffer = pNewBuffer;
            }

            SetSizeBits( reserved );
        }
    }

    //========================================================================
    // radString::Copy
    //========================================================================
        
    virtual void Copy( const char * pBuffer )
    {
        if ( pBuffer != NULL )
        {
            unsigned int sizeNeeded = strlen( pBuffer ) + 1;

            if ( sizeNeeded > GetSize( ) )
            {
                SetSizeBits( sizeNeeded );

                if ( m_pBuffer != NULL )
                {
                    rDebugString( "radString: realloc!\n" );
                    radMemoryFree( GetThisAllocator( ), m_pBuffer );
                }

                m_pBuffer = (char*) ::radMemoryAlloc( GetThisAllocator( ), GetSize( ) );
            }
            else
            {
                if ( m_pBuffer == NULL )
                {
                    m_pBuffer = (char*) ::radMemoryAlloc( GetThisAllocator( ), GetSize( ) );
                }
            }

            strcpy( m_pBuffer, pBuffer );
            SetNull( false );
        }
        else
        {
            SetNull( true );
        }    
    }

    //========================================================================
    // radString::Copy
    //========================================================================

    virtual void Copy( const IRadString * pIString )
    {
        rAssert( pIString != NULL );

        if( pIString != NULL )
        {
            if ( pIString != static_cast< IRadString * >( this ) )
            {
                Copy( pIString->GetChars( ) );
            }
        }
    }

    //========================================================================
    // radString::GetChars
    //========================================================================

    virtual void GetChars( char * pBuffer, unsigned int bufferSize ) const
    {
        rAssert( pBuffer != NULL && bufferSize > 0 );

        if ( pBuffer != NULL && bufferSize > 0 )
        {
            if ( IsNull( ) == false )
            {
                strncpy( pBuffer, m_pBuffer, bufferSize );
            }
            else
            {
                pBuffer[ 0 ] = '\0';
            } 
        }        
    }

    //========================================================================
    // radString::GetChars
    //========================================================================

    virtual const char * GetChars( void ) const
    {
        if ( IsNull( ) == true )
        {
            return NULL;
        }

        return m_pBuffer;
    }

    //========================================================================
    // radString::GetLength
    //========================================================================

    virtual unsigned int GetLength( void ) const
    {
        if ( IsNull( ) == false )
        {
            return strlen( m_pBuffer );
        }
        else
        {
            return 0;
        }
    }

    //========================================================================
    // radString::Append
    //========================================================================

    virtual void Append(IRadString * pIString )
    {
        if ( pIString != NULL )
        {
            Append( pIString->GetChars( ) );
        }

    }

    //========================================================================
    // radString::Append
    //========================================================================

    virtual void Append( const char * pBuffer )
    {
        if ( pBuffer != NULL )
        {
            if ( IsNull( ) == true )
            {
                Copy( pBuffer );
            }
            else
            {
                unsigned int sizeNeeded = strlen( pBuffer ) + strlen( m_pBuffer ) + 1;

                if ( sizeNeeded > GetSize( )  )
                {
                    rDebugString( "radString: realloc!\n" );

                    SetSizeBits( sizeNeeded );

                    char * pNewBuffer = (char*) radMemoryAlloc( GetThisAllocator( ), GetSize( ) );

                    strcpy( pNewBuffer, m_pBuffer );
                    strcat( pNewBuffer, pBuffer );

                    ::radMemoryFree( m_pBuffer );

                    m_pBuffer = pNewBuffer;
                }
                else
                {
                    strcat( m_pBuffer, pBuffer );
                }

                SetNull( false );
            }
        }
    }

    //========================================================================
    // radString::Append
    //========================================================================

    virtual void Append( const unsigned int x )
    {
        Append( (int) x );
    }

    //========================================================================
    // radString::Append
    //========================================================================

    virtual void Append( const int x )
    {
        char string[256];

        sprintf( string, "%d", x );

        Append( string );
    }

    //========================================================================
    // radString::Append
    //========================================================================
    
    virtual void Append( const char x )
    {
        char string[2];

        string[ 0 ] = x;
        string[ 1 ] = '\0';

        Append( string );
    }

    //========================================================================
    // radString::Append
    //========================================================================

    virtual void Append( const void * pPointer )
    {
        char string[256];

        sprintf( string, "%p", pPointer );

        Append( string );
    }
    //========================================================================
    // radString::Append
    //========================================================================

    virtual void Append( const float value )
    {
        char string[64];
        sprintf( string, "%f", value );
        Append( string );
    }
    //========================================================================
    // radString::Append
    //========================================================================

    virtual void Append( const bool value )
    {
        Append( value ? "true" : "false" );
    }

    virtual void ToLowercase( void )
    {
        if ( IsNull( ) == false )
        {
            SDL_strlwr( m_pBuffer );
        }
    }

    virtual void ToUppercase( void )
    {
        if( IsNull( ) == false )
        {
            SDL_strupr( m_pBuffer );
        }
    }

     //========================================================================
    // radString::MatchesWildCardPattern
    //========================================================================

    virtual bool MatchesWildCardPattern( const char * pWildCard )
    {
        return radStringMatchesWildCardPattern( m_pBuffer, pWildCard );
    }

    //========================================================================
    // radString::MatchesWildCardPattern
    //========================================================================

    virtual bool MatchesWildCardPattern( const IRadString * pIString_WildCardPattern )
    {
        rAssert( pIString_WildCardPattern != NULL );

        if ( pIString_WildCardPattern != NULL )
        {
            return MatchesWildCardPattern( pIString_WildCardPattern->GetChars( ) );
        }

        return false;
    }

    //========================================================================
    // radString::Compact
    //========================================================================

    virtual void Compact( void )
    {
        rAssert( false );
    }

    //========================================================================
    // radString::radString
    //========================================================================

    radString( void )
        :
        m_pBuffer( NULL ),
        m_IsNull_Size( IS_NULL_MASK | 0x01 )
    {
        radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radString" );
    }

    //========================================================================
    // radString::~radString
    //========================================================================

    ~radString( void )
    {
        if ( m_pBuffer != NULL )
        {
            radMemoryFree( GetThisAllocator( ), m_pBuffer );
            m_pBuffer = NULL;
        }
    }

    char * m_pBuffer;
    
    unsigned int m_IsNull_Size;    
};

void radStringCreate( IRadString** ppIRadString, radMemoryAllocator alloc )
{
   *ppIRadString = new ( alloc ) radString( );
}

bool radStringMatchesWildCardPattern
(
    const char* string,
    const char* pattern
)
{
    // NOTE: This code was swiped from tcl8.3.2
    int p, s;
    const char* pstart = pattern;
    
    while( true )
    {
        p = *pattern;
        s = *string;
        
        //
        // See if we're at the end of both the pattern and the string.  If
        // so, we succeeded.  If we're at the end of the pattern but not at
        // the end of the string, we failed.
        //
        
        if( p == '\0' )
        {
            if( s == '\0' )
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        if( ( s == '\0' ) && ( p != '*' ) )
        {
            return false;
        }
        
        //
        // Check for a "*" as the next pattern character.  It matches
        // any suradString.  We handle this by calling ourselves
        // recursively for each postfix of string, until either we
        // match or we reach the end of the string.
        //
        if( p == '*' )
        {
            pattern++;
            if( *pattern == '\0' )
            {
                return true;
            }
            while( true )
            {
                bool didmatch = radStringMatchesWildCardPattern( string, pattern );
                if( didmatch )
                {
                    return true;
                }
                if( *string == '\0' )
                {
                    return false;
                }
                string++;
            }
        }
        
        //
        // Check for a "?" as the next pattern character.  It matches
        // any single character.
        //        
        if( p == '?' )
        {
            char ch = string[ 0 ];
            
            pattern++;
            string++;
            continue;
        }
        
        //
        // Make sure that the next
        // bytes of each string match.
        //        
        if( s != p )
        {
            return false;
        }
        pattern++;
        string++;
    }
}

//=============================================================================
// SJIS -- lookup tables
//=============================================================================
static unsigned short rad_ascii_special[33][2] = {
    {0x8140, 32},        /*   */
    {0x8149, 33},        /* ! */
    {0x8168, 34},        /* " */
    {0x8194, 35},        /* # */
    {0x8190, 36},        /* $ */
    {0x8193, 37},        /* % */
    {0x8195, 38},        /* & */
    {0x8166, 39},        /* ' */
    {0x8169, 40},        /* ( */
    {0x816a, 41},        /* ) */
    {0x8196, 42},        /* * */
    {0x817b, 43},        /* + */
    {0x8143, 44},        /* , */
    {0x817c, 45},        /* - */
    {0x8144, 46},        /* . */
    {0x815e, 47},        /* / */
    {0x8146, 58},        /* : */
    {0x8147, 59},        /* ; */
    {0x8171, 60},        /* < */
    {0x8181, 61},        /* = */
    {0x8172, 62},        /* > */
    {0x8148, 63},        /* ? */
    {0x8197, 64},        /* @ */
    {0x816d, 91},        /* [ */
    {0x818f, 92},        /* \ */
    {0x816e, 93},        /* ] */
    {0x814f, 94},        /* ^ */
    {0x8151, 95},        /* _ */
    {0x8165, 96},        /* ` */
    {0x816f, 123},        /* { */
    {0x8162, 124},        /* | */
    {0x8170, 125},        /* } */
    {0x8150, 126},        /* ~ */
};

static unsigned short rad_ascii_table[3][2] = {
    {0x824f, 0x30},    /* 0-9  */
    {0x8260, 0x41},    /* A-Z  */
    {0x8281, 0x61},    /* a-z  */
};

//========================================================================
// Ascii2Sjis
//========================================================================

static radSJISChar Ascii2Sjis( char c )
{
    unsigned int sjis_code = 0;
    unsigned int ascii_code = c;

    unsigned char stmp = 0;
    unsigned char stmp2 = 0;

    if( (ascii_code >= 0x20) && (ascii_code <= 0x2f) )
    {
        stmp2 = 1;
    }
    else if( (ascii_code >= 0x30) && (ascii_code <= 0x39) )
    {
        stmp = 0;
    }
    else if( (ascii_code >= 0x3a) && (ascii_code <= 0x40) )
    {
        stmp2 = 11;
    }
    else if( (ascii_code >= 0x41) && (ascii_code <= 0x5a) )
    {
        stmp = 1;
    }
    else if( (ascii_code >= 0x5b) && (ascii_code <= 0x60) )
    {
        stmp2 = 37;
    }
    else if( (ascii_code >= 0x61) && (ascii_code <= 0x7a) )
    {
        stmp = 2;
    }
    else if( (ascii_code >= 0x7b) && (ascii_code <= 0x7e) )
    {
        stmp2 = 63;
    }
    else 
    {
        radSJISChar tmp;
        tmp.byte1 = 0;
        tmp.byte2 = 0;
        return tmp;
    }

    if (stmp2)
    {
        sjis_code = rad_ascii_special[ ascii_code - 0x20 - (stmp2 - 1) ][ 0 ];
    }
    else
    {
        sjis_code = rad_ascii_table[ stmp ][ 0 ] + ascii_code - rad_ascii_table[ stmp ][ 1 ];
    }

    radSJISChar schar;
    schar.byte1 = ( unsigned char ) ( (sjis_code & 0xff00) >> 8);
    schar.byte2 = ( unsigned char ) ( sjis_code & 0xff );
    
    return schar;
}

//========================================================================
// Sjis2Ascii
//========================================================================

static char Sjis2Ascii( radSJISChar character )
{
    unsigned char byte1 = character.byte1;
    unsigned char byte2 = character.byte2;
    char output = 0;
    int i;


    if( byte1 == 0x81 || byte1 == 0x82 )
    {
        if( byte1 == 0x82 )
        {
            if( ( ( byte2 >= 0x4f ) && ( byte2 <= 0x59 ) ) || 
                ( ( byte2 >= 0x60 ) && ( byte2 <= 0x7a ) ) )
            {
                output = byte2 - 0x1f;
            }
            else if( ( byte2 >= 0x81 ) && ( byte2 <= 0x9b) )
            {
                output = byte2 - 0x20;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            for( i = 0; i < 33; i++ )
            {
                if( byte2 == (rad_ascii_special[ i ][ 0 ] & 0x00ff ) )
                {             
                    output = (char) rad_ascii_special[ i ][ 1 ];
                    break;
                }
            }
            
            if(i == 33)
            {
                output = 0;
                return 0;
            }
        }
      }
    else
    {
        return 0;
    }
    
    return output;    
}

//========================================================================
// radIsSjis
//========================================================================

bool radIsSjis( const radSJISChar* str )
{
    unsigned int length = radSjisStrlen( str );

    for ( unsigned int i = 0; i < length; i++ )
    {
        unsigned char c = Sjis2Ascii( str[ i ] );
        if ( c == 0 )
        {
            return false;
        }
    }
    return true;
}

//========================================================================
// radIsAscii
//========================================================================

bool radIsAscii( const char* str )
{
    unsigned int length = strlen( str );

    for ( unsigned int i = 0; i < length; i++ )
    {
        radSJISChar c = Ascii2Sjis( str[ i ] );
        if ( c.byte1 == 0 && c.byte2 == 0 )
        {
            return false;
        }
    }

    return true;
}

//========================================================================
// radSjisToAscii
//========================================================================

bool radSjisToAscii( char* dest, const radSJISChar* src )
{
    unsigned int length = radSjisStrlen( src );

    for ( unsigned int i = 0; i < length; i++ )
    {
        unsigned char c = Sjis2Ascii( src[ i ] );
        if ( c == 0 )
        {
            return false;
        }
        dest[ i ] = c;
    }
    dest[ length ] = '\0';
    return true;
}

//========================================================================
// radAsciiToSjis
//========================================================================

bool radAsciiToSjis( radSJISChar* dest, const char* src )
{
    unsigned int length = strlen( src );

    for ( unsigned int i = 0; i < length; i++ )
    {
        radSJISChar c = Ascii2Sjis( src[ i ] );
        if ( c.byte1 == 0 && c.byte2 == 0 )
        {
            return false;
        }
        dest[ i ] = c;
    }
    dest[ length ].byte1 = 0;
    dest[ length ].byte2 = 0;

    return true;
}

//========================================================================
// radSjisStrlen
//========================================================================

unsigned int radSjisStrlen( const radSJISChar* str )
{
    unsigned int length = 0;
    while ( str->byte1 != 0 )
    {
        length++;
        str++;
    }
    return length;
}

//========================================================================
// radSjisStrcpy
//========================================================================

void radSjisStrcpy( radSJISChar* dest, const radSJISChar* src )
{
    *dest = *src;
    while ( dest->byte1 != 0 )
    {
        dest++;
        src++;
    }
}

//========================================================================
// radSjisStrncpy
//========================================================================

void radSjisStrncpy( radSJISChar* dest, const radSJISChar* src, unsigned int count )
{
    for ( unsigned int i = 0; i < count; i++ )
    {
        *dest = *src;
        if ( dest->byte1 == 0 )
        {
            break;
        }
        dest++;
        src++;
    }
}
