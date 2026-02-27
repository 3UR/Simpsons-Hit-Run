//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        drive.hpp
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

#ifndef	DRIVE_HPP
#define DRIVE_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radobject.hpp>
#include "saferefobject.hpp"
#include "platformdrives.hpp"
#include "cementer.hpp"

//=============================================================================
// Forward Class Declarations
//=============================================================================

class radFile;
class radRequest;
class CompletionCallbackRequest;
class radDriveThread;
class radCementLibrary;
class radCementLibraryList;

//=============================================================================
// Class Declarations
//=============================================================================

//
// Physical drive object base class. Derive various platform specific implementations
// from this class.
// All operations which will be implemented by subclasses do not need to be thread safe.
// We are guaranteed that these happen serially.
//
class radDrive : public IRefCount,
                 public radObject,
                 public radSafeRefCount
                 
{
public:

    radDrive( );
    virtual ~radDrive( );
    
    IMPLEMENT_SAFEREFCOUNTED;

    //
    // These are the replies that the physical drive must respond with.
    //
    enum CompletionStatus
    {
        Error,                      // Operation failed
        Complete,                   // Operation completed successfully
        MoreToDo,                   // Operation is partitioned, still more to do
        KeepAlive                   // Operation is finished, but don't delete it
    };        

    //
    // Get the last known media info. A request for this info will re-initialize the drive.
    //
    const IRadDrive::MediaInfo* GetMediaInfo( );

    radFileError GetLastError( void );

    //
    // This is called once per radFileService( ) on the user's thread.
    // This only needs to be virtual for the remote drive!!!!!
    //
    virtual void Service( void );

    //
    // Use this member to queue a request for execution on this physical drive.
    // Cancels are added to the head.
    //        
    void QueueRequest( radRequest* pRequest, radFilePriority priority, bool toHead = false );  

    //
    // Remove all request matching key from priority queue
    //
    void CancelRequests( void* key, radFilePriority priority );

    //
    // Check of the queue is empty.
    //
    bool OutstandingRequests( void );

    //
    // Check of the callback queue is empty.
    //
    bool OutstandingCallbacks( void );

    //
    // Add a callback to the service callback queue
    //
    void AddCallback( CompletionCallbackRequest* pCallback );

    //
    // Global error handling routines
    //
    void RegisterErrorHandler( IRadDriveErrorCallback* callback, void* pUserData );
    void UnregisterErrorHandler( IRadDriveErrorCallback* callback );
    void RegisterErrorEvent( IRadThreadSemaphore* pSemaphore );
    void UnregisterErrorEvent( IRadThreadSemaphore* pSemaphore );

    bool CheckForErrorState( void );
    void SetErrorState( void );
    
    void SetDefaultErrorBehaviour( IRadDrive::ErrorBehaviour behaviour );
    IRadDrive::ErrorBehaviour GetDefaultErrorBehaviour( void );

    void ResumeRequest( bool retry );

    void SetErrorClearReporting( bool notifyOnErrorClear );
    bool GetErrorClearReporting( void );

    //
    // These are needed for cement files.
    //
    void RegisterCementLibrary( radCementLibrary* pLib );
    void UnregisterCementLibrary( radCementLibrary* pLib );
    radCementLibraryList* GetLibraryList( void );
    
    /////////////////////////////////////////////////////
    // Operations to be implemented by subclasses
    /////////////////////////////////////////////////////

    //
    // Get the drive name
    //
    virtual const char* GetDriveName( void ) = 0;

    //
    // By default, this returns size.
    //
    virtual unsigned int GetCreationSize( radMemcardInfo* memcardInfo, unsigned int size );

    //  
    // Based on the capabilities of the physical drive object, these various functions
    // should be implemented. If the physical drive reports in its capabilities that it
    // is read only, then it need not implement the write file. Ones that must be implemented
    // are pure virtual.
    //
    virtual unsigned int GetCapabilities( void ) = 0;    

    //
    // Initialize is called after an error, to see if anything changed.
    // The only error it can cause is WrongMedia, if the wrong disk is in.
    //
    virtual CompletionStatus Initialize( void ) = 0;

    //
    // A drive must implement one or both of OpenFile and OpenSaveGame
    //
    virtual CompletionStatus OpenFile( const char* fileName, 
                                       radFileOpenFlags flags, 
                                       bool writeAccess, 
                                       radFileHandle* pHandle, 
                                       unsigned int* pSize );

//    virtual CompletionStatus OpenCacheFile( const char* fileName, radFileOpenFlags flags, bool writeAccess, bool fileAlreadyLoaded,
//                                          unsigned int* pBaseOffset, void* pHandle, unsigned int* pSize );

    virtual CompletionStatus OpenSaveGame( const char* fileName, 
                                           radFileOpenFlags flags, 
                                           bool writeAccess,
                                           radMemcardInfo* memcardInfo,
                                           unsigned int maxSize,
                                           radFileHandle* pHandle, 
                                           unsigned int* pSize );

    //
    // Should never cause an error since the user doesn't wait for it.
    //
    virtual CompletionStatus CloseFile( radFileHandle handle, const char* fileName ) = 0;

    //
    // Happily do nothing unless the drive implements it.
    //
    virtual CompletionStatus CommitFile( radFileHandle handle, const char* fileName );

    virtual CompletionStatus ReadFile( radFileHandle handle, 
                                       const char* fileName,
                                       IRadFile::BufferedReadState state,
                                       unsigned int position, 
                                       void* pData, 
                                       unsigned int bytesToRead, 
                                       unsigned int* bytesRead, 
                                       radMemorySpace pDataSpace ) = 0;

    virtual CompletionStatus WriteFile( radFileHandle handle,
                                        const char* fileName,
                                        IRadFile::BufferedReadState state,
                                        unsigned int position, 
                                        const void* pData, 
                                        unsigned int bytesToWrite, 
                                        unsigned int* bytesWritten, 
                                        unsigned int* size, 
                                        radMemorySpace pDataSpace );

    virtual CompletionStatus Format( void );

    virtual CompletionStatus CreateDir( const char* pName );

    virtual CompletionStatus DestroyDir( const char* pName );   

    virtual CompletionStatus DestroyFile( const char* filename );

    virtual CompletionStatus FindFirst( const char* searchSpec, 
                                        IRadDrive::DirectoryInfo* pDirectoryInfo, 
                                        radFileDirHandle* pHandle,
                                        bool firstSearch );

    virtual CompletionStatus FindNext( radFileDirHandle* pHandle, IRadDrive::DirectoryInfo* pDirectoryInfo );

    //
    // Should never cause an error, since the user doesn't wait for it.
    //
    virtual CompletionStatus FindClose( radFileDirHandle* pHandle );


//
// These members are used to keep track of the instance drives and files which are 
// referencing this drive.
//
#ifdef RAD_DEBUG
    unsigned int m_pNumRefFiles;
    unsigned int m_pNumRefDrives;

    void AddFileReference( void )
    {
        Lock( );
        m_pNumRefFiles++;
        Unlock( );
    }
    void RemoveFileReference( void )
    {
        Lock( );
        m_pNumRefFiles--;
        Unlock( );
    }

    void AddDriveReference( void )
    {
        Lock( );
        m_pNumRefDrives++;
        Unlock( );
    }
    void RemoveDriveReference( void )
    {
        Lock( );
        m_pNumRefDrives--;
        Unlock( );
    }
#endif
    
private:
    //
    // Cement file library list
    //
    radCementLibraryList    m_LibraryList;

    //
    // Callback queue
    //
    CompletionCallbackRequest*      m_CallbackHead;
    CompletionCallbackRequest*      m_CallbackTail;
 
    //
    // Global error handling.
    //
    IRadDrive::ErrorBehaviour   m_ErrorBehaviour;
    IRadThreadSemaphore*        m_pErrorUserSemaphore; // user's semaphore
    IRadThread*                 m_pErrorThread;        // thread to callback 
    IRadDriveErrorCallback*     m_pErrorCallback;      // user's callback
    void*                       m_pUserData;
    bool                        m_InError;             // indicate if we have an error
    bool                        m_NotifyClear;

    bool                        m_InCallback;   // indicate that have called the error callback

protected:

    //
    // Must be maintained during the initialize command
    //
    IRadDrive::MediaInfo    m_MediaInfo;

    //
    // Error state to be set by subclass. It is set to Success before each
    // request is run. All calls on the drive must set it appropriately.
    //
    radFileError    m_LastError;

    //
    // This is the thread on which requests will be run. Must be filled in by base class.
    //
    radDriveThread*     m_pDriveThread;
};

#endif 
