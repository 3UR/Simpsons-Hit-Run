//===========================================================================
// Copyright (C) 1999 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   localization library
//
// Description: The definition for the excel file object
//
// Authors:     Ian Gipson
//
// Revisions        Date        Author    Revision
//                    00/06/23    IJG        brought up to coding standards
//
//===========================================================================

// Recompilation protection flag.
#ifndef XLAT_HPP
#define XLAT_HPP

#ifndef WIN32
    #define _int32 int
#endif

//===========================================================================
// Nested Includes
//===========================================================================
#include "../strings/unicodeString.h"


//===========================================================================
// Forward References
//===========================================================================

//===========================================================================
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//===========================================================================
#define XL_FILE_ID "XLAT"
#define XL_MAX_REGISTERED_DATAFILES 4
#define XL_DEFAULT_LANGUAGE XL_ENGLISH

//===========================================================================
//
// Description: A list of languages for which translations will exist.
//
// Constraints: 
//
//===========================================================================
enum XLLanguage
{
    XL_ENGLISH          = 0,
    XL_FRENCH           = 1,
    XL_GERMAN           = 2,
    XL_UKENGLISH        = 3,
    XL_SWEDISH          = 4,
    XL_FINNISH          = 5,
    XL_ITALIAN          = 6,
    XL_SPANISH          = 7,
    XL_NORWEGIAN        = 8,
    XL_JAPANESE         = 9,
    XL_LAST_LANGUAGE    = 10        // number of languages present
};

//===========================================================================
//
// Description: A list of available platforms for which the translator class 
//                may differentiate.
//
// Constraints: 
//
//===========================================================================
enum XLPlatform
{
    XL_PSX              = 0,
    XL_WINDOWS          = 1,
    XL_OVERLAYS              = 2,
    XL_LAST_PLATFORM    = 3        // Number of platforms present.
};

//===========================================================================
//
// Description: Format of an entry in a datafile.
//
// Constraints: 
//
//===========================================================================
struct XLEntry
{
    unsigned int hash;
    int offset;            // measured from the beginnning of the datafile
};



//===========================================================================
//
// Description: Single-character id's for the languages.
//
// Constraints: 
//
//===========================================================================
const char language_ID[XL_LAST_LANGUAGE]=
{
    'E',      // English
    'F',      // French
    'G',      // German
    'U',      // Uk english
    'D',      // sweDish
    'N',      // fiNnish
    'I',      // Italian
    'S',      // Spanish
    'R',      // noRwegian
    'J',      // Japanese
};

//===========================================================================
//
// Description: Format of a datafile.
//
// Constraints: Following the last entry, are all the null-terminated strings,
//                one after the other.
//
//===========================================================================
struct XLDatafile
{
    char file_id[4];                    // expect TRANSLATOR_FILE_ID characters here
    unsigned char m_Modulo[4];
    unsigned char m_TotalEntries[4];

    unsigned long GetModulo() const;
    unsigned long GetTotalEntries() const;
    void SetModulo( const unsigned long modulo );
    void SetTotalEntries( const unsigned long modulo );
//    unsigned int modulo:32;            // modulo used by this file
//    unsigned int total_entries:32;     // number of entries in this datafile
};

//===========================================================================
//
// Description: Master Translator Class - This is what should be used in the
//                platform specific code
//
// Constraints: 
//
//===========================================================================
class XLTranslator
{
    public:
        //
        // Static constructor and destructor for the singleton
        // translator object.
        //
        static void DeleteTranslator();

        //
        // Public Methods
        //        

        //walks over the data file, correcting big/little endian problem
        void FixEndianProblem();

        //get the enumeration that tells us which language this is
        XLLanguage GetLanguage();

        //gets the extension of the language file
        const char* GetLanguageFileExtention();

        //gets the letter that defines this language
        char GetLanguageID();

        //gets the letter that defines this platform
        char GetPlatformID();

        //hashes the string according to the modulo for this bible
        unsigned long Hash( const char* s );

        //is loaded checks if the bible is actually loaded or not
        bool IsLoaded() const;

        //parses through a datafile that has been loaded into a memory mapped file
        int RegisterDatafile( XLDatafile* data );

        //sets the current language for translation
        void SetLanguage( XLLanguage language );

        //gets a translation out of the bible
        UnicodeString Translation( unsigned long hash_value );

        //dumps all the memory that has been allocated
        void UnregisterDatafile( XLDatafile* data );

        // this destructor/constructor should only be called by the above member functions
        // they should be private, but that generates compiler warnings
        XLTranslator();
        ~XLTranslator();

    private:
        //
        // Unused Constructors and Operators
        //
        XLTranslator(const XLTranslator& aXLTranslator);
        XLTranslator& operator=(const XLTranslator& aXLTranslator);

         //
        // Private Attributes
        //        
        XLLanguage mLanguage;
        XLDatafile* mDatafile[ XL_MAX_REGISTERED_DATAFILES ];
        unsigned char mMost_recently_used;
        bool m_EndianSwappingDone;
};

#endif
