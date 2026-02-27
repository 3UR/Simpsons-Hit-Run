//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        requests.hpp
//
// Subsystem:	Radical File System
//
// Description:	This file contains all definitions and classes relevant to
//              request objects. 
//
// Revisions:	
//
//=============================================================================

#ifndef	REQUESTS_HPP
#define REQUESTS_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radmemory.hpp>
#include <radfile.hpp>
#include <radthread.hpp>
#include "drivethread.hpp"
#include "drive.hpp"

//=============================================================================
// Forward Class Declarations
//=============================================================================

class radFile;
class radInstanceDrive;

//=============================================================================
// Functions
//=============================================================================

//
// These two members are used to initialize and terminate the request pool.
//
void radRequestPoolInitialize( unsigned int       maxOutstandingRequests, 
                               radMemoryAllocator alloc );

void radRequestPoolTerminate( void );

//=============================================================================
// Class Declarations
//=============================================================================

//=============================================================================
// Class: radRequest
//
// This is the base class for all requests. It is an internal object and
// is not reference counted.
//=============================================================================

class radRequest
{   
public:
    //
    // Constructor and destructor
    //
    radRequest( );
    virtual ~radRequest( );

    //
    // Need a placement new since all requests come from a pool,
    //
    void* operator new( size_t size ); 
    void operator delete( void* p );

    //
    // This member is invoked to actually carry out a request. It returns
    // a value indicating the completion status of the operation.
    //
    virtual radDrive::CompletionStatus DoRequest( void ) = 0;    

    //
    // This member is provided to determine if the request should
    // be canceled. A key is provided which must match an internal
    // key maintained by the request if it is cancellable. 
    //
    virtual bool ShouldCancel( void* key ) = 0;    
    
    //
    // This member returns the drive which owns this request
    //
    virtual radDrive* GetOwner( void ) = 0;

    //
    // Re-initialize the drive.
    //
    virtual radDrive::CompletionStatus ReInit( void ) = 0;

protected:
    friend class radDriveThread;
    friend class radDrive;

    //
    // We maintain requests using a linked list. The drive is a friend
    // class since it acually manages requests.
    //
    radRequest* m_Next;
};

//=============================================================================
// Class: CompletionEventRequest
//
// This class represents an event which is signalled after asynchronous
// operations complete.
//=============================================================================

class CompletionEventRequest : public radRequest
{
public:    
    CompletionEventRequest( IRadThreadSemaphore* pSema, radDrive* pDrive );
    virtual ~CompletionEventRequest( );

    radDrive::CompletionStatus DoRequest( void );
    bool ShouldCancel( void* key );    

    radDrive* GetOwner( void );
    radDrive::CompletionStatus ReInit( void );
private:
    IRadThreadSemaphore* m_pSema;
    radDrive*            m_pDrive;
};

//=============================================================================
// Class: CompletionCallbackRequest
//
// This class represents a callback which should be called after
// asynchronous operations complete. The thread id is needed so that the callback
// is only invoked on the calling thread. DoRequest( ) puts this request on the
// queue maintained by radDrive, and callback( ) calls the callback and
// return true if it did so, false if not.
//=============================================================================

class CompletionCallbackRequest : public radRequest
{
public:
    CompletionCallbackRequest( IRadThread* pThread );
    bool ShouldCancel( void* key );
    bool IsRegisteredThread( IRadThread* pThread );
    virtual void callback( void ) = 0;

    radDrive::CompletionStatus ReInit( void );
protected:
    IRadThread* m_pThread;
};

//=============================================================================
// Class: FileCompletionRequest
//=============================================================================

class FileCompletionRequest : public CompletionCallbackRequest
{
public:    
    FileCompletionRequest( IRadThread* pThread, radFile* pFile, IRadFileCompletionCallback* pCallback, void* pUserData );
    virtual ~FileCompletionRequest( );

    radDrive::CompletionStatus DoRequest( void );
    void callback( void );

    radDrive* GetOwner( void );
private:
    radFile*                     m_pFile;
    IRadFileCompletionCallback*  m_Callback;
    void*                        m_UserData;
};

//=============================================================================
// Class: DriveCompletionRequest
//=============================================================================

class DriveCompletionRequest : public CompletionCallbackRequest
{
public:    
    DriveCompletionRequest( IRadThread* pThread, radInstanceDrive* pDrive, IRadDriveCompletionCallback* pCallback, void* pUserData );
    virtual ~DriveCompletionRequest( );

    radDrive::CompletionStatus DoRequest( void );
    void callback( void );

    radDrive* GetOwner( void );
private:
    radInstanceDrive*               m_pInstDrive;
    IRadDriveCompletionCallback*    m_Callback;
    void*                           m_UserData;
};

//=============================================================================
// File Request Classes
//=============================================================================

//=============================================================================
// Class: radFileRequest
//
// This is a base class for file requests. Handles some basic communication
// with the file which has made this request.
//=============================================================================

class radFileRequest : public radRequest
{
public:
    //
    // Constructor, destructor deal with the file object.
    //
    radFileRequest( radFile* pFile );
    virtual ~radFileRequest( void );
    
    virtual bool ShouldCancel( void* key );    

    radDrive* GetOwner( void );
    radDrive::CompletionStatus ReInit( void );

    virtual const char* GetFilename( void );

protected:
    //
    // Helper function to handle error codes
    //
    radDrive::CompletionStatus handleError( radDrive::CompletionStatus status, radFileError error );

    radFile* m_pFile; 
};          

//=============================================================================
// Class: FileOpenRequest
//=============================================================================

class FileOpenRequest : public radFileRequest
{
public:
    FileOpenRequest( radFile* pFile, radFileOpenFlags flags, bool writeAccess );
    radDrive::CompletionStatus DoRequest( void );    
private:
    radFileOpenFlags m_Flags;
    bool m_WriteAccess;
};

//=============================================================================
// Class: SaveGameOpenRequest
//=============================================================================

class SaveGameOpenRequest : public radFileRequest
{
public:
    SaveGameOpenRequest( radFile* pFile, radFileOpenFlags flags, bool writeAccess, 
                         radMemcardInfo* memcardInfo, unsigned int maxSize );
    radDrive::CompletionStatus DoRequest( void );    
private:
    radFileOpenFlags m_Flags;
    bool m_WriteAccess;
    radMemcardInfo* m_MemcardInfo;
    unsigned int m_MaxSize;
};

//=============================================================================
// Class: FileCloseRequest
//=============================================================================

class FileCloseRequest : public radFileRequest
{
public:
    FileCloseRequest( radFile* pFile );
    radDrive::CompletionStatus DoRequest( void );
    radDrive::CompletionStatus ReInit( void );
};

//=============================================================================
// Class: FileCommitRequest
//=============================================================================

class FileCommitRequest : public radFileRequest
{
public:
    FileCommitRequest( radFile* pFile );
    radDrive::CompletionStatus DoRequest( void );
};

//=============================================================================
// Class: FileReadRequest
//=============================================================================

class FileReadRequest : public radFileRequest
{
public:
    FileReadRequest( radFile* pFile, void* pData, unsigned int bytesToRead, radMemorySpace pBufferSpace );
    radDrive::CompletionStatus DoRequest( void );
private:
    void*           m_pData;
    unsigned int    m_BytesToRead;
    radMemorySpace  m_pDataSpace;
};

//=============================================================================
// Class: FileWriteRequest
//=============================================================================

class FileWriteRequest : public radFileRequest
{
public:
    FileWriteRequest( radFile* pFile, const void* pData, unsigned int bytesToWrite, radMemorySpace pDataSpace );
    radDrive::CompletionStatus DoRequest( void );
private:
    const void*     m_pData;
    unsigned int    m_BytesToWrite;
    radMemorySpace  m_pDataSpace;
};

//=============================================================================
// Class: FileSetPositionRequest
//=============================================================================

class FileSetPositionRequest : public radFileRequest
{
public:
    FileSetPositionRequest( radFile* pFile, unsigned int position );
    radDrive::CompletionStatus DoRequest( void );
private:
    unsigned int m_Position;
};

//=============================================================================
// Class: FileGetPositionRequest
//=============================================================================

class FileGetPositionRequest : public radFileRequest
{
public:
    FileGetPositionRequest( radFile* pFile, unsigned int* position );
    radDrive::CompletionStatus DoRequest( void );
private:
    unsigned int* m_pPosition;
};

//=============================================================================
// Class: FileGetSizeRequest
//=============================================================================

class FileGetSizeRequest : public radFileRequest
{
public:
    FileGetSizeRequest( radFile* pFile, unsigned int* pSize );
    radDrive::CompletionStatus DoRequest( void );
private:
    unsigned int* m_pSize;
};

//=============================================================================
// Class: FileCancelRequest
//=============================================================================

class FileCancelRequest : public radFileRequest
{
public:
    FileCancelRequest( radFile* pFile );
    radDrive::CompletionStatus DoRequest( void );
    radDrive::CompletionStatus ReInit( void );
};

//=============================================================================
// Drive Request Classes
//=============================================================================

//=============================================================================
// Class: radDriveRequest
//
// This is a base class for drive requests. Handles some basic communication
// with the drive which has made this request.
//=============================================================================

class radDriveRequest : public radRequest
{
public:
    //
    // Constructor, destructor deal with the instance drive object.
    //
    radDriveRequest( radInstanceDrive* pInstDrive );
    virtual ~radDriveRequest( void );
    
    virtual bool ShouldCancel( void* key );    

    radDrive* GetOwner( void );
    radDrive::CompletionStatus ReInit( void );
protected:
    radInstanceDrive* m_pInstDrive;
};          

//=============================================================================
// Class: DriveOpenRequest
//=============================================================================

class DriveOpenRequest : public radDriveRequest
{
public:    
    DriveOpenRequest( radInstanceDrive* pInstDrive );
    radDrive::CompletionStatus DoRequest( void );
    radDrive::CompletionStatus ReInit( void );
};

//=============================================================================
// Class: DriveMediaRequest
//=============================================================================

class DriveMediaRequest : public radDriveRequest
{
public:    
    DriveMediaRequest( radInstanceDrive* pInstDrive, IRadDrive::MediaInfo* pMediaInfo );
    radDrive::CompletionStatus DoRequest( void );
    radDrive::CompletionStatus ReInit( void );
private:
    IRadDrive::MediaInfo* m_pMediaInfo;
};

//=============================================================================
// Class: DriveFormatRequest
//=============================================================================

class DriveFormatRequest : public radDriveRequest
{
public:    
    DriveFormatRequest( radInstanceDrive* pInstDrive );
    radDrive::CompletionStatus DoRequest( void );
};

//=============================================================================
// Class: DriveCreateDirectoryRequest
//=============================================================================

class DriveCreateDirectoryRequest : public radDriveRequest
{
public:    
    DriveCreateDirectoryRequest( radInstanceDrive* pInstDrive, const char* pDirName );
    radDrive::CompletionStatus DoRequest( void );
private:
    char m_pDirName[ radFileDrivenameMax + 1 ]; 
};

//=============================================================================
// Class: DriveDestroyDirectoryRequest
//=============================================================================

class DriveDestroyDirectoryRequest : public radDriveRequest
{
public:    
    DriveDestroyDirectoryRequest( radInstanceDrive* pInstDrive, const char* pDirName );
    radDrive::CompletionStatus DoRequest( void );
private:
    char m_pDirName[ radFileDrivenameMax + 1 ]; 
};

//=============================================================================
// Class: DriveFindFirstRequest
//=============================================================================

class DriveFindFirstRequest : public radDriveRequest
{
public:    
    DriveFindFirstRequest( radInstanceDrive* pInstDrive, const char* searchspec, IRadDrive::DirectoryInfo* pDirectoryInfo );
    radDrive::CompletionStatus DoRequest( void );
private:
    char m_pSearchSpec[ radFileFilenameMax + 1];
    IRadDrive::DirectoryInfo* m_pDirInfo;    
};

//=============================================================================
// Class: DriveFindNextRequest
//=============================================================================

class DriveFindNextRequest : public radDriveRequest
{
public:    
    DriveFindNextRequest( radInstanceDrive* pInstDrive, IRadDrive::DirectoryInfo* pDirectoryInfo );
    radDrive::CompletionStatus DoRequest( void );
private:
    IRadDrive::DirectoryInfo* m_pDirInfo;    
};

//=============================================================================
// Class: DriveFindCloseRequest
//=============================================================================

class DriveFindCloseRequest : public radDriveRequest
{
public:    
    DriveFindCloseRequest( radInstanceDrive* pInstDrive );
    radDrive::CompletionStatus DoRequest( void );
    radDrive::CompletionStatus ReInit( void );
};

//=============================================================================
// Class: DriveFileDestroyRequest
//=============================================================================

class DriveFileDestroyRequest : public radDriveRequest
{
public:    
    DriveFileDestroyRequest( radInstanceDrive* pInstDrive, const char* pFilename );
    radDrive::CompletionStatus DoRequest( void );
private:
    char m_pFileName[ radFileFilenameMax + 1];
};

//=============================================================================
// Class: DriveCancelRequest
//=============================================================================

class DriveCancelRequest : public radDriveRequest
{
public:    
    DriveCancelRequest( radInstanceDrive* pInstDrive );
    radDrive::CompletionStatus DoRequest( void );
    radDrive::CompletionStatus ReInit( void );
};

//
// We manage a pool of requests to prevent fragmentation. The following union
// is used to determine the size of the largest request. Make sure any new
// request types are added to the union.
//
union radRequestUnion
{
    char m_Space1[ sizeof( FileOpenRequest ) ];
    char m_Space2[ sizeof( FileCloseRequest ) ];
    char m_Space3[ sizeof( FileReadRequest ) ];
    char m_Space4[ sizeof( FileWriteRequest ) ];
    char m_Space5[ sizeof( FileSetPositionRequest ) ];
    char m_Space6[ sizeof( FileGetPositionRequest ) ];
    char m_Space7[ sizeof( FileGetSizeRequest ) ];
    char m_Space8[ sizeof( FileCancelRequest ) ];
    char m_Space9[ sizeof( FileCompletionRequest ) ];
    char m_SpaceA[ sizeof( DriveOpenRequest ) ];
    char m_SpaceB[ sizeof( DriveMediaRequest ) ];
    char m_SpaceC[ sizeof( DriveFormatRequest ) ];
    char m_SpaceD[ sizeof( DriveCreateDirectoryRequest ) ];
    char m_SpaceE[ sizeof( DriveDestroyDirectoryRequest ) ];
    char m_SpaceF[ sizeof( DriveFindFirstRequest ) ];
    char m_Space10[ sizeof( DriveFindNextRequest ) ];
    char m_Space11[ sizeof( DriveFindCloseRequest ) ];
    char m_Space12[ sizeof( DriveCancelRequest ) ];
    char m_Space13[ sizeof( DriveCompletionRequest ) ];
    char m_Space14[ sizeof( DriveFileDestroyRequest ) ];
    char m_Space15[ sizeof( CompletionEventRequest ) ];
    char m_Space16[ sizeof( SaveGameOpenRequest ) ];
    char m_Space17[ sizeof( FileCommitRequest ) ];
};

#endif // REQUESTS_HPP
