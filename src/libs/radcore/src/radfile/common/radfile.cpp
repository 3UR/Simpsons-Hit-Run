//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radfile.cpp
//
// Subsystem:   Radical File System
//
// Description:	This file contains the implementation of the global
//              radFile functions.
//
// Revisions:	
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <string.h>
#include <radfile.hpp>
#include <radobject.hpp>
#include <radstring.hpp>
#include "filesystem.hpp"

//=============================================================================
// Public Functions
//=============================================================================

//=============================================================================
// Function:    radFileInitialize
//=============================================================================
// Description: This routine is invoked to intialized the file system. 
//
// Parameters:  maximum number of outstanding requests.
//              maximum number of open files.
//              memory allocator
//
// Returns:     
//
// Notes:       Must frame all other requests.
//------------------------------------------------------------------------------

void radFileInitialize
( 
    unsigned int       maxOutstandingRequests,
    unsigned int       maxOpenFiles,
    radMemoryAllocator alloc 
)
{
    radFileSystem::Initialize( maxOutstandingRequests, maxOpenFiles, alloc );
}

//=============================================================================
// Function:    radFileTerminate
//=============================================================================
// Description: This routine is invoked to terminate the file system. 
//
// Parameters:  
//
// Returns:     
//
// Notes:       Must be the last function invoked.
//------------------------------------------------------------------------------

void radFileTerminate( void )
{
    radFileSystem::Terminate( );
}

//=============================================================================
// Function:    radFileService
//=============================================================================
// Description: This routine must be periodically invoked by the game to 
//              retrieve callbacks and service the debug comm.
//
// Parameters:  N/A
//
// Returns:     N/A
//
//------------------------------------------------------------------------------

void radFileService( void )
{
    radFileSystem::Service( );
}


//=============================================================================
// Function:    radFileSetRootDirectory
//=============================================================================
// Description: This routine can be used to perform root directory relocation
//              on a per-drive basis.
//
// Constraints: Any desired root directory relocation should be done before
//              any file or drive operations are started. Results are undefined
//              otherwise. 
//
//
// Parameters:  pDrive - the name of the drive to set the root path of, ie "C:"
//              pRootDir - the name of the directory to use as root.
//
//------------------------------------------------------------------------------

void radFileSetRootDirectory( const char* pDrive, const char *pRootDir )
{
    radFileSystem::SetRootDirectory( pDrive, pRootDir );
}

//=============================================================================
// Function:    radFileGetRootDirectory
//=============================================================================
// Description: This routine will obtain the name of the relocated root
//              directory of a given drive, if any. If the root of the
//              drive is not relocated, the return string will be empty.
//              
//              A non-empty return string will not have path separators
//              at the start or end, but may have them in the middle -
//              ie, "games\game01"
//
//              Generally the user should not need to query the identity
//              of the root directory. Once set, all file and drive
//              operations should be transparent.
//
// Parameters:  pDrive - the name of the drive to query ie, "C:", "CDROM:" etc.
//              pRootDir - buffer to store the directory name in. Must be
//                  long enough to hold a path.
//
//------------------------------------------------------------------------------
void radFileGetRootDirectory( const char* pDrive, char* pRootDir )
{
    radFileSystem::GetRootDirectory( pDrive, pRootDir );
}

//=============================================================================
// Function:    radFileOpenAsync
//=============================================================================
// Description: This routine will open a file asynchronously.
//
// Parameters:  pIRadFile - where to receive the file object.
//              pFileName - name of the file to open
//              a bunch of flags
//
// Returns:     N/A
//
//------------------------------------------------------------------------------

void radFileOpenAsync( IRadFile** pIRadFile, 
                  const char* pFileName,
                  bool writeAccess,
                  radFileOpenFlags flags,
                  radFilePriority priority, 
                  unsigned int cacheSize,
                  radMemoryAllocator alloc,
                  radMemorySpace cacheSpace )
{
    radFileSystem::FileOpen
    (
        pIRadFile,
        pFileName,
        writeAccess,
        flags,
        priority,
        cacheSize,
        alloc,
        cacheSpace
    );
}

//=============================================================================
// Function:    radFileOpenSync
//=============================================================================
// Description: This routine will open a file synchronously.
//
// Parameters:  pIRadFile - where to receive the file object.
//              pFileName - name of the file to open
//              a bunch of flags
//
// Returns:     N/A
//
//------------------------------------------------------------------------------

void radFileOpenSync( IRadFile** pIRadFile, 
                  const char* pFileName,
                  bool writeAccess,
                  radFileOpenFlags flags,
                  radFilePriority priority, 
                  unsigned int cacheSize,
                  radMemoryAllocator alloc,
                  radMemorySpace cacheSpace )
{
    radFileSystem::FileOpenSync
    (
        pIRadFile,
        pFileName,
        writeAccess,
        flags,
        priority,
        cacheSize,
        alloc,
        cacheSpace
    );
}

//=============================================================================
// Function:    radSaveGameOpenAsync
//=============================================================================
// Description: This routine will open a save game synchronously.
//
// Parameters:  pIRadFile - where to receive the file object.
//              pFileName - name of the file to open
//              a bunch of flags
//
// Returns:     N/A
//
//------------------------------------------------------------------------------

void radSaveGameOpenAsync( 
    IRadFile** pIRadFile, 
    const char* pFileName,
    bool writeAccess,
    radFileOpenFlags flags,
    radMemcardInfo* memcardInfo,
    unsigned int maxSize,
    radFilePriority priority
)
{
    radFileSystem::SaveGameOpen
    (
        pIRadFile, 
        pFileName,
        writeAccess,
        flags,
        memcardInfo,
        maxSize,
        priority
    );
}

//=============================================================================
// Function:    radSaveGameOpenSync
//=============================================================================
// Description: This routine will open a save game synchronously.
//
// Parameters:  pIRadFile - where to receive the file object.
//              pFileName - name of the file to open
//              a bunch of flags
//
// Returns:     N/A
//
//------------------------------------------------------------------------------

void radSaveGameOpenSync( 
    IRadFile** pIRadFile, 
    const char* pFileName,
    bool writeAccess,
    radFileOpenFlags flags,
    radMemcardInfo* memcardInfo,
    unsigned int maxSize,
    radFilePriority priority
)
{
    radFileSystem::SaveGameOpenSync
    (
        pIRadFile, 
        pFileName,
        writeAccess,
        flags,
        memcardInfo,
        maxSize,
        priority
    );
}

//=============================================================================
// Function:    radDriveOpenAsync
//=============================================================================
// Description: This routine will open a drive asynchronously.
//
// Parameters:  
//
// Returns:     N/A
//
//------------------------------------------------------------------------------

void radDriveOpenAsync
( 
    IRadDrive**     pIRadDrive,
    const char*     pDriveName,
    radFilePriority priority,
    radMemoryAllocator alloc
)
{
    radFileSystem::DriveOpen( pIRadDrive, pDriveName, priority, alloc );
}

//=============================================================================
// Function:    radDriveOpenSync
//=============================================================================
// Description: This routine will open a drive asynchronously.
//
// Parameters:  
//
// Returns:     N/A
//
//------------------------------------------------------------------------------

void radDriveOpenSync
( 
    IRadDrive**     pIRadDrive,
    const char*     pDriveName,
    radFilePriority priority,
    radMemoryAllocator alloc
)
{
    radFileSystem::DriveOpenSync( pIRadDrive, pDriveName, priority, alloc );
}

//=============================================================================
// Function:    radSetDefaultDrive
//=============================================================================
// Description: This routine will set the drive which is assumed if no drive is
//              specified.
//
// Parameters:  pDriveName - the default drive
//
// Returns:     N/A
//
//------------------------------------------------------------------------------

bool radSetDefaultDrive( const char* pDriveName )
{
    return radFileSystem::SetDefaultDrive( pDriveName );
}

//=============================================================================
// Function:    radSetDefaultDrive
//=============================================================================
// Description: This routine will return the default drive name.
//
// Parameters:  pDriveName - the default drive
//
// Returns:     N/A
//
//------------------------------------------------------------------------------

void radGetDefaultDrive( char* pDriveName )
{
    radFileSystem::GetDefaultDrive( pDriveName );
}

#ifdef CACHING_ENABLED
//=============================================================================
// Function:    radSetCacheFileNames
//=============================================================================
// Description: This routine will set the list of files that will be cached.
//
// Parameters:  pCacheFileNameArray - the array of file names to cache.  This
//                  array must always end with a pointer to NULL.
//
// Returns:     N/A
//
//------------------------------------------------------------------------------

void radSetCacheFileNames
(
    const char* pCacheFileNameArray[ ]
)
{
    radFileSystem::SetCacheFileNames( pCacheFileNameArray );
}

//=============================================================================
// Function:    radSetCacheDirectory
//=============================================================================
// Description: You can change the actual directory where files are cached.
//              Make sure that this is changed only when no files are
//              currently open.
//
// Parameters:  pCacheDirectory - the new cache directory
//
// Returns:     N/A
//
//=============================================================================

void radSetCacheDirectory
(
    const char* pCacheDirectory
)
{
    radFileSystem::SetCacheDirectory( pCacheDirectory );
}

#endif //CACHING_ENABLED

//=============================================================================
// Function:    radSetDefaultGranularity
//=============================================================================
// Description: Use this to set the default size of the various chunks
//              used during a write file operation.
//
// Parameters:  defaultGranularity - the new default granularity
//
// Returns:     N/A
//
//=============================================================================

void radSetDefaultGranularity( unsigned int defaultGranularity )
{
    radFileSystem::SetDefaultGranularity( defaultGranularity );
}

//=============================================================================
// Function:    radFileRegisterCementLibrary
//=============================================================================
// Description: Register a cement library asynchronously. 
//
// Parameters:  pIRadCementLibrary -
//                  (out) returns a pointer to the cement library.
//              cementLibraryFileName -
//                  the file anme of the data file for the cement library.
//              a bunch of flags
//
// Returns:     N/A
//
// Notes:       Registering a cement library is an asynchronous operation.
//              Use the pIRadCementLibrary object to determine
//              when the library will be available for use.
//
//------------------------------------------------------------------------------

void radFileRegisterCementLibraryAsync
(
    IRadCementLibrary** pIRadCementLibrary,
    const char* cementLibraryFileName,
    radCementLibraryPriority priority,
    unsigned int cacheSize,
    radMemoryAllocator alloc,
    radMemorySpace cacheSpace
)
{
    radFileSystem::RegisterCementLibrary
    (
        pIRadCementLibrary,
        cementLibraryFileName,
        priority,
        cacheSize,
        alloc,
        cacheSpace
    );
}

//=============================================================================
// Function:    radFileRegisterCementLibrary
//=============================================================================
// Description: Register a cement library asynchronously. 
//
// Parameters:  pIRadCementLibrary -
//                  (out) returns a pointer to the cement library.
//              cementLibraryFileName -
//                  the file anme of the data file for the cement library.
//              a bunch of flags
//
// Returns:     N/A
//
// Notes:       Registering a cement library is an asynchronous operation.
//              Use the pIRadCementLibrary object to determine
//              when the library will be available for use.
//
//------------------------------------------------------------------------------

void radFileRegisterCementLibrarySync
(
    IRadCementLibrary** pIRadCementLibrary,
    const char* cementLibraryFileName,
    radCementLibraryPriority priority,
    unsigned int cacheSize,
    radMemoryAllocator alloc,
    radMemorySpace cacheSpace
)
{
    radFileSystem::RegisterCementLibrarySync
    (
        pIRadCementLibrary,
        cementLibraryFileName,
        priority,
        cacheSize,
        alloc,
        cacheSpace
    );
}

//=============================================================================
// Function:    radFileSetConnectTimeOut
//=============================================================================
// Description: Sets remote drive connection timeout.
//
// Parameters:  time in milliseconds
//
// Returns:
//------------------------------------------------------------------------------

void radFileSetConnectTimeOut( unsigned int milliseconds )
{
    radFileSystem::SetConnectTimeOut( milliseconds );
}

bool radDriveMount( const char* pDriveSpec, radMemoryAllocator alloc )
{
    return radFileSystem::DriveMount( pDriveSpec, alloc );
}

bool radDriveUnmount( const char* pDriveSpec )
{
    return radFileSystem::DriveUnmount( pDriveSpec );
}


void radFileSetAutoMount( bool auto_mount )
{
    radFileSystem::SetAutoMount( auto_mount );
}