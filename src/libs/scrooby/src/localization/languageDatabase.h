//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   localization library
//
// Description: this contains the language database object
//
// Authors:     Ian Gipson
//
// Revisions        Date        Author    Revision
//                    00/07/17    IJG        Brought up to coding standard
//
//===========================================================================

// Recompilation protection flag.
#ifndef __RTF2LDB_DATABASE_H
#define __RTF2LDB_DATABASE_H

//===========================================================================
// Nested Includes
//===========================================================================
#pragma warning( disable : 4786 )

#include "LanguageEntry.h"
#include <map>
#include <set>
#include <stdio.h>
#include "..\strings\pcstring.h"
#include "..\strings\unicodeString.h"

//#pragma warning( disable : 4018 )                //IMPROVE: this warning should not be disabled


//===========================================================================
// Forward References
//===========================================================================

typedef std::map< PascalCString, LanguageEntry* > RidToEntryMap;
typedef std::map< PascalCString, RidToEntryMap* > ScreenToDatabaseMap;
typedef std::set< PascalCString > StringSet;

class LanguageDatabase
{
    public:
        LanguageDatabase();
        LanguageDatabase( const LanguageDatabase& right );
        LanguageDatabase& operator=( const LanguageDatabase& right );
        ~LanguageDatabase();

        //adds a translation to the database
        bool AddEntry( const PascalCString& rid, const UnicodeString& translation, const char chLanguage );

        //exports the database to file
        void Export( const char language, const _int8* mapBuffer, const _int8* stringBuffer ) const;

        //exports the database to file
        void ExportHeaderFile( FILE* fout ) const;

        //??
        bool FileLoaded( char* buffer = NULL ) const;

        //returns the filename
        PascalCString Filename() const;

        //gets all the RIDs that are valid 
        StringSet GetAllRids() const;

        //gets the character set used by this bible
        CharSet GetCharSet() const;

        //imports the database from file
        bool Import( const char* szFilename );

        //returns all the languages possible
        void Languages( char* buffer ) const;

        //determines the longest translation for a given RID
        UnicodeString LongestTranslation( const PascalCString& label ) const;

        //looks up the RID for a given language and platform
        bool LookupLabel( const UnicodeString& string, const char language, PascalCString& buffer ) const;

        //looks up the translation for a given RID
        bool LookupTranslation(    const PascalCString& label, const char language, UnicodeString& buffer ) const;

        //??    
        bool LookupTranslationForAPlatform( const PascalCString& label, const char language, const char platform, UnicodeString& buffer ) const;

        //returns the total number of strings in the bible
        int NumberOfStrings() const;

        //sets the hashing modulo
        void SetHashingModulo( const int modulo );

        //determines the byte length of all the strings
        int SizeOfAllStrings( const char language ) const;

        //populates a drop down list box with all the RIDs that are allowable for a platform
//        void PopulateRidListBox( const HWND hwndCombo, const PascalCString& screen, const char platform ) const;

    private:

        //the mapping of RID to language/translation/platform entries
        RidToEntryMap m_mapLabelToEntry;

        //maps from the name of the screen to the database (RidToEntryMap) that is valid here
        //ScreenToDatabaseMap m_screenDatabases;

        //the filename that we originally loaded
        PascalCString fileName;

        //the hashing modulo that we're using
        int hashingModulo;

        //imports the database from an LDB file
        bool ImportLdbFile( const char* szFilename );

        //imports the database from an XLS file 
        bool ImportXlsFile( const char* szFilename );
};

#endif
