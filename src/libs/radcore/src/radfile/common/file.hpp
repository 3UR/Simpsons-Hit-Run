//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        file.hpp
//
// Subsystem:	Radical File System
//
// Description:	This file contains all definitions and classes relevant to
//              the radFile object.
//
// Revisions:	
//
//=============================================================================

#ifndef	FILE_HPP
#define FILE_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radmemory.hpp>   
#include <radfile.hpp>
#include "saferefobject.hpp"
#include "platformdrives.hpp"
#include "filecache.hpp"

//=============================================================================
// Forward Class Declarations
//=============================================================================

class radDrive;

//=============================================================================
// Functions
//=============================================================================

//
// These two members are used to initialize and terminate the file pool.
//
void radFilePoolInitialize( unsigned int maxOpenFiles, radMemoryAllocator alloc );

void radFilePoolTerminate( void );

//=============================================================================
// Class Declarations
//=============================================================================

//
// This is the radFile object. 
//
class radFile : public IRadFile,
                public IRadMutable
{   
public:
    //
    // Constructors and destructor
    //

    //
    // Make a file.
    //
    radFile( radDrive*          pDrive,
             const char*        pFileName,
             bool               writeAccess,
             radFileOpenFlags   flags,
             radFilePriority    priority,
             unsigned int       cacheSize,
             radMemoryAllocator alloc,
             radMemorySpace     cacheSpace );

    //
    // Make a save game.
    //
    radFile( radDrive*          pDrive,
             const char*        pFileName,
             bool               writeAccess,
             radFileOpenFlags   flags,
             radMemcardInfo*    memcardInfo,
             unsigned int       maxSize,
             radFilePriority    priority );

    ~radFile( void );

	//
    // IRefCount
    //
    void AddRef( void );
	void Release( void );

    //
    // IRadMutable
    //
    void Lock( void );
    void Unlock( void );

    //
    // Need a placement new since all files come from a pool,
    //
    void* operator new( size_t size ); 
    void operator delete( void* p );
 
    //
    // IRadFile
    //
    virtual void ReadAsync( void* pBuffer, unsigned int bytesToRead, radMemorySpace pBufferSpace = radMemorySpace_Local );
    virtual void WriteAsync( const void* pBuffer, unsigned int bytesToWrite, radMemorySpace pBufferSpace = radMemorySpace_Local );
    virtual void SetPositionAsync( unsigned int position );
    virtual void GetPositionAsync( unsigned int* pPosition );
    virtual void GetSizeAsync( unsigned int* pSize );
    virtual void CommitAsync( void );

    virtual void ReadSync( void* pBuffer, unsigned int bytesToRead, radMemorySpace pBufferSpace = radMemorySpace_Local );
    virtual void WriteSync( const void* pBuffer, unsigned int bytesToWrite, radMemorySpace pBufferSpace = radMemorySpace_Local );
    virtual void SetPositionSync( unsigned int position );
    virtual void GetPositionSync( unsigned int* pPosition );
    virtual void GetSizeSync( unsigned int* pSize );
    virtual void CommitSync( void );

    virtual unsigned int GetSize( void );

    virtual void CancelAsync( void );
    virtual void AddCompletionCallback( IRadFileCompletionCallback* pCallback, void* pUserData );
    virtual void AddCompletionEvent( IRadThreadSemaphore* pSemaphore );
    virtual bool CheckForCompletion( void );
    virtual void WaitForCompletion( void );            

    virtual radFileError GetLastError( void );

    virtual bool IsOpen( void );
    virtual const char* GetFilename( void );
    virtual const char* GetDrivename( void );

    virtual void SetPriority( radFilePriority priority );
	virtual radFilePriority GetPriority( void );

    virtual bool IsSaveGame( void );

    virtual unsigned int GetOptimalSize( void );

    virtual void SetBufferedRead( BufferedReadState state );

    virtual void* GetHandle( void );

private:
    //
    // We allow the various request objects access to our private data.
    //
    friend class radFileRequest;
    friend class FileOpenRequest;
    friend class FileCloseRequest;
    friend class FileReadRequest;
    friend class FileCachedReadRequest;
    friend class FileWriteRequest;
    friend class FileSetPositionRequest;
    friend class FileGetPositionRequest;
    friend class FileGetSizeRequest;
    friend class FileCancelRequest;
    friend class FileCompletionRequest;
    friend class SaveGameOpenRequest;
    friend class FileCommitRequest;

    //
    // IRadRefcount
    //
    int m_RefCount;    

    //
    // This member contains reference to parent drive. 
    //     
    radDrive*       m_pDrive;

    //
    // This maintains the priority at which asynchronous operations should be carried
    // out.
    //
    radFilePriority m_Priority;

    radFileError    m_LastError;
    unsigned int    m_OutstandingRequests;

    bool            m_IsOpen;
    bool            m_WriteAccess;
    char            m_Filename[ radFileFilenameMax + 1 ];

    BufferedReadState m_BufferedState;

    //
    // These maintain the current file position and size.
    //
    unsigned int    m_Position;
    unsigned int    m_Size;

    //
    // For cement files, we need to know the offset into the file. We also need
    // to know which file we are actual using, so that it stays alive.
    //
    radFile*        m_pOriginalFile;
    unsigned int    m_BaseOffset;

    //
    // We keep a cache object, and a buffer to use in the cache.
    //
    FileCache           m_FileCache;
    unsigned char*      m_CacheMemory;
    radMemoryAllocator  m_CacheAllocator;

    //
    // For each platform, handles are required.
    //
    radFileHandle       m_Handle;

    //
    // Is it a save game or not.
    //
    bool    m_IsSaveGame;

    //
    // Remember the numbers for optimal alignment.
    //
    unsigned int    m_OptimalSize;

#ifdef RAD_DEBUG
    //
    // This is a member whose only use is to track references to the physical drive.
    //
    bool m_FirstRemoval;
#endif
};


#endif 
