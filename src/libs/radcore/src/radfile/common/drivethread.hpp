//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef	DRIVETHREAD_HPP
#define DRIVETHREAD_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radfile.hpp>
#include <radthread.hpp>
#include "file.hpp"

//=============================================================================
// Forward Class Declarations
//=============================================================================

class radRequest;
class radDrive;

//=============================================================================
// Class Declarations
//=============================================================================

class radDriveThread : public IRefCount,
                       public radObject,
                       public radSafeRefCount
{
public:

    radDriveThread( IRadThreadMutex* pMutex, radMemoryAllocator alloc, unsigned int stackSize = 4096 );
    virtual ~radDriveThread( );

    IMPLEMENT_SAFEREFCOUNTED;

    void QueueRequest( radRequest* pRequest, radFilePriority priority, bool toHead );  

    void CancelRequests( void* key, radFilePriority priority );

    const radRequest* const GetCurrentRequest() const;

    void Lock( void );
    void Unlock( void );

    void ResumeRequest( bool retry );

    bool OutstandingRequests( void );

private:

    //
    // Thread entry point for the drive thread.
    //
    static unsigned int DriveThreadEntry( void* userData );

    //
    // The mutex for critical sections
    //
    IRadThreadMutex* m_pMutex;

    //
    // The drive thread.
    //
    IRadThread* m_pThread;

    //
    // Boolean indicating whether the thread should terminate.
    //
    bool m_TerminateThread;

    //
    // Semaphore used for communication with the thread
    //
    IRadThreadSemaphore* m_pSema;

    //
    // This member maintains a prioritized linked list of requests. 
    //
    radRequest*     m_RequestHead[ NumPriorities ];
    radRequest*     m_RequestTail[ NumPriorities ];
    radRequest*     m_CurrentRequest;

    //
    // Semaphore to wait on during an error
    //
    IRadThreadSemaphore*    m_pErrorSemaphore;
    bool                    m_Retry;

    //
    // Null if there are no more, otherwise removes the next request and returns it.
    //
    radRequest* NextRequest( radFilePriority* priority );

    void SetCurrentRequest( radRequest* request );

    //
    // signals that an error has occured and waits until it is resolved.
    //
    void WaitGlobalError( radDrive* drive );

    //
    // We keep the number of outstanding requests so that we can return when everything is done.
    //
    unsigned int m_OutstandingRequests;

    //
    // We keep a cancel key which is checked by the thread after it finishes a request.
    // This allows the thread to know if the request it just finished was supposed to be
    // canceled and avoid retrying it.
    //
    void*        m_CancelKey;
};

#endif // DRIVETHREAD_HPP
