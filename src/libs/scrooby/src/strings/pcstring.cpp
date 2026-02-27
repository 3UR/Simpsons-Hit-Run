// @doc

//=============================================================================
//
// @module PCSTRING.CPP |
// Copyright <cp> 1997,1998 Radical Entertainment Ltd. All rights reserved.
//
// Class(es):   <c PascalCString>
//
// Parent(s):   None
//
// This file contains the representation of a Pascal-C string. This string, when 
// written to a file has its length (including the terminating null) pre-pended 
// to the actual string data. This makes loading the string from disk MUCH faster.
//
// Authors:     Kevin M. Loken (KML)
//
//============================================================================= <nl>
//  Version    Author    CR       Date      Brief Description                   <nl>
//----------------------------------------------------------------------------- <nl>
//  00.90.00    KML     ----    19-Aug-97   File creation.                      <nl>
//============================================================================= <nl>

//=============================================================================
// Include files
//=============================================================================
#include <assert.h>
#include <ctype.h>
#include <radmemory.hpp>
#include "strings/pcstring.h"
#include <stdlib.h>
#include <string.h>
#include "strings/stricmp.h"
#include <utility/memory.h>

int g_ScroobyMemoryCount = 0;

//=============================================================================
//
// @mfunc Default constructor.
//
// @rdesc N/A
//
//=============================================================================
PascalCString::PascalCString
(
)
:
    m_ichMac( 0 ),
    m_ichMax( 0 ),
    m_uiGrowBy( 16 )
#ifdef RAD_DEBUG
    ,m_NumberOfGrows( 0 )
#endif
{
    g_ScroobyMemoryCount += m_ichMax;
    m_szString = NULL;
}

//=============================================================================
//
// @mfunc Destructor.
//
// @rdesc N/A
//
//=============================================================================
PascalCString::~PascalCString
(
)
{
    g_ScroobyMemoryCount -= m_ichMax;
    
    delete [] m_szString;

    #ifdef RAD_DEBUG
        m_szString = 0;
        m_ichMac   = 0;
        m_ichMax   = 0;
        m_uiGrowBy = 0;
    #endif
}

//=============================================================================
//
// @mfunc Copy constructor from const UnicodeString
//
// @rdesc N/A
//
//=============================================================================
PascalCString::PascalCString( const UnicodeString& uString )
:
    m_ichMac( 0 ),
    m_ichMax( 0 ),
    m_uiGrowBy( 16 )
    #ifdef RAD_DEBUG
        ,m_NumberOfGrows( 0 )
    #endif
{
    AdjustMaxArraySize( uString.Length() + 1 );
    g_ScroobyMemoryCount += m_ichMax;
    m_szString = new char[m_ichMax];
    uString.MakeAscii( m_szString, m_ichMax );
}


//=============================================================================
//
// @mfunc Copy constructor.
//
// @rdesc N/A
//
//=============================================================================
PascalCString::PascalCString
(
    const char* szString // @parm The string to copy
)
:
    m_ichMax( 0 ),
    m_uiGrowBy( 16 )
    #ifdef RAD_DEBUG
    , m_NumberOfGrows( 0 )
    #endif
{
    if( szString )
    {
        m_ichMac = strlen( szString );
        AdjustMaxArraySize( m_ichMac + 1 );

        g_ScroobyMemoryCount += m_ichMax;
        m_szString = new char[m_ichMax];
        strcpy( m_szString, szString );
    }
    else
    {
        m_szString[0] = '\0';
        m_ichMac = 0;
    }
}

//=============================================================================
//
// @mfunc Copy constructor.
//
// @rdesc N/A
//
//=============================================================================
PascalCString::PascalCString
(
    const char ch // @parm The character to copy
)
:
    m_ichMax( 0 ),
    m_uiGrowBy( 16 )
    #ifdef RAD_DEBUG
    ,m_NumberOfGrows( 0 )
    #endif
{
    m_ichMac = 1;
    AdjustMaxArraySize( 2 );
    g_ScroobyMemoryCount += m_ichMax;
    m_szString = new char[m_ichMax];
    m_szString[0] = ch;
    m_szString[1] = 0;
}

//=============================================================================
//
// @mfunc Copy constructor.
//
// @rdesc N/A
//
//=============================================================================
PascalCString::PascalCString
(
    const PascalCString& rhs // @parm The string to copy
) :
    m_ichMax( 0 )
    #ifdef RAD_DEBUG
    ,m_NumberOfGrows( 0 )
    #endif
{
    m_uiGrowBy = rhs.m_uiGrowBy;
    m_ichMac   = rhs.m_ichMac;
    AdjustMaxArraySize( rhs.m_ichMax );

    g_ScroobyMemoryCount += m_ichMax;
    m_szString = new char[m_ichMax];
    strcpy( m_szString, rhs.m_szString );
}

//=============================================================================
//
// @mfunc Assignment operator
//
// @rdesc A reference to the new object
//
//=============================================================================
PascalCString& PascalCString::operator=
(
    const PascalCString& rhs // @parm The string to copy
)
{
    // check for assignment to self first
    if( this != &rhs ) 
    {
        g_ScroobyMemoryCount -= m_ichMax;

        if( m_ichMax < static_cast< unsigned int >( rhs.Length() ) + 1 )
        {
            AdjustMaxArraySize( rhs.m_ichMax );
            delete[] m_szString;
            m_szString = new char[m_ichMax];
        }

        // make sure original m_szString is de-allocated

        m_uiGrowBy = rhs.m_uiGrowBy;
        m_ichMac   = rhs.m_ichMac ;

        g_ScroobyMemoryCount += m_ichMax;
        strcpy( m_szString, rhs.m_szString );
    }

    return( *this ); 
}

//=============================================================================
//
// @mfunc Assignment operator
//
// @rdesc A reference to the new object
//
//=============================================================================
PascalCString& PascalCString::operator=
(
    const char* rhs // @parm The string to copy
)
{
    if( rhs )
    {
        m_ichMac = strlen( rhs );
        Grow( m_ichMac );

        strcpy( m_szString, rhs );
    }
    else
    {
        m_ichMac = 0;
        Grow( m_ichMac );

        m_szString[0] = 0;
    }

    return( *this );
}

//=============================================================================
//
// @mfunc Assignment operator
//
// @rdesc A reference to the new object
//
//=============================================================================
PascalCString& PascalCString::operator=
(
    const char rhs // @parm The character to copy
)
{
    Grow( 2 );

    m_szString[0] = rhs;
    m_szString[1] = 0;

    return( *this );
}

//=============================================================================
//
// @mfunc Grow the string
//
// @rdesc N/A
//
//=============================================================================
void PascalCString::Grow
(
    unsigned int uiLen // @parm We need to hold this many chars (+ a null)
)
{
    if( m_ichMax > uiLen )
    {
        return;
    }

    #ifdef RAD_DEBUG
        radMemoryAllocator current = ::radMemoryGetCurrentAllocator();
        if( current != RADMEMORY_ALLOC_TEMP )
        {
            rAssertMsg( m_NumberOfGrows == 0, "Growing strings cause fragmentation. Please ask Ian if you don't know how to prevent the grow" );
            ++m_NumberOfGrows;
        }
    #endif
    unsigned int temp = uiLen - m_ichMax;
    if(temp < m_uiGrowBy)
    {
        temp = m_uiGrowBy;
    }

    if( m_szString )
    {
        g_ScroobyMemoryCount -= m_ichMax;
    }

    AdjustMaxArraySize( uiLen + 1 );

    g_ScroobyMemoryCount += m_ichMax;
    radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
    char* szString = new char[m_ichMax];
    ::radMemorySetCurrentAllocator( old );
    assert( szString );

    if( m_szString )
    {
        strcpy( szString, m_szString );
        delete[] m_szString;
        m_szString = NULL ;
    }

    m_szString = szString;
}

//=============================================================================
//
// @mfunc Concatentation
//
// @rdesc A reference to this object (after concatenation)
//
//=============================================================================
PascalCString& PascalCString::operator+=
(
    const PascalCString& rhs // @parm What we are concatenating
)
{
    m_ichMac += rhs.m_ichMac;
    Grow( m_ichMac );

    strcat( m_szString, rhs.m_szString );

    return( *this );
}

//=============================================================================
//
// @mfunc Concatentation
//
// @rdesc A reference to this object (after concatenation)
//
//=============================================================================
PascalCString& PascalCString::operator+=
(
    const char* rhs // @parm What we are concatenating
)
{
    if( rhs )
    {
        m_ichMac += strlen( rhs );
        Grow( m_ichMac );

        strcat( m_szString, rhs );
    }

    return( *this );
}

//=============================================================================
//
// @mfunc Concatentation
//
// @rdesc A reference to this object (after concatenation)
//
//=============================================================================
PascalCString& PascalCString::operator+=
(
    const char rhs // @parm What we are concatenating
)
{
    m_ichMac += 1;
    Grow( m_ichMac );

    m_szString[ m_ichMac - 1 ] = rhs;
    m_szString[ m_ichMac ] = 0;

    return( *this );
}

//=============================================================================
//
// @mfunc Equality operator
//
// @rdesc Returns non-zero if equal, 0 if un-equal
//
//=============================================================================
bool PascalCString::operator==
(
    const PascalCString& rhs // @parm What we are comparing too
) const
{
    if( m_ichMac == rhs.m_ichMac )
    {
        // Compare the two strings. If they are equal (according to
        // the standard Windows rules for comparison) then strcmp
        // will return 0.
        return( strcmp( m_szString, rhs.m_szString ) == 0 );
    }
    else
    {
        // The two strings lengths are different so there is no
        // way that they are equal. This is a very fast check and
        // is much cheaper than doing the actual string comparison.
        return( false );
    }
}

//=============================================================================
//
// @mfunc Equality operator
//
// @rdesc Returns non-zero if equal, 0 if un-equal
//
//=============================================================================
bool PascalCString::operator==
(
    const char* rhs // @parm What we are comparing too
) const
{
    if( rhs )
    {
        // Compare the two strings. If they are equal (according to
        // the standard Windows rules for comparison) then strcmp
        // will return 0.
        return( strcmp( m_szString, rhs ) == 0 );
    }
    else
    {
        // The right hand side is a null string, so we can't be equal to it.
        return( false );
    }
}

//=============================================================================
//
// @mfunc Equality operator
//
// @rdesc Returns non-zero if equal, 0 if un-equal
//
//=============================================================================
bool PascalCString::operator==
(
    const char rhs // @parm What we are comparing too
) const
{
    if( m_ichMac == 1 )
    {
        // Compare the string and the character. If they are equal 
        // (according to the standard Windows rules for comparison) 
        // then we will return non-zero.
        return( m_szString[0] == rhs );
    }
    else
    {
        // Our string has more than a single character in it, so we can't
        // be equal.
        return( false );
    }
}

//=============================================================================
//
// @mfunc Inequality operator
//
// @rdesc Returns non-zero if equal, 0 if un-equal
//
//=============================================================================
bool PascalCString::operator!=
(
    const PascalCString& rhs // @parm What we are comparing too
) const
{
    return( !(*this == rhs) );
}

//=============================================================================
//
// @mfunc Inequality operator
//
// @rdesc Returns non-zero if equal, 0 if un-equal
//
//=============================================================================
bool PascalCString::operator!=
(
    const char* rhs // @parm What we are comparing too
) const
{
    return( !(*this == rhs) );
}

//=============================================================================
//
// @mfunc Equality operator
//
// @rdesc Returns non-zero if equal, 0 if un-equal
//
//=============================================================================
bool PascalCString::operator!=
(
    const char rhs // @parm What we are comparing too
) const
{
    return( !(*this == rhs) );
}

//=============================================================================
//
// @mfunc Less-than operator
//
// @rdesc Returns non-zero if less-than, 0 if greater or equal than
//
//=============================================================================
bool PascalCString::operator<
(
    const PascalCString& rhs // @parm What we are comparing too
) const
{
    // Compare the two strings.
    return( strcmp( m_szString, rhs.m_szString ) < 0 );
}

//=============================================================================
//
// @mfunc Less-than operator
//
// @rdesc Returns non-zero if less than, 0 if equal or greater
//
//=============================================================================
bool PascalCString::operator<
(
    const char* rhs // @parm What we are comparing too
) const
{
    if( rhs )
    {
        // Compare the two strings.
        return( strcmp( m_szString, rhs ) < 0 );
    }
    else
    {
        // The right hand side is a null string, so we can't be less than it.
        return( false );
    }
}

//=============================================================================
//
// @mfunc Less-than operator
//
// @rdesc Returns non-zero if less than, 0 if equal or greater
//
//=============================================================================
bool PascalCString::operator<
(
    const char rhs // @parm What we are comparing too
) const
{
    // Compare the string and the character.
    return( m_szString[0] < rhs );
}

//=============================================================================
//
// @mfunc Greater-than operator
//
// @rdesc Returns non-zero if greater-than, 0 if less or equal than
//
//=============================================================================
bool PascalCString::operator>
(
    const PascalCString& rhs // @parm What we are comparing too
) const
{
    // Compare the two strings.
    return( strcmp( m_szString, rhs.m_szString ) > 0 );
}

//=============================================================================
//
// @mfunc Greater-than operator
//
// @rdesc Returns non-zero if greater than, 0 if equal or less
//
//=============================================================================
bool PascalCString::operator>
(
    const char* rhs // @parm What we are comparing too
) const
{
    if( rhs )
    {
        // Compare the two strings.
        return( strcmp( m_szString, rhs ) > 0 );
    }
    else
    {
        // The right hand side is a null string, so we can't be greater than it.
        return( false );
    }
}

//=============================================================================
//
// @mfunc Greater-than operator
//
// @rdesc Returns non-zero if greater than, 0 if equal or less
//
//=============================================================================
bool PascalCString::operator>
(
    const char rhs // @parm What we are comparing too
) const
{
    // Compare the string and the character.
    if( m_ichMac == 1 )
    {
        return( m_szString[0] > rhs );
    }
    else if( m_ichMac > 1 )
    {
        return( m_szString[0] >= rhs );
    }
    else
    {
        return( false );
    }
}

//=============================================================================
//
// @mfunc Less-than or equal to operator
//
// @rdesc Returns non-zero if less-than or equal, 0 if greater 
//
//=============================================================================
bool PascalCString::operator<=
(
    const PascalCString& rhs // @parm What we are comparing too
) const
{
    // Compare the two strings.
    return( !(*this > rhs) );
}

//=============================================================================
//
// @mfunc Less-than or equal operator
//
// @rdesc Returns non-zero if less than or equal, 0 if greater
//
//=============================================================================
bool PascalCString::operator<=
(
    const char* rhs // @parm What we are comparing too
) const
{
    return( !(*this > rhs) );
}

//=============================================================================
//
// @mfunc Less-than or eqaul operator
//
// @rdesc Returns non-zero if less than or equal, 0 if greater
//
//=============================================================================
bool PascalCString::operator<=
(
    const char rhs // @parm What we are comparing too
) const
{
    return( !(*this > rhs) );
}

//=============================================================================
//
// @mfunc Greater-than or equal to operator
//
// @rdesc Returns non-zero if greater-than or equal, 0 if less
//
//=============================================================================
bool PascalCString::operator>=
(
    const PascalCString& rhs // @parm What we are comparing too
) const
{
    // Compare the two strings.
    return( !(*this < rhs) );
}

//=============================================================================
//
// @mfunc Greater-than or equal operator
//
// @rdesc Returns non-zero if greater than or equal, 0 if less
//
//=============================================================================
bool PascalCString::operator>=
(
    const char* rhs // @parm What we are comparing too
) const
{
    return( !(*this < rhs) );
}

//=============================================================================
//
// @mfunc Greater-than or eqaul operator
//
// @rdesc Returns non-zero if greater than or equal, 0 if less
//
//=============================================================================
bool PascalCString::operator>=
(
    const char rhs // @parm What we are comparing too
) const
{
    return( !(*this < rhs) );
}

//=============================================================================
//
// @mfunc operator[]. array access
//
// @rdesc array access
//
//=============================================================================
char PascalCString::operator[]( const int index ) const
{
    return this->m_szString[ index ];
}

//=============================================================================
//
// @mfunc operator[]. array access
//
// @rdesc array access
//
//=============================================================================
char& PascalCString::operator[]( const int index )
{
    return this->m_szString[ index ];
}


//=============================================================================
//
// @mfunc Length. Return the length of the string.
//
// @rdesc The length (number of characters) in the string (without the null)
//
//=============================================================================
int PascalCString::Length
(
) const
{
    return( m_ichMac );
}

//=============================================================================
//
// @mfunc Reserve space for the string. 
//
//=============================================================================
void PascalCString::Reserve
(
    int nSize // @parm The total number of characters we want in the string.
)
{
    Grow( nSize );
}

//=============================================================================
//
// @mfunc Conversion to a pointer to a character
//
// @rdesc Returns the underlying representation as a pointer to an array of 
//        chars.
//
//=============================================================================
PascalCString::operator char*
(
)
{
    return( m_szString );
}

//=============================================================================
//
// @mfunc Conversion to a pointer to a character
//
// @rdesc Returns the underlying representation as a pointer to an array of 
//        chars.
//
//=============================================================================
PascalCString::operator char*
(
) const
{
    return( m_szString );
}

//=============================================================================
//
// @mfunc Conversion to a pointer to a character
//
// @rdesc Returns the underlying representation as a pointer to an array of 
//        chars.
//
//=============================================================================
/*PascalCString::operator const char*
(
) const
{
    _asm int 3;
    return( (const char*)m_szString );
}*/

//=============================================================================
//
// @mfunc Figure out the new size of the array to resize this thing to
//
// @rdesc A hash value
//
//=============================================================================
void PascalCString::AdjustMaxArraySize( const unsigned int newSize )
{
//    rAssert( reinterpret_cast< unsigned int >( this ) != 0xd00517d0 );
    if( m_ichMax < newSize )
    {
        m_ichMax = newSize + m_uiGrowBy;
    }
}

//=============================================================================
//
// @mfunc Generate a hash value for the string given a modulo value.
//
// @rdesc A hash value
//
//=============================================================================
long PascalCString::Hash
(
    int iModulo             // The modulo to use in the hash table
)
{
    const char* pch = m_szString;
    long lHash = 0L;

    for( ; *pch != 0; ++pch )
    {
        lHash = ( 64 * lHash + *pch ) % iModulo;
    }

    return( lHash );
}

//=============================================================================
//
// @mfunc Finds a maximal match (starting at offset 0) between this string
//        and the supplied string. It is intended to be used to find the 
//        common root of a number of strings.
//
// @ex    For example, given two strings |
//          "R:\PROJECT\BB98\ART\SCREENS\SCREEN04\box.bmp"
//          "R:\PROJECT\BB98\ART\SCREENS\COMMON\backgrnd.bmp"
//
//        This method would return
//          "R:\PROJECT\BB98\ART\SCREENS\"
//
// @rdesc Returns a new PascalCString object that is the maximal match between
//        this string and the passed in string.
//
//=============================================================================
PascalCString PascalCString::Match
(
    const PascalCString& rhs // @parm The string we are trying to match
)
{
    // The maximal match could be the shortest of the two strings.
    int ichMax;
    if( Length() < rhs.Length() )
    {
        ichMax = Length();
    }
    else
    {
        ichMax = rhs.Length();
    }

    for( int ich = 0; ich < ichMax; ++ich )
    {
        if( m_szString[ich] != rhs.m_szString[ich] )
        {
            if( ich == 0 )
            {
                // No match so return a null string.
                return( "" );
            }
            else
            {
                // Take advantage of the return value obtimization (hopefully)
                return( SubString( 0, ich - 1 ) );
            }
        }
    }

    return( SubString( 0, ichMax ) );
}

//=============================================================================
//
// @mfunc Takes a substring out of the given object, starting at the supplied
//        offset and including the specified number of characters.
//
// @rdesc Returns a new PascalCString object that is the requested substring.
//
//=============================================================================
// This function is pretty slow. 
// 1) it allocates a temporary chunk of memory
// 2) it copies a string into that temporary buffer
// 3) it creates a pascal string
// 3.1) the constructor news a string of the correct length
// 3.2) the constructor copies the temporary buffer into its own buffer
// 4) the temporary buffer is thrown out
// wasted overhead: 1 new, 1 strcpy, 1 delete
PascalCString PascalCString::SubString
(
    int iStart, // @parm The starting offset
    int iChars  // @parm The number of characters to retrieve
)
{
    // Make sure we are asking for a valid substring
    assert( iStart + iChars <= int( m_ichMac + 1 ) );

    // Allocate the space for the new substring
    radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
    char* szSub = new char[iChars + 1];
    ::radMemorySetCurrentAllocator( old );

    // Copy the substring...
    strncpy( szSub, (const char*)&(m_szString[ iStart ]), iChars );

    // ...and make sure it is null terminated
    szSub[ iChars ] = 0;

    // Make a string object out of it.
    // g_ScroobyMemoryCount gets updated in the constructor
    old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
    PascalCString stSub( szSub );
    ::radMemorySetCurrentAllocator( old );

    // Free the memory we allocated
    delete[] szSub;  

    // Return the object
    return( stSub );
}

//===========================================================================
// StreamFirstChar
//===========================================================================
// Description: returns the first character from a string, and also removes it
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      the first character in the string
//
//===========================================================================
char PascalCString::StreamFirstChar()
{
    // Remember the first character
    char tempchar = m_szString[ 0 ];
    if( tempchar == 0 )
    {
        return tempchar;
    }

    // Shrink string by one character
    memcpy( m_szString, m_szString + 1, m_ichMac );
    m_ichMac--;
    
    return tempchar;
}

//===========================================================================
// StreamFirstWord
//===========================================================================
// Description: IAN doesn't know what this does - it's used by the XML subsystem
//
// Constraints:    NONE
//
// Parameters:    charToSplit - this is the token that terminates words
//
// Return:      the first token in the string split by the charToSplit parameter
//
//===========================================================================
PascalCString PascalCString::StreamFirstWord( char charToSplit )
{
    //find the pivot point of the string
    unsigned int i = 0;
    while( ( i < this->m_ichMac ) && ( this->m_szString[ i ] != charToSplit ) )
    {
        i++;
    }

    int pivot = i;
    PascalCString word = this->SubString( 0, pivot );
    int newLength = m_ichMac - ( pivot + 1 );
    if( newLength > 0 )
    {
        *this = this->SubString( pivot + 1, m_ichMac - ( pivot + 1 ) );
    }
    else
    {
        *this = "";
    }

    return word;
}

bool PascalCString::Find( char charToFind )
{
    for( int i=0; i<int(m_ichMac); i++ )
    {
        if( m_szString[ i ] == charToFind )
        {
            return true;
        }
    }
    return false;
}

bool PascalCString::EqualsInsensitive( const char* name )
{
    if( m_ichMac == strlen( name ) )
    {
        // Compare the two strings. If they are equal (according to
        // the standard Windows rules for comparison) then strcmp
        // will return 0.
        return( ::rstricmp( m_szString, name ) == 0 );
    }
    else
    {
        // The two strings lengths are different so there is no
        // way that they are equal. This is a very fast check and
        // is much cheaper than doing the actual string comparison.
        return( false );
    }
}

//=============================================================================
//
// @mfunc Replace all occurences of 'stFind' with 'stReplace'
//
//=============================================================================
void PascalCString::Replace( const PascalCString& stFind, const PascalCString& stReplace )
{
    // Do a quick check to see if stFind is > in length than this string,
    // if it is then there can't possibly be a match.
    if ( stFind.Length() > Length() )
    {
        return;
    }

    // Search for stFind in this string, and abort if we don't find it.
    char* szString = m_szString;
    char* szFind = strstr( szString, stFind.m_szString );
    if ( szFind == 0 )
    {
        return;
    }

    // We know that there is at least one occurence of stFind in this string,
    // so we now do the expensive part of replacing each occurence of
    // stFind with stReplace.
    radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
        PascalCString stNew = "";
        while ( szFind != 0 )
        {
            // replace the first character of found string in this string with a null
            *szFind = 0;

            // add what we have so far
            stNew += szString;

            // add in the replacement
            stNew += stReplace;

            // skip ahead by the length of the found string.
            szString = szFind + stFind.Length();

            // and try finding it again
            szFind = strstr( szString, stFind.m_szString );
        }
        stNew += szString;
    ::radMemorySetCurrentAllocator( old );

    // Last, but not least, assign the result to ourselves.
    *this = stNew;
}

//=============================================================================
//
// @mfunc Change all characters to uppercase.
//
//=============================================================================
void PascalCString::ToUpper( void )
{
    int index = 0;
    for ( index; index < Length(); ++index )
    {
        m_szString[ index ] = toupper( m_szString[ index ] );
    }
}

//=============================================================================
//
// @mfunc Change all characters to lowercase.
//
//=============================================================================
void PascalCString::ToLower( void )
{
    int index = 0;
    for ( index; index < Length(); ++index )
    {
        m_szString[ index ] = tolower( m_szString[ index ] );
    }
}

//=============================================================================
//
// @mfunc Retrieve the filename from a file/path string
//
//=============================================================================
PascalCString PascalCString::FullFilename()
{
    int index = Length();
    for( index; index >= 0; index-- )
    {
        if( (m_szString[index] == '/') || (m_szString[index] == '\\') )
        {
            break;
        }
    }
    radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
    PascalCString rValue( m_szString + index + 1 );
    ::radMemorySetCurrentAllocator( old );
    return rValue;
}

//=============================================================================
//
// @mfunc Retrieve the filename from a file/path string
//
//=============================================================================
PascalCString PascalCString::JustFilename()
{
    int index = Length();
    for( index; index >= 0; index-- )
    {
        if( (m_szString[index] == '/') || (m_szString[index] == '\\') )
        {
            break;
        }
    }
    char temp[20];
    strcpy( temp, m_szString + index + 1);
    for( index = 0; index < (int)strlen( temp ); index++ )
    {
        if( temp[index] == '.' )
        {
            temp[index] = 0;
            break;
        }
    }
    PascalCString rValue( temp );
    return rValue;
}


//=============================================================================
//
// @mfunc Retrieve the extension from a file/path string
//
//=============================================================================
PascalCString PascalCString::JustExtension()
{
    int index = Length();
    bool found = false;
    for( index; index > 0; index-- )
    {
        if( m_szString[index] == '.' )
        {
            found = true;
            break;
        }
    }
    radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
    PascalCString rValue;
    if( found )
    {
        rValue = m_szString + index + 1;
    }
    else
    {
        rValue = "";
    }
    ::radMemorySetCurrentAllocator( old );
    return rValue;
}

//=============================================================================
//
// @mfunc Retrieve the path portion from a file/path string
//
//=============================================================================
PascalCString PascalCString::JustPath()
{
    int index = Length();
    for( index; index > 0; index-- )
    {
        if( (m_szString[index] == '/') || (m_szString[index] == '\\') )
        {
            break;
        }
    }
    return SubString( 0, index );
}
