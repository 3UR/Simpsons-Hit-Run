
#include "languageDatabase.h"

#include <assert.h>
#include <direct.h>
#include "..\excel\excelfile.h"
#include <iostream>
#include "languageEntry.h"
#include "..\strings\pcstring.h"
#include "..\files\gfefile.h"





//-constants-------------------------------------------------------------------
const int ridColumn =        1;

//-----------------------------------------------------------------------------
LanguageDatabase::LanguageDatabase()
: 
    fileName( "" ),
    hashingModulo( 0x01479b30 )
{
}
//-----------------------------------------------------------------------------
LanguageDatabase::LanguageDatabase( const LanguageDatabase& right )
{
    //never execute the copy constructor
    assert( false );
};
//-----------------------------------------------------------------------------
LanguageDatabase& LanguageDatabase::operator=( const LanguageDatabase& right )
{
    assert( false );
    return *this;
};
//-----------------------------------------------------------------------------
LanguageDatabase::~LanguageDatabase()
{
    for( RidToEntryMap::iterator j = m_mapLabelToEntry.begin(); j != m_mapLabelToEntry.end(); j++ )
    {
        LanguageEntry* deleteMe = ( *j ).second;
        delete deleteMe;
    }
}
//-----------------------------------------------------------------------------
bool LanguageDatabase::AddEntry( const PascalCString& rid, const UnicodeString& translation, const char chLanguage )
{
    RidToEntryMap::iterator it = m_mapLabelToEntry.find( rid );

    if ( it == m_mapLabelToEntry.end() )
    {
        // The entry doesn't exist, so we must create a new one
        LanguageEntry* ple = new( ScroobyPermPool ) LanguageEntry( rid, translation, chLanguage );
        if ( ple != 0 )
        {
            m_mapLabelToEntry.insert( RidToEntryMap::value_type( rid, ple ) );
        }
        else
        {
            return ( false );
        }
    }
    else
    {
        LanguageEntry* ple = (*it).second;
        ple->Add( translation, chLanguage );
    }

    return ( true );
}


//=============================================================================
// void LanguageDatabase::Export( const char language, const _int8* mapBuffer, const _int8* stringBuffer ) const;
//=============================================================================
// Description: outputs all the strings for a given language into the two buffers required
//
// Constraints:    mapBuffer must be at least as long as the number of strings * ( 4 + 2 ) 4 for the hash value, 2 for the offset value
//                stringBuffer must be as long as totalSizeinbytes of all the characters
//
// Parameters:    language - the character indicating which language this is
//                mapBuffer - stores the hash->offset mapping table
//                stringBuffer - stores a concatenation of all the strings one after another
//
// Returns: nothing
//
//=============================================================================
void LanguageDatabase::Export( const char language, const _int8* mapBuffer, const _int8* stringBuffer ) const
{
    std::vector< PascalCString > vectorOfAllRidNames;
    std::vector< UnicodeString > vectorOfAllStrings;

    {
        const RidToEntryMap& ridToEntryMap = this->m_mapLabelToEntry;

        for( RidToEntryMap::const_iterator rit = ridToEntryMap.begin(); rit != ridToEntryMap.end(); rit++ )
        {
            PascalCString unadornedRidName = ( *rit ).first;
            LanguageEntry* languageEntry = ( *rit ).second;
            UnicodeString translation;
            bool isTranslation = languageEntry->LookupTranslation( language, translation );

            PascalCString ridName = unadornedRidName;
            //ridName.Replace( ".", "_" );        //IMPROVE: do I need to do this?

            //add the total rid name and the translation to the vectors
            vectorOfAllRidNames.push_back( ridName );
            vectorOfAllStrings.push_back( translation );
        }
    }

    int i = 0;
    //build a vector of hash values
    std::vector< _int32 > hashValues;
    for( i = 0; i < vectorOfAllRidNames.size(); i++ )
    {
        PascalCString theString = vectorOfAllRidNames[ i ];
        int hashValue = theString.Hash( this->hashingModulo );                        //IMPROVE: use the hash modulo here
        hashValues.push_back( hashValue );
    }

    //need to sort the vectors based on the hash value - this makes for easier runtime reading
    typedef std::map< _int32, int > HashMapType;
    HashMapType hashMap;
    for( i = 0; i < hashValues.size(); i++ )
    {
        HashMapType::iterator collisionIt = hashMap.find( hashValues[ i ] );
        if( collisionIt != hashMap.end() )
        {
            std::cout    << "hash collision "  
                        << hashValues[ i ] 
                        << " " 
                        << ( char* )vectorOfAllRidNames[ i ] 
                        << std::endl;
            //IMPROVE: add more intelligent info
        }
        else
        {
            HashMapType::value_type insertMe ( hashValues[ i ], i );
            hashMap.insert( insertMe );
        }
    }

    //rebuild all the output vectors of stuff
    std::vector< _int32 > outputHashes;
    std::vector< UnicodeString > outputStrings;
    HashMapType::iterator hIt = hashMap.begin(); 
    for(; hIt != hashMap.end(); hIt++ )
    {
        _int32 hashValue = ( *hIt ).first;
        int index = ( *hIt ).second;
        UnicodeString sortedString = vectorOfAllStrings[ index ];
        outputHashes.push_back( hashValue );
        outputStrings.push_back( sortedString );
    }



    //go through and build a vector of character offsets
    std::vector< _int32 > characterOffsets;
    int currentOffset = 0;        //12 is the end of the header - this should not be hardcoded though
    for( i = 0; i < outputStrings.size(); i++ )
    {
        characterOffsets.push_back( currentOffset + 12);
        UnicodeString theString = outputStrings[ i ];
        theString.MakeWide( (( unsigned short* )( stringBuffer + currentOffset )), theString.Length() + 2 );
        currentOffset += ( outputStrings[ i ].Length() + 1 ) * 2;
        if( currentOffset > 60000 )
        {
            assert( false );
        }
    }

    //go through and build up the mappingTable
    int mappingTableSize = vectorOfAllRidNames.size() * 8;
    int mappingTableIndex = 0;
    for( i = 0; i < outputHashes.size(); i++ )
    {
        _int32 hashValue = outputHashes[ i ];
        _int32 offset = characterOffsets[ i ];
        *(( _int32* )( mapBuffer + mappingTableIndex )) = hashValue;
        *(( _int32* )( mapBuffer + mappingTableIndex + 4)) = offset + mappingTableSize;
        mappingTableIndex += 8;
    }
}


//=============================================================================
// void LanguageDatabase::ExportHeaderFile( ) const;
//=============================================================================
// Description: outputs the header file so that text bible entries can be entered in code
//
// Constraints:    NOTHING
//
// Parameters:    NOTHING
//
// Returns: 
//
//=============================================================================
void LanguageDatabase::ExportHeaderFile( FILE* fout ) const
{
    //need a vector of the rid names, and another of the hash values

    std::vector< PascalCString > vectorOfAllRidNames;
    {
        const RidToEntryMap& ridToEntryMap = this->m_mapLabelToEntry;

        for( RidToEntryMap::const_iterator rit = ridToEntryMap.begin(); rit != ridToEntryMap.end(); rit++ )
        {
            PascalCString unadornedRidName = ( *rit ).first;
            LanguageEntry* languageEntry = ( *rit ).second;
            
            PascalCString ridName = unadornedRidName;
            //ridName.Replace( ".", "_" );

            //add the total rid name and the translation to the vectors
            vectorOfAllRidNames.push_back( ridName );
        }
    }
    
    int i = 0;
    //build a vector of hash values
    std::vector< _int32 > hashValues;
    for( i = 0; i < vectorOfAllRidNames.size(); i++ )
    {
        PascalCString theString = vectorOfAllRidNames[ i ];
        int hashValue = theString.Hash( this->hashingModulo );                        //IMPROVE: use the hash modulo here
        hashValues.push_back( hashValue );
    }


    //output the hash value header
    {
        fprintf(fout,"#  ifndef LANGUAGE_HASHING_MODULO\n");
        fprintf(fout,"#    define LANGUAGE_HASHING_MODULO 0x%08x // <M\n", this->hashingModulo );
        fprintf(fout,"#  else\n");
        fprintf(fout,"#    if LANGUAGE_HASHING_MODULO != 0x%08x\n", this->hashingModulo );
        fprintf(fout,"#      error HEADER CONFLICT! REBUILD *ALL* YOUR LANGUAGE DATAFILES!\n");
        fprintf(fout,"#    endif\n");
        fprintf(fout,"#  endif\n\n");
    }


    //output the hashvalues and the strings
    for( i = 0; i < vectorOfAllRidNames.size(); i++ )
    {
        _int32 hashValue = hashValues[ i ];
        fprintf(fout,"#  ifndef PHRASE_%s\n", vectorOfAllRidNames[ i ] );
        fprintf(fout,    "#    define PHRASE_%s 0x%08x // <P\n", ( char* )vectorOfAllRidNames[ i ], hashValue );
        fprintf(fout,"#  else\n");
        fprintf(fout,"#    if PHRASE_%s != 0x%08x\n", ( char* )vectorOfAllRidNames[ i ], hashValue );
        fprintf(fout,"#      error HEADER CONFLICT! REBUILD *ALL* YOUR LANGUAGE DATAFILES!\n");
        fprintf(fout,"#    endif\n");
        fprintf(fout,"#  endif\n\n");
    }
}


//-----------------------------------------------------------------------------
bool LanguageDatabase::FileLoaded( char* buffer ) const 
{
    if( buffer != NULL ) 
    {
        strcpy( buffer, fileName );
    }
    
    if( fileName == "" )
    {
        return false;
    }
    else
    {
        return true;
    }
};

//=============================================================================
// LanguageDatabase::PascalCString Filename() const
//=============================================================================
// Description: outputs the filename that the database is to be saved as
//
// Constraints:    NOTHING
//
// Parameters:    NOTHING
//
// Returns: a PascalCString repesenting the filename
//
//=============================================================================
PascalCString LanguageDatabase::Filename() const
{
    return this->fileName;
}
//-----------------------------------------------------------------------------
StringSet LanguageDatabase::GetAllRids( ) const
{
    StringSet returnMe;

    const RidToEntryMap& database = this->m_mapLabelToEntry;

    //copy the contents of this database to the set that's being returned
    RidToEntryMap::const_iterator it = database.begin();
    for(; it != database.end(); it++ )
    {
        LanguageEntry* languageEntry = ( *it ).second;
        PascalCString rid = ( *it ).first;
        returnMe.insert( rid );
    }

    return returnMe;
};

//=============================================================================
// CharSet GetCharSet() const;
//=============================================================================
// Description: gets the character set used by this bible
//
// Constraints:    NOTHING
//
// Parameters:    NOTHING
//
// Returns: CharSet a set of characters that are used by the bible
//
//=============================================================================
CharSet LanguageDatabase::GetCharSet() const
{
    CharSet returnMe;
    const RidToEntryMap& database = this->m_mapLabelToEntry;

    //copy the contents of this database to the set that's being returned
    RidToEntryMap::const_iterator it = database.begin();
    for(; it != database.end(); it++ )
    {
        LanguageEntry* languageEntry = ( *it ).second;
        CharSet insertMe = languageEntry->GetCharSet();

        CharSet::iterator it;
        for( it = insertMe.begin(); it != insertMe.end(); it++ )
        {
            UnicodeChar insertMeChar = *it;
            returnMe.insert( insertMeChar );
        }
    }

    return returnMe;
}

//-----------------------------------------------------------------------------
bool LanguageDatabase::Import( const char* szFilename )
{
    char currentPath[ 256 ] = "";
    ::_getcwd( currentPath, 256 );

    //check to see if this is an LDB file or an XLS file - check extension
    int filenameLength = strlen( szFilename );
    const char* extension = szFilename + filenameLength - 3;
    bool xlsFile = false;

    if( stricmp( extension , "XLS" ) == 0 )
    {
        xlsFile = true;
    }
    else
    {
        xlsFile = false;
    }

    bool ldbFile = false;

    if( xlsFile )
    {
        return ImportXlsFile( szFilename );
    }
    else
    {
        return ImportLdbFile( szFilename );
    }


}
//-----------------------------------------------------------------------------
bool LanguageDatabase::ImportLdbFile( const char* szFilename )
{
    //this should never be called
    assert( false );
    return false;
};
//-----------------------------------------------------------------------------
bool LanguageDatabase::ImportXlsFile( const char* szFilename )
{
    //IMPROVE: copying this code out of database.cpp is an absolute cop out

    //open the file as an XLS 
    ExcelFile excelFile;
    bool parseSuccess = excelFile.ParseFile( szFilename );
    if( parseSuccess == false )
    {
        return false;
    }

    //file should now be open, so start dealing with it

    //first check to see if some of the cells say what they should say for a text bible
    // Phrase Table,P,O,W,Space,then a list of languages
    bool headerRowOk = true;

    headerRowOk &= ( excelFile.GetCellContents( 0, 0 ) == "Languages" );
    headerRowOk &= ( excelFile.GetCellContents( 2, 1 ) == "PHRASE TABLE" );
    headerRowOk &= ( excelFile.GetCellContents( 3, 1 ) == "TERM" );

    const int headerRowLength = excelFile.RowLength( 2 );
    headerRowOk &= ( excelFile.GetCellContents( 2, headerRowLength - 1 ) == "NOTES" );
    if( headerRowOk == false )
    {
        //cout << "ERROR: RTF2LDB: Problem reading language and platform labels" << endl;
        return false;
    }

    int maxColumnLength = excelFile.MaxColumnLength();

    //get all the languages and the platforms
    UnicodeString languages = excelFile.GetCellContents( 0, 1 );
    int numLanguages = languages.Length();

    //get a mapping of column number to language (but put it in an array)
    char* cell2Language = static_cast< char* >( ScroobyGetMemory( ScroobyTempPool, headerRowLength ) );

    //go through all the header row titles looking for "E", "F", "P", etc
    for( int i = 0; i < headerRowLength; i++ )
    {
        char langCharAssignMe = -1;
        UnicodeString cellContents = excelFile.GetCellContents( 2, i );
        if( cellContents.Length() == 1 )
        {
            //only add to the mappnig if it's one of the characters in the language cell
            UnicodeChar firstChar = cellContents[ 0 ];

            //check to see if this character represents a language
            for( int j = 0; j < languages.Length(); j++ )
            {
                if( firstChar == languages[ j ] )
                {
                    langCharAssignMe = firstChar.Ascii();
                    break;
                }
            }
        }
        cell2Language[ i ] = langCharAssignMe;
    }
    //all the language and platform business should be sorted out now

    //loop down from row 5-bottom, looking at text labels, and all the translations
    int highestRowNumber = excelFile.MaxColumnLength();
    for( i = 5; i < highestRowNumber; i++ ) 
    {
        UnicodeString label =            excelFile.GetCellContents( i, ridColumn );
        PascalCString pcLabel =            PascalCString( label );


        //go through all the cells that are languages, and extract the translation
        for( int j = 0; j < headerRowLength; j++ )
        {
            if( cell2Language[ j ] != -1 )    //-1 indicates it's not a valid language
            {
                UnicodeString translation = excelFile.GetCellContents( i, j );

                char theLanguage = cell2Language[ j ];

                //go through all the platforms that this translation applies to
                {
                    char thePlatform = 'P';    //IMPROVE: hardcoded (and duplicated) character here

                    //add the phrase to the master databast of phrases
                    char theLanguage = cell2Language[ j ];
                    AddEntry( pcLabel, translation, theLanguage );
                }
            }
        }

    }

    delete[] cell2Language;
    cell2Language = NULL;
    fileName = szFilename;
    return true;
};
//-----------------------------------------------------------------------------
void LanguageDatabase::Languages( char* buffer ) const
{
    std::set< char > langSet;        //set of all the languages that exist

    {
        const RidToEntryMap& database = this->m_mapLabelToEntry;

        //iterate through a list of LanguageEntry* ,adding all the languages that we find to a set 
        RidToEntryMap::const_iterator it = database.begin();
        while ( it != database.end() )
        {
            LanguageEntry* entry = ( *it ).second;
            char languages[ 30 ] = "";        //can we ever have more than 30 languages?
            entry->Languages( languages );

            //add all the languages to the set
            for( unsigned int i = 0; i < strlen( languages ); i++ )
            {
                langSet.insert( langSet.begin(), languages[ i ] );
            }
            ++it;
        }
    }

    //put all the characters into the buffer
    int i = 0;
    std::set< char >::iterator sIt;
    for( sIt = langSet.begin(); sIt != langSet.end(); sIt++ )
    {
        buffer[ i ] = *sIt;
        i++;
    }
    buffer[ i ] = NULL;
};
//-----------------------------------------------------------------------------
UnicodeString LanguageDatabase::LongestTranslation( const PascalCString& label ) const 
{
    UnicodeString returnMe;

    //look up the database

    {
        const RidToEntryMap& database = this->m_mapLabelToEntry;

        //look up the entry associated with this label, and find the longest translation
        RidToEntryMap::const_iterator it = database.find( label );
        if( it != database.end() )
        {
            if( (*it).second->LongestTranslation().Length() > returnMe.Length() )
            {
                returnMe = (*it).second->LongestTranslation();
            }
        }
    }
    return returnMe;
};
//-----------------------------------------------------------------------------
bool LanguageDatabase::LookupLabel( const UnicodeString& string, const char language, PascalCString& buffer ) const 
{
    //find the database
    {
        const RidToEntryMap& database = this->m_mapLabelToEntry;

        //iterate through a list of LanguageEntry*, checking each for the label
        RidToEntryMap::const_iterator it = database.begin();
        while ( it != database.end() )
        {
            LanguageEntry* entry = (*it).second;
            
            //check out the phrase this maps to in this language
            UnicodeString compareAgainst;
            // if( entry->LookupTranslation( language, compareAgainst) )
            if( entry->LookupTranslation( language, compareAgainst) )
            {
                if( string == compareAgainst )
                {
                    //yay we found it
                    buffer = "";
                    buffer += entry->Label();
                    return true;
                }
            }; 
            ++it;
        }
    }
    return false;
}
//-----------------------------------------------------------------------------
bool LanguageDatabase::LookupTranslation( const PascalCString& label, const char language, UnicodeString& buffer ) const 
{
    //find the database
    {
        const RidToEntryMap& database = this->m_mapLabelToEntry;

        //look up the entry associated with this label, and find the longest translation
        RidToEntryMap::const_iterator it = database.find( label );
        if( it != database.end() )
        {
            (*it).second->LookupTranslation( language, buffer );
            return true;
        }
    }
    return false;
}
//-----------------------------------------------------------------------------
bool LanguageDatabase::LookupTranslationForAPlatform( const PascalCString& label, const char language, const char platform, UnicodeString& buffer ) const
{
    {
        const RidToEntryMap& database = this->m_mapLabelToEntry;

        //look up the entry associated with this label, and find the longest translation

        RidToEntryMap::const_iterator it = database.find( label );
        if( it != database.end() )
        {
            bool returnMe = (*it).second->LookupTranslation( language, buffer );
            return returnMe;
        }
    }
    return false;
}

//=============================================================================
// int LanguageDatabase::NumberOfStrings() const;
//=============================================================================
// Description: determines the number of strings in the bible
//
// Constraints:    NOTHING
//
// Parameters:    NOTHING
//
// Returns: total number of strings
//
//=============================================================================
int LanguageDatabase::NumberOfStrings() const
{
    int totalNumber = 0;
    const RidToEntryMap& ridToEntryMap = this->m_mapLabelToEntry;
    int number = ridToEntryMap.size();
    totalNumber += number;
    return totalNumber;
}

//=============================================================================
// void LanguageDatabase::SetHashingModulo( const int modulo )
//=============================================================================
// Description: alters the hashing modulo that's been stored
//
// Constraints:    NOTHING
//
// Parameters:    modulo - an integer that represents the new hash modulo
//
// Returns:
//
//=============================================================================
void LanguageDatabase::SetHashingModulo( const int modulo )
{
    this->hashingModulo = modulo;
}


//=============================================================================
// void LanguageDatabase::SizeOfAllStrings() const
//=============================================================================
// Description: determines the byte length of all the strings
//
// Constraints:    NOTHING
//
// Parameters:    language - the character indicating the lanugage we're concerned with
//
// Returns: byte length of all the strings put together (counts the double null at the end )
//
//=============================================================================
int LanguageDatabase::SizeOfAllStrings( const char language ) const
{
    int totalSize = 0;

    {
        const RidToEntryMap& ridToEntryMap = this->m_mapLabelToEntry;

        for( RidToEntryMap::const_iterator rit = ridToEntryMap.begin(); rit != ridToEntryMap.end(); rit++ )
        {
            LanguageEntry* languageEntry = ( *rit ).second;
            UnicodeString translation;
            bool isTranslation = languageEntry->LookupTranslation( language, translation );
            totalSize += ( translation.Length() + 1 ) * 2;
        }
    }
    return totalSize;
}
