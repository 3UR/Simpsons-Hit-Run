
#include "languageEntry.h"

#include <algorithm>
#include <assert.h>
#include "..\files\gfefile.h"
#include <map>
#include "..\strings\pcstring.h"
#include <vector>

//-----------------------------------------------------------------------------
LanguageEntry::LanguageEntry()
:
  m_stLabel(),
  m_mapTranslations(),
  m_rgchLanguages()
{
      //allocate some memory for the vectors right off the bat
      m_rgchLanguages.reserve( 10 ) ;
}
//-----------------------------------------------------------------------------
LanguageEntry::~LanguageEntry()
{
}
//-----------------------------------------------------------------------------
LanguageEntry::LanguageEntry( const PascalCString& rid, const UnicodeString& translation, const char chLanguage )
:
  m_stLabel( rid )
{
    //allocate some memory for the vectors right off the bat
    m_rgchLanguages.reserve( 10 ) ;

    m_mapTranslations.insert( LanguageToTranslationMap::value_type( chLanguage, translation ) );
    m_rgchLanguages.push_back( chLanguage );
}
//-----------------------------------------------------------------------------
bool LanguageEntry::Add( const UnicodeString& translation, const char chLanguage )
{
    char* buffer = ScroobyGetMemory( ScroobyPermPool, translation.Length() + 1 );
    translation.MakeAscii( buffer, translation.Length() + 1 ) ;
    delete[] buffer ;

    m_mapTranslations.insert( LanguageToTranslationMap::value_type( chLanguage, translation ) );

    LanguageArray::iterator itLanguage = std::find( m_rgchLanguages.begin(), m_rgchLanguages.end(), chLanguage );

    if ( itLanguage == m_rgchLanguages.end() )
    {
        m_rgchLanguages.push_back( chLanguage );
    }
//    else        //IMPROVE: what was this here for?  why do we ever want to return false?
//    {
//        return ( false );
//    }

    return ( true );
}
//-----------------------------------------------------------------------------
void LanguageEntry::Export( GfeFile& gf )
{
    //m_stLabel.WriteFile( gf );
    char label[ 100 ] ;
    strcpy( label, m_stLabel ) ;
    gf.Write( label, strlen( label ) ) ;

    int size = m_mapTranslations.size();
    gf << size;
    
    LanguageToTranslationMap::iterator it = m_mapTranslations.begin();
    char chFirst;
    while ( it != m_mapTranslations.end() )
    {
        chFirst = (*it).first;
        gf << chFirst;
//        ((*it).second).WriteFile( gf );        //IMPROVE: this won't work anymore

        ++it;
    }

    size = m_rgchLanguages.size();
    gf << size;

    LanguageArray::iterator itLanguage = m_rgchLanguages.begin();
    while ( itLanguage != m_rgchLanguages.end() )
    {
        gf << *itLanguage;
        ++itLanguage;
    }
}
//=============================================================================
// CharSet LanguageEntry::GetCharSet()
//=============================================================================
// Description: gets the character set used by this bible entry
//
// Constraints:    NOTHING
//
// Parameters:    NOTHING
//
// Returns: CharSet a set of characters that are used by the bible entry
//
//=============================================================================
CharSet LanguageEntry::GetCharSet() const
{
    CharSet returnMe;

    //iterate through the mapping of languages to translations, and find all 
    //the characters
    LanguageToTranslationMap::const_iterator it ; 
    for( it = m_mapTranslations.begin(); it != m_mapTranslations.end(); it++ )
    {
        UnicodeString translation = (*it).second;
        int i;
        for( i = 0; i < translation.Length(); i++ )
        {
            UnicodeChar insertMe = translation[ i ];
            returnMe.insert( insertMe );
        }
    }
    

    return returnMe;
}
//-----------------------------------------------------------------------------
void LanguageEntry::Import( GfeFile& gf )
{
    //m_stLabel.ReadFile( gf );

    int size_of_map;
    gf >> size_of_map;
    
    char chLanguage;
    PascalCString readTranslation ;
    UnicodeString translation;

    for ( int i = 0; i < size_of_map; ++i )
    {
        gf >> chLanguage;
//        readTranslation.ReadFile( gf );
        translation.ReadAscii( readTranslation ) ;

        m_mapTranslations.insert( LanguageToTranslationMap::value_type( chLanguage, translation ) );
    }

    int size_of_languages;
    gf >> size_of_languages;
    m_rgchLanguages.reserve( size_of_languages );

    for ( i = 0; i < size_of_languages; ++i )
    {
        gf >> chLanguage;
        m_rgchLanguages.push_back( chLanguage );
    }
}
//-----------------------------------------------------------------------------
const PascalCString& LanguageEntry::Label() const
{
    return( m_stLabel );
}
//-----------------------------------------------------------------------------
void LanguageEntry::Languages( char* buffer )
{
    LanguageArray::iterator it ;
    int index = 0 ;
    for( it = m_rgchLanguages.begin(); it != m_rgchLanguages.end(); it++ )
    {
        assert( *it != NULL ) ;
        buffer[ index ] = *it ;
        index++ ;
    }
    buffer[ index ] = NULL ;
}
//-----------------------------------------------------------------------------
UnicodeString LanguageEntry::LongestTranslation()
{
    //iterate through the mapping of languages to translations, and find the longest one
    UnicodeString returnMe ;

    LanguageToTranslationMap::iterator it ; 
    for( it = m_mapTranslations.begin(); it != m_mapTranslations.end(); it++ )
    {
        if( (*it).second.Length()  > returnMe.Length() )
        {
            returnMe = ( *it ).second ;
        }
    }
    return returnMe ;
};
//-----------------------------------------------------------------------------
bool LanguageEntry::LookupTranslation( const char language, UnicodeString& buffer ) const
{
    LanguageToTranslationMap::const_iterator it = m_mapTranslations.find( language ) ;
    if( it != m_mapTranslations.end() )
    {
        buffer = (*it).second ;
        return true ;
    }
    else
    {
        return false ;
    }
};
//-----------------------------------------------------------------------------
