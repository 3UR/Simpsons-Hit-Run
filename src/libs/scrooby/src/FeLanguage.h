//=============================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All Rights Reserved
//
// Component   : Scrooby
//
// Description : A class which represents a listing of strings in a particular
//               language.  It is for use with the FeTextBible object
//
// Author(s)   : Mike Perzel (01/29/02)
//
//=============================================================================

// Recompillation Protection
#ifndef FE_LANGUAGE_H
#define FE_LANGUAGE_H

//=============================================================================
// Nested Includes
//=============================================================================
#include "FeEntity.h"
#include "FeLoaders.h"
//=============================================================================
// Forward References
//=============================================================================

//=============================================================================
// Constants, Typedefs, and Macro Definitions
//=============================================================================

//=============================================================================
// Interface Definitions
//=============================================================================

class FeLanguage : public FeEntity
//=============================================================================
// Description : 
//
// Constraints : 
//
//=============================================================================
{
public:
    FeLanguage( const char* name = NULL );
    
    // Accessors
    char GetLanguageID();
    unsigned int GetModulo();
    unsigned int GetNumStrings();
    unsigned short* GetString( unsigned int hash );
    unsigned short* GetString( const char* name );
    unsigned int GetHash( const char* name );

    void SetLanguageID( const char ID );
    void SetModulo( unsigned int modulo );

    virtual bool IsDrawable() { return false; }

#ifdef RAD_DEBUG
    void Dump();
#endif

protected:
    ~FeLanguage();

    char m_languageID;

    unsigned int m_modulo;
    unsigned int m_bufferSize;
    unsigned int m_numStrings;

    unsigned int* m_hashes;
    unsigned int* m_offsets;
    unsigned short* m_buffer;
private:
    friend class FeTextBibleLoader;
};

//=============================================================================
// Inlines
//=============================================================================

//=============================================================================
// FeLanguage::GetLanguageID
//=============================================================================
// Return      : The single letter identifier of the language
//
//=============================================================================
inline char FeLanguage::GetLanguageID()
{
    return m_languageID;
}

//=============================================================================
// FeLanguage::GetModulo
//=============================================================================
// Return      : the hashing modulo used to turn strings into indexes
//
//=============================================================================
inline unsigned int FeLanguage::GetModulo()
{
    return m_modulo;
}
//=============================================================================
// FeLanguage::GetNumStrings
//=============================================================================
// Return      : the number of text strings in the language
//
//=============================================================================
inline unsigned int FeLanguage::GetNumStrings()
{
    return m_numStrings;
}
//=============================================================================
// FeLanguage::SetLanguageID
//=============================================================================
// Description : replaces the ID letter of the language
//
// Parameters  : ID - the new language ID
//
//=============================================================================
inline void FeLanguage::SetLanguageID( const char ID )
{
    m_languageID = ID;
}
//=============================================================================
// FeLanguage::SetModulo
//=============================================================================
// Description : replaces the current hashing modulo used to turn strings to ids
//
// Parameters  : modulo - the new modulo
//
//=============================================================================
inline void FeLanguage::SetModulo( unsigned int modulo )
{
    m_modulo = modulo;
}

// end conditional define
#endif