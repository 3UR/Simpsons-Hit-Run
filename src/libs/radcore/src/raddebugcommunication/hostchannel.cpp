//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        hostchannel.cpp
//
// Subsystem:   Radical Debug Communication Host
//
// Description:	This file contains the implementation of the Debug Communication
//              host channel. 
//
// Author:		PGM
//
// Revisions:	Nov 8, 2000    Creation
//
// Notes:       This implementation is for Windows only. 
//              This system is uses the DECI protocol even when we are communicating
//              using sockets. It simplyfies things a bit.
//
//=============================================================================

#include "pch.hpp"
#ifdef RAD_WIN32

//=============================================================================
// Include Files
//=============================================================================

#include <raddebug.hpp>
#include <raddebugcommunication.hpp>              
#include <radmemorymonitor.hpp>
#include "host.hpp"          
#include "hostchannel.hpp"   
#include "decitypes.h"
#include "deci2x.h"
#include "netmp.h"
#include "dcmp.h"
#include "protocol.hpp"
#include "host1394socket.hpp"

//=============================================================================
// Local Defintions
//=============================================================================

//
// This is the windows message used to define a socket event. The other is used
// to indicate DNS name lookup event. The last is an internal message we use
// to indicate that a send of data has been completed.
//
#define  WM_SOCKET              (WM_USER + 100)
#define  WM_DNSNAMELOOKUP       (WM_USER + 101)
#define  WM_SENDDATACOMPLETE    (WM_USER + 102)

//
// This is the ID of our timer used to perform time-out operations and the
// time-out value in milliseonds
//
#define  TIMER_ID           0x1234    
#define  TIMEOUT            1500000

//=============================================================================
// Static Data Defintions
//=============================================================================


//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    rDbgComHostChannel::rDbgComHostChannel
//=============================================================================
// Description: Constructor for the host channel object. Initialize things and
//              set state.
//
// Parameters:  Parent Host Object
//              index identifying target
//              protocl
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

rDbgComHostChannel::rDbgComHostChannel
( 
    rDbgComHost*    pHost, 
    unsigned int    targetIndex,
    unsigned short  protocol
)
    :
    m_ReferenceCount( 1 ),
    m_TargetIndex( targetIndex ),
    m_Protocol( protocol ),
    m_ConnectionState( Detached ),
    m_StatusCallback( NULL ),
    m_Socket( NULL ),
    m_DNSLookUpHandle( NULL ),
    m_InternalState( Idle ),
    m_TimerId( 0 ),
    m_ClientReceiveCallback( NULL ),
    m_ClientReceiveSize( 0 ),
    m_TxBlocked( false ),
    m_PendingBytesToSend( 0 ),
    m_ClientSendCallback( NULL ),
    m_ClientSendSize( 0 ),
    m_UsingDNSLookup( true )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "rDbgComHostChannel" );
    //
    // Save reference to parent host. Must update reference as we are holding it,
    //
    m_ParentHost = pHost;
    radAddRef( m_ParentHost, this );

    //
    // Construct a window to receive notification of network events from 
    // the underlying sockets.
    //
    m_hWnd = m_ParentHost->CreateWindowHelper( this );

    //
    // Set the initial info buffer string
    //
    strcpy( m_TextInfoBuffer, "Detached" );

    //
    // Lets select the use of sockets using the IP address.
    //
    if( INADDR_NONE == inet_addr( m_ParentHost->GetIpAddress( m_TargetIndex ) ) )
    {   
        //
        // For now, it the name is FireWire, then we use the 1394 socket implementation.
        //
        if( 0 == stricmp( m_ParentHost->GetIpAddress( m_TargetIndex ), "FireWire" ) )
        {
            char* p = new char[ sizeof( CHost1394Socket ) ];
            m_SocketImp = new( p ) CHost1394Socket( );
            // do not lookup DNS for firewire
            m_UsingDNSLookup = false;
        
            return;
        }

    }
            
    //
    // Here we use a real socket implemetaiton. Construct it usign placement new
    // so don't link in the memory allocator.
    //
    char* p = new char[ sizeof( radSocket ) ];
    m_SocketImp = new( p ) radSocket( );

}

//=============================================================================
// Function:    rDbgComHostChannel::~rDbgComHostChannel
//=============================================================================
// Description: Destructor for the host channel object. Simply unintialize. 
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

rDbgComHostChannel::~rDbgComHostChannel( void )
{
    if( m_ConnectionState != Detached )
    {
        Detach( );
    }

    m_ParentHost->DestroyWindowHelper( m_hWnd );

    m_ParentHost->FreeProtocol( m_TargetIndex, m_Protocol );

    radRelease( m_ParentHost, this );

    //
    // Free teh socket implementation directly so as not to invoke the radMemory stuff.
    //
    m_SocketImp->~radSocket( );
    
    delete [ ] ((char*) m_SocketImp);

}

//=============================================================================
// Function:    rDbgComHostChannel::RegisterStatusCallback
//=============================================================================
// Description: This routine allows the caller to register a callback which is
//              invoked any time a status chage occurs.
//
// Parameters:  Callback to invoke
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComHostChannel::RegisterStatusCallback
( 
    IRadDbgComChannelStatusCallback* pCallback
)
{
    //
    // Simply save the address.
    //
    m_StatusCallback = pCallback;

}

//=============================================================================
// Function:    rDbgComHostChannel::GetStatus
//=============================================================================
// Description: Use this routine to poll for connection status changes.
//
// Parameters:  Connection status
//              Optional buffer to receive text message
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComHostChannel::GetStatus
( 
    ConnectionState* pConnectionState, 
    char* pMessage
)
{
    *pConnectionState = m_ConnectionState;
    if( pMessage != NULL )
    {
        strcpy( pMessage, m_TextInfoBuffer );
    }
}

//=============================================================================
// Function:    rDbgComHostChannel::Attach
//=============================================================================
// Description: This routine is invoked to initiate the connection with the 
//              target.
//
// Parameters:  n/a
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComHostChannel::Attach( void )
{
    int result;
    //
    // Check the state. If not detached, then we simply return.
    //
    if( m_ConnectionState != Detached )
    {
        return;
    }

    //  
    // Our internal state must be idle..
    //
    rAssert( m_InternalState == Idle );

    //
    // Lets start the connection process. First lets create a socket for communication.
    //
    m_Socket = m_SocketImp->socket(PF_INET, SOCK_STREAM, 0 );  
    rAssert( m_Socket != INVALID_SOCKET ); 

    //  
    // Set socket option to linger so unsent data is sent when we close.
    //
    int DontLinger = 1;
    result = m_SocketImp->setsockopt( m_Socket, SOL_SOCKET, SO_DONTLINGER, (const char*) &DontLinger, sizeof( int ) );
    rAssert( result == 0 );    
 
    //  
    // Set socket option to send keep alive messages
    //
    int KeepAlive = 1;
    result = m_SocketImp->setsockopt( m_Socket, SOL_SOCKET, SO_KEEPALIVE, (const char*) &KeepAlive, sizeof( int ) );
    rAssert( result == 0 );    
 
    //
    // Bind to the socket.
    //
	memset( &m_SockAddr, 0, sizeof(m_SockAddr) );
	m_SockAddr.sin_family = AF_INET;
    m_SockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_SockAddr.sin_port = htons(0);

    result = m_SocketImp->bind( m_Socket, (struct sockaddr*) &m_SockAddr, sizeof( m_SockAddr ) );
    rAssert( result != SOCKET_ERROR );

    //
    // Set up the socket to non-blocking and to receive Windows messages for network
    // events.
    //
    result = m_SocketImp->AsyncSelect( m_Socket, m_hWnd, WM_SOCKET, FD_READ | FD_WRITE | FD_CLOSE | FD_CONNECT );
    rAssert( result != SOCKET_ERROR );
    
    //
    // Now get the address from our target table and initiate the connection.   
    //
	memset( &m_SockAddr, 0, sizeof(m_SockAddr) );
	m_SockAddr.sin_family = AF_INET;
	m_SockAddr.sin_addr.s_addr = inet_addr( m_ParentHost->GetIpAddress( m_TargetIndex ) );

    // 
    // Check if valid ip address. If not we assume it is a DNS name and we must look it up. We 
    // perform this operation asychronously.
    //
	if( m_SockAddr.sin_addr.s_addr == INADDR_NONE && m_UsingDNSLookup )
	{
        //
        // Issue the request to find the name.
        //
        m_DNSLookUpHandle = WSAAsyncGetHostByName( m_hWnd, WM_DNSNAMELOOKUP, m_ParentHost->GetIpAddress( m_TargetIndex ),
                                                   m_NameLookBuffer, sizeof( m_NameLookBuffer ) );

        rAssert( m_DNSLookUpHandle != 0 );
                  
        //
        // We are done for now. Update state and return.
        //
        SetState( Attaching, DNSLoopUp, "Looking up target IP address ...");
    }
    else
    {
        if( ! m_UsingDNSLookup )
        {
            m_SockAddr.sin_addr.s_addr = (unsigned long) m_ParentHost->GetIpAddress( m_TargetIndex );
        }    

        //
        // Here we have a valid IP address. Lets try to connect. Must set port.
        //
        m_SockAddr.sin_port = htons( m_ParentHost->GetPort( m_TargetIndex ) );     

        result = m_SocketImp->connect( m_Socket, (const struct sockaddr*) &m_SockAddr, sizeof( m_SockAddr ) );
        rAssert( (result == SOCKET_ERROR) && (WSAEWOULDBLOCK == m_SocketImp->lasterror( m_Socket ) ));

        //
        // We are done for now. Update state and return.
        //
        SetState( Attaching, Connecting, "Connecting to target ...");
    }
}

//=============================================================================
// Function:    rDbgComHostChannel::Detach
//=============================================================================
// Description: This routine is invoked to detach the channel.
//
// Parameters:  n/a
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComHostChannel::Detach( void )
{
    //
    // If already detached, do nothing.
    //
    if( m_ConnectionState == Detached )
    {
        return;
    }

    //
    // Check the internal state. If we are looking up the DNS name, clean up.
    //
    if( m_InternalState == DNSLoopUp )
    {
        WSACancelAsyncRequest( m_DNSLookUpHandle );
        m_DNSLookUpHandle = NULL;
    
        m_SocketImp->closesocket(m_Socket);
        m_Socket = NULL;

        SetState( Detached, Idle, "Detached" );
        return;
    }
    else
    {
        //
        // Lets send a kill message to free our protocol.
        //
        unsigned int bytesToSend = MakeNETMPKillPacket( m_TxBuffer, sizeof(m_TxBuffer), m_Protocol );       

        //
        // If this send fails, not to big a deal, as we are shutting down,
        //
        m_SocketImp->send( m_Socket, (const char*) m_TxBuffer, bytesToSend, 0 );

        //
        // Clean things up.
        //
        m_SocketImp->closesocket(m_Socket);
        m_Socket = NULL;
   
        if( m_TimerId != 0 )
        {
            KillTimer( m_hWnd, TIMER_ID );
            m_TimerId = 0;
        }

        SetState( Detached, Idle, "Detached" );
    }
}

//=============================================================================
// Function:    rDbgComHostChannel::ReceiveAsync
//=============================================================================
// Description: This routine is invoked to issue a receive. This system is
//              simplistic and is packet based. It expects that the caller 
//              have a receive buffer outstanding if data is expected and that
//              the buffer be large enough to handle the request.
//
// Parameters:  pBuffer - where to copy data
//              numBytes - size of the buffer
//              callback - routine to invoke when data arrives.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComHostChannel::ReceiveAsync
(   
    void*                           pBuffer,
    unsigned int                    numBytes,
    IRadDbgComChannelReceiveCallback* callback
)
{
    //
    // First assert that a receive has not already been issued on this channel.
    // and that the callers has provided a callback.
    rAssert( m_ClientReceiveCallback == NULL );
    rAssert( callback != NULL );

    //
    // Check the connection state. If not attached, fail receive with an error 
    // and print warning message.
    //
    if( m_ConnectionState != Attached )
    {
        callback->OnReceiveComplete( false, 0 );
        rDebugString( "Warning: rDbgCom cannot issue receive when not attached\n");
        return;
    }

    //
    // Save the callers data and return.
    //
    m_ClientReceiveCallback = callback;

    m_ClientReceiveCallback->AddRef( );

    m_ClientReceiveSize = numBytes;
    m_ClientReceiveBuffer = (unsigned char*) pBuffer;
}

//=============================================================================
// Function:    rDbgComHostChannel::SendAsync
//=============================================================================
// Description: This routine is invoked to issue a send. This system is
//              simplistic and is packet based. 
//
// Parameters:  pBuffer - where to get data
//              numBytes - size of the buffer
//              callback - routine to invoke when data sent
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComHostChannel::SendAsync
( 
    void*                        pBuffer, 
    unsigned int                 numBytes,
    IRadDbgComChannelSendCallback* callback
)
{
    //
    // First assert that a send has not already been issued on this channel.
    // and that the callers has provided a callback.
    rAssert( m_ClientSendCallback == NULL );
    rAssert( callback != NULL );

    //
    // Check the connection state. If not attached, fail semd with an error 
    // and print warning message.
    //
    if( m_ConnectionState != Attached )
    {
        callback->OnSendComplete( false );
        rDebugString( "Warning: rDbgCom cannot issue send when not attached\n");
        return;
    }

    //
    // Save the callers data
    //
    m_ClientSendCallback = callback;
    m_ClientSendCallback->AddRef( );
    
    m_ClientSendSize = numBytes;
    m_ClientSendBuffer = (unsigned char*) pBuffer;

    //  
    // First check if a previous transmission has been blocked. If so
    // we are done. When we are told to resume, things will continue.
    //
    if( !m_TxBlocked )
    {
        //
        // Here we are not blocked. Partion the send into the block size 
        // and initiate the send.
        //
        unsigned int bytesToSend;
        if( m_ClientSendSize > rDbgComMaxDataPacketSize )
        {
            bytesToSend = rDbgComMaxDataPacketSize;
        }
        else
        {
            bytesToSend = m_ClientSendSize;
        }

        //
        // Lets build up the send packet. We need to make a copy since we
        // may be informed of a blocked send after we have completed sending
        // the data.
        //
        m_PendingBytesToSend = MakeDataPacket( m_TxBuffer, sizeof(m_TxBuffer), m_Protocol, bytesToSend, m_ClientSendBuffer );       
        m_CurrentSendAddress = m_TxBuffer;
            
        //
        // Update the amount of client data to send and its pointer.
        //
        m_ClientSendSize -= bytesToSend;
        m_ClientSendBuffer += bytesToSend;

        //
        // Lets initiate the sending of the data.
        //
        m_LastSendSize = m_SocketImp->send( m_Socket, (const char*) m_TxBuffer, m_PendingBytesToSend, 0 );

        if( m_LastSendSize < 0 )
        {
            if( WSAEWOULDBLOCK != m_SocketImp->lasterror( m_Socket ) )
            {
                //
                // Failure to send. Close socket and reset state.
                //
                m_PendingBytesToSend = 0;
 
                //
                // Treat failure as if socket closed.
                //
                OnClose( 0 );
            }
            return;
        }

        //
    	// Adjust the remaining number of bytes to be sent next
        //    
        m_PendingBytesToSend -= m_LastSendSize;
        m_CurrentSendAddress += m_LastSendSize;

    	if( m_PendingBytesToSend == 0 )
        {
            //
            // Success. Lets post ourself a send data complete event. Allows callback
            // to be invoked off of another thread.
            //
            PostMessage( m_hWnd, WM_SENDDATACOMPLETE, m_Socket, NULL );
        
        }
    }           
}

//=============================================================================
// Function:    rDbgComHostChannel::OnDNSNameLoopUp
//=============================================================================
// Description: This routine is invoked when we receive the asych notification
//              of the name loop up.
//
// Parameters:  lookup handle
//              result of the lookup.
//              
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComHostChannel::OnDNSNameLoopUp
( 
    HANDLE       dnsLookUpHandle,
    unsigned int result
)
{
    //
    // Check the handle is correct. We clear the handle to indicate
    // we are finished.
    //
    if( dnsLookUpHandle != m_DNSLookUpHandle )
    {
        //
        // Here we must have an old message. Not problem, just ignore it.
        //
        return;
    }
    
    m_DNSLookUpHandle = NULL;
       
    if( result != 0 )
    {   
        //
        // In this case, the name was not found. We do not retry to connect as this
        // would be dumb. Update state and we are finished.
        //
        m_SocketImp->closesocket(m_Socket);
        m_Socket = NULL;

        SetState( Detached, Idle, "Failed to locate target name");
    }
    else
    {
        //
        // We successfully got the IP name. Initiate the connection.
        //
        LPHOSTENT lphost = (LPHOSTENT) m_NameLookBuffer;
    	memset( &m_SockAddr, 0, sizeof(m_SockAddr) );
	    m_SockAddr.sin_family = AF_INET;
    	m_SockAddr.sin_addr.s_addr = ((LPIN_ADDR)lphost->h_addr)->s_addr;
        m_SockAddr.sin_port = htons( m_ParentHost->GetPort( m_TargetIndex ) );     
                
        result = m_SocketImp->connect( m_Socket, (const struct sockaddr*) &m_SockAddr, sizeof( m_SockAddr ) );
        rAssert( (result == SOCKET_ERROR) && (WSAEWOULDBLOCK == WSAGetLastError( ) ));

        //
        // We are done for now. Update state and return.
        //
        SetState( Attaching, Connecting, "Connecting to target ...");
    }
}        

//=============================================================================
// Function:    rDbgComHostChannel::OnConnectComplete
//=============================================================================
// Description: This routine is invoked when we receive the asych notification
//              of a connection complete.
//
// Parameters:  result of connect
//              
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComHostChannel::OnConnectComplete( unsigned int result )
{
    //
    // Check the result. If error, we try again to connect. 
    //
    if( result != 0 )
    {
        //
        // Here we failed. Lets attempt to connect again. Address is already
        // set up and state is correct. May refine this reconnection in the future.
        //
        int result = m_SocketImp->connect( m_Socket, (const struct sockaddr*) &m_SockAddr, sizeof( m_SockAddr ) );
    }
    else
    {
        //
        // Here we have successfully connected. Lets initiate a connection. Send
        // a connect message. We simulate this protocol even when we are not using the
        // Deci system.
        //  
        unsigned int bytesToSend = MakeNETMPConnectPacket( m_TxBuffer, sizeof(m_TxBuffer), m_Protocol );       
         
        //
        // Send the connect packet.
        //
        if( bytesToSend != (unsigned int) SendHelper( m_Socket, (const char*) m_TxBuffer, bytesToSend, 0 ) )
        {
            //
            // Failure to send. Close socket and reset state.
            //
            m_SocketImp->closesocket(m_Socket);
            m_Socket = NULL;

            SetState( Detached, Idle, "Failed to send connect to target");
        }
        else
        {
            //
            // Set up the receiver in readiness for the reply.
            //
            m_CurrentBytesToRead = sizeof( DECI2_HDR );
            m_CurrentReadAddress = m_RxBuffer;
            m_ReadingHeader = true;

            //
            // Start a timer here as this may fail if we have connected the wrong 
            // target.
            //
            m_TimerId = SetTimer( m_hWnd, TIMER_ID, TIMEOUT, NULL );  

            SetState( Attaching, WaitForNETMPConnectResponse, "Waiting for NETMP connect response ...");
        }          
    }
}

//=============================================================================
// Function:    rDbgComHostChannel::OnReadyToRead
//=============================================================================
// Description: This routine is invoked when we receive the asych notification
//              of data ready to be read.
//
// Parameters:  result of the read.
//              
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComHostChannel::OnReadyToRead( unsigned int result )
{
    //
    // If not zero, not sure what to do, Just ignore.
    //
    if( result != 0 )
    {
        return;

    }

    //
    // Check if we are currently reading the packet header.
    //
    if( m_ReadingHeader )
    {
        //
        // Here we are waiting to receive the packet header. Attempt to read the remaining
        // bytes.
        //
        int bytesRead = m_SocketImp->recv( m_Socket, (char*) m_CurrentReadAddress, m_CurrentBytesToRead, 0 );
        if( bytesRead <= 0 )
        {
            return;
        }
        
        //
        // Update receive buffer and count.
        //
        m_CurrentBytesToRead -= bytesRead;
        m_CurrentReadAddress += bytesRead;
        
        if( m_CurrentBytesToRead != 0 )
        {
            return;
        }

        //
        // Finished reading the header. Set up the current bytes remaining and read these.
        //
        m_ReadingHeader = false;
        rAssert( sizeof(m_RxBuffer) >= ((DECI2_HDR*) m_RxBuffer)->length );
            
        m_CurrentBytesToRead = ((DECI2_HDR*) m_RxBuffer)->length - sizeof(DECI2_HDR) ;
    }

    //
    // Try to read some data. May have jsut read the header but not a problem.
    //
    int bytesRead = m_SocketImp->recv( m_Socket, (char*) m_CurrentReadAddress, m_CurrentBytesToRead, 0 );
    if( bytesRead <= 0 )
    {
        return;
    }
        
    //
    // Update receive buffer and count.
    //
    m_CurrentBytesToRead -= bytesRead;
    m_CurrentReadAddress += bytesRead;
        
    if( m_CurrentBytesToRead != 0 )
    {
        //
        // Not finished, just exit.
        //
        return;
    }
      
    //
    // Here we have finished reading the packet. Lets reset the receiver before we forget.
    //
    m_CurrentBytesToRead = sizeof( DECI2_HDR );
    m_CurrentReadAddress = m_RxBuffer;
    m_ReadingHeader = true;
    
    //
    // Lets find out what the packet is and act accordingly.
    //
    if( ((DECI2_HDR*) m_RxBuffer)->protocol == m_Protocol )
    {
        //
        // Here we know the message is our internal protocol. This message will
        // either be actual data or the connect request. Based on the state
        // we can infer if this an internal connect or an actual data packet.
        //
        if( m_InternalState == WaitForConnectMessage )
        {
            //
            // Here we have received a connect message. Make sure the command is correct.
            //
            rDbgComConnectRequestPacket* pConnect = (rDbgComConnectRequestPacket*) &m_RxBuffer[ sizeof( DECI2_HDR ) ];
            if( pConnect->m_Command == CmdConnectRequest )
            {
                //
                // Now lets build up the connect reply packet. Send it.
                //
                m_LastSendSize = MakeInternalConnectReplyPacket( m_TxBuffer, sizeof(m_TxBuffer), m_Protocol );       
            
                //
                // Send the connect packet.
                //
                if( m_LastSendSize != SendHelper( m_Socket, (const char*) m_TxBuffer, m_LastSendSize, 0 ) )
                {
                    //
                    // Failure to send. Close socket and reset state.
                    //
                    m_SocketImp->closesocket(m_Socket);
                    m_Socket = NULL;

                    SetState( Detached, Idle, "Failed to send connect reply to target");
                }
                else
                {
                    //
                    // Life is good. We are attached.
                    //
                    SetState( Attached, Ready, "Connected");
                }            
            }
        }
        else
        {                   
            //
            // Check if an internal disconnect. 
            //
            rDbgComDisconnectRequestPacket* pDisconnect = (rDbgComDisconnectRequestPacket*) &m_RxBuffer[ sizeof( DECI2_HDR ) ];
            rDbgComConnectRequestPacket* pConnect = (rDbgComConnectRequestPacket*) &m_RxBuffer[ sizeof( DECI2_HDR ) ];
            
            if( pDisconnect->m_Command == CmdDisconnectRequest )
            {
                m_TxBlocked = false;

                SetState( Attaching, WaitForConnectMessage, "Target Detached: Waiting for connect message ...");
            }
            else if( pConnect->m_Command == CmdConnectRequest )
            {
                //
                // Got a connect request, when we are not ready. Just ignore it.
                //
    
            }
            else
            {        

                //
                // Here we have an actual data pakcet. Lets make sure it is one.
                //
                rDbgComDataPacket* pData = (rDbgComDataPacket*) &m_RxBuffer[ sizeof( DECI2_HDR ) ];
                rAssert( pData->m_Command == CmdDataPacket );

                //
                // The system expects that the caller has a receiver. If he doesn't we assert.
                //
                unsigned int dataIndex = 0;
                while( pData->m_DataSize > 0 )
                {
                    rAssertMsg( m_ClientReceiveCallback != NULL, "Data received and not receive present\n");
                
                    unsigned int bytesCopied;
                    if( pData->m_DataSize >= m_ClientReceiveSize )
                    {
                        bytesCopied = m_ClientReceiveSize;
                    }
                    else
                    {
                        bytesCopied = pData->m_DataSize;
                    }

                    memcpy( m_ClientReceiveBuffer, &pData->m_Data[ dataIndex ], bytesCopied );
                
                    //
                    // Update things.
                    //
                    dataIndex += bytesCopied;
                    pData->m_DataSize -= bytesCopied;
                    m_ClientReceiveSize -= m_ClientReceiveSize;
        
                    //
                    // We can invoked the callback. We have eithor filled the callers
                    // buffer or have process all of the data in the received buffer.
                    // Invoke the receive callback. Make a copy and null the old one 
                    // because we will likely get a new receive in this callback.
                    //
                    IRadDbgComChannelReceiveCallback* pCallback = m_ClientReceiveCallback;
                    m_ClientReceiveCallback = NULL;
                    
                    pCallback->OnReceiveComplete( true, bytesCopied );
                    pCallback->Release( );
                }
            }
        }
    
    }
    else
    {
        //
        // Here we have one of the other protocol packets. Lets see what it is and process accordingly.
        //
        switch( ((DECI2_HDR*) m_RxBuffer)->protocol )
        {
            case DECI2_PROTO_DCMP :
            {
                //
                // Here we have a message from the DCMP system. Check out what kind of message.
                // Note that these messages are never received when we are taking to a target
                // not using DECI. We are only interested in two messsags, the lock and unlock
                // messages. These are send to us when we have performed a send and the system
                // is locked by the debugger.
                //
                DCMP_HDR* pDcmpHdr = (DCMP_HDR*) &m_RxBuffer[ sizeof( DECI2_HDR ) ];
 
                if((pDcmpHdr->type == DCMP_TYPE_ERROR) && (pDcmpHdr->code == DCMP_CODE_LOCKED ))
                {
                    //
                    // Informed of send error. Set flag indicating transmission blocked.
                    //
                    m_TxBlocked = true;
                }
                else if((pDcmpHdr->type == DCMP_TYPE_STATUS) && (pDcmpHdr->code == DCMP_CODE_UNLOCKED)) 
                {
                    //
                    //  Here we have been informed of an unlock. Re-send what we last sent.
                    //
                    if( m_TxBlocked )                       
                    {
                        m_TxBlocked = false;                              

                        if( m_LastSendSize != SendHelper( m_Socket, (const char*) m_TxBuffer, m_LastSendSize, 0 ) )
                        {
                            //
                            // Failure to send. Close socket and reset state.
                            //
                            OnClose( 0 );
                        }
                        else
                        {
                            //
                            // Success. Lets post ourself a send event. This allows us to invoke callback
                            // off of this thread.
                            //
                            PostMessage( m_hWnd, WM_SENDDATACOMPLETE, m_Socket, NULL );
                        }
                    }                    
                                   
                }    
                
                break;
            }

            case DECI2_PROTO_NETMP :
            {
                //
                // Here we have been informed of a NETMP message. Check out what it is. Note
                // that we will not receive some of these messages if we are not talking to
                // the actual Deci Manager.
                //
                NETMP_HDR* pDcmpHdr = (NETMP_HDR*) &m_RxBuffer[ sizeof( DECI2_HDR ) ];
                    
                if( pDcmpHdr->code == NETMP_CODE_RESETR )
                {
                    //
                    // Informed of a reset. This will only be sent by a real PS2 tool.
                    // Here we reset things. If we have a receive, cancel it. If we have 
                    // a complete it.
                    //
                    m_TxBlocked = false;

                    SetState( Attaching, WaitForConnectMessage, "Reset: Waiting for connect message ...");

                }
                else if( (pDcmpHdr->code == NETMP_CODE_CONNECTR) && (m_InternalState == WaitForNETMPConnectResponse ) )
                {
                    //
                    // Here we have a connect response and we are waiting for one. If we get one and not waiting
                    // for it, just ignore it. Lets see if connection was successful.
                    // We can kill our timer, since we got back the reply.
                    //
                    KillTimer( m_hWnd, TIMER_ID );
                    m_TimerId = 0;

                    if( pDcmpHdr->result == NETMP_ERR_OK )
                    {
                        //
                        // Here we have received our response to the DCMP connect. We are now waiting
                        // for our internal connect message. Send a string idenfifying ourself. Don't care
                        // about response. 
                        //
                        unsigned int bytesToSend = MakeNETMPMessagePacket( m_TxBuffer, sizeof(m_TxBuffer) );       
         
                        //
                        // Send the Message packet.
                        //
                        if( bytesToSend != (unsigned int) SendHelper( m_Socket, (const char*) m_TxBuffer, bytesToSend, 0 ) )
                        {
                            //
                            // Failure to send. Close socket and reset state.
                            //
                            m_SocketImp->closesocket(m_Socket);
                            m_Socket = NULL;

                            SetState( Detached, Idle, "Failed to send connect to target");
                        }
                        else
                        {
                            //
                            // We have no time-out here, since we do not know when the actual target
                            // app will start.
                            //
                            SetState( Attaching, WaitForConnectMessage, "Waiting for connect message");       
                        }
                    }
                    else
                    {
                        //
                        // Failure. Lets find out who is attached.
                        //                                                                       
                        unsigned int bytesToSend = MakeNETMPStatusPacket( m_TxBuffer, sizeof(m_TxBuffer) );       

                        //
                        // Send the Message packet.
                        //
                        if( bytesToSend != (unsigned int) SendHelper( m_Socket, (const char*) m_TxBuffer, bytesToSend, 0 ) )
                        {
                            //
                            // Failure to send. Close socket and reset state.
                            //
                            m_SocketImp->closesocket(m_Socket);
                            m_Socket = NULL;

                            SetState( Detached, Idle, "Failed to send status request to target");
                        }
                        else
                        {
                            //
                            // Start the timer.
                            //
                            m_TimerId = SetTimer( m_hWnd, TIMER_ID, TIMEOUT, NULL );  

                            SetState( Attaching, WaitForNETMPStatusResponse, "Waiting for status response");       
                        }

                    }

                }              
                else if( (pDcmpHdr->code == NETMP_CODE_STATUSR) && (m_InternalState == WaitForNETMPStatusResponse ) )
                {
                    //
                    // Stop the timer.
                    //
                    KillTimer( m_hWnd, TIMER_ID );
                    m_TimerId = 0;

                    //
                    // Lets build up a string which contains the name of the user currently attached.
                    //
                    char MessageBuffer[ 200 ];
                    wsprintf( MessageBuffer, "Detached. Target already in use by: %s", &(m_RxBuffer[ sizeof( DECI2_HDR ) + sizeof( NETMP_HDR ) + 10 ]) );

                    SetState( Detached, Idle, MessageBuffer );       
                    
                    //
                    // We close the socket in this case since things are in bad shape.

                    m_SocketImp->closesocket(m_Socket);
                    m_Socket = NULL;

                }        
                else if( pDcmpHdr->code == NETMP_CODE_MESSAGER )
                {
                    //
                    // Here we have received our message response. Lets just send
                    // a "ping message to the target. The DECI target is stupid and
                    // may not even know that we were gone. Only do so if we have not received
                    // a connect message.
                    //
                    if( m_InternalState != Ready )
                    {
                        unsigned int bytesToSend = MakeInternalReconnectPacket( m_TxBuffer, sizeof(m_TxBuffer), m_Protocol );       
         
                        //
                        // Send the Message packet.
                        //
                        if( bytesToSend != (unsigned int) SendHelper( m_Socket, (const char*) m_TxBuffer, bytesToSend, 0 ) )
                        {
                            //
                            // Failure to send. Close socket and reset state.
                            //
                            m_SocketImp->closesocket(m_Socket);
                            m_Socket = NULL;

                            SetState( Detached, Idle, "Failed to send reconnect to target");
                        }
                    }
                }
                else 
                {
                    rDebugString( "Unexpected message received");
                }

                break;
            } 

            default:
            {
                rAssert( false );
                break;
            }
        }
    }

}

//=============================================================================
// Function:    rDbgComHostChannel::OnReadyToWrite
//=============================================================================
// Description: This routine is invoked when we are free to send.
//
// Parameters:  result of connect
//              
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComHostChannel::OnReadyToWrite( unsigned int result )
{
    //
    // Check if we have pending bytes to send. If so do so at this time.
    //
    if( m_PendingBytesToSend != 0 )
    {
        //
        // Lets initiate the sending of the data.
        //
        m_LastSendSize = (unsigned int) m_SocketImp->send( m_Socket, (const char*) m_CurrentSendAddress, m_PendingBytesToSend, 0 );

        if( m_LastSendSize < 0 )
        {
            if( WSAEWOULDBLOCK != m_SocketImp->lasterror( m_Socket ) )
            {
                //
                // Failure to send. Close socket and reset state.
                //
                m_PendingBytesToSend = 0;
 
                //
                // Treat failure as if socket closed.
                //
                OnClose( 0 );
            }
            return;
        }

        //
    	// Adjust the remaining number of bytes to be sent next
        //    
        m_PendingBytesToSend -= m_LastSendSize;
        m_CurrentSendAddress += m_LastSendSize;

    	if( m_PendingBytesToSend == 0 )
        {
            //
            // Success. Lets post ourself a send data complete event. Allows callback
            // to be invoked off of another thread.
            //
            PostMessage( m_hWnd, WM_SENDDATACOMPLETE, m_Socket, NULL );
        
        }
    }           
            
}

//=============================================================================
// Function:    rDbgComHostChannel::OnSendCompete
//=============================================================================
// Description: This routine is invoked when a send of data has been completed. 
//
// Parameters:  n/a
//              
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComHostChannel::OnSendComplete( void )
{
    //
    // Check state. If connected and we have a callback, then see if we
    // are finished send.
    //
    if( (m_InternalState == Ready) && (m_ClientSendCallback != NULL ) ) 
    {
        //
        // Check if we have finished the send.
        //
        if( m_ClientSendSize == 0 )
        {
             IRadDbgComChannelSendCallback* pcallback = m_ClientSendCallback;
             m_ClientSendCallback = NULL;
             pcallback->OnSendComplete( true );
             pcallback->Release( );
        }
        else if( !m_TxBlocked && (m_PendingBytesToSend == 0 ))
        {
            //
            // Here we are not blocked. Partion the send into the block size 
            // and initiate the send.
            //
            unsigned int bytesToSend;
            if( m_ClientSendSize > rDbgComMaxDataPacketSize )
            {
                bytesToSend = rDbgComMaxDataPacketSize;
            }
            else
            {      
                bytesToSend = m_ClientSendSize;
            }

            //
            // Lets build up the send packet. We need to make a copy since we
            // may be informed of a blocked send after we have completed sending
            // the data.
            //
            m_PendingBytesToSend = MakeDataPacket( m_TxBuffer, sizeof(m_TxBuffer), m_Protocol, bytesToSend, m_ClientSendBuffer );
            m_CurrentSendAddress = m_TxBuffer;

            //
            // Update the amount of client data to send and its pointer.
            //
            m_ClientSendSize -= bytesToSend;
            m_ClientSendBuffer += bytesToSend;

            //
            // Lets initiate the sending of the data.
            //
            m_LastSendSize = (unsigned int) m_SocketImp->send( m_Socket, (const char*) m_CurrentSendAddress, m_PendingBytesToSend, 0 );

            if( m_LastSendSize < 0 )
            {
                if( WSAEWOULDBLOCK != m_SocketImp->lasterror( m_Socket ) )
                {
                    //
                    // Failure to send. Close socket and reset state.
                    //
                    m_PendingBytesToSend = 0;
 
                    //
                    // Treat failure as if socket closed.
                    //
                    OnClose( 0 );
                }
                return;
            }

            //
    	    // Adjust the remaining number of bytes to be sent next
            //    
            m_PendingBytesToSend -= m_LastSendSize;
            m_CurrentSendAddress += m_LastSendSize;

    	    if( m_PendingBytesToSend == 0 )
            {
                //
                // Success. Lets post ourself a send data complete event. Allows callback
                // to be invoked off of another thread.
                //
                PostMessage( m_hWnd, WM_SENDDATACOMPLETE, m_Socket, NULL );
        
            }
        }   
    }                 
            
}

//=============================================================================
// Function:    rDbgComHostChannel::OnClose
//=============================================================================
// Description: This routine is invoked when a close is received.
//
// Parameters:  result of connect
//              
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComHostChannel::OnClose( unsigned int result )
{
    //
    // Don't care about the result.
    //
    (void) result;

    //  
    // If we are state is detached, then do nothing.
    //
    if( m_ConnectionState == Detached)
    {
        return;
    }

    //
    // Set the state.
    //
    SetState( Attaching, Idle, "Connection with target closed");
             
    //
    // Clean up things.
    //
    if( m_InternalState == DNSLoopUp )
    {
        WSACancelAsyncRequest( m_DNSLookUpHandle );
        m_DNSLookUpHandle = NULL;
    }
    
    m_SocketImp->closesocket(m_Socket);
    m_Socket = NULL;
   
    if( m_TimerId != 0 )
    {
        KillTimer( m_hWnd, TIMER_ID );
        m_TimerId = 0;
    }

    //
    // Lets begin the attach process again.
    //
    m_ConnectionState = Detached;
    Attach( );
}

//=============================================================================
// Function:    rDbgComHostChannel::OnTimerExpired
//=============================================================================
// Description: This member is invoked when a timer expires.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComHostChannel::OnTimerExpired( void )
{
    //
    // If the timer is zero, we have killed it. Just ignore the message.
    //
    if( m_TimerId == 0 )
    {
        return;
    }
    
    //
    // Here we have a time-out. Close socket and set state. 
    //
    KillTimer( m_hWnd, TIMER_ID );
    m_TimerId = 0;
    
    m_SocketImp->closesocket(m_Socket);
    m_Socket = NULL;

    SetState( Detached, Idle, "Response timeout occurred.");
}

//=============================================================================
// Function:    rDbgComHostChannel::OnWindowMessage
//=============================================================================
// Description: This routine is invoked when wee receive a window message.
//
// Parameters:  see windows
//              
// Returns:     see windows
//
// Notes:
//------------------------------------------------------------------------------

LRESULT rDbgComHostChannel::OnWindowMessage
( 
    HWND    hwnd, 
    UINT    uMsg, 
    WPARAM  wParam, 
    LPARAM  lParam 
)
{
    LRESULT result;

    //
    // Simply find check the message and behave accordingly,
    //
    switch( uMsg )
    {
        case WM_DNSNAMELOOKUP :
        {
            //
            // Here we have been informed that the DNS name look up completed.
            // Invoke handler.
            //
            OnDNSNameLoopUp( (HANDLE) wParam, WSAGETASYNCERROR( lParam ) );
            result = 0;
            break;
        }
        
        case WM_SOCKET :
        {
            //
            // Here we have a socket message. Determine what event we are being
            // informed on and invoke appropriate handler. If the socket is
            // not correct, we assume this was a spurious message for some 
            // pending Windows Message.
            //
            if( (SOCKET) wParam == m_Socket )
            {
                switch( WSAGETSELECTEVENT( lParam ) )
                {
                    case FD_READ :
                    {
                        OnReadyToRead( WSAGETSELECTERROR(lParam) );
                        break;
                    };
                    
                    case FD_WRITE :      
                    {
                        //
                        // This message is internally generated by us to indicate
                        // that we sent the data packet.
                        //
                        OnReadyToWrite( WSAGETSELECTERROR(lParam) );
                        break;
                    };
    
                    case FD_CONNECT :      
                    {
                        OnConnectComplete( WSAGETSELECTERROR(lParam) );
                        break;
                    };
    
                    case FD_CLOSE :      
                    {
                        OnClose( WSAGETSELECTERROR(lParam) );
                        break;
                    };
                    
                    default:
                    {
                        rAssert( false );
                        break;
                    }
                }
            }
            result = 0;
            break;
        }

        case WM_SENDDATACOMPLETE:
        {
            //
            // Here we have received notification that the send of a data packet is
            // complete. Make sure socket hasn't been closed and process the message.
            //    
            if( (SOCKET) wParam == m_Socket )
            {
                OnSendComplete( );
            }

            result = 0;
            break;
        }
        
        case WM_TIMER :
        {
            //
            // Timer message. Invoke handler.
            //
            rAssert( wParam == TIMER_ID );
            
            OnTimerExpired( );
            result = 0;
            break;
        }

        default:
        {
            //
            // Not one of our messgae, Pass on for default handling.
            //
            result = DefWindowProc(hwnd, uMsg, wParam, lParam);
            break;
        }            
    }

    return( result );
}

//=============================================================================
// Function:    rDbgComHostChannel::SetState
//=============================================================================
// Description: This member updates the connection state and our internal state.
//              It will invoked users callback if one installed in the connection
//              state changes or if the message changes. 
//
// Parameters:  state - connect state
//              internal state
//              message
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComHostChannel::SetState
( 
    ConnectionState state, 
    InternalState   internalState,
    const char*     msg 
)
{
    //
    // Set new internal state.
    //
    m_InternalState = internalState;

    bool NeedToInvokeCallback = ( (state != m_ConnectionState) || 
                                  (0 != strcmp( m_TextInfoBuffer, msg) ) );
    
    //
    // Save new info regardless.
    //
    m_ConnectionState = state;
    
    rAssert( strlen( msg ) < sizeof( m_TextInfoBuffer ) );
    strcpy( m_TextInfoBuffer, msg );

    //
    // Check if we have a callback and we need to invoked it.
    //
    if( NeedToInvokeCallback && (m_StatusCallback != NULL ) )
    {
        m_StatusCallback->OnStatusChange( m_ConnectionState, m_TextInfoBuffer );
    }
    
    //
    // Check if the state is not attached. If so, fail any outstanding 
    // sends or receivces.
    //
    if( m_ConnectionState != Attached )
    {
        //
        // Complete any pending sends or receives.
        // 
        if( m_ClientReceiveCallback != NULL )
        {
            m_ClientReceiveCallback->OnReceiveComplete( false, 0 );
            
            // FIX James Tan, somehow the m_ClientReceiveCallback got set to NULL
            // if the game crashed.
            if ( m_ClientReceiveCallback != NULL)
            {
                m_ClientReceiveCallback->Release( );
                m_ClientReceiveCallback = NULL;
            }
        }

        if( m_ClientSendCallback != NULL )
        {
            m_ClientSendCallback->OnSendComplete( 0 );
            m_ClientSendCallback->Release( );
            m_ClientSendCallback = NULL;
        }
    }
}

//=============================================================================
// Function:    rDbgComHostChannel::AddRef
//=============================================================================
// Description: This member is used to update the reference count of the object.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
    
void rDbgComHostChannel::AddRef( void )
{
    m_ReferenceCount++;
}

//=============================================================================
// Function:    rDbgComHostChannel::Release
//=============================================================================
// Description: This member is used to update the reference count of the object.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
    
void rDbgComHostChannel::Release( void )
{
    m_ReferenceCount--;
    if( m_ReferenceCount == 0 )
    {
        delete this;
    }   
}

//=============================================================================
// Function:    rDbgComHostChannel::MakeMPConnectPacket
//=============================================================================
// Description: Helper to build up a NETMP connect packet.
//
// Parameters:  info to build packet
//
// Returns:     size of packet to send.
//
// Notes:
//------------------------------------------------------------------------------

unsigned int rDbgComHostChannel::MakeNETMPConnectPacket
( 
    unsigned char*  buffer, 
    unsigned int    bufferSize, 
    unsigned int    protocol
)
{
    //
    // Define the packet structure for a connect request.
    //
    struct ConnectPacket
    {
	    DECI2_HDR       m_Deci2;
		NETMP_HDR       m_Netmp;
		NETMP_PROTOS    m_Protos;
    };
   
    rAssert( bufferSize >= sizeof( ConnectPacket ) );
    ConnectPacket* pPacket = (ConnectPacket*) buffer;  
        
    //
    // Build the DECI part.
    //
    pPacket->m_Deci2.length = sizeof( ConnectPacket );
    pPacket->m_Deci2.reserved = 0;
    pPacket->m_Deci2.protocol = DECI2_PROTO_NETMP;
    pPacket->m_Deci2.source = DECI2_NODE_HOST;
    pPacket->m_Deci2.destination = DECI2_NODE_HOST;            
  
    //
    // Build up the netmp part.
    //
    pPacket->m_Netmp.code = NETMP_CODE_CONNECT;
    pPacket->m_Netmp.result = 0;
    pPacket->m_Protos.pri = NETMP_PRI_DEFAULT;
	pPacket->m_Protos.reserved = 0;
    pPacket->m_Protos.proto = protocol;

    return( sizeof( ConnectPacket ) );
}

//=============================================================================
// Function:    rDbgComHostChannel::MakeMPMessagePacket
//=============================================================================
// Description: Helper to build up a NETMP message packet.
//
// Parameters:  info to build packet
//
// Returns:     size of packet to send.
//
// Notes:
//------------------------------------------------------------------------------

unsigned int rDbgComHostChannel::MakeNETMPMessagePacket
( 
    unsigned char*  buffer, 
    unsigned int    bufferSize
)
{
    //
    // Define the packet structure for a connect request.
    //
    struct MessagePacket
    {
	    DECI2_HDR       m_Deci2;
		NETMP_HDR       m_Netmp;
		char            m_ComputerName[ MaxComputerName + 1 ];
    };
   
    rAssert( bufferSize >= sizeof( MessagePacket ) );
    MessagePacket* pPacket = (MessagePacket*) buffer;  
        
    //
    // Build the DECI part.
    //
    pPacket->m_Deci2.length = sizeof( MessagePacket );
    pPacket->m_Deci2.reserved = 0;
    pPacket->m_Deci2.protocol = DECI2_PROTO_NETMP;
    pPacket->m_Deci2.source = DECI2_NODE_HOST;
    pPacket->m_Deci2.destination = DECI2_NODE_HOST;            
  
    //
    // Build up the netmp part.
    //
    pPacket->m_Netmp.code = NETMP_CODE_MESSAGE;
    pPacket->m_Netmp.result = 0;

    //
    // Fill out computer name.  
    //
    memset( pPacket->m_ComputerName, 0, sizeof( pPacket->m_ComputerName ));
    unsigned long Size = MaxComputerName + 1;
    GetComputerName( pPacket->m_ComputerName, &Size );

    return( sizeof( MessagePacket ) );
}

//=============================================================================
// Function:    rDbgComHostChannel::MakeNETMPStatusPacket
//=============================================================================
// Description: Helper to build up a NETMP status packet.
//
// Parameters:  info to build packet
//
// Returns:     size of packet to send.
//
// Notes:
//------------------------------------------------------------------------------

unsigned int rDbgComHostChannel::MakeNETMPStatusPacket
( 
    unsigned char*  buffer, 
    unsigned int    bufferSize
)
{
    //
    // Define the packet structure for a connect request.
    //
    struct StatusPacket
    {
	    DECI2_HDR       m_Deci2;
		NETMP_HDR       m_Netmp;
    };
   
    rAssert( bufferSize >= sizeof( StatusPacket ) );
    StatusPacket* pPacket = (StatusPacket*) buffer;  
        
    //
    // Build the DECI part.
    //
    pPacket->m_Deci2.length = sizeof( StatusPacket );
    pPacket->m_Deci2.reserved = 0;
    pPacket->m_Deci2.protocol = DECI2_PROTO_NETMP;
    pPacket->m_Deci2.source = DECI2_NODE_HOST;
    pPacket->m_Deci2.destination = DECI2_NODE_HOST;            
  
    //
    // Build up the netmp part.
    //
    pPacket->m_Netmp.code = NETMP_CODE_STATUS;
    pPacket->m_Netmp.result = 0;

    return( sizeof( StatusPacket ) );
}

//=============================================================================
// Function:    rDbgComHostChannel::MakeDataPacket
//=============================================================================
// Description: Helper to build up a a data packet used to send actual client 
//              data.
//
// Parameters:  info to build packet
//
// Returns:     size of packet to send.
//
// Notes:
//------------------------------------------------------------------------------

unsigned int rDbgComHostChannel::MakeDataPacket
( 
    unsigned char*  buffer, 
    unsigned int    buffersize,
    unsigned int    m_Protocol,
    unsigned int    bytesToSend,
    unsigned char*  pData 
)
{
    //
    // Define the packet structure for a data packet
    //
    struct DataPacket
    {
	    DECI2_HDR           m_Deci2;
		rDbgComDataPacket   m_Data;
    };
   
    rAssert( buffersize >= sizeof( DataPacket ) );
    DataPacket* pPacket = (DataPacket*) buffer;  
        
    //
    // Build the DECI part. We make sure we keep it aligned to four bytes.
    //
    unsigned int packetSize = sizeof( DECI2_HDR) + sizeof( rDbgComDataPacket ) - (rDbgComMaxDataPacketSize - bytesToSend );
    if( packetSize % 4 != 0 )
    {
        packetSize = ((packetSize / 4) + 1) * 4;
    }

    pPacket->m_Deci2.length = packetSize;
    pPacket->m_Deci2.reserved = 0;
    pPacket->m_Deci2.protocol = m_Protocol;
    pPacket->m_Deci2.source = DECI2_NODE_HOST;
    pPacket->m_Deci2.destination = DECI2_NODE_EE;            
    
    pPacket->m_Data.m_Command = CmdDataPacket;
    pPacket->m_Data.m_DataSize = bytesToSend;
    memcpy( pPacket->m_Data.m_Data, pData, bytesToSend );

    return( packetSize );
}

//=============================================================================
// Function:    rDbgComHostChannel::MakeInternalConnectReplyPacket
//=============================================================================
// Description: Helper to build up a an internal connect reply packet.
//
// Parameters:  info to build packet
//
// Returns:     size of packet to send.
//
// Notes:
//------------------------------------------------------------------------------

unsigned int rDbgComHostChannel::MakeInternalConnectReplyPacket
( 
    unsigned char*  buffer, 
    unsigned int    buffersize,
    unsigned int    m_Protocol
)
{
    //
    // Define the packet structure for a data packet
    //
    struct ReplyPacket
    {
	    DECI2_HDR                  m_Deci2;
		rDbgComConnectReplyPacket  m_Reply;
    };
   
    rAssert( buffersize >= sizeof( ReplyPacket ) );
    ReplyPacket* pPacket = (ReplyPacket*) buffer;  
        
    //
    // Build the DECI part. 
    //
    pPacket->m_Deci2.length = sizeof( ReplyPacket );
    pPacket->m_Deci2.reserved = 0;
    pPacket->m_Deci2.protocol = m_Protocol;
    pPacket->m_Deci2.source = DECI2_NODE_HOST;
    pPacket->m_Deci2.destination = DECI2_NODE_EE;            
    
    pPacket->m_Reply.m_Command = CmdConnectReply;
        
    return( sizeof( ReplyPacket ) );
}

//=============================================================================
// Function:    rDbgComHostChannel::MakeMPKillPacket
//=============================================================================
// Description: Helper to build up a NETMP kill packet.
//
// Parameters:  info to build packet
//
// Returns:     size of packet to send.
//
// Notes:
//------------------------------------------------------------------------------

unsigned int rDbgComHostChannel::MakeNETMPKillPacket
( 
    unsigned char*  buffer, 
    unsigned int    bufferSize, 
    unsigned int    protocol
)
{
    //
    // Define the packet structure for a connect request.
    //
    struct KillPacket
    {
	    DECI2_HDR       m_Deci2;
		NETMP_HDR       m_Netmp;
		unsigned short  m_Protocol;
    };
   
    rAssert( bufferSize >= sizeof( KillPacket ) );
    KillPacket* pPacket = (KillPacket*) buffer;  
        
    //
    // Build the DECI part.
    //
    pPacket->m_Deci2.length = sizeof( KillPacket );
    pPacket->m_Deci2.reserved = 0;
    pPacket->m_Deci2.protocol = DECI2_PROTO_NETMP;
    pPacket->m_Deci2.source = DECI2_NODE_HOST;
    pPacket->m_Deci2.destination = DECI2_NODE_HOST;            
  
    //
    // Build up the netmp part.
    //
    pPacket->m_Netmp.code = NETMP_CODE_KILL;
    pPacket->m_Netmp.result = 0;
    pPacket->m_Protocol = protocol;

    return( sizeof( KillPacket ) );
}

//=============================================================================
// Function:    rDbgComHostChannel::MakeInternalReconnectReplyPacket
//=============================================================================
// Description: Helper to build up a an internal reconnect reply packet.
//
// Parameters:  info to build packet
//
// Returns:     size of packet to send.
//
// Notes:       On the true deci system, the host can vanish and the client will
//              not even know the host was gone (unless it is transmitting). This
//              packet lets the client know was have arrived.
//------------------------------------------------------------------------------

unsigned int rDbgComHostChannel::MakeInternalReconnectPacket
( 
    unsigned char*  buffer, 
    unsigned int    buffersize,
    unsigned int    m_Protocol
)
{
    //
    // Define the packet structure for a data packet
    //
    struct ReconnectPacket
    {
	    DECI2_HDR                  m_Deci2;
		rDbgComReconnectPacket     m_Reconnect;
    };
   
    rAssert( buffersize >= sizeof( ReconnectPacket ) );
    ReconnectPacket* pPacket = (ReconnectPacket*) buffer;  
        
    //
    // Build the DECI part. 
    //
    pPacket->m_Deci2.length = sizeof( ReconnectPacket );
    pPacket->m_Deci2.reserved = 0;
    pPacket->m_Deci2.protocol = m_Protocol;
    pPacket->m_Deci2.source = DECI2_NODE_HOST;
    pPacket->m_Deci2.destination = DECI2_NODE_EE;            
    
    pPacket->m_Reconnect.m_Command = CmdReconnect;
        
    return( sizeof( ReconnectPacket ) );
}

//=============================================================================
// Function:    rDbgComHostChannel::SendHelper
//=============================================================================
// Description: This function is used to send a packet. It is used to keep 
//              the logic easy as it was a biot of a hack added at the last
//              moment to ensure that all the data was sent.
//
// Parameters:  same as send
//
// Returns:     size of request or zero if error.
//
// Notes:       
//------------------------------------------------------------------------------

int rDbgComHostChannel::SendHelper
( 
    int         socket,
    const char* buf, 
    int         len,
    int         flags
)
{
    //
    // Stay in this routine until we either send all the data or get an error 
    // other than one indicating we would block.
    //
    int     bytesToSend = len;
    int     bytesSent = 0;

    while( bytesToSend != 0 )
    {
        int justSent = m_SocketImp->send( socket, &buf[ bytesSent ], bytesToSend, flags );

        if( justSent < 0 )
        {
            //
            // Error. Make sure it was a would block. If not, return zero.
            //
            if( WSAEWOULDBLOCK != m_SocketImp->lasterror( socket ) )
            {
                return( 0 );
            }
        
            //
            // Yeild 
            //
            Sleep( 0 );
        }
        else
        {
            //
            // Update counters and try some more.
            //
            bytesToSend -= justSent;
            bytesSent += justSent;
        }
    }     

    //
    // Success. Return the number requested to send.
    //
    return( len );
}

#endif
