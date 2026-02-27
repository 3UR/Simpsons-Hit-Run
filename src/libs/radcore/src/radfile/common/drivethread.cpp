//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include "drivethread.hpp"
#include "requests.hpp"

//=============================================================================
// Static Member Functions
//=============================================================================

//=============================================================================
// Function:    radDriveThread::DriveThreadEntry
//=============================================================================
// Description: Entry point for the thread. It reads requests and executes them.
//              If there are no requests it suspends and waits for the next one.
//
// Parameters:  userData -- assumed to be a pointer to the drive instance
//              
// Returns:     
//------------------------------------------------------------------------------

unsigned int radDriveThread::DriveThreadEntry( void* userData )
{
    radDriveThread* parent = ( radDriveThread* ) userData;
    bool errorRetry = false; // are we retrying an error

    while ( true )
    {
        //
        // Suspend (requests will wake us). If we wake up check if we should end.
        //
        parent->m_pSema->Wait( );
        if ( parent->m_TerminateThread )
        {
            break;
        }

        //
        // Process request
        //
        radFilePriority priority;
        radRequest* request = parent->NextRequest( &priority );
        rAssertMsg( request != NULL, "radDriveThread: thread synchronization error" );

        parent->SetCurrentRequest( request );
        
        radDrive::CompletionStatus completionStatus;

        //
        // Get the drive
        //
        radDrive* drive = request->GetOwner( );

        if ( drive->GetLastError( ) != Success )
        {
            //
            // re-init the drive
            //
            completionStatus = request->ReInit( );
            if ( completionStatus == radDrive::Complete )
            {
                completionStatus = request->DoRequest( );
            }
        }
        else
        {
            completionStatus = request->DoRequest( );
        }

        //
        // First check if we need to indicate that an error has cleared:
        //
        if ( errorRetry == true && 
             drive->GetLastError( ) == Success &&
             drive->GetDefaultErrorBehaviour( ) == IRadDrive::Suspend && 
             drive->GetErrorClearReporting( ) == true  )
        {
            parent->WaitGlobalError( drive );
            errorRetry = false;
        }
        
        switch( completionStatus )
        {
        case radDrive::Complete:
            delete request;
            parent->SetCurrentRequest( NULL );
            break;

        case radDrive::Error: 
            switch( drive->GetDefaultErrorBehaviour( ) )
            {
            case IRadDrive::Fail:
                rDebugPrintf( "radFileError: [%d]. Failing operation. For additional info install an error handler.\n", drive->GetLastError( ) );
                delete request;
                parent->SetCurrentRequest( NULL );
                break;
            case IRadDrive::Retry:
                rDebugPrintf( "radFileError: [%d]. Retrying operation. For additional info install an error handler.\n", drive->GetLastError( ) );
                parent->QueueRequest( request, priority, true );
                parent->SetCurrentRequest( NULL );
                break;
            case IRadDrive::Suspend: // signal user, then wait then retry/fail
                parent->WaitGlobalError( drive );
                
                if ( parent->m_Retry )
                {
                    parent->QueueRequest( request, priority, true );
                    parent->SetCurrentRequest( NULL );

                    parent->m_Retry = false;
                    errorRetry = true;
                }
                else
                {
                    delete request;
                    parent->SetCurrentRequest( NULL );
                }

                break;
            default:
                rAssertMsg( 0, "radDriveThread: internal error" );
            }
            break;

        case radDrive::MoreToDo:  // requeue at the head and at the same priority
            parent->QueueRequest( request, priority, true );
            parent->SetCurrentRequest( NULL );
            break;

        case radDrive::KeepAlive: // do nothing
            break;

        default:
            rAssertMsg( 0, "radDriveThread: internal error" );
        }

        //
        // Now that old request is done.
        //
        parent->Lock( );
        parent->m_OutstandingRequests--;
        parent->Unlock( );
    }

    rAssertMsg( parent->NextRequest( NULL ) == NULL, "radDriveThread: cannot terminate with outstanding requests" );
    return 0;
}

//=============================================================================
// Public Member Functions
//=============================================================================

radDriveThread::radDriveThread( IRadThreadMutex* pMutex, radMemoryAllocator alloc, unsigned int stackSize )
    :
    m_pMutex( pMutex ),
    m_pThread( NULL ),
    m_TerminateThread( false ),
    m_pSema( NULL ),
    m_CurrentRequest( NULL ),
    m_pErrorSemaphore( NULL ),
    m_Retry( false ),
    m_OutstandingRequests( 0 ),
    m_CancelKey( NULL )
{
    radAddRef( m_pMutex, this );

    for ( unsigned int i = 0; i < NumPriorities; i++ )
    {
        m_RequestHead[i] = NULL;
        m_RequestTail[i] = NULL;
    }

    radThreadCreateSemaphore( &m_pSema, 0, alloc );
    rAssertMsg( m_pSema != NULL, "radDriveThread: cannot create semaphore" );

    radThreadCreateSemaphore( &m_pErrorSemaphore, 0, alloc );
    rAssertMsg( m_pErrorSemaphore != NULL, "radDriveThread: cannot create semaphore" );

    radThreadCreateThread( &m_pThread, 
                           DriveThreadEntry, 
                           this, 
                           IRadThread::PriorityHigh, 
                           stackSize,
                           alloc );
    rAssertMsg( m_pThread != NULL, "radDriveThread: cannot create thread" );
}

radDriveThread::~radDriveThread( )
{

    // stop the thread
    m_TerminateThread = true;
    m_pSema->Signal( );
    m_pThread->WaitForTermination( );
    radRelease( m_pThread, this );

    radRelease( m_pMutex, this );
    radRelease( m_pSema, this );
    radRelease( m_pErrorSemaphore, this );
}

//=============================================================================
// Function:    radDriveThread::Lock
//=============================================================================
// Description: Start a critical section
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radDriveThread::Lock( void )
{
    m_pMutex->Lock( );
}

//=============================================================================
// Function:    radDriveThread::Unlock
//=============================================================================
// Description: End a critical section
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radDriveThread::Unlock( void )
{
    m_pMutex->Unlock( );
}

//=============================================================================
// Function:    radDriveThread::QueueRequest
//=============================================================================
// Description: This routine is used to queue a request. Each request is simply
//              added to the queue at the appropriate priority. If nothing is
//              running, then the request is executed right away.
//
// Parameters:  pRequest - request to queue
//              priority - priority of request.
//              toHead - true if should be queued to head of list.
//
// Returns:    
//------------------------------------------------------------------------------

void radDriveThread::QueueRequest
( 
    radRequest*          pRequest,
    radFilePriority      priority,
    bool                 toHead
)     
{
    Lock( );
    
    //
    // Add to the head or tail as appropriate
    //
    if ( toHead )
    {
        if ( m_RequestTail[ priority ] == NULL )
        {
            m_RequestTail[ priority ] = pRequest;
        }

        pRequest->m_Next = m_RequestHead[ priority ];
        m_RequestHead[ priority ] = pRequest;
    }
    else
    {
        if ( m_RequestHead[ priority ] == NULL )
        {
            m_RequestHead[ priority ] = pRequest;
        }
        else
        {
            m_RequestTail[ priority ]->m_Next = pRequest;
        }

        pRequest->m_Next = NULL;
        m_RequestTail[ priority ] = pRequest;
    }

    m_OutstandingRequests++;

    Unlock( );
    
    //
    // signal the thread.
    //    
    m_pSema->Signal( );
}

//=============================================================================
// Function:    radDriveThread::CancelRequests
//=============================================================================
// Description: This function is invoked to cancel all requests on this
//              drive thread object that match the key.
//
// Parameters:  key
//              priority
//
// Returns:     
//------------------------------------------------------------------------------

void radDriveThread::CancelRequests( void* key, radFilePriority priority )
{
    Lock( );

    radRequest** ppRequest = &m_RequestHead[ priority ];
    radRequest* pEnd = NULL;

    while ( *ppRequest != NULL )
    {
        if ( (*ppRequest)->ShouldCancel( key ) )
        {
            //
            // Remove the element
            //
            radRequest* pRequest = *ppRequest;
            *ppRequest = pRequest->m_Next;

            delete pRequest;
            //
            // Don't forget to tell the semaphore!
            // We need to wait (we will never block) so the count is correct
            //
            m_pSema->Wait( );
        }
        else
        {
            pEnd = *ppRequest;
            ppRequest = &( (*ppRequest)->m_Next );
        }
    }

    m_RequestTail[ priority ] = pEnd;

    Unlock( );
}

//=============================================================================
// Function:    radDriveThread::GetCurrentRequest
//=============================================================================
// Description: Returns the current request being processed.
//
// Parameters:  
//
// Returns:     The current request.
//------------------------------------------------------------------------------

const radRequest* const radDriveThread::GetCurrentRequest() const
{
    return m_CurrentRequest;
}

//=============================================================================
// Function:    radDriveThread::OutstandingRequests
//=============================================================================
// Description: Return whether there are any requests in the queue.
//
// Parameters:  
//
// Returns:     True if the queue is not empty.
//------------------------------------------------------------------------------

bool radDriveThread::OutstandingRequests( void )
{
    Lock( );
    bool ret = ( m_OutstandingRequests != 0 );
    Unlock( );

    return ret;
}

//=============================================================================
// Private Member Functions
//=============================================================================
    
//=============================================================================
// Function:    radDriveThread::WaitGlobalError
//=============================================================================
// Description: Signal that a error has occurred and wait for it to clear.
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radDriveThread::WaitGlobalError( radDrive* drive )
{
    drive->SetErrorState( );
    m_pErrorSemaphore->Wait( );
}

//=============================================================================
// Function:    radDriveThread::WaitGlobalError
//=============================================================================

void radDriveThread::ResumeRequest( bool retry )
{
    m_Retry = retry;
    m_pErrorSemaphore->Signal( );
}

//=============================================================================
// Function:    radDriveThread::NextRequest
//=============================================================================
// Description: This routine gets the next, highest priority request. The 
//              request is removed from the queue and returned.
//
// Parameters:  priority -- (optional) return the priority of the request
//
// Returns:     the next request or NULL if there are none
//------------------------------------------------------------------------------

radRequest* radDriveThread::NextRequest( radFilePriority* priority )
{
    radRequest* pRequest = NULL;
    
    Lock( );
    for ( unsigned int i = 0; i < NumPriorities; i++ )
    {
        if ( m_RequestHead[i] != NULL )
        {
            //
            // Special case if there's exactly one request on this queue
            //
            if ( m_RequestHead[i] == m_RequestTail[i] )
            {
                m_RequestTail[i] = NULL;
            }

            //
            // Remove the request
            //
            pRequest = m_RequestHead[i];
            m_RequestHead[i] = pRequest->m_Next;
            pRequest->m_Next = NULL;

            if ( priority != NULL )
            {
                *priority = ( radFilePriority )i;
            }

            break;
        }
    }
    Unlock( );

    return pRequest;
}

//=============================================================================
// Function:    radDriveThread::NextRequest
//=============================================================================
// Description: This sets the currect drive thread's current request.
//              
//
// Parameters:  The current request
//
// Returns:     nothing
//------------------------------------------------------------------------------

void radDriveThread::SetCurrentRequest( radRequest* request )
{
    m_CurrentRequest = request;
}

