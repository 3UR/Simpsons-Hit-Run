//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        ps2memcarddrive.hpp
//
// Subsystem:	Radical File System
//
// Description:	This file contains all definitions and classes relevant to
//              ps2 memory card drive.
//
// Revisions:	
//
//=============================================================================

#ifndef	PS2MEMCARDDDRIVE_HPP
#define PS2MEMCARDDDRIVE_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <libmc.h>
#include <radkey.hpp>
#include "../common/drive.hpp"
#include "../common/signeddrive.hpp"

//=============================================================================
// Statics
//=============================================================================

static const char s_PS2MemcardDrive[ ] = "MEMCARD??:";
#define PS2MEMCARDDRIVE_PORT_LOC    7
#define PS2MEMCARDDRIVE_SLOT_LOC    8

#define PS2MEMCARDDRIVE_TABLE_ALIGN     64

#define PS2MEMCARDDRIVE_SECTOR_SIZE     1024
#define PS2MEMCARDDRIVE_TRANSFER_BUFFER_SECTORS  1
#define PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE (PS2MEMCARDDRIVE_TRANSFER_BUFFER_SECTORS * PS2MEMCARDDRIVE_SECTOR_SIZE)

//
// Set up the alignment for our transfer buffer -- not really needed but can't hurt
//
#define PS2MEMCARDDRIVE_ALIGNMENT   64

//=============================================================================
// Public Functions
//=============================================================================

//
// Every physical drive type must provide a drive factory. This factory should be
// changed to handle the proxies.
//

void radPs2MemcardDriveFactory( radDrive** ppDrive, const char* driveSpec, radMemoryAllocator alloc );

//=============================================================================
// Class Declarations
//=============================================================================

class radPs2MemcardDrive : public radDrive,
                           public radSignedReader,
                           public radSignedWriter
{
public:

    //
    // Constructor / destructor.
    //
    radPs2MemcardDrive( const char* pdrivespec, radMemoryAllocator alloc );
    virtual ~radPs2MemcardDrive( void );

    void Lock( void );
    void Unlock( void );

    unsigned int GetCapabilities( void );

    const char* GetDriveName( void );

    unsigned int GetCreationSize( radMemcardInfo* memcardInfo, unsigned int size );

    CompletionStatus Initialize( void );

    CompletionStatus OpenSaveGame( const char*      fileName, 
                                   radFileOpenFlags flags, 
                                   bool             writeAccess,
                                   radMemcardInfo*  memcardInfo,
                                   unsigned int     maxSize,
                                   radFileHandle*   pHandle, 
                                   unsigned int*    pSize );

    CompletionStatus CloseFile( radFileHandle handle, const char* fileName );

    CompletionStatus CommitFile( radFileHandle  handle, const char* fileName );

    CompletionStatus ReadSignedBlock( radFileHandle  handle,
                                      const char*    fileName,
                                      unsigned int   block,
                                      unsigned int   position,
                                      unsigned int   numBytes,
                                      void*          pData, 
                                      radMemorySpace pDataSpace );

    unsigned int GetReadBlockSize( void );
    unsigned int GetReadHeaderSize( radFileHandle handle );

    CompletionStatus WriteSignedBlock( radFileHandle handle,
                                       const char* fileName,
                                       unsigned int block,
                                       unsigned int position,
                                       unsigned int numBytes,
                                       const void* pData,
                                       unsigned int* size,
                                       radMemorySpace pDataSpace );

    unsigned int GetWriteBlockSize( void );
    unsigned int GetWriteHeaderSize( radFileHandle handle );

    CompletionStatus DestroyFile( const char* fileName ); 

    CompletionStatus Format( void );

    CompletionStatus FindFirst( const char*                 searchSpec, 
                                IRadDrive::DirectoryInfo*   pDirectoryInfo, 
                                radFileDirHandle*           pHandle,
                                bool                        firstSearch );

    CompletionStatus FindNext( radFileDirHandle* pHandle, IRadDrive::DirectoryInfo* pDirectoryInfo );

    CompletionStatus FindClose( radFileDirHandle* pHandle );

    IMPLEMENT_SIGNED_READ;
    IMPLEMENT_SIGNED_WRITE;

private:
    //
    // Helper functions.
    //
    void SetMediaInfo( void );
    bool GetPort( int* port, int* slot );
    bool DeleteFiles( const char* subDir, int port, int slot );
    void BuildFileSpec( const char* fileName, char* fullName, unsigned int size, char** pName );
    int WaitForResult( void );
    bool SyncFunction( int result );
    bool ProcessError( int result );
    bool Open( radFileHandle* pHandle, unsigned int* pSize, char* pName, int port, int slot, int mode, bool create );
    bool CreateIcons( radMemcardInfo* memcardInfo, char* pName, int port, int slot );
    bool ReadIcons( radMemcardInfo* memcardInfo, char* pName, int port, int slot );

    char    m_DriveName[ sizeof( s_PS2MemcardDrive ) + 1 ];

    //
    // Need to keep the directory info block aligned to 64. 
    //
    unsigned char     m_DirSpace[ sizeof( sceMcTblGetDir ) + PS2MEMCARDDRIVE_TABLE_ALIGN ];
    sceMcTblGetDir*   m_pDirInfo;    
    radFileDirHandle* m_pDirHandle; // cache who did a find last

    unsigned int m_OpenFiles;
    bool         m_CardChanged;

    //
    // Helper function for block caching.
    //
    bool LoadBlock( radFileHandle handle, unsigned int block );
    bool FlushBlock( void );
    unsigned int GetTableSize( unsigned int fileSize );
    radCrc CalculateCRC( unsigned int fileSize );
    bool GetSizeFromHandle( radFileHandle handle, unsigned int* size );
    bool InitializeFile( radFileHandle handle, unsigned int size );
};

#endif // PS2MEMCARDDDRIVE_HPP

