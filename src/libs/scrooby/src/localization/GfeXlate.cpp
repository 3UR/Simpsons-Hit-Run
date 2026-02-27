
#include <assert.h>
#include "GfeXlate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "Radlib.h"
//#include "radmem.h"

const char* dot_str=".";

GfeTranslate::GfeTranslate() :
    languageData(NULL),
    languageFileIndex( -1 ),
#ifdef PLAT_PSX
    translator( XL_PSX )
#else
    translator( XL_WINDOWS )
#endif
{
}

GfeTranslate::~GfeTranslate()
{
    if (languageData != NULL)
    {
        translator.UnregisterDatafile((XLDatafile *)languageData);
        UnloadLanguageDatafile(languageData);
    }
}

UnicodeString GfeTranslate::Translate(unsigned long hash_value)
{

    if (hash_value == 0)
    {
        UnicodeString returnMe;
        returnMe.ReadAscii( "HASH == 0" );
        return returnMe;
    }
    return translator.Translation(hash_value);
}

void GfeTranslate::SetLanguage(const char* path, int lang)
{
    SetLanguageDatapath(path);
    SetLanguage(lang);
}

void GfeTranslate::SetLanguage(int lang) 
{
    translator.SetLanguage( static_cast< XLLanguage >( lang ) );

    // Load up the datafile.  If there already is a datafile, free it first
    if( languageData != NULL ) 
    {
        translator.UnregisterDatafile( ( XLDatafile* )languageData );
        UnloadLanguageDatafile( languageData );
    }

    const char* name = "";
    const char* ext = translator.GetLanguageFileExtention();
    char filename[255];

    //rAssertMsg(pathName != NULL, "Path to language data files is NULL");
    assert( pathName != NULL );
    sprintf( filename,"%s%s%s",pathName, name, ext );
    languageData = LoadLanguageDatafile( filename );

    // Register the new datafile
    translator.RegisterDatafile( ( XLDatafile* )languageData );
}

void GfeTranslate::SetLanguageDatapath(const char* path) 
{
    strcpy( pathName, path);
}

//===========================================================================
// void* GfeTranslate::LoadLanguageDatafile(const char* fileName)
//===========================================================================
// Description: loads a file into memory and returns a pointer to the data for parsing
//
// Constraints:    filename had better exist, and you sure as heck better delete 
//                the memory when you're done.
//
// Parameters:    filename - the filename of the file to be opened
//
// Return:      a void* to the data loaded
//
//===========================================================================
void* GfeTranslate::LoadLanguageDatafile(const char* fileName)
{
    char* pBuffer;

    FILE* myFile = fopen( fileName, "rb" );
    assert( myFile != NULL );

    fpos_t start;
    fpos_t fin;
    int success =    fgetpos( myFile, &start );        
    assert( success == 0 );
    int value = fseek( myFile, 0, SEEK_END );
    success = fgetpos( myFile, &fin );        
    assert( success == 0 );
    _int64 total = fin - start;
    assert( total > 0 );
    assert( total < 0x0FFFFFFF );        //file really should not be this big
    int numberOfBytes = static_cast< int >( total );
    rewind( myFile );
    assert( success == 0 );

    //read in the file
    pBuffer = static_cast< char* >( ScroobyGetMemory( ScroobyPermPool, numberOfBytes ) );
    assert( pBuffer != NULL );

    size_t bytesRead = fread( pBuffer, numberOfBytes, 1, myFile );
    success = fclose( myFile );                                    

    return pBuffer;
}

void GfeTranslate::UnloadLanguageDatafile( void* data )
{
    delete[] data;
}

