//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        ps2hostdrive.hpp
//
// Subsystem:	Radical File System
//
// Description:	This file contains all definitions and classes relevant to
//              ps2 host drive.
//
// Revisions:	
//
//=============================================================================

#ifndef	PS2HOSTDDDRIVE_HPP
#define PS2HOSTDDRIVE_HPP

//=============================================================================
// Include Files
//=============================================================================

#include "../common/drive.hpp"
#include "../common/drivethread.hpp"
#include "../common/buffereddrive.hpp"

//=============================================================================
// Defines
//=============================================================================

//
// Sector size -- simulates the PS2 CD drive
//
#define PS2_HOST_SECTOR_SIZE 2048

//
// Got a big transfer buffer because we can't read directly into IOP
//
#define PS2_HOST_DRIVE_TRANSFER_BUFFER_SECTORS  128
#define PS2_HOST_DRIVE_TRANSFER_BUFFER_SIZE (PS2_HOST_DRIVE_TRANSFER_BUFFER_SECTORS * PS2_HOST_SECTOR_SIZE)

//
// Set up the alignment for our transfer buffer
//
#define PS2_HOST_DRIVE_ALIGNMENT   128

//=============================================================================
// Statics
//=============================================================================

static const char s_PS2HostDriveName[ ] = "HOSTDRIVE:";

//=============================================================================
// Public Functions
//=============================================================================

//
// Every physical drive type must provide a drive factory.
//
void radPs2HostDriveFactory( radDrive** ppDrive, const char* driveSpec, radMemoryAllocator alloc );

//=============================================================================
// Class Declarations
//=============================================================================

class radPs2HostDrive : public radBufferedReader,
                        public radDrive
{
public:

    //
    // Constructor / destructor.
    //
    radPs2HostDrive( radMemoryAllocator alloc );
    virtual ~radPs2HostDrive( void );

    void Lock( void );
    void Unlock( void );

    //
    // This member reports this physical drives capabilities
    //
    unsigned int GetCapabilities( void );

    const char* GetDriveName( void );

    CompletionStatus Initialize( void );

    CompletionStatus OpenFile( const char*        fileName, 
                               radFileOpenFlags   flags, 
                               bool               writeAccess, 
                               radFileHandle*     pHandle, 
                               unsigned int*      pSize );

    CompletionStatus CloseFile( radFileHandle handle, const char* fileName );

    CompletionStatus ReadAligned( radFileHandle handle, 
                                  const char* fileName,
                                  unsigned int sector,
                                  unsigned int numSectors,
                                  void* pData, 
                                  radMemorySpace pDataSpace );
 
    CompletionStatus ReadBuffered( radFileHandle handle,
                                   const char* fileName,
                                   unsigned int sector,
                                   unsigned int numSectors,
                                   unsigned int position,
                                   unsigned int numBytes,
                                   void* pData, 
                                   radMemorySpace pDataSpace );

    unsigned int GetReadBufferSectors( void );

    CompletionStatus WriteFile( radFileHandle handle, 
                                const char* fileName,
                                unsigned int position, 
                                const void* pData, 
                                unsigned int bytesToWrite, 
                                unsigned int* bytesWritten, 
                                unsigned int* size, 
                                radMemorySpace pDataSpace );

    CompletionStatus DestroyFile( const char* filename );

    //
    // radBufferedReader
    //
    IMPLEMENT_BUFFERED_READ;

private:
    void BuildFileSpec( const char* fileName, char* fullName, unsigned int size, char** pName );

    //
    // This buffer is used to when the read request is not sector aligned.
    //
    unsigned char  m_TransferBufferSpace[ PS2_HOST_DRIVE_TRANSFER_BUFFER_SIZE + PS2_HOST_DRIVE_ALIGNMENT ];
    unsigned char* m_TransferBuffer;

    //
    // Mutex for critical sections
    //
    IRadThreadMutex*    m_pMutex;
};

#endif // PS2HOSTDDRIVE_HPP

