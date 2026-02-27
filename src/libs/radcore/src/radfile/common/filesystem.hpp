//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        filesystem.hpp
//
// Subsystem:	Radical File System
//
// Description:	This file contains all definitions needed to implement the
//              global filesystem. It is responsible for all global state.
//
// Revisions:	
//
//=============================================================================

#ifndef	FILESYSTEM_HPP
#define FILESYSTEM_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <raddebug.hpp>
#include <radfile.hpp>

//=============================================================================
// Forward Class/Struct Declarations
//=============================================================================

class CompletionCallbackRequest;
class radDrive;

//=============================================================================
// Defines and Macros
//=============================================================================

#define INITIAL_REMOTE_TIMEOUT 5000

//=============================================================================
// Class Declarations
//=============================================================================

//
// This class is the file system. This is a static singleton object.
//
class radFileSystem
{
public:

    //
    // Functions
    //

    //
    // Get the canonnical name of the file. Does not check anything for
    // existence, just parses.
    //
    static void ProcessFileName( const char* pFileName, 
                              char* fullFilename, 
                              char* driveSpec,
                              const char** pFilename,
							  bool simpleName = false
                              );
    
    static void Initialize( unsigned int maxOutstandingRequests, 
                            unsigned int maxOpenFiles,
                            radMemoryAllocator alloc );

    static void Terminate( void );

    static void Service( void );

    static void SetRootDirectory( const char* pDrive, const char *pRootDir );

    static void GetRootDirectory( const char* pDrive, char* pRootDir );

    static void FileOpen( IRadFile** pIRadFile, 
                      const char* pFileName,
                      bool writeAccess,
                      radFileOpenFlags flags,
                      radFilePriority priority,
                      unsigned int cacheSize,
                      radMemoryAllocator alloc,
                      radMemorySpace cacheSpace );

    static void FileOpenSync( IRadFile** pIRadFile, 
                      const char* pFileName,
                      bool writeAccess,
                      radFileOpenFlags flags,
                      radFilePriority priority,
                      unsigned int cacheSize,
                      radMemoryAllocator alloc,
                      radMemorySpace cacheSpace );

    static void SaveGameOpen( 
                    IRadFile**        pIRadFile, 
                    const char*       pFileName,
                    bool              writeAccess,
                    radFileOpenFlags  flags,
                    radMemcardInfo*   memcardInfo,
                    unsigned int      maxSize,
                    radFilePriority   priority );

    static void SaveGameOpenSync( 
                    IRadFile**        pIRadFile, 
                    const char*       pFileName,
                    bool              writeAccess,
                    radFileOpenFlags  flags,
                    radMemcardInfo*   memcardInfo,
                    unsigned int      maxSize,
                    radFilePriority   priority );

    static void DriveOpen( IRadDrive** pIRadDrive,
                       const char* pDriveName,
                       radFilePriority priority,
                       radMemoryAllocator alloc );

    static void DriveOpenSync( IRadDrive** pIRadDrive,
                       const char* pDriveName,
                       radFilePriority priority, 
                       radMemoryAllocator alloc );

    static bool SetDefaultDrive( const char* pDriveName );

    static void GetDefaultDrive( char* pDriveName );

    static void RegisterCementLibrary
    (
        IRadCementLibrary** pIRadCementLibrary,
        const char* cementLibraryFileName,
        radCementLibraryPriority priority,
        unsigned int cacheSize,
        radMemoryAllocator alloc,
        radMemorySpace cacheSpace
    );

    static void RegisterCementLibrarySync
    (
        IRadCementLibrary** pIRadCementLibrary,
        const char* cementLibraryFileName,
        radCementLibraryPriority priority,
        unsigned int cacheSize,
        radMemoryAllocator alloc,
        radMemorySpace cacheSpace
    );

    static void SetConnectTimeOut( unsigned int milliseconds );
    static unsigned int GetConnectTimeOut( void );

    static void SetCacheFileNames( const char* pCacheFileNameArray[ ] );

    static void SetCacheDirectory( const char* pCacheDirectory );

    static void SetDefaultGranularity( unsigned int defaultGranularity );

    static bool DriveMount( const char* pDriveSpec, radMemoryAllocator alloc );
    static bool DriveUnmount( const char* pDriveSpec );

    static void SetAutoMount( bool autoMount );

private:

    static int GetDriveIndex( const char* pDriveName );

    static bool ConstructDrive( radDrive** pDrive, const char* pDriveName, radMemoryAllocator alloc );
    
    //
    // Used to indicate that the file system has been initialized.
    //
    static bool             s_Initialized;

    //
    // Keep the allocator around for auto-mounting drives.
    //
    static radMemoryAllocator s_Allocator;

    //
    // Remote drive timeout
    //
    static unsigned int     s_RemoteDriveConnectTimeout;

    //
    // Maintains a count total drive objects ever made.
    //
    static unsigned int     s_TotalDriveCount;

    //
    // Map of the physical drives.
    //
    struct DriveMapEntry
    {
        char            m_Name[ radFileDrivenameMax + 1 ];
        radDrive*       m_pDrive;
//        char            m_Root[ radFileFilenameMax + 1 ];
    };
    static DriveMapEntry    s_DriveMap[ radFileDriveMax ];  

    //
    // Check if are allowed to auto mount drives.
    //
    static bool             s_AutoMount;

    //
    // Default drive
    //
    static char             s_DefaultDrive[ radFileDrivenameMax + 1 ];

    //
    // Default granularity
    //
    static unsigned int     s_DefaultGranularity;

    //
    // Exclusion object
    //
    static void Lock( void );
    static void Unlock( void );

    static IRadThreadMutex* s_pMutex;
};

#endif //FILE
