//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        xboxsavedrive.hpp
//
// Subsystem:	Radical File System
//
// Description:	This file contains the implementation of the xbox memcard drives
//              and the user hard drive.
//
// Revisions:	
//
// Notes:       XBox user drive is drive U: We store the actual size of the file
//              as a header, to make certain that the size hasn't changed. This
//              number is set to 0 when a file is created, and set appropriately
//              during a commit.
//              We can't use our CRC check, instead we must use the XBOX 
//              CRC stuff which is 20 bytes long. There is a distributed CRC. Every 
//              block has a CRC value at the end. When a block is loaded or flushed,
//              the CRC value is calculated. Therefore, given the block size and the 
//              CRC size.
//
//              the total size of the file is:
//                   numBlocks = ceil( dataSize / blockSize )
//                   fileSize = dataSize + ( numBlocks * crcSize )
//              
//              given fileSize, we can recover the other values:
//                   numBlocks = ceil( fileSize / ( blockSize + crcSize ) )
//                   dataSize = fileSize - ( numBlocks * crcSize )
//
//              Mounting: the card needs to be mounted before it can be used.
//              We mount the card before any operation. If the card is already
//              mounted we don't remount unless the card changed. Once a card
//              is pulled out, ops will return that there is no card. On re-init
//              the card will be unmounted, and nothing will continue until all
//              files are closed.
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include "xboxsavedrive.hpp"
#include <radplatform.hpp>
#include <limits.h>
#include <radstring.hpp>

//=============================================================================
// Statics
//=============================================================================

radDriveThread*     s_pDriveThread = NULL;
IRadThreadMutex*    s_pMutex = NULL;

//
// Buffer needed for caching.
//
unsigned char  s_TransferBufferSpace[ XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE + XBOXSAVEDRIVE_ALIGNMENT ];
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


static const char* s_SaveImage = "SaveImage.xbx";

//
// We need to keep a list of memory units which changed. The API will tell us the
// changes in ALL memory units since the last call. So we need a global list. Since
// all drive ops happen serially, there is no need for any protection.
//
union DeviceState
{
    struct
    {
        unsigned char m_PresentMask;
        unsigned char m_ChangedMask;
    }   m_Mask;

    //
    // This isn't necessary but helps in debugging. Just put it in a watch point
    // to see the state of everything.
    //
    struct
    {
        bool m_PresentPort1A : 1;
        bool m_PresentPort2A : 1;
        bool m_PresentPort3A : 1;
        bool m_PresentPort4A : 1;
        bool m_PresentPort1B : 1;
        bool m_PresentPort2B : 1;
        bool m_PresentPort3B : 1;
        bool m_PresentPort4B : 1;

        bool m_ChangedPort1A : 1;
        bool m_ChangedPort2A : 1;
        bool m_ChangedPort3A : 1;
        bool m_ChangedPort4A : 1;
        bool m_ChangedPort1B : 1;
        bool m_ChangedPort2B : 1;
        bool m_ChangedPort3B : 1;
        bool m_ChangedPort4B : 1;
    }   m_State;

    //
    // Init from a get device call.
    //
    void init( void )
    {
        DWORD mask = XGetDevices( XDEVICE_TYPE_MEMORY_UNIT );
        unsigned char presentMask = (unsigned char) ( ( mask & 0xF ) | ( ( mask >> 12 ) & 0xF0 ) & 0xFF );

        m_Mask.m_PresentMask = presentMask;
        m_Mask.m_ChangedMask = (unsigned char) 0;
    }

    //
    // Find changes from a get device changed call
    //
    void update( void )
    {
        DWORD insertMask;
        DWORD removeMask;
        if ( !XGetDeviceChanges( XDEVICE_TYPE_MEMORY_UNIT, &insertMask, &removeMask ) )
        {
            return;
        }

        //
        // Convert the masks into the right size.
        //
        unsigned char insertions = (unsigned char) ( ( insertMask & 0xF ) | ( ( insertMask >> 12 ) & 0xF0 ) & 0xFF );
        unsigned char removals = (unsigned char) ( ( removeMask & 0xF ) | ( ( removeMask >> 12 ) & 0xF0 ) & 0xFF );

        //
        // Check if the input is present. If it was both inserted and removed, then it
        // didn't change state, if it's one or the other, then it did.
        //
        m_Mask.m_PresentMask ^= insertions ^ removals;
        m_Mask.m_ChangedMask |= insertions | removals;
    }

    bool IsPresent( int port, int slot )
    {
        unsigned char bit = (unsigned char) 1 << ( port + slot * 4 );
        return ( m_Mask.m_PresentMask & bit ) != 0;
    }

    //
    // Tell if the card changed.
    //
    bool HasChanged( int port, int slot )
    {
        unsigned char bit = (unsigned char) 1 << ( port + slot * 4 );
        return ( m_Mask.m_ChangedMask & bit ) != 0;
    }

    //
    // Reset the card status to not changed.
    //
    void ClearChanged( int port, int slot )
    {
        unsigned char bit = (unsigned char) 1 << ( port + slot * 4 );
        m_Mask.m_ChangedMask &= ~bit;
    }
};

static DeviceState s_DeviceState;

//=============================================================================
// Public Functions 
//=============================================================================

//=============================================================================
// Function:    XboxCalculateCRC
//=============================================================================
// Description: Calculate a CRC using the Xbox functions.
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

bool XboxCalculateCRC( const unsigned char* buffer, unsigned int size, XCALCSIG_SIGNATURE* crc )
{
    HANDLE handle = ::XCalculateSignatureBegin( 0 );
    if ( handle == INVALID_HANDLE_VALUE )
    {
        return false;
    }

    DWORD dwSuccess = ::XCalculateSignatureUpdate( handle, buffer, size );
    rAssert( dwSuccess == ERROR_SUCCESS );

    dwSuccess = ::XCalculateSignatureEnd( handle, crc );
    rAssert( dwSuccess == ERROR_SUCCESS );
    (VOID)dwSuccess;

    return true;
}

//=============================================================================
// Function:    CalculateCRC
//=============================================================================
// Description: Calculate the crc. So as not to include the XBOX header file in
//              our header file, this isn't a member. It can fail too!
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

bool CalculateCRC( unsigned int fileSize, XCALCSIG_SIGNATURE* crc )
{
    //
    // Make sure not to do the CRC past the end of the user data (or on our crcs)
    //
    unsigned int start = s_CachePosition * XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE;    
    unsigned int end = start + XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE;
    end = end < fileSize ? end : fileSize;
    end -= sizeof( XCALCSIG_SIGNATURE );

    return XboxCalculateCRC( s_TransferBuffer, end - start, crc );
}

//=============================================================================
// Function:    radXboxSaveDriveFactory
//=============================================================================
// Description: This member is responsible for constructing a radXboxSaveDriveObject.
//
// Parameters:  pointer to receive drive object
//              pointer to the drive name
//              allocator
//              
// Returns:     
//------------------------------------------------------------------------------

void radXboxSaveDriveFactory
( 
    radDrive**         ppDrive, 
    const char*        pDriveName,
    radMemoryAllocator alloc
)
{
    //
    // Simply constuct the drive object.
    //
    *ppDrive = new( alloc ) radXboxSaveDrive( pDriveName, alloc );
    rAssert( *ppDrive != NULL );
}

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    radXboxSaveDrive::radXboxSaveDrive
//=============================================================================

radXboxSaveDrive::radXboxSaveDrive( const char* pdrivespec, radMemoryAllocator alloc )
    : 
    radDrive( ),
    m_OpenFiles( 0 ),
    m_IsMounted( false ),
    m_DriveLetter( '\0' )
{
    static bool g_IsInitialized = false;
    if( !g_IsInitialized )
    {
        //
        // Initialize stuff.
        //
        rAssert( radPlatformGet( ) != NULL );
        s_DeviceState.init( );

        //
        // Set up our transfer buffer
        //
        s_TransferBuffer = (unsigned char*) ::radMemoryRoundUp( (unsigned int) s_TransferBufferSpace, XBOXSAVEDRIVE_ALIGNMENT );
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
    // Figure out the capabilitites based on the drive name.
    //
    if ( strcmp( pdrivespec, "U:" ) == 0 )
    {
        //
        // It's the HDD.
        //
        m_Capabilities = ( radDriveWriteable | radDriveSaveGame | radDriveEnumerable );

        //
        // It can't be mounted or unmounted, so set that up here.
        //
        m_IsMounted = true;
        m_DriveLetter = 'U';

        //
        // Put in NULL as the volume name, since there is no personalized name.
        //
        m_MediaInfo.m_VolumeName[ 0 ] = '\0';
    }
    else
    {
        //
        // It's a memory unit.
        //
        m_Capabilities = ( radDriveRemovable | radDriveWriteable | radDriveSaveGame | radDriveEnumerable );
    }

    //
    // Copy over the name of this drive.
    //
    strncpy( m_DriveName, pdrivespec, radFileDrivenameMax );
    m_DriveName[ radFileDrivenameMax ] = '\0';

    //
    // Setup the init list.
    //
    for ( unsigned int i = 0; i < XBOX_MAX_CARD_HANDLES; i++ )
    {
        m_InitList[ i ].m_Handle = INVALID_HANDLE_VALUE;
    }

    //
    // Set up the directory search handles.
    //
    for ( unsigned int i = 0; i < XBOX_MAX_CARD_HANDLES; i++ )
    {
        m_HandlePool[ i ].m_Handle = INVALID_HANDLE_VALUE;
    }
}

//=============================================================================
// Function:    radXboxSaveDrive::~radXboxSaveDrive
//=============================================================================

radXboxSaveDrive::~radXboxSaveDrive( void )
{
    Unmount( );

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
// Function:    radXboxSaveDrive::Lock
//=============================================================================
// Description: Start a critical section
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radXboxSaveDrive::Lock( void )
{
    s_pMutex->Lock( );
}

//=============================================================================
// Function:    radXboxSaveDrive::Unlock
//=============================================================================
// Description: End a critical section
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radXboxSaveDrive::Unlock( void )
{
    s_pMutex->Unlock( );
}

//=============================================================================
// Function:    radXboxSaveDrive::GetCapabilities
//=============================================================================

unsigned int radXboxSaveDrive::GetCapabilities( void )
{
    return m_Capabilities;
}

//=============================================================================
// Function:    radXboxSaveDrive::GetDriveName
//=============================================================================

const char* radXboxSaveDrive::GetDriveName( void )
{
    return m_DriveName;
}

//=============================================================================
// Function:    radXboxSaveDrive::Initialize
//=============================================================================

radDrive::CompletionStatus radXboxSaveDrive::Initialize( void )
{
    SetMediaInfo( );

    if ( m_DriveLetter != 'U' )
    {
        int port, slot;
        GetPort( &port, &slot );

        if ( m_OpenFiles != 0 )
        {
            //
            // Check if there was a media change
            //
            if ( s_DeviceState.HasChanged( port, slot ) )
            {
                if ( s_DeviceState.IsPresent( port, slot ) )
                {
                    m_LastError = WrongMedia;
                }
                else
                {
                    m_LastError = NoMedia;
                }
                return Error;
            }
        }
        else
        {
            s_DeviceState.ClearChanged( port, slot );
        }
    }

    //
    // Success
    //
    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radXboxSaveDriveGame::OpenSave
//=============================================================================

radDrive::CompletionStatus radXboxSaveDrive::OpenSaveGame
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
    if ( !Mount( ) )
    {
        return Error;
    }

    //
    // Build the full filename
    //
    char fullName[ radFileFilenameMax + 1 ];
    char* pName;
    BuildFileSpec( fileName, fullName, radFileFilenameMax + 1, &pName );
    char Drive[ 4 ];
    strncpy( Drive, fullName, 3 );
    Drive[ 3 ] = '\0';

    //
    // Translate flags to windows
    //
    DWORD accessFlag = writeAccess ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ;
    DWORD sharingFlag = writeAccess ? FILE_SHARE_READ | FILE_SHARE_WRITE : FILE_SHARE_READ;                

    //
    // Create the directory. First we try to open the directory.
    //
    WCHAR wName[ MAX_PATH ];
    AsciiToUnicode( wName, pName );

    char savePath[ MAX_PATH ];
    DWORD error = ::XCreateSaveGame( Drive, wName, OPEN_EXISTING, 0, savePath, MAX_PATH );
    if ( error != ERROR_SUCCESS && error != ERROR_PATH_NOT_FOUND )
    {
        HandleError( error );
        return Error;
    }

    unsigned int saveLen = strlen( savePath );

    bool create = !( error != ERROR_PATH_NOT_FOUND && ( flags == OpenExisting || flags == OpenAlways ) );

    //
    // Now react depending on our open flags.
    //
    if ( flags == CreateAlways )
    {
        //
        // Create always, so we need to delete, then open.
        //
        if ( error != ERROR_PATH_NOT_FOUND )
        {
            if ( !HandleError( ::XDeleteSaveGame( Drive, wName ) ) )
            {
                return Error;
            }
        }
    }
    else if ( flags == OpenExisting )
    {
        if ( error == ERROR_PATH_NOT_FOUND )
        {
            HandleError( error );
            return Error;
        }
    }
    else if ( flags == OpenAlways )
    {
        // nothing to do ...
    }
    else
    {
        rAssertMsg( false, "XboxSaveDrive: invalid open flags." );
    }

    //
    // Create save game container and write the save game icon
    //
    if ( create )
    {
        rAssert( memcardInfo != NULL );

        //
        // Check how much space we have.
        //
        ULARGE_INTEGER FreeBytesAvailableToCaller; 
        ULARGE_INTEGER TotalNumberOfBytes; 
        ULARGE_INTEGER TotalNumberOfFreeBytes; 

        if ( !::GetDiskFreeSpaceEx( Drive, &FreeBytesAvailableToCaller, 
                                    &TotalNumberOfBytes, &TotalNumberOfFreeBytes ) )
        {
            HandleError( ::GetLastError( ) );
            return Error;
        }

        unsigned int bytesFree;
        if( FreeBytesAvailableToCaller.HighPart != 0 )
        {
            bytesFree = UINT_MAX;
        }
        else
        {
            bytesFree = FreeBytesAvailableToCaller.LowPart;
        }        

        //
        // Create a save game container if there's room.
        //
        if ( bytesFree < GetCreationSize( memcardInfo, maxSize ) )
        {
            m_LastError = NoFreeSpace;
            return Error;
        }

        if ( !HandleError( ::XCreateSaveGame( Drive, wName, CREATE_NEW, 0, savePath, MAX_PATH ) ) )
        {
            return Error;
        }
        
        //
        // Need to update the length of the buffer!!
        //
        saveLen = strlen( savePath );

        //
        // Write the icon.
        //
        if ( memcardInfo->m_Icon != NULL )
        {
            strcpy( &savePath[ saveLen ], s_SaveImage );
    
            HANDLE handle = ::CreateFile( savePath, GENERIC_READ | GENERIC_WRITE,  FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                          NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
        
            if ( handle == INVALID_HANDLE_VALUE )
            {
                HandleError( ::GetLastError( ) );
                return Error;
            }

            unsigned long bytesWritten = 0;
            if (
                ::SetFilePointer( handle, (LONG) memcardInfo->m_IconSize, NULL, FILE_BEGIN ) != memcardInfo->m_IconSize ||
                !::SetEndOfFile( handle ) ||
                ::SetFilePointer( handle, (LONG) 0, NULL, FILE_BEGIN ) != 0 ||
                !::WriteFile( handle, memcardInfo->m_Icon, memcardInfo->m_IconSize, &bytesWritten, NULL )
               )
            {
                ::CloseHandle( handle );
                HandleError( ::GetLastError( ) );
                return Error;
            }
            ::CloseHandle( handle );
        }
    }

    //
    // Make the name of the save game.
    //
    strcpy( &savePath[ saveLen ], "05" ); // hardcode the 'real' filename, since simpson2 descriptive name contains ':' and '/'

    //
    // Open the file for resizing and initializing.
    //
    *pHandle = ::CreateFile( savePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
                             create ? CREATE_NEW : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

    if ( *pHandle != INVALID_HANDLE_VALUE )
    {
        if ( create )
        {
            //
            // Find the creation size
            //
            unsigned int numBlocks = 
                ( maxSize + XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE - 1 ) / XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE;
            unsigned int fsize = maxSize + numBlocks * sizeof( XCALCSIG_SIGNATURE ) + sizeof( unsigned int );

            //
            // Pre-size the file.
            //
            if ( ::SetFilePointer( *pHandle, (LONG) fsize, NULL, FILE_BEGIN ) != fsize ||
                 !::SetEndOfFile( *pHandle ) ||
                 !InitializeFile( *pHandle, fsize, writeAccess == false )
               )
            {
                ::CloseHandle( *pHandle );
                HandleError( ::GetLastError( ) );
                return Error;
            }

        }
        ::CloseHandle( *pHandle );
    }
    else
    {
        HandleError( ::GetLastError( ) );
        return Error;
    }

    //
    // Now open the file for real.
    //
    *pHandle = ::CreateFile( savePath, accessFlag, sharingFlag, NULL, 
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

    if ( *pHandle == INVALID_HANDLE_VALUE )
    {
        ::CloseHandle( *pHandle );
        HandleError( ::GetLastError( ) );
        return Error;
    }

    unsigned int openSize;
    if ( !GetSizeFromHandle( *pHandle, &openSize ) )
    {
        ::CloseHandle( *pHandle );
        return Error;
    }

    //
    // If it was just created, add it to the init list, otherwise load up the first block.
    //
    if ( create )
    {
        //
        // Find a free entry in our init list.
        //
        unsigned int entry = 0;
        for ( entry = 0; entry < XBOX_MAX_CARD_HANDLES; entry++ )
        {
            if ( m_InitList[ entry ].m_Handle == INVALID_HANDLE_VALUE )
            {
                m_InitList[ entry ].m_Handle = *pHandle;
                break;
            }
        }
        rAssert( entry < XBOX_MAX_CARD_HANDLES );
    }
    else
    {
        if ( openSize < sizeof( unsigned int ) + sizeof( XCALCSIG_SIGNATURE ) )
        {
            ::CloseHandle( *pHandle );
            s_CacheState = CacheInvalid;
            m_LastError = DataCorrupt;
            return Error;
        }

        if ( !LoadBlock( *pHandle, 0 ) )
        {
            ::CloseHandle( *pHandle );
            return Error;
        }

        //
        // Check that the file size hasn't changed.
        //
        if ( memcmp( s_TransferBuffer, &openSize, sizeof( unsigned int ) ) != 0 )
        {
            ::CloseHandle( *pHandle );
            s_CacheState = CacheInvalid;
            m_LastError = DataCorrupt;
            return Error;
        }
    }

    *pSize = openSize - GetTableSize( openSize ) - sizeof( unsigned int );
    m_OpenFiles++;

    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radXboxSaveDrive::CloseFile
//=============================================================================

radDrive::CompletionStatus radXboxSaveDrive::CloseFile( radFileHandle handle, const char* fileName )
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
    
    ::CloseHandle( handle );

    m_OpenFiles--;
    return Complete;
}

//=============================================================================
// Function:    radXboxSaveDrive::CommitFile
//=============================================================================

radDrive::CompletionStatus radXboxSaveDrive::CommitFile( radFileHandle handle, const char* fileName )
{
    // Mount the drive.
    //
    if ( !Mount( ) )
    {
        return Error;
    }

    //
    // Flush the block first.
    //
    if ( !FlushBlock( ) )
    {
        return Error;
    }

    //
    // Check if we need to add in the header and put it in.
    //
    for ( unsigned int i = 0; i < XBOX_MAX_CARD_HANDLES; i++ )
    {
        if ( s_CacheHandle == m_InitList[ i ].m_Handle )
        {
            if ( !LoadBlock( handle, 0 ) )
            {
                return Error;
            }

            unsigned int fileSize;
            if ( !GetSizeFromHandle( handle, &fileSize ) )
            {
                return Error;
            }

            s_CacheState = CacheInvalid;
            memcpy( s_TransferBuffer, &fileSize, sizeof( unsigned int ) );
            s_CacheState = CacheDirty;

            if ( !FlushBlock( ) )
            {
                return Error;
            }

            m_InitList[ i ].m_Handle = INVALID_HANDLE_VALUE;
            break;
        }
    }

    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radXboxSaveDrive::GetReadBlockSize
//=============================================================================

unsigned int radXboxSaveDrive::GetReadBlockSize( void )
{
    return XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE - sizeof( XCALCSIG_SIGNATURE );
}

//=============================================================================
// Function:    radXboxSaveDrive::GetReadHeaderSize
//=============================================================================

unsigned int radXboxSaveDrive::GetReadHeaderSize( radFileHandle handle )
{
    return sizeof( unsigned int );
}

//=============================================================================
// Function:    radXboxSaveDrive::GetWriteBlockSize
//=============================================================================

unsigned int radXboxSaveDrive::GetWriteBlockSize( void )
{
    return XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE - sizeof( XCALCSIG_SIGNATURE );
}

//=============================================================================
// Function:    radXboxSaveDrive::GetWriteHeaderSize
//=============================================================================

unsigned int radXboxSaveDrive::GetWriteHeaderSize( radFileHandle handle )
{
    return sizeof( unsigned int );
}

//=============================================================================
// Function:    radXboxSaveDrive::ReadSignedBlock
//=============================================================================

radDrive::CompletionStatus radXboxSaveDrive::ReadSignedBlock
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
                "radFileSystem: XboxSaveDrive: External memory not supported for reads." );

    //
    // Mount the drive.
    //
    if ( !Mount( ) )
    {
        return Error;
    }

    //
    // Check if we're reading past the end of the file.
    //
    unsigned int size;
    if ( !GetSizeFromHandle( handle, &size ) )
    {
        return Error;
    }

    if ( block * XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE >= size )
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
// Function:    radXboxSaveDrive::WriteSignedBlock
//=============================================================================

radDrive::CompletionStatus radXboxSaveDrive::WriteSignedBlock
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
                "radFileSystem: XboxSaveDrive: External memory not supported for writes." );

    //
    // Mount the drive.
    //
    if ( !Mount( ) )
    {
        return Error;
    }

    //
    // Get the file size
    //
    unsigned int size;
    if ( !GetSizeFromHandle( handle, &size ) )
    {
        return Error;
    }
    size = size - GetTableSize( size );

    rAssertMsg( 
        block * ( XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE - sizeof( XCALCSIG_SIGNATURE ) ) + position + numBytes <= size,
        "radFileSystem: XboxSaveDrive: Cannot change file size." );

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
// Function:    radXboxSaveDrive::DestroyFile
//=============================================================================

radDrive::CompletionStatus radXboxSaveDrive::DestroyFile( const char* filename )
{
    //
    // Mount the drive.
    //
    if ( !Mount( ) )
    {
        return Error;
    }

    //
    // Get the name.
    //
    char fullName[ radFileFilenameMax + 1 ];
    char* pName;
    BuildFileSpec( filename, fullName, radFileFilenameMax + 1, &pName );
    char Drive[ 4 ];
    strncpy( Drive, fullName, 3 );
    Drive[ 3 ] = '\0';

    //
    // Delete the save game and all it stands for.
    //
    WCHAR wName[ MAX_PATH ];
    AsciiToUnicode( wName, pName );

    DWORD error = ::XDeleteSaveGame( Drive, wName );
    if ( error != ERROR_SUCCESS )
    {
        HandleError( error );
        return Error;
    }

    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radXboxSaveDrive::FindFirst
//=============================================================================

radDrive::CompletionStatus radXboxSaveDrive::FindFirst
( 
    const char*                 searchSpec, 
    IRadDrive::DirectoryInfo*   pDirectoryInfo, 
    radFileDirHandle*           pHandle,
    bool                        firstSearch
)
{
    //
    // Mount the drive.
    //
    if ( !Mount( ) )
    {
        return Error;
    }

    //
    // Get the name.
    //
    char fullName[ radFileFilenameMax + 1 ];
    char* pName;
    BuildFileSpec( searchSpec, fullName, radFileFilenameMax + 1, &pName );
    char Drive[ 4 ];
    strncpy( Drive, fullName, 3 );
    Drive[ 3 ] = '\0';

    //
    // Close this search if it already exists
    //
    int tmpHandle = -1;
    if ( !firstSearch && m_HandlePool[ (unsigned int) *pHandle ].m_Handle != INVALID_HANDLE_VALUE )
    {
        //
        // Close the handle.
        //
        tmpHandle = (unsigned int) *pHandle;
        if ( m_HandlePool[ tmpHandle ].m_Handle != INVALID_HANDLE_VALUE )
        {
            ::XFindClose( m_HandlePool[ tmpHandle ].m_Handle );
            m_HandlePool[ tmpHandle ].m_Handle = INVALID_HANDLE_VALUE;
        }
    }
    else
    {
        //
        // Find a free handle.
        //
        for ( unsigned int i = 0; i < XBOX_MAX_CARD_HANDLES; i++ )
        {
            if ( m_HandlePool[ i ].m_Handle == INVALID_HANDLE_VALUE )
            {
                tmpHandle = i;
                break;
            }
        }
    }
    rAssert( tmpHandle >= 0 );

    unsigned int handle = (unsigned int) tmpHandle;

    //
    // XBOX find first!
    //
    XGAME_FIND_DATA FindData;
    m_HandlePool[ handle ].m_Handle = ::XFindFirstSaveGame( Drive, &FindData );

    //
    // We now have a name, but we need to match it. Store the search spec.
    //
    strncpy( m_HandlePool[ handle ].m_pSpec, pName, radFileFilenameMax );
    m_HandlePool[ handle ].m_pSpec[ radFileFilenameMax ] = '\0';

    //
    // Check the results.
    //
    *pHandle = (radFileDirHandle) handle;
    DirResult dirResult = TranslateDirInfo( pDirectoryInfo, &FindData, m_HandlePool[ handle ].m_Handle, m_HandlePool[ handle ].m_pSpec );
    if ( dirResult == Done )
    {
        return Complete;
    }
    else if ( dirResult == Fail )
    {
        m_HandlePool[ handle ].m_Handle = INVALID_HANDLE_VALUE;
        return Error;
    }
    else
    {
        return FindNext( pHandle, pDirectoryInfo );
    }

    return Complete;
}

//=============================================================================
// Function:    radXboxSaveDrive::FindNext
//=============================================================================

radDrive::CompletionStatus radXboxSaveDrive::FindNext( radFileDirHandle* pHandle, IRadDrive::DirectoryInfo* pDirectoryInfo )
{
    //
    // If it's an invalid handle, just return file not found
    //
    unsigned int handle = (unsigned int) *pHandle;

    if ( m_HandlePool[ handle ].m_Handle == INVALID_HANDLE_VALUE )
    {
        m_LastError = FileNotFound;
        return Error;
    }

    //
    // Mount the drive.
    //
    if ( !Mount( ) )
    {
        return Error;
    }

    XGAME_FIND_DATA FindData;;
    DirResult dirResult = NoMatch;

    while ( dirResult == NoMatch )
    {
        //
        // Find the next game.
        //
        if ( !::XFindNextSaveGame( m_HandlePool[ handle ].m_Handle, &FindData ) )
        {
            dirResult = TranslateDirInfo( pDirectoryInfo, &FindData, INVALID_HANDLE_VALUE, m_HandlePool[ handle ].m_pSpec );
        }
        else
        {
            dirResult = TranslateDirInfo( pDirectoryInfo, &FindData, m_HandlePool[ handle ].m_Handle, m_HandlePool[ handle ].m_pSpec );
        }

        //
        // Check if it matched.
        //
        if ( dirResult == Done )
        {
            return Complete;
        }
        else if ( dirResult == Fail )
        {
            return Error;
        }
        else
        {
            //
            // Try again!
            //
        }
    }

    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radXboxSaveDrive::FindCloseX
//=============================================================================

radDrive::CompletionStatus radXboxSaveDrive::FindCloseX( radFileDirHandle* pHandle )
{
    unsigned int handle = (unsigned int) *pHandle;

    if ( m_HandlePool[ (unsigned int)*pHandle ].m_Handle != INVALID_HANDLE_VALUE )
    {
        ::XFindClose( m_HandlePool[ (unsigned int)*pHandle ].m_Handle );
        m_HandlePool[ (unsigned int)*pHandle ].m_Handle = INVALID_HANDLE_VALUE;
    }

    return Complete;
}

//=============================================================================
// Function:    radXboxSaveDrive::GetCreationSize
//=============================================================================
// Description: Get the actual size for creation.
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

unsigned int radXboxSaveDrive::GetCreationSize( radMemcardInfo* memcardInfo, unsigned int size )
{
    rAssert( memcardInfo != NULL );
    
    //
    // 4 blocks of data are created when the card is mounted, so we do not need to include that.
    //

    //
    // Two blocks are used by the file system.
    //
    unsigned int csize = 2 * 16 * 1024;

    //
    // Our game with crcs and header.
    //
    unsigned int numBlocks = 
        ( size + XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE - 1 ) / XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE;
    csize += ::radMemoryRoundUp( size + numBlocks * sizeof( XCALCSIG_SIGNATURE ) + sizeof( unsigned int ), 16 * 1024 );

    //
    // If we save a seperate icon for each game, we need to include that.
    //
    if ( memcardInfo->m_Icon != NULL )
    {
        rAssert( memcardInfo->m_IconSize <= 16 * 1024 );
        csize += 16 * 1024;
    }

    return csize;
}

//=============================================================================
// Private Member Functions
//=============================================================================

//=============================================================================
// Function:    radXboxSaveDrive::SetMediaInfo
//=============================================================================
// Description: Set up the media info. The volume name is the personalized name
//              name of the card, or appropriate name as defined by XBOX.
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

void radXboxSaveDrive::SetMediaInfo( void )
{
    m_MediaInfo.m_SectorSize = 512;
    m_MediaInfo.m_FreeSpace = 0;
    m_MediaInfo.m_FreeFiles = 0;
    
    //
    // Check if we should remount.
    //
    radFileError error = Success;
    if ( m_DriveLetter != 'U' )
    {
        m_MediaInfo.m_VolumeName[ 0 ] = '\0';

        int port, slot;
        GetPort( &port, &slot );

        s_DeviceState.update( );
        if ( s_DeviceState.HasChanged( port, slot ) )
        {
            //
            // This may print out a warning, but no other choice if we want the info to be updated
            //
            Unmount( );
        }

        if ( s_DeviceState.IsPresent( port, slot ) )
        {
            if ( !Mount( ) )
            {
                m_MediaInfo.m_MediaState = ErrorToMediaState( m_LastError );
                m_LastError = Success;
                return;
            }
        }
        else
        {
            m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaNotPresent;
            return;
        }

        //
        // Device is mounted! Find the personalized name.
        //
        memset( m_MediaInfo.m_VolumeName, 0, sizeof( m_MediaInfo.m_VolumeName ) );
        error = TranslateError( 
            ::XMUNameFromDriveLetter( m_DriveLetter, (WCHAR*) m_MediaInfo.m_VolumeName, sizeof( m_MediaInfo.m_VolumeName ) / sizeof( WCHAR ) )
            );
     
        if ( error != Success )
        {
            m_MediaInfo.m_VolumeName[ 0 ] = '\0';
            m_MediaInfo.m_MediaState = ErrorToMediaState( error );
            return;
        }
    }

    //
    // Get the drive name.
    //
    char Drive[ 4 ];
    Drive[ 0 ] = m_DriveLetter;
    Drive[ 1 ] = ':';
    Drive[ 2 ] = '\\';
    Drive[ 3 ] = '\0';

    //
    // Get the size.
    //
    ULARGE_INTEGER FreeBytesAvailableToCaller; 
    ULARGE_INTEGER TotalNumberOfBytes; 
    ULARGE_INTEGER TotalNumberOfFreeBytes; 
    
    if ( !::GetDiskFreeSpaceEx( Drive, &FreeBytesAvailableToCaller, 
                                &TotalNumberOfBytes, &TotalNumberOfFreeBytes ) )
    {
        m_MediaInfo.m_VolumeName[ 0 ] = '\0';
        error = TranslateError( ::GetLastError( ) );
        rAssert( error == Success );
        m_MediaInfo.m_MediaState = ErrorToMediaState( error );
        return;
    }

    //
    // Fill in the info and we're done.
    // 
    m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaPresent; 
    
    if( FreeBytesAvailableToCaller.HighPart != 0 )
    {
        m_MediaInfo.m_FreeSpace = UINT_MAX;
    }
    else
    {
        m_MediaInfo.m_FreeSpace = FreeBytesAvailableToCaller.LowPart;
    }        

    DWORD sectorSize = XGetDiskSectorSize( Drive );
    if ( sectorSize != 0 )
    {
        m_MediaInfo.m_SectorSize = sectorSize;
    }
    else
    {
        m_MediaInfo.m_SectorSize = 512;
    }

    //
    // No file limit, so base it on free space
    //
    m_MediaInfo.m_FreeFiles = m_MediaInfo.m_FreeSpace / m_MediaInfo.m_SectorSize;
}

//=============================================================================
// Function:    radXboxSaveDrive::ErrorToMediaState
//=============================================================================

IRadDrive::MediaInfo::MediaState radXboxSaveDrive::ErrorToMediaState( radFileError result )
{
    switch( m_LastError )
    {
    case Success:
    case NoFreeSpace:
        return IRadDrive::MediaInfo::MediaPresent;

    case MediaNotFormatted:
        return IRadDrive::MediaInfo::MediaNotFormatted;

    case NoMedia:
        return IRadDrive::MediaInfo::MediaNotPresent;

    case MediaInvalid:
        return IRadDrive::MediaInfo::MediaInvalid;

    case MediaCorrupt:
    default:
        return IRadDrive::MediaInfo::MediaDamaged;
    }
}

//=============================================================================
// Function:    radXboxSaveDrive::BuildFileSpec
//=============================================================================
// Description: Set up the file name to be used by the underlying file system.
//              pName points to the name AFTER the initial slash. The drive
//              name before that is always in the form `U:\\'
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

void radXboxSaveDrive::BuildFileSpec( const char* fileName, char* fullName, unsigned int size, char** pName )
{
    //
    // We can only build a proper name after the card has been mounted!
    //
    rAssert( m_IsMounted );

    //
    // Add drive name with slash
    //
    fullName[ 0 ] = m_DriveLetter;
    fullName[ 1 ] = ':';
    fullName[ 2 ] = '\\';
    int len = 3;

    //
    // Don't repeat the leading slash.
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
    rAssertMsg( strchr( *pName, '\\' ) == NULL, "XBoxSaveDrive: subdirectories are not supported." );
}

//=============================================================================
// Function:    radXboxSaveDrive::TranslateError
//=============================================================================
// Description: This translates the windows error code into our error enums.
//
// Parameters:  Win32 error code
//              
// Returns:     Our error code
//------------------------------------------------------------------------------

radFileError radXboxSaveDrive::TranslateError( unsigned int windowsError )
{
    switch( windowsError )
    {
    case ERROR_SUCCESS:
        return ( Success );

    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_INVALID_DRIVE:
        return ( FileNotFound );

    case ERROR_WRONG_DISK :
    case ERROR_NOT_SAME_DEVICE:
        return ( WrongMedia );

    case ERROR_NOT_DOS_DISK:
        return ( MediaInvalid );

    case ERROR_DEVICE_NOT_CONNECTED:
        //
        // Operation failed, check if something connected.
        //
        s_DeviceState.update( );
        int port, slot;
        GetPort( &port, &slot );
        if ( s_DeviceState.IsPresent( port, slot ) )
        {
            return ( WrongMedia );
        }
        else
        {
            return ( NoMedia );
        }

    case ERROR_BAD_FORMAT:
    case ERROR_SECTOR_NOT_FOUND:
    case ERROR_UNRECOGNIZED_VOLUME:
        return ( MediaNotFormatted );

    case ERROR_DISK_FULL:
        return ( NoFreeSpace );

    case ERROR_ALREADY_ASSIGNED: // not serious, but we tried to remount a card.
        rDebugPrintf( "XboxSaveDrive: internal error.\n" );
        return ( Success );

    case ERROR_OUTOFMEMORY:
        rDebugPrintf( "XboxSaveDrive: out of memory!!\n" );
        return ( HardwareFailure );
        
    default:
        return( HardwareFailure );
    }
}

//=============================================================================
// Function:    radXboxSaveDrive::TranslateDirInfo
//=============================================================================
// Description: Get the result of a directory search. Sets m_LastError appropriately.
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

radXboxSaveDrive::DirResult radXboxSaveDrive::TranslateDirInfo
( 
    IRadDrive::DirectoryInfo*   pDirectoryInfo, 
    const void*                 pFindDataVoid,
    const void*                 winHandle,
    const char*                 pSpec
)
{
    const XGAME_FIND_DATA* pFindData = (XGAME_FIND_DATA*) pFindDataVoid;
    
    if ( winHandle == INVALID_HANDLE_VALUE )
    {
        //
        // Either we failed or we're out of games.
        //
        DWORD error = ::GetLastError( );
        if ( error == ERROR_NO_MORE_FILES )
        {
            pDirectoryInfo->m_Name[0] = '\0';
            pDirectoryInfo->m_Type = IRadDrive::DirectoryInfo::IsDone;
        }
        else
        {
            HandleError( error );
            return Fail;
        }
    }
    else
    {
        //
        // Match the save game name.
        //
        char saveName[ MAX_GAMENAME ];
        UnicodeToAscii( saveName, pFindData->szSaveGameName );
        if ( FileNameMatchesSearch( saveName, pSpec ) )
        {
            strncpy( pDirectoryInfo->m_Name, saveName, radFileFilenameMax );
            pDirectoryInfo->m_Name[ radFileFilenameMax ] = '\0';
            pDirectoryInfo->m_Type = IRadDrive::DirectoryInfo::IsSaveGame;
        }
        else
        {
            return NoMatch;
        }
    }

    m_LastError = Success;
    return Done;
}

//=============================================================================
// Function:    radXboxSaveDrive::GetPort
//=============================================================================

void radXboxSaveDrive::GetPort( int* port, int* slot )
{
    //
    // Handle the HDD
    //
    if ( m_DriveName[ 0 ] == 'U' )
    {
        *port = -1;
        *slot = -1;
        return;
    }

    //
    // Based on the drive name, lets map it to port and slot.
    //
    *port = ( int )( m_DriveName[ XBOXMEMCARDDRIVE_PORT_LOC ] - '1' );   
    *slot = ( int )( m_DriveName[ XBOXMEMCARDDRIVE_SLOT_LOC ] - 'A' );
    rAssert( (*port >= 0) && (*port <= 3) );
    rAssert( (*slot >= 0) && (*slot <= 1) );
}

//=============================================================================
// Function:    radXboxSaveDrive::Mount
//=============================================================================

bool radXboxSaveDrive::Mount( void )
{
    int port, slot;
    GetPort( &port, &slot );

    //
    // When we mount, first check if the card changed. If it did, then
    // we fail, unless there are no open files.
    //
    s_DeviceState.update( );
    if ( s_DeviceState.HasChanged( port, slot ) )
    {
        if ( m_OpenFiles > 0 )
        {
            m_LastError = TranslateError( ERROR_DEVICE_NOT_CONNECTED );
            return false;
        }
        else
        {
            //
            // Get ready to remount the card.
            //
            s_DeviceState.ClearChanged( port, slot );
            Unmount( );
        }
    }

    if ( !m_IsMounted )
    {
        DWORD error = XMountMU( port, slot, &m_DriveLetter );
        if ( error != ERROR_SUCCESS )
        {
            m_LastError = TranslateError( error );
            return false;
        }
    }

    //
    // The card is now mounted.
    //
    m_IsMounted = true;
    m_LastError = Success;
    return true;
}

//=============================================================================
// Function:    radXboxSaveDrive::Unmount
//=============================================================================

void radXboxSaveDrive::Unmount( void )
{
    //
    // If we have the hard drive, there's no such thing as unmounting
    //
    if ( m_DriveLetter == 'U' )
    {
        return;
    }

    if ( m_IsMounted )
    {
        //
        // close all directory handles.
        //
        for ( unsigned int i = 0; i < XBOX_MAX_CARD_HANDLES; i++ )
        {
            if ( m_HandlePool[ i ].m_Handle != INVALID_HANDLE_VALUE )
            {
                ::XFindClose( m_HandlePool[ i ].m_Handle );
                m_HandlePool[ i ].m_Handle = INVALID_HANDLE_VALUE;
            }
        }

        int port, slot;
        GetPort( &port, &slot );
        DWORD dwSuccess = XUnmountMU( port, slot );
        rAssert( dwSuccess == ERROR_SUCCESS );
        (VOID)dwSuccess;

        m_IsMounted = false;
    }
}

//=============================================================================
// Function:    radXboxSaveDrive::HandleError
//=============================================================================
// Description: Hook for handling errors. Set m_LastError and returns a value.
//              Can be used as shorthand for translateError: 
//                  HandleError( ::GetLastError( ) )
//              or to parse a return value:
//                  if ( !HandleError( f( ) ) ) { return Error; }
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

bool radXboxSaveDrive::HandleError( unsigned int error )
{
    radFileError fileError = TranslateError( error );
    m_LastError = fileError;

    //
    // If we want to unmount when nothing is referenced, this is the place to put it.
    //

    return ( fileError == Success );
}

//=============================================================================
// Function:    radXboxSaveDrive::AsciiToUnicode
//=============================================================================

void radXboxSaveDrive::AsciiToUnicode( WCHAR* dest, const char* src )
{
    char* ptr1 = (char*) dest;
    const char* ptr2 = src;
    while ( *ptr1++ = *ptr2++ )
    {
        *ptr1++ = 0;
    }
    *ptr1++ = 0;
    *ptr1++ = 0;
}

//=============================================================================
// Function:    radXboxSaveDrive::UnicodeToAscii
// Drops lots of info ...
//=============================================================================

void radXboxSaveDrive::UnicodeToAscii( char* dest, const WCHAR* src )
{
    char* ptr1 = dest;
    const char* ptr2 = (char*) src;
    while ( *ptr1++ = *ptr2 )
    {
        ptr2 += 2;
    }
    *ptr1++ = '\0';
}

//=============================================================================
// Function:    radXboxSaveDrive::LoadBlock
//=============================================================================

bool radXboxSaveDrive::LoadBlock( radFileHandle handle, unsigned int block )
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
    unsigned int start = block * XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE;    
    unsigned int end = start + XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE;
    end = end < fileSize ? end : fileSize;
    unsigned int size = end - start;
    end -= sizeof( XCALCSIG_SIGNATURE );
    
    //
    // Read the block.
    //
    unsigned long bytesRead;
    if (
        ::SetFilePointer( handle, (LONG) start, NULL, FILE_BEGIN ) != start ||
        !::ReadFile( handle, s_TransferBuffer, size, &bytesRead, NULL )
       )
    {
        HandleError( ::GetLastError( ) );
        return false;
    }

    //
    // Get the crc.
    //
    XCALCSIG_SIGNATURE crc;
    memcpy( &crc, &s_TransferBuffer[ end - start ], sizeof( crc ) );

    s_CacheHandle = handle;
    s_CachePosition = block;

    //
    // Verify the data.
    //
    XCALCSIG_SIGNATURE newCrc;
    if ( !CalculateCRC( fileSize, &newCrc ) )
    {
        HandleError( ::GetLastError( ) );
        return false;
    }

    if ( memcmp( &newCrc, &crc, sizeof( crc ) ) != 0 )
    {
        m_LastError = DataCorrupt;
        return false;
    }

    s_CacheState = CacheClean;

    return true;
}

//=============================================================================
// Function:    radXboxSaveDrive::FlushBlock
//=============================================================================

bool radXboxSaveDrive::FlushBlock( void )
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
    XCALCSIG_SIGNATURE crc;
    if ( !CalculateCRC( fileSize, &crc ) )
    {
        HandleError( ::GetLastError( ) );
        return false;
    }

    //
    // Set up the write.
    //
    unsigned int start = s_CachePosition * XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE;    
    unsigned int end = start + XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE;
    end = end < fileSize ? end : fileSize;
    unsigned int size = end - start;
    end -= sizeof( XCALCSIG_SIGNATURE );

    //
    // Copy in the crc.
    //
    memcpy( &s_TransferBuffer[ end - start ], &crc, sizeof( crc ) );

    //
    // Write the block.
    //
    unsigned long bytesWritten;
    if (
        ::SetFilePointer( s_CacheHandle, (LONG) start, NULL, FILE_BEGIN ) != start ||
        !::WriteFile( s_CacheHandle, s_TransferBuffer, size, &bytesWritten, NULL )
       )
    {
        HandleError( ::GetLastError( ) );
        return false;
    }

    s_CacheState = CacheClean;

    return true;
}

//=============================================================================
// Function:    radXboxSaveDrive::GetSizeFromHandle
//=============================================================================

bool radXboxSaveDrive::GetSizeFromHandle( radFileHandle handle, unsigned int* size )
{
    *size = ::GetFileSize( handle, NULL );
    if ( (int) (*size) == -1 )
    {
        HandleError( ::GetLastError( ) );
        return false;
    }
    return true;
}

//=============================================================================
// Function:    radXboxSaveDrive::GetTableSize
//=============================================================================

unsigned int radXboxSaveDrive::GetTableSize( unsigned int fileSize )
{
    unsigned int divisor = XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE + sizeof( XCALCSIG_SIGNATURE );
    unsigned int numBlocks = ( fileSize + divisor - 1 ) / divisor;

    return numBlocks * sizeof( XCALCSIG_SIGNATURE );
}

//=============================================================================
// Function:    radXboxSaveDrive::InitializeFile
//=============================================================================

bool radXboxSaveDrive::InitializeFile( radFileHandle handle, unsigned int size, bool validHeader )
{
    unsigned long bytesWritten;

    //
    // Set our transfer buffer to 0.
    //
    if ( !FlushBlock( ) )
    {
        return false;
    }
    s_CacheState = CacheInvalid;
    
    memset( s_TransferBuffer, 0, XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE );

    //
    // How much of the buffer has a crc check done on it.
    //
    unsigned int userSize = XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE - sizeof( XCALCSIG_SIGNATURE );
    unsigned int fullSize = size;

    //
    // If the file is created, but not writeable, put in the header now.
    //
    if ( validHeader )
    {
        memcpy( s_TransferBuffer, &size, sizeof( unsigned int ) );
    }

    //
    // Add in the crc value.
    //
    XCALCSIG_SIGNATURE crc;
    if ( size > XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE )
    {
        if ( !XboxCalculateCRC( s_TransferBuffer, userSize, &crc ) )
        {
            HandleError( ::GetLastError( ) );
            return false;
        }
        memcpy( &s_TransferBuffer[ userSize ], &crc, sizeof( crc ) );
    }

    //
    // Seek to the beginning
    //
    if ( ::SetFilePointer( handle, (LONG) 0, NULL, FILE_BEGIN ) != 0 )
    {
        HandleError( ::GetLastError( ) );
        return false;
    }

    //
    // Write all but the last block
    //
    unsigned int block = 0;
    while ( fullSize > XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE )
    {
        if ( !::WriteFile( handle, s_TransferBuffer, XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE, &bytesWritten, NULL ) )
        {
            HandleError( ::GetLastError( ) );
            return false;
        }

        //
        // If we have a header in there, remove it for the next blocks!
        //
        if ( block == 0 && validHeader )
        {
            memset( s_TransferBuffer, 0, sizeof( unsigned int ) );
            if ( !XboxCalculateCRC( s_TransferBuffer, userSize, &crc ) )
            {
                HandleError( ::GetLastError( ) );
                return false;
            }
            memcpy( &s_TransferBuffer[ userSize ], &crc, sizeof( crc ) );
        }

        fullSize -= XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE;
        block++;
    }

    //
    // Only the last block left.
    //
    if ( fullSize > 0 )
    {
        //
        // guarantee that the header is no longer in the buffer.
        //
        memset( s_TransferBuffer, 0, sizeof( unsigned int ) );

        if ( !XboxCalculateCRC( s_TransferBuffer, fullSize - sizeof( crc ), &crc ) )
        {
            HandleError( ::GetLastError( ) );
            return false;
        }
        memcpy( &s_TransferBuffer[ fullSize - sizeof( crc ) ], &crc, sizeof( crc ) );

        if ( !::WriteFile( handle, s_TransferBuffer, fullSize, &bytesWritten, NULL ) )
        {
            HandleError( ::GetLastError( ) );
            return false;
        }
        block++;
    }
    
    s_CacheHandle = handle;
    s_CachePosition = block - 1;
    s_CacheState = CacheClean;  

    return true;
}

//=============================================================================
// Function:    radXboxSaveDrive:FileNameMatchesSearch
//=============================================================================
// Description: This is a helper for this drive's enumeration facilities. It
//              determines if a given path/file name matches a user search
//              spec.
//  
//              This is a bit of a hack - we don't have a platform-independant
//              wildcard matcher. This one tries to accomodate both searches
//              with and without extensions: ie, "*.*" versus "*". It tries to
//              simulate Windows style matching.
//
// Parameters:  sSearchSpec - a path with wildcards to be matched against
//              pFileName - a filename to test against the search spec
//              
// Returns:     true if there is a match
//------------------------------------------------------------------------------

bool radXboxSaveDrive::FileNameMatchesSearch
( 
    const char* pFileName, 
    const char* pSearchSpec 
)
{
    rAssert( pFileName != NULL );
    rAssert( pSearchSpec != NULL );

    //
    // If the spec doesn't have a '.' in it, we can do this easily.
    //
    if( strchr( pSearchSpec, '.' ) == NULL )
    {
        return ::radStringMatchesWildCardPattern( pFileName, pSearchSpec );
    }

    //
    // Make local copies of the input strings for hacking up.
    //
    char nameBuf[ radFileFilenameMax + 1 ];
    char* nameAfterDot;
    char specBuf[ radFileFilenameMax + 1 ];
    char* specAfterDot;
    strcpy( nameBuf, pFileName );
    strcpy( specBuf, pSearchSpec );
    nameAfterDot = strchr( nameBuf, '.' );
    specAfterDot = strchr( specBuf, '.' );
   
    //
    // Now split each string into two at the dot.
    //
    if( nameAfterDot != NULL )
    {
        *nameAfterDot = '\0';
        nameAfterDot++;
    }
    *specAfterDot = '\0';
    specAfterDot++;

    //
    // Perform matching on the two separate parts.
    //
    bool result1 = ::radStringMatchesWildCardPattern( nameBuf, specBuf );
    bool result2;
    if( nameAfterDot != NULL )
    {
        result2 = ::radStringMatchesWildCardPattern( nameAfterDot, specAfterDot );
    }
    else
    {
        result2 = ::radStringMatchesWildCardPattern( "", specAfterDot );
    }
    return( result1 && result2 );
}
