//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        file.cpp
//
// Subsystem:   Radical Drive System
//
// Description:	This file contains the implementation of the radFile class.
//
// Revisions:
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <string.h>
#include "requests.hpp"
#include "file.hpp"
#include "drive.hpp"

//=============================================================================
// Static Data Definitions
//=============================================================================

static IRadMemoryPool*  s_FilePool = NULL;
static IRadThreadMutex* s_Mutex = NULL;

//=============================================================================
// Functions
//=============================================================================

//=============================================================================
// Function:    radFilePoolInitialize
//=============================================================================
// Description: This routine is invoked to intialized the pool for allocating
//              File objects.
//
// Parameters:  maximum number of file.
//
// Returns:     
//------------------------------------------------------------------------------

void radFilePoolInitialize
( 
    unsigned int        maxOpenFiles, 
    radMemoryAllocator  alloc
)
{
    rAssertMsg( s_FilePool == NULL, "radFileSystem: file pool already initialized" );

    radThreadCreateMutex( &s_Mutex, alloc );
    rAssert( s_Mutex != NULL );

    //
    // Create a memory pool for allocating File objects.
    //
    radMemoryCreatePool( &s_FilePool, sizeof( radFile ), maxOpenFiles, 0, false, None, alloc, "radFile" );
    rAssert( s_FilePool != NULL );
}

//=============================================================================
// Function:    radFilePoolTerminate
//=============================================================================
// Description: This routine is invoked to free the file pool. 
//
// Parameters: 
//
// Returns:     
//------------------------------------------------------------------------------

void radFilePoolTerminate( void )
{
    rAssertMsg( s_FilePool, "radFileSystem: file pool not initialized");

    radRelease( s_Mutex, NULL );
    s_Mutex = NULL;

    unsigned int elementSize;
    unsigned int numberAllocated;
    unsigned int numberFree;
   
    s_FilePool->GetStatus( &elementSize, &numberFree, &numberAllocated );
    rAssertMsg( numberAllocated == 0, "radFileSystem: not all files were destroyed." );

    radRelease( s_FilePool, NULL );
    s_FilePool = NULL;
}

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    radFile::operator new
//=============================================================================
// Description: This member is the placement new. Required since file objects
//              are allocated from a pool.
//
// Parameters:  size
//
// Returns:     address 
//------------------------------------------------------------------------------

void* radFile::operator new( size_t size )
{
    (void) size;

    rAssertMsg( s_FilePool, "radFileSystem: file pool not initialized");
    
    s_Mutex->Lock( );

    void* p = s_FilePool->GetMemory( );
    rAssertMsg( p != NULL, "radFileSystem: increase maximum number of files" );

    s_Mutex->Unlock( );

    return( p );
}

//=============================================================================
// Function:    radFile::operator delete
//=============================================================================
// Description: Return file to pool
//
// Parameters:  p
//
// Returns:    
//------------------------------------------------------------------------------

void radFile::operator delete( void* p )
{
    rAssertMsg( s_FilePool, "radFileSystem: file pool not initialized");

    s_Mutex->Lock( );

    s_FilePool->FreeMemory( p );

    s_Mutex->Unlock( );
}

//=============================================================================
// Function:    radFile::radFile
//=============================================================================
// Description: Constructor for the file.
//
// Parameters:  pointer to parent drive object.
//              priority.
//              
// Returns:     
//------------------------------------------------------------------------------

radFile::radFile
(
    radDrive*          pDrive,
    const char*        pFileName,
    bool               writeAccess,
    radFileOpenFlags   flags, 
    radFilePriority    priority,
    unsigned int       cacheSize,
    radMemoryAllocator alloc,
    radMemorySpace     cacheSpace
)
    :
    m_RefCount( 1 ),
    m_pDrive( pDrive ),
    m_Priority( priority ),
    m_LastError( Success ),
    m_OutstandingRequests( 0 ),
    m_IsOpen( false ),
    m_WriteAccess( writeAccess ),
    m_BufferedState( BufferedReadAuto ),
    m_Position( 0 ),
    m_Size( 0 ),
    m_pOriginalFile( NULL ),
    m_BaseOffset( 0 ),
    m_FileCache( ),
    m_CacheMemory( 0 ),
    m_CacheAllocator( 0 ),
    m_IsSaveGame( false ),
    m_OptimalSize( radFileMaxSectorSize )
{
    rAssert( m_pDrive != NULL );
    m_pDrive->AddRef( );
    rAssert( priority <= LowPriority );

#ifdef RAD_DEBUG
    m_FirstRemoval = true;
    m_pDrive->AddFileReference( );
#endif

    strncpy( m_Filename, pFileName, radFileFilenameMax ); 
    m_Filename[ radFileFilenameMax ] = '\0';

    //
    // Check if we can actually do this
    //
    unsigned int capabilities = m_pDrive->GetCapabilities( );

    rAssertMsg( capabilities & radDriveFile, "radFileSystem: drive does not support regular files." );
    if ( writeAccess || (flags != OpenExisting) )
    {
        rAssertMsg( capabilities & radDriveWriteable, "radFileSystem: drive is not writeable." );
    }

    //
    // Construct a cache object if needed.
    //
    if ( cacheSize > 0 )
    {
        char cacheName[ radFileFilenameMax + 13 ];
        sprintf( cacheName, "FILE_CACHE: %s", pFileName );
        
        ::radMemorySetAllocationName( cacheName );

        //
        // Allocate enough memory to align things just right.
        //
        m_CacheMemory = (unsigned char*) ::radMemorySpaceAlloc( cacheSpace, alloc, cacheSize + radFileOptimalMemoryAlignment );

        if( m_CacheMemory == NULL )
        {
#       ifdef RAD_DEBUG
            if ( m_CacheMemory == NULL )
            {
                rDebugPrintf( "radFile: ran out of memory for file caching on file [%s]\n", pFileName );
            }
#       endif
        }
        else
        {
            m_CacheAllocator = alloc;
            m_FileCache.InitCache( 
                ( unsigned char* )::radMemoryRoundUp( (uintptr_t) m_CacheMemory, radFileOptimalMemoryAlignment ),
                cacheSize, 
                cacheSpace );
        }
    }

    //
    // Lets construct an init file request and queue to the file. 
    //
    m_pDrive->QueueRequest( new FileOpenRequest( this, flags, writeAccess ), m_Priority );
}

//=============================================================================
// Function:    radFile::radFile -- for SaveGames
//=============================================================================

radFile::radFile
( 
    radDrive*          pDrive,
    const char*        pFileName,
    bool               writeAccess,
    radFileOpenFlags   flags,
    radMemcardInfo*    memcardInfo,
    unsigned int       maxSize,
    radFilePriority    priority 
)
    :
    m_RefCount( 1 ),
    m_pDrive( pDrive ),
    m_Priority( priority ),
    m_LastError( Success ),
    m_OutstandingRequests( 0 ),
    m_IsOpen( false ),
    m_WriteAccess( writeAccess ),
    m_BufferedState( BufferedReadAuto ),
    m_Position( 0 ),
    m_Size( 0 ),
    m_pOriginalFile( NULL ),
    m_BaseOffset( 0 ),
    m_FileCache( ),
    m_CacheMemory( 0 ),
    m_CacheAllocator( 0 ),
    m_IsSaveGame( true ),
    m_OptimalSize( radFileMaxSectorSize )
{
    rAssert( m_pDrive != NULL );
    m_pDrive->AddRef( );
    rAssert( priority <= LowPriority );

#ifdef RAD_DEBUG
    m_FirstRemoval = true;
    m_pDrive->AddFileReference( );
#endif

    strncpy( m_Filename, pFileName, radFileFilenameMax ); 
    m_Filename[ radFileFilenameMax ] = '\0';

    //
    // Check if we can actually do this
    //
    unsigned int capabilities = m_pDrive->GetCapabilities( );
    rAssertMsg( capabilities & radDriveSaveGame, "radFileSystem: drive does not support save games." );
    if ( writeAccess || (flags != OpenExisting) )
    {
        rAssertMsg( capabilities & radDriveWriteable, "radFileSystem: drive is not writeable." );
    }

    //
    // Lets construct an init file request and queue to the file. 
    //
    m_pDrive->QueueRequest( new SaveGameOpenRequest( this, flags, writeAccess, memcardInfo, maxSize ),  m_Priority );
}

//=============================================================================
// Function:    radFile::~radFile
//=============================================================================
// Description: Destructor for the file.
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

radFile::~radFile( void )
{
    m_pDrive->Release( );
}

//=============================================================================
// Function:    radFile::AddRef
//=============================================================================
// Description: Increment reference count
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

void radFile::AddRef( void )
{
    Lock( );
    rAssert( m_RefCount < MAX_REFCOUNT );
    m_RefCount++;
    Unlock( );
}

//=============================================================================
// Function:    radFile::Release
//=============================================================================
// Description: Decrement reference count
//
// Parameters:  
//              
// Returns:     
//
// Notes:       A close request is issued once all references are gone.
//------------------------------------------------------------------------------

void radFile::Release( void )
{
    Lock( );
    rAssert( m_RefCount > 0 && m_RefCount <  MAX_REFCOUNT );
    m_RefCount--;
    if (m_RefCount == 0 )
    {
        //
        // Destroy our cache.
        //
        if ( m_CacheMemory != NULL )
        {
            ::radMemorySpaceFree( m_FileCache.GetCacheMemorySpace( ), m_CacheAllocator, m_CacheMemory );
            m_CacheMemory = NULL;
        }

        if ( m_IsOpen )
        {
            //
            // Destroy our cache.
            //
            if ( m_CacheMemory != NULL )
            {
                ::radMemorySpaceFree( m_FileCache.GetCacheMemorySpace( ), m_CacheAllocator, m_CacheMemory );
            }

            Unlock( );

#      ifdef RAD_DEBUG
            //
            // We only remove the reference the FIRST time the ref count hits 0.
            //
            if ( m_FirstRemoval )
            {
                m_pDrive->RemoveFileReference( );
                m_FirstRemoval = false;
            }
#       endif

            m_pDrive->QueueRequest( new FileCloseRequest( this ), m_Priority );
        }
        else
        {
            m_RefCount = MAX_REFCOUNT / 2;
            Unlock( );

#      ifdef RAD_DEBUG
            //
            // We only remove the reference the FIRST time the ref count hits 0.
            //
            if ( m_FirstRemoval )
            {
                m_pDrive->RemoveFileReference( );
                m_FirstRemoval = false;
            }
#       endif

            delete this;
        }
    }
    else
    {
        Unlock( );
    }
}

//=============================================================================
// Function:    radFile::Lock
//=============================================================================
// Description: Start a critical section
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

void radFile::Lock( void )
{
    s_Mutex->Lock( );
}

//=============================================================================
// Function:    radFile::Unlock
//=============================================================================
// Description: End a critical section
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

void radFile::Unlock( void )
{
    s_Mutex->Unlock( );
}

//=============================================================================
// Function:    radFile::CommitAsync
//=============================================================================

void radFile::CommitAsync( void )
{
    m_pDrive->QueueRequest( new FileCommitRequest( this ), m_Priority );
}

//=============================================================================
// Function:    radFile::ReadAsync
//=============================================================================
// Description: Asynchronous read from file.
//
// Parameters:  pBuffer -- buffer to read into
//              bytesToRead -- amount to read
//              pBufferSpace -- memory space to read into
//              
// Returns:     
//------------------------------------------------------------------------------

void radFile::ReadAsync( void* pBuffer, unsigned int bytesToRead, radMemorySpace pBufferSpace )
{
    rAssert( pBuffer != NULL );
    rAssert( pBufferSpace != radMemorySpace_Null );

    m_pDrive->QueueRequest( new FileReadRequest( this, pBuffer, bytesToRead, pBufferSpace ), m_Priority );
}

//=============================================================================
// Function:    radFile::WriteAsync
//=============================================================================
// Description: Asynchronous write to file.
//
// Parameters:  pBuffer -- buffer to read from
//              bytesToWrite -- amount to write
//              pBufferSpace -- memory space to write from
//              
// Returns:     
//------------------------------------------------------------------------------

void radFile::WriteAsync( const void* pBuffer, unsigned int bytesToWrite, radMemorySpace pBufferSpace )
{
    rAssert( pBuffer != NULL );
    rAssertMsg( m_WriteAccess,"radFileSystem: File is not writeable." );

    m_pDrive->QueueRequest( new FileWriteRequest( this, pBuffer, bytesToWrite, pBufferSpace ), m_Priority );
}

//=============================================================================
// Function:    radFile::SetPositionAsync
//=============================================================================
// Description: Set the file pointer absolutely from the beginning of the file
//
// Parameters:  position -- must be less than the the size of the file
//              
// Returns:     
//------------------------------------------------------------------------------

void radFile::SetPositionAsync( unsigned int position )
{
    m_pDrive->QueueRequest( new FileSetPositionRequest( this, position ), m_Priority );
}

//=============================================================================
// Function:    radFile::GetPositionAsync
//=============================================================================
// Description: Get the file pointer.
//
// Parameters:  pPosition
//              
// Returns:     
//------------------------------------------------------------------------------

void radFile::GetPositionAsync( unsigned int* pPosition )
{
    rAssert( pPosition != NULL );
    m_pDrive->QueueRequest( new FileGetPositionRequest( this, pPosition ), m_Priority );
}

//=============================================================================
// Function:    radFile::GetSizeAsync
//=============================================================================
// Description: Get the size of the file.
//
// Parameters:  pSize
//              
// Returns:     
//------------------------------------------------------------------------------

void radFile::GetSizeAsync( unsigned int* pSize )
{
    rAssert( pSize != NULL );
    m_pDrive->QueueRequest( new FileGetSizeRequest( this, pSize ), m_Priority );
}

//=============================================================================
// Synchronous versions of the above functions
//=============================================================================
void radFile::ReadSync( void* pBuffer, unsigned int bytesToRead, radMemorySpace pBufferSpace )
{
    ReadAsync( pBuffer, bytesToRead, pBufferSpace );
    WaitForCompletion( );
}

void radFile::WriteSync( const void* pBuffer, unsigned int bytesToWrite, radMemorySpace pBufferSpace )
{
    WriteAsync( pBuffer, bytesToWrite, pBufferSpace );
    WaitForCompletion( );
}

void radFile::SetPositionSync( unsigned int position )
{
    SetPositionAsync( position );
    WaitForCompletion( );
}

void radFile::GetPositionSync( unsigned int* pPosition )
{
    GetPositionAsync( pPosition );
    WaitForCompletion( );
}

void radFile::GetSizeSync( unsigned int* pSize )
{
    GetSizeAsync( pSize );
    WaitForCompletion( );
}

void radFile::CommitSync( void )
{
    CommitAsync( );
    WaitForCompletion( );
}

unsigned int radFile::GetSize( void )
{
    rAssert( m_IsOpen );
    return m_Size;
}

//=============================================================================
// Function:    radFile::CancelAsync
//=============================================================================
// Description: Remove all outstanding requests.
//
// Parameters:  
//              
// Returns:     
//
// Notes:       requests are only removed at the current priority, so changing
//              the priority will mean that requests won't be cancelled.
//------------------------------------------------------------------------------

void radFile::CancelAsync( void )
{
    m_pDrive->QueueRequest( new FileCancelRequest( this ), m_Priority, true );
}

//=============================================================================
// Function:    radFile::AddCompletionCallback
//=============================================================================
// Description: Add a callback
//
// Parameters:  pCallback
//              pUserData
//              
// Returns:     
//------------------------------------------------------------------------------

void radFile::AddCompletionCallback( IRadFileCompletionCallback* pCallback, void* pUserData )
{
    rAssert( pCallback != NULL );
    m_pDrive->QueueRequest( new FileCompletionRequest( ::radThreadGetActiveThread( ), this, pCallback, pUserData ), m_Priority, false );
}

//=============================================================================
// Function:    radFile::AddCompletionEvent
//=============================================================================
// Description: Add a semaphore
//
// Parameters:  pSemaphore
//              
// Returns:     
//------------------------------------------------------------------------------

void radFile::AddCompletionEvent( IRadThreadSemaphore* pSemaphore )
{
    rAssert( pSemaphore != NULL );
    m_pDrive->QueueRequest( new CompletionEventRequest( pSemaphore, m_pDrive ), m_Priority, false );
}

//=============================================================================
// Function:    radFile::CheckForCompletion
//=============================================================================
// Description: Check if there are any outstanding requests
//
// Parameters:  
//              
// Returns:     true if there are no outstanding requests  
//------------------------------------------------------------------------------

bool radFile::CheckForCompletion( void )
{
    Lock( );
    bool result = ( m_OutstandingRequests == 0 );
    Unlock( );

    return result;
}

//=============================================================================
// Function:    radFile::WaitForCompletion
//=============================================================================
// Description: Wait until there are no outstanding requests
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

void radFile::WaitForCompletion( void )
{
    while ( !CheckForCompletion( ) )
    {
        m_pDrive->Service( );
//        radThreadSleep(0);
    }
}

//=============================================================================
// Function:    radFile::GetLastError
//=============================================================================
// Description: Return the error code from the last operation on this file.
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

radFileError radFile::GetLastError( void )
{
    Lock( );
    radFileError error = m_LastError;
    Unlock( );
    return error;
}

//=============================================================================
// Function:    radFile::IsOpen
//=============================================================================
// Description: Return whether the file is open
//
// Parameters:  
//              
// Returns:    true if the file is open 
//------------------------------------------------------------------------------

bool radFile::IsOpen( void )
{
    return m_IsOpen;
}

//=============================================================================
// Function:    radFile::GetFilename
//=============================================================================
// Description: Return the file name
//
// Parameters:  
//              
// Returns:     pointer to the filename
//------------------------------------------------------------------------------

const char* radFile::GetFilename( void )
{
    return m_Filename;
}

//=============================================================================
// Function:    radFile::GetFilename
//=============================================================================
// Description: Return the file name
//
// Parameters:  
//              
// Returns:     pointer to the filename
//------------------------------------------------------------------------------

const char* radFile::GetDrivename( void )
{
    return m_pDrive->GetDriveName( );
}

//=============================================================================
// Function:    radFile::SetPriority
//=============================================================================
// Description: Set the priority of requests for this file
//
// Parameters:  priority
//              
// Returns:  
//------------------------------------------------------------------------------

void radFile::SetPriority( radFilePriority priority )
{
    rAssert( priority <= LowPriority );
    rWarningMsg( m_OutstandingRequests == 0,
        "radFileSystem: changing the priority of a file while there are outstanding requests." );

    m_Priority = priority;
}

//=============================================================================
// Function:    radFile::GetPriority
//=============================================================================
// Description: Get the priority of requests for this file
//
// Parameters:  
//              
// Returns:     priority
//------------------------------------------------------------------------------

radFilePriority radFile::GetPriority( void )
{
    return m_Priority;
}

//=============================================================================
// Function:    radFile::IsSaveGame
//=============================================================================
// Description: Is the file a save game
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

bool radFile::IsSaveGame( void )
{
    rAssert( m_IsOpen );
    return m_IsSaveGame;
}

//=============================================================================
// Function:    radFile::GetOptimalSize
//=============================================================================
// Description: Get the optimal read size/alignment
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

unsigned int radFile::GetOptimalSize( void )
{
    rAssert( m_IsOpen );
    return m_OptimalSize;
}

//=============================================================================
// Function:    radFile::SetBufferedRead
//=============================================================================
// Description: Turn on/off bad read warnings.
//
// Parameters:  
//              
// Returns:    
//------------------------------------------------------------------------------

void radFile::SetBufferedRead( BufferedReadState state )
{
    rAssert( m_OutstandingRequests == 0 );
    m_BufferedState = state;
}

//=============================================================================
// Function:    radFile::GetHandle
//=============================================================================
// Description: Return the system specific handle.
//
// Parameters:  
//              
// Returns:    
//------------------------------------------------------------------------------

void* radFile::GetHandle(void)
{
    rAssert( m_IsOpen );
    return m_Handle;
}
