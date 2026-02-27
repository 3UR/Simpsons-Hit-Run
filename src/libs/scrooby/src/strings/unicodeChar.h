//===========================================================================
// Copyright (C) 1999 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Unicode Module
//
// Description: Class definitions for a Unicode Character unicodeChar
//
// Authors:     Ian Gipson
//
// Revisions        Date        Author    Revision
//                    99/09/30    IJG        Created
//
//===========================================================================

// Recompilation protection flag.
#ifndef __UNICODECHAR_H
#define __UNICODECHAR_H


//===========================================================================
// Nested Includes
//===========================================================================
#include "../utility/memory.h"
//===========================================================================
// Forward References
//===========================================================================


//===========================================================================
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//===========================================================================
//using std::ostream ;
//using std::istream ;


//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: Generally, describe what behaviour this class possesses that
//              clients can rely on, and the actions that this service
//              guarantees to clients.
//
// Constraints: Describe here what you require of the client which uses or
//              has this class - essentially, the converse of the description
//              above. A constraint is an expression of some semantic
//              condition that must be preserved, an invariant of a class or
//              relationship that must be preserved while the system is in a
//              steady state.
//
//===========================================================================

class UnicodeChar
{
    public:
        UnicodeChar() ;
        UnicodeChar( const unsigned short right ) ;
        UnicodeChar( const unsigned char hiByte, const unsigned char loByte ) ;

        char Ascii() const ;
        bool IsExtended() const ;
        UnicodeChar& operator=( const UnicodeChar& right );
        bool operator==( const UnicodeChar& right ) const ;
        bool operator!=( const UnicodeChar& right ) const ;
        bool operator<( const UnicodeChar& right ) const ;
        bool operator>( const UnicodeChar& right ) const ;
        bool operator<=( const UnicodeChar& right ) const ;
        bool operator>=( const UnicodeChar& right ) const ;
        operator unsigned short() const ;
        void SwapEndian();            
//        friend ostream& operator<< ( ostream& os, const UnicodeChar& c ) ;
//        friend istream& operator>> ( istream& is, UnicodeChar& c ) ;


    protected:
        unsigned char    loByte ;        //one byte of the two byte unicode character
        unsigned char    hiByte ;        //second byte of the two byte unicode character

    private:
 
};

//===========================================================================
// Inline Functions
//===========================================================================


#endif                                  // End conditional inclusion
