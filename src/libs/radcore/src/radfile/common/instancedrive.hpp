//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        instancedrive.hpp
//
// Subsystem:	Radical Drive System
//
// Description:	This file contains all definitions and classes relevant to
//              the instance drive. This is a logical user drive object not
//				the physical drive object.	
//
// Revisions:	
//
//=============================================================================

#ifndef	INSTANCEDRIVE_HPP
#define INSTANCEDRIVE_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radfile.hpp>
#include "saferefobject.hpp"
#include "platformdrives.hpp"

//=============================================================================
// Forward Declarations
//=============================================================================

class radDrive;

//=============================================================================
// Functions
//=============================================================================

//
// These two members are used to initialize and terminate the drive pool.
//
void radDrivePoolInitialize( unsigned int maxDrives, radMemoryAllocator alloc );

void radDrivePoolTerminate( void );

//=============================================================================
// Class Declarations
//=============================================================================

//
// This class represents the instance of a drive object. It is not the physical
// drive object for which there is only a singleton. They are almost the same, but
// the instance drive object maintains instance data for a particular client.
//
class radInstanceDrive : public IRadDrive,
                         public IRadMutable 
{
public:
    //
    // Constructor gets pointer to physical drive.
    //
    radInstanceDrive( radDrive* pDrive, radFilePriority priority );
    ~radInstanceDrive( void );

    //
    // IRefCount
    //
	virtual void AddRef( void );
	virtual void Release( void );
    
    //
    // IRadMutable
    //
    virtual void Lock( void );
    virtual void Unlock( void );

    //
    // Need a placement new since all instance drives come from a pool,
    //
    void* operator new( size_t size ); 
    void operator delete( void* p );
    
    //
    // IRadDrive
    //
    virtual unsigned int GetCapabilities( void );    

    virtual void GetMediaInfoAsync( MediaInfo* pMediaInfo );
    virtual void FormatAsync( void );
    virtual void CreateDirectoryAsync( const char* pDirectoryName );
    virtual void DestroyDirectoryAsync( const char* pDirectoryName );
    virtual void FindFirstAsync( const char* searchspec, DirectoryInfo* pDirectoryInfo );
    virtual void FindNextAsync(  DirectoryInfo* pDirectoryInfo );
    virtual void DestroyFileAsync( const char* pFileName, bool simpleName = false );

    virtual void GetMediaInfoSync( MediaInfo* pMediaInfo );
    virtual void FormatSync( void );
    virtual void CreateDirectorySync( const char* pDirectoryName );
    virtual void DestroyDirectorySync( const char* pDirectoryName );
    virtual void FindFirstSync( const char* searchspec, DirectoryInfo* pDirectoryInfo );
    virtual void FindNextSync( DirectoryInfo* pDirectoryInfo );
    virtual void DestroyFileSync( const char* pFileName, bool simpleName = false );

    virtual void CancelAsync( void );
    virtual void AddCompletionCallback( IRadDriveCompletionCallback* pCallback, void* pUserData );
    virtual void AddCompletionEvent( IRadThreadSemaphore* pSemaphore );
    virtual bool CheckForCompletion( void );
    virtual void WaitForCompletion( void );            

    virtual radFileError GetLastError( void );

    virtual void RegisterErrorHandler( IRadDriveErrorCallback* callback, void* pUserData );
    virtual void UnregisterErrorHandler( IRadDriveErrorCallback* callback );
    virtual void RegisterErrorEvent( IRadThreadSemaphore* pSemaphore );
    virtual void UnregisterErrorEvent( IRadThreadSemaphore* pSemaphore );
    virtual bool CheckForErrorState( void );
    virtual void ResumeRequest( bool retry );
    virtual void SetDefaultErrorBehaviour( ErrorBehaviour behaviour );
    virtual void SetErrorClearReporting( bool notifyOnErrorClear );


    virtual void SetReadWriteGranularity( unsigned int Size );
    virtual unsigned int GetReadWriteGranularity( void );

    // TEMPORARY!!!!!!!!!
    virtual void FileOpen( IRadFile**         pIRadFile, 
                           const char*        pFileName,
                           bool               writeAccess,
                           radFileOpenFlags   flags,
                           radFilePriority    priority, 
                           unsigned int       cacheSize,
                           radMemoryAllocator alloc,
                           radMemorySpace     cacheSpace )
    {
        FileOpenAsync( pIRadFile, pFileName, writeAccess, flags, priority, cacheSize, alloc, cacheSpace );
    }

    virtual void FileOpenAsync( IRadFile**    pIRadFile, 
                           const char*        pFileName,
                           bool               writeAccess,
                           radFileOpenFlags   flags,
                           radFilePriority    priority, 
                           unsigned int       cacheSize,
                           radMemoryAllocator alloc,
                           radMemorySpace     cacheSpace );

    virtual void SaveGameOpenAsync( IRadFile**        pIRadFile, 
                                const char*      pFileName,
                                bool             writeAccess,
                                radFileOpenFlags flags,
                                radMemcardInfo*  memcardInfo,
                                unsigned int     maxSize,
								bool			 simpleName,
                                radFilePriority  priority );

    virtual void SaveGameOpenSync( IRadFile**       pIRadFile, 
                                   const char*      pFileName,
                                   bool             writeAccess,
                                   radFileOpenFlags flags,
                                   radMemcardInfo*  memcardInfo,
                                   unsigned int     maxSize,
								   bool			 simpleName,
                                   radFilePriority  priority );

    virtual void FileOpenSync( IRadFile**         pIRadFile, 
                               const char*        pFileName,
                               bool               writeAccess,
                               radFileOpenFlags   flags,
                               radFilePriority    priority, 
                               unsigned int       cacheSize,
                               radMemoryAllocator alloc,
                               radMemorySpace     cacheSpace );

    virtual void SetPriority( radFilePriority priority );
	virtual radFilePriority GetPriority( void );

    virtual const char* GetDriveName( void );

    virtual unsigned int GetCreationSize( radMemcardInfo* memcardInfo, unsigned int size );
private:

    //
    // Various requests associated with this object.
    //
    friend class radDriveRequest;
    friend class DriveOpenRequest;
    friend class DriveMediaRequest;
    friend class DriveFormatRequest;
    friend class DriveCreateDirectoryRequest;
    friend class DriveDestroyDirectoryRequest;
    friend class DriveFindFirstRequest;
    friend class DriveFindNextRequest;
    friend class DriveFindCloseRequest;
    friend class DriveFileDestroyRequest;
    friend class DriveCancelRequest;
    friend class DriveCompletionRequest;

    //
    // IRadRefcount
    //
    int m_RefCount;    

    //
    // This member contains reference to physical drive. 
    //     
    radDrive*       m_pDrive;

    //
    // This maintains the priority at which asynchronous operations should be carried
    // out.
    //
    radFilePriority m_Priority;

    unsigned int    m_OutstandingRequests;

    //
    // Directory search members
    //
    radFileDirHandle    m_DirHandle;
    bool                m_DirSearch; // have we done a findFirst?

#ifdef RAD_DEBUG
    //
    // This is a member whose only use is to track references to the physical drive.
    //
    bool m_FirstRemoval;
#endif
};

#endif // INSTANCEDRIVE_HPP
