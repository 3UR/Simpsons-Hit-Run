//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Asynchronous File Loader
//
// Description: This class does an asynchronous read of the specified file.
//
// Authors:     Darwin Chau
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================

#include "AsyncFileLoader.h"
#include "debugMessages.h"
#include <raddebug.hpp>
#include <string.h>

// needed to get access to the getcwd command
#if defined(RAD_WIN32) || defined(RAD_UWP)
    #include <direct.h>
#endif

//===========================================================================
// Global Data Initialization
//===========================================================================
bool AsyncFileLoader::g_UseBackSlashes = true;
//IRadDrive* AsyncFileLoader::m_pDrive = NULL;
//IAN IMPROVE: wrong name

//
// This is the name of a drive we are using
//
#if defined(RAD_WIN32) || defined(RAD_UWP)
static char MyDrive[ ] = "C:";               //windows
#endif

//===========================================================================
// Public Member Functions
//===========================================================================

//===========================================================================
// AsyncFileLoader::AsyncFileLoader
//===========================================================================
// Description: Constructor.
//
// Parameters:    None.
//
// Return:      N/A.
//
//===========================================================================
AsyncFileLoader::AsyncFileLoader()
:
    mRefCount( 1 ),
    mBuffer( NULL ),
    mBufferSize( 0 ),
    mCallback( NULL ),
    mLoadPending( false )
{
    //
    // Figure out the drive letter on the PC
    //
#ifdef _WINDOWS
    char path[ 256 ];
    ::_getcwd( path, 256 );
    MyDrive[ 0 ] = path[ 0 ];
#endif

    //
    // Open a drive.
    //
    //radDriveOpen( &m_pDrive, MyDrive, NormalPriority, ScroobyPermPool );
    //rAssert( m_pDrive != NULL );
}


//===========================================================================
// AsyncFileLoader::~AsyncFileLoader
//===========================================================================
// Description: Destructor
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      Nothing
//
//===========================================================================
AsyncFileLoader::~AsyncFileLoader()
{
    //
    // Nothing to see here...
    //
    if( mRefCount != 0 )
    {
        rAssert( mRefCount == 1 );
        mRefCount = 0;
    }

    delete[] mBuffer;
    mBuffer = NULL;
}

//===========================================================================
// AsyncFileLoader::FixSlashes
//===========================================================================
// Description: fixes slashes in the filename
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
void AsyncFileLoader::FixSlashes( char* filename )
{
    const char forwardSlash = '/';
    const char backSlash = '\\';

    char slashToUse;
    if( g_UseBackSlashes )
    {
        slashToUse = backSlash;
    }
    else
    {
        slashToUse = forwardSlash;
    }

    int size = ::strlen( filename );
    int i;
    for( i = 0; i < size; i++ )
    {
        char& current = filename[ i ];
        if( ( current == forwardSlash ) || ( current == backSlash ) )
        {
            current = slashToUse;
        }
    }
}

//===========================================================================
// AsyncFileLoader::ForceBackSlashes
//===========================================================================
// Description: different slashes are required for filenames on different 
//              platforms.  This function allows you to set them up properly
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
void AsyncFileLoader::ForceBackSlashes( bool useBackSlashes )
{
    g_UseBackSlashes = useBackSlashes;
}


//===========================================================================
// AsyncFileLoader::LoadFile
//===========================================================================
// Description: Asynchronous request to load a file.
//
// Constraints:    The client is responsible for installing the error handler
//              to deal with missing files.
//
//              Each instance of this class can only have one load request
//              pending.  Subsequent calls to LoadFile() are ignored.
//
// Parameters:    fileName - e.g. "C:\\MYFILE.TXT", "..\\TEST.BIN", etc.
//
//              pCallback - This client implemented callback will be invoked
//                          when the file load is complete.
//
//              buffer - The client may optionally provide the memory buffer
//                       for the file to be read into.
//
//              bufferSize - Size in bytes of the client supplied buffer.                          
//
// Return:      Nothing
//
//===========================================================================
void AsyncFileLoader::LoadFile
(
    const char* filename,
    AsyncFileLoaderCallback* pCallback,
    char* buffer,
    unsigned int bufferSize,
    unsigned int allocator
)
{
    #ifndef RAD_RELEASE
        if( Scrooby::AsyncFileLoaderMessagesOn )
        {
            char message[ 256 ] = "";
            sprintf( message, "AsyncFileLoader::LoadFile( %s )", filename );
            Scrooby::ScroobyInfo( message );
        }
    #endif

    bool filenameValid = AsyncFileLoader::ValidFilename( filename );
    if( filenameValid == false )
    {
        char message[ 256 ] = "";
        sprintf( message, "NON 8.3 file on Device \"%s\"", filename );
        Scrooby::ScroobyWarning( 1, message );
    }


    //
    // Ignore subsequent load calls until the first request is complete.
    //
    if( mLoadPending )
    {
        Scrooby::ScroobyWarning( 0, "AsyncFileLoader::Still busy loading a file. Request was ignored!" );
        return;
    }

    //
    // Set the flag to prevent multiple load requests.
    //
    mLoadPending = true;

    //
    // Store these for use once the load is complete.
    //
    mCallback = pCallback;
    mBuffer = buffer;
    mBufferSize = bufferSize;
    
    //
    // Lets open the file asynchrnously. This call initiates the open. The file is not opened until
    // the callback is invoked. We pass in the user data field a value to indicate this is the open completing.
    //
    radFileOpen( &m_pIRadFile, filename, false, OpenExisting, NormalPriority, 0, allocator );
    rAssert( m_pIRadFile != NULL );
    m_pIRadFile->AddCompletionCallback( this, (void*) 0 );
}


//===========================================================================
// AsyncFileLoader::OnFileOperationsComplete( void* pUserData )
//===========================================================================
// Description: This callback gets invoked when previously issued file operations
//              complete. We used this to signal when the file was openned and when
//              the read completes.
//
// Constraints:    The client can ignore this.
//
// Parameters:    userData = 0, file open completed.
//              userData = 1, file read completed.
//
// Return:      Nothing
//
//===========================================================================
void AsyncFileLoader::OnFileOperationsComplete( void* pUserData )
{
    //
    // Check which operation completed.
    //
    if( pUserData == nullptr )
    {
        //
        // Here we know the callback was invoked after the file open completed. We 
        // can get the file size and allocate memory.
        //
        mFileSize = m_pIRadFile->GetSize( );

        //
        // TROUBLE! The buffer the client provided isn't big enough!!!
        //
        if( (NULL != mBuffer) && (mBufferSize < mFileSize) )
        {
                
            //
            // Toggle the flag so clients can now request another file load.
            //
            mLoadPending = false;

            //
            // Free the file object.
            //  
            m_pIRadFile->Release( );
            
            //
            // Call the client back with these invalid parameters.
            //
            mCallback->OnFileLoadComplete( 0, NULL );

            return;
        }

        //
        // Allocate new storage for the file if the client hasn't provided it.
        //
        if( NULL == mBuffer )
        {
            mBuffer = new char[mFileSize + 10];
            //rAssert( (int)mBuffer != 0x024d2ca8 );
            rAssert( mBuffer != NULL );
            memset( mBuffer, 0x00, mFileSize + 10 );
            rAssert( mBuffer );
        }

        //
        // Lets issue an asynchronous read. Again queue a callback, but this time with
        // user data of 1, to indicate that the read is completing.
        //
        m_pIRadFile->ReadAsync( mBuffer, mFileSize );
        m_pIRadFile->AddCompletionCallback( this, (void*) 1 );
        return;
    }
    else
    {
        //
        // Here the read of the file has completed. We can release the file, causing
        // it to be closed.
        //
        m_pIRadFile->Release( );

        //
        // Toggle the flag so clients can now request another file load.
        //
        mLoadPending = false;

        //
        // We disavow all knowledge of the allocated buffer.
        //
        char* old_mBuffer = mBuffer;
        mBuffer = NULL;
        mBufferSize = 0;

        //
        // The file is finally loaded so invoked the client provided callback and
        // pass in the filesize and pointer to the file data buffer.
        //
        mCallback->OnFileLoadComplete( mFileSize, old_mBuffer );
    }
}

//===========================================================================
// AsyncFileLoader::RegisterCementFile( const char* cementFileName )
//===========================================================================
// Description: tells the Asyncfile loader to look in a specific cement file 
//
// Constraints:    
//
// Parameters:    cementFIleName - the name of hte file to be used to look at
//
// Return:      Nothing
//
//===========================================================================
void AsyncFileLoader::RegisterCementFile( const char* cementFileName )
{
    //
    // Lets register our cement library
    //
    IRadCementLibrary * pLibrary = NULL;
    radFileRegisterCementLibrary( &pLibrary, cementFileName );
    rAssert( pLibrary );

    Scrooby::ScroobyWarning( 0, "SCROOBY is not actually registering the cement library asynchronously" );
    pLibrary->WaitForCompletion( );
    Scrooby::ScroobyWarning( pLibrary->IsOpen( ), "SCROOBY could not find the cement file - proceeding from disk" );

}

//===========================================================================
// Overrides required for COM 
//===========================================================================

//===========================================================================
// AsyncFileLoader::AddRef
//===========================================================================
// Description: Handle the reference count for this COM stuff.
//
// Constraints:    The client can ignore this.
//
// Parameters:    None
//
// Return:      Nothing
//
//===========================================================================
void AsyncFileLoader::AddRef( void )
{
     mRefCount++;
}


//===========================================================================
// AsyncFileLoader::Release
//===========================================================================
// Description: Handle the reference count for this COM stuff.
//
// Constraints:    The client can ignore this.
//
// Parameters:    None
//
// Return:      Nothing
//
//===========================================================================
void AsyncFileLoader::Release( void )
{ 
    mRefCount--;
    if( mRefCount == 0 )
    {
        delete this; 
    }
}

//===========================================================================
// AsyncFileLoader::Release
//===========================================================================
// Description: Handle the reference count for this COM stuff.
//
// Constraints:    The client can ignore this.
//
// Parameters:    None
//
// Return:      Nothing
//
//===========================================================================
bool AsyncFileLoader::ValidFilename( const char* filename )
{
    //directory size =    11
    //filename size =    8
    //extension size =    3
    //no spaces allowed

    const int maxDirectoryLength = 11;
    const int maxFilenameLength = 8;
    const int maxExtensionLength = 3;

    bool seenTheDot = false;

    int i;
    int max = strlen( filename );
    int currentCount = 0;
    for( i = 0; i < max; i++ )
    {
        if( ( filename[ i ] == '/' ) || ( filename[ i ] == '\\' ) )
        {
            //we've seen a slash, so this was a directory name
            if( currentCount > maxDirectoryLength )
            {
                return false;
            }
            else
            {
                seenTheDot = false; //handle the ../ cases
                currentCount = 0;
            }
        }
        else if( filename[ i ] == '.' )
        {
            //we've just seen the filename
            seenTheDot = true;
            if( currentCount > maxFilenameLength )
            {
                return false;
            }
            else
            {
                currentCount = 0;
            }
        }
        else if( filename[ i ] == ' ' )
        {
            return false;
        }
        else
        {
            currentCount++;
        }
    }

    //now we've seen all the characters in the file
    if( seenTheDot == true )
    {
        //the file had an extension, check if it was too large
        if( currentCount > maxExtensionLength )
        {
            return false;
        }
    }
    else
    {
        //there was no extension
        if( currentCount > maxFilenameLength )
        {
            return false;
        }
    }
    return true;
}
