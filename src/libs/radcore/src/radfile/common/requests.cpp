//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        requests.cpp
//
// Subsystem:   Radical Drive System
//
// Description: This file contains the implementation of the various
//              request classes.
//
// Revisions:   
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <string.h>
#include <radthread.hpp>
#include "requests.hpp"
#include "instancedrive.hpp"
#include "file.hpp"

//=============================================================================
// Static Data Definitions
//=============================================================================

static IRadMemoryPool*  s_RequestPool = NULL;
static IRadThreadMutex* s_Mutex = NULL;

//=============================================================================
// Functions
//=============================================================================

//=============================================================================
// Function:    radRequestPoolInitialize
//=============================================================================
// Description: This routine is invoked to intialized the pool for allocating
//              request objects.
//
// Parameters:  maximum number of outstanding requests.
//
// Returns:     
//
//------------------------------------------------------------------------------

void radRequestPoolInitialize
( 
    unsigned int        maxOutstandingRequests, 
    radMemoryAllocator  alloc
)
{
    rAssertMsg( s_RequestPool == NULL, "radFileSystem: request pool already initialized" );

    radThreadCreateMutex( &s_Mutex, alloc );
    rAssert( s_Mutex != NULL );

    //
    // Create a memory pool for allocating request objects.
    //
    radMemoryCreatePool( &s_RequestPool, sizeof( radRequestUnion ), maxOutstandingRequests, 0, false, None, alloc, "radFileRequest" );
    rAssert( s_RequestPool != NULL );
}

//=============================================================================
// Function:    radRequestPoolTerminate
//=============================================================================
// Description: This routine is invoked to free the request pool. 
//
// Parameters: 
//
// Returns:    
//
//------------------------------------------------------------------------------

void radRequestPoolTerminate( void )
{
    rAssertMsg( s_RequestPool, "radFileSystem: request pool not initialized");

    radRelease( s_Mutex, NULL );
    s_Mutex = NULL;

    unsigned int elementSize;
    unsigned int numberAllocated;
    unsigned int numberFree;
   
    s_RequestPool->GetStatus( &elementSize, &numberFree, &numberAllocated );
    rAssertMsg( numberAllocated == 0, "radFileSystem: not all requests were destroyed.\n" );

    radRelease( s_RequestPool, NULL );
    s_RequestPool = NULL;
}

//=============================================================================
// Member Functions
//=============================================================================

//=============================================================================
// Function:    radRequest::operator new
//=============================================================================
// Description: This member is the placement new. Required since request objects
//              are allocated from a pool.
//
// Parameters:  size
//
// Returns:     address 
//
//------------------------------------------------------------------------------

void* radRequest::operator new( size_t size )
{
    (void) size;

    rAssertMsg( s_RequestPool, "radFileSystem: request pool not initialized");
    
    s_Mutex->Lock( );

    void* p = s_RequestPool->GetMemory( );
    rAssertMsg( p != NULL, "radFileSystem: increase number of outstanding requests" );

    s_Mutex->Unlock( );

    return( p );
}

//=============================================================================
// Function:    radRequest::operator delete
//=============================================================================
// Description: Return request to pool
//
// Parameters:  p
//
// Returns:    
//
//------------------------------------------------------------------------------

void radRequest::operator delete( void* p )
{
    rAssertMsg( s_RequestPool, "radFileSystem: request pool not initialized");

    s_Mutex->Lock( );

    s_RequestPool->FreeMemory( p );

    s_Mutex->Unlock( );
}

//=============================================================================
// Function:    radRequest::radRequest
//=============================================================================
// Description: Constructor for the base request. Simply initialized members
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

radRequest::radRequest( )
    :
    m_Next( NULL )
{
}

//=============================================================================
// Function:    radRequest::~radRequest
//=============================================================================
// Description: Destructor for the base request. Does nothing
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

radRequest::~radRequest( )
{
}

//=============================================================================
// CompletionEventRequest
//=============================================================================

CompletionEventRequest::CompletionEventRequest( IRadThreadSemaphore* pSema, radDrive* pDrive )
    : 
    m_pSema( pSema ),
    m_pDrive( pDrive )
{
    m_pSema->AddRef( );
    m_pDrive->AddRef( );
}

CompletionEventRequest::~CompletionEventRequest( )
{
    m_pSema->Release( );
    m_pDrive->Release( );
}

radDrive::CompletionStatus CompletionEventRequest::DoRequest( void )
{
    m_pSema->Signal( );

    return radDrive::Complete;
}

bool CompletionEventRequest::ShouldCancel( void* key )
{
    (void) key;
    return false;
}

radDrive* CompletionEventRequest::GetOwner( void )
{
    return m_pDrive;
}

radDrive::CompletionStatus CompletionEventRequest::ReInit( void )
{
    //
    // Completion event does not affect the drive.
    //
    return radDrive::Complete;
}

//=============================================================================
// CompletionCallbackRequest
//=============================================================================

CompletionCallbackRequest::CompletionCallbackRequest( IRadThread* pThread )
    : m_pThread( pThread )
{
}

bool CompletionCallbackRequest::ShouldCancel( void* key )
{
    (void) key;
    return false;
}

bool CompletionCallbackRequest::IsRegisteredThread( IRadThread* pThread )
{
    return true; // until p3d is threadsafe in this way ...
    //    return ( pThread == m_pThread );
}

radDrive::CompletionStatus CompletionCallbackRequest::ReInit( void )
{
    //
    // Completion callback does not affect the drive.
    //
    return radDrive::Complete;
}

//=============================================================================
// FileCompletionRequest
//=============================================================================
FileCompletionRequest::FileCompletionRequest
( 
    IRadThread* pThread, 
    radFile* pFile,
    IRadFileCompletionCallback* pCallback, 
    void* pUserData 
)
    : 
    CompletionCallbackRequest( pThread ),
    m_pFile( pFile ),
    m_Callback( pCallback ),
    m_UserData( pUserData )
{
    m_Callback->AddRef( );

    m_pFile->Lock( );
    m_pFile->m_OutstandingRequests++;
    m_pFile->Unlock( );

    m_pFile->AddRef( );

}

FileCompletionRequest::~FileCompletionRequest( )
{
    m_Callback->Release( );

    m_pFile->Release( );
}

radDrive::CompletionStatus FileCompletionRequest::DoRequest( void )
{
    m_pFile->m_pDrive->AddCallback( this );
    return radDrive::KeepAlive;
}

void FileCompletionRequest::callback( void )
{
    //
    // We are executing the callback, so it's not really outstanding.
    //
    m_pFile->Lock( );
    m_pFile->m_OutstandingRequests--;
    m_pFile->Unlock( );

    m_Callback->OnFileOperationsComplete( m_UserData );
}

radDrive* FileCompletionRequest::GetOwner( void )
{
    return m_pFile->m_pDrive;
}

//=============================================================================
// DriveCompletionRequest
//=============================================================================

DriveCompletionRequest::DriveCompletionRequest
( 
    IRadThread* pThread, 
    radInstanceDrive* pDrive,
    IRadDriveCompletionCallback* pCallback, 
    void* pUserData 
)
    : 
    CompletionCallbackRequest( pThread ),
    m_pInstDrive( pDrive ),
    m_Callback( pCallback ),
    m_UserData( pUserData )
{
    m_Callback->AddRef( );

    m_pInstDrive->Lock( );
    m_pInstDrive->m_OutstandingRequests++;
    m_pInstDrive->Unlock( );

    m_pInstDrive->AddRef( );
}

DriveCompletionRequest::~DriveCompletionRequest( )
{
    m_Callback->Release( );

    m_pInstDrive->Release( );
}

radDrive::CompletionStatus DriveCompletionRequest::DoRequest( void )
{
    m_pInstDrive->m_pDrive->AddCallback( this );
    return radDrive::KeepAlive;
}

void DriveCompletionRequest::callback( void )
{
    //
    // We are executing the callback, so it's not really outstanding.
    //
    m_pInstDrive->Lock( );
    m_pInstDrive->m_OutstandingRequests--;
    m_pInstDrive->Unlock( );

    m_Callback->OnDriveOperationsComplete( m_UserData );
}

radDrive* DriveCompletionRequest::GetOwner( void )
{
    return m_pInstDrive->m_pDrive;
}

//=============================================================================
// radFileRequest
//=============================================================================

radFileRequest::radFileRequest( radFile* pFile )
    : m_pFile( pFile )
{
    m_pFile->Lock( );
    m_pFile->m_OutstandingRequests++;
    m_pFile->Unlock( );

    m_pFile->AddRef( );
}

radFileRequest::~radFileRequest( void )
{
    m_pFile->Lock( );
    m_pFile->m_OutstandingRequests--;
    m_pFile->Unlock( );

    m_pFile->Release( );
}

radDrive::CompletionStatus radFileRequest::handleError
( 
    radDrive::CompletionStatus status,
    radFileError error
)
{
    m_pFile->Lock( );
    m_pFile->m_LastError = error;
    m_pFile->Unlock( );

    return status;
}

bool radFileRequest::ShouldCancel( void* key )
{
    return( key == (void*) m_pFile );
}

radDrive* radFileRequest::GetOwner( void )
{
    return m_pFile->m_pDrive;
}

radDrive::CompletionStatus radFileRequest::ReInit( void )
{
    radDrive* pDrive = m_pFile->m_pDrive;

    radDrive::CompletionStatus status = pDrive->Initialize( );
    return handleError( status, pDrive->GetLastError( ) );
}

const char* radFileRequest::GetFilename( void )
{
    return m_pFile->GetFilename();
}


//=============================================================================
// FileOpenRequest
//=============================================================================

FileOpenRequest::FileOpenRequest( radFile* pFile, radFileOpenFlags flags, bool writeAccess )
    : 
    radFileRequest( pFile ),
    m_Flags( flags ),
    m_WriteAccess( writeAccess )
{
}

radDrive::CompletionStatus FileOpenRequest::DoRequest( void )
{
    radDrive* pDrive = m_pFile->m_pDrive;

    //
    // We first search through all of the cement files on this drive.
    //
    radCementLibraryList* pLibList = pDrive->GetLibraryList( );
    pLibList->BeginIteration( radCementLibraryBeforeDrive );

    radCementLibrary* pLib = NULL;
    while ( ( pLib = pLibList->GetNext( ) ) != NULL )
    {
        unsigned int offset, size;
        radFile* origFile = pLib->OpenFile( m_pFile->m_Filename, &offset, &size );
        if ( origFile != NULL )
        {
            //
            // Found a match! Fill in the info.
            //
            rAssertMsg( m_pFile->m_WriteAccess == false, "Cement files are not writeable." );
            rAssertMsg( m_Flags != CreateAlways, "Cannot create file in a cement file." );

            origFile->AddRef( );
            m_pFile->m_pOriginalFile = origFile;
            m_pFile->m_Handle = origFile->m_Handle;
            m_pFile->m_Size = size;
            m_pFile->m_BaseOffset = offset;
            m_pFile->m_IsOpen = true;
            m_pFile->m_OptimalSize = origFile->m_OptimalSize;

            return handleError( radDrive::Complete, Success );
        }
    }

    //
    // If we got here, then the file was not found in a cement library.
    //
    radDrive::CompletionStatus status = pDrive->OpenFile
                                            ( 
                                                m_pFile->m_Filename,
                                                m_Flags,
                                                m_WriteAccess,
                                                &m_pFile->m_Handle,
                                                &m_pFile->m_Size
                                            );

    if ( status == radDrive::Complete )
    {
        m_pFile->m_IsOpen = true;
        m_pFile->m_OptimalSize = pDrive->GetMediaInfo( )->m_SectorSize;

        //
        // Finish initializing the file cache.
        //
        m_pFile->m_FileCache.DoInit( pDrive->GetMediaInfo( )->m_SectorSize, m_pFile->m_Filename );

        return handleError( radDrive::Complete, pDrive->GetLastError( ) );
    }

    //
    // Next search through all of the cement files after this drive.
    //
    pLibList->BeginIteration( radCementLibraryAfterDrive );

    pLib = NULL;
    while ( ( pLib = pLibList->GetNext( ) ) != NULL )
    {
        unsigned int offset, size;
        radFile* origFile = pLib->OpenFile( m_pFile->m_Filename, &offset, &size );
        if ( origFile != NULL )
        {
            //
            // Found a match! Fill in the info.
            //
            rAssertMsg( m_pFile->m_WriteAccess == false, "Cement files are not writeable." );
            rAssertMsg( m_Flags != CreateAlways, "Cannot create file in a cement file." );

            origFile->AddRef( );
            m_pFile->m_pOriginalFile = origFile;
            m_pFile->m_Handle = origFile->m_Handle;
            m_pFile->m_Size = size;
            m_pFile->m_BaseOffset = offset;
            m_pFile->m_IsOpen = true;
            m_pFile->m_OptimalSize = origFile->m_OptimalSize;

            return handleError( radDrive::Complete, Success );
        }
    }

    //
    // Return the status. This will be FileNotFound or a real error.
    //
    return handleError( status, pDrive->GetLastError( ) );
}

//=============================================================================
// SaveGameOpenRequest
//=============================================================================

SaveGameOpenRequest::SaveGameOpenRequest
( 
    radFile* pFile, 
    radFileOpenFlags flags, 
    bool writeAccess,
    radMemcardInfo* memcardInfo, 
    unsigned int maxSize
)
    : 
    radFileRequest( pFile ),
    m_Flags( flags ),
    m_WriteAccess( writeAccess ),
    m_MemcardInfo( memcardInfo ), 
    m_MaxSize( maxSize )

{
}

radDrive::CompletionStatus SaveGameOpenRequest::DoRequest( void )
{
    radDrive* pDrive = m_pFile->m_pDrive;
    radDrive::CompletionStatus status = pDrive->OpenSaveGame
                                            ( 
                                                m_pFile->m_Filename,
                                                m_Flags,
                                                m_WriteAccess,
                                                m_MemcardInfo,
                                                m_MaxSize,
                                                &m_pFile->m_Handle,
                                                &m_pFile->m_Size
                                            );
    if ( status == radDrive::Complete )
    {
        m_pFile->m_IsOpen = true;
        m_pFile->m_OptimalSize = pDrive->GetMediaInfo( )->m_SectorSize;
    }

    return handleError( status, pDrive->GetLastError( ) );
}

//=============================================================================
// FileCloseRequest
//=============================================================================

FileCloseRequest::FileCloseRequest( radFile* pFile )
    : radFileRequest( pFile )
{
}

radDrive::CompletionStatus FileCloseRequest::ReInit( void )
{
    //
    // Closing a file should not cause a new error, so don't re-init!
    //
    return radDrive::Complete;
}

radDrive::CompletionStatus FileCloseRequest::DoRequest( void )
{
    radDrive* pDrive = m_pFile->m_pDrive;

    rAssertMsg( m_pFile->m_IsOpen, rNullStringWrap( m_pFile->m_Filename ) );

    //
    // If this is a cement file, then we don't actually close the handle.
    // In either case this shouldn't cause an error.
    //
    if ( m_pFile->m_pOriginalFile == NULL )
    {
        pDrive->CloseFile( m_pFile->m_Handle, m_pFile->m_Filename );
    }
    else
    {
        m_pFile->m_pOriginalFile->Release( );
        m_pFile->m_pOriginalFile = NULL;
    }

    m_pFile->m_IsOpen = false;


    return handleError( radDrive::Complete, Success );
}

//=============================================================================
// FileCommitRequest
//=============================================================================

FileCommitRequest::FileCommitRequest( radFile* pFile )
    : radFileRequest( pFile )
{
}

radDrive::CompletionStatus FileCommitRequest::DoRequest( void )
{
    radDrive* pDrive = m_pFile->m_pDrive;

    rAssertMsg( m_pFile->m_IsOpen, rNullStringWrap( m_pFile->m_Filename ) );

    radDrive::CompletionStatus status = pDrive->CommitFile( m_pFile->m_Handle, m_pFile->m_Filename );

    return handleError( status, pDrive->GetLastError( ) );
}

//=============================================================================
// FileReadRequest
//=============================================================================

FileReadRequest::FileReadRequest
( 
    radFile* pFile, 
    void* pData, 
    unsigned int bytesToRead, 
    radMemorySpace pBufferSpace
)
    :
    radFileRequest( pFile ),
    m_pData( pData ),
    m_BytesToRead( bytesToRead ),
    m_pDataSpace( pBufferSpace )
{
}

radDrive::CompletionStatus FileReadRequest::DoRequest( void )
{
    rAssertMsg( m_pFile->m_IsOpen, rNullStringWrap( m_pFile->m_Filename ) );

    radDrive* pDrive = m_pFile->m_pDrive;

    unsigned int fileSize = m_pFile->m_Size;

    //
    // Cap the values to read to the end of the file, if necessary
    //
    unsigned int bytesToRead = m_BytesToRead;

    if ( m_pFile->m_Position >= fileSize )
    {
        bytesToRead = 0;
    }

    //
    // Check if we read past the last sector of the file and cap it.
    //
    unsigned int fileEnd = ::radMemoryRoundUp( fileSize, pDrive->GetMediaInfo( )->m_SectorSize );
    if ( m_pFile->m_Position + bytesToRead > fileEnd )
    {
        bytesToRead = fileEnd - m_pFile->m_Position;

        rDebugPrintf( "radFile: attempting to read beyond the last sector of file [%s].\n"
                      "radFile: capping read to size [%u].\n", 
                      m_pFile->GetFilename( ), bytesToRead );
    }

    radDrive::CompletionStatus status;

    //
    // Read the file and update the position
    //
    if ( bytesToRead > 0 )
    {
        unsigned int bytesRead;

        //
        // Check if we should use the file cache.
        //
        if ( m_pFile->m_FileCache.ShouldUseCache( m_pFile->m_Position + m_pFile->m_BaseOffset, bytesToRead ) )
        {
            status =
                m_pFile->m_FileCache.Read
                (
                    pDrive, 
                    m_pFile->m_Handle, 
                    m_pFile->m_Size + m_pFile->m_BaseOffset,
                    m_pFile->m_Size,
                    m_pFile->m_Filename,
                    m_pFile->m_BufferedState,
                    m_pFile->m_Position + m_pFile->m_BaseOffset, 
                    m_pData, 
                    bytesToRead, 
                    &bytesRead, 
                    m_pDataSpace
                );
        }
        else
        {
            status =         
                pDrive->ReadFile
                ( 
                    m_pFile->m_Handle,
                    m_pFile->m_Filename,
                    m_pFile->m_BufferedState,
                    m_pFile->m_Position + m_pFile->m_BaseOffset,
                    m_pData, 
                    bytesToRead, 
                    &bytesRead, 
                    m_pDataSpace
                );
        }

        if ( status == radDrive::Complete )
        {
            m_pFile->m_Position += bytesRead; // or up to end of the file.
        }
    }
    else
    {
        status = radDrive::Complete;
    }

    return handleError( status, pDrive->GetLastError( ) );
}

//=============================================================================
// FileWriteRequest
//=============================================================================

FileWriteRequest::FileWriteRequest
( 
    radFile* pFile, 
    const void* pData, 
    unsigned int bytesToWrite, 
    radMemorySpace pDataSpace 
)
    :
    radFileRequest( pFile ),
    m_pData( pData ),
    m_BytesToWrite( bytesToWrite ),
    m_pDataSpace( pDataSpace )
{
}

radDrive::CompletionStatus FileWriteRequest::DoRequest( void )
{
    rAssertMsg( m_pFile->m_IsOpen, rNullStringWrap( m_pFile->m_Filename ) );
    rAssertMsg( m_pFile->m_BaseOffset == 0, "Cement files do not support writing." );

    radDrive* pDrive = m_pFile->m_pDrive;
    radDrive::CompletionStatus status;

    if ( m_BytesToWrite > 0 )
    {
        unsigned int bytesWritten;

        m_pFile->m_FileCache.InvalidateCache( m_pFile->m_Position, m_pFile->m_Size );

        status = pDrive->WriteFile
        ( 
            m_pFile->m_Handle,
            m_pFile->m_Filename,
            m_pFile->m_BufferedState,
            m_pFile->m_Position,
            m_pData, 
            m_BytesToWrite, 
            &bytesWritten,
            &m_pFile->m_Size,
            m_pDataSpace
        );

        if ( status == radDrive::Complete )
        {
            //
            // Check if it really succeeded
            //
            rAssert( bytesWritten == m_BytesToWrite );
            m_pFile->m_Position += bytesWritten;
        }
    }
    else
    {
        status = radDrive::Complete;
    }

    return handleError( status, pDrive->GetLastError( ) );
}

//=============================================================================
// FileSetPositionRequest
//=============================================================================

FileSetPositionRequest::FileSetPositionRequest( radFile* pFile, unsigned int position )
    :
    radFileRequest( pFile ),
    m_Position( position )
{
}

radDrive::CompletionStatus FileSetPositionRequest::DoRequest( void )
{
    rAssertMsg( m_pFile->m_IsOpen, rNullStringWrap( m_pFile->m_Filename ) );
    rAssertMsg( m_Position <= m_pFile->m_Size, "radFileSystem: setting file pointer past end of file." );

    m_pFile->m_Position = m_Position;
    return handleError( radDrive::Complete, Success );
}

//=============================================================================
// FileGetPositionRequest
//=============================================================================

FileGetPositionRequest::FileGetPositionRequest( radFile* pFile, unsigned int* position )
    :
    radFileRequest( pFile ),
    m_pPosition( position )
{
}

radDrive::CompletionStatus FileGetPositionRequest::DoRequest( void )
{
    rAssertMsg( m_pFile->m_IsOpen, rNullStringWrap( m_pFile->m_Filename ) );

    *m_pPosition = m_pFile->m_Position;
    return handleError( radDrive::Complete, Success );
}

//=============================================================================
// FileGetSizeRequest
//=============================================================================

FileGetSizeRequest::FileGetSizeRequest( radFile* pFile, unsigned int* pSize )
    :
    radFileRequest( pFile ),
    m_pSize( pSize )
{
}

radDrive::CompletionStatus FileGetSizeRequest::DoRequest( void )
{
    rAssertMsg( m_pFile->m_IsOpen, rNullStringWrap( m_pFile->m_Filename ) );

    *m_pSize = m_pFile->m_Size;
    return handleError( radDrive::Complete, Success );
}

//=============================================================================
// FileCancelRequest
//=============================================================================

FileCancelRequest::FileCancelRequest( radFile* pFile )
    : radFileRequest( pFile )
{
}

radDrive::CompletionStatus FileCancelRequest::DoRequest( void )
{
    radDrive* pDrive = m_pFile->m_pDrive;

    pDrive->CancelRequests( m_pFile, m_pFile->m_Priority );
    return handleError( radDrive::Complete, Success );
}

radDrive::CompletionStatus FileCancelRequest::ReInit( void )
{
    //
    // Cancelling should not affect drive status.
    //
    return radDrive::Complete;
}

//=============================================================================
// radDriveRequest
//=============================================================================

radDriveRequest::radDriveRequest( radInstanceDrive* pInstDrive )
    : m_pInstDrive( pInstDrive )
{
    m_pInstDrive->Lock( );
    m_pInstDrive->m_OutstandingRequests++;
    m_pInstDrive->Unlock( );

    m_pInstDrive->AddRef( );
}

radDriveRequest::~radDriveRequest( void )
{
    m_pInstDrive->Lock( );
    m_pInstDrive->m_OutstandingRequests--;
    m_pInstDrive->Unlock( );

    m_pInstDrive->Release( );
}
    
bool radDriveRequest::ShouldCancel( void* key )
{
    return( key == (void*) m_pInstDrive );
}

radDrive* radDriveRequest::GetOwner( void )
{
    return m_pInstDrive->m_pDrive;
}

radDrive::CompletionStatus radDriveRequest::ReInit( void )
{
    radDrive* pDrive = m_pInstDrive->m_pDrive;

    return pDrive->Initialize( );
}

//=============================================================================
// DriveOpenRequest
//=============================================================================

DriveOpenRequest::DriveOpenRequest( radInstanceDrive* pInstDrive )
    : radDriveRequest( pInstDrive )
{
}

radDrive::CompletionStatus DriveOpenRequest::DoRequest( void )
{
    radDrive* pDrive = m_pInstDrive->m_pDrive;

    return pDrive->Initialize( );
}

radDrive::CompletionStatus DriveOpenRequest::ReInit( void )
{
    //
    // Don't need to initialize twice!
    //
    return radDrive::Complete;
}

//=============================================================================
// DriveMediaRequest
//=============================================================================

DriveMediaRequest::DriveMediaRequest
( 
    radInstanceDrive* pInstDrive, 
    IRadDrive::MediaInfo* pMediaInfo 
)
    : 
    radDriveRequest( pInstDrive ),
    m_pMediaInfo( pMediaInfo )
{
}

radDrive::CompletionStatus DriveMediaRequest::DoRequest( void )
{
    radDrive* pDrive = m_pInstDrive->m_pDrive;

    //
    // Explicitly re-init the drive!
    //
    radDrive::CompletionStatus status = pDrive->Initialize( );

    *m_pMediaInfo = *pDrive->GetMediaInfo( );
    
    return status;
}

radDrive::CompletionStatus DriveMediaRequest::ReInit( void )
{
    //
    // Don't need to initialize twice!
    //
    return radDrive::Complete;
}

//=============================================================================
// DriveFormatRequest
//=============================================================================

DriveFormatRequest::DriveFormatRequest( radInstanceDrive* pInstDrive )
    : radDriveRequest( pInstDrive )
{
}

radDrive::CompletionStatus DriveFormatRequest::DoRequest( void )
{
    radDrive* pDrive = m_pInstDrive->m_pDrive;

    return pDrive->Format( );
}

//=============================================================================
// DriveCreateDirectoryRequest
//=============================================================================

DriveCreateDirectoryRequest::DriveCreateDirectoryRequest
( 
    radInstanceDrive* pInstDrive, 
    const char* pDirName 
)
    : radDriveRequest( pInstDrive )
{
    strncpy( m_pDirName, pDirName, radFileDrivenameMax );
    m_pDirName[ radFileDrivenameMax ] = '\0';
}

radDrive::CompletionStatus DriveCreateDirectoryRequest::DoRequest( void )
{
    radDrive* pDrive = m_pInstDrive->m_pDrive;

    return pDrive->CreateDir( m_pDirName );
}

//=============================================================================
// DriveDestroyDirectoryRequest
//=============================================================================

DriveDestroyDirectoryRequest::DriveDestroyDirectoryRequest
( 
    radInstanceDrive* pInstDrive,
    const char* pDirName 
)
    : 
    radDriveRequest( pInstDrive )
{
    strncpy( m_pDirName, pDirName, radFileDrivenameMax );
    m_pDirName[ radFileDrivenameMax ] = '\0';

}

radDrive::CompletionStatus DriveDestroyDirectoryRequest::DoRequest( void )
{
    radDrive* pDrive = m_pInstDrive->m_pDrive;

    return pDrive->DestroyDir( m_pDirName );
}

//=============================================================================
// DriveFindFirstRequest
//=============================================================================

DriveFindFirstRequest::DriveFindFirstRequest
( 
    radInstanceDrive* pInstDrive, 
    const char* searchspec, 
    IRadDrive::DirectoryInfo* pDirectoryInfo 
)
    : 
    radDriveRequest( pInstDrive ),
    m_pDirInfo( pDirectoryInfo )
{
    strncpy( m_pSearchSpec, searchspec, radFileFilenameMax );
    m_pSearchSpec[ radFileFilenameMax ] ='\0';
}

radDrive::CompletionStatus DriveFindFirstRequest::DoRequest( void )
{
    radDrive* pDrive = m_pInstDrive->m_pDrive;

    radDrive::CompletionStatus status = 
        pDrive->FindFirst( m_pSearchSpec, m_pDirInfo, &m_pInstDrive->m_DirHandle, !m_pInstDrive->m_DirSearch );

    if ( status == radDrive::Complete )
    {
        m_pInstDrive->m_DirSearch = true;
    }
    else
    {
        m_pInstDrive->m_DirSearch = false;
    }
    return status;
}

//=============================================================================
// DriveFindNextRequest
//=============================================================================

DriveFindNextRequest::DriveFindNextRequest
( 
    radInstanceDrive* pInstDrive,
    IRadDrive::DirectoryInfo* pDirectoryInfo 
)
    : 
    radDriveRequest( pInstDrive ),
    m_pDirInfo( pDirectoryInfo )
{
}

radDrive::CompletionStatus DriveFindNextRequest::DoRequest( void )
{
    radDrive* pDrive = m_pInstDrive->m_pDrive;

    rAssert( m_pInstDrive->m_DirSearch == true );
    return pDrive->FindNext( &m_pInstDrive->m_DirHandle, m_pDirInfo );
}

//=============================================================================
// DriveFindCloseRequest
//=============================================================================

DriveFindCloseRequest::DriveFindCloseRequest( radInstanceDrive* pInstDrive )
    : radDriveRequest( pInstDrive )
{
}

radDrive::CompletionStatus DriveFindCloseRequest::DoRequest( void )
{
    radDrive* pDrive = m_pInstDrive->m_pDrive;

    m_pInstDrive->m_DirSearch = false;
    pDrive->FindClose( &m_pInstDrive->m_DirHandle );
    return radDrive::Complete;
}

radDrive::CompletionStatus DriveFindCloseRequest::ReInit( void )
{
    //
    // Closing a search handle should not cause a new error, so don't re-init!
    //
    return radDrive::Complete;
}

//=============================================================================
// DriveFileDestroyRequest
//=============================================================================

DriveFileDestroyRequest::DriveFileDestroyRequest
( 
    radInstanceDrive* pInstDrive, 
    const char* pFilename 
)
    : radDriveRequest( pInstDrive )
{
    strncpy( m_pFileName, pFilename, radFileFilenameMax );
    m_pFileName[ radFileFilenameMax ] = '\0';
}

radDrive::CompletionStatus DriveFileDestroyRequest::DoRequest( void )
{
    radDrive* pDrive = m_pInstDrive->m_pDrive;

    return pDrive->DestroyFile( m_pFileName );
}

//=============================================================================
// DriveCancelRequest
//=============================================================================

DriveCancelRequest::DriveCancelRequest( radInstanceDrive* pInstDrive )
    : radDriveRequest( pInstDrive )
{
}

radDrive::CompletionStatus DriveCancelRequest::DoRequest( void )
{
    radDrive* pDrive = m_pInstDrive->m_pDrive;

    pDrive->CancelRequests( m_pInstDrive, m_pInstDrive->m_Priority );
    return radDrive::Complete;
}

radDrive::CompletionStatus DriveCancelRequest::ReInit( void )
{
    //
    // Cancelling does not affect the drive.
    //
    return radDrive::Complete;
}
