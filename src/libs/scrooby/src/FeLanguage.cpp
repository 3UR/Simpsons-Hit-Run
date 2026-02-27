//=============================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All Rights Reserved.
//
// Component   : Scrooby
//
// Description : A container for a list of strings in a particular language
//
// Authors     : Mike Perzel (01/29/02)
//
//=============================================================================

//=============================================================================
// Includes
//=============================================================================
#include "FeLanguage.h"
#include "utility/debugmessages.h"
#include <string.h>

//=============================================================================
// Local constants, typedefs, macros
//=============================================================================

//=============================================================================
// Global data, local data, local classes
//=============================================================================

//=============================================================================
// Member functions
//=============================================================================

//=============================================================================
// thisclass::memberfunction
//=============================================================================
// Description :
//
// Constraints :
//
// Parameters  :
//
// Return      :
//
//=============================================================================
FeLanguage::FeLanguage( const char* name )
:
m_languageID( ' ' ),
m_modulo( 0 ),
m_bufferSize( 0 ),
m_numStrings( 0 ),
m_hashes( NULL ),
m_offsets( NULL ),
m_buffer( NULL )
{
    if( name )
    {
        SetName( name );
    }
}

FeLanguage::~FeLanguage()
{
    if( m_hashes )
    {
        delete[] m_hashes;
    }

    if( m_offsets )
    {
        delete[] m_offsets;
    }

    if( m_buffer )
    {
        delete[] m_buffer;
    }
}

unsigned short* FeLanguage::GetString( const char* name )
{
    return GetString( GetHash( name ) );
}

unsigned short* FeLanguage::GetString( unsigned int hash )
{
    //MIKE IMPROVE : make this hashing actually do something useful, instead of a linear sort
    unsigned int i = 0;
    for( ; i < m_numStrings; i++ )
    {
        if( m_hashes[i] == hash )
        {
            return m_buffer+(m_offsets[i]/2);
        }
    }
    return NULL;
}

unsigned int FeLanguage::GetHash( const char* name )
{
    const char* pch = name;
    unsigned int lHash = 0L;

    while( *pch != 0 )
    {
        lHash = ( 64 * lHash + *pch++ ) % m_modulo;
    }

    return( lHash );
}

#ifdef RAD_DEBUG
void FeLanguage::Dump()
{
    unsigned int i = 0;
    for( ; i < m_numStrings; i++ )
    {
        Scrooby::Log::Message( Scrooby::LVL_DEBUG, "Hash %ud (off%ud)", m_hashes[i], m_offsets[i] );
    }
}
#endif