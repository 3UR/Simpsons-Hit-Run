//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        ps2cddrive.cpp
//
// Subsystem:	Radical File System
//
// Description:	This file contains the implementation of the ps2 cd/dvd drive.
//
// Revisions:	
//
// Notes:       This implements the ISO standard. We check if the tray was opened
//              before all operations. If it was, then we check if it's the same
//              disk. If not, we either load the new disk if no files are open,
//              or return an error. The logic for all this is in SetMediaInfo
//              and DiskChanged.
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include <libcdvd.h>
#include <string.h>
#include <stdlib.h>
#include <radplatform.hpp>
#include <radstring.hpp>
#include <radtime.hpp>
#include "ps2cddrive.hpp"

//=============================================================================
// Local Definitions
//=============================================================================

//
// The following are magic numbers from the ISO 9660 standard
// *_LOC    -- location within the record
// *_LEN    -- length
// *_ADDR   -- logical sector number
//

//
// Volume Descriptor info:
// VOL_DESCRIPTOR_IDENT_LEN:    length of the identifier
// VOL_DESCRIPTOR_ADDR:         address of descriptor
// VOL_DESCRIPTOR_IDENT_LOC:    location of the identifier
// VOL_DESCRIPTOR_RECORD_LOC:   location of directory record
//
#define VOL_DESCRIPTOR_IDENT_LEN    64
#define VOL_DESCRIPTOR_ADDR         16

#define VOL_DESCRIPTOR_IDENT_LOC    8
#define VOL_DESCRIPTOR_RECORD_LOC   156

//
// Directory record info:
// DIR_RECORD_LEN_LOC           location of the length of the directory record
// DIR_RECORD_EXTENT_ADDR_LOC:  location of the address of the extent
// DIR_RECORD_EXTENT_LEN_LOC:   location of the kength of the extent
// DIR_RECORD_FLAGS_LOC:        location of file flags
// DIR_RECORD_IDENT_LEN_LOC:    location of the length of the identifier
// DIR_RECORD_IDENT_LOC:        location of the identifier
//
#define DIR_RECORD_LEN_LOC              0
#define DIR_RECORD_EXTENT_ADDR_LOC      2
#define DIR_RECORD_EXTENT_LEN_LOC       10
#define DIR_RECORD_FLAGS_LOC            25
#define DIR_RECORD_IDENT_LEN_LOC        32
#define DIR_RECORD_IDENT_LOC            33

//
// File flags.
//
#define FILE_FLAG_DIR                   0x02

//
// Read a 32 bit little endian number
//
inline unsigned int littleEndianToUInt( unsigned char* pChar )
{
    return ((unsigned int)((pChar[3] << 24) + (pChar[2] << 16) + (pChar[1] << 8) + pChar[0]));
}

inline unsigned int HashFilename( const char* pToken, unsigned int keyValue = 0 )
{
	if ( pToken != NULL )
	{
		while( *pToken )
		{
			//
			// X31 Alogrithm
			//
			keyValue = ( keyValue << 5 ) - keyValue + (unsigned int) *pToken;
    
			//
			// Update the token's position
			//
			pToken++;
		}
    
		return( keyValue );
	}

	return 0;
}

//=============================================================================
// Public Functions 
//=============================================================================

//=============================================================================
// Function:    radPs2CdDriveFactory
//=============================================================================
// Description: This member is responsible for constructing a radPs2CdDrive object.
//
// Parameters:  pointer to receive drive object
//              pointer to the drive name
//              allocator
//              
// Returns:     
//------------------------------------------------------------------------------

void radPs2CdDriveFactory
( 
    radDrive**         ppDrive, 
    const char*        pDriveName,
    radMemoryAllocator alloc
)
{
    //
    // Simply constuct the drive object.
    //
    *ppDrive = new( alloc ) radPs2CdDrive( alloc );
    rAssert( *ppDrive != NULL );
}

//=============================================================================
// Public Member Functions
//=============================================================================

radPs2CdDrive::radPs2CdDrive( radMemoryAllocator alloc )
    : 
    radDrive( ),
    m_OpenFiles( 0 ),
    m_NumDirEntries( 0 ),
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
    // Media info: these variables never change
    //
    m_MediaInfo.m_FreeFiles = 0;
    m_MediaInfo.m_FreeSpace = 0;

    //
    // Initially we have no disk.
    //
    m_VolumeName[ 0 ] = '\0';

    //
    // Set up our aligned buffer
    //
    rAssert( radMemorySpace_OptimalAlignment <= PS2_CD_DRIVE_ALIGNMENT );
    rAssert( radFileOptimalMemoryAlignment <= PS2_CD_DRIVE_ALIGNMENT );

    m_SectorBuffer = (unsigned char*) ::radMemoryRoundUp( (unsigned int) m_SectorBufferSpace, PS2_CD_DRIVE_ALIGNMENT );
}

radPs2CdDrive::~radPs2CdDrive( void )
{
    m_pMutex->Release( );
    m_pDriveThread->Release( );
}

//=============================================================================
// Function:    radPs2CdDrive::Lock
//=============================================================================
// Description: Start a critical section
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radPs2CdDrive::Lock( void )
{
    m_pMutex->Lock( );
}

//=============================================================================
// Function:    radPs2CdDrive::Unlock
//=============================================================================
// Description: End a critical section
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radPs2CdDrive::Unlock( void )
{
    m_pMutex->Unlock( );
}

//=============================================================================
// Function:    radPs2CdDrive::GetCapabilities
//=============================================================================

unsigned int radPs2CdDrive::GetCapabilities( void )
{
    return ( radDriveRemovable | radDriveEnumerable | radDriveFile );
}

//=============================================================================
// Function:    radPs2CdDrive::GetDriveName
//=============================================================================

const char* radPs2CdDrive::GetDriveName( void )
{
    return s_PS2CDDriveName;
}

//=============================================================================
// Function:    radPs2CdDrive::GetReadBufferSectors
//=============================================================================

unsigned int radPs2CdDrive::GetReadBufferSectors( void )
{
    return PS2_CD_DRIVE_TRANSFER_BUFFER_SECTORS;
}

//=============================================================================
// Function:    radPs2CdDrive::Initialize
//=============================================================================

radDrive::CompletionStatus radPs2CdDrive::Initialize( void )
{
    //
    // First lets check if we have initialized the underlying system. This is done by 
    // the platform system since it needs to load IRXs.
    //
    static bool g_IsInitialized = false;
    if( !g_IsInitialized )
    {
        //
        // Simply get the plaform interface. If non-null, things have been initialized.
        //
        rAssert( radPlatformGet( ) != NULL );
     
        g_IsInitialized = true;
    }

    if ( !SetMediaInfo( ) )
    {
        if ( m_OpenFiles > 0 )
        {
            //
            // If there's media present, then it's wrong!
            //
            switch( m_MediaInfo.m_MediaState )
            {
            case IRadDrive::MediaInfo::MediaPresent:
            case IRadDrive::MediaInfo::MediaNotFormatted:
            case IRadDrive::MediaInfo::MediaEncodingErr:
            case IRadDrive::MediaInfo::MediaWrongType:
            case IRadDrive::MediaInfo::MediaInvalid:
            case IRadDrive::MediaInfo::MediaDamaged:
                m_LastError = WrongMedia;
                break;
            default:
                break;
            }
        }
        return Error;
    }

    //
    // Success
    //
    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radPs2CdDrive::OpenFile
//=============================================================================

radDrive::CompletionStatus radPs2CdDrive::OpenFile
( 
    const char*        fileName, 
    radFileOpenFlags   flags, 
    bool               writeAccess, 
    radFileHandle*     pHandle, 
    unsigned int*      pSize 
)
{
    rAssert( writeAccess == false );
    rAssert( flags == OpenExisting );

    //
    // Check if the disk changed.
    //
    radFileError chError = DiskChanged( );
    if ( chError != Success )
    {
        m_LastError = chError;
        return Error;
    }

    //
    // Build the full filename
    //
    char fullName[ MAX_PS2FILENAMELEN + 1 ];
    char* pName;
    BuildFileSpec( fileName, fullName, MAX_PS2FILENAMELEN + 1, &pName );

    //
    // Hash the name (in upper case) without the drive.
    //
    strupr( fullName );
    DirectoryEntry hashEntry;
    hashEntry.m_HashValue = HashFilename( pName );

    //
    // Look up the name using a binary search
    //
    unsigned int index;
    if ( !radBinarySearch( hashEntry, m_Directory, m_NumDirEntries, &index ) )
    {
        m_LastError = FileNotFound;
        return Error;
    }

    *pHandle = m_Directory[ index ].m_Lsn;
    *pSize = m_Directory[ index ].m_Size;

    m_OpenFiles++;
    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radPs2CdDrive::CloseFile
//=============================================================================

radDrive::CompletionStatus radPs2CdDrive::CloseFile( radFileHandle handle, const char* fileName )
{
    m_OpenFiles--;
    return Complete;
}

//=============================================================================
// Function:    radPs2CdDrive::ReadAligned
//=============================================================================

radDrive::CompletionStatus radPs2CdDrive::ReadAligned
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
                "radFileSystem: radPs2CdDrive: reads only supported for EE or IOP memory." );

    //
    // Check if the disk changed.
    //
    radFileError chError = DiskChanged( );
    if ( chError != Success )
    {
        m_LastError = chError;
        return Error;
    }
#ifdef RADFILE_SPEW    
    rReleasePrintf( "FILE: (optimal)  [%s]", fileName );
#endif        
    m_LastError = Read( handle + sector, pData, numSectors, pDataSpace == radMemorySpace_Iop );

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
// Function:    radPs2CdDrive::ReadBuffered
//=============================================================================

radDrive::CompletionStatus radPs2CdDrive::ReadBuffered
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
                "radFileSystem: radPs2CdDrive: reads only supported for EE or IOP memory." );
    
    //
    // Check if the disk changed.
    //
    radFileError chError = DiskChanged( );
    if ( chError != Success )
    {
        m_LastError = chError;
        return Error;
    }

    //
    // Read into our buffer (in local space!!)
    //
#ifdef RADFILE_SPEW
    rReleasePrintf( "FILE: (buffered) [%s]", fileName );
#endif
    m_LastError = Read( handle + sector, m_SectorBuffer, numSectors, false );

    if ( m_LastError != Success )
    {
        return Error;
    }

    //
    // Copy into the user's buffer.
    //
    IRadMemorySpaceCopyRequest* pMemCpyRequest = 
        radMemorySpaceCopyAsync( pData, 
                                 pDataSpace, 
                                 &m_SectorBuffer[ position ], 
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
    };
    
    pMemCpyRequest->Release( );
    pMemCpyRequest = NULL;
    
    return Complete;
}

//=============================================================================
// Function:    radPs2CdDrive::FindFirst
//=============================================================================

radDrive::CompletionStatus radPs2CdDrive::FindFirst
(
    const char*                 searchSpec, 
    IRadDrive::DirectoryInfo*   pDirectoryInfo, 
    radFileDirHandle*           pHandle,
    bool                        firstSearch 
)
{
    //
    // Check if the disk changed.
    //
    radFileError chError = DiskChanged( );
    if ( chError != Success )
    {
        m_LastError = chError;
        return Error;
    }

    //
    // Build the full filename
    //
    char fullName[ radFileFilenameMax + 1 ];
    char* pName;
    BuildFileSpec( searchSpec, fullName, radFileFilenameMax + 1, &pName );

    //
    // Find a directory part and a wildcard part
    // There is no initial slash, so end == 0 means no directory was found.
    //
    unsigned int end = strlen( pName ) - 1;
    while ( end > 0 && pName[ end ] != '\\' )
    {
        end--;
    }
    rAssertMsg( strlen( &pName[ end + 1 ] ) <= RAD_PS2_SPEC_LEN, "radFileSystem: long search spec." );

    //
    // end points to the slash before the wildcard, or if 0, there is no directory
    //
    unsigned int size;
    radFileError result = FindDirectorySector( pName, end, &pHandle->m_Lsn, &size );
    if ( result != Success )
    {
        m_LastError = result;
        return Error;
    }

    //
    // Set up the directory handle and find the next occurence
    //
    pHandle->m_Offset = 0;
    pHandle->m_RemSect = ( size + m_MediaInfo.m_SectorSize - 1 ) / m_MediaInfo.m_SectorSize;

    if ( end > 0 )
    {
        strcpy( pHandle->m_pSpec, &pName[ end + 1 ] );
    }
    else
    {
        strcpy( pHandle->m_pSpec, pName );
    }

    return FindNext( pHandle, pDirectoryInfo );
}

//=============================================================================
// Function:    radPs2CdDrive::FindNext
//=============================================================================

radDrive::CompletionStatus radPs2CdDrive::FindNext( radFileDirHandle* pHandle, IRadDrive::DirectoryInfo* pDirectoryInfo )
{
    //
    // Check if the disk changed.
    //
    radFileError chError = DiskChanged( );
    if ( chError != Success )
    {
        m_LastError = chError;
        return Error;
    }

    //
    // Take the directory handle, and go through the current directory matching files
    //
    bool found = false;
    while ( true )
    {
        //
        // Read a sector if there are any left
        //
        if ( pHandle->m_RemSect == 0 )
        {
            pDirectoryInfo->m_Name[0] = '\0';
            pDirectoryInfo->m_Type = IRadDrive::DirectoryInfo::IsDone;
            m_LastError = Success;
            return Complete;
        }

        radFileError result = Read( pHandle->m_Lsn, m_SectorBuffer, 1, false );
        if ( result != Success )
        {
            m_LastError = result;
            return Error;
        }

        unsigned char* pDirInfo = &m_SectorBuffer[ pHandle->m_Offset ];

        //
        // Parse through the sector
        //
        unsigned char* pSectorEnd = m_SectorBuffer + m_MediaInfo.m_SectorSize - 1;
        while ( !found && pDirInfo < pSectorEnd && pDirInfo[ DIR_RECORD_LEN_LOC ] != 0 )
        {
            //
            // Read the next record and check if it's right
            //
            char pName[ MAX_PS2FILENAMELEN ];
            unsigned int nameLen = pDirInfo[ DIR_RECORD_IDENT_LEN_LOC ];
            strncpy( pName, (char*) &pDirInfo[ DIR_RECORD_IDENT_LOC ], nameLen );
            pName[ nameLen ] = '\0';
            strupr( pName );

            //
            // If this isn't the current or parent directory, try and match it.
            //
            if ( pName[ 0 ] != 0 && pName[ 0 ] != 1 )
            {
                if ( !( pDirInfo[ DIR_RECORD_FLAGS_LOC ] & FILE_FLAG_DIR ) )
                {
                    pName[ nameLen - 2 ] = '\0'; // remove ;1
                }

                if ( FileNameMatchesSearch( pName, pHandle->m_pSpec ) )
                {
                    //
                    // found a match!
                    //
                    found = true;
                    if ( pDirInfo[ DIR_RECORD_FLAGS_LOC ] & FILE_FLAG_DIR )
                    {
                        pDirectoryInfo->m_Type = IRadDrive::DirectoryInfo::IsDirectory;
                    }
                    else
                    {
                        pDirectoryInfo->m_Type = IRadDrive::DirectoryInfo::IsFile;
                    }
                    strcpy( pDirectoryInfo->m_Name, pName );
                }
            }

            //
            // Go to the next record
            //
            pHandle->m_Offset += pDirInfo[ DIR_RECORD_LEN_LOC ];
            pDirInfo += pDirInfo[ DIR_RECORD_LEN_LOC ];
        }

        if ( found )
        {
            break;
        }

        //
        // Continue the search
        //
        pHandle->m_Offset = 0;
        pHandle->m_Lsn++;
        pHandle->m_RemSect--;
    }

    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radPs2CdDrive::FindClose
//=============================================================================

radDrive::CompletionStatus radPs2CdDrive::FindClose( radFileDirHandle* pHandle )
{
    return Complete;
}

//=============================================================================
// Private Member Functions
//=============================================================================

IRadDrive::MediaInfo::MediaState ErrorToMediaState( radFileError error )
{
    switch( error )
    {
    case Success:
        return IRadDrive::MediaInfo::MediaPresent;

    case NoMedia:
    case ShellOpen:
        return IRadDrive::MediaInfo::MediaNotPresent;

    case WrongMedia:
        return IRadDrive::MediaInfo::MediaPresent;

    case MediaNotFormatted:
        return IRadDrive::MediaInfo::MediaNotFormatted;

    case MediaCorrupt:
        return IRadDrive::MediaInfo::MediaDamaged;

    case MediaEncodingErr:
        return IRadDrive::MediaInfo::MediaEncodingErr;

    case MediaWrongType:
        return IRadDrive::MediaInfo::MediaWrongType;
    
    case MediaInvalid:
        return IRadDrive::MediaInfo::MediaInvalid;

    case HardwareFailure:
    case NoFreeSpace:
    case FileNotFound:
    case DataCorrupt:
    default:
        return IRadDrive::MediaInfo::MediaNotPresent;
    };
}

//=============================================================================
// Function:    radPs2CdDrive::SetMediaInfo
//=============================================================================
// Description: Set up the media info.
//
// Parameters:  
//------------------------------------------------------------------------------

bool radPs2CdDrive::SetMediaInfo( void )
{
    radFileError result = Success;

    //
    // Initialize our check for if the disk has changed.
    //
    unsigned int tmp;
    ::sceCdTrayReq( SCECdTrayCheck, &tmp );

    //
    // Wait for device to become ready. Do not wait if shell open however.
    //
    while( ::sceCdDiskReady( 1 ) == SCECdNotReady )
    {
        if ( ::sceCdStatus( ) == SCECdStatShellOpen )
        {
            m_MediaInfo.m_SectorSize = PS2_DVD_SECTOR_SIZE;
            m_MediaInfo.m_VolumeName[0] = '\0';
            m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaNotPresent;
            m_LastError = ShellOpen;
            return false;
        }
        ::radThreadSleep( 1 );
    }

    //
    // Get media type and set up sector size appropriately. We ignore most types
    // since they we will be caught during the read operation.
    //
    switch( ::sceCdGetDiskType( ) )
    {
    case SCECdPS2CD:
    case SCECdPS2CDDA:
    case SCECdPSCD:
    case SCECdPSCDDA:
        m_MediaInfo.m_SectorSize = PS2_CD_SECTOR_SIZE;
        break;

    default:
        m_MediaInfo.m_SectorSize = PS2_DVD_SECTOR_SIZE;
        break;
    }

    //
    // Get the volume name. And check to see if we need to load the directory info.
    //
    result = ReadVolumeName( m_MediaInfo.m_VolumeName );
    if ( result != Success )
    {
        m_MediaInfo.m_SectorSize = PS2_DVD_SECTOR_SIZE;
        m_MediaInfo.m_VolumeName[0] = '\0';
        m_MediaInfo.m_MediaState = ErrorToMediaState( result );
        m_LastError = result;
        return false;
    }

    if ( ::memcmp( m_VolumeName, m_MediaInfo.m_VolumeName, VOL_DESCRIPTOR_IDENT_LEN ) != 0 )
    {
        //
        // Disk changed!
        //
        if ( m_OpenFiles > 0 )
        {
            m_LastError = WrongMedia;
            m_MediaInfo.m_MediaState = ErrorToMediaState( WrongMedia );
            return false;
        }

        result = LoadDirectory( );
        if ( result != Success )
        {
            m_MediaInfo.m_SectorSize = PS2_DVD_SECTOR_SIZE;
            m_MediaInfo.m_VolumeName[0] = '\0';
            m_MediaInfo.m_MediaState = ErrorToMediaState( result );
            m_LastError = result;
            return false;
        }
        ::memcpy( m_VolumeName, m_MediaInfo.m_VolumeName, VOL_DESCRIPTOR_IDENT_LEN );
    }

    m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaPresent;
    m_LastError = Success;
    return true;
}

//=============================================================================
// Function:    radPs2CdDrive::ReadVolumeName
//=============================================================================
// Description: Read the volume name.
//
// Parameters:  nameBuffer -- buffer to put the name into.
//              
// Returns:     
//------------------------------------------------------------------------------

radFileError radPs2CdDrive::ReadVolumeName( char* nameBuffer )
{
    //
    // First get the volume descriptor; just use our sector buffer for the read
    //
    unsigned char* pVolDesc = m_SectorBuffer;
    radFileError result = Read( VOL_DESCRIPTOR_ADDR, pVolDesc, 1, false );

    if ( result == Success )
    {
        ::memcpy( nameBuffer, &pVolDesc[ VOL_DESCRIPTOR_IDENT_LOC ], VOL_DESCRIPTOR_IDENT_LEN );
        nameBuffer[ VOL_DESCRIPTOR_IDENT_LEN ] = '\0';
    }

    return result;
}

//=============================================================================
// Function:    radPs2CdDrive::LoadDirectory
//=============================================================================
// Description: Load our hash table for directories. The volume
//              descriptor must be in m_SectorBuffer for this to work.
//
// Parameters:
//              
// Returns:     
//------------------------------------------------------------------------------

radFileError radPs2CdDrive::LoadDirectory( void )
{
    //
    // build table of files
    //
    m_NumDirEntries = 0;

    unsigned char* pVolDesc = m_SectorBuffer;
    unsigned char* pDirRecord = &pVolDesc[ VOL_DESCRIPTOR_RECORD_LOC ];

    char pName[ MAX_PS2FILENAMELEN + 1 ];
    radFileError result = BuildDirectory( 
                            littleEndianToUInt( &pDirRecord[ DIR_RECORD_EXTENT_ADDR_LOC ] ),
                            0, 
                            pName, 
                            0 );

    if ( result == Success )
    {
        //
        // sort our table by hash key
        //
        ::qsort( 
            m_Directory, 
            m_NumDirEntries, 
            sizeof( DirectoryEntry ), 
            &DirectoryEntry::compare );
    }
    else
    {
        m_NumDirEntries = 0;
    }

    return result;
}

//=============================================================================
// Function:    radPs2CdDrive::BuildDirectory
//=============================================================================
// Description: Recursively build up the directory structure.
//
// Parameters:  dirAddress -- sector address of current directory
//              level -- how many subdirectories deep are we.
//              pName -- buffer to use so as not to make lots of stack variables.
//              key -- the hash key of the full filename so far.
//
// Returns:     
//
// Notes:      This is a recursive function. It finds all files in the directory
//             and adds them to the hash table. All directories are put in temporary
//             traversal storage. Then once the directory is fully parsed, it recurses
//             on all the directories. The hash value of the name up to that point is
//             passed so that a running hash can be used to find the names of the files
//             in the subdirectory.
//------------------------------------------------------------------------------

radFileError radPs2CdDrive::BuildDirectory( unsigned int dirAddress, unsigned int level, char* pName, unsigned int key )
{
    rAssert( level < MAX_SUBDIRS );
    unsigned int numDirs = 0;

    //
    // Read the given sector, which should be the current directory.
    //
    unsigned char* pDirInfo = m_SectorBuffer;
    radFileError result = Read( dirAddress, pDirInfo, 1, false );
    if ( result != Success )
    {
        return result;
    }

    //
    // Find the extent and the number of sectors in the extent
    //
    unsigned int extent = littleEndianToUInt( &pDirInfo[ DIR_RECORD_EXTENT_ADDR_LOC ] );
    unsigned extentLen = littleEndianToUInt( &pDirInfo[ DIR_RECORD_EXTENT_LEN_LOC ] );

    rAssert( pDirInfo[ DIR_RECORD_FLAGS_LOC ] & FILE_FLAG_DIR );
    rAssert( extent == dirAddress );
    
    unsigned int numSectors = ( extentLen + m_MediaInfo.m_SectorSize - 1 ) / m_MediaInfo.m_SectorSize;
    rAssert( numSectors >= 1 );

    //
    // Go to the next record (which should be the parent directory) and read 
    // a new sector if needed. (don't need to read new sector?? guaranteed size 34?)
    //
    unsigned char* pSectorEnd = m_SectorBuffer + m_MediaInfo.m_SectorSize - 1;
    pDirInfo += pDirInfo[ DIR_RECORD_LEN_LOC ];

    if ( pDirInfo > pSectorEnd )
    {
        extent++;
        result = Read( extent, m_SectorBuffer, 1, false );
        if ( result != Success )
        {
            return result;
        }

        numSectors--;
        pDirInfo = m_SectorBuffer;
    }

    //
    // Read the rest of the entries, reading new sectors as requires
    //
    while( true )
    {
        pDirInfo += pDirInfo[ DIR_RECORD_LEN_LOC ];

        //
        // Check of there are more sectors to read
        //
        if ( pDirInfo > pSectorEnd || pDirInfo[ DIR_RECORD_LEN_LOC ] == 0 )
        {
            if ( numSectors == 1 )
            {
                break;
            }

            //
            // Recurse on any directories from the last sector
            //
            for ( unsigned int i = 0; i < numDirs; i++ )
            {
                result = BuildDirectory( 
                    m_TmpTravInfo[ level ][ i ].m_address, 
                    level + 1, 
                    pName, 
                    m_TmpTravInfo[ level ][ i ].m_key );
            }

            numDirs = 0;
            extent++;
            result = Read( extent, m_SectorBuffer, 1, false );
            if ( result != Success )
            {
                return result;
            }

            numSectors--;
            pDirInfo = m_SectorBuffer;
        }

        //
        // Got a valid entry, find its name. If it's a directory store it for 
        // recursion, if it's a file put it in the table.
        //
        unsigned int nameLen = pDirInfo[ DIR_RECORD_IDENT_LEN_LOC ];
        rAssert( nameLen <= MAX_PS2FILENAMELEN );

        strncpy( pName, (char*) &pDirInfo[ DIR_RECORD_IDENT_LOC ], nameLen );
        pName[ nameLen ] = '\0';
        strupr( pName );

        if ( pDirInfo[ DIR_RECORD_FLAGS_LOC ] & FILE_FLAG_DIR )
        {
            //
            // Directory, add in a slash
            //
            pName[ nameLen ] = '\\';
            pName[ nameLen + 1] = '\0';

            m_TmpTravInfo[ level ][ numDirs ].m_key = HashFilename( pName, key );
            m_TmpTravInfo[ level ][ numDirs ].m_address = 
                littleEndianToUInt( &pDirInfo[ DIR_RECORD_EXTENT_ADDR_LOC ] );
            numDirs++;
        }
        else
        {
            //
            // File, store it in our table.
            //
            pName[ nameLen - 2 ] = '\0'; // remove ;1
            
            rAssert( m_NumDirEntries < MAX_DIRECTORYFILES );

            m_Directory[ m_NumDirEntries ].m_HashValue = HashFilename( pName, key );
            m_Directory[ m_NumDirEntries ].m_Lsn = 
                littleEndianToUInt( &pDirInfo[ DIR_RECORD_EXTENT_ADDR_LOC ] );
            m_Directory[ m_NumDirEntries ].m_Size = 
                littleEndianToUInt( &pDirInfo[ DIR_RECORD_EXTENT_LEN_LOC ] );

            //
            // In debug mode, check for a hash collisions
            //
#       ifdef RAD_DEBUG
            unsigned int hashKey = m_Directory[ m_NumDirEntries ].m_HashValue;
            for ( unsigned int i = 0; i < m_NumDirEntries; i++ )
            {
                if ( m_Directory[ i ].m_HashValue == hashKey )
                {
                    rDebugPrintf( "radFileSystem: PS2CD hash collision with key %u on file %s\n", 
                        hashKey, pName );
                    rAssert( false );
                }
            }
#       endif

            m_NumDirEntries++;
        }
    }

    //
    // Recurse on any directories from the last sector
    //
    for ( unsigned int i = 0; i < numDirs; i++ )
    {
        result = BuildDirectory( 
            m_TmpTravInfo[ level ][ i ].m_address, 
            level + 1, 
            pName, 
            m_TmpTravInfo[ level ][ i ].m_key );
    }

    return result;
}

//=============================================================================
// Function:    radPs2CdDrive::FindDirectorySector
//=============================================================================
// Description: Find the logical sector address and size for the directory matching
//              the name.
//
// Parameters:  dirName -- name of the directory with no leading or ending slash
//              dirNameLen -- size of the name
//              lsn, size -- results!
//
// Returns:     
//------------------------------------------------------------------------------

radFileError radPs2CdDrive::FindDirectorySector
( 
    const char*     dirName, 
    unsigned int    dirNameLen, 
    unsigned int*   lsn,
    unsigned int*   size
)
{
    //
    // Get the volume descriptor
    //
    unsigned char* pVolDesc = m_SectorBuffer;
    radFileError result = Read( VOL_DESCRIPTOR_ADDR, pVolDesc, 1, false );
    if ( result != Success )
    {
        return result;
    }

    //
    // Get the root directory record address
    //
    unsigned char* pDirInfo = &pVolDesc[ VOL_DESCRIPTOR_RECORD_LOC ];
    unsigned int extent = littleEndianToUInt( &pDirInfo[ DIR_RECORD_EXTENT_ADDR_LOC ] );
    unsigned extentLen = littleEndianToUInt( &pDirInfo[ DIR_RECORD_EXTENT_LEN_LOC ] );

    //
    // Keep processing while there are more sub-directories.
    // Assume extent and extentLen are the current directory.
    //
    unsigned char* pSectorEnd = m_SectorBuffer + m_MediaInfo.m_SectorSize - 1;
    const char* dirNameEnd = &dirName[ dirNameLen ];
    const char* curr = dirName;
    
    while ( curr < dirNameEnd )
    {
        //
        // Find the end of the directory name
        //
        const char* end = curr;
        while ( end < dirNameEnd && *end != '\\' )
        {
            end++;
        }
        unsigned int len = end - curr;
        
        //
        // Read through the directory records until we find the name
        //
        unsigned int numSectors = ( extentLen + m_MediaInfo.m_SectorSize - 1 ) / m_MediaInfo.m_SectorSize;

        bool found = false;
        while ( !found )
        {
            //
            // Read a sector if there are any left
            //
            if ( numSectors == 0 )
            {
                return FileNotFound;
            }

            result = Read( extent, m_SectorBuffer, 1, false );
            if ( result != Success )
            {
                return result;
            }

            extent++;
            numSectors--;
            pDirInfo = m_SectorBuffer;

            //
            // Parse through the sector
            //
            while ( pDirInfo < pSectorEnd && pDirInfo[ DIR_RECORD_LEN_LOC ] != 0 )
            {
                //
                // Read the next record and check if it's right
                //
                if ( pDirInfo[ DIR_RECORD_FLAGS_LOC ] & FILE_FLAG_DIR )
                {
                    char pName[ MAX_PS2FILENAMELEN ];
                    unsigned int nameLen = pDirInfo[ DIR_RECORD_IDENT_LEN_LOC ];
                    strncpy( pName, (char*) &pDirInfo[ DIR_RECORD_IDENT_LOC ], nameLen );
                    pName[ nameLen ] = '\0';
                    strupr( pName );

                    if ( strncmp( pName, curr, len ) == 0 )
                    {
                        found = true;
                        break;
                    }
                }

                //
                // Go to the next record
                //
                pDirInfo += pDirInfo[ DIR_RECORD_LEN_LOC ];
            }
        }

        //
        // Found the directory. Set up variables to find the next subdirectory
        //
        extent = littleEndianToUInt( &pDirInfo[ DIR_RECORD_EXTENT_ADDR_LOC ] );
        extentLen = littleEndianToUInt( &pDirInfo[ DIR_RECORD_EXTENT_LEN_LOC ] );
        curr = end + 1; // skip the slash
    }

    //
    // We have found our subdirectory, so return it.
    //
    *lsn = extent;
    *size = extentLen;
    return Success;
}

//=============================================================================
// Function:    radPs2CdDrive::BuildFileSpec
//=============================================================================
// Description: Make the filename in format CDROM:filename (no slash).
//
// Parameters:  fileName -- the name given by the user
//              fullName -- buffer to give back the name
//              size -- size of fullName buffer
//              pName -- pointer to the beginning of the filename (after the colon)
//
// Returns:     
//------------------------------------------------------------------------------

void radPs2CdDrive::BuildFileSpec( const char* fileName, char* fullName, unsigned int size, char** pName )
{
    int len = strlen( s_PS2CDDriveName );
    strncpy( fullName, s_PS2CDDriveName, size - 1 );

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

//=============================================================================
// Function:    radPs2CdDrive::Read
//=============================================================================

radFileError radPs2CdDrive::Read
( 
    unsigned int    lsn,
    void*           buffer,
    unsigned int    sectors,
    bool            IntoIOP
)
{
    static unsigned int lastLsn = 0;
    //
    // Set up the mode. Try forever, normal read speed, sector size 2048
    //
    
    sceCdRMode mode __attribute__ ((aligned(16)));

    radFileError FileResult;
#ifdef RADFILE_SPEW
    rReleasePrintf(
        " - Dist: [0x%x], Bytes: [0x%x], t:[0x%x]\n",
        ( lastLsn > lsn ) ? ( lastLsn - lsn ) : ( lsn - lastLsn ),
        sectors * 2048,
         radTimeGetMilliseconds( ) );
#endif
        
    lastLsn = lsn + sectors;
        
    //
    // Do some of our own retries as the libraries do not always work that reliably.
    //
    unsigned int retries = PS2CD_READ_RETRY;
    do
    {
        mode.trycount = 0;
        mode.spindlctrl = SCECdSpinStm/*SCECdSpinNom*/;
        mode.datapattern = SCECdSecS2048;

        //
        // Wait for device to become ready. Do not wait if shell open however.
        //
        while( ::sceCdDiskReady( 1 ) == SCECdNotReady )
        {
            if ( ::sceCdStatus( ) == SCECdStatShellOpen )
            {
                return ShellOpen;
            }
            radThreadSleep( 1 );
        }

        int result;

        if( IntoIOP )
        {
            result = ::sceCdReadIOPm( lsn, sectors, buffer, &mode );
        }
        else    
        {
            result = ::sceCdRead( lsn, sectors, buffer, &mode );
        }
    
        if( result == 1 )
        {
            //
            // Wait for the operation to complete.
            //
            ::sceCdSync( 0 );

            //
            // Check if successful.
            //
            int error = ::sceCdGetError( );
            switch( error )
            {
            case SCECdErFAIL :
                FileResult = HardwareFailure;
                break;

            case SCECdErNO :
                FileResult = Success;
                break;

            case SCECdErNODISC :
                FileResult = NoMedia;
                break;

            case SCECdErOPENS :
            case SCECdErTRMOPN :
                FileResult = ShellOpen;
                break;

            default:
                FileResult = HardwareFailure;
                break;
            }     
        }
        else
        {
            //
            // Check for tray open or a no CD.
            //
            if ( ::sceCdStatus( ) == SCECdStatShellOpen )
            {
                return ShellOpen;
            }

            switch( ::sceCdGetDiskType( ) )
            {
            case SCECdDETCT:
            case SCECdNODISC: 
                FileResult = NoMedia;
                break;

            case SCECdUNKNOWN:
                FileResult = MediaInvalid;
                break;

            case SCECdIllgalMedia:
            case SCECdCDDA:
                FileResult = MediaWrongType;
                break;

            case SCECdPS2DVD:
            case SCECdDVDV:
                if ( radPlatformGet( )->GetMediaType( ) == GameMediaDVD )
                {
                    FileResult = HardwareFailure; // why did it fail?
                }
                else
                {
                    FileResult = MediaWrongType;
                }
                break;
                break;

            case SCECdPS2CD:
            case SCECdPS2CDDA:
            case SCECdPSCD:
            case SCECdPSCDDA:
                if ( radPlatformGet( )->GetMediaType( ) == GameMediaCD )
                {
                    FileResult = HardwareFailure; // why did it fail?
                }
                else
                {
                    FileResult = MediaWrongType;
                }
                break;

            default:
                FileResult = HardwareFailure;
                break;
            }
        }

        retries--;

        //
        // Keep retrying if we get hardware failures, indicating something is going wrong.
        // Errors like no disk or shell opened are not retried as this level.
        //
    }
    while( (retries > 0) && (FileResult == HardwareFailure) );

    return FileResult;
}

//=============================================================================
// Function:    radPs2CdDrive::DiskChanged
//=============================================================================
// Description: Check if the disk changed. If it did, but no files are open, 
//              re-initializes the disk. Returns WrongMedia if needed.
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

radFileError radPs2CdDrive::DiskChanged( void )
{
    //
    // See if the door was opened.
    //
    unsigned int opened = 0;
    ::sceCdTrayReq( SCECdTrayCheck, &opened );
    if ( opened == 0 )
    {
        return Success;
    }

    //
    // The tray has been opened, so we need to check if the disk is okay!
    //
    char name[ sizeof( m_VolumeName ) ];
    radFileError result = ReadVolumeName( name );
    if ( result != Success )
    {
        return result;
    }

    //
    // Check if the current disk has changed.
    //
    if ( ::memcmp( m_VolumeName, name, VOL_DESCRIPTOR_IDENT_LEN ) != 0 )
    {
        //
        // Disk has changed! If there are no open files, just load up this disk.
        //
        if ( m_OpenFiles > 0 )
        {
            return WrongMedia;
        }
        else
        {
            radFileError tmp = m_LastError;
            Initialize( );
            m_LastError = tmp;
            return Success;
        }
    }
    else
    {
        return Success;
    }
}

//=============================================================================
// Function:    radPs2CdDrive:FileNameMatchesSearch
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

bool radPs2CdDrive::FileNameMatchesSearch
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
    char nameBuf[ RAD_PS2_SPEC_LEN + 1 ];
    char* nameAfterDot;
    char specBuf[ RAD_PS2_SPEC_LEN + 1 ];
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
