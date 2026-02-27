//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        filesystem.cpp
//
// Subsystem:   Radical File System
//
// Description:	This file contains the implementation of the radFileSystem
//              class. 
//
// Revisions:	
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <ctype.h>
#include <raddebug.hpp>
#include <string.h>
#include "filesystem.hpp"
#include "platformdrives.hpp"
#include "requests.hpp"
#include "instancedrive.hpp"
#include "file.hpp"

//=============================================================================
// Static Data Defintions
//=============================================================================

bool                radFileSystem::s_Initialized = false;
radMemoryAllocator  radFileSystem::s_Allocator = 0;
unsigned int        radFileSystem::s_RemoteDriveConnectTimeout = INITIAL_REMOTE_TIMEOUT;
unsigned int        radFileSystem::s_TotalDriveCount = 0;
char                radFileSystem::s_DefaultDrive[ radFileDrivenameMax + 1 ];
bool                radFileSystem::s_AutoMount = true;
IRadThreadMutex*    radFileSystem::s_pMutex = NULL;

radFileSystem::DriveMapEntry    radFileSystem::s_DriveMap[ radFileDriveMax ];

//=============================================================================
// Static Functions
//=============================================================================

//=============================================================================
// Function:    stringUpper
//=============================================================================
// Description: Convert a string to upper case (in place).
//
// Parameters:  char* string
//
// Returns:     
//------------------------------------------------------------------------------

static void stringUpper( char* str )
{
    while( *str != '\0' )
    {
        *str = (char) toupper( *str );
        str++;
    }
}

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    radFileSystem::Initialize
//=============================================================================
// Description: Initialize the global file system, including all pools.
//
// Parameters:  maxOutstandingRequests
//              maxOpenFiles
//              alloc
//
// Returns:     
//------------------------------------------------------------------------------

void radFileSystem::Initialize
(
    unsigned int maxOutstandingRequests, 
    unsigned int maxOpenFiles,
    radMemoryAllocator alloc 
)
{
    rAssertMsg( !s_Initialized, "radFileSystem already initialized" );

    //
    // Set default drive
    //
    ::PlatformDrivesGetDefaultDrive( s_DefaultDrive );

    //
    // Set up pools
    //    
    radRequestPoolInitialize( maxOutstandingRequests, alloc );
    radDrivePoolInitialize( maxOutstandingRequests, alloc );
    radFilePoolInitialize( maxOpenFiles, alloc );

    //
    // Set up our mutex
    //
    radThreadCreateMutex( &s_pMutex, alloc );
    rAssert( s_pMutex != NULL );

    //
    // Set up drive map
    //
    for ( unsigned int i = 0; i < radFileDriveMax; i++)
    {
        s_DriveMap[ i ].m_pDrive = NULL;
    }

    s_Allocator = alloc;
    s_Initialized = true;

    //
    // Initialize the CRC system
    //
    ::radCRCInit( );
}

//=============================================================================
// Function:    radFileSystem::Terminate
//=============================================================================
// Description: Terminate the global file system, including all pools. All drives
//              should be unmounted and all pools should have their members deleted.
//
// Parameters: 
//
// Returns:     
//------------------------------------------------------------------------------

void radFileSystem::Terminate( void )
{
    rAssertMsg( s_Initialized, "radFileSystem not initialized" );

    if ( !s_AutoMount )
    {
        for ( unsigned int i = 0; i < radFileDriveMax; i++)
        {
            rAssertMsg
            ( 
                s_DriveMap[ i ].m_pDrive == NULL, 
                "radFileSystem: not all drives have been unmounted." 
            );
        }
    }
    else
    {
        for ( unsigned int i = 0; i < radFileDriveMax; i++ )
        {
            if ( s_DriveMap[ i ].m_pDrive != NULL )
            {
                rDebugPrintf( "radFile: auto-unmounting drive [%s]\n", s_DriveMap[ i ].m_Name );
                DriveUnmount( s_DriveMap[ i ].m_Name );
            }
        }
    }

    radFilePoolTerminate( );
    radDrivePoolTerminate( );
    radRequestPoolTerminate( );

    radRelease( s_pMutex, NULL );
    s_pMutex = NULL;
    s_Initialized = false;
}

//=============================================================================
// Function:    radFileSystem::Lock
//=============================================================================
// Description: Begin a critical section.
//
// Parameters: 
//
// Returns:     
//------------------------------------------------------------------------------

void radFileSystem::Lock( )
{
    s_pMutex->Lock( );
}

//=============================================================================
// Function:    radFileSystem::Unlock
//=============================================================================
// Description: End a critical section.
//
// Parameters: 
//
// Returns:     
//------------------------------------------------------------------------------

void radFileSystem::Unlock( )
{
    s_pMutex->Unlock( );
}

//=============================================================================
// Function:    radFileSystem::Service
//=============================================================================
// Description: Service the file system. This will invoke any outstanding callbacks
//              for this thread.
//
// Parameters: 
//
// Returns:     
//------------------------------------------------------------------------------

void radFileSystem::Service( void )
{
    rAssertMsg( s_Initialized, "radFileSystem not initialized" );

    //
    // Run the Service( ) on every drive.
    //
    Lock( );
    for ( unsigned int i = 0; i < s_TotalDriveCount; i++ )
    {
        if ( s_DriveMap[ i ].m_pDrive != NULL )
        {
            //
            // Grab a reference of the drive, then unlock.
            //
            radDrive* pDrive = s_DriveMap[ i ].m_pDrive;
            pDrive->AddRef( );
            Unlock( );

            pDrive->Service( );
            pDrive->Release( );
            Lock( );
        }
    }
    Unlock( );
}

//=============================================================================
// Function:    ProcessFileName
//=============================================================================
// Description: This routine takes a file name, splits it into a drive and
//              a file, and reformats it to use the correct slashes and capitalizes
//              the drive spec. If no drive is specified, then the default drive is used.
//
// Parameters:  pFileName - the original file name
//              filename - (out) the modified file spec
//              driveSpec - (out) the drive spec
//
// Returns:     
//------------------------------------------------------------------------------

void radFileSystem::ProcessFileName
( 
    const char* pFileName, 
    char* fullFilename, 
    char* driveSpec,
    const char** pFilename,
	bool simpleName
)
{
    rAssertMsg( s_Initialized, "radFileSystem not initialized" );

    //
    // First find the drive. If it exists, then there's a `:' in the name.
    // If no drive is specified, put in the default.
    // length should then be the length of the drive name and fname should point
    // to the beginning of the filename
    // 
    const char* fname;
    unsigned int length = 0;
    const char* drive = strchr( pFileName, ':' );
    if( simpleName==false && drive != NULL )
    {
        length = (uintptr_t)drive - (uintptr_t)pFileName + 1;        strncpy( driveSpec, pFileName, length );
        driveSpec[ length ] = '\0';
        fname = &pFileName[ length ];
    }
    else
    {
        Lock( );
        strcpy( driveSpec, s_DefaultDrive );
        Unlock( );

        length = strlen( driveSpec );
        fname = pFileName;
    }

    stringUpper( driveSpec );

    //
    // Process the filename
    //
    strcpy( fullFilename, driveSpec );

    *pFilename = &fullFilename[ length ];
    char* p = &fullFilename[length];
    
    //
    // Copy filename over
    //
    strcpy( p, fname );

    //
    // Lets translate any / to the correct way \\.
    //
    while( *p && simpleName==false)
    {
        if( *p == '/' )
        {
            *p = '\\';
        }
        p++;
    }
}

//=============================================================================
// Function:    radFileSystem::SetRootDirectory
//=============================================================================
void radFileSystem::SetRootDirectory( const char* pDrive, const char *pRootDir )
{
    rAssertMsg( s_Initialized, "radFileSystem not initialized" );
}

//=============================================================================
// Function:    radFileSystem::GetRootDirectory
//=============================================================================
void radFileSystem::GetRootDirectory( const char* pDrive, char* pRootDir )
{
    rAssertMsg( s_Initialized, "radFileSystem not initialized" );
}

//=============================================================================
// Function:    radFileSystem::FileOpen
//=============================================================================
// Description: Open a file from a mounted drive asynchronously.
//
// Parameters:  lots
//
// Returns:     
//------------------------------------------------------------------------------

void radFileSystem::FileOpen
( 
    IRadFile**          pIRadFile, 
    const char*         pFileName,
    bool                writeAccess,
    radFileOpenFlags    flags,
    radFilePriority     priority, 
    unsigned int        cacheSize,
    radMemoryAllocator  alloc,
    radMemorySpace      cacheSpace 
)
{
    rAssertMsg( s_Initialized, "radFileSystem not initialized" );
	rAssert( pFileName != NULL );
    rAssert( pIRadFile != NULL );

    //
    // Process the filename to get the drive specification
    //
    char fullFilename[ radFileFilenameMax + 1 ];
    char driveSpec[ radFileDrivenameMax + 1 ];
    const char* filePart = NULL;
    ProcessFileName( pFileName, fullFilename, driveSpec, &filePart );

    Lock( );

    //
    // Lets try to get the drive object. If fails, cannot attempt to get the file.
    //
    int index = GetDriveIndex( driveSpec );

    if( index == -1 || s_DriveMap[ index ].m_pDrive == NULL )
    {
        //
        // Auto-mount if we're allowed.
        //
        bool mounted = false;
        if ( s_AutoMount )
        {
            rDebugPrintf( "radFile: auto-mounting drive [%s].\n", driveSpec );

            Unlock( );
            DriveMount( driveSpec, s_Allocator );
            Lock( );

            //
            // Since we were unlocked, do a paranoia check.
            //
            index = GetDriveIndex( driveSpec );
            mounted = ( index > -1 && s_DriveMap[ index ].m_pDrive != NULL );
        }

        if ( !mounted )
        {
            *pIRadFile = NULL;
            rDebugPrintf( "radFile: cannot open file [%s] since drive [%s] was not mounted.\n", 
                          pFileName, driveSpec );

            Unlock( );
            return;
        }
    }

    //
    // Lets new up the file object.
    //
    *pIRadFile =
        new radFile
        (
            s_DriveMap[ index ].m_pDrive,
            filePart,
            writeAccess,
            flags,
            priority,
            cacheSize,
            alloc,
            cacheSpace
        );

    Unlock( );
}

//=============================================================================
// Function:    radFileSystem::FileOpenSync
//=============================================================================
// Description: Open a file from a mounted drive synchronously.
//
// Parameters:  lots
//
// Returns:     
//------------------------------------------------------------------------------

void radFileSystem::FileOpenSync
( 
    IRadFile**          pIRadFile, 
    const char*         pFileName,
    bool                writeAccess,
    radFileOpenFlags    flags,
    radFilePriority     priority,
    unsigned int        cacheSize,
    radMemoryAllocator  alloc,
    radMemorySpace      cacheSpace
)
{
#ifndef FINAL
    rReleaseString( "\n\nFileOpenSync: WE NEED TO ELIMINATE SYNC CALLS!\n\n" );
#endif

    FileOpen( pIRadFile, pFileName, writeAccess, flags, priority, cacheSize, alloc, cacheSpace );
    if ( *pIRadFile != NULL )
    {
        (*pIRadFile)->WaitForCompletion( );
    }
}

//=============================================================================
// Function:    radFileSystem::SaveGameOpen
//=============================================================================
// Description: Open a save game from a mounted drive asynchronously.
//
// Parameters:  lots
//
// Returns:     
//------------------------------------------------------------------------------

void radFileSystem::SaveGameOpen
( 
    IRadFile**        pIRadFile, 
    const char*       pFileName,
    bool              writeAccess,
    radFileOpenFlags  flags,
    radMemcardInfo*   memcardInfo,
    unsigned int      maxSize,
    radFilePriority   priority
)
{
    rAssertMsg( s_Initialized, "radFileSystem not initialized" );
	rAssert( pFileName != NULL );
    rAssert( pIRadFile != NULL );

    //
    // Process the filename to get the drive specification
    //
    char fullFilename[ radFileFilenameMax + 1 ];
    char driveSpec[ radFileDrivenameMax + 1 ];
    const char* filePart = NULL;
    ProcessFileName( pFileName, fullFilename, driveSpec, &filePart );

    Lock( );

    //
    // Lets try to get the drive object. If fails, cannot attempt to get the file.
    //
    int index = GetDriveIndex( driveSpec );

    if( index == -1 || s_DriveMap[ index ].m_pDrive == NULL )
    {
        //
        // Auto-mount if we're allowed.
        //
        bool mounted = false;
        if ( s_AutoMount )
        {
            rDebugPrintf( "radFile: auto-mounting drive [%s].\n", driveSpec );

            Unlock( );
            DriveMount( driveSpec, s_Allocator );
            Lock( );

            //
            // Since we were unlocked, do a paranoia check.
            //
            index = GetDriveIndex( driveSpec );
            mounted = ( index > -1 && s_DriveMap[ index ].m_pDrive != NULL );
        }

        if ( !mounted )
        {
            *pIRadFile = NULL;
            rDebugPrintf( "radFile: cannot open save game [%s] since drive [%s] was not mounted.\n", 
                          pFileName, driveSpec );

            Unlock( );
            return;
        }
    }

    //
    // Lets new up the file object.
    //
    *pIRadFile =
        new radFile
        (
            s_DriveMap[ index ].m_pDrive,
            filePart,
            writeAccess,
            flags,
            memcardInfo,
            maxSize,
            priority
         );

    Unlock( );
}

//=============================================================================
// Function:    radFileSystem::SaveGameOpenSync
//=============================================================================
// Description: Open a save game from a mounted drive asynchronously.
//
// Parameters:  lots
//
// Returns:     
//------------------------------------------------------------------------------

void radFileSystem::SaveGameOpenSync
( 
    IRadFile**        pIRadFile, 
    const char*       pFileName,
    bool              writeAccess,
    radFileOpenFlags  flags,
    radMemcardInfo*   memcardInfo,
    unsigned int      maxSize,
    radFilePriority   priority
)
{
    rReleaseString( "\n\nSaveGameOpenSync: WE NEED TO ELIMINATE SYNC CALLS!\n\n" );

    SaveGameOpen( pIRadFile, pFileName, writeAccess, flags, memcardInfo, maxSize, priority );
    if ( *pIRadFile != NULL )
    {
        (*pIRadFile)->WaitForCompletion( );
    }
}

//=============================================================================
// Function:    radFileSystem::DriveOpen
//=============================================================================
// Description: Open a user drive from a mounted drive asynchronously.
//
// Parameters:  lots
//
// Returns:     
//------------------------------------------------------------------------------
void radFileSystem::DriveOpen
( 
    IRadDrive**         pIRadDrive,
    const char*         pDriveName,
    radFilePriority     priority,
    radMemoryAllocator  alloc
)
{
    rAssertMsg( s_Initialized, "radFileSystem not initialized" );
    rAssert( pIRadDrive != NULL );
    rAssert( pDriveName != NULL );

    //
    // Convert the drive name to upper case.
    //
    char pDriveSpec[ radFileDrivenameMax + 1 ];
    strncpy( pDriveSpec, pDriveName, radFileDrivenameMax );
    pDriveSpec[ radFileDrivenameMax ] = '\0';
    stringUpper( pDriveSpec );

    Lock( );

    int index = GetDriveIndex( pDriveSpec );

    if( index == -1 || s_DriveMap[ index ].m_pDrive == NULL )
    {
        //
        // Auto-mount if we're allowed.
        //
        bool mounted = false;
        if ( s_AutoMount )
        {
            rDebugPrintf( "radFile: auto-mounting drive [%s].\n", pDriveSpec );

            Unlock( );
            DriveMount( pDriveSpec, s_Allocator );
            Lock( );

            //
            // Since we were unlocked, do a paranoia check.
            //
            index = GetDriveIndex( pDriveSpec );
            mounted = ( index > -1 && s_DriveMap[ index ].m_pDrive != NULL );
        }

        if ( !mounted )
        {
            *pIRadDrive = NULL;
            rDebugPrintf( "radFile: cannot open drive [%s] since it was not mounted.\n", pDriveName );

            Unlock( );
            return;
        }
    }

    //
    // Make an instance drive for the user
    //
    *pIRadDrive = new radInstanceDrive( s_DriveMap[ index ].m_pDrive, priority );

    Unlock( );
}

//=============================================================================
// Function:    radFileSystem::DriveOpenSync
//=============================================================================
// Description: Open a user drive from a mounted drive synchronously.
//
// Parameters:  lots
//
// Returns:     
//------------------------------------------------------------------------------

void radFileSystem::DriveOpenSync
(
    IRadDrive**         pIRadDrive,
    const char*         pDriveName,
    radFilePriority     priority,
    radMemoryAllocator  alloc
)
{
    rReleaseString( "\n\nDriveOpenSync: WE NEED TO ELIMINATE SYNC CALLS!\n\n" );

    DriveOpen( pIRadDrive, pDriveName, priority, alloc );
    if ( *pIRadDrive != NULL )
    {
        (*pIRadDrive)->WaitForCompletion( );
    }
}

//=============================================================================
// Function:    radFileSystem::SetDefaultDrive
//=============================================================================
// Description: Sets the drive to be used if none is specified.
//
// Parameters:  pDriveName
//
// Returns:     true if the drive is valid
//------------------------------------------------------------------------------

bool radFileSystem::SetDefaultDrive( const char* pDriveName )
{
    rAssertMsg( s_Initialized, "radFileSystem not initialized" );

    char pDriveSpec[ radFileDrivenameMax + 1 ];
    strncpy( pDriveSpec, pDriveName, radFileDrivenameMax );
    pDriveSpec[ radFileDrivenameMax ] = '\0';
    stringUpper( pDriveSpec );

    if ( ::PlatformDrivesValidateDriveName( pDriveSpec ) )
    {
        Lock( );
        strncpy( s_DefaultDrive, pDriveSpec, radFileDrivenameMax );
        s_DefaultDrive[ radFileDrivenameMax ] = '\0';
        Unlock( );
        return true;
    }
    else
    {
        return false;
    }
}

//=============================================================================
// Function:    radFileSystem::GetDefaultDrive
//=============================================================================
// Description: Gets the drive to be used if none is specified.
//
// Parameters:  [out] pDriveName
//
// Returns:     
//------------------------------------------------------------------------------

void radFileSystem::GetDefaultDrive( char* pDriveName )
{
    rAssertMsg( s_Initialized, "radFileSystem not initialized" );

    Lock( );
    strcpy( pDriveName, s_DefaultDrive );
    Unlock( );
}

//=============================================================================
// Function:    radFileSystem::RegisterCementLibrary
//=============================================================================

void radFileSystem::RegisterCementLibrary
(
    IRadCementLibrary** pIRadCementLibrary,
    const char* cementLibraryFileName,
    radCementLibraryPriority priority,
    unsigned int cacheSize,
    radMemoryAllocator alloc,
    radMemorySpace cacheSpace
)
{
    rAssertMsg( s_Initialized, "radFileSystem not initialized" );
    rAssert( pIRadCementLibrary != NULL );

    //
    // Process the filename to get the drive specification
    //
    char fullFilename[ radFileFilenameMax + 1 ];
    char driveSpec[ radFileDrivenameMax + 1 ];
    const char* filePart = NULL;
    ProcessFileName( cementLibraryFileName, fullFilename, driveSpec, &filePart );

    Lock( );

    //
    // Lets try to get the drive object. If fails, cannot attempt to get the file.
    //
    int index = GetDriveIndex( driveSpec );

    if( index == -1 || s_DriveMap[ index ].m_pDrive == NULL )
    {
        //
        // Auto-mount if we're allowed.
        //
        bool mounted = false;
        if ( s_AutoMount )
        {
            rDebugPrintf( "radFile: auto-mounting drive [%s].\n", driveSpec );

            Unlock( );
            DriveMount( driveSpec, s_Allocator );
            Lock( );

            //
            // Since we were unlocked, do a paranoia check.
            //
            index = GetDriveIndex( driveSpec );
            mounted = ( index > -1 && s_DriveMap[ index ].m_pDrive != NULL );
        }

        if ( !mounted )
        {
            *pIRadCementLibrary = NULL;
            rDebugPrintf( "radFile: cannot register cement library [%s] since drive [%s] was not mounted.\n", 
                          cementLibraryFileName, driveSpec );

            Unlock( );
            return;
        }
    }

    //
    // Lets new up the cement library.
    //
    radCementLibraryCreate( 
        pIRadCementLibrary, 
        s_DriveMap[ index ].m_pDrive, 
        filePart, 
        priority,
        cacheSize, 
        alloc, 
        cacheSpace );

    Unlock( );
}

//=============================================================================
// Function:    radFileSystem::RegisterCementLibrarySync
//=============================================================================
void radFileSystem::RegisterCementLibrarySync
(
    IRadCementLibrary** pIRadCementLibrary,
    const char* fileName,
    radCementLibraryPriority priority,
    unsigned int cacheSize,
    radMemoryAllocator alloc,
    radMemorySpace cacheSpace
)
{
//    rReleaseAssertMsg( false, "\n\nRegisterCementLibrarySync: WE NEED TO ELIMINATE SYNC CALLS!\n\n" );

    RegisterCementLibrary( pIRadCementLibrary, fileName, priority, cacheSize, alloc, cacheSpace );
    if ( *pIRadCementLibrary != NULL )
    {
        (*pIRadCementLibrary)->WaitForCompletion( );
    }
}

//=============================================================================
// Function:    radFileSystem::SetConnectTimeOut
//=============================================================================
void radFileSystem::SetConnectTimeOut( unsigned int milliseconds )
{
    rAssertMsg( s_Initialized, "radFileSystem not initialized" );

    s_RemoteDriveConnectTimeout = milliseconds;
}

//=============================================================================
// Function:    radFileSystem::SetConnectTimeOut
//=============================================================================
unsigned int radFileSystem::GetConnectTimeOut( void )
{
    return s_RemoteDriveConnectTimeout;
}

//=============================================================================
// Function:    radFileSystem::SetCacheFileNames
//=============================================================================
void radFileSystem::SetCacheFileNames( const char* pCacheFileNameArray[ ] )
{
    rAssertMsg( s_Initialized, "radFileSystem not initialized" );
}

//=============================================================================
// Function:    radFileSystem::SetCacheDirectory
//=============================================================================
void radFileSystem::SetCacheDirectory( const char* pCacheDirectory )
{
    rAssertMsg( s_Initialized, "radFileSystem not initialized" );
}

//=============================================================================
// Function:    radFileSystem::SetDefaultGranularity
//=============================================================================
void radFileSystem::SetDefaultGranularity( unsigned int defaultGranularity )
{
    rAssertMsg( s_Initialized, "radFileSystem not initialized" );
}

//=============================================================================
// Function:    radFileSystem::DriveMount
//=============================================================================
// Description: Mounts a physical drive into the filesystem. This must be done
//              off the thread which initialized the file system.
//
// Parameters:  pDriveSpec
//              alloc
//
// Returns:     true if the drive was mounted.
//------------------------------------------------------------------------------

bool radFileSystem::DriveMount( const char* pDriveSpec, radMemoryAllocator alloc )
{
    rAssertMsg( s_Initialized, "radFileSystem not initialized" );
    
    radDrive* pDrive;
    bool ret;

    Lock( );

    char driveSpec[ radFileDrivenameMax + 1];
    if ( pDriveSpec == NULL )
    {
        strcpy( driveSpec, s_DefaultDrive );
    }
    else
    {
        strncpy( driveSpec, pDriveSpec, radFileDrivenameMax );
        driveSpec[ radFileDrivenameMax ] ='\0';
        stringUpper( driveSpec );
    }

    ret = ConstructDrive( &pDrive, driveSpec, alloc );
    Unlock( );

#ifdef RAD_DEBUG
    if ( ret == false )
    {
        rDebugPrintf( "radFile: cannot mount drive [%s] since it is already mounted.\n", &driveSpec );
    }
#endif

    return ( pDrive == NULL );
}

//=============================================================================
// Function:    radFileSystem::DriveUnmount
//=============================================================================
// Description: Unmounts a physical drive into the filesystem. This must be done
//              off the thread which initialized the file system.
//
// Parameters:  pDriveSpec
//
// Returns:     true if the drive was mounted.
//
// Notes:       this will wait for its requests to finish.
//------------------------------------------------------------------------------

bool radFileSystem::DriveUnmount( const char* pDriveSpec )
{
    //
    // If this assert is removed, put in a Lock( )
    //
    rAssertMsg( s_Initialized, "radFileSystem not initialized" );

    //
    // Get the name and index.
    //
    int index;
    char driveSpec[ radFileDrivenameMax + 1 ];

    Lock( );

    if ( pDriveSpec == NULL )
    {
        strcpy( driveSpec, s_DefaultDrive );
    }
    else
    {
        strncpy( driveSpec, pDriveSpec, radFileDrivenameMax );
        driveSpec[ radFileDrivenameMax ] ='\0';
        stringUpper( driveSpec );
    }
    
    index = GetDriveIndex( driveSpec );

    //
    // Unmount the drive.
    //
    if ( index == -1 )
    {
        Unlock( );
        return false;
    }
    else
    {
        //
        // First assert on outstanding callbacks. Then assert on files or drives 
        // referencing this physical drive.
        //
#   ifdef RAD_DEBUG
        bool outstandingCallbacks = s_DriveMap[ index ].m_pDrive->OutstandingCallbacks( );
        if ( outstandingCallbacks )
        {
            rDebugPrintf( "radFile: cannot unmount drive [%s] since there are oustanding callbacks\n", &driveSpec );
        }
        rAssert( !outstandingCallbacks );

        if ( s_DriveMap[ index ].m_pDrive->m_pNumRefFiles > 0 )
        {
            rDebugPrintf( "radFile: cannot unmount drive [%s] since there are [%u] open files\n", &driveSpec, s_DriveMap[ index ].m_pDrive->m_pNumRefFiles );
        }
        else if ( s_DriveMap[ index ].m_pDrive->m_pNumRefDrives > 0 )
        {
            rDebugPrintf( "radFile: cannot unmount drive [%s] since there are [%u] open drives\n", &driveSpec, s_DriveMap[ index ].m_pDrive->m_pNumRefDrives );
        }

        rAssert( s_DriveMap[ index ].m_pDrive->m_pNumRefFiles == 0 && s_DriveMap[ index ].m_pDrive->m_pNumRefDrives == 0 );
#   endif

        //
        // Grab a copy of the drive and take it off the list, then wait for it to finish.
        // Since we are basically just removing it from the list, we don't bother releasing
        // it until everything's done.
        //
        radDrive* pDrive = s_DriveMap[ index ].m_pDrive;
        s_DriveMap[ index ].m_pDrive = NULL;
        Unlock( );

        //
        // Wait for all outstanding requests to finish.
        //
        while( pDrive->OutstandingRequests( ) )
        {
            ::radFileService( );
            ::radThreadSleep( 0 );
        }

        pDrive->Release( );
        return true;
    }
}

//=============================================================================
// Function:    radFileSystem::SetAutoMount
//=============================================================================
// Description: Turn on/off auto-mounting
//
// Parameters:  
//
// Returns:    
//------------------------------------------------------------------------------

void radFileSystem::SetAutoMount( bool autoMount )
{
    Lock( );
    s_AutoMount = autoMount;
    Unlock( );
}

//=============================================================================
// Private Member Functions
//=============================================================================

//=============================================================================
// Function:    radFileSystem::GetDriveIndex
//=============================================================================
// Description: This routine gets the index of the drive from the list of mounted
//              drives.
//
// Parameters:  pDriveName 
//
// Returns:     index or -1 if it didn't exist
//
// Notes:       Not locked!
//------------------------------------------------------------------------------

int radFileSystem::GetDriveIndex( const char* pDriveName )
{
    for( unsigned int i = 0; i < s_TotalDriveCount; i++ )
    {
        if( strcmp( pDriveName, s_DriveMap[ i ].m_Name) == 0 )
        {
            //
            // Found a match!
            //
            return i;
        }
    }
    return -1;
}

//=============================================================================
// Function:    radFileSystem::ConstructDrive
//=============================================================================
// Description: This routine is invoked to find and contruct the physical drive
//              object if not already done so. Does not addRef( ) further.
//
// Parameters:  pDrive - drive object
//              pDriveName - name to look for.
//              allocator 
//
// Returns:     true if it needed to be built
//
// Notes:       Not locked!
//------------------------------------------------------------------------------

bool radFileSystem::ConstructDrive
( 
    radDrive**  ppDrive, 
    const char* pDriveName, 
    radMemoryAllocator alloc 
)
{
    //
    // Here's how the drive table works. When a drive is constructed, its name is
    // stored, as well as a reference to the drive. When it is removed, the pointer
    // is removed, but the drive name and any global info remain.
    //
    int index = GetDriveIndex( pDriveName );

    *ppDrive = NULL;
    DriveMapEntry* pDriveEntry;

    //
    // Return the drive if it exists, otherwise set up an entry to be constructed
    //
    if ( index != -1 )
    {
        pDriveEntry = &s_DriveMap[ index ];
        if( pDriveEntry->m_pDrive != NULL )
        {
            *ppDrive = pDriveEntry->m_pDrive;
            return false;
        }
    }
    else
    {
        //
        // Check if there's space
        //
        if( s_TotalDriveCount >= radFileDriveMax )
        {
            rDebugPrintf( "radFileSystem: maximum number of drives [%d] exceeded.\n", radFileDriveMax );
            *ppDrive = NULL;
            return true;
        }

        pDriveEntry = &s_DriveMap[ s_TotalDriveCount ];
    }

    //
    // Construct a new drive after validating the name.
    //
    if ( !::PlatformDrivesValidateDriveName( pDriveName ) )
    {
        rDebugPrintf( "radFileSystem: [%s] is an invalid drive.\n", pDriveName );
        return true;
    }
    ::PlatformDrivesFactory( ppDrive, pDriveName, alloc );
    if( *ppDrive == NULL )
    {
        rDebugPrintf( "radFileSystem: Could not open drive [%s]\n", pDriveName );
        return true;
    }

    //
    // Set up the drive. If it's new set it up, otherwise set its global info
    //
    if( index == -1 )
    {
        strncpy( pDriveEntry->m_Name, pDriveName, radFileDrivenameMax );
        pDriveEntry->m_Name[ radFileDrivenameMax ] = '\0';
        s_TotalDriveCount++;
    }

    pDriveEntry->m_pDrive = *ppDrive;

    return true;
}

