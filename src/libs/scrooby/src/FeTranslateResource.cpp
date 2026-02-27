#include "FeApp.h"
#include "FeTranslateResource.h"
#include <raddebug.hpp>
#include "ResourceManager/FeResourceManager.h"
#include <stdio.h>
#include <string.h>

FeResourceTextBible::FeResourceTextBible()
{
    currentLanguage = FeApp::GetInstance()->GetFeResourceManager().GetLocalizationLanguage();
    strcpy( mPathName, "" );
    for( int i = 0; i < XL_LAST_LANGUAGE; i++ )
    {
        handleToMemoryBuffer[ i ] = -1;
        mPathName[ i ] = 0;
    }
    mModulo = 0;
}

//IAN IMPROVE: can these parameters be removed?
FeResourceTextBible::FeResourceTextBible( const char*, const char* path)
{
    currentLanguage = FeApp::GetInstance()->GetFeResourceManager().GetLocalizationLanguage();
    strcpy( mPathName, "" );
    mModulo = 0;
    SetLanguageDatapath( path );
    for( int i = 0; i < XL_LAST_LANGUAGE; i++ )
    {
        handleToMemoryBuffer[ i ] = -1;

        // Set the language in the translator
         this->mTranslator[ i ].SetLanguage( static_cast< XLLanguage >( i ) );

        if ( currentLanguage == i )
        {
            rAssertMsg( mPathName[ 0 ] != 0, "Path to language data files is NULL" );
            
            //load up the datafile
            const char* name = "";
            const char* ext = mTranslator[ i ].GetLanguageFileExtention();
            char filename[ 255 ];
            
            sprintf( filename,"%s%s%s", mPathName, name, ext );
            
            //trigger a load of the datafile
            LoadLanguageDatafile( filename, i );
        }
    }
}

FeResourceTextBible::~FeResourceTextBible()
{
    XLTranslator::DeleteTranslator();
}

void FeResourceTextBible::LoadLanguageDatafile( const char* filename, const int index )
{
    //tell the resource manager to load this filename into a memory buffer
    int handle = FeApp::GetInstance()->GetFeResourceManager().AddFilenameMemoryBuffer( filename );
    this->handleToMemoryBuffer[ index ] = handle;
}

//===========================================================================
// Hash
//===========================================================================
// Description: hashes the string according to the hash function of this bible
//
// Constraints:    None
//
// Parameters:    string - a char* representing an RID name
//
// Return:      the hash value
//
//===========================================================================
unsigned long FeResourceTextBible::Hash( const char* string )
{
    if( this->mTranslator[ currentLanguage ].IsLoaded() )
    {
        return this->mTranslator[ currentLanguage ].Hash( string );
    }
    else
    {
        return 0;
    }
}

//===========================================================================
// void FeResourceTextBible::Load()
//===========================================================================
// Description: triggers this object to be loaded asynchronously
//
// Constraints:    None
//
// Parameters:    NONE
//
// Return:      bool - was a load actually required?
//
//===========================================================================
bool FeResourceTextBible::Load()
{
    int handle = this->handleToMemoryBuffer[ this->currentLanguage ];

    //try to get the memory buffer
    char* buffer = FeApp::GetInstance()->GetFeResourceManager().GetResourceMemoryBuffer( handle );

    if( buffer == NULL )
    {
        //trigger a load of the memory buffer
        FeApp::GetInstance()->GetFeResourceManager().LoadMemoryBuffer( handle, FeApp::GetInstance()->GetFeResourceManager().GetCallback() );
        return false;
    }
    else
    {
        //buffer is loaded, so all loading of this resource is done - interpret the data
        this->mTranslator[ this->currentLanguage ].RegisterDatafile( reinterpret_cast< XLDatafile* >( buffer ) );
    }
    return true;
}

//===========================================================================
// Translate()
//===========================================================================
// Description: looks up a hash name and returns the appropriate unicode string
//
// Constraints:    NONE
//
// Parameters:    hash_name - the symbolic name to be looked up in the bible
//
// Return:      UnicodeString - the string that we looked up
//
//===========================================================================
const UnicodeString FeResourceTextBible::Translate( const char* hash_name )
{
    return Translate( Hash( hash_name ) );
}

//===========================================================================
// Translate()
//===========================================================================
// Description: looks up a hash name and returns the appropriate unicode string
//
// Constraints:    NONE
//
// Parameters:    hash_value - the symbolic value to be looked up in the bible
//
// Return:      UnicodeString - the string that we looked up
//
//===========================================================================
const UnicodeString FeResourceTextBible::Translate( unsigned long hash_value )
{
    if( this->mTranslator[ currentLanguage ].IsLoaded() )
    {
        //this is the actual translation
        if (hash_value == 0)
        {
            UnicodeString returnMe;
            returnMe.ReadAscii( "HASH == 0" );
            return returnMe;
        }
        //rAssert( translator != NULL );
        //rAssertMsg((translator!=NULL),"Error:  Translator not initialized!");
        UnicodeString returnMe = this->mTranslator[ currentLanguage ].Translation(hash_value);
        return returnMe;
    }
    else
    {
        this->Load();

        //this path means that the bible is not yet loaded
        UnicodeString returnMe;
        returnMe.ReadAscii( "BIBLE NOT LOADED" );
        return returnMe;
    }
}


void FeResourceTextBible::SetLanguageDatapath( const char* path )
{
    strcpy( mPathName, path );
}


void FeResourceTextBible::SetLanguage( XLLanguage lang )
{
    // MIKE IMPROVE : This will load a new language file without unloading the previous ones
    if( this->currentLanguage != lang )
    {
        this->currentLanguage = lang;
        
        // Set the language in the translator
        this->mTranslator[ 0 ].SetLanguage( static_cast< XLLanguage >( lang ) );
        
        rAssertMsg( mPathName[ 0 ] != 0, "Path to language data files is NULL" );
        
        //load up the datafile
        const char* name = "";
        const char* ext = mTranslator[ 0 ].GetLanguageFileExtention();
        char filename[255];
        
        sprintf( filename,"%s%s%s",mPathName, name, ext );
        
        //trigger a load of the datafile
        LoadLanguageDatafile( filename, lang );
        Load();
    }
}


void FeResourceTextBible::SetLanguage(const char* path, XLLanguage lang)
{
    SetLanguageDatapath( path );
    SetLanguage( lang );
}
