//=============================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All Rights Reserved
//
// Component   : Scrooby
//
// Description : Classes to use text bibles, which are lists of strings which
//               are referenced by a hash value combined with a language, and 
//               potentially other options, like platform.
//
// Author(s)   : Mike Perzel
//
//=============================================================================

// Recompillation Protection
#ifndef FE_TEXT_BIBLE_H
#define FE_TEXT_BIBLE_H

//=============================================================================
// Nested Includes
//=============================================================================
#include <textbible.h>
#include "FeEntity.h"
#include "FeLoaders.h"
#include "utility/rVector.hpp"
#include "strings/unicodestring.h"

//=============================================================================
// Forward References
//=============================================================================
class FeLanguage;
//=============================================================================
// Constants, Typedefs, and Macro Definitions
//=============================================================================

//=============================================================================
// Interface Definitions
//=============================================================================

class FeTextBible : public FeEntity,
                    virtual public Scrooby::TextBible
//=============================================================================
// Description : A class to encapsulate a list of strings referenced by a hash
//               value.
//
// Constraints : Should only be used internally
//=============================================================================
{
public:
    FeTextBible();

    unsigned int HashStringID( const char* key );

    UnicodeString GetString( unsigned int hash );
    UnicodeString GetString( const char* key );

    UnicodeChar* GetWChar( unsigned int hash );
    UnicodeChar* GetWChar( const char* key );

    void SetLanguage( char language );
    void SetLanguage( unsigned int index );

    virtual bool IsDrawable() { return false; }

    static unsigned int GetLanguageIndex();
    static char GetCurrentLanguageID();

protected:
    // Protect this so they have to use the Release call
    virtual ~FeTextBible();

    static const int MAX_NUM_LANGUAGES = 16;
    static char m_languageIDs[ MAX_NUM_LANGUAGES ];
    static unsigned int m_index;
    rVector<FeLanguage*> m_languages;
    UnicodeString* m_tempString;

//=============================================================================
// Data Members
//=============================================================================

private:

    friend class FeTextBibleLoader;
};

inline unsigned int FeTextBible::GetLanguageIndex()
{
    return m_index;
}

inline char FeTextBible::GetCurrentLanguageID()
{
    if( m_languageIDs != NULL )
    {
        return m_languageIDs[ m_index ];
    }

    return '\0';
}

#endif
