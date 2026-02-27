//=============================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All Rights Reserved.
//
// Component   : Scrooby
//
// Description : toollib accessor classes for scrooby text bibles
//
// Authors     : Mike Perzel (01/28/02)
//
//=============================================================================

//=============================================================================
// Includes
//=============================================================================
#include "toollib/tlTextBible.hpp"
#include "tlDataChunk.hpp"
#include "tlScroobyProjectChunk.hpp"
#include <memory.h>
#include <string.h>
#include "unicodeString.h"

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
// tlLanguage::tlLanguage
//=============================================================================
// Description : constructor
//
// Parameters  : ch(optional) - tlDataChunk to load the language from
//
//=============================================================================
tlLanguage::tlLanguage()
:
m_language( ' ' ),
m_modulo( 0 ),
m_bufferSize( 0 ),
m_buffer( NULL )
{
}
tlLanguage::tlLanguage( tlDataChunk* ch )
:
m_language( ' ' ),
m_modulo( 0 ),
m_bufferSize( 0 ),
m_buffer( NULL )
{
    LoadFromChunk( ch );
}

//=============================================================================
// tlLanguage::~tlLanguage
//=============================================================================
// Description : destructor
//
//=============================================================================
tlLanguage::~tlLanguage()
{
    if( m_buffer )
    {
        delete[] m_buffer;
    }
}

//=============================================================================
// tlLanguage::LoadFromChunk
//=============================================================================
// Description : loads the language in from a data chunk
//
// Parameters  : ch - tlDataChunk to load the language from
//
//=============================================================================
void tlLanguage::LoadFromChunk( tlDataChunk* ch )
{
    assert(ch->ID() == Pure3D::Scrooby::LANGUAGE);
    tlScroobyLanguageChunk* sch = dynamic_cast<tlScroobyLanguageChunk*>( ch );
    SetName( sch->GetName() );
    SetLanguage( sch->GetLanguage() );
    unsigned int numStrings = sch->GetNumStrings();
    SetModulo( sch->GetModulo() );
    unsigned int bufferSize = sch->GetSizeOfBuffer();
    unsigned long* hashes = sch->GetHashes();
    unsigned long* offsets = sch->GetOffsets();
    SetBuffer( sch->GetBuffer(), bufferSize );
    unsigned int i = 0;
    for( ; i < numStrings; i++ )
    {
        AddHash( hashes[i], offsets[i] );
    }

}

//=============================================================================
// tlLanguage::Chunk()
//=============================================================================
// Description : saves out the language into a data chunk
//
// Returns     : tlDataChunk with the data from the tlLanguage object
//
//=============================================================================
tlDataChunk* tlLanguage::Chunk()
{
    tlScroobyLanguageChunk* rValue = new tlScroobyLanguageChunk();
    rValue->SetName( GetName() );
    rValue->SetLanguage( GetLanguage() );
    rValue->SetNumStrings( GetNumStrings() );
    rValue->SetModulo( GetModulo() );
    rValue->SetSizeOfBuffer( GetBufferSize() );
    rValue->SetHashes( m_hashes.Addr( 0 ), m_hashes.Count() );
    rValue->SetOffsets( m_offsets.Addr( 0 ), m_offsets.Count() );
    rValue->SetBuffer( m_buffer, m_bufferSize+1 );
    return rValue;
}

//=============================================================================
// tlLanguage::GetNumStrings()
//=============================================================================
// Returns     : number of offset/hash pairs in the chunk
//
//=============================================================================
unsigned int tlLanguage::GetNumStrings()
{
    assert( m_hashes.Count() == m_offsets.Count() );
    return m_hashes.Count();
}

//=============================================================================
// tlLanguage::GetString()
//=============================================================================
// Parameters  : name - the ID name of the string in the bible.  This will be
//                      hashed, then looked up in the bible
//               hash - the hashID of the string to look up in the bible.
//               string - a UnicodeString object to receive the data
//               language (optional) - the language to retrieve the string from
//
// Returns     : A Unicode string retrieved from the character buffer
//
//=============================================================================
bool tlLanguage::GetString( const char* name, UnicodeString& string )
{
    return GetString( GetHash( name ), string );
}
bool tlLanguage::GetString( unsigned long hash, UnicodeString& string )
{
    for( int i = 0; i < m_hashes.Count(); i++ )
    {
        if( m_hashes[i] == hash )
        {
            string.ReadUnicode( (const char*)(m_buffer+m_offsets[i]) );
            return true;
        }
    }
    string.ReadAscii( "No Translation" );
    return false;
}

//=============================================================================
// tlLanguage::GetHash()
//=============================================================================
// Parameters  : name - the ID name of the be hashed
//
// Returns     : the hash value
//
//=============================================================================
unsigned long tlLanguage::GetHash( const char* name )
{
    const char* pch = name;
    unsigned int lHash = 0L;

    while( *pch != 0 )
    {
        lHash = ( 64 * lHash + *pch++ ) % m_modulo;
    }

    return( lHash );
}



//=============================================================================
// tlLanguage::SetHash()
//=============================================================================
// Description : Changes the value of one of the hashes in the list
//
// Constraints : the index must be a valid index into the hash array
//
// Parameters  : index - the index of the hash value to be set
//               hash - the new hash value
//
//=============================================================================
void tlLanguage::SetHash( unsigned int index, unsigned long hash )
{
    assert( index > 0 && index < static_cast<unsigned int>(m_hashes.Count()) );
    m_hashes[index] = hash;
}

//=============================================================================
// tlLanguage::SetOffset()
//=============================================================================
// Description : Changes the value of one of the string offsets in the list
//
// Constraints : the index must be a valid index into the offset array
//
// Parameters  : index - the index of the offset value to be set
//               offset - number of BYTES into the buffer that the string for
//                        this index can be found
//
//=============================================================================
void tlLanguage::SetOffset( unsigned int index, unsigned long offset )
{
    assert( index > 0 && index < static_cast<unsigned int>(m_offsets.Count()) );
    m_offsets[index] = offset;
}

//=============================================================================
// tlLanguage::SetBuffer()
//=============================================================================
// Description : Sets the string buffer data object
//
// Parameters  : buffer - continuous buffer of NULL terminated unicode strings
//               length - the length of the buffer
//
//=============================================================================
void tlLanguage::SetBuffer( unsigned char* buffer, unsigned int length )
{
    if( m_buffer )
    {
        delete[] m_buffer;
    }
    m_bufferSize = length;
    m_buffer = new unsigned char[m_bufferSize+1];
    memcpy( m_buffer, buffer, length );
}

//=============================================================================
// tlLanguage::AddHash()
//=============================================================================
// Description : Add a hash/offset pair to the text bible
//
// Parameters  : hash - the hash value of the string
//               offset - number of BYTES into the buffer that the string for
//                        this index can be found
//
//=============================================================================
void tlLanguage::AddHash( unsigned long hash, unsigned long offset )
{
    m_hashes.Append( hash );
    m_offsets.Append( offset );
}


//=============================================================================
// tlLanguageLoader::tlLanguageLoader()
//=============================================================================
// Description : constructor
//
//=============================================================================
tlLanguageLoader::tlLanguageLoader() : tlEntityLoader( Pure3D::Scrooby::LANGUAGE )
{
}

//=============================================================================
// tlTextBibleLoader::tlTextBibleLoader()
//=============================================================================
// Description : Loads a tlTextBible from a data chunk
//
// Parameters  : chunk - data chunk to load the textbible from
//
// Returns     : a new tlEntity based on the data in the supplied chunk
//
//=============================================================================
tlEntity* tlLanguageLoader::Load( tlDataChunk* chunk )
{
    return new tlLanguage( chunk );
}



//=============================================================================
// tlTextBible::tlTextBible
//=============================================================================
// Description : constructor
//
// Parameters  : ch(optional) - tlDataChunk to load the text bible from
//
//=============================================================================
tlTextBible::tlTextBible()
:
m_languages( NULL )
{
}
tlTextBible::tlTextBible( tlDataChunk* ch )
:
m_languages( NULL )
{
    LoadFromChunk( ch );
}

//=============================================================================
// tlTextBible::~tlTextBible
//=============================================================================
// Description : destructor
//
//=============================================================================
tlTextBible::~tlTextBible()
{
    if( m_languages )
    {
        delete[] m_languages;
    }
}

//=============================================================================
// tlTextBible::LoadFromChunk
//=============================================================================
// Description : loads the text bible in from a data chunk
//
// Parameters  : ch - tlDataChunk to load the text bible from
//
//=============================================================================
void tlTextBible::LoadFromChunk( tlDataChunk* ch )
{
    assert( ch->ID() == Pure3D::Scrooby::TEXTBIBLE );
    tlScroobyTextBibleChunk* sch = dynamic_cast<tlScroobyTextBibleChunk*>(ch);
    SetName( sch->GetName() );
    unsigned long numLanguages = sch->GetNumLanguages();
    SetLanguages( (char*)sch->GetLanguages() );
    int i = 0;
    for( ; i < sch->SubChunkCount(); i++ )
    {
        tlDataChunk* sub = sch->GetSubChunk( i );
        switch( sub->ID() )
        {
            case Pure3D::Scrooby::LANGUAGE:
            {
                tlLanguage* language = new tlLanguage( sub );
                AddLanguage( language );
                break;
            }
            default:
                break;
        }
    }
}

//=============================================================================
// tlTextBible::Chunk()
//=============================================================================
// Description : saves out the text bible into a data chunk
//
// Returns     : tlDataChunk with the data from the tlTextBible object
//
//=============================================================================
tlDataChunk* tlTextBible::Chunk()
{
    tlScroobyTextBibleChunk* rValue = new tlScroobyTextBibleChunk();
    rValue->SetName( GetName() );
    rValue->SetNumLanguages( m_language.Count() );
    rValue->SetLanguages( GetLanguages() );
    int i = 0;
    for( ; i < m_language.Count(); i++ )
    {
        rValue->AppendSubChunk( m_language[i]->Chunk() );
    }
    return rValue;
}

//=============================================================================
// tlLanguage::GetString()
//=============================================================================
// Parameters  : name - the ID name of the string in the bible.  This will be
//                      hashed, then looked up in the bible
//               hash - the hashID of the string to look up in the bible.
//               string - a UnicodeString object to receive the data
//               language (optional) - the language to retrieve the string from
//
// Returns     : A Unicode string retrieved from the character buffer
//
//=============================================================================
bool tlTextBible::GetString( const char* name, UnicodeString& string, int language )
{
    return m_language[language]->GetString( name, string );
}

//=============================================================================
// tlTextBible::SetLanguages
//=============================================================================
// Description : loads the text bible in from a data chunk
//
// Parameters  : languages - a string, with each character representing a language
//
//=============================================================================
void tlTextBible::SetLanguages( char* languages )
{
    if( m_languages )
    {
        delete[] m_languages;
    }
    m_languages = new char[strlen( languages )+1];
    strcpy( m_languages, languages );
}

//=============================================================================
// tlTextBible::SetLanguage
//=============================================================================
// Description : replaces one of the existing languages with a new one
//
// Constraints : the index must be a valid index into the array
//
// Parameters  : index - the index into the language array to replace
//               language - the new language to be added in
//
//=============================================================================
void tlTextBible::SetLanguage( unsigned int index, tlLanguage* language )
{
    if( m_language[index] )
    {
        delete m_language[index];
    }
    m_language[index] = language;
}

//=============================================================================
// tlTextBible::AddLanguage
//=============================================================================
// Description : Adds a tlLanguage object to the text bible
//
// Parameters  : language - the new language to be added in
//
//=============================================================================
void tlTextBible::AddLanguage( tlLanguage* language )
{
    m_language.Append( language );
}



//=============================================================================
// tlTextBibleLoader::tlTextBibleLoader()
//=============================================================================
// Description : constructor
//
//=============================================================================
tlTextBibleLoader::tlTextBibleLoader() : tlEntityLoader( Pure3D::Scrooby::TEXTBIBLE )
{
}

//=============================================================================
// tlTextBibleLoader::tlTextBibleLoader()
//=============================================================================
// Description : Loads a tlTextBible from a data chunk
//
// Parameters  : chunk - data chunk to load the textbible from
//
// Returns     : a new tlEntity based on the data in the supplied chunk
//
//=============================================================================

tlEntity* tlTextBibleLoader::Load( tlDataChunk* chunk )
{
    return new tlTextBible( chunk );
}

