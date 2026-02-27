//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        drive.cpp
//
// Subsystem:	Radical Drive System
//
// Description:	This file contains all definitions and classes relevant to
//              the physical drive base class. All real drives derive from
//              this base class.
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
#include "drive.hpp"
#include "requests.hpp"
#include "drivethread.hpp"
#include "cementer.hpp"
#include "file.hpp"

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    radDrive::radDrive
//=============================================================================
// Description: Constructor for the drive. Initialize things.
//
// Parameters:  drive name
//              
// Returns:     
//------------------------------------------------------------------------------

radDrive::radDrive( )
    :
    radSafeRefCount( 1 ),
    m_CallbackHead( NULL ),
    m_CallbackTail( NULL ),
    m_ErrorBehaviour( IRadDrive::Fail ),
    m_pErrorUserSemaphore( NULL ),
    m_pErrorThread( NULL ),
    m_pErrorCallback( NULL ),
    m_pUserData( NULL ),
    m_InError( false ),
    m_NotifyClear( true ),
    m_InCallback( false ),
    m_LastError( WrongMedia ), // make certain that we get initialized with the current disc
    m_pDriveThread( NULL )
{
#ifdef RAD_DEBUG
    m_pNumRefFiles = 0;
    m_pNumRefDrives = 0;
#endif
}

//=============================================================================
// Function:    radDrive::~radDrive
//=============================================================================
// Description: This routine is invoked to clean-up the drive.
//
// Parameters:  
//
// Returns:     
//
//------------------------------------------------------------------------------

radDrive::~radDrive( )
{
    rAssertMsg( m_CallbackHead == NULL, "radFileSystem: destroying drive with outstanding callbacks." );
}

//=============================================================================
// Function:    radDrive::GetLastError
//=============================================================================
// Description: Return the last error.
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

radFileError radDrive::GetLastError( void )
{
    Lock( );
    radFileError lastError = m_LastError;
    Unlock( );
    return lastError;
}

//=============================================================================
// Function:    radDrive::GetMediaInfo
//=============================================================================
// Description: Return the last error.
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

const IRadDrive::MediaInfo* radDrive::GetMediaInfo( )
{
    return &m_MediaInfo;
}

//=============================================================================
// Function:    radDrive::AddCallback
//=============================================================================
// Description: Add a callback to the service callback queue.
//
// Parameters:  pCallback
//
// Returns:     
//------------------------------------------------------------------------------

void radDrive::AddCallback( CompletionCallbackRequest* pCallback )
{
    Lock( );
    
    if ( m_CallbackHead == NULL )
    {
        m_CallbackHead = pCallback;
    }
    else
    {
        m_CallbackTail->m_Next = pCallback;
    }

    pCallback->m_Next = NULL;
    m_CallbackTail = pCallback;

    Unlock( );
}

//=============================================================================
// Function:    radDrive::Service
//=============================================================================
// Description: Call any callbacks which are queued for this thread.
//
// Parameters:
//
// Returns:     
//------------------------------------------------------------------------------

void radDrive::Service( void )
{
    //
    // Get the active thread and go through the callback queue
    //
    IRadThread* pThread = ::radThreadGetActiveThread( );

    Lock( );
    
    CompletionCallbackRequest** ppCallback = &m_CallbackHead;
    CompletionCallbackRequest*  pEnd = NULL;

    while ( *ppCallback != NULL )
    {
        if ( (*ppCallback)->IsRegisteredThread( pThread ) )
        {
            //
            // Take the callback off the list so it doesn't get called twice.
            //
            CompletionCallbackRequest* pCallback = *ppCallback;
            *ppCallback = ( CompletionCallbackRequest* ) pCallback->m_Next;
            
            //
            // Unlock for the actual callback, so we don't deadlock
            //
            Unlock( );
            pCallback->callback( );
            Lock( );

            delete pCallback;
        }
        else
        {
            pEnd = *ppCallback;
            CompletionCallbackRequest* pCallback = ( CompletionCallbackRequest* ) (*ppCallback)->m_Next;
            ppCallback = &pCallback;
        }
    }

    m_CallbackTail = pEnd;
    
    //
    // Global Error handler
    //
    if ( m_InError == true && m_pErrorCallback != NULL )
    {
        //
        // If we are in a callback, don't call it again!
        //
        if ( m_pErrorThread == ::radThreadGetActiveThread( ) && !m_InCallback )
        {
            m_InCallback = true;
            Unlock( );

            if ( m_pUserData == NULL )
            {
                ResumeRequest( 
                    m_pErrorCallback->OnDriveError( m_LastError, GetDriveName( ), (void*)(m_pDriveThread->GetCurrentRequest()) )
                    );
            }
            else
            {
                ResumeRequest( 
                    m_pErrorCallback->OnDriveError( m_LastError, GetDriveName( ), m_pUserData )
                    );
            }

            m_InCallback = false;
        }
        else
        {
            Unlock( );
        }
    }
    else
    {
        Unlock( );
    }
}

//=============================================================================
// Function:    radDrive::RegisterErrorHandler
//=============================================================================
// Description: Add a global callback.
//
// Parameters:  callback
//              pUserData
//
// Returns:     
//
// Notes:       sets behaviour to Suspend
//------------------------------------------------------------------------------

void radDrive::RegisterErrorHandler( IRadDriveErrorCallback* callback, void* pUserData )
{
    rAssert( m_pErrorCallback == NULL );

    Lock( );
    m_pErrorThread = ::radThreadGetActiveThread( );
    m_pErrorCallback = callback;
    m_pUserData = pUserData;
    m_ErrorBehaviour = IRadDrive::Suspend;
    Unlock( );
}

//=============================================================================
// Function:    radDrive::UnregisterErrorHandler
//=============================================================================
// Description: Remove the global callback. Must be the same callback as is registered.
//
// Parameters:  callback
//
// Returns:     
//
// Notes:       sets behaviour to Fail
//------------------------------------------------------------------------------

void radDrive::UnregisterErrorHandler( IRadDriveErrorCallback* callback )
{
    rAssertMsg( m_pErrorCallback == callback, "radFileSystem: cannot unregister a different callback." );

    Lock( );
    m_pErrorCallback = NULL;
    m_pErrorThread = NULL;
    m_ErrorBehaviour = IRadDrive::Fail;
    Unlock( );
}

//=============================================================================
// Function:    radDrive::RegisterErrorEvent
//=============================================================================
// Description: Add a semaphore to be signaled on error
//
// Parameters:  pSemaphore
//
// Returns:     
//
// Notes:       sets behaviour to Suspend
//------------------------------------------------------------------------------

void radDrive::RegisterErrorEvent( IRadThreadSemaphore* pSemaphore )
{
    rAssert( m_pErrorUserSemaphore == NULL );

    Lock( );
    m_pErrorUserSemaphore = pSemaphore;
    m_ErrorBehaviour = IRadDrive::Suspend;
    Unlock( );
}

//=============================================================================
// Function:    radDrive::UnregisterErrorEvent
//=============================================================================
// Description: remove the global error semaphore. Must be the same one as registered.
//
// Parameters:  pSemaphore
//
// Returns:     
//
// Notes:       sets behaviour to Fail
//------------------------------------------------------------------------------

void radDrive::UnregisterErrorEvent( IRadThreadSemaphore* pSemaphore )
{
    rAssertMsg( m_pErrorUserSemaphore == pSemaphore, "radFileSystem: cannot unregister a different event." );

    Lock( );
    m_pErrorUserSemaphore = pSemaphore;
    m_ErrorBehaviour = IRadDrive::Fail;
    Unlock( );
}

//=============================================================================
// Function:    radDrive::SetDefaultErrorBehaviour
//=============================================================================
// Description: Set the error behaviour
//
// Parameters:  behaviour
//
// Returns:     
//------------------------------------------------------------------------------

void radDrive::SetDefaultErrorBehaviour( IRadDrive::ErrorBehaviour behaviour )
{
    Lock( );
    m_ErrorBehaviour = behaviour;
    Unlock( );
}

//=============================================================================
// Function:    radDrive::GetDefaultErrorBehaviour
//=============================================================================

IRadDrive::ErrorBehaviour radDrive::GetDefaultErrorBehaviour( void )
{
    Lock( );
    IRadDrive::ErrorBehaviour errorBehaviour = m_ErrorBehaviour;
    Unlock( );
    return errorBehaviour;
}

//=============================================================================
// Function:    radDrive::ResumeRequest
//=============================================================================
// Description: Clear error condition
//
// Parameters:  retry -- if true the request is retried, otherwise it is failed
//
// Returns:     
//
// Notes:       this must be called if an error event is used.
//------------------------------------------------------------------------------

void radDrive::ResumeRequest( bool retry )
{
    m_InError = false;
    m_pDriveThread->ResumeRequest( retry );
}

//=============================================================================
// Function:    radDrive::CheckForErrorState
//=============================================================================
// Description: Indicate if there is an error. Used for polling if behaviour is
//              Suspend.
//
// Parameters:  
//
// Returns:     true if there is an error state
//------------------------------------------------------------------------------

bool radDrive::CheckForErrorState( void )
{
    return m_InError;
}

//=============================================================================
// Function:    radDrive::SetErrorState
//=============================================================================

void radDrive::SetErrorState( void )
{
    m_InError = true;
    Lock( );
    if ( m_pErrorUserSemaphore != NULL )
    {
        Unlock( );
        m_pErrorUserSemaphore->Signal( );
    }
    else
    {
        Unlock( );
    }
}

//=============================================================================
// Function:    radDrive::SetErrorClearReporting
//=============================================================================
// Description: indicate if we invoke the global error handler after an error 
//              has cleared
//
// Parameters:  notifyOnErrorClear -- if true we re-enter on success
//
// Returns:     
//------------------------------------------------------------------------------

void radDrive::SetErrorClearReporting( bool notifyOnErrorClear )
{
    m_NotifyClear = notifyOnErrorClear;
}

//=============================================================================
// Function:    radDrive::SetErrorClearReporting
//=============================================================================

bool radDrive::GetErrorClearReporting( void )
{
    return m_NotifyClear;
}

//=============================================================================
// Function:    radDrive::QueueRequest
//=============================================================================

void radDrive::QueueRequest
( 
    radRequest*          pRequest,
    radFilePriority      priority,
    bool                 toHead
)     
{
    m_pDriveThread->QueueRequest( pRequest, priority, toHead );
}

//=============================================================================
// Function:    radDrive::CancelRequests
//=============================================================================

void radDrive::CancelRequests( void* key, radFilePriority priority )
{
    m_pDriveThread->CancelRequests( key, priority );
}

//=============================================================================
// Function:    radDrive::OutstandingRequests
//=============================================================================

bool radDrive::OutstandingRequests( void )
{
    return m_pDriveThread->OutstandingRequests( );
}

//=============================================================================
// Function:    radDrive::OutstandingCallbacks
//=============================================================================

bool radDrive::OutstandingCallbacks( void )
{
    return ( m_CallbackHead != NULL );
}

//=============================================================================
// Function:    radDrive::RegisterCementLibrary
//=============================================================================

void radDrive::RegisterCementLibrary( radCementLibrary* pLib )
{
    m_LibraryList.AddLibrary( pLib );
}

//=============================================================================
// Function:    radDrive::UnregisterCementLibrary
//=============================================================================
void radDrive::UnregisterCementLibrary( radCementLibrary* pLib )
{
    m_LibraryList.RemoveLibrary( pLib );
}

//=============================================================================
// Function:    radDrive::GetLibraryList
//=============================================================================

radCementLibraryList* radDrive::GetLibraryList( void )
{
    return &m_LibraryList;
}

//=============================================================================
// Function:    radDrive::GetCreationSize
//=============================================================================
// Description: Get the actual size for creation.
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

unsigned int radDrive::GetCreationSize( radMemcardInfo* memcardInfo, unsigned int size )
{
    return size;
}

//=============================================================================
// Basic operations which don't need to be supported.
//=============================================================================

radDrive::CompletionStatus radDrive::CommitFile( radFileHandle handle, const char* fileName )
{
    return Complete;
}

radDrive::CompletionStatus radDrive::OpenFile( const char* fileName, 
                                       radFileOpenFlags flags, 
                                       bool writeAccess, 
                                       radFileHandle* pHandle, 
                                       unsigned int* pSize )
{
    rWarningMsg( false, "This drive does not support the OpenFile function." );
    return( Error );
}

radDrive::CompletionStatus radDrive::OpenSaveGame( const char* fileName, 
                                       radFileOpenFlags flags, 
                                       bool writeAccess,
                                       radMemcardInfo* memcardInfo,
                                       unsigned int maxSize,
                                       radFileHandle* pHandle, 
                                       unsigned int* pSize )
{
    rWarningMsg( false, "This drive does not support the OpenSaveGame function." );
    return( Error );
}

radDrive::CompletionStatus radDrive::WriteFile( radFileHandle handle,
                            const char* fileName,
                            IRadFile::BufferedReadState state,
                            unsigned int position, 
                            const void* pData, 
                            unsigned int bytesToWrite, 
                            unsigned int* bytesWritten, 
                            unsigned int* size, 
                            radMemorySpace pDataSpace )
{
    rWarningMsg( false, "This drive does not support the WriteFile function." );
    return( Error );
}

radDrive::CompletionStatus radDrive::Format( void )
{
    rWarningMsg( false, "This drive does not support the Format function." );
    return( Error );
}

radDrive::CompletionStatus radDrive::CreateDir( const char* pName )
{
    rWarningMsg( false, "This drive does not support the CreateDir function." );
    return( Error );
}

radDrive::CompletionStatus radDrive::DestroyDir( const char* pName )
{
    rWarningMsg( false, "This drive does not support the DestroyDir function." );
    return( Error );
}

radDrive::CompletionStatus radDrive::DestroyFile( const char* filename )
{
    rWarningMsg( false, "This drive does not support the DestroyFile function." );
    return( Error );
}

radDrive::CompletionStatus radDrive::FindFirst
( 
    const char* searchSpec, IRadDrive::DirectoryInfo* pDirectoryInfo, radFileDirHandle* pHandle, bool firstSearch 
)
{
    rWarningMsg( false, "This drive does not support the FindFirst function." );
    return( Error );
}

radDrive::CompletionStatus radDrive::FindNext( radFileDirHandle* pHandle, IRadDrive::DirectoryInfo* pDirectoryInfo )
{
    rWarningMsg( false, "This drive does not support the FindNext function." );
    return( Error );
}

radDrive::CompletionStatus radDrive::FindClose( radFileDirHandle* pHandle )
{
    rWarningMsg( false, "This drive does not support the FindClose function." );
    return( Complete );
}
