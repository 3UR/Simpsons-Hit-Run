//===========================================================================
// Copyright (C) 1999 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Unicode
//
// Description: This file contains the implementation of the class UnicodeChar
//
// Authors:     Ian Gipson
//
// Revisions        Date        Author    Revision
//                    99/09/30    IJG        Created
//
//===========================================================================

//---------------------------------------------------------------------------
// Includes
//===========================================================================

#include "unicodeChar.h"       // XxxClassName specification/inlines


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
// UnicodeChar::UnicodeChar()
//===========================================================================
// Description: Default Constructor
//
// Constraints:    none
//
// Parameters:    none
//
// Return:      none
//
//===========================================================================
UnicodeChar::UnicodeChar()
:
    loByte( 0x00 ),
    hiByte( 0x00 )
{
};

//===========================================================================
// UnicodeChar::UnicodeChar()
//===========================================================================
// Description: constructor from a 16bit integer
//
// Constraints:    none
//
// Parameters:    hiByte - the 8 bits of the high portion of the character
//                loByte - the 8 bits of the lo   portion of the character
//            
//
// Return:      none
//
//===========================================================================
UnicodeChar::UnicodeChar( const unsigned char hiByte, const unsigned char loByte )
:
    loByte( loByte ),
    hiByte( hiByte )
{
}

//===========================================================================
// UnicodeChar::UnicodeChar()
//===========================================================================
// Description: constructor from a 16bit integer
//
// Constraints:    none
//
// Parameters:    right - a 16 bit number that contains the unicode character
//                this could also be an 8 bit ascii character that has been 
//                promoted to a 16bit number.
//
// Return:      none
//
//===========================================================================
UnicodeChar::UnicodeChar( const unsigned short right)
{
#ifndef __BIG_ENDIAN__
    this->loByte = right & 0x00FF ;
    this->hiByte = ( right & 0xFF00 ) >> 8 ;
#else
    this->loByte = ( right & 0xFF00 ) >> 8 ;
    this->hiByte = right & 0x00FF ;
#endif
}



//===========================================================================
// UnicodeChar::operator<( const UnicodeChar& right ) const
//===========================================================================
// Description: if the character is an Ascii character, then it gets returned
//                otherwise it returns a '?'
//
// Constraints:    none
//
// Parameters:    none
//
// Return:      bool - true or false
//
//===========================================================================
char UnicodeChar::Ascii() const
{
    if( hiByte == 0 )
    {
        return loByte ;
    }
    else
    {
        return '?' ;
    }
}

//===========================================================================
// IsExtended
//===========================================================================
// Description: if the character is beyond the base ascii set, then return
//                true 
//
// Constraints:    none
//
// Parameters:    none
//
// Return:      bool - true or false
//
//===========================================================================
bool UnicodeChar::IsExtended() const 
{
    //check to see if value of the character is less than 0x0080 (128)
    if( ( hiByte == 0x00 ) && (loByte < 0x80 ) )
    {
        return false ;
    }
    else
    {
        return true ;
    }
}

//===========================================================================
// bool UnicodeChar::operator=( const UnicodeChar& right )
//===========================================================================
// Description: assignment operator
//
// Constraints:    none
//
// Parameters:    right - a unicode character that is to be assigned from
//
// Return:      reference to the current object
//
//===========================================================================
UnicodeChar& UnicodeChar::operator=( const UnicodeChar& right )
{
    this->loByte = right.loByte;
    this->hiByte = right.hiByte;
    return *this;
}

//===========================================================================
// bool UnicodeChar::operator==( const UnicodeChar& right ) const
//===========================================================================
// Description: equality operator
//
// Constraints:    none
//
// Parameters:    right - a unicode character that is to be compared against
//
// Return:      bool - true or false
//
//===========================================================================
bool UnicodeChar::operator==( const UnicodeChar& right ) const
{
    //IMPROVE: make this way more efficient by doing a bitwise comparison
    if( *this < right ) 
    {
        return false ;
    }
    else if( right < *this )
    {
        return false ;
    }
    else
    {
        return true ;
    }
}




//===========================================================================
// bool UnicodeChar::operator!=( const UnicodeChar& right ) const
//===========================================================================
// Description: inequality operator
//
// Constraints:    none
//
// Parameters:    right - a unicode character that is to be compared against
//
// Return:      bool - true or false
//
//===========================================================================
bool UnicodeChar::operator!=( const UnicodeChar& right ) const
{
    return !( *this == right ) ;
}

//===========================================================================
// UnicodeChar::operator<( const UnicodeChar& right ) const
//===========================================================================
// Description: less than operator - this is only really good for sorting in 
//                stl sets and the like.  It's not unicode ordering
//
// Constraints:    none
//
// Parameters:    right - a unicode character that is to be compared against
//
// Return:      bool - true or false
//
//===========================================================================
bool UnicodeChar::operator<( const UnicodeChar& right ) const
{
    if( hiByte < right.hiByte )
    {
        return true ;
    }
    else if( hiByte > right.hiByte )
    {
        return false ;
    }
    else if( loByte < right.loByte )
    {
        return true ;
    }
    else
    {
        return false ;
    }
};

//===========================================================================
// bool UnicodeChar::operator<( const UnicodeChar& right ) const 
//===========================================================================
// Description: less than operator 
//
// Constraints:    none
//
// Parameters:    right - a unicode character that is to be compared against
//
// Return:      bool - true or false
//
//===========================================================================
bool UnicodeChar::operator>( const UnicodeChar& right ) const 
{
    return ( right < *this ) ;
}

//===========================================================================
// bool operator<=( const UnicodeChar& right ) const
//===========================================================================
// Description: greater than equal to operator 
//
// Constraints:    none
//
// Parameters:    right - a unicode character that is to be compared against
//
// Return:      bool - true or false
//
//===========================================================================
bool UnicodeChar::operator<=( const UnicodeChar& right ) const
{
    return !( *this > right );     
}

//===========================================================================
// bool operator>=( const UnicodeChar& right ) const
//===========================================================================
// Description: greater than equal to operator 
//
// Constraints:    none
//
// Parameters:    right - a unicode character that is to be compared against
//
// Return:      bool - true or false
//
//===========================================================================
bool UnicodeChar::operator>=( const UnicodeChar& right ) const
{
    return !( *this < right ); 
}


//===========================================================================
// UnicodeChar::operator unsigned short() const
//===========================================================================
// Description: casts the two bytes to a word
//
// Constraints:    none
//
// Parameters:    
//
// Return:      
//
//===========================================================================
UnicodeChar::operator unsigned short() const
{
    unsigned short returnMe = 0;
    returnMe = hiByte << 8 ;
    returnMe += loByte ;
    return returnMe ;
}

//===========================================================================
// UnicodeChar::SwapEndian
//===========================================================================
// Description: swaps the endianness of the character
//
// Constraints:    none
//
// Parameters:    none
//
// Return:      none
//
//===========================================================================
void UnicodeChar::SwapEndian()
{
    unsigned char tmp = hiByte;
    hiByte = loByte;
    loByte = tmp;
}

//===========================================================================
// ostream& operator<< ( ostream& os, const UnicodeChar c )
//===========================================================================
// Description: ostream output operator for the UnicodeChar class
//
// Constraints:    none
//
// Parameters:    ostream& os - the output stream
//                UnicodeChar c - the character to be output
//
// Return:      none
//
//===========================================================================
/*ostream& operator<< ( ostream& os, const UnicodeChar& c )
{
    os.put( c.loByte ) ;
    os.put( c.hiByte ) ;
//    os << c.loByte ;    //all windows programs have the lobyte first
//    os << c.hiByte ;
    return os ;
};
*/

//===========================================================================
// istream& operator<< ( istream& is, const UnicodeChar c ) ;
//===========================================================================
// Description: istream input operator for the UnicodeChar class
//
// Constraints:    none
//
// Parameters:    istream& is - the input stream
//                UnicodeChar c - the character to be input
//
// Return:      none
//
//===========================================================================
/*
istream& operator>> ( istream& is, UnicodeChar& c ) 
{
    char c0 ;
    char c1 ;
    is.get( c0 ) ;
    is.get( c1 ) ;
    c.loByte = c0 ;
    c.hiByte = c1 ;
//    is >> c.loByte ;        //All windows programs have the lobyte first
//    is >> c.hiByte ;
    if( c.hiByte != 00 )
    {
        int a = 10 ;
        a++ ;
    }
    return is ;
};
*/
