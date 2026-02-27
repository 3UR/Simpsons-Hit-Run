//===========================================================================
// Copyright (C) 1999 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Localization Library
//
// Description: 
//
// Authors:     Tom Legal, Ian Gipson    
//
// Revisions        Date        Author    Revision
//                    98/06/15    TOM        Created
//                    00/06/23    IJG        Broubht up to coding standards
//
//===========================================================================

//---------------------------------------------------------------------------
// Includes
//===========================================================================

#include <assert.h>
#include "GTxlate.h"
#include <raddebug.hpp>
#include <stdlib.h>
#include <string.h>


//===========================================================================
// Local Constants, Typedefs, and Macros
//===========================================================================

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================

//===========================================================================
// Member Functions
//===========================================================================

//===========================================================================
// XLDatafile::GetModulo
//===========================================================================
// Description: gets the modulo out of the XLDatafile structure
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
unsigned long XLDatafile::GetModulo() const
{
    unsigned long returnMe = 0;
    returnMe += m_Modulo[ 3 ];
    returnMe <<= 8;
    returnMe += m_Modulo[ 2 ];
    returnMe <<= 8;
    returnMe += m_Modulo[ 1 ];
    returnMe <<= 8;
    returnMe += m_Modulo[ 0 ];
    return returnMe;
}

//===========================================================================
// XLDatafile::GetTotalEntries
//===========================================================================
// Description: gets the total number of entries out of the structure
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
unsigned long XLDatafile::GetTotalEntries() const
{
    unsigned long returnMe = 0;
    returnMe += m_TotalEntries[ 3 ];
    returnMe <<= 8;
    returnMe += m_TotalEntries[ 2 ];
    returnMe <<= 8;
    returnMe += m_TotalEntries[ 1 ];
    returnMe <<= 8;
    returnMe += m_TotalEntries[ 0 ];
    return returnMe;
}

//===========================================================================
// XLDatafile::SetModulo
//===========================================================================
// Description: constructor
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
void XLDatafile::SetModulo( const unsigned long modulo )
{
    unsigned long totalEntries = modulo;
    m_Modulo[ 0 ] = static_cast< unsigned char >( totalEntries & 0x000000ff );
    totalEntries >>= 8;
    m_Modulo[ 1 ] = static_cast< unsigned char >( totalEntries & 0x000000ff );
    totalEntries >>= 8;
    m_Modulo[ 2 ] = static_cast< unsigned char >( totalEntries & 0x000000ff );
    totalEntries >>= 8;
    m_Modulo[ 3 ] = static_cast< unsigned char >( totalEntries & 0x000000ff );
}

//===========================================================================
// XLDatafile::SetTotalEntries
//===========================================================================
// Description: constructor
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
void XLDatafile::SetTotalEntries( const unsigned long total )
{
    unsigned long totalEntries = total;
    m_TotalEntries[ 0 ] = static_cast< unsigned char >( totalEntries & 0x000000ff );;
    totalEntries >>= 8;
    m_TotalEntries[ 1 ] = static_cast< unsigned char >( totalEntries & 0x000000ff );;
    totalEntries >>= 8;
    m_TotalEntries[ 2 ] = static_cast< unsigned char >( totalEntries & 0x000000ff );;
    totalEntries >>= 8;
    m_TotalEntries[ 3 ] = static_cast< unsigned char >( totalEntries & 0x000000ff );;
}


//===========================================================================
// void XLTranslator::DeleteTranslator()
//===========================================================================
// Description: constructor
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
void XLTranslator::DeleteTranslator()
{
}


//*******************************************************************
// Public Methods
//*******************************************************************
//===================================================================
//  GetLanguage
//===================================================================
XLLanguage XLTranslator::GetLanguage()
{
    return mLanguage;
}

//===========================================================================
// XLTranslator::Hash
//===========================================================================
// Description: hasing function for looking strings up by RID
//
// Constraints:    NONE
//
// Parameters:    s - ASCII string to be hashed
//
// Return:      unsigned long - hash valuse
//
//===========================================================================
unsigned long XLTranslator::Hash( const char* s )
{
    const char* pch = s;
    unsigned long lHash = 0L;

    int datafileSize = sizeof( *mDatafile[ mMost_recently_used ] );
//    int offset0 = offsetof( XLDatafile, modulo );
//    int offset1 = offsetof( XLDatafile, total_entries );

    rAssert( mDatafile[ mMost_recently_used ] != NULL );
    unsigned long modulo = mDatafile[ mMost_recently_used ]->GetModulo();

    for( ; *pch != 0; ++pch )
    {
        lHash = ( 64 * lHash + *pch ) % modulo;
    }

    return( lHash );
}

//===========================================================================
// XLTranslator::IsLoaded()
//===========================================================================
// Description: determines if the bible data file has been loaded or not
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      true or false - bible has been loaded
//
//===========================================================================
bool XLTranslator::IsLoaded() const
{
    if( this->mDatafile[ this->mMost_recently_used ] == NULL )
    {
        return false;
    }
    else
    {
        return true;
    }
}

//===================================================================
//  GetLanguageFileExtention
//   - Get the extention for a language file to load,
//     based on currently selected platform and language.
//     (eg. returns ".LPE" for Psx, English)
//===================================================================
const char* XLTranslator::GetLanguageFileExtention()
{
    //
    // Create a static file extention.
    //
    static char extention[] = "x";

    //
    // Set the platform and language characters.
    //
//    int pIndex = static_cast< int >( mPlatform );
    extention[ 0 ] = language_ID[ mLanguage ];
    return extention;
}

//===================================================================
//  GetLanguageID
//   - Returns the single-character ID of the currently active
//     language.
//===================================================================
char XLTranslator::GetLanguageID()
{
    return language_ID[mLanguage];
}


//===================================================================
//  GetPlatformID
//   - Returns the single-character ID of the currently active
//     platform.
//===================================================================
/*
char XLTranslator::GetPlatformID()
{
    return platform_ID[mPlatform];
}
*/
    
//===================================================================
//  RegisterDatafile
//   - Tell translator to check this data for translations.
//===================================================================
int XLTranslator::RegisterDatafile(XLDatafile* data)
{
    //rAssertMsg(data,"*** XLTranslator::RegisterDatafile - data is NULL!!!");
    rAssert( data != NULL );

    if ( data != NULL )
    {
        for( int n=0; n < 1; n++ )
        {
//            if( mDatafile[ n ] == 0 )
            {
                //
                // Found a free location, place it here...
                // Is this datafile looking much like a translator datafile?
                //
                int ok = 1;
                for(int t=0; t<(int)strlen(XL_FILE_ID); t++)
                {
                    if(data->file_id[t]!=XL_FILE_ID[t])
                    {
                        ok = 0;
                    }
                }
                if (ok)
                {
                    //
                    // All's well, remember this datafile
                    //
                    mDatafile[n] = data;
                }
                else
                {
                    rAssert( false );    //IMPROVE: bad thing to assert
//                    rAssertMsg(0,"*** TRANSLATOR::RegisterDatafile - That aint-no datafile to me!!");
                }
                this->FixEndianProblem();
                return n;
            }
        }
        rAssert( false ) ;
        //rAssertMsg(0,"*** TRANSLATOR::RegisterDatafile - too many datafiles, raise #define MAX_REGISTERED_DATAFILES in XLATE.HPP");
    }
    return -1 ;
}


//===================================================================
//  SetLanguage
//   - Choose a language to translate to.
//===================================================================
void XLTranslator::SetLanguage(XLLanguage language)
{
    mLanguage = language;
}


//===================================================================
//  Translation
//   - Get a translation.
//===================================================================
UnicodeString XLTranslator::Translation(unsigned long hash_value)
{
    //
    // We're going to travel a loop through the registered datafiles
    // looking for the translation. Remember where we started!
    // This is done because translations tend to come from the same
    // datafile in groups, so this minimizes look-up time, assuming
    // you aren't randomly jumping between datafiles.
    //
    int first_checked = mMost_recently_used;

    //
    // Look-up a translation.
    //
    do
    {
        //
        // Is a datafile loaded in this slot?
        //
        if (mDatafile[mMost_recently_used] != NULL)
        {
            //
            // Calculate the address of the first entry.
            //
            XLEntry* entry_array = (XLEntry*)(&(mDatafile[mMost_recently_used])[1]);

            //ian's debugging check of all the HASH values
//            int total = mDatafile[ mMost_recently_used ]->total_entries - 1 ;
//            for( int i = 0; i < total ; i++ )
//            {
//                XLEntry* entry = &(entry_array[ i ] );
//            }

            //this used to be a linear search of entry_array[ n ].hash but that's really inefficient
            //so instead here's a homegrown binary search! YAY!
            unsigned int lo = 0 ;
            unsigned int hi = mDatafile[ mMost_recently_used ]->GetTotalEntries() - 1 ;
            unsigned int pivot = ( hi + lo ) / 2 ;
            bool found = false ;
            int correctNvalue = -1 ;

            //check the hi end
            if( entry_array[ hi ].hash == hash_value )
            {
                found = true ;
                correctNvalue = hi ;
            }
            //check the lo end
            else if( entry_array[ lo ].hash == hash_value )
            {
                found = true ;
                correctNvalue = lo ;
            }

            while( ( found == false ) && ( ( hi - lo ) > 1 ) )
            {
                //need to do the binary search
                unsigned int hashValueToCheck = entry_array[ pivot ].hash ;

                //is the pivot too high?
                if( hashValueToCheck > hash_value )
                {
                    hi = pivot ;
                }

                //is the pivot too low?
                if( hashValueToCheck < hash_value )
                {
                    lo = pivot ;
                }

                //did we find it?
                if( hashValueToCheck == hash_value )
                {
                    found = true ;
                    correctNvalue = pivot ;
                }
                pivot = ( hi + lo ) / 2 ;
            }


            if( found )        //we found a match
            {
                char* stringPointer = &( ( char* )mDatafile[ mMost_recently_used ] )[ entry_array[ correctNvalue ].offset ] ;
                UnicodeString returnMe ;
                returnMe.ReadUnicode( stringPointer ) ;
#ifdef __BIG_ENDIAN__
                returnMe.SwapEndian();
#endif
                return  returnMe ;
            }
        }
        mMost_recently_used = (mMost_recently_used+1) % XL_MAX_REGISTERED_DATAFILES;
    }
    while (mMost_recently_used != first_checked);

    //
    // No luck looking up the translation.
    //
    //rAssertMsg(0,"*** XLTranslator::Translation - couldn't find a translation");
    UnicodeString returnMe ;
    returnMe.ReadAscii( "-NO TRANS-" ) ;
    return returnMe ;
}


//===================================================================
//  UnregisterDatafile
//   - Tell translator not to check here for translations anymore.
//===================================================================
void XLTranslator::UnregisterDatafile(XLDatafile* data)
{
    rAssert( data != NULL ) ;
    //rAssertMsg((data!=NULL),"*** XLTranslator::UnregisterDatafile - data is  NULL!!!");

    if (data != NULL)
    {
        for (int n=0; n<XL_MAX_REGISTERED_DATAFILES; n++)
        {
            if (mDatafile[n] == data)
            {
                mDatafile[n] = 0;
                return;
            }
        }
        rAssert( false ) ;
//        rAssertMsg(0,"*** XLTranslator::UnregisterDatafile - couldn't find datafile to unregister!!!");
    }
}

void SwapEndian( unsigned long int& theInt )
{
#ifdef __BIG_ENDIAN__
    unsigned char b0 = ( theInt & 0x000000FF ) >> 0 ;
    unsigned char b1 = ( theInt & 0x0000FF00 ) >> 8 ;
    unsigned char b2 = ( theInt & 0x00FF0000 ) >> 16 ;
    unsigned char b3 = ( theInt & 0xFF000000 ) >> 24 ;

    unsigned long value;
    value = 0;
    value |= b0;
    value <<= 8;
    value |= b1;
    value <<= 8;
    value |= b2;
    value <<= 8;
    value |= b3;
    theInt = value;
#endif
}

/*void SwapEndian( unsigned _int32& theInt )
{
#ifdef __BIG_ENDIAN__
    unsigned char b0 = ( theInt & 0x000000FF ) >> 0 ;
    unsigned char b1 = ( theInt & 0x0000FF00 ) >> 8 ;
    unsigned char b2 = ( theInt & 0x00FF0000 ) >> 16 ;
    unsigned char b3 = ( theInt & 0xFF000000 ) >> 24 ;

    unsigned _int32 value;
    value = 0;
    value |= b0;
    value <<= 8;
    value |= b1;
    value <<= 8;
    value |= b2;
    value <<= 8;
    value |= b3;
    theInt = value;
#endif
}*/

void SwapEndian( unsigned int& theInt )
{
#ifdef __BIG_ENDIAN__
    unsigned char b0 = ( theInt & 0x000000FF ) >> 0 ;
    unsigned char b1 = ( theInt & 0x0000FF00 ) >> 8 ;
    unsigned char b2 = ( theInt & 0x00FF0000 ) >> 16 ;
    unsigned char b3 = ( theInt & 0xFF000000 ) >> 24 ;

    int value;
    value = 0;
    value |= b0;
    value <<= 8;
    value |= b1;
    value <<= 8;
    value |= b2;
    value <<= 8;
    value |= b3;
    theInt = value;
#endif
}

//===========================================================================
// void XLTranslator::FixEndianProblem()
//===========================================================================
// Description: walks through all the translations to make sure nothing crashes
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
void XLTranslator::FixEndianProblem()
{
    #ifdef PS2EE
        return;
    #endif
    
    if( m_EndianSwappingDone )
    {
        return;
    }
    assert( m_EndianSwappingDone != true );

    XLEntry* entry_array = (XLEntry*)(&(mDatafile[mMost_recently_used])[1]);
    rAssert( entry_array != NULL );
    if( entry_array == NULL )
    {
        return;
    }

    //fix the endianness of all the data in the file
    unsigned long int maxEntry = mDatafile[ mMost_recently_used ]->GetTotalEntries();
    //SwapEndian( maxEntry );
    mDatafile[ mMost_recently_used ]->SetTotalEntries( maxEntry );

    unsigned long modulo = mDatafile[ mMost_recently_used ]->GetModulo();
    //SwapEndian( modulo );
    mDatafile[ mMost_recently_used ]->SetModulo( modulo );

    char* basePointer = ( char* )mDatafile[ mMost_recently_used ];

    unsigned int i;
    for( i = 0; i < maxEntry; i++ )
    {
    
        unsigned int hash = entry_array[ i ].hash;
        SwapEndian( hash );
        entry_array[ i ].hash = hash;

        unsigned int offset = entry_array[ i ].offset;
        SwapEndian( offset );
        entry_array[ i ].offset = offset;

        //output the strings to the screen for debugging
//        char* stringPointer =  &basePointer[ offset ] ;
//        UnicodeString returnMe ;
//        returnMe.ReadUnicode( stringPointer ) ;
//
//        char output[ 256 ] = "" ;
//        returnMe.MakeAscii( output, 256 );
//        std::cout << output << std::endl; 
    }
    m_EndianSwappingDone = true;
}



//*******************************************************************
// Private Methods
//*******************************************************************
//-------------------------------------------------------------------
//  XLTranslator
//-------------------------------------------------------------------
XLTranslator::XLTranslator() :
    mLanguage(XL_DEFAULT_LANGUAGE),
    mMost_recently_used(0),
    m_EndianSwappingDone( false )
{
    //
    // Set all datafile pointer to NULL to begin with.
    //
    for(int n=0; n<XL_MAX_REGISTERED_DATAFILES; n++)
    {
        mDatafile[n] = NULL;
    }
}


//-------------------------------------------------------------------
//  ~XLTranslator
//-------------------------------------------------------------------
XLTranslator::~XLTranslator()
{
    //
    // Make sure that all datafile have all been unregistered.
    //

    //IAN IMPROVE: who cares if all the datafies are unregistered - we delete the memory buffers anyways
    /*
    for(int n=0; n<XL_MAX_REGISTERED_DATAFILES; n++)
    {
        rAssert( mDatafile[ n ] == NULL ) ;
//        rAssertMsg(!mDatafile[n],"XLTranslator reports a datafile is still registered with it!");
    }
    */
}
