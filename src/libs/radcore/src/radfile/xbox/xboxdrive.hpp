//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        xboxdrive.hpp
//
// Subsystem:	Radical File System
//
// Description:	This file contains all definitions and classes relevant to
//              xbox dvd and hard drive.
//
// Revisions:	
//
//=============================================================================

#ifndef	XBOXDRIVE_HPP
#define XBOXDRIVE_HPP

//=============================================================================
// Include Files
//=============================================================================

#include "../common/drive.hpp"
#include "../common/drivethread.hpp"

//=============================================================================
// Public Functions
//=============================================================================

//
// Every physical drive type must provide a drive factory.
//
void radXboxDriveFactory( radDrive** ppDrive, const char* driveSpec, radMemoryAllocator alloc );

//=============================================================================
// Class Declarations
//=============================================================================

class radXboxDrive : public radDrive
{
public:

    //
    // Constructor / destructor.
    //
    radXboxDrive( const char* pdrivespec, radMemoryAllocator alloc );
    virtual ~radXboxDrive( void );

    //
    // IRadMutable
    //
    void Lock( void );
    void Unlock( void );

    unsigned int GetCapabilities( void );

    const char* GetDriveName( void );
 
    CompletionStatus Initialize( void );

    CompletionStatus OpenFile( const char*        fileName, 
                               radFileOpenFlags   flags, 
                               bool               writeAccess, 
                               radFileHandle*     pHandle, 
                               unsigned int*      pSize );

    CompletionStatus CloseFile( radFileHandle handle, const char* fileName );

    CompletionStatus ReadFile( radFileHandle      handle,
                               const char*        fileName,
                               IRadFile::BufferedReadState buffState,
                               unsigned int       position, 
                               void*              pData, 
                               unsigned int       bytesToRead, 
                               unsigned int*      bytesRead, 
                               radMemorySpace     pDataSpace );

    CompletionStatus WriteFile( radFileHandle     handle, 
                                const char*       fileName,
                                IRadFile::BufferedReadState buffState,
                                unsigned int      position, 
                                const void*       pData, 
                                unsigned int      bytesToWrite, 
                                unsigned int*     bytesWritten, 
                                unsigned int*     size, 
                                radMemorySpace    pDataSpace );

    CompletionStatus Format( void );

    CompletionStatus FindFirst( const char*                 searchSpec, 
                                IRadDrive::DirectoryInfo*   pDirectoryInfo, 
                                radFileDirHandle*           pHandle,
                                bool                        firstSearch );

    CompletionStatus FindNext( radFileDirHandle* pHandle, IRadDrive::DirectoryInfo* pDirectoryInfo );

    //
    // This is needed to avoid clashing with an XBOX define.
    //
    CompletionStatus FindClose( radFileDirHandle* pHandle )
    {
        return FindCloseX( pHandle );
    }

    CompletionStatus FindCloseX( radFileDirHandle* pHandle );

    CompletionStatus CreateDir( const char* pName );

    CompletionStatus DestroyDir( const char* pName );   

    CompletionStatus DestroyFile( const char* filename );

private:
    //
    // Helper functions
    //
    void SetMediaInfo( void );
    void BuildFileSpec( const char* fileName, char* fullName, unsigned int size );
    radFileError TranslateError( unsigned int windowsError );
    radFileError TranslateDirInfo( IRadDrive::DirectoryInfo*   pDirectoryInfo, 
                           const void*                 pFindData,
                           const radFileDirHandle*     pHandle );

    unsigned int    m_Capabilities;
    unsigned int    m_OpenFiles;
    char            m_DriveName[ radFileDrivenameMax + 1 ];

    //
    // Mutex for critical sections
    //
    IRadThreadMutex*    m_pMutex;
};

#endif // XBOXDRIVE_HPP

