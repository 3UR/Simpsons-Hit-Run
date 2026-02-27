//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        xboxsavedrive.hpp
//
// Subsystem:	Radical File System
//
// Description:	This file contains all definitions and classes relevant to
//              xbox memory cards and user hard drive.
//
// Revisions:	
//
//=============================================================================

#ifndef	XBOXSAVEDRIVE_HPP
#define XBOXSAVEDRIVE_HPP

//=============================================================================
// Include Files
//=============================================================================

#include "../common/drive.hpp"
#include "../common/drivethread.hpp"
#include "../common/signeddrive.hpp"

//=============================================================================
// Defines
//=============================================================================

static const char s_XBOXMemcardDrive[ ] = "MEMCARD??:";
#define XBOXMEMCARDDRIVE_PORT_LOC    7
#define XBOXMEMCARDDRIVE_SLOT_LOC    8

#define XBOXSAVEDRIVE_SECTOR_SIZE     4096
#define XBOXSAVEDRIVE_TRANSFER_BUFFER_SECTORS  2
#define XBOXSAVEDRIVE_TRANSFER_BUFFER_SIZE (XBOXSAVEDRIVE_TRANSFER_BUFFER_SECTORS * XBOXSAVEDRIVE_SECTOR_SIZE)

//
// Set up the alignment for our transfer buffer -- not really needed but can't hurt
//
#define XBOXSAVEDRIVE_ALIGNMENT   32

//
// Number of directory search handles per drive.
//
#define XBOX_MAX_CARD_HANDLES     4

//=============================================================================
// Public Functions
//=============================================================================

//
// Every physical drive type must provide a drive factory.
//
void radXboxSaveDriveFactory( radDrive** ppDrive, const char* driveSpec, radMemoryAllocator alloc );

//=============================================================================
// Class Declarations
//=============================================================================

class radXboxSaveDrive : public radDrive,
                         public radSignedReader,
                         public radSignedWriter
{
public:

    //
    // Constructor / destructor.
    //
    radXboxSaveDrive( const char* pdrivespec, radMemoryAllocator alloc );
    virtual ~radXboxSaveDrive( void );

    void Lock( void );
    void Unlock( void );

    //
    // This member reports this physical drives capabilities
    //
    unsigned int GetCapabilities( void );

    const char* GetDriveName( void );

    unsigned int GetCreationSize( radMemcardInfo* memcardInfo, unsigned int size );
 
    CompletionStatus Initialize( void );

    CompletionStatus OpenSaveGame( const char*      fileName, 
                                   radFileOpenFlags flags, 
                                   bool             writeAccess,
                                   radMemcardInfo*  memcardInfo,
                                   unsigned int     maxSize,
                                   radFileHandle*   pHandle, 
                                   unsigned int*    pSize );

    CompletionStatus CloseFile( radFileHandle   handle, const char* fileName );

    CompletionStatus CommitFile( radFileHandle  handle, const char* fileName );

    CompletionStatus ReadSignedBlock( radFileHandle  handle,
                                      const char*    fileName,
                                      unsigned int   block,
                                      unsigned int   position,
                                      unsigned int   numBytes,
                                      void*          pData, 
                                      radMemorySpace pDataSpace );

    unsigned int GetReadBlockSize( void );
    unsigned int GetReadHeaderSize( radFileHandle handle );

    CompletionStatus WriteSignedBlock( radFileHandle handle,
                                       const char*  fileName,
                                       unsigned int block,
                                       unsigned int position,
                                       unsigned int numBytes,
                                       const void* pData,
                                       unsigned int* size,
                                       radMemorySpace pDataSpace );

    unsigned int GetWriteBlockSize( void );
    unsigned int GetWriteHeaderSize( radFileHandle handle );

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

    CompletionStatus DestroyFile( const char* filename );

    IMPLEMENT_SIGNED_READ;
    IMPLEMENT_SIGNED_WRITE;

private:
    //
    // Helper functions.
    //
    void SetMediaInfo( void );
    void BuildFileSpec( const char* fileName, char* fullName, unsigned int size, char** pName );
    radFileError TranslateError( unsigned int windowsError );
    void GetPort( int* port, int* slot );
    bool Mount( void );
    void Unmount( void );
    bool HandleError( unsigned int error );
    IRadDrive::MediaInfo::MediaState ErrorToMediaState( radFileError result );
    void AsciiToUnicode( WCHAR* dest, const char* src );
    void UnicodeToAscii( char* dest, const WCHAR* src );

    unsigned int    m_Capabilities;
    unsigned int    m_OpenFiles;
    char            m_DriveName[ radFileDrivenameMax + 1 ];

    //
    // Check if a card is mounted, and keep it's real name.
    //
    bool            m_IsMounted;
    char            m_DriveLetter;

    //
    // We need to keep a small list to indicate if a file has been written to yet.
    //
    struct MemcardInitList
    {
        radFileHandle m_Handle;
    };
    MemcardInitList m_InitList[ XBOX_MAX_CARD_HANDLES ];

    //
    // Directory searching is a bit strange. We actually have to do the wildcard matching
    // ourselves!
    //
    struct MemcardDirHandle
    {
        char    m_pSpec[ radFileFilenameMax + 1 ];
        void*   m_Handle;
    };
    bool FileNameMatchesSearch( const char* pFileName, const char* pSearchSpec );

    MemcardDirHandle m_HandlePool[ XBOX_MAX_CARD_HANDLES ];

    enum DirResult
    {
        Done,
        Fail,
        NoMatch
    };  

    DirResult TranslateDirInfo( IRadDrive::DirectoryInfo*   pDirectoryInfo, 
                                const void*                 pFindData,
                                const void*                 winHandle,
                                const char*                 pSpec );
    //
    // Helper function for block caching.
    //
    bool LoadBlock( radFileHandle handle, unsigned int block );
    bool FlushBlock( void );
    unsigned int GetTableSize( unsigned int fileSize );
    bool GetSizeFromHandle( radFileHandle handle, unsigned int* size );
    bool InitializeFile( radFileHandle handle, unsigned int size, bool validHeader );
};

#endif // XBOXSAVEDRIVE_HPP

