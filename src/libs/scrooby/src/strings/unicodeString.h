//===========================================================================
// Copyright (C) 1999 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Unicode Module
//
// Description: Class definitions for a Unicode String UnicodeString
//
// Authors:     Ian Gipson
//
// Revisions        Date        Author    Revision
//                    99/10/04    IJG        Created
//
//===========================================================================

// Recompilation protection flag.
#ifndef __UNICODESTRING_H
#define __UNICODESTRING_H


//===========================================================================
// Nested Includes
//===========================================================================
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
#pragma warning ( disable : 4786 ) 
#endif
#include <stddef.h>

//===========================================================================
// Forward References
//===========================================================================

//===========================================================================
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//===========================================================================
typedef unsigned short UnicodeChar;

//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: Class represents a string of double byte characters that is 
//                not tied into MFC
//
// Constraints: 
//
//===========================================================================
class UnicodeString
{
    //DATA MEMBERS-------------------------------------------------------------
    protected:
    UnicodeChar*        m_charVector ;
    int                 m_vectorSize;

    //FUNCTIONS----------------------------------------------------------------
    public:
        //default constructor
        UnicodeString() ;

        //copy constructor
        UnicodeString( const UnicodeString& right) ;
        UnicodeString( const char* right );

        //destructor
        ~UnicodeString() ;

        //assignment operator
        UnicodeString& operator=( const UnicodeString& right ) ;

        //appends a character to the end of the string
        void Append( const UnicodeChar& c ) ;

        //emptys the string
        void Clear() ;

        //gets direct access to the character buffer
        UnicodeChar* GetBuffer() ;

        //string length operatorl
        int Length() const ;

        //prints some version of the string in an ASCII format
        void MakeAscii( char* buffer, int size ) const ;

        //prints some version of the string in a Unicode format
        void MakeWide( UnicodeChar* buffer, int size ) const ;

        //append operator for ascii strings
        void operator+=( const UnicodeString& right );
        void operator+=( const UnicodeChar&   right );

        //equality operator
        bool operator==( const UnicodeString& right ) const ;

        //equality operator for ascii strings
        bool operator==( const char* right ) const ;

        //less than operator
        bool operator< ( const UnicodeString& right ) const ;

        //random acess operator
        const UnicodeChar& operator[]( const int index ) const ;
              UnicodeChar& operator[]( const int index );

        //constructs a unicode string from an ASCII string
        void ReadAscii( const char* asciiString, const int count = -1 ) ;
    
        //constructs a unicode string from a unicode string
        void ReadUnicode( const char* unicodeString, const int lengthInBytes = -2 ) ;

        //constructs a unicode string from a wchar_t array
        void ReadUnicode( const UnicodeChar* unicodeString, const int lengthInDblBytes = -1 ) ;

        //replaces character in a unicode string
        void Replace( const UnicodeChar find, const UnicodeChar replace ) ;

        //replaces a substring in a unicode string with another string
        void Replace( const UnicodeString& find, const UnicodeString& replace );

#ifndef RAD_RELEASE
        // Run this as a coverage test
        static void Test();
#endif

    protected:
        int  FindFirstSubstring( const UnicodeString& str ) const;
        void Resize( unsigned int newSize );
        UnicodeString Substring( const unsigned int index, const unsigned int size ) const;

    private:
 
};


#endif                                  // End conditional inclusion
