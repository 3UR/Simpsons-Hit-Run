//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radKey.hpp
//
// Subsystem:   Foundation Technologies - Radcore
//
// Description: Function(s) for hashing string names into numbers.
//
//=============================================================================

#ifndef RADKEY_HPP
#define RADKEY_HPP

#include <stdlib.h>
#include <raddebug.hpp>
#include <main/tuidunaligned.h>

//=============================================================================
// Typedefs
//=============================================================================

//
// This is a key
//
typedef tUidUnaligned radKey;

//=============================================================================
// Functions
//=============================================================================

//=============================================================================
// Function:    radMakeKey
//=============================================================================
// Description: Takes a string and returns a key value corresponding to
//              it. It is a running hash, i.e. finding the hash of the 
//              part of the string, then passing that value in and hashing
//              the rest will yield the same result as hashing the whole
//              string at once.
//
// Parameters:  pToken - a NULL terminated string
//
// Returns:     A key corresponding to the given string
//
// Notes:       Although the current algorithm does not guarentee that
//              every string will return a unique hash value, it does
//              provide odds of approximately one in four billion.
//------------------------------------------------------------------------------

inline radKey radMakeKey( const char* pToken, radKey keyValue = 0 )
{
	if ( pToken != NULL )
	{
		while( *pToken )
		{
            // Pure3D Algorithm
            keyValue *= 65599;
            keyValue = (keyValue ^ ( static_cast< radKey >( *pToken ) ) );
            
			//
			// Update the token's position
			//
			pToken++;
		}
    
		return( keyValue );
	}

	return keyValue;
}

//=============================================================================
// Function:    radMakeCaseInsensitiveKey
//=============================================================================
// Description: Takes a string and returns a key value corresponding to it.
//              The key value is independant of the given string's case.
//
// Parameters:  pToken - a NULL terminated string
//
// Returns:     A key corresponding to the given string, ignoring
//              the string's case.
//
//------------------------------------------------------------------------------

inline radKey radMakeCaseInsensitiveKey( const char* pToken, radKey keyValue = 0 )
{
    while( *pToken )
    {
        // Case insensitive Pure3d Algorithm
        keyValue *= 65599;
        keyValue = (keyValue ^ static_cast< radKey >(*pToken < 'a' ? *pToken + 'a' - 'A' : *pToken));

        //
        // Update the token's position
        //
        pToken++;
    }
    
    return( keyValue );
}


//
// This is a 32 bit key
//
typedef unsigned int radKey32;

//=============================================================================
// Function:    radMakeKey32
//=============================================================================
// Description: Takes a string and returns a key value corresponding to
//              it. It is a running hash, i.e. finding the hash of the 
//              part of the string, then passing that value in and hashing
//              the rest will yield the same result as hashing the whole
//              string at once.
//
// Parameters:  pToken - a NULL terminated string
//
// Returns:     A key corresponding to the given string
//
// Notes:       Although the current algorithm does not guarentee that
//              every string will return a unique hash value, it does
//              provide odds of approximately one in four billion.
//------------------------------------------------------------------------------

inline radKey32 radMakeKey32( const char* pToken, radKey32 keyValue = 0 )
{
	if ( pToken != NULL )
	{
		while( *pToken )
		{
            
			//
			// X31 Alogrithm
			//
			keyValue = ( keyValue << 5 ) - keyValue + (radKey32) *pToken;
    
			//
			// Radcore algorithm
			//
			//keyValue = keyValue + (keyValue >> 24);
			//keyValue = (keyValue << 7) + *pToken;
        
			//
			// Update the token's position
			//
			pToken++;
		}
    
		return( keyValue );
	}

	return keyValue;
}

//=============================================================================
// Function:    radMakeCaseInsensitiveKey32
//=============================================================================
// Description: Takes a string and returns a key value corresponding to it.
//              The key value is independant of the given string's case.
//
// Parameters:  pToken - a NULL terminated string
//
// Returns:     A key corresponding to the given string, ignoring
//              the string's case.
//
//------------------------------------------------------------------------------

inline radKey32 radMakeCaseInsensitiveKey32( const char* pToken, radKey32 keyValue = 0 )
{
    radKey32 c;

    while( *pToken )
    {
        //
        // Case insensitive X31 alogrithm
        //
        c = static_cast< radKey32 >
        (
            ( *pToken < 'a' ) ? ( *pToken + 'a' - 'A' ) : ( *pToken )
        );
        keyValue = ( keyValue << 5 ) - keyValue + c;
        
        //
        // Update the token's position
        //
        pToken++;
    }
    
    return( keyValue );
}

//=============================================================================
// Function:    radBinarySearch
//=============================================================================
// Description: Do a binary search through a sorted array for an element.
//
// Parameters:  matchElement - an element to find (by == operator)
//              pSortedArray - a pointer to a sorted array to search
//              numElements - the number of elements in the sorted array
//              pIndex - (out) the index of the matching element if found
//              
// Returns:     Returns true if a match is found, otherwise false.
//
// Notes:       The template uses the == and < operators on class A
//------------------------------------------------------------------------------

template< class A >
bool radBinarySearch
(
    const A& matchElement,
    const A* pSortedArray,
    const int numElements,
    unsigned int* pIndex
)
{
    //
    // Iterate through cache file names using a binary search
    //
    bool matchfound = false;
    unsigned int low = 0;
    unsigned int mid;
    unsigned int high = numElements;

    while( low <= high )
    {
        //
        // Calculate the mid point.  The actual function is:
        //      mid = ( low + high ) / 2;
        // We spice it up a bit to avoid overflow problems and
        // to confuse anybody who may be reading it ;)
        //
        mid = ( low / 2 ) + ( high / 2 );
        if( low&0x01 && high&0x01 )
        {
            mid++;
        }
    
        //
        // Do they match?
        //
        if( pSortedArray[ mid ] == matchElement )
        {
            matchfound = true;
            (*pIndex) = mid;
            break;
        }

        //
        // Update the search
        //
        if( pSortedArray[ mid ] < matchElement )
        {
            low = mid + 1;
        }
        else
        {
            //
            // We don't want to "underflow" unsigned int
            //
            if( mid == 0 )
            {
                break;
            }
            high = mid - 1;
        }
    }

    return matchfound;
}

//=============================================================================
// Statics needed for table driven CRC check
//=============================================================================

#define RADCRCPOLYNOMIAL 0x04c11db7L
#define RADCRCTABLESIZE  256

typedef unsigned int radCrc;

extern radCrc s_CRCTable[ RADCRCTABLESIZE ];
extern bool   s_CRCIsInitialized;

//=============================================================================
// Function:    radCRCInit
//=============================================================================
// Description: Generate the table of CRC remainders for all possible bytes
//
// Parameters:  
//              
// Returns:     
//
//------------------------------------------------------------------------------

inline void radCRCInit( void )
{
    if ( !s_CRCIsInitialized )
    {
        rAssertMsg( sizeof( radCrc ) == 4, "radKey: radCrc must be 32 bit integers." );

        unsigned int i, j;
        radCrc CRCRem;

        for ( i = 0; i < RADCRCTABLESIZE; i++ ) 
        {
            CRCRem = ( (radCrc) i << 24 );

            for ( j = 0; j < 8; j++ ) 
            {
                if ( CRCRem & 0x80000000L )
                {
                    CRCRem = ( CRCRem << 1 ) ^ RADCRCPOLYNOMIAL;
                }
                else
                {
                    CRCRem = ( CRCRem << 1 );
                }
            }

            s_CRCTable[ i ] = CRCRem;
        }

        s_CRCIsInitialized = true;
    }
}

//=============================================================================
// Function:    radCRCCalculate
//=============================================================================
// Description: Calculate a running CRC32 hash of a byte stream.
//
// Parameters:  buf -- the buffer to hash
//              buf_size -- the size the of the buffer
//              CRC -- the current CRC hash value
//              
// Returns:     new CRC value
//
//------------------------------------------------------------------------------

inline radCrc radCRCCalculate( const char* buf, unsigned int buf_size, radCrc CRC = 0xffffffff )
{
    rAssertMsg( s_CRCIsInitialized, "radKey: CRC table not initialized." );

    int i;
    unsigned int j;

    for ( j = 0; j < buf_size; j++ ) 
    {
        i = ((int) (CRC >> 24) ^ buf[ j ]) & 0xff;
        CRC = (CRC << 8) ^ s_CRCTable[ i ];
    }
    return CRC;
}

void KeyToStringKey32( char * pBuf, radKey32 key );
radKey32 StringKeyToKey32( char * pBuf );


#endif //RADKEY_HPP

