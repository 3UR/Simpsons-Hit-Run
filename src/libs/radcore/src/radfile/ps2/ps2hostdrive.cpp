//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

//=============================================================================
//
// File:        ps2hostdrive.hpp
//
// Subsystem:	Radical File System
//
// Description:	This file contains the implementation of the ps2 host drive.
//
// Revisions:	
//

//=============================================================================
// Include Files
//=============================================================================

#include <sifdev.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include "ps2hostdrive.hpp"

//=============================================================================
// Public Functions 
//=============================================================================

//=============================================================================
// Function:    radPs2HostDriveFactory
//=============================================================================
// Description: This member is responsible for constructing a radPs2HostDrive object.
//
// Parameters:  pointer to receive drive object
//              pointer to the drive name
//              allocator
//              
// Returns:     
//------------------------------------------------------------------------------

void radPs2HostDriveFactory
( 
    radDrive**         ppDrive, 
    const char*        pDriveName,
    radMemoryAllocator alloc
)
{
    //
    // Simply constuct the drive object.
    //
    *ppDrive = new( alloc ) radPs2HostDrive( alloc );
    rAssert( *ppDrive != NULL );
}

//=============================================================================
// Public Member Functions
//=============================================================================

radPs2HostDrive::radPs2HostDrive( radMemoryAllocator alloc )
    : 
    radDrive( ),
    m_pMutex( NULL )
{
    //
    // Create a mutex for lock/unlock
    //
    radThreadCreateMutex( &m_pMutex, alloc );
    rAssert( m_pMutex != NULL );

    //
    // Create the drive thread.
    //
    m_pDriveThread = new( alloc ) radDriveThread( m_pMutex, alloc, 5 * 1024 );
    rAssert( m_pDriveThread != NULL );

    //
    // Not removeable, so set this up just once. We want to simulate the PS2 CD drive.
    //
    m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaPresent;
    m_MediaInfo.m_FreeSpace = UINT_MAX;
    m_MediaInfo.m_FreeFiles = UINT_MAX / PS2_HOST_SECTOR_SIZE;
    m_MediaInfo.m_SectorSize = PS2_HOST_SECTOR_SIZE;
    strncpy( m_MediaInfo.m_VolumeName, s_PS2HostDriveName, sizeof( m_MediaInfo.m_VolumeName )  );
    m_MediaInfo.m_VolumeName[ sizeof( m_MediaInfo.m_VolumeName ) - 1 ] = '\0';

    //
    // Set up our aligned buffer
    //
    rAssert( radMemorySpace_OptimalAlignment <= PS2_HOST_DRIVE_ALIGNMENT );
    rAssert( radFileOptimalMemoryAlignment <= PS2_HOST_DRIVE_ALIGNMENT );

    m_TransferBuffer = (unsigned char*) ::radMemoryRoundUp( (unsigned int) m_TransferBufferSpace, PS2_HOST_DRIVE_ALIGNMENT );
}

radPs2HostDrive::~radPs2HostDrive( void )
{
    m_pMutex->Release( );
    m_pDriveThread->Release( );
}

//=============================================================================
// Function:    radPs2HostDrive::Lock
//=============================================================================
// Description: Start a critical section
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radPs2HostDrive::Lock( void )
{
    m_pMutex->Lock( );
}

//=============================================================================
// Function:    radPs2HostDrive::Unlock
//=============================================================================
// Description: End a critical section
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radPs2HostDrive::Unlock( void )
{
    m_pMutex->Unlock( );
}

//=============================================================================
// Function:    radPs2HostDrive::GetCapabilities
//=============================================================================

unsigned int radPs2HostDrive::GetCapabilities( void )
{
    return ( radDriveWriteable | radDriveFile );
}

//=============================================================================
// Function:    radPs2HostDrive::GetDriveName
//=============================================================================

const char* radPs2HostDrive::GetDriveName( void )
{
    return s_PS2HostDriveName;
}

//=============================================================================
// Function:    radPs2HostDrive::GetReadBufferSectors
//=============================================================================

unsigned int radPs2HostDrive::GetReadBufferSectors( void )
{
    return PS2_HOST_DRIVE_TRANSFER_BUFFER_SECTORS;
}

//=============================================================================
// Function:    radPs2HostDrive::Initialize
//=============================================================================

radDrive::CompletionStatus radPs2HostDrive::Initialize( void )
{
    //
    // Nothing is needed!
    //
    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radPs2HostDrive::OpenFile
//=============================================================================

radDrive::CompletionStatus radPs2HostDrive::OpenFile
( 
    const char*        fileName, 
    radFileOpenFlags   flags, 
    bool               writeAccess, 
    radFileHandle*     pHandle, 
    unsigned int*      pSize 
)
{
    //
    // Set up the open flags.
    //
    int openFlags = writeAccess ? SCE_RDWR : SCE_RDONLY ;

    if( flags == CreateAlways )
    {
        openFlags |= ( SCE_CREAT | SCE_TRUNC );
    }
    else if( flags == OpenAlways )
    {
        openFlags |= SCE_CREAT;
    }

    //
    // Try to open the file.
    //
    char fullName[ radFileFilenameMax + 1 ];
    char* pName;
    BuildFileSpec( fileName, fullName, radFileFilenameMax + 1, &pName );
    int hostHandle = ::sceOpen( fullName, openFlags );

    //
    // check if it succeeded
    //
    if ( hostHandle < 0 )
    {
        if ( hostHandle == -ENOSPC )
        {
            m_LastError = NoFreeSpace;
        }
        else
        {
            m_LastError = FileNotFound;
        }
        return Error;
    }

    //
    // Find the size of the file by seeking to the end.
    //
    int size = ::sceLseek( hostHandle, 0, SCE_SEEK_END );
    if( size < 0 )
    {
        ::sceClose( hostHandle );
        m_LastError = HardwareFailure;
        return Error;
    }

    *pSize = (unsigned int) size;
    *pHandle = (unsigned int) hostHandle;

    //
    // We are finished!
    //
    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radPs2HostDrive::CloseFile
//=============================================================================

radDrive::CompletionStatus radPs2HostDrive::CloseFile( radFileHandle handle, const char* fileName )
{
    ::sceClose( (int) handle ); 
    
    return Complete;
}

//=============================================================================
// Function:    radPs2HostDrive::ReadAligned
//=============================================================================

radDrive::CompletionStatus radPs2HostDrive::ReadAligned
( 
    radFileHandle handle, 
    const char* fileName,
    unsigned int sector,
    unsigned int numSectors,
    void* pData, 
    radMemorySpace pDataSpace 
)
{
    rAssertMsg( pDataSpace == radMemorySpace_Ee || pDataSpace == radMemorySpace_Iop, 
                "radFileSystem: radPs2HostDrive: reads only supported for EE or IOP memory." );
    
    //
    // Seek to the position
    //
    if ( ::sceLseek( (int) handle, sector * m_MediaInfo.m_SectorSize, SCE_SEEK_SET ) < 0 )
    {
        m_LastError = HardwareFailure;
        return Error;
    }

    unsigned int readSize = numSectors * m_MediaInfo.m_SectorSize;
    
    //
    // We can only read into EE memory, so we must use the transfer buffer to read into IOP
    //
    if ( pDataSpace == radMemorySpace_Ee )
    {
#ifdef RADFILE_SPEW
        rReleasePrintf( "FILE: (optimal)  [%s], Bytes: [0x%x]\n", fileName, readSize );
#endif

        if ( ::sceRead( (int) handle, pData, readSize ) < 0 )
        {
            m_LastError = HardwareFailure;
            return Error;
        }
    }
    else
    {
        // We need to split the read size up into section that fit into our transfer buffer
        //
        unsigned int numReads = readSize / PS2_HOST_DRIVE_TRANSFER_BUFFER_SIZE;
        unsigned int location = sector;
        char* userBuffer = (char*) pData;
        
        for ( unsigned int i = 0; i < numReads; i++ )
        {
            if ( ReadBuffered( handle, fileName, location, PS2_HOST_DRIVE_TRANSFER_BUFFER_SECTORS, 0, 
                PS2_HOST_DRIVE_TRANSFER_BUFFER_SIZE, userBuffer, pDataSpace ) != Complete )
            {
                return Error;
            }
            
            userBuffer += PS2_HOST_DRIVE_TRANSFER_BUFFER_SIZE;
            location += PS2_HOST_DRIVE_TRANSFER_BUFFER_SECTORS;
        }

        //
        // Read the last bytes if the read size wasn't a multiple of the transfer buffer.
        //
        unsigned int endBytes = readSize % PS2_HOST_DRIVE_TRANSFER_BUFFER_SIZE;
        if ( endBytes > 0 )
        {
            unsigned int endSectors = numSectors - numReads * PS2_HOST_DRIVE_TRANSFER_BUFFER_SECTORS;
            if ( ReadBuffered( handle, fileName, location, endSectors, 0, 
                endBytes, userBuffer, pDataSpace ) != Complete )
            {
                return Error;
            }
        }
    }
    
    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radPs2HostDrive::ReadBuffered
//=============================================================================

radDrive::CompletionStatus radPs2HostDrive::ReadBuffered
( 
    radFileHandle handle,
    const char* fileName,
    unsigned int sector,
    unsigned int numSectors,
    unsigned int position,
    unsigned int numBytes,
    void* pData, 
    radMemorySpace pDataSpace 
)
{
    rAssertMsg( pDataSpace == radMemorySpace_Ee || pDataSpace == radMemorySpace_Iop, 
                "radFileSystem: radPs2HostDrive: reads only supported for EE or IOP memory." );
    
    //
    // Seek to the position
    //
    if ( ::sceLseek( (int) handle, sector * m_MediaInfo.m_SectorSize, SCE_SEEK_SET ) < 0 )
    {
        m_LastError = HardwareFailure;
        return Error;
    }

    //
    // Do the read.
    //
#ifdef RADFILE_SPEW    
    rReleasePrintf( "FILE: (buffered) [%s], Bytes: [0x%x]\n", fileName, numBytes );
#endif

    if ( ::sceRead( (int) handle, m_TransferBuffer, numSectors * m_MediaInfo.m_SectorSize ) < 0 )
    {
        m_LastError = HardwareFailure;
        return Error;
    }

    //
    // Transfer to the user's buffer.
    //
    IRadMemorySpaceCopyRequest* pMemCpyRequest = 
        radMemorySpaceCopyAsync( pData, 
                                 pDataSpace, 
                                 &m_TransferBuffer[ position ], 
                                 radMemorySpace_Local,
                                 numBytes );

    rAssert( pMemCpyRequest != NULL );
    pMemCpyRequest->AddRef( );

    //
    // Run the request
    //
    while ( !pMemCpyRequest->IsDone( ) )
    {
        ::radThreadSleep( 0 );
    }

    pMemCpyRequest->Release( );
    pMemCpyRequest = NULL;

    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radPs2HostDrive::WriteFile
//=============================================================================

radDrive::CompletionStatus radPs2HostDrive::WriteFile
(
    radFileHandle handle,
    const char* fileName,
    unsigned int position, 
    const void* pData, 
    unsigned int bytesToWrite, 
    unsigned int* bytesWritten, 
    unsigned int* pSize, 
    radMemorySpace pDataSpace
)
{
    rAssertMsg( pDataSpace == radMemorySpace_Local, 
                "radPs2HostDrive: External memory not supported for writes." );

    //
    // Seek to the position
    //
    if ( ::sceLseek( (int) handle, position, SCE_SEEK_SET ) < 0 )
    {
        m_LastError = HardwareFailure;
        return Error;
    }

    //
    // Do the write
    //
    if ( ::sceWrite( (int) handle, pData, bytesToWrite ) < 0 )
    {
        m_LastError = HardwareFailure;
        return Error;
    }

    int newSize = ::sceLseek( handle, 0, SCE_SEEK_END );
    if( newSize < 0 )
    {
        m_LastError = HardwareFailure;
        return Error;
    }

    *pSize = (unsigned int) newSize;
    *bytesWritten = bytesToWrite;

    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radPs2HostDrive::DestroyFile
//=============================================================================

radDrive::CompletionStatus radPs2HostDrive::DestroyFile( const char* fileName )
{
    rDebugPrintf( "Warning: file deletion is no longer supported by Sony.\n" );

    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Private Member Functions
//=============================================================================

//=============================================================================
// Function:    radPs2HostDrive::BuildFileSpec
//=============================================================================
// Description: returns a string in the form host:filename (no leading slash).
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radPs2HostDrive::BuildFileSpec( const char* fileName, char* fullName, unsigned int size, char** pName )
{
    int len = 5;
    strncpy( fullName, "host:", size - 1 );

    //
    // Strip leading slash.
    //
    if ( fileName[ 0 ] == '\\' )
    {
        strncpy( &fullName[ len ], &fileName[ 1 ] , size - len - 1 );
    }
    else
    {
        strncpy( &fullName[ len ], fileName, size - len - 1 );
    }
    fullName[ size - 1 ] = '\0';
    *pName = &fullName[ len ];
}
