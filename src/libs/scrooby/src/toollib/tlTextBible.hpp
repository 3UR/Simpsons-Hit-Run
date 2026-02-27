//=============================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All Rights Reserved
//
// Component   :
//
// Description :
//
// Author(s)   :
//
//=============================================================================
#ifndef _TLTEXTBIBLE_HPP
#define _TLTEXTBIBLE_HPP

//=============================================================================
// Nested Includes
//=============================================================================

#include <tlLoadManager.hpp>
#include <assert.h>

//=============================================================================
// Forward References
//=============================================================================
class UnicodeString;
//=============================================================================
// Constants, Typedefs, and Macro Definitions
//=============================================================================

//=============================================================================
// Interface Definitions
//=============================================================================
class tlLanguage : public tlEntity
//=============================================================================
// Description : parent toollib textbible class
//
//=============================================================================
{
public:
    tlLanguage();
    ~tlLanguage();

    tlLanguage(tlDataChunk* ch);

    void LoadFromChunk(tlDataChunk* ch);
    tlDataChunk* Chunk();

    void LoadFromChunk16(tlDataChunk* ch)   { LoadFromChunk( ch ); }
    tlDataChunk* Chunk16()                  { return Chunk(); }

    // Accessors
    inline unsigned char GetLanguage() { return m_language; };
    unsigned int GetNumStrings();
    inline unsigned long GetModulo() { return m_modulo; };
    inline unsigned long GetBufferSize() { return m_bufferSize; };
    unsigned long GetHash( unsigned int index ) { return m_hashes[index]; };
    unsigned long GetHash( const char* name );
    unsigned long GetOffset( unsigned int index ){ return m_offsets[index]; };
    inline unsigned char* GetBuffer() { return m_buffer; };

    inline void SetLanguage( char language ) { m_language = language; };
    inline void SetModulo( unsigned long modulo ) { m_modulo = modulo; };
    void SetHash( unsigned int index, unsigned long hash );
    void SetOffset( unsigned int index, unsigned long offset );
    void SetBuffer( unsigned char* buffer, unsigned int length );

    void AddHash( unsigned long hash, unsigned long offset );

    // Utility Functions
    bool GetString( const char* name, UnicodeString& string );
    bool GetString( unsigned long name, UnicodeString& string );

protected:
    unsigned char m_language;
    unsigned long m_modulo;
    unsigned int m_bufferSize;
    tlTable<unsigned long> m_hashes;
    tlTable<unsigned long> m_offsets;
    unsigned char* m_buffer;
};

class tlLanguageLoader : public tlEntityLoader
//=============================================================================
// Description : loader for tlTextBible class
//
//=============================================================================
{
public:
    tlLanguageLoader();

    virtual tlEntity* Load(tlDataChunk* chunk);
};

class tlTextBible : public tlEntity
//=============================================================================
// Description : parent toollib textbible class
//
//=============================================================================
{
public:
    tlTextBible();
    ~tlTextBible();

    tlTextBible(tlDataChunk* ch);

    void LoadFromChunk(tlDataChunk* ch);
    tlDataChunk* Chunk();

    void LoadFromChunk16(tlDataChunk* ch)   { LoadFromChunk( ch ); }
    tlDataChunk* Chunk16()                  { return Chunk(); }

    // Accessors
    unsigned int GetNumLanguages() { return m_language.Count(); };
    inline char* GetLanguages() { return m_languages; };
    tlLanguage* GetLanguage( unsigned int index ) { return m_language[index]; };

    void SetLanguages( char* languages );
    void SetLanguage( unsigned int index, tlLanguage* language );

    void AddLanguage( tlLanguage* language );

    //Utility Functions

    bool GetString( const char* name, UnicodeString& string, int language = 0 );

protected:
    char* m_languages;
    tlTable<tlLanguage*> m_language;
};

class tlTextBibleLoader : public tlEntityLoader
//=============================================================================
// Description : loader for tlTextBible class
//
//=============================================================================
{
public:
    tlTextBibleLoader();

    virtual tlEntity* Load(tlDataChunk* chunk);
};

#endif

