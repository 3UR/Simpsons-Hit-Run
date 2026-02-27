//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        instancedrive.cpp
//
// Subsystem:   Radical Drive System
//
// Description:	This file contains the implementation of the radInstanceDrive class.
//				This class repesents the user data instance associated with a
//				drive. 
//
// Revisions:
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <string.h>
#include "filesystem.hpp"
#include "requests.hpp"
#include "drive.hpp"
#include "instancedrive.hpp"

//=============================================================================
// Static Data Definitions
//=============================================================================

static IRadMemoryPool*  s_DrivePool = NULL;
static IRadThreadMutex* s_Mutex = NULL;

//=============================================================================
// Functions
//=============================================================================

//=============================================================================
// Function:    radDrivePoolInitialize
//=============================================================================
// Description: This routine is invoked to intialized the pool for allocating
//              drive objects.
//
// Parameters:  maximum number of drives.
//
// Returns:     
//------------------------------------------------------------------------------

void radDrivePoolInitialize
( 
    unsigned int        maxDrives, 
    radMemoryAllocator  alloc
)
{
    rAssertMsg( s_DrivePool == NULL, "radFileSystem: drive pool already initialized" );

    radThreadCreateMutex( &s_Mutex, alloc );
    rAssert( s_Mutex != NULL );

    //
    // Create a memory pool for allocating drive objects.
    //
    radMemoryCreatePool( &s_DrivePool, sizeof( radInstanceDrive ), maxDrives, 0, false, None, alloc, "radInstanceDrive" );
    rAssert( s_DrivePool != NULL );
}

//=============================================================================
// Function:    radDrivePoolTerminate
//=============================================================================
// Description: This routine is invoked to free the drive pool. 
//
// Parameters: 
//
// Returns:     
//------------------------------------------------------------------------------

void radDrivePoolTerminate( void )
{
    rAssertMsg( s_DrivePool, "radFileSystem: drive pool not initialized");

    radRelease( s_Mutex, NULL );
    s_Mutex = NULL;

    unsigned int elementSize;
    unsigned int numberAllocated;
    unsigned int numberFree;
   
    s_DrivePool->GetStatus( &elementSize, &numberFree, &numberAllocated );
    rAssertMsg( numberAllocated == 0, "radFileSystem: not all drives were destroyed." );

    radRelease( s_DrivePool, NULL );
    s_DrivePool = NULL;
}

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    radInstanceDrive::operator new
//=============================================================================
// Description: This member is the placement new. Required since drive objects
//              are allocated from a pool.
//
// Parameters:  size
//
// Returns:     address 
//------------------------------------------------------------------------------

void* radInstanceDrive::operator new( size_t size )
{
    (void) size;

    rAssertMsg( s_DrivePool, "radFileSystem: drive pool not initialized");
    
    s_Mutex->Lock( );

    void* p = s_DrivePool->GetMemory( );
    rAssertMsg( p != NULL, "radFileSystem: increase maximum number of drives" );

    s_Mutex->Unlock( );

    return( p );
}

//=============================================================================
// Function:    radInstanceDrive::operator delete
//=============================================================================
// Description: Return request to pool
//
// Parameters:  p
//
// Returns:    
//------------------------------------------------------------------------------

void radInstanceDrive::operator delete( void* p )
{
    rAssertMsg( s_DrivePool, "radFileSystem: drive pool not initialized");

    s_Mutex->Lock( );

    s_DrivePool->FreeMemory( p );

    s_Mutex->Unlock( );
}

//=============================================================================
// Function:    radInstanceDrive::radInstanceDrive
//=============================================================================
// Description: Constructor for the instance drive.
//
// Parameters:  pointer to parent drive object.
//              priority.
//              
// Returns:     
//------------------------------------------------------------------------------

radInstanceDrive::radInstanceDrive( radDrive* pDrive, radFilePriority priority )
    :
    m_RefCount( (int) 1 ),
    m_pDrive( pDrive ),
    m_Priority( priority ),
    m_OutstandingRequests( 0 ),
    m_DirSearch( false )
{
    rAssert( m_pDrive != NULL );
    m_pDrive->AddRef( );

    rAssert( priority <= LowPriority );

#ifdef RAD_DEBUG
    m_FirstRemoval = true;
    m_pDrive->AddDriveReference( );
#endif

    //
    // Lets construct an init drive request and queue to the drive. 
    //
    m_pDrive->QueueRequest( new DriveOpenRequest( this ),  m_Priority );
}

//=============================================================================
// Function:    radInstanceDrive::~radInstanceDrive
//=============================================================================
// Description: Destructor
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

radInstanceDrive::~radInstanceDrive( void )
{
    m_pDrive->Release( );
}

//=============================================================================
// Function:    radInstanceDrive::AddRef
//=============================================================================
// Description: This member is used to update the reference count of the object
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::AddRef( void )
{
    Lock( );
    rAssert( m_RefCount < MAX_REFCOUNT );
    m_RefCount++;
    Unlock( );
}

//=============================================================================
// Function:    radInstanceDrive::Release
//=============================================================================
// Description: This member is used to update the reference count of the object
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::Release( void )
{
    Lock( );
    rAssert( m_RefCount > 0 && m_RefCount <  MAX_REFCOUNT );
    m_RefCount--;
    if (m_RefCount == 0 )
    {
        if ( m_DirSearch )
        {
            m_DirSearch = false;
            Unlock( );

#      ifdef RAD_DEBUG
            //
            // We only remove the reference the FIRST time the ref count hits 0.
            //
            if ( m_FirstRemoval )
            {
                m_pDrive->RemoveDriveReference( );
                m_FirstRemoval = false;
            }
#     endif

            m_pDrive->QueueRequest( new DriveFindCloseRequest( this ), m_Priority );
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
                m_pDrive->RemoveDriveReference( );
                m_FirstRemoval = false;
            }
#     endif

            delete this;
        }
    }
    else
    {
        Unlock( );
    }
}

//=============================================================================
// Function:    radInstanceDrive::Lock
//=============================================================================
// Description: This member starts a critical section
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::Lock( void )
{
    rAssert( s_Mutex != NULL );
    s_Mutex->Lock( );
}

//=============================================================================
// Function:    radInstanceDrive::Unlock
//=============================================================================
// Description: This member ends a critical section
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::Unlock( void )
{
    rAssert( s_Mutex != NULL );
    s_Mutex->Unlock( );
}

//=============================================================================
// Function:    radInstanceDrive::GetCapabilities
//=============================================================================
// Description: This member gets the capabilities of the drive. Not locked since
//              it should be set at initilialization time and not change.
//
// Parameters:  
//              
// Returns:     drive capabilities
//------------------------------------------------------------------------------

unsigned int radInstanceDrive::GetCapabilities( void )
{
    return m_pDrive->GetCapabilities( );
}

//=============================================================================
// Function:    radInstanceDrive::GetMediaInfoAsync
//=============================================================================
// Description: This member gets the media information
//
// Parameters:  [out] pMediaInfo
//              
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::GetMediaInfoAsync( MediaInfo* pMediaInfo )
{
    rAssert( pMediaInfo != NULL );
    m_pDrive->QueueRequest( new DriveMediaRequest( this, pMediaInfo ),  m_Priority );
}

//=============================================================================
// Function:    radInstanceDrive::FormatAsync
//=============================================================================
// Description: Asynchronous format
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::FormatAsync( void )
{
    m_pDrive->QueueRequest( new DriveFormatRequest( this ),  m_Priority );
}

//=============================================================================
// Function:    radInstanceDrive::CreateDirectoryAsync
//=============================================================================
// Description: Asynchronous directory creation
//
// Parameters:  name of the directory
//              
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::CreateDirectoryAsync( const char* pDirectoryName )
{
    rAssert( pDirectoryName != NULL );

    //
    // Process the filename to get the file name in canonical form
    //
    char fullFilename[ radFileFilenameMax + 1 ];
    char driveSpec[ radFileDrivenameMax + 1 ];
    const char* filePart = NULL;
    radFileSystem::ProcessFileName( pDirectoryName, fullFilename, driveSpec, &filePart );

    m_pDrive->QueueRequest( new DriveCreateDirectoryRequest( this, filePart ), m_Priority );
}

//=============================================================================
// Function:    radInstanceDrive::DestroyDirectoryAsync
//=============================================================================
// Description: Asynchronous directory destruction
//
// Parameters:  name of the directory
//              
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::DestroyDirectoryAsync( const char* pDirectoryName )
{
    rAssert( pDirectoryName != NULL );

    //
    // Process the filename to get the file name in canonical form
    //
    char fullFilename[ radFileFilenameMax + 1 ];
    char driveSpec[ radFileDrivenameMax + 1 ];
    const char* filePart = NULL;
    radFileSystem::ProcessFileName( pDirectoryName, fullFilename, driveSpec, &filePart );

    m_pDrive->QueueRequest( new DriveDestroyDirectoryRequest( this, filePart ), m_Priority );
}

//=============================================================================
// Function:    radInstanceDrive::FindFirstAsync
//=============================================================================
// Description: Asynchronous start of directory enumeration
//
// Parameters:  name of the directory
//              [out] information about the first entry
//              
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::FindFirstAsync( const char* searchspec, DirectoryInfo* pDirectoryInfo )
{
    rAssert( searchspec != NULL );
    rAssert( pDirectoryInfo != NULL );

    //
    // Process the filename to get the file name in canonical form
    //
    char fullFilename[ radFileFilenameMax + 1 ];
    char driveSpec[ radFileDrivenameMax + 1 ];
    const char* filePart = NULL;
    radFileSystem::ProcessFileName( searchspec, fullFilename, driveSpec, &filePart );

    m_pDrive->QueueRequest( new DriveFindFirstRequest( this, filePart, pDirectoryInfo ), m_Priority );

}

//=============================================================================
// Function:    radInstanceDrive::FindNextAsync
//=============================================================================
// Description: Asynchronous continue of directory enumeration
//
// Parameters:  name of the directory
//              [out] information about the next entry
//              
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::FindNextAsync( DirectoryInfo* pDirectoryInfo )
{
    rAssert( pDirectoryInfo != NULL );
    m_pDrive->QueueRequest( new DriveFindNextRequest( this, pDirectoryInfo ), m_Priority );
}

//=============================================================================
// Function:    radInstanceDrive::DestroyFileAsync
//=============================================================================
// Description: Asynchronous file destruction
//
// Parameters:  name of the file
//              
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::DestroyFileAsync( const char* pFileName, bool simpleName )
{
    rAssert( pFileName != NULL );

    //
    // Process the filename to get the file name in canonical form
    //
    char fullFilename[ radFileFilenameMax + 1 ];
    char driveSpec[ radFileDrivenameMax + 1 ];
    const char* filePart = NULL;
    radFileSystem::ProcessFileName( pFileName, fullFilename, driveSpec, &filePart, simpleName );

    m_pDrive->QueueRequest( new DriveFileDestroyRequest( this, filePart ), m_Priority );
}

//=============================================================================
// Synchronous versions of the above functions.
//=============================================================================

void radInstanceDrive::GetMediaInfoSync( MediaInfo* pMediaInfo )
{
    GetMediaInfoAsync( pMediaInfo );
    WaitForCompletion( );
}

void radInstanceDrive::FormatSync( void )
{
    FormatAsync( );
    WaitForCompletion( );
}

void radInstanceDrive::CreateDirectorySync( const char* pDirectoryName )
{
    CreateDirectoryAsync( pDirectoryName );
    WaitForCompletion( );
}

void radInstanceDrive::DestroyDirectorySync( const char* pDirectoryName )
{
    DestroyDirectoryAsync( pDirectoryName );
    WaitForCompletion( );
}

void radInstanceDrive::FindFirstSync( const char* searchspec, DirectoryInfo* pDirectoryInfo )
{
    FindFirstAsync( searchspec, pDirectoryInfo );
    WaitForCompletion( );
}

void radInstanceDrive::FindNextSync( DirectoryInfo* pDirectoryInfo )
{
    FindNextAsync( pDirectoryInfo );
    WaitForCompletion( );
}

void radInstanceDrive::DestroyFileSync( const char* pFileName, bool simpleName )
{
    DestroyFileAsync( pFileName, simpleName );
    WaitForCompletion( );
}

//=============================================================================
// Function:    radInstanceDrive::CancelAsync
//=============================================================================
// Description: cancel all asynchronous operation which haven't started executing
//
// Parameters:  
//              
// Returns:     
//
// Notes:       cancels all appropriate requests at the current priority. If the
//              priority has changed before calling this, the previous requests
//              will not be cancelled.
//------------------------------------------------------------------------------

void radInstanceDrive::CancelAsync( void )
{
    m_pDrive->QueueRequest( new DriveCancelRequest( this ), m_Priority, true );

}

//=============================================================================
// Function:    radInstanceDrive::AddCompletionCallback
//=============================================================================
// Description: Add a callback
//
// Parameters:  pCallback
//              pUserData
//              
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::AddCompletionCallback( IRadDriveCompletionCallback* pCallback, void* pUserData )
{
    rAssert( pCallback != NULL );
    m_pDrive->QueueRequest( new DriveCompletionRequest( ::radThreadGetActiveThread( ), this, pCallback, pUserData ), m_Priority, false );
}

//=============================================================================
// Function:    radInstanceDrive::AddCompletionEvent
//=============================================================================
// Description: Add a semaphore
//
// Parameters:  pSemaphore
//              
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::AddCompletionEvent( IRadThreadSemaphore* pSemaphore )
{
    rAssert( pSemaphore != NULL );
    m_pDrive->QueueRequest( new CompletionEventRequest( pSemaphore, m_pDrive ), m_Priority, false );
}

//=============================================================================
// Function:    radInstanceDrive::CheckForCompletion
//=============================================================================
// Description: Check if there are any outstanding requests
//
// Parameters:  
//              
// Returns:     true if there are no outstanding requests  
//------------------------------------------------------------------------------

bool radInstanceDrive::CheckForCompletion( void )
{
    Lock( );
    bool result = ( m_OutstandingRequests == 0 );
    Unlock( );

    return result;
}

//=============================================================================
// Function:    radInstanceDrive::WaitForCompletion
//=============================================================================
// Description: Wait until there are no outstanding requests
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::WaitForCompletion( void )
{
    while ( !CheckForCompletion( ) )
    {
        m_pDrive->Service( );
        radThreadSleep(0);
    }
}

//=============================================================================
// Function:    radInstanceDrive::GetLastError
//=============================================================================
// Description: Return the error code from the last operation on this drive.
//              If we are in an error state, it gets the error from the physical
//              drive. So if we open a file without giving a drive, we can ask
//              any instance drive what the error was.
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

radFileError radInstanceDrive::GetLastError( void )
{
    if ( m_pDrive->CheckForErrorState( ) )
    {
        return m_pDrive->GetLastError( );
    }
    else
    {
        return Success;
    }
}

//=============================================================================
// Function:    radInstanceDrive::RegisterErrorHandler
//=============================================================================
// Description: Add a global callback.
//
// Parameters:  callback
//              pUserData
//
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::RegisterErrorHandler( IRadDriveErrorCallback* callback, void* pUserData )
{
    rAssert( callback != NULL );
    m_pDrive->RegisterErrorHandler( callback, pUserData );
}

//=============================================================================
// Function:    radInstanceDrive::UnregisterErrorHandler
//=============================================================================
// Description: Remove a global callback. Must be the same callback as is registered.
//
// Parameters:  callback
//
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::UnregisterErrorHandler( IRadDriveErrorCallback* callback )
{
    m_pDrive->UnregisterErrorHandler( callback );
}

//=============================================================================
// Function:    radInstanceDrive::RegisterErrorEvent
//=============================================================================
// Description: Add a semaphore to be signaled on error
//
// Parameters:  pSemaphore
//
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::RegisterErrorEvent( IRadThreadSemaphore* pSemaphore )
{
    rAssert( pSemaphore != NULL );
    m_pDrive->RegisterErrorEvent( pSemaphore );
}

//=============================================================================
// Function:    radInstanceDrive::RegisterErrorEvent
//=============================================================================
// Description: Add a semaphore to be signaled on error
//
// Parameters:  pSemaphore
//
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::UnregisterErrorEvent( IRadThreadSemaphore* pSemaphore )
{
    rAssert( pSemaphore != NULL );
    m_pDrive->RegisterErrorEvent( pSemaphore );
}

//=============================================================================
// Function:    radInstanceDrive::CheckForErrorState
//=============================================================================
// Description: Indicate if there is an error. Used for polling if behaviour is
//              Suspend.
//
// Parameters:  
//
// Returns:     true if there is an error state
//------------------------------------------------------------------------------

bool radInstanceDrive::CheckForErrorState( void )
{
    return m_pDrive->CheckForErrorState( );
}

//=============================================================================
// Function:    radInstanceDrive::ResumeRequest
//=============================================================================
// Description: Clear error condition
//
// Parameters:  retry -- if true the request is retried, otherwise it is failed
//
// Returns:     
//
// Notes:       this must be called if an error event is used.
//------------------------------------------------------------------------------

void radInstanceDrive::ResumeRequest( bool retry )
{
    m_pDrive->ResumeRequest( retry );
}

//=============================================================================
// Function:    radInstanceDrive::SetDefaultErrorBehaviour
//=============================================================================
// Description: Set the error behaviour
//
// Parameters:  behaviour
//
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::SetDefaultErrorBehaviour( ErrorBehaviour behaviour )
{
    m_pDrive->SetDefaultErrorBehaviour( behaviour );
}

//=============================================================================
// Function:    radInstanceDrive::SetErrorClearReporting
//=============================================================================
// Description: indicate if we invoke the global error handler after an error 
//              has cleared
//
// Parameters:  notifyOnErrorClear -- if true we re-enter on success
//
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::SetErrorClearReporting( bool notifyOnErrorClear )
{
    m_pDrive->SetErrorClearReporting( notifyOnErrorClear );
}

void radInstanceDrive::SetReadWriteGranularity( unsigned int Size )
{
}

unsigned int radInstanceDrive::GetReadWriteGranularity( void )
{
    return 0xFFFFF;
}

//=============================================================================
// Function:    radFileSystem::FileOpenAsync
//=============================================================================
// Description: Open a file asynchronously.
//
// Parameters:  lots
//
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::FileOpenAsync
( 
    IRadFile**          pIRadFile, 
    const char*         pFileName,
    bool                writeAccess,
    radFileOpenFlags    flags,
    radFilePriority     priority, 
    unsigned int        cacheSize,
    radMemoryAllocator  alloc,
    radMemorySpace      cacheSpace 
)
{
	rAssert( pFileName != NULL );
    rAssert( pIRadFile != NULL );

    rAssertMsg( strchr( pFileName, ':' ) == NULL, "radFileSystem: drive name included in IRadFile::FileOpen( )" );
    
    //
    // Process the filename to get the file name in canonical form
    //
    char fullFilename[ radFileFilenameMax + 1 ];
    char driveSpec[ radFileDrivenameMax + 1 ];
    const char* filePart = NULL;
    radFileSystem::ProcessFileName( pFileName, fullFilename, driveSpec, &filePart );

    //
    // Lets new up the file object.
    //
    *pIRadFile =
        new radFile
        (
            m_pDrive,
            filePart,
            writeAccess,
            flags,
            priority,
            cacheSize,
            alloc,
            cacheSpace
        );
}

//=============================================================================
// Function:    radFileSystem::FileOpenSync
//=============================================================================
// Description: Open a file synchronously.
//
// Parameters:  lots
//
// Returns:     
//------------------------------------------------------------------------------

void radInstanceDrive::FileOpenSync
( 
    IRadFile**          pIRadFile, 
    const char*         pFileName,
    bool                writeAccess,
    radFileOpenFlags    flags,
    radFilePriority     priority, 
    unsigned int        cacheSize,
    radMemoryAllocator  alloc,
    radMemorySpace      cacheSpace 
)
{
    FileOpenAsync( pIRadFile, pFileName, writeAccess, flags, priority, cacheSize, alloc, cacheSpace );
    (*pIRadFile)->WaitForCompletion( );
}

void radInstanceDrive::SaveGameOpenAsync
( 
    IRadFile**       pIRadFile, 
    const char*      pFileName,
    bool             writeAccess,
    radFileOpenFlags flags,
    radMemcardInfo*  memcardInfo,
    unsigned int     maxSize,
	bool			 simpleName,
    radFilePriority  priority
)
{
	rAssert( pFileName != NULL );
    rAssert( pIRadFile != NULL );

	if (simpleName==false)
	    rAssertMsg( strchr( pFileName, ':' ) == NULL, "radFileSystem: drive name included in IRadFile::FileOpen( )" );
    //
    // Process the filename to get the file name in canonical form
    //
    char fullFilename[ radFileFilenameMax + 1 ];
    char driveSpec[ radFileDrivenameMax + 1 ];
    const char* filePart = NULL;
    radFileSystem::ProcessFileName( pFileName, fullFilename, driveSpec, &filePart, simpleName );

    //
    // Lets new up the file object.
    //
    *pIRadFile =
        new radFile
        (
            m_pDrive,
            filePart,
            writeAccess,
            flags,
            memcardInfo,
            maxSize,
            priority
        );
}

void radInstanceDrive::SaveGameOpenSync
( 
    IRadFile**       pIRadFile, 
    const char*      pFileName,
    bool             writeAccess,
    radFileOpenFlags flags,
    radMemcardInfo*  memcardInfo,
    unsigned int     maxSize,
	bool			 simpleName,
    radFilePriority  priority
)
{
    SaveGameOpenAsync( pIRadFile, pFileName, writeAccess, flags, memcardInfo, maxSize, simpleName ,priority );
    (*pIRadFile)->WaitForCompletion( );
}

//=============================================================================
// Function:    radInstanceDrive::SetPriority
//=============================================================================
// Description: Set the priority of requests for this file
//
// Parameters:  priority
//              
// Returns:  
//------------------------------------------------------------------------------

void radInstanceDrive::SetPriority( radFilePriority priority )
{
    rAssert( priority <= LowPriority );
    rWarningMsg( m_OutstandingRequests == 0,
        "radFileSystem: changing the priority of a drive while there are outstanding requests." );

    m_Priority = priority;
}

//=============================================================================
// Function:    radInstanceDrive::GetPriority
//=============================================================================
// Description: Get the priority of requests for this file
//
// Parameters:  
//              
// Returns:     priority
//------------------------------------------------------------------------------

radFilePriority radInstanceDrive::GetPriority( void )
{
    return m_Priority;
}

//=============================================================================
// Function:    radInstanceDrive::GetPriority
//=============================================================================
// Description: Get the drive name.
//
// Parameters:  
//              
// Returns:     drive name
//------------------------------------------------------------------------------

const char* radInstanceDrive::GetDriveName( void )
{
    return m_pDrive->GetDriveName( );
}

//=============================================================================
// Function:    radInstanceDrive::GetCreationSize
//=============================================================================
// Description: Get the actual size for creation.
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

unsigned int radInstanceDrive::GetCreationSize( radMemcardInfo* memcardInfo, unsigned int size )
{
    return m_pDrive->GetCreationSize( memcardInfo, size );
}
