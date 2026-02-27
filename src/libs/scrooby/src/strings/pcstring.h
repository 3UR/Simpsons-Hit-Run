// @doc

//=============================================================================
//
// File: PCSTRING.H
// Copyright (c) 1997 Radical Entertainment Ltd. All rights reserved.
//
// Class(es):   PascalCString
//
// Parent(s):   None
//
// This file contains the representation of a Pascal-C string. This string, 
// when written to a file has its length (including the terminating null) 
// pre-pended to the actual string data. This makes loading the string from 
// disk MUCH faster.
//
// Authors:     Kevin M. Loken (KML)
//
//=============================================================================
//  Version    Author    CR       Date      Brief Description
//-----------------------------------------------------------------------------
//  00.90.00    KML     ----    19-Aug-97   File creation.
//=============================================================================

//=============================================================================
// Prevent multiple inclusion
//=============================================================================
#ifndef __PCSTRING_H
#define __PCSTRING_H

//=============================================================================
// Nested includes
//=============================================================================
//#include "stdgfe.h"
#include "strings/unicodeString.h"

//=============================================================================
// Forward references
//=============================================================================
class GfeFile;

//=============================================================================
//
// @class PascalCString | This is a representation of a string that supports
//        a "natural" interface to the string (equality, comparison, 
//        and concatentation for example). It hides all of the memory allocation
//        and management from the user. It also supports the efficient reading
//        and writing of data from/to persistent storage by prefixing the string
//        with its length.
//
class PascalCString
{
    public:
        // @cmember Default constructor
        PascalCString();

        // @cmember Destructor.
        ~PascalCString();

        // @cmember Copy constructor from const UnicodeString
        explicit PascalCString( const UnicodeString& uString );

        // @cmember Copy constructor from const char*
        PascalCString( const char* szString );

        // @cmember Copy constructor from char
        PascalCString( const char ch );

        // @cmember Copy constructor from another PCString
        PascalCString( const PascalCString& rhs );

        // @cmember Assignment operator from another PCString
        PascalCString& operator=( const PascalCString& rhs );

        // @cmember Assignment operator from a string
        PascalCString& operator=( const char* rhs );

        // @cmember Assignment operator from a char;
        PascalCString& operator=( const char rhs );

        // @cmember Concatentation
        PascalCString& operator+=( const PascalCString& rhs );

        // @cmember Concatentation
        PascalCString& operator+=( const char* rhs );

        // @cmember Concatentation
        PascalCString& operator+=( const char rhs );

        //array access
        char operator[]( const int index ) const;

        //array access
        char& operator[]( const int index );

        // @cmember comparison (equality) operator
        bool operator==( const PascalCString& rhs ) const;

        // @cmember comparison (equality) operator
        bool operator==( const char* rhs ) const;

        // @cmember comparison (equality) operator
        bool operator==( const char rhs ) const;

        // @cmember comparison (inequality) operator
        bool operator!=( const PascalCString& rhs ) const;

        // @cmember comparison (inequality) operator
        bool operator!=( const char* rhs ) const;

        // @cmember comparison (inequality) operator
        bool operator!=( const char rhs ) const;

        // @cmember comparison (less than) operator
        bool operator<( const PascalCString& rhs ) const;

        // @cmember comparison (less than) operator
        bool operator<( const char* rhs ) const;

        // @cmember comparison (less than) operator
        bool operator<( const char rhs ) const;

        // @cmember comparison (less than) operator
        bool operator<=( const PascalCString& rhs ) const;

        // @cmember comparison (less than) operator
        bool operator<=( const char* rhs ) const;

        // @cmember comparison (less than) operator
        bool operator<=( const char rhs ) const;

        // @cmember comparison (greater than) operator
        bool operator>( const PascalCString& rhs ) const;

        // @cmember comparison (greater than) operator
        bool operator>( const char* rhs ) const;

        // @cmember comparison (greater than) operator
        bool operator>( const char rhs ) const;

        // @cmember comparison (less than) operator
        bool operator>=( const PascalCString& rhs ) const;

        // @cmember comparison (less than) operator
        bool operator>=( const char* rhs ) const;

        // @cmember comparison (less than) operator
        bool operator>=( const char rhs ) const;

        // @cmember Get the length (exclusive of null) of string
        int Length() const;

        // @cmember Reserve space for the string.
        void Reserve( int nSize );

        // @cmember Implicit type conversion operator
        operator char*();

        // @cmember Implicit type conversion operator
        operator char*() const;

        // @cmember Grow the character array
        void Grow( unsigned int uiLen );

        // @cmember Generate a hash value for this string
        long Hash( int iModulo );

        // @cmember Read the string from the file
        //void ReadFile( GfeFile& gf );

        // @cmember Write the string to a file
        //void WriteFile( GfeFile& gf );

        // @cmember Find a match between the two strings
        PascalCString Match( const PascalCString& rhs );

        // @cmember Replace all occurences of the first string with the second
        void Replace( const PascalCString& stFind, const PascalCString& stReplace );

        // @cmember Make the string all uppercase
        void ToUpper( void );

        // @cmember Make the string all lowercase
        void ToLower( void );

        // @cmember Get a substring from the object
        PascalCString SubString( int iStart, int iChars );

        char StreamFirstChar();
        PascalCString StreamFirstWord( char charToSplit );
        bool Find( char charToFind );
        bool EqualsInsensitive( const char* name );

        // Special functions just for file entries
        PascalCString JustFilename();
        PascalCString FullFilename();
        PascalCString JustExtension();
        PascalCString JustPath();

    protected:
        void AdjustMaxArraySize( const unsigned int newSize );

    private:

        // @cmember The array of chars
        char* m_szString;

        // @cmember The number of characters in the string (strlen + 1)
        unsigned int m_ichMac;

        // @cmember The size of the array.
        unsigned int m_ichMax;

        // @cmember The "grow by" size;
        unsigned int m_uiGrowBy;
        #ifdef RAD_DEBUG
            int m_NumberOfGrows;
        #endif
};

#endif // End of conditional inclusion

