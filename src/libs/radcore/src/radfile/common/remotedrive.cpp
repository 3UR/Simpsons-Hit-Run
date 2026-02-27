//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        remotedrive.cpp
//
// Subsystem:   Radical Drive System
//
// Description:	This file contains the implementation of the  Remote Drive class.
//              It is used to direct file requests to the host file server.
//
// Notes:       Media present -- attached.
//              Media invalid -- haven't attached yet.
//              Media not present -- failed to attach or connection lost.
//
// Control flow can happen in 3 places (thread, service or callback), but none of these
// can overlap.
//      The thread issues a request and changes state from Idle.
//      Service runs Do* to execute the request and possibly start a transaction.
//      Callback runs Recieve* to get the information from the transaction and sets
//          the state to DonePrrocessing.
//      When all the callbacks are finished, and the state is DoneProcessing, the Service
//        call wakes up the thread, which handles the results.
//
// Directory handles are NOT, in general, unsigned ints. So the directory handles are mem copied
// and no endian conversion is needed.
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <string.h>
#include <limits.h>
#include <radmemorymonitor.hpp>
#include <radplatform.hpp>
#include "drivethread.hpp"
#include "filesystem.hpp"
#include "remotedrive.hpp"

//=============================================================================
// Local Definitions
//=============================================================================

//
// Define a sector size for the remotedrive. This should be the sector size of
// the host machine - usually 512 bytes for PCs. If this value is wrong, the
// file server could die with an assert if target tries to read too far past the
// end of the file.
//
#define REMOTE_SECTOR_SIZE   512

//=============================================================================
// Public Functions 
//=============================================================================

//=============================================================================
// Function:    radRemoteDriveFactory
//=============================================================================
// Description: This member is responsible for constructing a radRemoteDriveObject.
//
// Parameters:  pointer to receive drive object
//              pointer to parent drive system.
//              pDriveName,
//              allocator
//              
// Returns:
//------------------------------------------------------------------------------

void radRemoteDriveFactory
( 
    radDrive**      ppDrive, 
    const char*     driveSpec,
    radMemoryAllocator alloc
)
{
    //
    // Attempt to get a communication channel. If we can't it means the system
    // has not been initialized and we will not allow communication using this
    // drive.
    //
    IRadDbgComChannel* channel;
    radDbgComTargetCreateChannel( HOST_FILE_PROTOCOL, &channel, alloc );
    
    if( channel == NULL )
    {
        rDebugString( "radFileSystem: Warning remote drive cannot be used.\n");
        *ppDrive = NULL;
        return;
    }

    //
    // Construct the remote drive, passing it its com channel.
    //
    *ppDrive = new( alloc ) radRemoteDrive( alloc, channel );

    //
    // We can drop our reference to the channel.
    //
    radRelease( channel, NULL );
}


//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    radRemoteDrive::radRemoteDrive
//=============================================================================
// Description: Constructor for the drive. Initialize things.
//
// Parameters:  pointer to parent drive system.
//              name of drive
//              communication channel
//              
// Returns:
//------------------------------------------------------------------------------

radRemoteDrive::radRemoteDrive
( 
    radMemoryAllocator  alloc,
    IRadDbgComChannel*  pChannel
)
    :
    radDrive( ),
    m_State( Idle ),
    m_SendOutstanding( false ),
    m_ReceiveOutstanding( false ),
    m_TimerActive( false ),
    m_pMutex( NULL ),
    m_pSema( NULL ),
    m_pMemCpyRequest( NULL )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radRemoteDrive" );

    //
    // We use timer services.
    //
    radTimeInitialize( );

    //
    // Create a mutex for lock/unlock
    //
    radThreadCreateMutex( &m_pMutex, alloc );
    rAssert( m_pMutex != NULL );

    //
    // Create a semaphore to communicate with the main thread.
    //
    radThreadCreateSemaphore( &m_pSema );
    rAssert( m_pSema != NULL );

    //
    // Create the drive thread.
    //
    m_pDriveThread = new( alloc ) radDriveThread( m_pMutex, alloc );
    rAssert( m_pDriveThread != NULL );

    //
    // This will be set in the initialize function.
    //
    m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaInvalid;             

    //
    // Set up other media info. Fix name and size. 
    //
    m_MediaInfo.m_SectorSize = REMOTE_SECTOR_SIZE;
    strcpy( m_MediaInfo.m_VolumeName, s_RemoteDriveName );
    m_MediaInfo.m_FreeSpace = UINT_MAX;
    m_MediaInfo.m_FreeFiles = m_MediaInfo.m_FreeSpace / REMOTE_SECTOR_SIZE;

    //
    // Set up our aligned buffer
    //
    rAssert( radMemorySpace_OptimalAlignment <= REMOTE_DRIVE_ALIGNMENT );
    rAssert( radFileOptimalMemoryAlignment <= REMOTE_DRIVE_ALIGNMENT );

    m_TransferBuffer = (unsigned char*) ::radMemoryRoundUp( (uintptr_t) m_TransferBufferSpace, REMOTE_DRIVE_ALIGNMENT );

    //
    // Attach 
    //
    m_Channel = pChannel;
    radAddRef( m_Channel, this );

    m_Channel->Attach( );
}

//=============================================================================
// Function:    radRemoteDrive::~radRemoteDrive
//=============================================================================
// Description: Destructor for the drive. Restore things
//
// Parameters:  
//              
// Returns:
//------------------------------------------------------------------------------

radRemoteDrive::~radRemoteDrive( void )
{
    //
    // Simply detach from the host and release channel interface.
    //
    m_Channel->Detach( );
    radRelease( m_Channel, this );

    radTimeTerminate( );

    m_pMutex->Release( );
    m_pSema->Release( );
    m_pDriveThread->Release( );
}

//=============================================================================
// Function:    radRemoteDrive::Lock
//=============================================================================
// Description: Start a critical section
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radRemoteDrive::Lock( void )
{
    m_pMutex->Lock( );
}

//=============================================================================
// Function:    radRemoteDrive::Unlock
//=============================================================================
// Description: End a critical section
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radRemoteDrive::Unlock( void )
{
    m_pMutex->Unlock( );
}

//=============================================================================
// Function:    radRemoteDrive::GetDriveName
//=============================================================================

const char* radRemoteDrive::GetDriveName( void )
{
    return s_RemoteDriveName;
}

//=============================================================================
// Function:    radRemoteDrive::GetCapabilities
//=============================================================================
// Description: Return what type of drive this is,

unsigned int radRemoteDrive::GetCapabilities( void )
{
    return radDriveWriteable | radDriveEnumerable | radDriveFile; 
;
}

//=============================================================================
// Function:    radRemoteDrive::Initialize
//=============================================================================

radDrive::CompletionStatus radRemoteDrive::Initialize( void )
{
    //
    // If we have no media, then we already know that we have an unrecoverable error.
    //
    if ( m_MediaInfo.m_MediaState == IRadDrive::MediaInfo::MediaNotPresent )
    {
        m_LastError = Success;
        return Complete;
    }
    
    //
    // Set the state to initializing and go to sleep.
    //
    m_State = Initializing;
    m_pSema->Wait( );

    return Complete;
}

void radRemoteDrive::DoInitialize( void )
{
    //
    // Check if we're attached
    //
    IRadDbgComChannel::ConnectionState connectionState;
    m_Channel->GetStatus( &connectionState, NULL );

    if( connectionState == IRadDbgComChannel::Attached )
    {
        //
        // Life is good.
        //
        m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaPresent;             

        m_LastError = Success;
        m_State = Idle;
        m_pSema->Signal( );
    }
    else
    {
        //
        // Set a timeout if we are just trying to attach for the first time.
        //
        if ( m_MediaInfo.m_MediaState == IRadDrive::MediaInfo::MediaInvalid )
        {
            if ( m_TimerActive )
            {
                //
                // Check if we've timed-out.
                //
                if( (radTimeGetMilliseconds( ) - m_StartTime) > radFileSystem::GetConnectTimeOut( ) )
                {
                    //
                    // Timeout occured.
                    //
                    m_TimerActive = false;
                    m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaNotPresent;
                    m_MediaInfo.m_VolumeName[ 0 ] = '\0';
                    m_MediaInfo.m_FreeSpace = 0;
                    m_MediaInfo.m_FreeFiles = 0;

                    rTunePrintf( "radFileSystem: Could not connect to Radical File Server on host\n" );

                    m_LastError = NoMedia;
                    m_State = Idle;
                    m_pSema->Signal( );
                }
            }
            else
            {
                //
                // Start the timeout
                //
                m_StartTime = radTimeGetMilliseconds( );
                m_TimerActive = true;
            }
        }
        else
        {
            //
            // We're not attached, and this isn't the first execution, so fail.
            //
            m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaNotPresent;

            m_LastError = NoMedia;
            m_State = Idle;
            m_pSema->Signal( );
        }
    }
}

//=============================================================================
// Function:    radRemoteDrive::Open
//=============================================================================

radDrive::CompletionStatus radRemoteDrive::OpenFile
(
    const char*         fileName, 
    radFileOpenFlags    flags, 
    bool                writeAccess, 
    radFileHandle*      pHandle, 
    unsigned int*       pSize
)
{ 
    //  
    // Set up the open request structure. Strip leading \ if specified,
    //
    if( fileName[0] == '\\' )
    {
        fileName++;
    }

    m_RequestData.m_OpenData.m_pFileName = fileName;
    m_RequestData.m_OpenData.m_Flags = flags;
    m_RequestData.m_OpenData.m_WriteAccess = writeAccess;

    m_State = Opening;
    m_pSema->Wait( );

    //
    // Check for errors.
    //
    if ( m_LastError != Success )
    {
        return Error;
    }
    
    *pHandle = m_RequestData.m_OpenData.m_Handle;
    *pSize = m_RequestData.m_OpenData.m_Size;

    return Complete;
}    

void radRemoteDrive::DoOpen( void )
{
    //
    // Build up the transmit buffer containing the open request.
    //
    HfpOpenCmd* pCommand = (HfpOpenCmd*) m_TransferBuffer;
       
    pCommand->m_Command = (HfpCommand) radPlatformEndian32( HfsOpen );
    pCommand->m_Flags = radPlatformEndian32(m_RequestData.m_OpenData.m_Flags );
    pCommand->m_WriteAccess = radPlatformEndian32( m_RequestData.m_OpenData.m_WriteAccess );
    strcpy( (char*) &pCommand->m_FileName[ 0 ], m_RequestData.m_OpenData.m_pFileName );
    
    //
    // Lets issue the send and receive. We use the same buffer but this will not
    // present a problem as the receive will not occur until the send completes.
    //
    m_SendOutstanding = true;
    m_ReceiveOutstanding = true;
    m_Channel->ReceiveAsync( m_TransferBuffer, sizeof( HfpOpenRpy ), this );
    m_Channel->SendAsync( m_TransferBuffer, sizeof( HfpOpenCmd ), this );
}

void radRemoteDrive::ReceiveOpen( unsigned int numBytes )
{
    HfpOpenRpy* pReply = (HfpOpenRpy*) m_TransferBuffer;        
    if( (numBytes != sizeof(HfpOpenRpy)) || ( (HfpCommand) radPlatformEndian32( pReply->m_Command ) != HfsOpen) )
    {
        ProcessDisconnect( );
        return;
    }

    //
    // Process the result of the open.
    //
    m_RequestData.m_OpenData.m_Size = radPlatformEndian32( pReply->m_Size );
   
    if( 
        radPlatformEndian( pReply->m_Handle ) == (void*)(-1) ||
        radPlatformEndian( pReply->m_Handle) == 0
      )
    {
        //
        // Failed to open.
        //
        rTunePrintf( "Failed to open file [ %s ] on host server\n", m_RequestData.m_OpenData.m_pFileName );
        m_LastError = FileNotFound;
    }
    else
    {
        m_RequestData.m_OpenData.m_Handle = (radFileHandle) radPlatformEndian( pReply->m_Handle );
        m_LastError = Success;
    }

    m_State = DoneProcessing;
}

//=============================================================================
// Function:    radRemoteDrive::Close
//=============================================================================

radDrive::CompletionStatus radRemoteDrive::CloseFile( radFileHandle handle, const char* fileName )
{
    m_RequestData.m_CloseData.m_Handle = handle;

    m_State = Closing;
    m_pSema->Wait( );

    if ( m_LastError != Success )
    {
        return Error;
    }

    return Complete;
}

void radRemoteDrive::DoClose( void )
{
    //
    // Build up the transmit buffer containing the request.
    //
    HfpCloseCmd* pCommand = (HfpCloseCmd*) m_TransferBuffer;
       
    pCommand->m_Command = (HfpCommand) radPlatformEndian32( HfsClose );
    pCommand->m_Handle = radPlatformEndian( m_RequestData.m_CloseData.m_Handle );
    
    //
    // Lets issue the send and receive. We use the same buffer but this will not
    // present a problem as the receive will not occur until the send completes.
    //
    m_SendOutstanding = true;
    m_ReceiveOutstanding = true;
    m_Channel->ReceiveAsync( m_TransferBuffer, sizeof( HfpCloseRpy ), this );
    m_Channel->SendAsync( m_TransferBuffer, sizeof( HfpCloseCmd ), this );
}

void radRemoteDrive::ReceiveClose( unsigned int numBytes )
{
    HfpCloseRpy* pReply = (HfpCloseRpy*) m_TransferBuffer;        
    if( (numBytes != sizeof(HfpCloseRpy)) || ((HfpCommand) radPlatformEndian32( pReply->m_Command) != HfsClose) )
    {
        ProcessDisconnect( );
        return;
    }

    m_LastError = Success;
    m_State = DoneProcessing;
}

//=============================================================================
// Function:    radRemoteDrive::DestroyFile
//=============================================================================

radDrive::CompletionStatus radRemoteDrive::DestroyFile( const char* fileName )
{ 
    if( fileName[0] == '\\' )
    {
        fileName++;
    }

    m_RequestData.m_DestroyData.m_pFileName = fileName;

    m_State = Destroying;
    m_pSema->Wait( );

    if ( m_LastError != Success )
    {
        return Error;
    }

    return Complete;
}

void radRemoteDrive::DoDestroy( void )
{
    //
    // Build up the transmit buffer containing the request.
    //
    HfpDestroyCmd* pCommand = (HfpDestroyCmd*) m_TransferBuffer;
    pCommand->m_Command = (HfpCommand) radPlatformEndian32( HfsDestroy );
    strcpy( (char*) &pCommand->m_FileName[ 0 ], m_RequestData.m_DestroyData.m_pFileName );

    //
    // Lets issue the send and receive. We use the same buffer but this will not
    // present a problem as the receive will not occur until the send completes.
    //
    m_SendOutstanding = true;
    m_ReceiveOutstanding = true;
    m_Channel->ReceiveAsync( m_TransferBuffer, sizeof( HfpDestroyRpy ), this );
    m_Channel->SendAsync( m_TransferBuffer, sizeof( HfpDestroyCmd ), this );
}

void radRemoteDrive::ReceiveDestroy( unsigned int numBytes )
{
    HfpDestroyRpy* pReply = (HfpDestroyRpy*) m_TransferBuffer;        
    if( (numBytes != sizeof(HfpDestroyRpy)) || ((HfpCommand) radPlatformEndian32( pReply->m_Command) != HfsDestroy) )
    {
        ProcessDisconnect( );
        return;
    }

    if( radPlatformEndian32( pReply->m_Result) != 0 )
    {    
        rDebugPrintf( "Failed to destroy file [%s] on host server.\n", m_RequestData.m_DestroyData.m_pFileName );
        m_LastError = FileNotFound;
    }
    else
    {
        m_LastError = Success;
    }

    m_State = DoneProcessing;
}

//=============================================================================
// Function:    radRemoteDrive::ReadFile
//=============================================================================

radDrive::CompletionStatus radRemoteDrive::ReadFile
( 
    radFileHandle      handle,
    const char*        fileName,
    IRadFile::BufferedReadState buffState,
    unsigned int       position, 
    void*              pData, 
    unsigned int       bytesToRead, 
    unsigned int*      bytesRead, 
    radMemorySpace     pDataSpace
)
{
    m_RequestData.m_ReadData.m_Handle = handle;
    m_RequestData.m_ReadData.m_Position = position;
    m_RequestData.m_ReadData.m_pData = pData;
    m_RequestData.m_ReadData.m_BytesToRead = bytesToRead;
    m_RequestData.m_ReadData.m_BytesRead = 0;
    m_RequestData.m_ReadData.m_pDataSpace = pDataSpace; 
   
    m_State = Reading;
    m_pSema->Wait( );

    if ( m_LastError != Success )
    {
        return Error;
    }
    *bytesRead = m_RequestData.m_ReadData.m_BytesRead;
    
    return Complete;
}

void radRemoteDrive::DoRead( void )
{
    //
    // Check if we're in the middle of a copy.
    //
    if ( m_pMemCpyRequest != NULL )
    {
        //
        // Continue our copy.
        //
        if ( m_pMemCpyRequest->IsDone( ) )
        {
            m_pMemCpyRequest->Release( );
            m_pMemCpyRequest = NULL;
        }
        else
        {
            //
            // Copy isn't done yet, so leave.
            //
            return;
        }
    }
    
    //
    // Check if there's anything to read.
    //
    if ( m_RequestData.m_ReadData.m_BytesToRead == 0 )
    {
        m_LastError = Success;
        m_State = Idle;
        m_pSema->Signal( );
        return;
    }

    //
    // Partition the read.
    //
    unsigned int readSize = HFP_MAX_READWRITE;
    if( m_RequestData.m_ReadData.m_BytesToRead < HFP_MAX_READWRITE )
    {
        readSize = m_RequestData.m_ReadData.m_BytesToRead;
    }  

    HfpReadCmd* pCommand = (HfpReadCmd*) m_TransferBuffer;
        
    pCommand->m_Command = (HfpCommand) radPlatformEndian32( HfsRead );
    pCommand->m_Handle =  radPlatformEndian( m_RequestData.m_ReadData.m_Handle );
    pCommand->m_Position = radPlatformEndian32( m_RequestData.m_ReadData.m_Position );
    pCommand->m_NumBytes = radPlatformEndian32( readSize );

    //
    // Lets issue the send and receive. We use the same buffer but this will not
    // present a problem as the receive will not occur until the send completes.
    //
    m_SendOutstanding = true;
    m_ReceiveOutstanding = true;
    m_Channel->ReceiveAsync( m_TransferBuffer, sizeof( HfpReadRpy ), this );
    m_Channel->SendAsync( m_TransferBuffer, sizeof( HfpReadCmd ), this );
}

void radRemoteDrive::ReceiveRead( unsigned int numBytes )
{
    HfpReadRpy* pReply = (HfpReadRpy*) m_TransferBuffer;        
    if( (HfpCommand) radPlatformEndian32( pReply->m_Command) != HfsRead ) 
    {
        ProcessDisconnect( );
        return;
    }

    unsigned int bytesRead = radPlatformEndian32( pReply->m_NumBytes );

    //
    // Copy in the data and set up the request for the next service call.
    //
    m_pMemCpyRequest = 
        radMemorySpaceCopyAsync( m_RequestData.m_ReadData.m_pData, 
                                 m_RequestData.m_ReadData.m_pDataSpace, 
                                 pReply->m_Data, 
                                 radMemorySpace_Local,
                                 bytesRead );

    rAssert( m_pMemCpyRequest != NULL );
    m_pMemCpyRequest->AddRef( );

    //
    // Set up the variables for the next read.
    //
    char* tmp = (char*) m_RequestData.m_ReadData.m_pData;
    m_RequestData.m_ReadData.m_pData = (void*) &tmp[ bytesRead ];
    m_RequestData.m_ReadData.m_BytesToRead -= bytesRead;
    m_RequestData.m_ReadData.m_BytesRead += bytesRead;
    m_RequestData.m_ReadData.m_Position += bytesRead;
}

//=============================================================================
// Function:    radRemoteDrive::WriteFile
//=============================================================================

radDrive::CompletionStatus radRemoteDrive::WriteFile
( 
    radFileHandle     handle, 
    const char*       fileName,
    IRadFile::BufferedReadState buffState,
    unsigned int      position, 
    const void*       pData, 
    unsigned int      bytesToWrite, 
    unsigned int*     pBytesWritten, 
    unsigned int*     pSize, 
    radMemorySpace    pDataSpace
)
{
    // 
    // This drive does not (yet) support the use of buffers in external memory.
    //
    rAssertMsg( pDataSpace == radMemorySpace_Local, 
                "radRemoteDrive: External memory not supported for writes." );


    m_RequestData.m_WriteData.m_Handle = handle;
    m_RequestData.m_WriteData.m_Position = position;
    m_RequestData.m_WriteData.m_pData = pData;
    m_RequestData.m_WriteData.m_BytesToWrite = bytesToWrite;
    m_RequestData.m_WriteData.m_BytesWritten = 0;

    m_State = Writing;
    m_pSema->Wait( );

    if ( m_LastError != Success )
    {
        return Error;
    }
    *pSize = m_RequestData.m_WriteData.m_Size;
    *pBytesWritten = m_RequestData.m_WriteData.m_BytesWritten;

    return Complete;
}

void radRemoteDrive::DoWrite( void )
{
    //
    // Partition the write.
    //
    unsigned int writeSize = HFP_MAX_READWRITE;
    if( m_RequestData.m_WriteData.m_BytesToWrite < HFP_MAX_READWRITE )
    {
        writeSize = m_RequestData.m_WriteData.m_BytesToWrite;
    }  

    HfpWriteCmd* pCommand = (HfpWriteCmd*) m_TransferBuffer;
    pCommand->m_Command = (HfpCommand) radPlatformEndian32( HfsWrite );
    pCommand->m_Handle = radPlatformEndian( m_RequestData.m_WriteData.m_Handle );
    pCommand->m_Position = radPlatformEndian32( m_RequestData.m_WriteData.m_Position );
    pCommand->m_NumBytes = radPlatformEndian32( writeSize );
    memcpy( &pCommand->m_Data[0], m_RequestData.m_WriteData.m_pData, writeSize );

    //
    // Lets issue the send and receive. We use the same buffer but this will not
    // present a problem as the receive will not occur until the send completes.
    //
    m_SendOutstanding = true;
    m_ReceiveOutstanding = true;
    m_Channel->ReceiveAsync( m_TransferBuffer, sizeof( HfpWriteRpy ), this );
    m_Channel->SendAsync( m_TransferBuffer, sizeof( HfpWriteCmd ) - (HFP_MAX_READWRITE - writeSize), this );
}

void radRemoteDrive::ReceiveWrite( unsigned int numBytes )
{
    HfpWriteRpy* pReply = (HfpWriteRpy*) m_TransferBuffer;        
    if( (numBytes != sizeof(HfpWriteRpy)) || ((HfpCommand) radPlatformEndian32( pReply->m_Command) != HfsWrite) )
    {
        ProcessDisconnect( );
        return;
    }

    m_RequestData.m_WriteData.m_Size = radPlatformEndian32( pReply->m_NewSize );

    unsigned int bytesWritten = radPlatformEndian32( pReply->m_NumBytes );

    //
    // Set up the next request
    //
    char* tmp = (char*) m_RequestData.m_WriteData.m_pData;
    m_RequestData.m_WriteData.m_pData = (void*) &tmp[ bytesWritten ];
    m_RequestData.m_WriteData.m_BytesWritten += bytesWritten;
    m_RequestData.m_WriteData.m_BytesToWrite -= bytesWritten;
    m_RequestData.m_WriteData.m_Position += bytesWritten;

    //
    // Check if we're done.
    //
    if ( m_RequestData.m_WriteData.m_BytesToWrite == 0 )
    {
        m_LastError = Success;
        m_State = DoneProcessing;
    }
}

//=============================================================================
// Function:    radRemoteDrive::FindFirst
//=============================================================================

radDrive::CompletionStatus radRemoteDrive::FindFirst( 
    const char*                 searchSpec, 
    IRadDrive::DirectoryInfo*   pDirectoryInfo, 
    radFileDirHandle*           pHandle,
    bool                        firstSearch 
)
{
    if( searchSpec[0] == '\\' )
    {
        searchSpec++;
    }

    m_RequestData.m_FindData.m_pDirInfo = pDirectoryInfo;
    m_RequestData.m_FindData.m_pSearchSpec = searchSpec;
   
    m_State = FindingFirst;
    m_pSema->Wait( );

    if ( m_LastError != Success )
    {
        return Error;
    }

    memcpy( pHandle, &( m_RequestData.m_FindData.m_Handle ), sizeof( unsigned int ) );

    return Complete;
}

void radRemoteDrive::DoFindFirst( void )
{
    HfpFindFirstCmd* pCommand = (HfpFindFirstCmd*) m_TransferBuffer;
        
    pCommand->m_Command = (HfpCommand) radPlatformEndian32( HfsFindFirst );
    strcpy( pCommand->m_SearchSpec, m_RequestData.m_FindData.m_pSearchSpec );

    //
    // Lets issue the send and receive. We use the same buffer but this will not
    // present a problem as the receive will not occur until the send completes.
    //
    m_SendOutstanding = true;
    m_ReceiveOutstanding = true;
    m_Channel->ReceiveAsync( m_TransferBuffer, sizeof( HfpFindFirstRpy ), this );
    m_Channel->SendAsync( m_TransferBuffer, sizeof( HfpFindFirstCmd ), this );
}

void radRemoteDrive::ReceiveFindFirst( unsigned int numBytes )
{
    HfpFindFirstRpy* pReply = (HfpFindFirstRpy*) m_TransferBuffer;        
    if( (numBytes != sizeof(HfpFindFirstRpy)) || ( (HfpCommand) radPlatformEndian32( pReply->m_Command ) != HfsFindFirst) )
    {
        ProcessDisconnect( );
        return;
    }

    //
    // Process the result of the FindFirst
    //
    if( radPlatformEndian32( pReply->m_Handle ) == 0xffffffff )
    {
        //
        // Operation failed. 
        //
        rTunePrintf( "FindFirst [ %s ] failed on host server.\n", m_RequestData.m_FindData.m_pSearchSpec );
        m_LastError = FileNotFound;
    } 
    else
    {
        //
        // Operation succeeded.
        //
        m_RequestData.m_FindData.m_Handle = pReply->m_Handle;
        strcpy( m_RequestData.m_FindData.m_pDirInfo->m_Name, pReply->m_Name );
        m_RequestData.m_FindData.m_pDirInfo->m_Type = 
            ( IRadDrive::DirectoryInfo::DirectoryEntryType ) 
            radPlatformEndian32( ( unsigned int ) pReply->m_Type );
        m_LastError = Success;
    }
    m_State = DoneProcessing;
}

//=============================================================================
// Function:    radRemoteDrive::FindNext
//=============================================================================
// Description: This member is used to continue a directory traversal.
//
// Parameters:  pDirectoryInfo - place to store info about matching files
//              pHandle - place where search handle was stored by FindFirst.
//              
// Returns:     CompletionStatus
//------------------------------------------------------------------------------

radDrive::CompletionStatus radRemoteDrive::FindNext( radFileDirHandle* pHandle, IRadDrive::DirectoryInfo* pDirectoryInfo )
{
    memcpy( &( m_RequestData.m_FindData.m_Handle ), pHandle, sizeof( unsigned int ) );
    m_RequestData.m_FindData.m_pDirInfo = pDirectoryInfo;
    m_RequestData.m_FindData.m_pSearchSpec = NULL;
   
    m_State = FindingNext;
    m_pSema->Wait( );

    if ( m_LastError != Success )
    {
        return Error;
    }

    return Complete;
}

void radRemoteDrive::DoFindNext( void )
{
    HfpFindNextCmd* pCommand = (HfpFindNextCmd*) m_TransferBuffer;
        
    pCommand->m_Command = (HfpCommand) radPlatformEndian32( HfsFindNext );
    pCommand->m_Handle = m_RequestData.m_FindData.m_Handle;

    //
    // Lets issue the send and receive. We use the same buffer but this will not
    // present a problem as the receive will not occur until the send completes.
    //
    m_SendOutstanding = true;
    m_ReceiveOutstanding = true;
    m_Channel->ReceiveAsync( m_TransferBuffer, sizeof( HfpFindNextRpy ), this );
    m_Channel->SendAsync( m_TransferBuffer, sizeof( HfpFindNextCmd ), this );
}

void radRemoteDrive::ReceiveFindNext( unsigned int numBytes )
{
    HfpFindNextRpy* pReply = (HfpFindNextRpy*) m_TransferBuffer;        
    if( (numBytes != sizeof(HfpFindNextRpy)) || ( (HfpCommand) radPlatformEndian32( pReply->m_Command ) != HfsFindNext) )
    {
        ProcessDisconnect( );
        return;
    }

    //
    // Process the result of the FindNext
    //
    if( radPlatformEndian32( pReply->m_Result ) == 0xffffffff )
    {
        //
        // Operation failed. 
        //
        rTunePrintf( "FindNext [ %s ] failed on host server.\n", m_RequestData.m_FindData.m_pSearchSpec );
        m_LastError = FileNotFound;
    } 
    else
    {
        //
        // Operation succeeded.
        //
        strcpy( m_RequestData.m_FindData.m_pDirInfo->m_Name, pReply->m_Name );
        m_RequestData.m_FindData.m_pDirInfo->m_Type = 
            ( IRadDrive::DirectoryInfo::DirectoryEntryType ) 
            radPlatformEndian32( ( unsigned int ) pReply->m_Type );
        m_LastError = Success;
    }
    m_State = DoneProcessing;
}

//=============================================================================
// Function:    radRemoteDrive::FindClose
//=============================================================================

radDrive::CompletionStatus radRemoteDrive::FindClose( radFileDirHandle* pHandle )
{
    memcpy( &( m_RequestData.m_FindData.m_Handle ), pHandle, sizeof( unsigned int ) );
    m_RequestData.m_FindData.m_pDirInfo = NULL;
    m_RequestData.m_FindData.m_pSearchSpec = NULL;
   
    m_State = ClosingFind;
    m_pSema->Wait( );

    if ( m_LastError != Success )
    {
        return Error;
    }

    memcpy( pHandle, &( m_RequestData.m_FindData.m_Handle ), sizeof( unsigned int ) );

    return Complete;
}

void radRemoteDrive::DoFindClose( void )
{
    HfpFindCloseCmd* pCommand = (HfpFindCloseCmd*) m_TransferBuffer;
        
    pCommand->m_Command = (HfpCommand) radPlatformEndian32( HfsFindClose );
    pCommand->m_Handle = m_RequestData.m_FindData.m_Handle;

    //
    // Lets issue the send and receive. We use the same buffer but this will not
    // present a problem as the receive will not occur until the send completes.
    //
    m_SendOutstanding = true;
    m_ReceiveOutstanding = true;
    m_Channel->ReceiveAsync( m_TransferBuffer, sizeof( HfpFindCloseRpy ), this );
    m_Channel->SendAsync( m_TransferBuffer, sizeof( HfpFindCloseCmd ), this );
}

void radRemoteDrive::ReceiveFindClose( unsigned int numBytes )
{
    HfpFindCloseRpy* pReply = (HfpFindCloseRpy*) m_TransferBuffer;        
    if( (numBytes != sizeof(HfpFindCloseRpy)) || ( (HfpCommand) radPlatformEndian32( pReply->m_Command ) != HfsFindClose) )
    {
        ProcessDisconnect( );
        return;
    }

    //
    // Process the result of the FindClose
    //
    if( radPlatformEndian32( pReply->m_Result ) == 0xffffffff )
    {
        //
        // Operation failed. 
        //
        rTunePrintf( "FindClose [ %s ] failed on host server.\n", m_RequestData.m_FindData.m_pSearchSpec );
        m_LastError = FileNotFound;
    } 
    else
    {
        //
        // Operation succeeded.
        //
        m_RequestData.m_FindData.m_Handle = 0xffffffff;
        m_LastError = Success;
    }
    m_State = DoneProcessing;
}

//=============================================================================
// Function:    radRemoteDrive::Service
//=============================================================================

void radRemoteDrive::Service( void )
{
    //
    // Check if we've given up due to an error.
    //
    if ( m_MediaInfo.m_MediaState == IRadDrive::MediaInfo::MediaNotPresent )
    {
        //
        // The network failed, return error.
        //
        if ( m_State != Idle )
        {
            m_LastError = NoMedia;
            m_State = Idle;
            m_pSema->Signal( );
        }
    }
    else
    {
        ::radDbgComService( );

        //
        // If there are outstanding transactions, the control is in the callbacks.
        //
        if ( m_SendOutstanding || m_ReceiveOutstanding )
        {
            return;
        }

        //
        // Check our state and act accordingly.
        //
        switch( m_State )
        {
        case Idle:
            //
            // Out of our hands ...
            //
            break;

        case DoneProcessing:
            m_State = Idle;
            m_pSema->Signal( );
            break;

        case Initializing:
            DoInitialize( );
            break;

        case Opening:
            DoOpen( );
            break;

        case Closing:
            DoClose( );
            break;

        case Writing:
            DoWrite( );
            break;

        case Reading:
            DoRead( );
            break;

        case Destroying:
            DoDestroy( );
            break;

        case FindingFirst:
            DoFindFirst( );
            break;

        case FindingNext:
            DoFindNext( );
            break;

        case ClosingFind:
            DoFindClose( );
            break;

        default:
            rAssertMsg( false, "internal error" );
            break;
        }
    }

    //
    // Call the base class service.
    //
    radDrive::Service( );
}

//=============================================================================
// Function:    radRemoteDrive::OnSendComplete
//=============================================================================

void radRemoteDrive::OnSendComplete( bool Successful )
{
    m_SendOutstanding = false;

    if( Successful )
    {
    }
    else
    {
        ProcessDisconnect( );
    }
}

//=============================================================================
// Function:    radRemoteDrive::OnReceiveComplete
//=============================================================================

void radRemoteDrive::OnReceiveComplete( bool Successful, unsigned int numBytes )
{
    m_ReceiveOutstanding = false;

    if( !Successful )
    {
        ProcessDisconnect( );
        return;
    }

    switch( m_State )
    {

    case Opening:
        ReceiveOpen( numBytes );
        break;

    case Closing:
        ReceiveClose( numBytes );
        break;

    case Writing:
        ReceiveWrite( numBytes );
        break;

    case Reading:
        ReceiveRead( numBytes );
        break;

    case Destroying:
        ReceiveDestroy( numBytes );
        break;

    case FindingFirst:
        ReceiveFindFirst( numBytes );
        break;

    case FindingNext:
        ReceiveFindNext( numBytes );
        break;

    case ClosingFind:
        ReceiveFindClose( numBytes );
        break;

    default:
        rAssertMsg( false, "internal error" );
        break;
    }
}

//=============================================================================
// Function:    radRemoteDrive::ProcessDisconnect
//=============================================================================

void radRemoteDrive::ProcessDisconnect( void )
{
    //
    // Used in callbacks.
    //
    m_LastError = NoMedia;
    m_State = DoneProcessing;
}