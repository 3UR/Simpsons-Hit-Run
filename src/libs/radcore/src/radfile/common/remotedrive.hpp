//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        remotedrive.hpp
//
// Subsystem:	Radical Drive System
//
// Description:	This file contains all definitions and classes relevant to
//              a remote drive. This class routes file requests to a host
//              server program using the radical debug communication services.
//
// Note:        This will service Debug Communication.
//=============================================================================

#ifndef	REMOTEDRIVE_HPP
#define REMOTEDRIVE_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <raddebugcommunication.hpp>
#include <radtime.hpp>
#include "../../radprotocols/fileprotocol.hpp"
#include "drive.hpp"

//=============================================================================
// Statics
//=============================================================================

static const char s_RemoteDriveName[ ] = "REMOTEDRIVE:";

//
// Set up the alignment for our transfer buffer
//
#define REMOTE_DRIVE_ALIGNMENT   128

//=============================================================================
//  Function Definitions.
//=============================================================================

//
// Every physical drive type must provide a drivefactory.
//
void radRemoteDriveFactory( radDrive** ppDrive, const char* driveSpec, radMemoryAllocator alloc );

//=============================================================================
// Class Declarations
//=============================================================================

//
// This is a Remote Drive. It implements the appropriate raddrive members. 
//
class radRemoteDrive : public radDrive,
                       public IRadDbgComChannelSendCallback,
                       public IRadDbgComChannelReceiveCallback
{
public:
	//
    // Constructor and destructor
    //
    radRemoteDrive( radMemoryAllocator alloc, IRadDbgComChannel* channel );
    ~radRemoteDrive( void );

    void Lock( void );
    void Unlock( void );

    //
    // Use base class implementation.
    //
    virtual void AddRef( ) { radDrive::AddRef( ); }
    virtual void Release( ) { radDrive::Release( );}

    const char* GetDriveName( void );

    unsigned int GetCapabilities( void );

    CompletionStatus Initialize( void );

    CompletionStatus OpenFile( const char* fileName, 
                               radFileOpenFlags flags, 
                               bool writeAccess, 
                               radFileHandle* pHandle, 
                               unsigned int* pSize );

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


    CompletionStatus DestroyFile( const char* filename );

    CompletionStatus FindFirst( const char*                 searchSpec, 
                                IRadDrive::DirectoryInfo*   pDirectoryInfo, 
                                radFileDirHandle*           pHandle,
                                bool                        firstSearch );

    CompletionStatus FindNext( radFileDirHandle* pHandle, IRadDrive::DirectoryInfo* pDirectoryInfo );

    CompletionStatus FindClose( radFileDirHandle* pHandle );

    //
    // Debug communication callbacks
    //
    virtual void OnSendComplete( bool Successful ); 
    virtual void OnReceiveComplete( bool Successful, unsigned int bytesReceived );


private:
    void ProcessDisconnect( void );

    void Service( void );

    //
    // Service call routines
    //
    void DoInitialize( void );
    void DoOpen( void );
    void DoClose( void );
    void DoRead( void );
    void DoWrite( void );
    void DoDestroy( void );
    void DoFindFirst( void );
    void DoFindNext( void );
    void DoFindClose( void );

    //
    // Routine which are called on the callback.
    //
    void ReceiveOpen( unsigned int numBytes );
    void ReceiveClose( unsigned int numBytes );
    void ReceiveRead( unsigned int numBytes );
    void ReceiveWrite( unsigned int numBytes );
    void ReceiveDestroy( unsigned int numBytes );
    void ReceiveFindFirst( unsigned int numBytes );
    void ReceiveFindNext( unsigned int numBytes );
    void ReceiveFindClose( unsigned int numBytes );

    //
    // This is the interface to the channel we use for communication with the
    // host.
    //
    IRadDbgComChannel* m_Channel;
 
    //
    // Maintains state information about this drive object.
    // 
    enum State
    {
        Idle,                           // Not active
        DoneProcessing,                 // All processing done, waiting for callbacks
        Initializing,                   // Waiting for attachment
        Opening,                        // Open outstanding
        Closing,                        // Closeing a file
        Writing,                        // Writing a file
        Reading,                        // Reading a file
        Destroying,                     // Destroying a file
        FindingFirst,                   // Finding first search match
        FindingNext,                    // Continuing search
        ClosingFind                     // Terminating search
    };
  
    State           m_State;            // Current State
    
    //
    // Need a flag to monitor if a transmit is outstanding. Even though 
    // the protocol is send to host and wait for reply, we can have the receive
    // notification occur prior to the send complete.
    //
    bool            m_SendOutstanding;
    bool            m_ReceiveOutstanding;

    //
    // These two members are used to perfrom timeout operations.
    //
    unsigned int  m_StartTime;          // Used to perform timeout operations
    bool          m_TimerActive;
    
    //
    // This buffer we use for send and receiving file requests to the host
    //
    unsigned char   m_TransferBufferSpace[ sizeof( HfpReadRpy) + REMOTE_DRIVE_ALIGNMENT ];
    unsigned char*  m_TransferBuffer;

    //
    // These are used to hold information regarding the request in the case
    // where a connection must first be established.
    //
    union RequestData
    {
        struct 
        {
            const char*      m_pFileName;
            radFileOpenFlags m_Flags; 
            bool             m_WriteAccess;
            radFileHandle    m_Handle;
            unsigned int     m_Size;
        } m_OpenData;       

        struct 
        {
            radFileHandle    m_Handle;
        } m_CloseData;       
        
        struct 
        {
            const char*      m_pFileName;
        } m_DestroyData;       
        
        struct
        {
            radFileHandle    m_Handle;
            unsigned int     m_Position; 
            void*            m_pData; 
            unsigned int     m_BytesToRead;
            unsigned int     m_BytesRead; 
            radMemorySpace   m_pDataSpace;
        } m_ReadData;
        
        struct
        {
            radFileHandle    m_Handle;
            unsigned int     m_Position; 
            const void*      m_pData; 
            unsigned int     m_BytesToWrite;
            unsigned int     m_BytesWritten; 
            unsigned int     m_Size;
        } m_WriteData;
        
        struct
        {
            unsigned int              m_Handle;
            IRadDrive::DirectoryInfo* m_pDirInfo;
            const char*               m_pSearchSpec;
        } m_FindData;

    } m_RequestData;

    //
    // Mutex for critical sections
    //
    IRadThreadMutex*    m_pMutex;

    //
    // Semaphore to communicate with the service call.
    //
    IRadThreadSemaphore* m_pSema;

    //
    // Async copy request.
    //
    IRadMemorySpaceCopyRequest* m_pMemCpyRequest;
};

#endif

