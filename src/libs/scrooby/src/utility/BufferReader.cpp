//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Stream Reader 
//
// Description: This class does buffered reads of the specified file and
//              returns a single char to the client on request.
//
// Authors:     Darwin Chau
//              Eric Tong
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================
#include "utility/BufferReader.h"

#include <raddebug.hpp>

//===========================================================================
// StreamReader::StreamReader
//===========================================================================
// Description: Constructor.
//
// Constraints:    
//
// Parameters:    
//
// Return:      Nothing
//
//===========================================================================
StreamReader::StreamReader( const char* buffer, 
                            const unsigned int bufferSize )
:
    mBuffer( buffer ),
    mBufferSize( bufferSize ),
    mBytesStreamed( 0 )
{
    rAssert( buffer );
}


//===========================================================================
// StreamReader::GetNextChar
//===========================================================================
// Description: Returns next available character
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      char - the next char - '\0' if nothing
//
//===========================================================================
char StreamReader::GetNextChar()
{
    //
    // At the end of the file.
    //
    if( mBytesStreamed == mBufferSize )
    {
        return( '\0' );    
    }

    return( mBuffer[ mBytesStreamed++ ] );
}


/*
//===========================================================================
// Includes
//===========================================================================

#include "StreamReader.h"

#include <stdlib.h>

#include <bObjMngr.hpp>
#include <ComponentId.hpp> 
#include <bDispatch.hpp>
#include <raddebug.hpp>


//===========================================================================
// Member Functions
//===========================================================================

//===========================================================================
// StreamReader::StreamReader
//===========================================================================
// Description: Constructor.
//
// Constraints:    The client is responsible for installing the error handler
//              to deal with missing files.
//
// Parameters:    fileName - e.g. "C:\\MYFILE.TXT", "..\\TEST.BIN", etc.
//
//              readBufferSize - number of bytes to read at a time.
//
// Return:      Nothing
//
//===========================================================================
StreamReader::StreamReader( const char* fileName, 
                            const unsigned int readBufferSize )
:
    mRefCount( 1 ),
    mpDispatcherHack( NULL ),
    mpIReadAsync( NULL ),
    mFileReady( false ),
    mReadComplete( false ),
    mBufferSize( readBufferSize ),
    mFileSize( 0 ),
    mBytesStreamed( 0 ),
    mIndex( 0 ),
    mBuffer( NULL )
{
    //
    // We will read chunks of the file into here.
    //
    mBuffer = new char[ readBufferSize ];

    //
    // Get the interface to the drive system.
    //
    bITokenObjectNameSpace* pINameSpace;
    
    bGetObject( CID_DriveSystem,
                NULL,
                IID_bITokenObjectNameSpace,
                reinterpret_cast<void**>( &pINameSpace ) );

    rAssert( pINameSpace != NULL );

    //
    // Start by finding the size of this file which will in turn get
    // the read file interface.
    //
    pINameSpace->GetTokenNameObject( fileName, IID_bISizeSync, this ); 
    pINameSpace->Release();


    //
    // Get interface pointer to the hacked dispatcher;
    //
    bIDispatcher* pDispatcher;      
    bGetObject( CID_Dispatcher,
                NULL,
                IID_bIDispatcher,
                reinterpret_cast<void**>( &pDispatcher ) );

    rAssert( pDispatcher != NULL );

    pDispatcher->GetInterface( IID_bIDispatcherHack, (void**) &mpDispatcherHack );
    rAssert( mpDispatcherHack != NULL );

    pDispatcher->Release();
    
    //
    // Service the other pending tasks until the file is ready.
    // We'll loop here until our ObjectAvaiable() callback is invoked.
    //
    while( !mFileReady )
    {
        mpDispatcherHack->Do();
    }
}


//===========================================================================
// StreamReader::~StreamReader
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
StreamReader::~StreamReader()
{
    //
    // Free up the read buffer we allocated.
    //
    delete [] mBuffer;
}



//===========================================================================
// StreamReader::OnObjectAvailable
//===========================================================================
// Description: This callback gets invoked once we have been granted 
//              access to the file.
//
// Constraints:    The client can ignore this.
//
// Parameters:    pObject - the interface to the file is passed into us.
//
// Return:      Nothing
//
//===========================================================================
void StreamReader::OnObjectAvailable( bIObject* pObject )
{
    if( pObject == NULL )
    {     
        rAssertMsg( 0, "We should not be here\n" );
        return;
    }
   
    //
    // We can now get the size of the file.
    //
    bISizeSync* pISizeSync = reinterpret_cast<bISizeSync*>( pObject );

    mFileSize = pISizeSync->GetSizeSync( );

    //
    // Get and store the interface to read a file.
    //
    pISizeSync->GetInterface( IID_bIReadAsync,
                              reinterpret_cast<void**>( &mpIReadAsync ) );
    
    rAssert( mpIReadAsync != NULL );
    
    //
    // That's it for now.  We've basically "Open()"ed the file and 
    // fetched the size.  Control will now go back to the constructor
    // which will just exit.
    //
    mFileReady = true;
}
 


//===========================================================================
// StreamReader::GetNextBufferFromFile
//===========================================================================
// Description: Reads a chunk of the file and puts it into our buffer.
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      Nothing
//
//===========================================================================
void StreamReader::GetNextBufferFromFile()
{   
    //
    // Read a full buffer or just the remainder of the file?
    //
    unsigned int readSize;
    readSize = __min( mBufferSize, mFileSize - mBytesStreamed );
    
    //
    // The logic in GetNextChar() should never allow this to happen.
    //
    rAssert( readSize != 0 );

    //
    // We're using multiple inheritance in this class and we need to get
    // back up to a bIObject.
    //
    bIObject* pObj; 
    pObj = static_cast<bIObject*>( static_cast<bIObjectAvailableCallback*>( this ) );

    //
    // Request that the file be read into our buffer.
    //
    mpIReadAsync->ReadAsync( pObj, static_cast<void*>( mBuffer ), readSize, this );

    //
    // Service the other pending tasks until the file load is complete.
    // We'll loop here until our OnReadAsyncComplete() callback is invoked.
    //
    while( !mReadComplete )
    {
        mpDispatcherHack->Do();
    }
    
    mReadComplete = false;
}



//===========================================================================
// StreamReader::OnReadAsyncComplete
//===========================================================================
// Description: This callback gets invoked once the file read is complete.
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      Nothing
//
//===========================================================================
void StreamReader::OnReadAsyncComplete( void )
{
    //
    // Finished reading the file, set our flag so we can stop spinning the
    // dispatcher.
    //
    mReadComplete = true;
}



//===========================================================================
// StreamReader::GetNextChar
//===========================================================================
// Description: Returns next available character
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      char - the next char - '\0' if nothing
//
//===========================================================================
char StreamReader::GetNextChar()
{
    //
    // At the end of the file.
    //
    if( mBytesStreamed == mFileSize )
    {
        //
        // We're done so release all the file system references.
        //
        if( mpIReadAsync != NULL )
        {
            mpIReadAsync->Release();
            mpIReadAsync = NULL;
        }
        
        if( mpDispatcherHack != NULL )
        {
            mpDispatcherHack->Release();
            mpDispatcherHack = NULL;
        }

        return( '\0' );    
    }

    //
    // At the start of the file or need to re-fill the buffer.
    //
    if( mBytesStreamed == 0 || (mIndex == mBufferSize - 1) )
    {
        this->GetNextBufferFromFile();
        mIndex = 0;
        ++mBytesStreamed;
        return( mBuffer[ mIndex ] );
    }
    //
    // Reading from the current buffer.
    //
    else
    {
        ++mBytesStreamed;
        return( mBuffer[ ++mIndex ] );
    }
}



//===========================================================================
// StreamReader::AddRef
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
void StreamReader::AddRef( void )
{
     mRefCount++;
};


//===========================================================================
// StreamReader::Release
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
void StreamReader::Release( void )
{ 
    mRefCount--;
    if( mRefCount == 0 )
    {
        delete this; 
    }
}


//===========================================================================
// StreamReader::GetInterface
//===========================================================================
// Description: Handle the interface for this COM stuff.
//
// Constraints:    The client can ignore this.
//
// Parameters:    None
//
// Return:      Nothing
//
//===========================================================================
void StreamReader::GetInterface( bInterfaceId interfaceId, void** ppInterface )
{
    *ppInterface = NULL;
    
    if( interfaceId == IID_bIObjectAvailableCallback )
    {
        *ppInterface = static_cast< bIObjectAvailableCallback* >( this );
    }
    else if( interfaceId == IID_bIObject )
    {
        bIObjectAvailableCallback* pItons = static_cast< bIObjectAvailableCallback* >( this );    
        *ppInterface = static_cast< bIObject* >( pItons );
    }    
    else if( interfaceId == IID_bIReadAsyncCallback )
    {
        *ppInterface = static_cast< bIReadAsyncCallback* >( this );
    }

    if( *ppInterface != NULL )
    {
        reinterpret_cast< bIObject* >(*ppInterface)->AddRef( );
    }    
}
*/
