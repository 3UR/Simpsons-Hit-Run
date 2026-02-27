//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        ps2memcarddrive.hpp
//
// Subsystem:	Radical File System
//
// Description:	This file contains the implementation of the PS2 memory card drive.
//
// Revisions:	
//
// Notes:       This drive handles data verification in 2 ways. First, when a file
//              is opened for writing, then "icon.sys" is renamed "~icon.sys".
//              It is named back when the file is committed.
//              Second, there is a distributed CRC. Every block has a CRC value
//              at the end. When a block is loaded or flushed, the CRC value is
//              calculated. Therefore, given the block size and the CRC size
//
//              the total size of the file is:
//                   numBlocks = ceil( dataSize / blockSize )
//                   fileSize = dataSize + ( numBlocks * crcSize )
//              
//              given fileSize, we can recover the other values:
//                   numBlocks = ceil( fileSize / ( blockSize + crcSize ) )
//                   dataSize = fileSize - ( numBlocks * crcSize )
//              
//              When an operation is done on the wrong card, m_CardChanged is
//              set. When all files are closed, then it is cleared.
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include <radplatform.hpp>
#include <string.h>
#include <radstring.hpp>
#include <ctype.h>
#include <radkey.hpp>
#include <libmtap.h>
#include "../common/drivethread.hpp"
#include "ps2memcarddrive.hpp"
#include "errno.h"

//=============================================================================
// Statics
//=============================================================================

radDriveThread*     s_pDriveThread = NULL;
IRadThreadMutex*    s_pMutex = NULL;

//
// Buffer needed for caching.
//
unsigned char  s_TransferBufferSpace[ PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE + PS2MEMCARDDRIVE_ALIGNMENT ];
unsigned char* s_TransferBuffer = NULL;

//
// For caching, we need the file handle we are accessing and the current position (in
// increments of the transfer buffer size).
// These dictate the state of the transfer buffer.
//
radFileHandle   s_CacheHandle = 0;
unsigned int    s_CachePosition = 0;

enum CacheState
{
    CacheInvalid,
    CacheClean,
    CacheDirty
};
CacheState      s_CacheState = CacheInvalid;

const char* iconSys = "icon.sys";
const char* tmpIconSys = "~icon.sys";

//=============================================================================
// Public Functions 
//=============================================================================

//=============================================================================
// Function:    radPs2MemcardDriveFactory
//=============================================================================
// Description: This member is responsible for constructing a radPs2MemcardDrive object.
//
// Parameters:  pointer to receive drive object
//              pointer to the drive name
//              allocator
//              
// Returns:     
//------------------------------------------------------------------------------

void radPs2MemcardDriveFactory
( 
    radDrive**         ppDrive, 
    const char*        pDriveName,
    radMemoryAllocator alloc
)
{
    //
    // Simply constuct the drive object.
    //
    *ppDrive = new( alloc ) radPs2MemcardDrive( pDriveName, alloc );
    rAssert( *ppDrive != NULL );
}

#ifdef RAD_DEBUG

//=============================================================================
// Function:    ValidPs2Name
//=============================================================================

static bool ValidPs2Name( char* pName )
{
    int len = strlen( pName );
    if ( len > 20 )
    {
        rDebugPrintf( "TRC Violation: filename [%s] too long.\n", pName );
        return false;
    }
    if ( !radStringMatchesWildCardPattern( pName, "BISLPS-?????*" ) &&
         !radStringMatchesWildCardPattern( pName, "BISLPM-?????*" ) &&
         !radStringMatchesWildCardPattern( pName, "BISCPS-?????*" ) &&
         !radStringMatchesWildCardPattern( pName, "BASLUS-?????*" ) &&         
         !radStringMatchesWildCardPattern( pName, "BASCUS-?????*" ) &&
         !radStringMatchesWildCardPattern( pName, "BESLES-?????*" ) &&    
         !radStringMatchesWildCardPattern( pName, "BESCES-?????*" ) )
    {
        rDebugPrintf( "TRC Violation: filename [%s] not in a liscenced format.\n", pName );
        rDebugPrintf( "Example: BASLUS-12345filename.\n" );
        return false;
    }
    
    int i;
    for ( i = 7; i < 11; i++ )
    {
        if ( pName[ i ] < '0' || pName[ i ] > '9' )
        {
            rDebugPrintf( "TRC Violation: filename [%s] has non numeric product code.\n", pName );
            rDebugPrintf( "Format is BASLUS-xxxxx, where xxxxx are digits.\n" );
            return false;
        }
    }
    for ( i = 12; i < len; i++ )
    {
        if ( pName[ i ] < 0x20 || pName[ i ] > 0x7E ||
             pName[ i ] == 0x3F || pName[ i ] == 0x2A || pName[ i ] == 0x2F )
        {
            rDebugPrintf( "TRC Violation: filename [%s] has invalid characters.\n", pName );
            rDebugPrintf( "Name must use ASCII character codes 0x20 through 0x7E excluding.\n" );
            rDebugPrintf( "characters 0x3F ('?'), 0x2A ('*') and 0x2F ('/').\n" );
            return false;
        }
    }
    return true;
}
#endif

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    radPs2MemcardDrive::radPs2MemcardDrive
//=============================================================================

radPs2MemcardDrive::radPs2MemcardDrive( const char* pdrivespec, radMemoryAllocator alloc )
    : 
    radDrive( ),
    m_pDirHandle( NULL ),
    m_OpenFiles( 0 ),
    m_CardChanged( false )
{
    //
    // First lets check if we have initialized the underlying system. This is done by 
    // the platform system since it needs to load IRXs.
    //
    static bool g_IsInitialized = false;
    if( !g_IsInitialized )
    {
        //
        // Load IRXs
        //
        rAssert( radPlatformGet( ) != NULL );
        radPlatformGet( )->LoadIrxModule( "sio2man.irx" );
        radPlatformGet( )->LoadIrxModule( "mcman.irx" );
        radPlatformGet( )->LoadIrxModule( "mcserv.irx" );

        //
        // Initialize the memcard system.
        //
        ::sceMcInit( );
		::sceMtapPortOpen(2);
		::sceMtapPortOpen(3);

        //
        // Initialize our transfer buffer. Only need to do it once.
        //
        s_TransferBuffer = (unsigned char*) ::radMemoryRoundUp( (unsigned int) s_TransferBufferSpace, PS2MEMCARDDRIVE_ALIGNMENT );

        g_IsInitialized = true;
    }

    //
    // Create the drive thread if it's not already done
    //
    if ( s_pDriveThread == NULL )
    {
        rAssert( s_pMutex == NULL );

        //
        // Create a mutex for lock/unlock
        //
        radThreadCreateMutex( &s_pMutex, alloc );
        rAssert( s_pMutex != NULL );

        //
        // Create the drive thread.
        //
        s_pDriveThread = new( alloc ) radDriveThread( s_pMutex, alloc );
        rAssert( s_pDriveThread != NULL );

        m_pDriveThread = s_pDriveThread;
    }
    else
    {
        m_pDriveThread = s_pDriveThread;
        m_pDriveThread->AddRef( );
    }
    
    //
    // We only accept one sector size
    //
    m_MediaInfo.m_SectorSize = PS2MEMCARDDRIVE_SECTOR_SIZE;

    //
    // Align the directory structure
    //
    m_pDirInfo = (sceMcTblGetDir*) ::radMemoryRoundUp( (unsigned int) m_DirSpace, PS2MEMCARDDRIVE_TABLE_ALIGN );

    //
    // Copy over the name of this drive.
    //
    strcpy( m_DriveName, pdrivespec );
}

//=============================================================================
// Function:    radPs2MemcardDrive::~radPs2MemcardDrive
//=============================================================================

radPs2MemcardDrive::~radPs2MemcardDrive( void )
{
    //
    // Release the drive thread, and destroy it if needed.
    //
    if ( m_pDriveThread->GetRefCount( ) == 1 )
    {
        s_pMutex->Release( );
        s_pMutex = NULL;
        s_pDriveThread->Release( );
        s_pDriveThread = NULL;
    }
    else
    {
        m_pDriveThread->Release( );
    }
}

//=============================================================================
// Function:    radPs2MemcardDrive::Lock
//=============================================================================
// Description: Start a critical section
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radPs2MemcardDrive::Lock( void )
{
    s_pMutex->Lock( );
}

//=============================================================================
// Function:    radPs2MemcardDrive::Unlock
//=============================================================================
// Description: End a critical section
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radPs2MemcardDrive::Unlock( void )
{
    s_pMutex->Unlock( );
}

//=============================================================================
// Function:    radPs2MemcardDrive::GetCapabilities
//=============================================================================

unsigned int radPs2MemcardDrive::GetCapabilities( void )
{
    return ( radDriveRemovable | radDriveWriteable | radDriveSaveGame | radDriveEnumerable | radDriveFormat );
}

//=============================================================================
// Function:    radPs2MemcardDrive::GetDriveName
//=============================================================================

const char* radPs2MemcardDrive::GetDriveName( void )
{
    return m_DriveName;
}

//=============================================================================
// Function:    radPs2MemcardDrive::Initialize
//=============================================================================

radDrive::CompletionStatus radPs2MemcardDrive::Initialize( void )
{
    if ( m_OpenFiles != 0 )
    {
        //
        // Check if there was a media change. Either we know the card was replaced,
        // or if the card isn't present, then it must have changed!
        //
        SetMediaInfo( );
        if ( m_CardChanged || m_MediaInfo.m_MediaState != IRadDrive::MediaInfo::MediaPresent )
        {
            m_CardChanged = true;
            m_LastError = m_MediaInfo.m_MediaState == IRadDrive::MediaInfo::MediaNotPresent ? NoMedia : WrongMedia;
            return Error;
        }
    }
    else
    {
        SetMediaInfo( );
        m_CardChanged = false;
    }

    //
    // Success
    //
    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radPs2MemcardDrive::OpenSaveGame
//=============================================================================

radDrive::CompletionStatus radPs2MemcardDrive::OpenSaveGame
( 
    const char*        fileName, 
    radFileOpenFlags   flags, 
    bool               writeAccess, 
    radMemcardInfo*    memcardInfo,
    unsigned int       maxSize,
    radFileHandle*     pHandle, 
    unsigned int*      pSize 
)
{
    //
    // Get the name and the port.
    //
    char fullName[ radFileFilenameMax + 1 ];
    char* pName;
    BuildFileSpec( fileName, fullName, radFileFilenameMax + 1, &pName );

    //
    // Check the filename for a valid license format.
    //
#ifdef RAD_DEBUG
    ValidPs2Name( pName );
#endif

    int port, slot;
    if ( !GetPort( &port, &slot ) )
    {
        m_LastError = NoMedia;
        return Error;
    }

    if ( m_OpenFiles == 0 )
    {
        //
        // We start by doing a quick operation on the card. PS2 needs this to calm down
        // if the card recently changed. Since no files are open yet, we want to accept
        // the current card.
        //
        ::sceMcGetInfo( port, slot, NULL, NULL, NULL );
        WaitForResult( );
    }

    //
    // Check if the directory exists.
    //
    if ( ::sceMcGetDir( port, slot, pName, 0, -1, m_pDirInfo ) < 0 )
    {
        m_LastError = HardwareFailure;
        return Error;
    }

    int dirNum = WaitForResult( );
    if ( dirNum < 0 )
    {
        ProcessError( dirNum );
        return Error;
    }

    //
    // Make the directory if needed and set open flags
    //
    int openFlags = writeAccess ? SCE_RDWR : SCE_RDONLY;

    if ( flags == OpenExisting )
    {
        if ( dirNum == 0 )
        {
            m_LastError = FileNotFound;
            return Error;
        }
    }
    else if ( flags == CreateAlways )
    {
        //
        // If the directory exists, then delete its files.
        //
        if ( dirNum > 0 )
        {
            if ( !DeleteFiles( pName, port, slot ) )
            {
                return Error;
            }

            if ( !SyncFunction( ::sceMcDelete( port, slot, pName ) ) )
            {
                return Error;
            }
        }
    }
    else if ( flags == OpenAlways )
    {
        // nothing to do.
    }
    else
    {
        rAssertMsg( false, "PS2MemcardDrive: invalid open flags." );
    }

    //
    // Do we need to create the files or just open them.
    //
    bool create = !( dirNum > 0 && ( flags == OpenExisting || flags == OpenAlways ) );

    if ( create )
    {
        //
        // Check if there's room.
        //
        int bytesFree;
        ::sceMcGetInfo( port, slot, NULL, &bytesFree, NULL );
        if ( ProcessError( WaitForResult( ) ) == false )
        {
            return Error;
        }

        if ( (unsigned int)bytesFree * PS2MEMCARDDRIVE_SECTOR_SIZE < GetCreationSize( memcardInfo, maxSize ) )
        {
            m_LastError = NoFreeSpace;
            return Error;
        }

        //
        // Create the directory.
        //
        if ( !SyncFunction( ::sceMcMkdir( port, slot, pName ) ) )
        {
            return Error;
        }

        //
        // Write the icon files.
        //
        if ( !CreateIcons( memcardInfo, pName, port, slot ) )
        {
            return Error;
        }
    }
    else
    {
        //
        // Read the icon.sys file and check existence of the icon files
        //
        if ( !ReadIcons( memcardInfo, pName, port, slot ) )
        {
            return Error;
        }
    }

    //
    // Create the name of the save game file directory
    //
    radFileHandle handle;
    unsigned int size;

    char saveName[ radFileFilenameMax + 1 ];
    int len = strlen( pName );
    strcpy( saveName, pName );
    saveName[ len ] = '/';

    //
    // Set up the name of icon.sys and flags for opening the file.
    // 
    if ( create )
    {
        //
        // We need to calculate the actual file size, including the crc table
        //
        unsigned int numBlocks = 
            ( maxSize + PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE - 1 ) / PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE;
        unsigned int fsize = maxSize + numBlocks * sizeof( radCrc );

        //
        // We need to create the file and set its size.
        //
        strcpy( &saveName[ len + 1 ], pName );
        if ( 
            !Open( &handle, &size, saveName, port, slot, SCE_RDWR | SCE_CREAT, true ) ||
            !InitializeFile( handle, fsize ) ||
            !SyncFunction( ::sceMcClose( handle ) )
           )
        {
            return Error;
        }

        //
        // icon.sys already has the temporary name, so rename it if we're not writing.
        //
        if ( !writeAccess )
        {
            strcpy( &saveName[ len + 1 ], tmpIconSys );
            if ( !SyncFunction( ::sceMcRename( port, slot, saveName, iconSys ) ) )
            {
                return Error;
            }
        }
    }
    else
    {
        //
        // Rename icon.sys to the temp name if we are writing. 
        //
        if ( writeAccess )
        {
            strcpy( &saveName[ len + 1 ], iconSys );
            if ( !SyncFunction( ::sceMcRename( port, slot, saveName, tmpIconSys ) ) )
            {
                return Error;
            }
        }
    }

    //
    // Open the file.
    //
    strcpy( &saveName[ len + 1 ], pName );
    if ( !Open( &handle, &size, saveName, port, slot, openFlags, create ) )
    {
        return Error;
    }

    *pHandle = handle;
    *pSize = size - GetTableSize( size );

    m_OpenFiles++;

    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radPs2MemcardDrive::CloseFile
//=============================================================================

radDrive::CompletionStatus radPs2MemcardDrive::CloseFile( radFileHandle handle, const char* fileName )
{
    //
    // We don't report failure for this operation.
    //
    CommitFile( handle, fileName );

    //
    // Invalidate the cache
    //
    if ( s_CacheHandle == handle )
    {
        s_CacheState = CacheInvalid;
    }

    ::sceMcClose( (int) handle );
    WaitForResult( );

    m_OpenFiles--;

    return Complete;
}

//=============================================================================
// Function:    radPs2MemcardDrive::CommitFile
//=============================================================================

radDrive::CompletionStatus radPs2MemcardDrive::CommitFile( radFileHandle handle, const char* fileName )
{
    //
    // Get the port and make the full name of the icon.sys file
    //
    int port, slot;
    if ( !GetPort( &port, &slot ) )
    {
        m_LastError = NoMedia;
        return Error;
    }

    char fullName[ radFileFilenameMax + 1 ];
    char* pName;
    BuildFileSpec( fileName, fullName, radFileFilenameMax + 1, &pName );

    char saveName[ radFileFilenameMax + 1 ];
    int len = strlen( pName );
    strcpy( saveName, pName );
    saveName[ len ] = '/';
    strcpy( &saveName[ len + 1 ], tmpIconSys );

    //
    // Check if tmpIconSys exists
    //
    if ( ::sceMcGetDir( port, slot, saveName, 0, -1, m_pDirInfo ) < 0 )
    {
        m_LastError = HardwareFailure;
        return Error;
    }

    int dirNum = WaitForResult( );
    if( dirNum < 0 )
    {
        {
            ProcessError( dirNum );
        }
        return Error;
    }

    //
    // Rename tmpIconSys to iconSys and flush the handle.
    //
    if ( dirNum > 0 )
    {
        if ( 
            !FlushBlock( ) ||
            !SyncFunction( ::sceMcFlush( (int) handle ) ) ||
            !SyncFunction( ::sceMcRename( port, slot, saveName, iconSys ) )
           )
        {
            return Error;
        }
    }

    m_LastError = Success;   
    return Complete;
}

//=============================================================================
// Function:    radPs2MemcardDrive::GetReadBlockSize
//=============================================================================

unsigned int radPs2MemcardDrive::GetReadBlockSize( void )
{
    return PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE - sizeof( radCrc );
}

//=============================================================================
// Function:    radPs2MemcardDrive::GetReadHeaderSize
//=============================================================================

unsigned int radPs2MemcardDrive::GetReadHeaderSize( radFileHandle handle )
{
    return 0;
}

//=============================================================================
// Function:    radPs2MemcardDrive::GetWriteBlockSize
//=============================================================================

unsigned int radPs2MemcardDrive::GetWriteBlockSize( void )
{
    return PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE - sizeof( radCrc );
}

//=============================================================================
// Function:    radPs2MemcardDrive::GetWriteHeaderSize
//=============================================================================

unsigned int radPs2MemcardDrive::GetWriteHeaderSize( radFileHandle handle )
{
    return 0;
}

//=============================================================================
// Function:    radPs2MemcardDrive::ReadSignedBlock
//=============================================================================

radDrive::CompletionStatus radPs2MemcardDrive::ReadSignedBlock
(
    radFileHandle   handle,
    const char*     fileName,
    unsigned int    block,
    unsigned int    position,
    unsigned int    numBytes,
    void*           pData, 
    radMemorySpace  pDataSpace
)
{
    rAssertMsg( pDataSpace == radMemorySpace_Local,
                "radFileSystem: radPs2MemcardDrive: External memory not supported for reads." );

    //
    // Check if we're reading past the end of the file.
    //
    unsigned int size;
    if ( !GetSizeFromHandle( handle, &size ) )
    {
        return Error;
    }

    if ( block * PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE >= size )
    {
        m_LastError = Success;
        return Complete;
    }

    if ( !LoadBlock( handle, block ) )
    {
        return Error;
    }

    memcpy( pData, &s_TransferBuffer[ position ], numBytes );

    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radPs2MemcardDrive::WriteSignedBlock
//=============================================================================

radDrive::CompletionStatus radPs2MemcardDrive::WriteSignedBlock
( 
    radFileHandle handle,
    const char*  fileName,
    unsigned int block,
    unsigned int position,
    unsigned int numBytes,
    const void* pData,
    unsigned int* pSize,
    radMemorySpace pDataSpace
)
{
    rAssertMsg( pDataSpace == radMemorySpace_Local, 
                "radFileSystem: radPs2MemcardDrive: External memory not supported for writes." );

    //
    // Get the file size
    //
    unsigned int size;
    if ( !GetSizeFromHandle( handle, &size ) )
    {
        return Error;
    }
    size -= GetTableSize( size );

    rAssertMsg( 
        block * ( PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE - sizeof( radCrc ) ) + position + numBytes <= size,
        "radFileSystem: radPs2MemcardDrive: Cannot change file size." );

    if ( !LoadBlock( handle, block ) )
    {
        return Error;
    }

    //
    // Transfer the user's data into the transfer buffer.
    //
    s_CacheState = CacheInvalid;
    memcpy( &s_TransferBuffer[ position ], pData, numBytes );
    s_CacheState = CacheDirty;

    *pSize = size;

    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radPs2MemcardDrive::DestroyFile
//=============================================================================

radDrive::CompletionStatus radPs2MemcardDrive::DestroyFile( const char* fileName )
{
    //
    // Get the name and the port.
    //
    char fullName[ radFileFilenameMax + 1 ];
    char* pName;
    BuildFileSpec( fileName, fullName, radFileFilenameMax + 1, &pName );

    int port, slot;
    if ( !GetPort( &port, &slot ) )
    {
        m_LastError = NoMedia;
        return Error;
    }

    if ( m_OpenFiles == 0 )
    {
        //
        // Let the card calm down if it changed.
        //
        ::sceMcGetInfo( port, slot, NULL, NULL, NULL );
        WaitForResult( );
    }

    //
    // Check if the directory exists.
    //
    if ( ::sceMcGetDir( port, slot, pName, 0, -1, m_pDirInfo ) < 0 )
    {
        m_LastError = HardwareFailure;
        return Error;
    }

    int dirNum = WaitForResult( );
    if ( dirNum < 0 )
    {
        ProcessError( dirNum );
        return Error;
    }
    
    if ( dirNum != 1 )
    {
        m_LastError = FileNotFound;
        return Error;
    }

    //
    // Destroy the contents of the directory and the directory itself.
    //
    if ( 
        !DeleteFiles( pName, port, slot ) ||
        !SyncFunction( ::sceMcDelete( port, slot, pName ) ) 
       )
    {
        return Error;
    }

    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radPs2MemcardDrive::Format
//=============================================================================

radDrive::CompletionStatus radPs2MemcardDrive::Format( void )
{
    rAssert( m_OpenFiles == 0 );

    int port, slot;
    if ( !GetPort( &port, &slot ) )
    {
        m_LastError = NoMedia;
        return Error;
    }

    if ( m_OpenFiles == 0 )
    {
        //
        // Let the card calm down if it changed.
        //
        ::sceMcGetInfo( port, slot, NULL, NULL, NULL );
        WaitForResult( );
    }

    if ( !SyncFunction( ::sceMcFormat( port, slot ) ) )
    {
        return Error;
    }

    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radPs2MemcardDrive::FindFirst
//=============================================================================

radDrive::CompletionStatus radPs2MemcardDrive::FindFirst
( 
    const char*                 searchSpec, 
    IRadDrive::DirectoryInfo*   pDirectoryInfo, 
    radFileDirHandle*           pHandle,
    bool                        firstSearch 
)
{
    //
    // Build the full filename
    //
    char fullName[ radFileFilenameMax + 1 ];
    char* pName;
    BuildFileSpec( searchSpec, fullName, radFileFilenameMax + 1, &pName );
    rAssertMsg( strlen( pName ) <= RAD_PS2_SPEC_LEN, "radFileSystem: long search spec." );

    //
    // There is no directory handle, so the search spec must be the wildcard part
    // Fill in the directory handle:
    //      m_pSpec -- the search spec
    //      m_Offset -- current entry number
    //      m_RemSect -- either 1 or 0 if there are entries or not
    //      other handle variables are not used.
    //
    strcpy( pHandle->m_pSpec, pName );
    pHandle->m_Offset = 0;
    pHandle->m_RemSect = 1;

    m_pDirHandle = pHandle;
    return FindNext( pHandle, pDirectoryInfo );
}

//=============================================================================
// Function:    radPs2MemcardDrive::FindNext
//=============================================================================

radDrive::CompletionStatus radPs2MemcardDrive::FindNext
( 
    radFileDirHandle* pHandle, 
    IRadDrive::DirectoryInfo* pDirectoryInfo
)
{
    //
    // Check if there's anything to be done
    //
    if ( pHandle->m_RemSect == 0 )
    {
        pDirectoryInfo->m_Name[ 0 ] = '\0';
        pDirectoryInfo->m_Type = IRadDrive::DirectoryInfo::IsDone;
        m_LastError = Success;
        return Complete;
    }

    int port, slot;
    if ( !GetPort( &port, &slot ) )
    {
        m_LastError = NoMedia;
        return Error;
    }

    if ( m_OpenFiles == 0 )
    {
        //
        // Let the card calm down if it changed.
        //
        ::sceMcGetInfo( port, slot, NULL, NULL, NULL );
        WaitForResult( );
    }

    //
    // If someone else did a find, we need to catch up
    //
    if ( m_pDirHandle != pHandle )
    {
        m_pDirHandle = pHandle;
        for ( unsigned int i = 0; i < pHandle->m_Offset; i++ )
        {
            if ( ::sceMcGetDir( port, slot, pHandle->m_pSpec, i, 1, m_pDirInfo ) < 0 )
            {
                m_LastError = HardwareFailure;
                return Error;
            }

            int dirNum = WaitForResult( );
            if ( dirNum <= 0 )
            {
                if ( dirNum == 0 )
                {
                    m_LastError = FileNotFound;
                }
                else
                {
                    ProcessError( dirNum );
                }
                return Error;
            }
        }
    }

    //
    // Now get the next entry
    //
    if ( ::sceMcGetDir( port, slot, pHandle->m_pSpec, pHandle->m_Offset, 1, m_pDirInfo ) < 0 )
    {
        m_LastError = HardwareFailure;
        return Error;
    }

    int dirNum = WaitForResult( );
    if ( dirNum < 0 )
    {
        ProcessError( dirNum );
        return Error;
    }

    //
    // Found an entry, or reached the end
    //
    if ( dirNum == 0 )
    {
        pHandle->m_RemSect = 0;
        pDirectoryInfo->m_Name[ 0 ] = '\0';
        pDirectoryInfo->m_Type = IRadDrive::DirectoryInfo::IsDone;
    }
    else
    {
        pHandle->m_Offset++;
        strncpy( pDirectoryInfo->m_Name, (char*) m_pDirInfo->EntryName, sizeof( pDirectoryInfo->m_Name ) );
        pDirectoryInfo->m_Name[ sizeof( pDirectoryInfo->m_Name ) ] = '\0';
        pDirectoryInfo->m_Type = IRadDrive::DirectoryInfo::IsSaveGame;
    }

    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radPs2MemcardDrive::FindClose
//=============================================================================

radDrive::CompletionStatus radPs2MemcardDrive::FindClose( radFileDirHandle* pHandle )
{
    return Complete;
}

//=============================================================================
// Function:    radPs2MemcardDrive::GetCreationSize
//=============================================================================
// Description: Get the actual size for creation.
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

unsigned int radPs2MemcardDrive::GetCreationSize( radMemcardInfo* memcardInfo, unsigned int size )
{
    rAssertMsg( memcardInfo != NULL &&
        memcardInfo->m_ListIcon != NULL && memcardInfo->m_ListIconSize > 0 &&
        memcardInfo->m_CopyIcon != NULL && memcardInfo->m_CopyIconSize > 0 &&
        memcardInfo->m_DelIcon != NULL && memcardInfo->m_DelIconSize > 0,
        "radFile: invalid radMemcardInfo." );

    //
    // Add in the size of the icon files, and icon.sys, and round each one up to 1024
    //
    unsigned int csize = 
        ::radMemoryRoundUp( memcardInfo->m_ListIconSize, PS2MEMCARDDRIVE_SECTOR_SIZE ) +
        ::radMemoryRoundUp( memcardInfo->m_CopyIconSize, PS2MEMCARDDRIVE_SECTOR_SIZE ) +
        ::radMemoryRoundUp( memcardInfo->m_DelIconSize, PS2MEMCARDDRIVE_SECTOR_SIZE ) +
        ::radMemoryRoundUp( sizeof(memcardInfo->m_IconSys), PS2MEMCARDDRIVE_SECTOR_SIZE );

    //
    // Add in the size of the save game, including our crc checks, rounded up to 1024
    //
    unsigned int numBlocks = 
        ( size + PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE - 1 ) / PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE;
    csize += ::radMemoryRoundUp( size + numBlocks * sizeof( radCrc ), PS2MEMCARDDRIVE_SECTOR_SIZE );

    //
    // Add in the number of clusters used by the file entries 
    // We have 5 files and there are 2 entries per cluster.
    //
    csize += 3 * PS2MEMCARDDRIVE_SECTOR_SIZE;

    //
    // Add in the number of clusters used in the directory entry (2 clusters)
    //
    csize += 2 * PS2MEMCARDDRIVE_SECTOR_SIZE;

    return csize;
}

//=============================================================================
// Private Members
//=============================================================================

//=============================================================================
// Function:    radPs2MemcardDrive::DeleteFiles
//=============================================================================
// Description: Delete all files in a directory. Assumes that subDir exists.
//              Sets m_LastError and returns true on Success.
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

bool radPs2MemcardDrive::DeleteFiles( const char* subDir, int port, int slot )
{
    //
    // Make the search spec
    //
    char spec[ radFileFilenameMax + 1 ];
    int len = strlen( subDir );
    strcpy( spec, subDir );
    strcpy( &spec[ len ], "/*" );

    //
    // Set up the subdirectory name for deletion
    //
    char name[ radFileFilenameMax + 1 ];
    strcpy( name, spec );

    //
    // Find first.
    //
    m_pDirHandle = NULL;
    if ( ::sceMcGetDir( port, slot, spec, 0, 1, m_pDirInfo ) < 0 )
    {
        m_LastError = HardwareFailure;
        return false;
    }

    int dirNum = WaitForResult( );
    if ( dirNum < 0 )
    {
        ProcessError( dirNum );
        return false;
    }

    while ( dirNum > 0 )
    {
        //
        // delete the file, unless it's '.' or '..'
        //
        if ( strcmp( (char*) m_pDirInfo->EntryName, "." ) != 0 &&
             strcmp( (char*) m_pDirInfo->EntryName, ".." ) != 0 )
        {
            strcpy( &name[ len + 1 ], (char*) m_pDirInfo->EntryName );
            if ( !SyncFunction( ::sceMcDelete( port, slot, name ) ) )
            {
                return false;
            }
        }
        
        //
        // Find next.
        //
        if ( ::sceMcGetDir( port, slot, spec, 1, 1, m_pDirInfo ) < 0 )
        {
            m_LastError = HardwareFailure;
            return false;
        }

        dirNum = WaitForResult( );
        if ( dirNum < 0 )
        {
            ProcessError( dirNum );
            return false;
        }
    }

    m_LastError = Success;
    return true;
}

//=============================================================================
// Function:    radPs2MemcardDrive::SetMediaInfo
//=============================================================================

void radPs2MemcardDrive::SetMediaInfo( void )
{
    m_MediaInfo.m_VolumeName[ 0 ] = '\0';
    m_MediaInfo.m_FreeSpace = 0;
    m_MediaInfo.m_FreeFiles = 0;

    int port, slot;
    if ( !GetPort( &port, &slot ) )
    {
        m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaNotPresent;
        return;
    }

    //
    // get the memory card info
    //
    int type, free, format;
    int result;
    while ( ::sceMcGetInfo( port, slot, &type, &free, &format ) != 0 )
    {
        //
        // The operation failed. This shouldn't happen
        //
        rDebugString( "PS2MemcardDrive: getting media info failed. This shouldn't have happened.\n" );
    }

    //
    // Wait for completion
    //
    result = WaitForResult( );

    //
    // Check for a card that is present and valid
    //
    switch( type )
    {
    //
    // No memory card
    //
    case sceMcTypeNoCard:
        m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaNotPresent;
        return;

    //
    // there is a memory card, but we should keep checking.
    //
    case sceMcTypePS2:
        m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaPresent;
        break;

    //
    // Unsupported types
    //
    case sceMcTypePS1:
    case sceMcTypePDA:
    default:
        m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaNotPresent;
    //     m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaWrongType;	 for srr2 we don't want to know this card
        return;
    }

    //
    // check if it is formatted
    //
    if ( format != 1 )
    {
        m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaNotFormatted;
        return;
    }

    //
    // check for card change
    //
    if ( result < 0 )
    {
        if ( result == sceMcResChangedCard )
        {
            rAssert( m_MediaInfo.m_MediaState == IRadDrive::MediaInfo::MediaPresent );
            m_CardChanged = true;
        }
        else if ( result == sceMcResNoFormat )
        {
            rAssert( m_MediaInfo.m_MediaState == IRadDrive::MediaInfo::MediaNotFormatted );
            m_CardChanged = true;
            return;
        }
        else
        {
            //
            // The operation failed.
            //
            m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaNotPresent;
            return;
        }
    }

    //
    // We have a valid card! Fill in it's free space and name.
    //
    m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaPresent;
    strncpy( m_MediaInfo.m_VolumeName, m_DriveName, sizeof( m_MediaInfo.m_VolumeName ) );
    m_MediaInfo.m_VolumeName[ sizeof( m_MediaInfo.m_VolumeName) - 1 ] = '\0';
    m_MediaInfo.m_FreeSpace = free * PS2MEMCARDDRIVE_SECTOR_SIZE;
    m_MediaInfo.m_FreeFiles = free;
}

//=============================================================================
// Function:    radPs2MemcardDrive::GetPort
//=============================================================================

bool radPs2MemcardDrive::GetPort( int* port, int* slot )
{
    //
    // Based on the drive name, lets map it to port and slot.
    //
    *port = ( int )( m_DriveName[ PS2MEMCARDDRIVE_PORT_LOC ] - '1' );   
    *slot = ( int )( m_DriveName[ PS2MEMCARDDRIVE_SLOT_LOC ] - 'A' );
    rAssert( (*port >= 0) && (*port <= 1) );
    rAssert( (*slot >= 0) && (*slot <= 3) );

    if ( *slot >= ::sceMcGetSlotMax( *port ) )
    {
        return false;
    }

    return true;
}

//=============================================================================
// Function:    radPs2MemcardDrive::BuildFileSpec
//=============================================================================
// Description: drivename:filename (no initial slash)
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radPs2MemcardDrive::BuildFileSpec( const char* fileName, char* fullName, unsigned int size, char** pName )
{
    int len = strlen( m_DriveName );
    strncpy( fullName, m_DriveName, size - 1 );

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

    //
    // We can't have subdirectories, so assert on that.
    //
    rAssertMsg( strchr( fullName, '\\' ) == NULL, "PS2MemcardDrive: subdirectories are not supported." );
}

//=============================================================================
// Function:    radPs2MemcardDrive::WaitForResult
//=============================================================================
// Description: Wait for an asynchronous function to complete and return the error code
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

int radPs2MemcardDrive::WaitForResult( void )
{
    //
    // Wait for completion
    //
    int cmd, result;
    ::sceMcSync( 0, &cmd, &result );

    return result;
}

//=============================================================================
// Function:    radPs2MemcardDrive::SyncFunction
//=============================================================================
// Description: wait for an synchronous function to complete, and parse its error
//              code. Usage: SyncFunction( ::sceMc*( ) ). Sets m_LastError and
//              returns true on success.
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

bool radPs2MemcardDrive::SyncFunction( int result )
{
    if ( result < 0 )
    {
        m_LastError = HardwareFailure;
        return false;
    }

    int syncResult = WaitForResult( );
    if ( syncResult < 0 )
    {
        ProcessError( syncResult );
        return false;
    }

    return true;
}

//=============================================================================
// Function:    radPs2MemcardDrive::ProcessError
//=============================================================================
// Description: Sets m_LastError appropriately.
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

bool radPs2MemcardDrive::ProcessError( int result )
{
    switch( result )
    {
    case sceMcResSucceed:
        m_LastError = Success;
        return true;

    case sceMcResChangedCard:
        m_CardChanged = true;
        m_LastError = WrongMedia;
        return false;

    case sceMcResNoFormat:
        m_LastError = MediaNotFormatted;
        return false;

    case sceMcResFullDevice:
        m_LastError = NoFreeSpace;
        return false;

    case sceMcResNoEntry:
        m_LastError = FileNotFound;
        return false;

    case sceMcResDeniedPermit:
        rDebugString( "PS2MemcardDrive: permission denied.\n" );
        m_LastError = FileNotFound;
        return false;

    case sceMcResNotEmpty:
        rDebugString( "PS2MemcardDrive: internal error.\n" );
        m_LastError = HardwareFailure;
        return false;

    case sceMcResUpLimitHandle:
        rDebugString( "PS2MemcardDrive: ran out of file handles.\n" );
        m_LastError = HardwareFailure;
        return false;

    case sceMcResFailReplace:
        m_LastError = MediaCorrupt;
        return false;

    case ( 1 - EACCES ):
    {
        //
        // The card was pulled out! See if any card is present.
        //
        m_CardChanged = true;

        int port, slot;
        GetPort( &port, &slot );

        int type = sceMcTypeNoCard;
        ::sceMcGetInfo( port, slot, &type, NULL, NULL );
        WaitForResult( );

        if ( type == sceMcTypeNoCard )
        {
            m_LastError = NoMedia;
        }
        else
        {
            m_LastError = WrongMedia;
        }
        return false;
    }

    default:
        m_LastError = NoMedia;
        return false;
    }
}

//=============================================================================
// Function:    radPs2MemcardDrive::CreateIcons
//=============================================================================
// Description: Create the icon files, using the temporary icon file name.
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

bool radPs2MemcardDrive::CreateIcons
( 
    radMemcardInfo* memcardInfo, 
    char* pName, 
    int port, 
    int slot
)
{
    //
    // If we are creating the file, we need something to write
    //
    rAssertMsg( memcardInfo != NULL &&
        memcardInfo->m_ListIcon != NULL && memcardInfo->m_ListIconSize > 0 &&
        memcardInfo->m_CopyIcon != NULL && memcardInfo->m_CopyIconSize > 0 &&
        memcardInfo->m_DelIcon != NULL && memcardInfo->m_DelIconSize > 0,
        "radFile: cannot create save game without a radMemcardInfo." );

    int iconOpenFlags = SCE_RDWR | SCE_CREAT;

    radFileHandle handle;
    unsigned int size;

    char saveName[ radFileFilenameMax + 1 ];
    int len = strlen( pName );
    strcpy( saveName, pName );
    saveName[ len ] = '/';

    ////////////////////////////////////////////////////////////////
    // Create the icon.sys file.
    ////////////////////////////////////////////////////////////////
    strcpy( &saveName[ len + 1 ], tmpIconSys );

    if ( 
        !Open( &handle, &size, saveName, port, slot, iconOpenFlags, true ) ||
        !SyncFunction( ::sceMcSeek( (int) handle, 0, 0 ) ) ||
        !SyncFunction( ::sceMcWrite( (int) handle, (void*) &(memcardInfo->m_IconSys), sizeof(memcardInfo->m_IconSys) ) ) ||
        !SyncFunction( ::sceMcClose( (int) handle ) )
       )
    {
        return false;
    }

    ////////////////////////////////////////////////////////////////
    // Create  the list icon file.
    ////////////////////////////////////////////////////////////////
    unsigned int iconSize = memcardInfo->m_ListIconSize;
    void* icon = memcardInfo->m_ListIcon;
    strcpy( &saveName[ len + 1 ], memcardInfo->m_IconSys.m_ListIconName );
    
    if ( 
        !Open( &handle, &size, saveName, port, slot, iconOpenFlags, true ) ||
        !SyncFunction( ::sceMcSeek( (int) handle, 0, 0 ) ) ||
        !SyncFunction( ::sceMcWrite( (int) handle, icon, iconSize ) ) ||
        !SyncFunction( ::sceMcClose( (int) handle ) ) 
       )
    {
        return false;
    }

    ////////////////////////////////////////////////////////////////
    // Create the copy icon file.
    ////////////////////////////////////////////////////////////////
    iconSize = memcardInfo->m_CopyIconSize;
    icon = memcardInfo->m_CopyIcon;
    strcpy( &saveName[ len + 1 ], memcardInfo->m_IconSys.m_CopyIconName );
    
    if ( 
        !Open( &handle, &size, saveName, port, slot, iconOpenFlags, true ) ||
        !SyncFunction( ::sceMcSeek( (int) handle, 0, 0 ) ) ||
        !SyncFunction( ::sceMcWrite( (int) handle, icon, iconSize ) ) ||
        !SyncFunction( ::sceMcClose( (int) handle ) ) 
       )
    {
        return false;
    }
    
    ////////////////////////////////////////////////////////////////
    // Create the del icon file.
    ////////////////////////////////////////////////////////////////
    iconSize = memcardInfo->m_DelIconSize;
    icon = memcardInfo->m_DelIcon;
    strcpy( &saveName[ len + 1 ], memcardInfo->m_IconSys.m_DeleteIconName );
    
    if ( 
        !Open( &handle, &size, saveName, port, slot, iconOpenFlags, true ) ||
        !SyncFunction( ::sceMcSeek( (int) handle, 0, 0 ) ) ||
        !SyncFunction( ::sceMcWrite( (int) handle, icon, iconSize ) ) ||
        !SyncFunction( ::sceMcClose( (int) handle ) ) 
       )
    {
        return false;
    }

    return true;
}

//=============================================================================
// Function:    radPs2MemcardDrive::ReadIcons
//=============================================================================
// Description: Do a basic sanity check on the icons in a save game.
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

bool radPs2MemcardDrive::ReadIcons
( 
    radMemcardInfo* memcardInfo, 
    char* pName,
    int port, 
    int slot
)
{
    //
    // If we aren't given a memcardInfo, then make a temporary one to do the checks with
    //
    radMemcardInfo* pMemInfo = memcardInfo;
    radMemcardInfo tmpInfo;
    if ( memcardInfo == NULL )
    {
        pMemInfo = &tmpInfo;
    }

    int iconOpenFlags = SCE_RDONLY;

    radFileHandle handle;
    unsigned int size;
    int dirNum;

    char saveName[ radFileFilenameMax + 1 ];
    int len = strlen( pName );
    strcpy( saveName, pName );
    saveName[ len ] = '/';

    ////////////////////////////////////////////////////////////////
    // Read the icon.sys file.
    ////////////////////////////////////////////////////////////////
    strcpy( &saveName[ len + 1 ], iconSys );

    if ( 
        !Open( &handle, &size, saveName, port, slot, iconOpenFlags, false ) ||
        !SyncFunction( ::sceMcSeek( (int) handle, 0, 0 ) ) ||
        !SyncFunction( ::sceMcRead( (int) handle, (void*) &(pMemInfo->m_IconSys), sizeof(pMemInfo->m_IconSys) ) ) ||
        !SyncFunction( ::sceMcClose( (int) handle ) )
       )
    {
        return false;
    }

    ////////////////////////////////////////////////////////////////
    // Check the list icon file.
    ////////////////////////////////////////////////////////////////
    strcpy( &saveName[ len + 1 ], pMemInfo->m_IconSys.m_ListIconName );

    if ( ::sceMcGetDir( port, slot, saveName, 0, -1, m_pDirInfo ) < 0 )
    {
        m_LastError = HardwareFailure;
        return false;
    }

    dirNum = WaitForResult( );
    if( dirNum <= 0 )
    {
        if ( dirNum == 0 )
        {
            m_LastError = DataCorrupt;
        }
        else
        {
            ProcessError( dirNum );
        }
        return false;
    }

    ////////////////////////////////////////////////////////////////
    // Check the copy icon file.
    ////////////////////////////////////////////////////////////////
    strcpy( &saveName[ len + 1 ], pMemInfo->m_IconSys.m_CopyIconName );

    if ( ::sceMcGetDir( port, slot, saveName, 0, -1, m_pDirInfo ) < 0 )
    {
        m_LastError = HardwareFailure;
        return false;
    }

    dirNum = WaitForResult( );
    if( dirNum <= 0 )
    {
        if ( dirNum == 0 )
        {
            m_LastError = DataCorrupt;
        }
        else
        {
            ProcessError( dirNum );
        }
        return false;
    }

    ////////////////////////////////////////////////////////////////
    // Check the delete icon file.
    ////////////////////////////////////////////////////////////////
    strcpy( &saveName[ len + 1 ], pMemInfo->m_IconSys.m_DeleteIconName );

    if ( ::sceMcGetDir( port, slot, saveName, 0, -1, m_pDirInfo ) < 0 )
    {
        m_LastError = HardwareFailure;
        return false;
    }

    dirNum = WaitForResult( );
    if( dirNum <= 0 )
    {
        if ( dirNum == 0 )
        {
            m_LastError = DataCorrupt;
        }
        else
        {
            ProcessError( dirNum );
        }
        return false;
    }

    return true;
}

//=============================================================================
// Function:    radPs2MemcardDrive::Open
// Open a file and return its size and handle. If is isn't to be created and
// doesn't exists, sets m_LastError to DataCorrupt
//=============================================================================
// Description: Opens a file and returns its size and handle. If is not to be
//              created and doesn't exists, m_LastError is set to DataCorrupt.
//              m_LastError is set to the error in all cases, and returns true
//              on Success.
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

bool radPs2MemcardDrive::Open
( 
    radFileHandle* pHandle,
    unsigned int* pSize,
    char* pName,
    int port, 
    int slot,
    int mode,
    bool create
)
{
    //
    // Check for a corrupt card. 
    //
    if ( !create )
    {
        //
        // Check if the save game file exists.
        //
        if ( ::sceMcGetDir( port, slot, pName, 0, -1, m_pDirInfo ) < 0 )
        {
            m_LastError = HardwareFailure;
            return false;
        }

        int dirNum = WaitForResult( );
        if( dirNum <= 0 )
        {
            if ( dirNum == 0 )
            {
                m_LastError = DataCorrupt;
            }
            else
            {
                ProcessError( dirNum );
            }
            return false;
        }
    }

    //
    // We can now open the file.
    //
    if ( ::sceMcOpen( port, slot, pName, mode ) < 0 )
    {
        m_LastError = HardwareFailure;
        return false;
    }

    int hostHandle = WaitForResult( );
    if ( hostHandle < 0 )
    {
        ProcessError( hostHandle );
        return false;
    }

    //
    // Find the size of the file.
    //
    if ( !GetSizeFromHandle( (radFileHandle) hostHandle, pSize ) )
    {
        return false;
    }

    *pHandle = (radFileHandle) hostHandle;

    return true;
}

//=============================================================================
// Function:    radPs2MemcardDrive::LoadBlock
//=============================================================================

bool radPs2MemcardDrive::LoadBlock( radFileHandle handle, unsigned int block )
{
    //
    // Check if this is the block we already have.
    //
    if ( s_CacheState != CacheInvalid && s_CacheHandle == handle && s_CachePosition == block )
    {
        return true;
    }

    //
    // Get the file size
    //
    unsigned int fileSize;
    if ( !GetSizeFromHandle( handle, &fileSize ) )
    {
        return false;
    }

    if ( !FlushBlock( ) )
    {
        return false;
    }

    s_CacheState = CacheInvalid;

    //
    // Set up the read.
    //
    unsigned int start = block * PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE;    
    unsigned int end = start + PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE;
    end = end < fileSize ? end : fileSize;
    unsigned int size = end - start;
    end -= sizeof( radCrc );

    //
    // Read the block.
    //
    if (
        !SyncFunction( ::sceMcSeek( (int) handle, start, 0 ) ) ||
        !SyncFunction( ::sceMcRead( (int) handle, s_TransferBuffer, size ) )
       )
    {
        return false;
    }

    //
    // Get the crc.
    //
    radCrc crc;
    memcpy( &crc, &s_TransferBuffer[ end - start ], sizeof( crc ) );

    s_CacheHandle = handle;
    s_CachePosition = block;

    //
    // Verify the data.
    //
    if ( CalculateCRC( fileSize ) != crc )
    {
        m_LastError = DataCorrupt;
        return false;
    }

    s_CacheState = CacheClean;

    return true;
}

//=============================================================================
// Function:    radPs2MemcardDrive::FlushBlock
//=============================================================================

bool radPs2MemcardDrive::FlushBlock( void )
{
    if ( s_CacheState != CacheDirty )
    {
        return true;
    }

    //
    // Get the file size
    //
    unsigned int fileSize;
    if ( !GetSizeFromHandle( s_CacheHandle, &fileSize ) )
    {
        return false;
    }

    //
    // Calculate the new crc.
    //
    radCrc crc = CalculateCRC( fileSize );

    //
    // Set up the write.
    //
    unsigned int start = s_CachePosition * PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE;    
    unsigned int end = start + PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE;
    end = end < fileSize ? end : fileSize;
    unsigned int size = end - start;
    end -= sizeof( radCrc );

    //
    // Copy in the crc.
    //
    memcpy( &s_TransferBuffer[ end - start ], &crc, sizeof( radCrc ) );

    //
    // Write the block.
    //
    if (
        !SyncFunction( ::sceMcSeek( (int) s_CacheHandle, start, 0 ) ) ||
        !SyncFunction( ::sceMcWrite( (int) s_CacheHandle, s_TransferBuffer, size ) ) ||
        !SyncFunction( ::sceMcFlush( (int) s_CacheHandle ) )
       )
    {
        return false;
    }

    s_CacheState = CacheClean;

    return true;
}

//=============================================================================
// Function:    radPs2MemcardDrive::GetTableSize
//=============================================================================

unsigned int radPs2MemcardDrive::GetTableSize( unsigned int fileSize )
{
    unsigned int divisor = PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE + sizeof( radCrc );
    unsigned int numBlocks = ( fileSize + divisor - 1 ) / divisor;

    return numBlocks * sizeof( radCrc );
}

//=============================================================================
// Function:    radPs2MemcardDrive::CalculateCRC
//=============================================================================

radCrc radPs2MemcardDrive::CalculateCRC( unsigned int fileSize )
{
    //
    // We only do the CRC check on the user's data, so we must be careful not to
    // over-run into our CRC table.
    //
    unsigned int start = s_CachePosition * PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE;    
    unsigned int end = start + PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE;
    end = end < fileSize ? end : fileSize;
    end -= sizeof( radCrc );
    
    return radCRCCalculate( (char*) s_TransferBuffer, end - start );
}

//=============================================================================
// Function:    radPs2MemcardDrive::GetSizeFromHandle
//=============================================================================

bool radPs2MemcardDrive::GetSizeFromHandle( unsigned int handle, unsigned int* size )
{
    if ( ::sceMcSeek( (int) handle, 0, 2 ) < 0 )
    {
        m_LastError = HardwareFailure;
        return false;
    }

    int syncResult = WaitForResult( );
    if ( syncResult < 0 )
    {
        ProcessError( syncResult );
        return false;
    }

    *size = syncResult;
    return true;
}

//=============================================================================
// Function:    radPs2MemcardDrive::InitializeFile
//=============================================================================

bool radPs2MemcardDrive::InitializeFile( radFileHandle handle, unsigned int size )
{
    //
    // Set our transfer buffer to 0.
    //
    if ( !FlushBlock( ) )
    {
        return false;
    }
    s_CacheState = CacheInvalid;
    
    memset( s_TransferBuffer, 0, PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE );

    //
    // How much of the buffer has a crc check done on it.
    //
    unsigned int userSize = PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE - sizeof( radCrc );
    unsigned int fullSize = size;

    //
    // Add in the crc value.
    //
    if ( size > PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE )
    {
        radCrc crc = radCRCCalculate( (char*) s_TransferBuffer, userSize );
        memcpy( &s_TransferBuffer[ userSize ], &crc, sizeof( radCrc ) );
    }

    //
    // Seek to the beginning.
    //
    if ( !SyncFunction( ::sceMcSeek( (int) handle, 0, 0 ) ) )
    {
        return false;
    }

    //
    // Write all but the last block
    //
    unsigned int block = 0;
    while ( fullSize > PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE )
    {
        if ( !SyncFunction( ::sceMcWrite( (int) handle, s_TransferBuffer, PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE ) ) )
        {
            return false;
        }

        fullSize -= PS2MEMCARDDRIVE_TRANSFER_BUFFER_SIZE;
        block++;
    }

    //
    // Only the last block left.
    //
    if ( fullSize > 0 )
    {
        radCrc crc = radCRCCalculate( (char*) s_TransferBuffer, fullSize - sizeof( radCrc ) );
        memcpy( &s_TransferBuffer[ fullSize - sizeof( radCrc ) ], &crc, sizeof( radCrc ) );

        if ( !SyncFunction( ::sceMcWrite( (int) handle, s_TransferBuffer, fullSize ) ) )
        {
            return false;
        }
        block++;
    }
    
    s_CacheHandle = handle;
    s_CachePosition = block - 1;
    s_CacheState = CacheClean;  

    return true;
}
