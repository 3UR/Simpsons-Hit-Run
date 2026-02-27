//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        ps2cddrive.hpp
//
// Subsystem:	Radical File System
//
// Description:	This file contains all definitions and classes relevant to
//              ps2 cd/dvd drive.
//
// Revisions:	
//
//=============================================================================

#ifndef	PS2CDDDRIVE_HPP
#define PS2CDDDRIVE_HPP

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
// Sector and file name sizes
// 
#define PS2_CD_SECTOR_SIZE     2048
#define PS2_DVD_SECTOR_SIZE    2048
#define PS2_MAX_SECTOR_SIZE    2048
#define MAX_PS2FILENAMELEN     76 

//
// Number of times to retry a read
//
#define PS2CD_READ_RETRY    128

//
// Maximum number of files on the disc
//
#define MAX_DIRECTORYFILES  1024

//
// Maximum directory records per sector (i.e. sector size / 34)
//
#define MAX_DIRS_PER_SECTOR 60

//
// Maximum number of subdirectories in a hierarchy
//
#define MAX_SUBDIRS         8

//
// Number of sectors in the transfer buffer.
//
#define PS2_CD_DRIVE_TRANSFER_BUFFER_SECTORS  32
#define PS2_CD_DRIVE_TRANSFER_BUFFER_SIZE (PS2_CD_DRIVE_TRANSFER_BUFFER_SECTORS * PS2_MAX_SECTOR_SIZE)

//
// Set up the alignment for our transfer buffer
//
#define PS2_CD_DRIVE_ALIGNMENT   128

//=============================================================================
// Statics
//=============================================================================

//
// The name of the drive.
//
static const char s_PS2CDDriveName[ ] = "CDROM:";

//=============================================================================
// Public Functions
//=============================================================================

//
// Every physical drive type must provide a drive factory.
//
void radPs2CdDriveFactory( radDrive** ppDrive, const char* driveSpec, radMemoryAllocator alloc );

//=============================================================================
// Class Declarations
//=============================================================================

class radPs2CdDrive : public radBufferedReader,
                      public radDrive
{
public:

    //
    // Constructor / destructor.
    //
    radPs2CdDrive( radMemoryAllocator alloc );
    virtual ~radPs2CdDrive( void );

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

    CompletionStatus FindFirst( const char*                 searchSpec, 
                                IRadDrive::DirectoryInfo*   pDirectoryInfo, 
                                radFileDirHandle*           pHandle,
                                bool                        firstSearch );

    CompletionStatus FindNext( radFileDirHandle* pHandle, IRadDrive::DirectoryInfo* pDirectoryInfo );

    CompletionStatus FindClose( radFileDirHandle* pHandle );

    //
    // radBufferedReader
    //
    IMPLEMENT_BUFFERED_READ;

private:
    bool SetMediaInfo( void );
    radFileError ReadVolumeName( char* nameBuffer );
    radFileError LoadDirectory( void );
    radFileError BuildDirectory( unsigned int dirAddress, unsigned int level, char* pName, unsigned int key );
    void BuildFileSpec( const char* fileName, char* fullName, unsigned int size, char** pName );
    radFileError FindDirectorySector( const char* dirName, unsigned int dirNameLen, unsigned int* lsn, unsigned int* size );
    bool FileNameMatchesSearch( const char* pFileName, const char* pSearchSpec );
    radFileError Read(unsigned int lsn, void* buffer, unsigned int sectors, bool IntoIOP );
    radFileError DiskChanged( void );

    unsigned int            m_OpenFiles;

    //
    // This is the volume name of the current disk. Even if the media info changes,
    // this is the disk that is accepted.
    //
    char                    m_VolumeName[ 64 + 1];


    //
    // This buffer is used to when the read request is not sector aligned. It needs
    // to have the alignment because we dma from this buffer to IOP sometimes and keeping
    // it aligned to 128 greatly improves transfer rates
    //
    unsigned char  m_SectorBufferSpace[ PS2_CD_DRIVE_TRANSFER_BUFFER_SIZE + PS2_CD_DRIVE_ALIGNMENT ];
    unsigned char* m_SectorBuffer;

    //
    // These data structures are used to maintain the directory information. We read
    // the entire directory tree and hash all of the filenames.
    //
    struct DirectoryEntry
    {
        unsigned int m_HashValue;           // Hashed filename
        unsigned int m_Lsn;                 // Logical sector number of start of file.
        unsigned int m_Size;                // Size of file.
        inline bool operator==( const DirectoryEntry& dirEntry ) const
        { 
            return m_HashValue == dirEntry.m_HashValue;
        }
        inline bool operator<( const DirectoryEntry& dirEntry ) const
        { 
            return m_HashValue < dirEntry.m_HashValue;
        }
        static int compare( const void* elem1, const void* elem2 )
        {
            return ((DirectoryEntry*) elem1)->m_HashValue < ((DirectoryEntry*) elem2)->m_HashValue ? -1 : 1;
        }
    };

    DirectoryEntry  m_Directory[ MAX_DIRECTORYFILES ];  // Table of directory entries
    unsigned int    m_NumDirEntries;    // Number of entries actually loaded.

    //
    // temporary data for traversal
    //
    struct TravInfo
    {
        unsigned int    m_address;
        unsigned int    m_key;
    };

    TravInfo    m_TmpTravInfo[ MAX_SUBDIRS ][ MAX_DIRS_PER_SECTOR ];

    //
    // Mutex for critical sections
    //
    IRadThreadMutex*    m_pMutex;
};

#endif // PS2CDDDRIVE_HPP

