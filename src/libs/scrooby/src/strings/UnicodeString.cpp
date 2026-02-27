//===========================================================================
// Copyright (C) 1999 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Unicode
//
// Description: This file contains the implementation of the class UnicodeString
//
// Authors:     Ian Gipson
//
// Revisions        Date        Author    Revision
//                    99/10/04    IJG        Created
//
//===========================================================================

//---------------------------------------------------------------------------
// Includes
//===========================================================================

#include <assert.h>
#include <string.h>
#include "utility/memory.h"
#include "strings/unicodeString.h"
#include "strings/pcstring.h"


//===========================================================================
// Local Constants, Typedefs, and Macros
//===========================================================================

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================

//===========================================================================
// Member Functions
//===========================================================================
void test()
{
    static int tested = false;
    if( tested )
    {
        return;
    }
    tested = true;
    UnicodeString master = "x%dx";
    UnicodeString replace = "300";
    master.Replace( "%d", replace );
}
//===========================================================================
// UnicodeString::UnicodeString()
//===========================================================================
// Description: Default Constructor
//
//===========================================================================
UnicodeString::UnicodeString()
:
m_charVector( NULL )
{
    Clear();
    test();
};


//===========================================================================
// UnicodeString::UnicodeString( const UnicodeString& right )
//===========================================================================
// Description: copy Constructor
//
//===========================================================================
UnicodeString::UnicodeString( const UnicodeString& right )
:
m_charVector( NULL )
{
    if( this != &right )
    {
        Clear();
        *this = right;
    }
};

//===========================================================================
// UnicodeString::UnicodeString( const UnicodeString& right )
//===========================================================================
// Description: copy Constructor
//
//===========================================================================
UnicodeString::UnicodeString( const char* right ):
    m_charVector( NULL )
{
    size_t length = strlen( right );
    ReadAscii( right, length );
}

//===========================================================================
// UnicodeString::UnicodeString()
//===========================================================================
// Description: destructor
//
//===========================================================================
UnicodeString::~UnicodeString() 
{
    Clear();
};



//===========================================================================
// UnicodeString& UnicodeString::operator=( const UnicodeString& right )
//===========================================================================
// Description: Assignment Operator
//
//===========================================================================
UnicodeString& UnicodeString::operator=( const UnicodeString& right )
{
    if( &right == this )
    {
        return *this;
    }

    // Wipe the current object
    if( m_vectorSize < right.m_vectorSize )
    {
        Clear();
        // Copy the size
        Resize( right.m_vectorSize );
    }
    // Copy the character vector
    if( m_vectorSize > 0 )
    {
        if( right.m_charVector != NULL )
        {
            memcpy( m_charVector, right.m_charVector, (right.m_vectorSize + 1) * sizeof( UnicodeChar ) );
        }
        else
        {
            m_charVector[0] = 0x0000;
        }
    }
    return *this;
};



//===========================================================================
// bool UnicodeString::operator==( const UnicodeString& right ) const 
//===========================================================================
// Description: Equality Operator
//
// Constraints:    none
//
// Parameters:    none
//
// Return:      none
//
//===========================================================================
bool UnicodeString::operator==( const UnicodeString& right ) const 
{
    UnicodeChar* cLeft = m_charVector;
    UnicodeChar* cRight = right.m_charVector;
    if( !cLeft || !cRight )
    {
        return ( !cLeft && !cRight );
    }
    while( ( *cLeft != 0x0000  ) && ( *cRight != 0x0000 ) )
    {
        if( *cLeft != *cRight )
        {
            return false;
        }
        cLeft++;
        cRight++;
    }

    // At least one of the strings is at it's end.  Check to make sure both are
    if( *cLeft != *cRight )
    {
        return false;
    }
    return true;
};

//===========================================================================
// void UnicodeString::operator+=( const UnicodeString& right ) const;
//===========================================================================
// Description: append operator
//
// Constraints:    This will be slow, since we need to grow the array every time
//
//===========================================================================
void UnicodeString::operator+=( const UnicodeString& right )
{
    int length = Length();
    int rightLength = right.Length();
    if( length+rightLength > 0 )
    {        
        UnicodeChar* newArray = new UnicodeChar[length+rightLength+1];
        
        // copy both strings into the new array
        memcpy( newArray, m_charVector, length * sizeof(UnicodeChar) );
        memcpy( newArray+length, right.m_charVector, rightLength * sizeof(UnicodeChar) );
        newArray[length+rightLength] = 0x0000;
        
        Clear();
        m_charVector = newArray;
        m_vectorSize = length + rightLength;
    }
}

//===========================================================================
// UnicodeString::operator+=
//===========================================================================
// Description: append operator
//
// Constraints:    This will be slow, since we need to grow the array every time
//
//===========================================================================
void UnicodeString::operator+=( const UnicodeChar& right )
{
    UnicodeString addme;
    addme.Resize( 1 );
    addme[ 0 ] = right;
    operator+=( addme );
}

//===========================================================================
// bool UnicodeString::operator< ( const UnicodeString& right ) const
//===========================================================================
// Description: less than operator
//
// Parameters:    the other unicode string to compare to the current object
//
// Return:      true if the current object is less than the passed object,
//              false otherwise
//
//===========================================================================
bool UnicodeString::operator< ( const UnicodeString& right ) const
{
    int smallerSize;
    if( Length() < right.Length() )
    {
        smallerSize = Length();
    }
    else
    {
        smallerSize = right.Length();
    }

    for( int i = 0; i < smallerSize; i++ )
    {
        if( ( *this )[ i ] < right[ i ] )
        {
            return true;
        }
        if( ( *this )[ i ] > right[ i ] )
        {
            return false;
        }
    }

    //we got to the end of the smaller string, and theyre equal so far
    if( Length() < right.Length() )
    {
        return true;
    }

    return false;
};




//===========================================================================
// UnicodeChar UnicodeString::operator[]( const int index ) const 
//===========================================================================
// Description: random acess operator Operator
//
// Constraints:    the index must be less than the length of the string
//
// Parameters:    index - the index position of the character to retrieve
//
// Return:      the unicode character at that location
//
//===========================================================================
const UnicodeChar& UnicodeString::operator[]( const int index ) const 
{
    assert( index < Length() + 1 );
    return m_charVector[ index ];
}

//===========================================================================
// UnicodeChar UnicodeString::operator[]( const int index ) 
//===========================================================================
// Description: random acess operator Operator
//
// Constraints:    the index must be less than the length of the string
//
// Parameters:    index - the index position of the character to retrieve
//
// Return:      the unicode character at that location
//
//===========================================================================
UnicodeChar& UnicodeString::operator[]( const int index )
{
    assert( index < Length() + 1 );
    return m_charVector[ index ];   
}

//===========================================================================
// void UnicodeString::Append( const UnicodeChar& c )
//===========================================================================
// Description: appends a character to the end of the string
//
// Constraints: this will likely grow the array when called, so try and avoid
//
// Parameters:  c - the character to add to the array
//
//===========================================================================
void UnicodeString::Append( const UnicodeChar& c )
{
    int strLength = Length();

    // add one for the null character, and one for the new character   
    if( m_vectorSize < strLength + 2 )
    {
        //resizing is required
        UnicodeChar* newArray = new UnicodeChar[strLength+2];
        memcpy( newArray, m_charVector, strLength * sizeof(UnicodeChar) );

        Clear();
        m_charVector = newArray;
        m_vectorSize = strLength+1;
    }
    m_charVector[ strLength ] = c;
    m_charVector[ strLength + 1 ] = 0x0000;
}

//===========================================================================
// void UnicodeString::Clear()
//===========================================================================
// Description: clears the string
//
//===========================================================================
void UnicodeString::Clear()
{
    if( m_charVector )
    {
        delete[] m_charVector;
        m_charVector = NULL;
    }
    m_vectorSize = 0;
};

//===========================================================================
// UnicodeString::FindFirstSubstring
//===========================================================================
// Description: finds the first instance of the substring in the master string
//
//===========================================================================
int UnicodeString::FindFirstSubstring( const UnicodeString& str ) const
{
    int i;
    int strLength = str.Length();
    int size = Length() - strLength + 1;
    for( i = 0; i < size; ++i )
    {
        int j;
        bool matching = true;
        for( j = 0; j < strLength; ++j )
        {
            bool charEqual = ( operator []( i + j ) == str[ j ] );
            if( !charEqual )
            {
                matching = false;
                break;
            }
        }
        if( matching )
        {
            return i;
        }
    }
    return -1;
}

//===========================================================================
// UnicodeChar* UnicodeString::GetBuffer()
//===========================================================================
// Description: gets direct acess to the buffer
//
// Return:      UnicodeChar* pointer to the buffer 
//
//===========================================================================
UnicodeChar* UnicodeString::GetBuffer()
{
    return m_charVector;
}

//===========================================================================
// int UnicodeString::Length() const
//===========================================================================
// Description: string length operator
//
// Return:      the length of the null terminated string in the buffer
//
//===========================================================================
int UnicodeString::Length() const
{
    if( m_charVector == NULL )
    {
        return 0;
    }

    int i = 0;
    while( m_charVector[ i ] != UnicodeChar( 0x0000 ) )
    {
        i++;
    }
    return i;
}


//===========================================================================
// void UnicodeString::MakeAscii( char* buffer, int size ) const 
//===========================================================================
// Description: prints some version of the string in an ASCII format
//
// Constraints:    none
//
// Parameters:    none
//
// Return:      none
//
//===========================================================================
void UnicodeString::MakeAscii( char* buffer, int size ) const 
{
    int length = Length();
    assert( size >= length + 1 );

    int i;
    for( i = 0; i < Length(); i++ )
    {
        // Because the byte order can be reversed in big endian systems, we have to use subtraction
        // instead of bit tests
        buffer[ i ] = ( (m_charVector[i] - static_cast<unsigned char>( m_charVector[i] )) == 0) ? static_cast<char>( m_charVector[i] ) : '?';
    }
    buffer[ i ] = 0; 
}

//===========================================================================
// void UnicodeString::MakeWide( unsigned short* buffer, int size ) const;
//===========================================================================
// Description: prints some version of the string in an ASCII format
//
// Constraints:    none
//
// Parameters:    none
//
// Return:      none
//
//===========================================================================
void UnicodeString::MakeWide( unsigned short* buffer, int size ) const
{
    assert( size >= Length() + 1 );
    memcpy( buffer, m_charVector, size*2 );
}


//===========================================================================
// bool UnicodeString::operator==( const char* right ) const;
//===========================================================================
// Description: equality operator for ascii strings
//
// Constraints:    none
//
// Parameters:    none
//
// Return:      none
//
//===========================================================================
bool UnicodeString::operator==( const char* right ) const
{
    //make the ASCII string a unicode string, and perform the comparison
    UnicodeString unicodeRight;
    unicodeRight.ReadAscii( right );
    return ( *this ) == unicodeRight;
}

//===========================================================================
// UnicodeString::ReadAscii( const char* asciiString )
//===========================================================================
// Description: Constructs a Unicode string from an ASCII string
//
// Parameters:    asciiString - a pointer to a char array that contains a string
//
//===========================================================================
void UnicodeString::ReadAscii( const char* asciiString, const int count )
{
    Clear();
    int length = 0;
    if( count == -1 )
    {
        //autodetect the string length
        length = strlen( asciiString );
    }
    else
    {
        //otherwise use the length that was passed in
        length = count;
    }
    
    Resize( length );

    for( int i = 0; i < length; i++ )
    {
        m_charVector[ i ] = static_cast<UnicodeChar>( asciiString[i] );
        m_charVector[ i ] &= 0x00FF; // set unicode page to ascii
    }
    m_charVector[ length ] = 0x0000;
};


//===========================================================================
// UnicodeString::ReadUnicode( const UnicodeChar* unicodeString, const int lengthInDblBytes ) 
//===========================================================================
// Description: constructs a unicode string from a UnicodeChar array
//
// Constraints:    lengthInDblBytes must be accurate
//
// Parameters:    unicodeString - a pointer to an array of UnicodeChar that contain
//                the unicode string
//
//                lengthInBytes - the number of double bytes in the unicode string
//
//===========================================================================
void UnicodeString::ReadUnicode( const UnicodeChar* unicodeString, const int lengthInDblBytes )
{
    Clear();
    m_vectorSize = lengthInDblBytes;
    if( m_vectorSize < 0 )
    {
        m_vectorSize = 0;
        while( unicodeString[m_vectorSize] != 0x0000 )
        {
            m_vectorSize++;
        }
        //m_vectorSize++;
    }
    Resize( m_vectorSize );
    memcpy( m_charVector, unicodeString, (m_vectorSize + 1) * sizeof( UnicodeChar ) );
    m_charVector[ m_vectorSize ] = 0x0000;
}

//===========================================================================
// UnicodeString::ReadUnicode( const char* unicodeString, const int lengthInBytes ) 
//===========================================================================
// Description: Constructs a Unicode string from an Unicode string
//
// Constraints:    lengthInBytes must be accurate
//
// Parameters:    unicodeString - a pointer to an array of characters that contain
//                the unicode string
//
//                lengthInBytes - the number of bytes in the unicode string
//
// Return:      none
//
//===========================================================================
void UnicodeString::ReadUnicode( const char* unicodeString, const int lengthInBytes )
{
    // needs to be in UnicodeChar sized increments
    assert( lengthInBytes % sizeof(UnicodeChar) == 0 );
    ReadUnicode( reinterpret_cast<const UnicodeChar*>(unicodeString), lengthInBytes / 2 );
}


//===========================================================================
// void UnicodeString::Replace( const UnicodeChar find, const UnicodeChar replace )
//===========================================================================
// Description: replaces character in a unicode string
//
// Parameters:    find - character to find
//              replace - character to change instances of find to
//
//===========================================================================
void UnicodeString::Replace( const UnicodeChar find, const UnicodeChar replace )
{
    int size = Length();
    for( int i = 0; i < size; i++ )
    {
        if( m_charVector[ i ] == find )
        {
            m_charVector[ i ] = replace;
        }
    }
}

//===========================================================================
// UnicodeString::Replace
//===========================================================================
// Description: replaces a substring in a unicode string with another string
//
// Parameters:  find    - substring to find
//              replace - substring to change instances of find to
//
//===========================================================================
void UnicodeString::Replace( const UnicodeString& find, const UnicodeString& replace )
{
    //
    // Find the initial string
    //
    int index = FindFirstSubstring( find );

    // didn't find it - early out
    if( index == -1 )
    {
        return;
    }

    //
    // Split the string at the find point
    //
    int myLength = Length();
    int findLength = find.Length();
    int preLength = index;
    UnicodeString pre  = Substring( 0, preLength );
    int postLength = myLength - preLength - findLength;
    UnicodeString post = Substring( index + findLength, postLength );

    UnicodeString newString = pre;
    newString += replace;
    newString += post;
    *this = newString;
}

//===========================================================================
// UnicodeString::Resize
//===========================================================================
// Description: resizes the internal buffer
//
// Parameters:  newsize = the size in characters to resize to
//
//===========================================================================
void UnicodeString::Resize( unsigned int newSize )
{
    // MIKE IMPROVE : Should this copy data so Append and += can use it?
    Clear();
    m_vectorSize = newSize;
    UnicodeChar* newBuffer = new UnicodeChar[ m_vectorSize + 1 ];
    m_charVector = newBuffer;
    int i;
    int size = m_vectorSize + 1;
    for( i = 0; i < size; ++i )
    {
        m_charVector[ i ] = 0x0000;
    }
}

//===========================================================================
// UnicodeString::Substring
//===========================================================================
// Description: Gets a substring out of a unicode string
//
// Parameters:  find    - substring to find
//              replace - substring to change instances of find to
//
//===========================================================================
UnicodeString UnicodeString::Substring( const unsigned int index, const unsigned int size ) const
{
    UnicodeString returnMe;
    unsigned int i;
    for( i = index; i < index + size; ++i  )
    {
        const UnicodeChar& appendMe = operator[]( i );
        returnMe += appendMe;
    }
    return returnMe;
}



#ifndef RAD_RELEASE
void UnicodeString::Test()
{
    // Test equality operator
    UnicodeString string1;
    UnicodeString string2;
    assert( string1 == string2 );
    string2.ReadAscii( "Blah" );
    assert( !(string1 == string2) );
    assert( string2 == "Blah" );

    // test copy constructor
    UnicodeString string3( string1 );
    UnicodeString string4( string2 );
    assert( string1 == string3 );
    assert( string2 == string4 );

    // test concatenation operator
    string4.ReadAscii( "BlahBlah" );
    string2 += string2;
    string1 += string1;
    assert( string1 == string3 );
    assert( string2 == string4 );

    // test clear
    string1.ReadAscii( "Blah" );
    string1.Clear();
    assert( string1 == string3 );

    // test replace
    string2.ReadAscii( "Blah" );
    string2.Replace( 0xff00, (UnicodeChar)'a');
    string2.Replace( (UnicodeChar)'h', (UnicodeChar)'t' );
    string4.ReadAscii( "Blat" );
    assert( string2 == string4 );

    // test less than operator
    string1.Clear();
    string2.ReadAscii( "a" );
    string3.ReadAscii( "b" );
    string4.ReadAscii( "aa" );
    assert( string1 < string2 );
    assert( string2 < string3 );
    assert( string4 < string3 );
    assert( string2 < string4 );
    string1.ReadAscii( "" );
    assert( string1 < string2 );

    // test append
    int i = 0;
    string1.Clear();
    for( i; i < 5; i++ )
    {
        string1.Append( static_cast<UnicodeChar>('a') );
    }
}
#endif
