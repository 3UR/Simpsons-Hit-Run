#ifndef __RTF2LDB_ENTRY_H
#define __RTF2LDB_ENTRY_H

//#include "stdgfe.h"
#pragma warning( disable : 4786 )

#include "..\strings\pcstring.h"
#include "..\strings\unicodeString.h"
#include <map>
#include <set>
#include <vector>

typedef std::map< char, UnicodeString > LanguageToTranslationMap;
typedef std::vector< char > LanguageArray;
typedef std::set< UnicodeChar > CharSet;
//typedef std::vector< char > PlatformArray;

class GfeFile;

class LanguageEntry
{
    public:
        LanguageEntry();
        LanguageEntry( const PascalCString& rid, const UnicodeString& translation, const char chLanguage );
        ~LanguageEntry();

        bool Add( const UnicodeString& translation, const char chLanguage );
        void Export( GfeFile& gf );
        CharSet GetCharSet() const;
        void Import( GfeFile& gf );

        const PascalCString& Label() const;
        void Languages( char* buffer ) ;
        UnicodeString LongestTranslation();
        bool LookupTranslation( const char language, UnicodeString& buffer ) const;

    private:

        PascalCString                m_stLabel;
        LanguageToTranslationMap    m_mapTranslations;
        LanguageArray                m_rgchLanguages;
};

#endif
