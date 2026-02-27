//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Component
//
// Description: this object contains the translator resource for the runtime
//
// Authors:     Ian Gipson
//
// Revisions        Date            Author    Revision
//                    2000/07/14        IJG        Modified from wilken's version
//
//===========================================================================

#ifndef FETRANSLATERESOURCE_H
#define FETRANSLATERESOURCE_H

//===========================================================================
// Nested Includes
//===========================================================================
#include "localization/GTxlate.h"

//===========================================================================
// Forward References
//===========================================================================

//===========================================================================
// Interface Definitions
//===========================================================================
class FeResourceTextBible
:
    public FeAllocatesMemory
{
    public:
        //constructor
        FeResourceTextBible();
        FeResourceTextBible( const char* name, const char* path );
        virtual ~FeResourceTextBible();

        //hashes a string
        unsigned long Hash(const char *string);

        //triggers this object to be loaded asynchronously
        bool Load();

        //sets the current language of translation for a translator
        void SetLanguage(const char* path, XLLanguage lang);

        //changes the language
        void SetLanguage( XLLanguage lang );

        //translates a string given its hash value
        const UnicodeString Translate(unsigned long hash_value);

        //translates a string given a string which is then hashed
        const UnicodeString Translate(const char *hash_name);


    protected: 

        //???
        void LoadLanguageDatafile( const char* fileName, const int index );

        //???
        void SetLanguageDatapath(const char* path);

        //these are handles to memory buffers for various languages
        int handleToMemoryBuffer[ XL_LAST_LANGUAGE ];

        //this is an array of translator objects that contain the datafiles
        XLTranslator mTranslator[ XL_LAST_LANGUAGE ];

        //???
        char mPathName[ 255 ];      //IAN IMPROVE: hardcoded string length

        //the current language
        XLLanguage currentLanguage;

        //???
        long mModulo;
};

#endif
