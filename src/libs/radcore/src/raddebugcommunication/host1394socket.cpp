//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        host1394socket.cpp
//
// Subsystem:	Radical Debug Communication System
//
// Description:	This file contains the an implementation of sockets using
//              the rad1394 FireWire communicaiton system.
//
//				The code contained in this file is strictly for the host.
//				This implementation makes many assumptions on how 
//              it is used. It is in no way a complete and true implementation
//              of sockets.
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	Jul 16, 2001
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <raddebug.hpp>
#include <radmemorymonitor.hpp>
#include "host1394socket.hpp"

//=============================================================================
// Constants.
//=============================================================================

#define MUTEX_NAME           "RadCore1394Mutex"
#define MEMORYFILE_NAME      "RadCore1394Memory"

//
// Time-out for target communication.
//
#define TARGET_COMMUNICATION_TIMEOUT    5000
#define SOCKET_CONNECTION_TIMEOUT_REGISTRY_KEY    "Software\\Radical Entertainment\\rDbgCom\\SocketConnectionTimeout"
#define SOCKET_TIMEOUT_FIELD       "TimeoutValue"

//
// Time-out for connections waiting for a listen.
//
#define CONNECTION_TIMEOUT              5000

//
// Current implementation assumes BUS number 0x3ff and node number 0. In future, we
// can obtain this from the target editor is this is not the case.
//
#define BUS_NUMBER          0x3ff
#define NODE_NUMBER         0x00

//=============================================================================
// Static Data
//=============================================================================

unsigned int g_SequenceNumber = 0x11234343;
unsigned int g_LastSequence = 75848434;

//=============================================================================
// Member Functions
//=============================================================================

//=============================================================================
// Function:    CHost1394Socket::CHost1394Socket
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

CHost1394Socket::CHost1394Socket( void )
    :
        m_p1394Memory( NULL )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "CHost1394Socket" );

    //
    // Set up the socket connection timeout value based on the entry in Windows Registry.
	// This allows the game team to adjust the timeout value more easily.
    //
	m_SocketConnectionTimeoutValue = TARGET_COMMUNICATION_TIMEOUT;

    HKEY RegKey;

    //
	// try to open the registry key testing if it exists or not
    //
	LONG result = RegOpenKeyEx(HKEY_CURRENT_USER,
								SOCKET_CONNECTION_TIMEOUT_REGISTRY_KEY,
								0,
								KEY_ALL_ACCESS,
								&RegKey);

	if( (result == ERROR_SUCCESS) || (result == REG_OPENED_EXISTING_KEY) )
	{
        //
		// the key exists
		unsigned int timeoutValue = 0;
	    DWORD tmpLength = sizeof( unsigned int );

		// query the field value
        LONG result = RegQueryValueEx( RegKey, SOCKET_TIMEOUT_FIELD, 0, NULL, (unsigned char *)&timeoutValue, &tmpLength);
		if(result == ERROR_SUCCESS)
		{
			// the field exists
			m_SocketConnectionTimeoutValue = timeoutValue;
		}
	}    

    RegCloseKey( RegKey );

 
    //
    // First obtain access to the mutex we use to protect manipulation of 
    // shared memory.
    //
    m_MutexHandle = CreateMutex( NULL, FALSE, MUTEX_NAME );
    rAssert( m_MutexHandle != NULL );

    //
    // Now gain ownership of this mutex. Wait forever.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );

    //
    // Gain access to the shared memory containing our socket info.
    // If it does not exist yet, we are responsible for initializing it.
    //
    m_MemoryFileHandle = CreateFileMapping( (HANDLE) 0xFFFFFFFF, NULL, PAGE_READWRITE, 0, 
                                            sizeof( SharedMemoryMap ),
                                            MEMORYFILE_NAME );
    rAssert( m_MemoryFileHandle  != NULL );

    //
    // Get the last error to see if the file already exists. If not, we are responsible
    // for initializing it.
    //
    bool AlreadyInitialized = (ERROR_ALREADY_EXISTS == GetLastError( ) );

    //
    // Map the view of the file into our process space.
    //
    m_pSharedData = (SharedMemoryMap*) MapViewOfFile( m_MemoryFileHandle, FILE_MAP_WRITE, 0, 0, 0 );
    rAssert( m_pSharedData != NULL );

    //
    // Initialize the fire wire stuff.
    //
    rad1394Initialize( sizeof( FW1394MemoryMap ) );
    m_pIRad1394Master = rad1394Get( );
    rAssert( m_pIRad1394Master != NULL );        
    m_pIRad1394Master->AddRef( );
 
    if( !AlreadyInitialized )
    {
        //
        // We are the first process launched. Lets initialized the shared memory.
        //
        for( unsigned int i = 0 ; i < FW1394_CHANNEL_COUNT ; i++ )
        {
            m_pSharedData->m_SocketArray[ i ].m_BytesInRxBuffer = 0;      
            m_pSharedData->m_SocketArray[ i ].m_BytesInTxBuffer = 0;   
            m_pSharedData->m_SocketArray[ i ].m_State = Free;
        }

        //
        // Clear flag inidicating that no thread is currently doing the 1394 i/o interface
        // calls. First created socket will do this. Also indicate that things need
        // to be initialized.
        //
        m_pSharedData->m_1394ThreadActive = false;
        m_pSharedData->m_1394DataInitialized = false;
    }

    //
    // Since we have the shared memory visble in this process space and it has been
    // initialized, we can release the mutex.
    //
    ReleaseMutex( m_MutexHandle );
}

//=============================================================================
// Function:    CHost1394Socket::~CHost1394Socket( void )
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

CHost1394Socket::~CHost1394Socket( void )
{
    //
    // Lets clean-up. Release our resources.
    // Gain ownership of this mutex. Wait forever.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );

    m_pIRad1394Master->Release( );
    rad1394Terminate( );

    //
    // Unmap view of the file and close it.
    //
    UnmapViewOfFile( m_pSharedData );

    CloseHandle( m_MemoryFileHandle );

    //
    // Release our mutex.
    //
    ReleaseMutex( m_MutexHandle );
            
    CloseHandle( m_MutexHandle );
}

//=============================================================================
// Function:    CHost1394Socket::socket
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------
    
int CHost1394Socket::socket
( 
    int     af, 
    int     type,
    int     protocol
)
{
    //
    // Find a free entry in the socket pool and return it.
    // Do this under protection of our
    // mutex as this table is used by all processes and the 1394 thread.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );

    for( unsigned int i = 0 ; i < FW1394_CHANNEL_COUNT ; i++ )
    {
        if( m_pSharedData->m_SocketArray[ i ].m_State == Free )
        {
            m_pSharedData->m_SocketArray[ i ].m_State = Idle;
            m_pSharedData->m_SocketArray[ i ].m_BytesInRxBuffer = 0;
            m_pSharedData->m_SocketArray[ i ].m_BytesInTxBuffer = 0;
            
            ReleaseMutex( m_MutexHandle );
          
            //
            // Save the parent object in the table so the thread can get it back.
            //
            m_pSharedData->m_SocketArray[ i ].m_pHost1394Socket = this;

            //
            // We also save the address of the socket in the shared memory. This memory
            // is not guareented to be mapped into the same address space in all processes
            //
            m_pSharedData->m_SocketArray[ i ].m_Socket = &m_pSharedData->m_SocketArray[ i ];
                
            //
            // Lets create a thread to do work on behalf of this socket.
            //
            m_pSharedData->m_SocketArray[ i ].m_ThreadHandle = 
                CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) SocketThread, 
                              &m_pSharedData->m_SocketArray[ i ], 0, 
                              &m_pSharedData->m_SocketArray[ i ].m_ThreadID );

            return( (int) &m_pSharedData->m_SocketArray[ i ] );
        }        
    }
    
    //
    // If we get here, we have too many connections.
    //
    ReleaseMutex( m_MutexHandle );
    
    return( INVALID_SOCKET );
}
    
//=============================================================================
// Function:    CHost1394Socket::setsockopt
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHost1394Socket::setsockopt
( 
    int             sock, 
    int             level, 
    int             optname,
    const char*     optval,
    int             optlen
)
{
    //
    // We just return success as we know the socket behavior.
    //
    return( 0 );
}

//=============================================================================
// Function:    CHost1394Socket::bind
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHost1394Socket::bind
( 
    int     sock,
    struct  sockaddr* addr,
    int     addrlen
)
{
    //
    // Does nothing
    //
    return( 0 );
};

//=============================================================================
// Function:    CHost1394Socket::listen
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHost1394Socket::listen( int sock, int backlog)
{
    // Not invoked by host
    rAssert( false );
    return( -1 );
}

//=============================================================================
// Function:    CHost1394Socket::accept
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHost1394Socket::accept( int sock, struct sockaddr* addr, int* addrlen)
{
    // Not invoked by host
    rAssert( false );
    return( -1 );
}


//=============================================================================
// Function:    CHost1394Socket::AsyncSelect
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHost1394Socket::AsyncSelect( SOCKET s, HWND Wnd, unsigned int wMsg, long levent )
{
    //
    // Socket passed in is actual a pointer into the shared memory socket structure
    //
    Socket* pSocket = (Socket*) s ;

    //
    // Save the Window handle and the mesage 
    //
    pSocket->m_hWnd = Wnd;
    pSocket->m_WndMsg = wMsg;

    return( 0 );
}

//=============================================================================
// Function:    CHost1394Socket::closesocket
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHost1394Socket::closesocket( int sock )
{
    //
    // Socket passed in is actual a pointer into the shared memory socket structure
    //
    Socket* pSocket = (Socket*) sock ;

    //
    // Grab the mutex and check the state.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );

    //
    // Check if connected. If not don't worry.
    //
    if( pSocket->m_State == Connected )
    {
        //
        // Check it we have data to send. If so set pending disconnect.
        // Else set disconnect.
        //
        if( pSocket->m_BytesInTxBuffer == 0 )
        {
            pSocket->m_State = Disconnected;
        }
        else
        {
            pSocket->m_State = PendingDisconnect;
        }
    }
    else
    {
        pSocket->m_State = ToBeFreed;
    }                   
    
    HANDLE threadHandle = pSocket->m_ThreadHandle; 

    ReleaseMutex( m_MutexHandle );

    //
    // Wait here for the thread to terminate. It will since we ininitaited a free.
    //
    WaitForSingleObject( threadHandle, INFINITE );

    CloseHandle( threadHandle );

    return( 0 );
}

//=============================================================================
// Function:    CHost1394Socket::connect
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHost1394Socket::connect( int sock, const struct sockaddr* addr, int addrlen)
{
    //
    // Socket passed in is actual a pointer into the shared memory socket structure
    //
    Socket* pSocket = (Socket*) sock ;

    //
    // IP address is ignored as the connection is point to point.
    //
   
    // 
    // Set state to connecting. All stuff happens in tha background.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );

    rAssert( pSocket->m_State == Idle );
    pSocket->m_State = Connecting;
    pSocket->m_ConnectTime = GetTickCount( );
    
    //
    // Set the  channel index to invalid. This will signal we have yet to obtain
    // a channel.
    //
    pSocket->m_1394Channel = FW1394_CHANNEL_COUNT;
       
    ReleaseMutex( m_MutexHandle );

    return( SOCKET_ERROR );
}

//=============================================================================
// Function:    CHost1394Socket::lasterror
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHost1394Socket::lasterror( int sock )
{

    //
    // Socket passed in is actual a pointer into the shared memory socket structure
    //
    Socket* pSocket = (Socket*) sock ;

    //
    // This routine gets the last error. It assumes that only called when 
    // a send or receive fails or when connect not complete.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );

    if( (pSocket->m_State == Connected) || (pSocket->m_State == Connecting ) )
    {
        ReleaseMutex( m_MutexHandle );
        return( WSAEWOULDBLOCK );
    }
    else
    {
        ReleaseMutex( m_MutexHandle );
        return( WSAENOTCONN );
    }
}

//=============================================================================
// Function:    CHost1394Socket::recv
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHost1394Socket::recv( int sock, char* buf, int len, int flags)
{
    //
    // Socket passed in is actual a pointer into the shared memory socket structure
    //
    Socket* pSocket = (Socket*) sock ;

    WaitForSingleObject( m_MutexHandle, INFINITE );

    //
    // Make sure connecteded. If not return -1;
    //
    if( pSocket->m_State != Connected )
    {
        ReleaseMutex( m_MutexHandle );
        return( -1 );
    }
    
    //
    // See if any data. If so return as much as the caller asked for.
    //
    if( pSocket->m_BytesInRxBuffer == 0 )
    {
       ReleaseMutex( m_MutexHandle );
       return( 0 );
    }
    
    if( len >= (int) pSocket->m_BytesInRxBuffer )
    {
        len = pSocket->m_BytesInRxBuffer;
        memcpy( buf, pSocket->m_RxBuffer, len );
        pSocket->m_BytesInRxBuffer = 0;
    }
    else
    {
        pSocket->m_BytesInRxBuffer -= len;
        memcpy( buf, pSocket->m_RxBuffer, len );
        
        //
        // This is lazy, but will revisit if performance is bad.
        //
        memmove( pSocket->m_RxBuffer, &pSocket->m_RxBuffer[ len ], pSocket->m_BytesInRxBuffer );
    }

    ReleaseMutex( m_MutexHandle );

    return( len );
}

//=============================================================================
// Function:    CHost1394Socket::send
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHost1394Socket::send( int sock, const char* buf, int len, int flags)
{
    //
    // Socket passed in is actual a pointer into the shared memory socket structure
    //
    Socket* pSocket = (Socket*) sock ;

    WaitForSingleObject( m_MutexHandle, INFINITE );

    //
    // Make sure connecteded. If not return -1;
    //
    if( pSocket->m_State != Connected )
    {
        ReleaseMutex( m_MutexHandle );
        return( -1 );
    }
    
    //
    // See if any data. If so return as much as the caller asked for.
    //
    if( pSocket->m_BytesInTxBuffer != 0 )
    {   
        ReleaseMutex( m_MutexHandle );
        return( 0 );
    }
 
    if( len > FW1394_CHANNEL_SIZE )
    {
        len = FW1394_CHANNEL_SIZE;
    }

    memcpy( pSocket->m_TxBuffer, buf, len );
    pSocket->m_BytesInTxBuffer = len;

    ReleaseMutex( m_MutexHandle );

    return( len );
}

//=============================================================================
// Function:    CHost1394Socket::ioctlsocket
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHost1394Socket::ioctlsocket( SOCKET s, long cmd, u_long *argp )
{
    return( 0 );
}

//=============================================================================
// Function:    CHost1394Socket::SocketThread
//=============================================================================
// Description: This is the thread that does work for the socket. One of 
//              the threads is responsible for performing i/o on behalf of
//              the all processes. 
//
// Parameters:  Pointer to socket in shared memory
//              
// Returns:     0
//
// Notes:       
//------------------------------------------------------------------------------

DWORD WINAPI CHost1394Socket::SocketThread
( 
    struct Socket* pSocket
)
{
    //
    // Get the socket implementation object from the socket.
    // 
    CHost1394Socket* pHost1394Socket = pSocket->m_pHost1394Socket;

    //
    // Here we initialized a flag indicating that we are not the thread that
    // does target communition. Will change based on creation and termination
    // of processe which are using this system.
    //
    bool    doingTargetCommunicaiton = false;

    do
    {
        //
        // Be a well behaved citizen. Should developed this system to suspend exclicitly
        // waiting for changes. Sleep for now.
        //
        Sleep( 0 );

        //
        // Will do the communicaiton if no other thread is.
        //
        doingTargetCommunicaiton = pHost1394Socket->DoTargetCommunication( doingTargetCommunicaiton );

    }
    while( pSocket->m_State != ToBeFreed );

    //
    // Actually set the socket free.
    //
    pSocket->m_State = Free;    
     
    //
    // We we break out of the above loop, it means the socket was disconnected or closed.
    // If we were doing target i/o for everyone else, then miust release the 1394 resources,
    // and set a flag so some other thread will take up the torch.
    //
    if( doingTargetCommunicaiton )
    {
        pHost1394Socket->DoTargetCommunicationCleanUp(  );
    }

    return( 0 );
}

//=============================================================================
// Function:    DoTargetCommunication:        
//=============================================================================
// Description: This function is invoked for two purposes. One is to check if
//              a thread exists for doing the host communication. If so, then
//              do nothing. If not, then this thread becomes the thread responsible
//              for performing i/o.
//
// Parameters:  boolean indicating if the thread was previously doing host i/o
//              
// Returns:     true if this thread is doing the i/o.
//              false if not.
//
// Notes:       
//------------------------------------------------------------------------------

bool CHost1394Socket::DoTargetCommunication( bool doingTargetCommunication )
{
    unsigned int i;
    FW1394ChannelControlBlock  controlBlock;

    if( !doingTargetCommunication )
    {
        //
        // Lets see if there is another thread doing the work. If so, we don't
        // need to do anything.
        //
        WaitForSingleObject( m_MutexHandle, INFINITE );
        
        if( m_pSharedData->m_1394ThreadActive )
        {
            //
            // Here the flag indicates that there is a thread doing host communication.
            // No need for us to do it.
            //
            ReleaseMutex( m_MutexHandle );
            
            return( doingTargetCommunication );
        }
    
        //
        // Here there is not thread doing the communication. Set ourself up as this
        // thread and initialized things.
        //
        doingTargetCommunication = true;
        m_pSharedData->m_1394ThreadActive = true;

        ReleaseMutex( m_MutexHandle );
    
        //
        // Since we have just become the owner, set time of last contact to the current
        // time. Use this to time out loss of contact with target.
        //
        m_TimeOfLastContact = GetTickCount( );

    }        

    //
    // Lets see if the actual 1394 shared data needs initialization. Only done if first time
    // or if we lose contact.
    //
    if( !m_pSharedData->m_1394DataInitialized )
    {
        //
        // Lets initialize things. Set up the shared memory. We will give it to the target.
        //
        for( unsigned int i = 0 ; i < FW1394_CHANNEL_COUNT ; i++ )
        {
            controlBlock.m_ChannelAllocationMap[ i ] = 0;
            controlBlock.m_HostTransmitCount[ i ] = 0;
            controlBlock.m_TargetTransmitCount[ i ] = 0;
            controlBlock.m_MessageMap[ i ] = FW1394ChannelControlBlock::NullMessage;
        }
        
        //
        // Set the owner of this memory as the target. Also set Crc.
        //
        controlBlock.m_HeadSequenceNumber = g_SequenceNumber;
        controlBlock.m_TailSequenceNumber = g_SequenceNumber;
        g_SequenceNumber++;
        controlBlock.m_MemoryOwner = FW1394ChannelControlBlock::TARGET_OWNS_MEMORY;
        controlBlock.m_Crc = 0;
        controlBlock.m_Crc = MakeCrc( &controlBlock, sizeof( controlBlock ) );
    
        //
        // Lets write the data into the shared memory.
        //
        bool result = m_pIRad1394Master->WriteMemory( BUS_NUMBER, NODE_NUMBER, (unsigned int)  &m_p1394Memory->m_ChannelControlBlock,  
                                        sizeof( controlBlock ), &controlBlock );

        if( result )
        {
            m_pSharedData->m_1394DataInitialized = true;
        }            
    }

    //
    // Here we are the thread doing the communication with the target. Read the shared memory and see it ownership
    // has been transfer to us.
    //
    unsigned int crcFailCount = 0;
ReRead:
    bool result =  m_pIRad1394Master->ReadMemory( BUS_NUMBER, NODE_NUMBER, (unsigned int)  &m_p1394Memory->m_ChannelControlBlock,  
                                            sizeof( controlBlock ), &controlBlock );
    
    unsigned int crc = controlBlock.m_Crc;

    if( !result )
    {
        //
        // Error. Treat as a com time out.
        //
        goto TimeOut;
    }

    //
    // Check crc. If invalid, reread.
    //
    controlBlock.m_Crc = 0;
    if( crc != MakeCrc( &controlBlock, sizeof( controlBlock ) ) )
    {
        crcFailCount++;

        if( crcFailCount < 50 )
        {
             goto ReRead;
        }
        else
        {
            crcFailCount = 0;
            goto TimeOut;
        }
             
    }

    //
    // Check if target restarted on us. Again treat like timeout
    //
    if( controlBlock.m_MemoryOwner == FW1394ChannelControlBlock::TARGET_RESTARTED )
    {
        goto TimeOut;
    }           

    if( controlBlock.m_MemoryOwner == FW1394ChannelControlBlock::TARGET_OWNS_MEMORY )
    {
        //
        // Here there has been an error or the target not granted us ownership See if time-out occurred.
        //
        if( GetTickCount( ) - m_TimeOfLastContact < m_SocketConnectionTimeoutValue )
        {
            //
            // No problem.
            //
            return( doingTargetCommunication );
        }

        TimeOut: ;       

        //
        // Here we have experience a time out. Set state of all non-connecting, non-idle, non-free
        // sockets to remote disconnet.
        //
        WaitForSingleObject( m_MutexHandle, INFINITE );

        for( i = 0 ; i < FW1394_CHANNEL_COUNT ; i++ )
        {
            if( (m_pSharedData->m_SocketArray[ i ].m_State != Connecting) && (m_pSharedData->m_SocketArray[ i ].m_State != Free)
                && (m_pSharedData->m_SocketArray[ i ].m_State != Idle) && (m_pSharedData->m_SocketArray[ i ].m_State != ToBeFreed) )
            {
                m_pSharedData->m_SocketArray[ i ].m_BytesInRxBuffer = 0;      
                m_pSharedData->m_SocketArray[ i ].m_BytesInTxBuffer = 0;   
                if( m_pSharedData->m_SocketArray[ i ].m_State == Connected )
                {
                    m_pSharedData->m_SocketArray[ i ].m_State = RemoteDisconnect;
                    //
                    // Send message disconnect occured.
                    //
                    PostMessage( m_pSharedData->m_SocketArray[ i ].m_hWnd, m_pSharedData->m_SocketArray[ i ].m_WndMsg,
                                 (unsigned int) m_pSharedData->m_SocketArray[ i ].m_Socket, FD_CLOSE );
   
                }
                else
                {
                    m_pSharedData->m_SocketArray[ i ].m_State = ToBeFreed;
                }
            } 
            else if( m_pSharedData->m_SocketArray[ i ].m_State == Connecting )
            {
                if( controlBlock.m_MemoryOwner == FW1394ChannelControlBlock::TARGET_RESTARTED )
                {
                    //
                    // Special case added by wng
                    //
					m_pSharedData->m_SocketArray[ i ].m_State = Idle;

                    PostMessage( m_pSharedData->m_SocketArray[ i ].m_hWnd, m_pSharedData->m_SocketArray[ i ].m_WndMsg,
                                 (unsigned int) m_pSharedData->m_SocketArray[ i ].m_Socket, FD_CLOSE );
                }
                else     
                {
                    //
                    // Set state to idle and post a message inidcating connection failed. 
                    //
                    m_pSharedData->m_SocketArray[ i ].m_State = Idle;
                    PostMessage( m_pSharedData->m_SocketArray[ i ].m_hWnd, m_pSharedData->m_SocketArray[ i ].m_WndMsg,
                                 (unsigned int) m_pSharedData->m_SocketArray[ i ].m_Socket, 0xFFFF0000 | FD_CONNECT );
                }
            }
        }

        ReleaseMutex( m_MutexHandle );
    
        m_pSharedData->m_1394DataInitialized = false;

        m_TimeOfLastContact = GetTickCount( );

        return( doingTargetCommunication );
    }

    //
    // Check sequence number. If not the same, re-read control block.
    //
    if( controlBlock.m_HeadSequenceNumber != controlBlock.m_TailSequenceNumber )
    {
        goto ReRead;
    }

 
    if( controlBlock.m_HeadSequenceNumber == g_LastSequence )
    {
        goto ReRead; 
    }
    g_LastSequence = controlBlock.m_HeadSequenceNumber;

    
    //
    // Here we have received ownership of the shared 1394 memory. 
    // Protect access during this part.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );

    //
    // Lets first read up any received data. 
    //      
    for( i = 0 ; i < FW1394_CHANNEL_COUNT ; i++ )
    {
        if( m_pSharedData->m_SocketArray[ i ].m_State == Connected )
        {
            unsigned int bytesInReceiver = controlBlock.m_TargetTransmitCount[  m_pSharedData->m_SocketArray[ i ].m_1394Channel ];
                   
            if( (bytesInReceiver != 0) && ( m_pSharedData->m_SocketArray[ i ].m_BytesInRxBuffer == 0) )  
            {
                //
                // Here we have some data to read and a place to put it. Do the read. Make sure
                // read is a multiple of 16, always rounding up as necessary.
                //
        tryAgain:
                if( m_pIRad1394Master->ReadMemory( BUS_NUMBER, NODE_NUMBER, (unsigned int) &m_p1394Memory->m_Channel[ m_pSharedData->m_SocketArray[ i ].m_1394Channel ].m_TargetTransmitBuffer,
                           RoundUpTo16( bytesInReceiver ), m_pSharedData->m_SocketArray[ i ].m_RxBuffer ) )
                {


                    if( controlBlock.m_TargetTransmitCrc[ m_pSharedData->m_SocketArray[ i ].m_1394Channel ] != 
                        MakeCrc( m_pSharedData->m_SocketArray[ i ].m_RxBuffer, bytesInReceiver ) )
                    {
                        rDebugPrintf( "Bad Crc In received detected\n");
                        goto tryAgain;
                    }

                    m_pSharedData->m_SocketArray[ i ].m_BytesInRxBuffer = bytesInReceiver;
                    controlBlock.m_TargetTransmitCount[ m_pSharedData->m_SocketArray[ i ].m_1394Channel ] = 0;
        
                    //
                    // Indicate data is available.
                    //
                    PostMessage( m_pSharedData->m_SocketArray[ i ].m_hWnd, m_pSharedData->m_SocketArray[ i ].m_WndMsg,
                                 (unsigned int) m_pSharedData->m_SocketArray[ i ].m_Socket, FD_READ );
                }
            }
        }
    }
      
    //
    // Now lets send any data that we can.
    //
    for( i = 0 ; i < FW1394_CHANNEL_COUNT ; i++ )
    {            
        if( (m_pSharedData->m_SocketArray[ i ].m_State == Connected) || (m_pSharedData->m_SocketArray[ i ].m_State == PendingDisconnect) )
        {
            unsigned int bytesInTransmitter = controlBlock.m_HostTransmitCount[ m_pSharedData->m_SocketArray[ i ].m_1394Channel ];
                   
            if( (bytesInTransmitter == 0) && (m_pSharedData->m_SocketArray[ i ].m_BytesInTxBuffer != 0) )
            {
                //
                // Here we have some data to write and a place to put it.
                //
                if( m_pIRad1394Master->WriteMemory( BUS_NUMBER, NODE_NUMBER, (unsigned int) &m_p1394Memory->m_Channel[ m_pSharedData->m_SocketArray[ i ].m_1394Channel ].m_HostTransmitBuffer,
                            RoundUpTo16( m_pSharedData->m_SocketArray[ i ].m_BytesInTxBuffer ), m_pSharedData->m_SocketArray[ i ].m_TxBuffer ) )
                {
                    controlBlock.m_HostTransmitCrc[ m_pSharedData->m_SocketArray[ i ].m_1394Channel ] =
                        MakeCrc( m_pSharedData->m_SocketArray[ i ].m_TxBuffer, m_pSharedData->m_SocketArray[ i ].m_BytesInTxBuffer );
                    controlBlock.m_HostTransmitCount[ m_pSharedData->m_SocketArray[ i ].m_1394Channel ] = 
                                m_pSharedData->m_SocketArray[ i ].m_BytesInTxBuffer;
                    m_pSharedData->m_SocketArray[ i ].m_BytesInTxBuffer = 0;

                    //
                    // Indicate transmitter is empty.
                    //
                    PostMessage( m_pSharedData->m_SocketArray[ i ].m_hWnd, m_pSharedData->m_SocketArray[ i ].m_WndMsg,
                                 (unsigned int) m_pSharedData->m_SocketArray[ i ].m_Socket, FD_WRITE );
                }
            }
        }
    }

    //
    // Now lets resolve state and process messages. First lets deal with connected or local disconnected
    // channels.
    //
    for( i = 0; i < FW1394_CHANNEL_COUNT ; i++ )
    {
        if( (m_pSharedData->m_SocketArray[ i ].m_State == Connected) || 
            (m_pSharedData->m_SocketArray[ i ].m_State == PendingDisconnect) ||
            (m_pSharedData->m_SocketArray[ i ].m_State == Disconnected) )
        {
            //
            // Here we have a connection or are about to shut one down. We have 
            // a valid index into the message table. The only valid messages from the
            // target are disconnect or null message.
            //
            unsigned int channel = m_pSharedData->m_SocketArray[ i ].m_1394Channel;
            FW1394ChannelControlBlock::ControlMessage message = controlBlock.m_MessageMap[ channel ];;
            rAssert( (message == FW1394ChannelControlBlock::NullMessage) || (message == FW1394ChannelControlBlock::DisconnectMessage ) );
 
            //
            // Lets let the target disconnect message take priority. Set our state to disconnected.
            //
            if( message == FW1394ChannelControlBlock::DisconnectMessage )
            {
                //
                // Clear the message entry, free the channel, and set our local state. 
                // 
                controlBlock.m_MessageMap[ channel ] = FW1394ChannelControlBlock::NullMessage;
                controlBlock.m_ChannelAllocationMap[ channel ] = 0;
                if( m_pSharedData->m_SocketArray[ i ].m_State == Connected )
                {
                    //
                    // Set it disconnected. The client will free the entry,
                    //
                    m_pSharedData->m_SocketArray[ i ].m_State = RemoteDisconnect;

                    PostMessage( m_pSharedData->m_SocketArray[ i ].m_hWnd, m_pSharedData->m_SocketArray[ i ].m_WndMsg,
                                 (unsigned int) m_pSharedData->m_SocketArray[ i ].m_Socket, FD_CLOSE );

                }
                else
                {
                    m_pSharedData->m_SocketArray[ i ].m_State = ToBeFreed; 
                }
            }
            else
            {
                //
                // No message from the target. Check our state. If connected, do nothing. If
                // disconnected, send disconnect message. If Pending disconnect, and tx buffer
                // empty, send disconnect message. Target frees channel when sees message
                //
                if( (m_pSharedData->m_SocketArray[ i ].m_State == Disconnected) ||
                    ( (m_pSharedData->m_SocketArray[ i ].m_State == PendingDisconnect) && (m_pSharedData->m_SocketArray[ i ].m_BytesInTxBuffer == 0) ) )
                {
                    controlBlock.m_MessageMap[ channel ] = FW1394ChannelControlBlock::DisconnectMessage;
                    m_pSharedData->m_SocketArray[ i ].m_State = ToBeFreed; 
                }                                
            }
        }  
    }

    //
    // Here we have dealt with all the established or disconnected sockets. Now lets deal with
    // the connecting sockets. Lets iterate over all allocated shared memory channels.
    //
    for( i = 0 ; i < FW1394_CHANNEL_COUNT ; i++ )
    {
        if( m_pSharedData->m_SocketArray[ i ].m_State == Connecting )
        {
            //
            // Check if we have allocated a channel. If not do so now and send the message.
            //
            if( m_pSharedData->m_SocketArray[ i ].m_1394Channel == FW1394_CHANNEL_COUNT )
            {
                //
                // Note allocated. There had better be a free channel or logic is hooped.
                //
                for( unsigned int channel = 0 ; channel < FW1394_CHANNEL_COUNT ; channel++ )
                {
                    if( controlBlock.m_ChannelAllocationMap[ channel ] == 0 )
                    {
                        //
                        // Set it allocated and send the message.
                        //
                        controlBlock.m_ChannelAllocationMap[ channel ] = 1;
                        controlBlock.m_HostTransmitCount[ channel ] = 0;
                        controlBlock.m_TargetTransmitCount[ channel ] = 0;
                        controlBlock.m_MessageMap[ channel ] = FW1394ChannelControlBlock::ConnectRequestMessage;
                        m_pSharedData->m_SocketArray[ i ].m_1394Channel = channel;

                        break;
                    }
                }
                
            }
            else
            {
                //
                // We have a channel. The response must eithor be a connect accept or reject.
                //
                unsigned int channel = m_pSharedData->m_SocketArray[ i ].m_1394Channel;
                FW1394ChannelControlBlock::ControlMessage message = controlBlock.m_MessageMap[ channel ];
                rAssert( (message == FW1394ChannelControlBlock::ConnectAcceptMessage ) ||
                         (message == FW1394ChannelControlBlock::ConnectRejectMessage ) );
            
                if( message == FW1394ChannelControlBlock::ConnectAcceptMessage )
                {
                    //
                    // Set state to connected and inform the client of the successful connect.
                    //
                    m_pSharedData->m_SocketArray[ i ].m_State = Connected;
                    controlBlock.m_MessageMap[ channel ] = FW1394ChannelControlBlock::NullMessage;

                    PostMessage( m_pSharedData->m_SocketArray[ i ].m_hWnd, m_pSharedData->m_SocketArray[ i ].m_WndMsg,
                                 (unsigned int) m_pSharedData->m_SocketArray[ i ].m_Socket, FD_CONNECT );
                }
                else
                {
                    //
                    // Message was a connect reject. Check time-out if not expired, sent another connect.
                    //
                    if( GetTickCount( ) - m_pSharedData->m_SocketArray[ i ].m_ConnectTime > CONNECTION_TIMEOUT )
                    {
                        //
                        // Set state to idle and post a message inidcating connection failed. Free channel
                        //
                        controlBlock.m_MessageMap[ channel ] = FW1394ChannelControlBlock::NullMessage;
                        controlBlock.m_ChannelAllocationMap[ channel ] = 0;
                        m_pSharedData->m_SocketArray[ i ].m_State = Idle;
                        PostMessage( m_pSharedData->m_SocketArray[ i ].m_hWnd, m_pSharedData->m_SocketArray[ i ].m_WndMsg,
                                 (unsigned int) m_pSharedData->m_SocketArray[ i ].m_Socket, 0xFFFF0000 | FD_CONNECT );
                    }
                    else
                    {
                        //
                        // Send another connect request.
                        //
                        controlBlock.m_MessageMap[ channel ] = FW1394ChannelControlBlock::ConnectRequestMessage;
                    } 
                }
            }
        }
    }

    ReleaseMutex( m_MutexHandle );

    //
    // Phew!! Everything is tiddly. Lets write the control block and transfer ownership to target.
    //
    controlBlock.m_MemoryOwner = FW1394ChannelControlBlock::TARGET_OWNS_MEMORY;

    controlBlock.m_HeadSequenceNumber = g_SequenceNumber;
    controlBlock.m_TailSequenceNumber = g_SequenceNumber;
    g_SequenceNumber++;

    controlBlock.m_Crc = 0;
    controlBlock.m_Crc = MakeCrc( &controlBlock, sizeof( controlBlock ) );
 
    //
    // Lets write the data into the shared memory.
    //
    m_pIRad1394Master->WriteMemory( BUS_NUMBER, NODE_NUMBER, (unsigned int)  &m_p1394Memory->m_ChannelControlBlock,  
                                    sizeof( controlBlock ), &controlBlock );


    m_TimeOfLastContact = GetTickCount( );

    return( doingTargetCommunication );
}

//=============================================================================
// Function:    DoTargetCommunicationCleanUp
//=============================================================================
// Description: Release the 1394 I/O resources so another thread can
//              use it.
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

void CHost1394Socket::DoTargetCommunicationCleanUp( void )
{

//    radRelease( m_pIRad1394Master, this );
//    rad1394Terminate( );   

    //
    // Indicate that some other thread should take up the torch.
    //
    m_pSharedData->m_1394ThreadActive = false;
}

//=============================================================================
// Function:    CHost1394Socket::RoundUpTo16
//=============================================================================
// Description: Member to round number up to multiple of 16
//
// Parameters:  unsinged int
//              
// Returns:     converted usinged int.
//
// Notes:       
//------------------------------------------------------------------------------

unsigned int CHost1394Socket::RoundUpTo16
( 
    unsigned int value
)
{
    if( value % 16 == 0 )
    {
        return( value );
    }
    else
    {
        return( ((value / 16) * 16) + 16 );
    }
}

//=============================================================================
// Function:    CHost1394Socket::MakeCRC
//=============================================================================
// Description: 
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

unsigned int CHost1394Socket::MakeCrc( void* pBuffer, unsigned int size )
{
    unsigned char* pData = (unsigned char*) pBuffer;
    unsigned char x;
    unsigned int value = 0;
    for( unsigned int i = 0; i < size; i++ )
    {
        x = pData[ i ];
        value = value + (unsigned int) x;   
    }
    return( value );
};