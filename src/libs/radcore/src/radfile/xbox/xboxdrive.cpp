//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        xboxdrive.hpp
//
// Subsystem:	Radical File System
//
// Description:	This file contains the implementation of the xbox dvd and hard drive.
//
// Revisions:	
//
// Notes:       This drives handles drive D: (DVD) on its own thread. All other
//              hard drives share a thread, except U: (save data) which shares a
//              thread with the memory units (xboxsavedrive.hpp). Drive Z: (utility)
//              is the only one which can be formatted. The DVD drive is not removable
//              since that is a hardware reset.
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include "xboxdrive.hpp"
#include <limits.h>
#include <radtime.hpp>

//=============================================================================
// Statics
//=============================================================================

//
// This drive handles both the hard drive and DVD drive. These run on seperate threads
// so we have two drive threads.
//
radDriveThread*     s_pDVDDriveThread = NULL;
IRadThreadMutex*    s_pDVDMutex = NULL;

radDriveThread*     s_pHDDDriveThread = NULL;
IRadThreadMutex*    s_pHDDMutex = NULL;

//=============================================================================
// Public Functions 
//=============================================================================

//=============================================================================
// Function:    radXboxDriveFactory
//=============================================================================
// Description: This member is responsible for constructing a radXboxDriveObject.
//
// Parameters:  pointer to receive drive object
//              pointer to the drive name
//              allocator
//              
// Returns:     
//------------------------------------------------------------------------------

void radXboxDriveFactory
( 
    radDrive**         ppDrive, 
    const char*        pDriveName,
    radMemoryAllocator alloc
)
{
    //
    // Simply constuct the drive object.
    //
    *ppDrive = new( alloc ) radXboxDrive( pDriveName, alloc );
    rAssert( *ppDrive != NULL );
}


//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    radXboxDrive::radXboxDrive
//=============================================================================

radXboxDrive::radXboxDrive( const char* pdrivespec, radMemoryAllocator alloc )
    : 
    radDrive( ),
    m_OpenFiles( 0 ),
    m_pMutex( NULL )
{
    //
    // Depending on the drive letter, set the capabilities and the drive thread
    //
    if ( strcmp( pdrivespec, "D:" ) == 0 )
    {
        //
        // It's the DVD drive
        //
        m_Capabilities = ( radDriveEnumerable | radDriveFile );

        strcpy( m_MediaInfo.m_VolumeName, "DVD" );

        if ( s_pDVDDriveThread == NULL )
        {
            rAssert( s_pDVDMutex == NULL );

            //
            // Create a mutex for lock/unlock
            //
            radThreadCreateMutex( &s_pDVDMutex, alloc );
            rAssert( s_pDVDMutex != NULL );

            //
            // Create the drive thread
            //
            s_pDVDDriveThread = new( alloc ) radDriveThread( s_pDVDMutex, alloc );
            rAssert( s_pDVDDriveThread != NULL );

            //
            // Set the members to point to the globals
            //
            m_pMutex = s_pDVDMutex;
            m_pDriveThread = s_pDVDDriveThread;
        }
        else
        {
            //
            // Add ref the globals
            //
            m_pMutex = s_pDVDMutex;
            m_pMutex->AddRef( );

            m_pDriveThread = s_pDVDDriveThread;
            m_pDriveThread->AddRef( );
        }
    }
    else
    {
        //
        // It's a hard drive.
        //
        m_Capabilities = ( radDriveEnumerable | radDriveWriteable | radDriveDirectory | radDriveFile );
        if ( strcmp( pdrivespec, "Z:" ) == 0 )
        {
            //
            // We can format the utility drive.
            //
            m_Capabilities |= radDriveFormat;
        }
        else if( strcmp( pdrivespec, "W:" ) == 0 )
        {
            m_Capabilities = radDriveFile;
        }

        strcpy( m_MediaInfo.m_VolumeName, "Xbox Hard Disk" );

        if ( s_pHDDDriveThread == NULL )
        {
            rAssert( s_pHDDMutex == NULL );

            //
            // Create a mutex for lock/unlock
            //
            radThreadCreateMutex( &s_pHDDMutex, alloc );
            rAssert( s_pHDDMutex != NULL );

            //
            // Create the drive thread
            //
            s_pHDDDriveThread = new( alloc ) radDriveThread( s_pHDDMutex, alloc );
            rAssert( s_pHDDDriveThread != NULL );

            //
            // Set the members to point to the globals
            //
            m_pMutex = s_pHDDMutex;
            m_pDriveThread = s_pHDDDriveThread;
        }
        else
        {
            //
            // Add ref the globals
            //
            m_pMutex = s_pHDDMutex;
            m_pMutex->AddRef( );

            m_pDriveThread = s_pHDDDriveThread;
            m_pDriveThread->AddRef( );
        }
    }
    
    //
    // Nothing can be removed!
    //
    m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaPresent; 

    //
    // Copy the drivename
    //
    strncpy( m_DriveName, pdrivespec, radFileDrivenameMax );
    m_DriveName[ radFileDrivenameMax ] = '\0';
}

//=============================================================================
// Function:    radXboxDrive::~radXboxDrive
//=============================================================================

radXboxDrive::~radXboxDrive( void )
{
    if ( m_pDriveThread->GetRefCount( ) == 1 )
    {
        m_pMutex->Release( );
        m_pDriveThread->Release( );

        if ( strcmp( m_DriveName, "D:" ) == 0 )
        {
            s_pDVDDriveThread = NULL;
            s_pDVDMutex = NULL;
        }
        else
        {
            s_pHDDDriveThread = NULL;
            s_pHDDMutex = NULL;
        }
    }
    else
    {
        m_pMutex->Release( );
        m_pDriveThread->Release( );
    }
}

//=============================================================================
// Function:    radXboxDrive::Lock
//=============================================================================
// Description: Start a critical section
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radXboxDrive::Lock( void )
{
    m_pMutex->Lock( );
}

//=============================================================================
// Function:    radXboxDrive::Unlock
//=============================================================================
// Description: End a critical section
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radXboxDrive::Unlock( void )
{
    m_pMutex->Unlock( );
}

//=============================================================================
// Function:    radXboxDrive::GetCapabilities
//=============================================================================

unsigned int radXboxDrive::GetCapabilities( void )
{
    return m_Capabilities;
}

//=============================================================================
// Function:    radXboxDrive::GetDriveName
//=============================================================================

const char* radXboxDrive::GetDriveName( void )
{
    return m_DriveName;
}

//=============================================================================
// Function:    radXboxDrive::Initialize
//=============================================================================

radDrive::CompletionStatus radXboxDrive::Initialize( void )
{
    SetMediaInfo( );

    //
    // Success
    //
    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radXboxDrive::OpenFile
//=============================================================================

radDrive::CompletionStatus radXboxDrive::OpenFile
( 
    const char*         fileName, 
    radFileOpenFlags    flags, 
    bool                writeAccess, 
    radFileHandle*      pHandle, 
    unsigned int*       pSize 
)
{    
    //
    // Build the full filename
    //
    char fullName[ radFileFilenameMax + 1 ];
    BuildFileSpec( fileName, fullName, radFileFilenameMax + 1 );

    if ( strncmp( fullName, "W:", 2 ) == 0 )
    {
        //
        // Parse the filename and open a WMA file.
        // W:\<soundtrack id>\<song number>
        //
        DWORD soundtrackID;
        DWORD songNumber;
        if ( sscanf( fullName, "W:\\%u\\%u", &soundtrackID, &songNumber ) != 2 )
        {
            m_LastError = FileNotFound;
            return Error;
        }

        DWORD songID;
        DWORD songLen;
        XGetSoundtrackSongInfo( soundtrackID, songNumber, &songID, &songLen, NULL, 0 );
        *pHandle = XOpenSoundtrackSong( songID, false );
    }
    else
    {
        //
        // Translate flags to windows
        //
        DWORD accessFlag = writeAccess ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ;
        DWORD sharingFlag = writeAccess ? FILE_SHARE_READ | FILE_SHARE_WRITE : FILE_SHARE_READ;                
        DWORD createFlags;    
        switch( flags )
        {
        case OpenExisting:
            createFlags = OPEN_EXISTING;
            break;
        case OpenAlways:
            createFlags = OPEN_ALWAYS;
            break;
        case CreateAlways:
            createFlags = CREATE_ALWAYS;
            break;
        default:
            rAssertMsg( false, "radFileSystem: win32drive: attempting to open file with unknown flag" );
        }

        //
        // Right now using buffered reads, change it later for performance.
        //
        *pHandle = ::CreateFile( fullName, accessFlag, sharingFlag, NULL, 
                                createFlags, FILE_ATTRIBUTE_NORMAL, NULL );
    }

    if ( *pHandle != INVALID_HANDLE_VALUE )
    {
        //
        // Success! Set file size.
        //
        m_OpenFiles++;
        *pSize = ::GetFileSize( *pHandle, NULL );
        m_LastError = Success;
        return Complete;
    }
    else
    {
        m_LastError = TranslateError( ::GetLastError( ) );
        return Error;
    }
}

//=============================================================================
// Function:    radXboxDrive::CloseFile
//=============================================================================

radDrive::CompletionStatus radXboxDrive::CloseFile( radFileHandle handle, const char* fileName )
{
    ::CloseHandle( handle );
    m_OpenFiles--;
    return Complete;
}

//=============================================================================
// Function:    radXboxDrive::ReadFile
//=============================================================================

radDrive::CompletionStatus radXboxDrive::ReadFile
( 
    radFileHandle   handle,
    const char*     fileName,
    IRadFile::BufferedReadState buffState,
    unsigned int    position, 
    void*           pData, 
    unsigned int    bytesToRead, 
    unsigned int*   bytesRead, 
    radMemorySpace  pDataSpace 
)
{
    rAssertMsg( pDataSpace == radMemorySpace_Local, 
                "radFileSystem: radXboxDrive: External memory not supported for reads." );

    //
    // set file pointer
    //
    if ( ::SetFilePointer( handle, (LONG) position, NULL, FILE_BEGIN ) == (DWORD) position )
    {
        rReleasePrintf( "File: [%s] Bytes: [0x%x] t:[0x%x]\n", fileName, bytesToRead, radTimeGetMilliseconds( ) );
        if ( ::ReadFile( handle, pData, bytesToRead, (unsigned long*) bytesRead, NULL ) )
        {
            //
            // Successful read!
            //
            
            //
            // Change this during buffered read!!
            //
            *bytesRead = bytesToRead;
            m_LastError = Success;
            return Complete;
        }
    }

    //
    // Failed!
    //
    m_LastError = TranslateError( ::GetLastError( ) );
    return Error;
}

//=============================================================================
// Function:    radXboxDrive::WriteFile
//=============================================================================

radDrive::CompletionStatus radXboxDrive::WriteFile
( 
    radFileHandle     handle,
    const char*       fileName,
    IRadFile::BufferedReadState buffState,
    unsigned int      position, 
    const void*       pData, 
    unsigned int      bytesToWrite, 
    unsigned int*     bytesWritten, 
    unsigned int*     pSize, 
    radMemorySpace    pDataSpace 
)
{
    if ( !( m_Capabilities & radDriveWriteable ) )
    {
        rWarningMsg( m_Capabilities & radDriveWriteable, "This drive does not support the WriteFile function." );
        return Error;
    }

    rAssertMsg( pDataSpace == radMemorySpace_Local, 
                "radFileSystem: radXboxDrive: External memory not supported for reads." );
    
    //
    // presize the write
    //
    if ( position + bytesToWrite > *pSize )
    {
        ::SetFilePointer( handle, (LONG) position + bytesToWrite, NULL, FILE_BEGIN );
        ::SetEndOfFile( handle );
    }

    //
    // do the write
    //
    if ( ::SetFilePointer( handle, (LONG) position, NULL, FILE_BEGIN ) == (DWORD) position )
    {
        if ( ::WriteFile( handle, pData, bytesToWrite, (unsigned long*) bytesWritten, NULL ) )
        {
            //
            // Sucessful write
            //
            *pSize = ::GetFileSize( handle, NULL );
            m_LastError = Success;
            return Complete;
        }
    }

    //
    // Failed!
    //
    m_LastError = TranslateError( ::GetLastError( ) );
    return Error;
}

//=============================================================================
// Function:    radXboxDrive::FindFirst
//=============================================================================

radDrive::CompletionStatus radXboxDrive::FindFirst
( 
    const char*                 searchSpec, 
    IRadDrive::DirectoryInfo*   pDirectoryInfo, 
    radFileDirHandle*           pHandle,
    bool                        firstSearch
)
{
    rWarningMsg( m_Capabilities & radDriveEnumerable,
        "This drive does not support the FindFirst function." );

    //
    // Build the full filename
    //
    char fullSpec[ radFileFilenameMax + 1 ];
    BuildFileSpec( searchSpec, fullSpec, radFileFilenameMax + 1 );

    //
    // Close this search if it already exists
    //
    if ( !firstSearch && *pHandle != INVALID_HANDLE_VALUE )
    {
        ::FindClose( *pHandle );
    }

    //
    // Find first
    //
    WIN32_FIND_DATA FindData;
    *pHandle = ::FindFirstFile( fullSpec, &FindData );

    //
    // Fill in our directory info structure
    //
    m_LastError = TranslateDirInfo( pDirectoryInfo, &FindData, pHandle );

    if ( m_LastError == Success )
    {
        return Complete;
    }
    else
    {
        return Error;
    }
}

//=============================================================================
// Function:    radXboxDrive::FindNext
//=============================================================================

radDrive::CompletionStatus radXboxDrive::FindNext( radFileDirHandle* pHandle, IRadDrive::DirectoryInfo* pDirectoryInfo )
{
    rWarningMsg( m_Capabilities & radDriveEnumerable,
        "This drive does not support the FindNext function." );

    //
    // If we don't have a handle, return file not found.
    //
    if ( *pHandle == INVALID_HANDLE_VALUE )
    {
        m_LastError = FileNotFound;
        return Error;
    }

    //
    // Find the next entry
    //
    WIN32_FIND_DATA FindData;
    if ( !::FindNextFile( *pHandle, &FindData ) )
    {
        radFileDirHandle h = INVALID_HANDLE_VALUE;
        m_LastError = TranslateDirInfo( pDirectoryInfo, &FindData, &h );
    }
    else
    {
        m_LastError = TranslateDirInfo( pDirectoryInfo, &FindData, pHandle );
    }

    if ( m_LastError == Success )
    {
        return Complete;
    }
    else
    {
        return Error;
    }
}

//=============================================================================
// Function:    radXboxDrive::FindCloseX
//=============================================================================

radDrive::CompletionStatus radXboxDrive::FindCloseX( radFileDirHandle* pHandle )
{
    rWarningMsg( m_Capabilities & radDriveEnumerable,
        "This drive does not support the FindClose function." );

    ::FindClose( *pHandle );
    *pHandle = INVALID_HANDLE_VALUE;

    return Complete;
}

//=============================================================================
// Function:    radXboxDrive::CreateDir
//=============================================================================

radDrive::CompletionStatus radXboxDrive::CreateDir( const char* pName )
{
    rWarningMsg( m_Capabilities & radDriveDirectory, 
        "This drive does not support the CreateDir function." );

    //
    // Build the full filename
    //
    char fullSpec[ radFileFilenameMax + 1 ];
    BuildFileSpec( pName, fullSpec, radFileFilenameMax + 1 );

    if ( ::CreateDirectory( fullSpec, NULL ) )
    {
        m_LastError = Success;
        return Complete;
    }
    else
    {
        m_LastError = TranslateError( ::GetLastError( ) );
        return Error;
    }
}

//=============================================================================
// Function:    radXboxDrive::DestroyDir
//=============================================================================

radDrive::CompletionStatus radXboxDrive::DestroyDir( const char* pName )
{
    rWarningMsg( m_Capabilities & radDriveDirectory,
        "This drive does not support the DestroyDir function." );

    //
    // Build the full filename
    //
    char fullSpec[ radFileFilenameMax + 1 ];
    BuildFileSpec( pName, fullSpec, radFileFilenameMax + 1 );

    if ( ::RemoveDirectory( fullSpec ) )
    {
        m_LastError = Success;
        return Complete;
    }
    else
    {
        m_LastError = TranslateError( ::GetLastError( ) );
        return Error;
    }
}

//=============================================================================
// Function:    radXboxDrive::DestroyFile
//=============================================================================

radDrive::CompletionStatus radXboxDrive::DestroyFile( const char* filename )
{
    rWarningMsg( m_Capabilities & radDriveWriteable, 
        "This drive does not support the DestroyFile function." );

    //
    // Someday check if the file is open!
    //

    //
    // Build the full filename
    //
    char fullSpec[ radFileFilenameMax + 1 ];
    BuildFileSpec( filename, fullSpec, radFileFilenameMax + 1 );

    if ( ::DeleteFile( fullSpec ) )
    {
        m_LastError = Success;
        return Complete;
    }
    else
    {
        m_LastError = TranslateError( ::GetLastError( ) );
        return Error;
    }
}

//=============================================================================
// Function:    radXboxDrive::Format
//=============================================================================

radDrive::CompletionStatus radXboxDrive::Format( void )
{
    if ( m_Capabilities & radDriveFormat )
    {
        if ( !::XFormatUtilityDrive( ) )
        {
            m_LastError = TranslateError( ::GetLastError( ) );
        }
    }

    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Private Member Functions
//=============================================================================

//=============================================================================
// Function:    radXboxDrive::SetMediaInfo
//=============================================================================
// Description: Set up the media info free space and sector size.
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

void radXboxDrive::SetMediaInfo( void )
{
    char Drive[ 4 ];
    rAssert( strlen( m_DriveName ) == 2 );
    strcpy( Drive, m_DriveName );
    strcat( Drive, "\\");
    
    ULARGE_INTEGER FreeBytesAvailableToCaller; 
    ULARGE_INTEGER TotalNumberOfBytes; 
    ULARGE_INTEGER TotalNumberOfFreeBytes; 
    
    ::GetDiskFreeSpaceEx( Drive, &FreeBytesAvailableToCaller,          
        &TotalNumberOfBytes, &TotalNumberOfFreeBytes );
    
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
    // No file limit!
    //
    m_MediaInfo.m_FreeFiles = m_MediaInfo.m_FreeSpace / m_MediaInfo.m_SectorSize;
}

//=============================================================================
// Function:    radXboxDrive::BuildFileSpec
//=============================================================================

void radXboxDrive::BuildFileSpec( const char* fileName, char* fullName, unsigned int size )
{
    //
    // Add drive name with slash
    //
    int len = strlen( m_DriveName ) + 1;
    strncpy( fullName, m_DriveName, size - 1 );
    strncpy( &fullName[ len - 1 ], "\\", size - len );

    //
    // Don't repeat the leading slash.
    //
    if ( fileName[ 0 ] == '\\' )
    {
        strncpy( &fullName[ len ], &fileName[ 1 ], size - len - 1 );
    }
    else
    {
        strncpy( &fullName[ len ], fileName, size - len - 1 );
    }
    fullName[ size - 1 ] = '\0';
}

//=============================================================================
// Function:    radXboxDrive::TranslateError
//=============================================================================
// Description: This translates the windows error code into our error enums.
//
// Parameters:  Win32 error code
//              
// Returns:     Our error code
//------------------------------------------------------------------------------

radFileError radXboxDrive::TranslateError( unsigned int windowsError )
{
    //
    // we shouldn't have success here. If we do we made a mistake earlier.
    //
    switch( windowsError )
    {
    case ERROR_SUCCESS:
        return( Success );

    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_INVALID_DRIVE:
        return( FileNotFound );

    case ERROR_WRONG_DISK :
    case ERROR_NOT_SAME_DEVICE:
        return( WrongMedia );

    case ERROR_NOT_READY:
        return( ShellOpen );

    case ERROR_UNRECOGNIZED_VOLUME:
        return ( MediaCorrupt );

    case ERROR_NOT_DOS_DISK:
        return ( MediaInvalid );

    case ERROR_DEVICE_NOT_CONNECTED:
        return( NoMedia );

    case ERROR_BAD_FORMAT:
    case ERROR_SECTOR_NOT_FOUND:
        return( MediaNotFormatted );

    case ERROR_DISK_FULL:
        return ( NoFreeSpace );

    case ERROR_OUTOFMEMORY:
        return ( HardwareFailure );
        
    default:
        return( HardwareFailure );
    }
}

//=============================================================================
// Function:    radXboxDrive::TranslateDirInfo
//=============================================================================
// Description: Translate the directory info and return an error status. A handle
//              with value INVALID_HANDLE_VALUE means the find_first/next call
//              failed and needs to be checked if something went wrong or of the
//              search just ended.
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------
radFileError radXboxDrive::TranslateDirInfo
( 
    IRadDrive::DirectoryInfo*   pDirectoryInfo, 
    const void*                 pFindDataVoid,
    const radFileDirHandle*     pHandle
)
{
    const WIN32_FIND_DATA* pFindData = (WIN32_FIND_DATA*) pFindDataVoid;

    if ( *pHandle == INVALID_HANDLE_VALUE )
    {
        //
        // Either we failed or we're out of games.
        //
        DWORD error = ::GetLastError( );
        if ( error == ERROR_NO_MORE_FILES || error == ERROR_FILE_NOT_FOUND )
        {
            pDirectoryInfo->m_Name[0] = '\0';
            pDirectoryInfo->m_Type = IRadDrive::DirectoryInfo::IsDone;
        }
        else
        {
            return TranslateError( error );
        }
    }
    else
    {
        strncpy( pDirectoryInfo->m_Name, pFindData->cFileName, radFileFilenameMax );
        pDirectoryInfo->m_Name[ radFileFilenameMax ] = '\0';

        if ( pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        {
            pDirectoryInfo->m_Type = IRadDrive::DirectoryInfo::IsDirectory;
        }
        else
        {
            pDirectoryInfo->m_Type = IRadDrive::DirectoryInfo::IsFile;
        }
    }
    return Success;
}
