//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radstring.hpp
//
// Subsystem:	Radical Fountation Tech - String Interface
//
// Description:	This file contains the interface to interact with a string
//				object.
//
// Revisions:	Mar 05, 2001 Creation
//
//=============================================================================

#ifndef	RADSTRING_HPP
#define RADSTRING_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radobject.hpp>
#include <radmemory.hpp>

//=============================================================================
// Forward Class Declarations
//=============================================================================

struct IRadString;

//=============================================================================
// Functions
//=============================================================================

//
// Use this function to construct a string object. 
// 
void radStringCreate( IRadString** ppIRadString, radMemoryAllocator alloc = RADMEMORY_ALLOC_DEFAULT );
IRadString * radStringCreate( IRadMemoryAllocator * pIRadMemoryAllocator );

bool radStringMatchesWildCardPattern( const char * pString, const char * pPattern );

//
// SJIS routines. Convert between ascii and sjis strings and check if strings are in 
// that format. A NULL in SJIS is byte1 == 0, byte2 undefined.
//
struct radSJISChar
{
	unsigned char byte1;
	unsigned char byte2;
};

bool radIsSjis( const radSJISChar* str );
bool radIsAscii( const char* str );
bool radSjisToAscii( char* dest, const radSJISChar* src );
bool radAsciiToSjis( radSJISChar* dest, const char* src );

unsigned int radSjisStrlen( const radSJISChar* str );
void radSjisStrcpy( radSJISChar* dest, const radSJISChar* src );
void radSjisStrncpy( radSJISChar* dest, const radSJISChar* src, unsigned int count );

//=============================================================================
// Interface Definitions
//=============================================================================

struct IRadString : public IRefCount
{
    virtual bool Equals( const char * pBuffer ) = 0;
    virtual bool Equals( IRadString * pIString ) = 0;

	virtual void Copy( const char * pBuffer ) = 0;
    virtual void Copy( const IRadString * pIString ) = 0;

	virtual void GetChars( char * pBuffer, unsigned int bufferSize ) const = 0;
    virtual const char * GetChars( void ) const = 0; // never store this pointer!

	virtual unsigned int GetLength( void ) const = 0;

    virtual void Append( IRadString * pIString ) = 0;
    virtual void Append( const char * pBuffer ) = 0;
    virtual void Append( const unsigned int x ) = 0;
    virtual void Append( const int x ) = 0;
    virtual void Append( const char ) = 0;
    virtual void Append( const void * ) = 0;
    virtual void Append( const float ) = 0;
    virtual void Append( const bool ) = 0; 

    virtual void ToLowercase( void ) = 0;
    virtual void ToUppercase( void ) = 0;

    virtual bool MatchesWildCardPattern( const char * pWildCardPattern ) = 0;
    virtual bool MatchesWildCardPattern( const IRadString * pIString_WildCardPattern ) = 0;

	virtual void SetSize( unsigned int reserved ) = 0;
	virtual unsigned int GetSize( void ) = 0;

	virtual void Compact( void ) = 0;
};

#endif

