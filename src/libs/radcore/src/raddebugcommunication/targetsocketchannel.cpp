//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        TargetSocketChannel.cpp
//
// Subsystem:   Radical Debug Communication Socket Target Channel
//
// Description:	This file contains the implementation of the Debug Communication
//              socket target Channel
//
// Author:		PGM
//
// Revisions:	Nov 28, 2000    Creation
//
// Notes:       
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#if defined(RAD_WIN32) || defined(RAD_UWP)
#include <windows.h>
#endif // RAD_WIN32 || RAD_UWP

#include <string.h>
//#include <stddef.h>
#include <radplatform.hpp>
#include <radmemorymonitor.hpp>
#include <radthread.hpp>
#include "targetx.hpp"          
#include "targetsocketchannel.hpp"   
#include "targetconnection.hpp"
#include "protocol.hpp"
#include "decitypes.h"
#include "deci2x.h"
#include "netmp.h"
#include "dcmp.h"

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    rDbgComSocketTargetChannel::rDbgComSocketTargetChannel
//=============================================================================
// Description: Constructor for the Socket target channel object. Initialize things and
//              set state.
//
// Parameters:  Parent Target Object
//              protocol
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

rDbgComSocketTargetChannel::rDbgComSocketTargetChannel
( 
    rDbgComTarget*    pTarget, 
    unsigned short  protocol
)
    :
    m_ReferenceCount( 1 ),
    m_Protocol( protocol ),
    m_ConnectionState( Detached ),
    m_StatusCallback( NULL ),
    m_Socket( 0 ),
    m_InternalState( Idle ),
    m_ClientReceiveCallback( NULL ),
    m_ClientReceiveSize( 0 ),
    m_ClientSendCallback( NULL ),
    m_ClientSendSize( 0 )
{
    Lock( );

    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "rDbgComSocketTargetChannel" );

    //
    // Save reference to parent target. Must update reference as we are holding it,
    //
    m_ParentTarget = pTarget;
    radAddRef( m_ParentTarget, this );

    //
    // Set the initial info buffer string
    //
    strcpy( m_TextInfoBuffer, "Detached" );

    //
    // Now lets set if we have a connection (socket ) already which we can use for 
    // communication.
    //
    ProtocolInfoEntry* pEntry = m_ParentTarget->FindProtocol( m_Protocol );

    if( pEntry != NULL ) 
    {
        //  
        // Here there is an entry on this protocol. It had better be in
        // a pending connection state or things are in bad shape. Most likely
        // another cleint on the target is using the same protocol.
        //
        rAssert( pEntry->m_State == ProtocolInfoEntry::ConnectionPending );
            
        //
        // Lets get the socket from this pending connection and update the
        // state of things.
        //
        m_Socket = pEntry->m_pConnection->GetSocket( );
            
        //
        // Update the ProtocoolInfo Entry.
        //  
        pEntry->m_pConnection = NULL;
        pEntry->m_State = ProtocolInfoEntry::Connected;
           
    }
    else
    {
        //
        // Here there is no pending connection. Find a free entry in the table
        // and add ourself.
        //
        pEntry = m_ParentTarget->FindFreeProtocol(  );   
        rAssert( pEntry != NULL );
        
        pEntry->m_pChannel = this;
        pEntry->m_Protocol = m_Protocol;
        pEntry->m_State = ProtocolInfoEntry::ChannelPending;
        
     }

    Unlock( );
}

//=============================================================================
// Function:    rDbgComSocketTargetChannel::~rDbgComSocketTargetChannel
//=============================================================================
// Description: Destructor for the host channel object. Simply unintialize. 
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

rDbgComSocketTargetChannel::~rDbgComSocketTargetChannel( void )
{
    Lock( );

    if( m_Protocol != 0 )
    {
        ProtocolInfoEntry* pEntry = m_ParentTarget->FindProtocol( m_Protocol );

        rAssert( pEntry != NULL );
        pEntry->m_State = ProtocolInfoEntry::Free;
    }

    radRelease( m_ParentTarget, this );

    Unlock( );
}

//=============================================================================
// Function:    rDbgComSocketTargetChannel::RegisterStatusCallback
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

void rDbgComSocketTargetChannel::RegisterStatusCallback
( 
    IRadDbgComChannelStatusCallback* pCallback
)
{
    Lock( );
    //
    // Simply save the address.
    //
    m_StatusCallback = pCallback;
    Unlock( );
}

//=============================================================================
// Function:    rDbgComSocketTargetChannel::GetStatus
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

void rDbgComSocketTargetChannel::GetStatus
( 
    ConnectionState* pConnectionState, 
    char* pMessage
)
{
    Lock( );
    *pConnectionState = m_ConnectionState;
    if( pMessage != NULL )
    {
        strcpy( pMessage, m_TextInfoBuffer );
    }
    Unlock( );
}

//=============================================================================
// Function:    rDbgComSocketTargetChannel::Attach
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

void rDbgComSocketTargetChannel::Attach( void )
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

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
    // Lets check if we have a socket. If so, lets initiate the send of a connect
    // packet.
    // 
    if( m_Socket == 0 )
    {
        //
        // Here we do not have a socket. We will get one when the host issues 
        // an attached. In the meantime, we must wait for one.
        //
        SetState( Attaching, WaitingForSocket, "Waiting for host to connect");
    }
    else
    {
        //  
        // Here we have a socket. Lets initiate the connection by sending 
        // a connect packet. To be really perfect, we should start a connection
        // time-out. However, this error will be detected by the close of a socket.
        //
        m_CurrentBytesToWrite = MakeInternalConnectPacket( );
        m_CurrentWriteAddress = m_TxBuffer;

        m_CurrentBytesToRead = sizeof( DECI2_HDR );
        m_ReadingHeader = true;
        m_CurrentReadAddress = m_RxBuffer;

        //
        // Set our state.
        //
        SetState( Attaching, WaitingForConnectResponse, "Waiting for host reply");

        m_Active = true;

        //
        // Queue ourself to the dispatcher. This will initiate the sending 
        // of the data.
        //
        m_ParentTarget->m_Dispatcher->QueueCallback( this, NULL );
    }
}

//=============================================================================
// Function:    rDbgComSocketTargetChannel::Detach
//=============================================================================
// Description: This routine is invoked to detach the channel.
//
// Parameters:  n/a
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComSocketTargetChannel::Detach( void )
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

    //
    // If already detached, do nothing.
    //
    if( m_ConnectionState == Detached )
    {
        return;
    }

    //
    // If attached, need to send the kill packet.
    //
    if( m_ConnectionState == Attaching )        
    {
        //
        // If waiting for socket, then things are simple.
        //
        if( m_InternalState == WaitingForSocket )
        {
            SetState( Detached, Idle, "Detached");
        }
        else
        {
            //
            // Here we have a socket but have not got our reply back from the host.
            //
            m_ParentTarget->m_SocketImp->closesocket( m_Socket );
            m_Socket = 0;

            m_Active = false;

            ProtocolInfoEntry* pEntry = m_ParentTarget->FindProtocol( m_Protocol );

            rAssert( pEntry != NULL );
            
            pEntry->m_pChannel = this;
            pEntry->m_State = ProtocolInfoEntry::ChannelPending;
    
            SetState( Detached, Idle, "Detached");
        }
    }
    else
    {
        //
        // Here we are attached. Send the disconnect packet.
        //
        SetState( Detaching, WaitToSendDisconnectMessage, "Sending disconnect");

        //
        // Set channel free since we are closing
        //
        ProtocolInfoEntry* pEntry = m_ParentTarget->FindProtocol( m_Protocol );
        rAssert( pEntry != NULL );
        pEntry->m_State = ProtocolInfoEntry::Free;

        //
        // Send the shutdown message.
        //
        m_CurrentBytesToWrite = MakeInternalDisconnectPacket( );
        m_CurrentWriteAddress = m_TxBuffer;
    }
}   

//=============================================================================
// Function:    rDbgComSocketTargetChannel::ReceiveAsync
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

void rDbgComSocketTargetChannel::ReceiveAsync
(   
    void*                           pBuffer,
    unsigned int                    numBytes,
    IRadDbgComChannelReceiveCallback* callback
)
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

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
// Function:    rDbgComSocketTargetChannel::SendAsync
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

void rDbgComSocketTargetChannel::SendAsync
( 
    void*                        pBuffer, 
    unsigned int                 numBytes,
    IRadDbgComChannelSendCallback* callback
)
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

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
    // Partion the send into the block size and initiate the send.
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
    // Lets build up the send packet. 
    //
    m_CurrentBytesToWrite = MakeDataPacket( bytesToSend, m_ClientSendBuffer );
    m_CurrentWriteAddress = m_TxBuffer;
        
    //
    // Update the amount of client data to send and its pointer.
    //
    m_ClientSendSize -= bytesToSend;
    m_ClientSendBuffer += bytesToSend;

    //
    // Send will be caried out by the dispatch event.
    //
}

//=============================================================================
// Function:    rDbgComSocketTargetChannel::OnDispatchEvent
//=============================================================================
// Description: This member is invoked to check for activity on our socket.
//
// Parameters:  
//
// Returns:    
//------------------------------------------------------------------------------

void rDbgComSocketTargetChannel::OnDispatchCallack( void* userData )
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

    (void) userData;

    //
    // If we are no longer active, return.
    //
    if( !m_Active )
    {
        return;
    }

    //
    // Check if we are receiving data. If so attempt to receive it.
    //
    if( m_CurrentBytesToRead != 0 )
    {
        //
        // Here we are receiving data. Attempt to read the data.
        //
ReadMore:
        int bytesRead = m_ParentTarget->m_SocketImp->recv( m_Socket, (char*) m_CurrentReadAddress, m_CurrentBytesToRead, 0 );
        if( bytesRead <= 0 )
        {
            //
            // Check last error. If it is not a would block indication, things are not
            // go and we just return.
            //
            if( WSAEWOULDBLOCK != m_ParentTarget->m_SocketImp->lasterror( m_Socket ) )
            {
                ProcessError( );
                return;
            }
        }
        else
        {
            //
            // Update number of bytes received.
            //
            m_CurrentReadAddress += bytesRead;
            m_CurrentBytesToRead -= bytesRead;
            if( m_CurrentBytesToRead == 0 )
            {
                //
                // Here we have received the number of bytes. Check if it was
                // the header.
                //
                if( m_ReadingHeader )
                {
                    m_ReadingHeader = false;
                    rAssert( sizeof(m_RxBuffer) >= radPlatformEndian16( ((DECI2_HDR*) m_RxBuffer)->length ) );
                    m_CurrentBytesToRead = radPlatformEndian16( ((DECI2_HDR*) m_RxBuffer)->length) - sizeof(DECI2_HDR) ;

                    //
                    // Try to read more.
                    //
                    goto ReadMore;
 
                }
                else
                {                                                  
                    //
                    // Reset the receiver.
                    //
                    m_CurrentBytesToRead = sizeof( DECI2_HDR );
                    m_ReadingHeader = true;
                    m_CurrentReadAddress = m_RxBuffer;

                    //
                    // Here we have received data. Process the packet. Will return false
                    // if problem.
                    //
                    if( !ProcessReceiveComplete( ) )
                    {
                        //
                        // Return. Things are not good.
                        //
                        ProcessError( );
                        return;
                    }
                }
            }
        }
    }     
                        
    //
    // Check if any more data to send.
    //  
    if( m_CurrentBytesToWrite != 0 )
    {
        //
        // Here we have data to send.
        //
        int bytesWritten = m_ParentTarget->m_SocketImp->send( m_Socket, (char*) m_CurrentWriteAddress, m_CurrentBytesToWrite, 0 );
        if( bytesWritten <= 0 )
        {
            //
            // Check last error. If it is not a would block indication, things are not
            // go and we just return.
            //
            if( WSAEWOULDBLOCK != m_ParentTarget->m_SocketImp->lasterror( m_Socket ) )
            {
                //
                // Return. Things are not good.
                //
                ProcessError( );
                return;
            }
        }
        else
        {
            //
            // Update number of bytes written.
            //
            m_CurrentWriteAddress += bytesWritten;
            m_CurrentBytesToWrite -= bytesWritten;
            if( m_CurrentBytesToWrite == 0 )
            {
                if( !ProcessSendComplete( ) )
                {
                    //
                    // Return. Things are not good.
                    //
                    ProcessError( );
                    return;
                }
            }
        }
    }
        
    //
    // Here we dispatch ourselves.
    //
    m_ParentTarget->m_Dispatcher->QueueCallback( this, NULL );
    
}


//=============================================================================
// Function:    rDbgComSocketTargetChannel::ProcessSendComplete
//=============================================================================
// Description: This member is invoked when the current number of expected bytes
//              has been sent
//
// Parameters:  n/a
//
// Returns:     true - things are fine.
//              false - close connection.
//
// Notes:
//------------------------------------------------------------------------------

bool rDbgComSocketTargetChannel::ProcessSendComplete( void )
{   
    radSingleLock< IRadThreadMutex > singleLock( this, true );

    //
    // Check the state and act accordingly.
    //
    if( m_InternalState == WaitingForConnectResponse )
    {
        //
        // Here we have finished sending the connect message. Things
        // are fine. Return true.
        //
        return( true );
    }
 
    if( m_InternalState == WaitToSendDisconnectMessage )
    {
        SetState( Detached, Idle, "Detached");

        //
        // Here we have completed sending the detach message. We freed the protocol
        // entry proior to sending. If is it not free it means it has been gained
        // by another instance using this protocol. 
        //
        ProtocolInfoEntry* pEntry = m_ParentTarget->FindProtocol( m_Protocol );
        if( pEntry != NULL )
        {
            //  
            // Protocol consumed by another instance.  We will be descructed. Clear
            // protocols so it will not be found. This object will deconstruct.
            //
            m_Protocol = 0;
        }
        else
        {
            //
            // Regain an entry.
            //
            pEntry = m_ParentTarget->FindFreeProtocol(  );   
            rAssert( pEntry != NULL );
        
            pEntry->m_pChannel = this;
            pEntry->m_Protocol = m_Protocol;
            pEntry->m_State = ProtocolInfoEntry::ChannelPending;
        }

        //
        // Here we have finished sending our disconnect message. We can return
        // false, which will close things down.
        //
        return( false );
    }

    if( m_InternalState == Connected )
    {
        //
        // Here we better have a pending send. Check if it is complete. If so
        // invoked callback. If not update the send buffer.
        //
        rAssert( m_ClientSendCallback != NULL );

        if( m_ClientSendSize == 0 )
        {
            //
            // Here all of the cleints data has been sent. Inform the client.
            //
            IRadDbgComChannelSendCallback* callback = m_ClientSendCallback;
            m_ClientSendCallback = NULL;
            callback->OnSendComplete( true );

            callback->Release( );
        }
        else
        {
            //
            // Lets initiate the send of more data. Partion the send into the block 
            // size and initiate the send.
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
            // Lets build up the send packet. 
            //
            m_CurrentBytesToWrite = MakeDataPacket( bytesToSend, m_ClientSendBuffer );
            m_CurrentWriteAddress = m_TxBuffer;
        
            //
            // Update the amount of client data to send and its pointer.
            //
            m_ClientSendSize -= bytesToSend;
            m_ClientSendBuffer += bytesToSend;

            //
            // Send will be caried out by the dispatch event.
            //
        }

        return( true );
    }

    //
    // Assert here since the send complete should never be invoked in any other state.
    //
    rAssert( false );
        
    return( false );
}

//=============================================================================
// Function:    rDbgComSocketTargetChannel::ProcessReceiveComplete
//=============================================================================
// Description: This member is invoked when the current number of expected bytes
//              has been received.
//
// Parameters:  n/a
//
// Returns:     true - no error detected, false error detected. Controls
//              whether the channel connection is dropped.
//
// Notes:
//------------------------------------------------------------------------------

bool rDbgComSocketTargetChannel::ProcessReceiveComplete( void )
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

    //
    // Check the state.
    //
    if( m_InternalState == WaitingForConnectResponse )
    {
        //
        // Here we are waiting for a connection response. Make sure we 
        // got one. If not return false.
        //
        struct ReplyPacket
        {
	        DECI2_HDR                  m_Deci2;
		    rDbgComConnectReplyPacket  m_Reply;
        };
   
        ReplyPacket* pPacket = (ReplyPacket*) m_RxBuffer;  
    
        if( (radPlatformEndian16( pPacket->m_Deci2.protocol) != m_Protocol ) ||
            (radPlatformEndian32(pPacket->m_Reply.m_Command) != CmdConnectReply) )
        {
            //
            // Fail the operation.  
            //
            return( false );
        }
                          
        //
        // Things are good. Set our state to attached.
        //
        SetState( Attached, Connected, "Connected");
                    
        return( true );
    }
          
    //
    // Here the state must be eithor connected, or waiting to send disconnect,
    // If sending disconnect, ignore packets.
    //
    if( m_InternalState == WaitToSendDisconnectMessage )
    {
        return( true );
    }

    //
    // Here the state must be connected. Make sure it is.
    //
    if( m_InternalState != Connected )
    {
        return( false );
    }

    //
    // Check the packet. It must eithor be a kill packet or data packet.
    //
    struct KillPacket
    {
	    DECI2_HDR       m_Deci2;
		NETMP_HDR       m_Netmp;
		unsigned short  m_Protocol;
    };
   
    KillPacket* pKillPacket = (KillPacket*) m_RxBuffer;  
    
    if( (radPlatformEndian16( pKillPacket->m_Deci2.length) == sizeof( KillPacket ) ) &&
        (radPlatformEndian16( pKillPacket->m_Deci2.protocol) == DECI2_PROTO_NETMP) &&
        (radPlatformEndian16( pKillPacket->m_Protocol) == m_Protocol ) )
    {
        //
        // We are being killed. Return false.
        //
        return( false );
    }

    //
    // Here the packet had better be a data packet or things are messed up.
    //        

    rDbgComDataPacket* pData = (rDbgComDataPacket*) &m_RxBuffer[ sizeof( DECI2_HDR ) ];
    if( radPlatformEndian32(pData->m_Command) != CmdDataPacket )
    {
        return(false);
    }  

    //
    // The system expects that the caller has a receiver. If he doesn't we assert.
    //
    unsigned int dataIndex = 0;
    while( radPlatformEndian32( pData->m_DataSize ) > 0 )
    {
        rAssertMsg( m_ClientReceiveCallback != NULL, "Data received and not receive present\n");
        
        unsigned int bytesCopied;
        if( radPlatformEndian32( pData->m_DataSize ) >= m_ClientReceiveSize )
        {
            bytesCopied = m_ClientReceiveSize;
        }
        else
        {
            bytesCopied = radPlatformEndian32( pData->m_DataSize );
        }

        memcpy( m_ClientReceiveBuffer, &pData->m_Data[ dataIndex ], bytesCopied );
        
        //
        // Update things.
        //
        dataIndex += bytesCopied;
        pData->m_DataSize = radPlatformEndian32( radPlatformEndian32( pData->m_DataSize ) - bytesCopied );
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

    return( true );
}

//=============================================================================
// Function:    rDbgComSocketTargetChannel::ProcessError
//=============================================================================
// Description: This member is invoked to deal with an error condition.
//
// Parameters:  n/a
//
// Returns:     
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComSocketTargetChannel::ProcessError( void )
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

    //
    // Here we have a socket but have not got our reply back from the host.
    //
    if( m_Socket != 0 )
    {
        m_ParentTarget->m_SocketImp->closesocket( m_Socket );
        m_Socket = 0;
    }
    m_Active = false;

    if( m_Protocol == 0 )
    {
        return;
    }

    ProtocolInfoEntry* pEntry = m_ParentTarget->FindProtocol( m_Protocol );

    rAssert( pEntry != NULL );
    
    pEntry->m_pChannel = this;
    pEntry->m_State = ProtocolInfoEntry::ChannelPending;

    if( m_ConnectionState != Detached )
    {
        SetState( Attaching, WaitingForSocket, "Waiting for host to connect");
    }
}   

//=============================================================================
// Function:    rDbgComSocketTargetChannel::SetSocket
//=============================================================================
// Description: This member is used to provide this object with the socket
//              to use for communication.
//
// Parameters:  socket
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComSocketTargetChannel::SetSocket
( 
    SOCKET lsocket
)
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

    //
    // Save the socket.
    //
    m_Socket = lsocket;
        
    //
    // Check the state. If not attaching, just return.
    //
    if( m_ConnectionState != Attaching )
    {
        return;
    }

    //  
    // Our internal state must be waiting for socket.
    //
    rAssert( m_InternalState == WaitingForSocket );

    //
    //  
    // Here we have a socket. Lets initiate the connection by sending 
    // a connect packet. To be really perfect, we should start a connection
    // time-out. However, this error will be detected by the close of a socket.
    //
    m_CurrentBytesToWrite = MakeInternalConnectPacket( );
    m_CurrentWriteAddress = m_TxBuffer;

    m_CurrentBytesToRead = sizeof( DECI2_HDR );
    m_ReadingHeader = true;
    m_CurrentReadAddress = m_RxBuffer;

    //
    // Set our state.
    //
    SetState( Attaching, WaitingForConnectResponse, "Waiting for host reply");

    m_Active = true;

    //
    // Queue ourself to the dispatcher. This will initiate the sending 
    // of the data.
    //
    m_ParentTarget->m_Dispatcher->QueueCallback( this, NULL );
}
    
//=============================================================================
// Function:    rDbgComSocketTargetChannel::SetState
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

void rDbgComSocketTargetChannel::SetState
( 
    ConnectionState state, 
    InternalState   internalState,
    const char*     msg 
)
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

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
            m_ClientReceiveCallback->Release( );
            m_ClientReceiveCallback = NULL;
        }

        if( m_ClientSendCallback != NULL )
        {
            m_ClientSendCallback->OnSendComplete( false );
            m_ClientSendCallback->Release( );
            m_ClientSendCallback = NULL;
        }
    }
}

//=============================================================================
// Function:    rDbgComSocketTargetChannel::MakeInternalDisconnectPacket
//=============================================================================
// Description: Helper to build up a an internal disconnect packet.
//
// Parameters:  n/a
//
// Returns:     size of packet to send.
//
// Notes:
//------------------------------------------------------------------------------

unsigned int rDbgComSocketTargetChannel::MakeInternalDisconnectPacket
( 
    void
)
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

    //
    // Define the packet structure for a connect packet
    //
    struct DisconnectPacket
    {
	    DECI2_HDR                       m_Deci2;
		rDbgComDisconnectRequestPacket  m_Request;
    };
   
    DisconnectPacket* pPacket = (DisconnectPacket*) m_TxBuffer;  
        
    //
    // Build the DECI part. 
    //
    pPacket->m_Deci2.length = radPlatformEndian16( sizeof( DisconnectPacket ) );
    pPacket->m_Deci2.reserved = 0;
    pPacket->m_Deci2.protocol = radPlatformEndian16( m_Protocol );
    pPacket->m_Deci2.source = DECI2_NODE_EE;
    pPacket->m_Deci2.destination = DECI2_NODE_HOST;            
    
    pPacket->m_Request.m_Command = ( rDbgComCommand ) radPlatformEndian32( CmdDisconnectRequest );
        
    return( sizeof( DisconnectPacket ) );
}

//=============================================================================
// Function:    rDbgComSocketTargetChannel::MakeInternalConnectPacket
//=============================================================================
// Description: Helper to build up a an internal connect packet.
//
// Parameters:  n/a
//
// Returns:     size of packet to send.
//
// Notes:
//------------------------------------------------------------------------------

unsigned int rDbgComSocketTargetChannel::MakeInternalConnectPacket
( 
    void
)
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

    //
    // Define the packet structure for a connect packet
    //
    struct ConnectPacket
    {
	    DECI2_HDR                   m_Deci2;
		rDbgComConnectRequestPacket m_Request;
    };
   
    ConnectPacket* pPacket = (ConnectPacket*) m_TxBuffer;  
        
    //
    // Build the DECI part. 
    //
    pPacket->m_Deci2.length = radPlatformEndian16( sizeof( ConnectPacket ) );
    pPacket->m_Deci2.reserved = 0;
    pPacket->m_Deci2.protocol = radPlatformEndian16( m_Protocol );
    pPacket->m_Deci2.source = DECI2_NODE_EE;
    pPacket->m_Deci2.destination = DECI2_NODE_HOST;            
    
    pPacket->m_Request.m_Command = (rDbgComCommand )radPlatformEndian32( CmdConnectRequest );
        
    return( sizeof( ConnectPacket ) );
}

//=============================================================================
// Function:    rDbgComSocketTargetChannel::MakeDataPacket
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

unsigned int rDbgComSocketTargetChannel::MakeDataPacket
( 
    unsigned int    bytesToSend,
    unsigned char*  pData 
)
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

    //
    // Define the packet structure for a data packet
    //
    struct DataPacket
    {
	    DECI2_HDR           m_Deci2;
		rDbgComDataPacket   m_Data;
    };
   
    DataPacket* pPacket = (DataPacket*) m_TxBuffer;  
        
    //
    // Build the DECI part. We make sure we keep it aligned to four bytes.
    //
    unsigned int packetSize = sizeof( DECI2_HDR) + sizeof( rDbgComDataPacket ) - (rDbgComMaxDataPacketSize - bytesToSend );
    if( packetSize % 4 != 0 )
    {
        packetSize = ((packetSize / 4) + 1) * 4;
    }

    pPacket->m_Deci2.length = radPlatformEndian16( packetSize );
    pPacket->m_Deci2.reserved = 0;
    pPacket->m_Deci2.protocol = radPlatformEndian16( m_Protocol );
    pPacket->m_Deci2.source = DECI2_NODE_EE;
    pPacket->m_Deci2.destination = DECI2_NODE_HOST;            
    
    pPacket->m_Data.m_Command = (rDbgComCommand) radPlatformEndian32( CmdDataPacket );
    pPacket->m_Data.m_DataSize = radPlatformEndian32( bytesToSend );
    memcpy( pPacket->m_Data.m_Data, pData, bytesToSend );

    return( packetSize );
}

//=============================================================================
// Function:    rDbgComSocketTargetChannel::AddRef
//=============================================================================
// Description: This member is used to update the reference count of the object.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
    
void rDbgComSocketTargetChannel::AddRef( void )
{
    Lock( );
    m_ReferenceCount++;
    Unlock( );
}

//=============================================================================
// Function:    rDbgComSocketTargetChannel::Release
//=============================================================================
// Description: This member is used to update the reference count of the object.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
    
void rDbgComSocketTargetChannel::Release( void )
{
    Lock( );
    m_ReferenceCount--;

    if( m_ReferenceCount == 0 )
    {
        Unlock( );
        delete this;
    }
    else
    {
        Unlock( );
    }
}

//=============================================================================
// Function:    rDbgComSocketTargetChannel::Lock
//=============================================================================
// Description: Mutex lock
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComSocketTargetChannel::Lock( void )
{
    radDbgComMutexLock( );
}

//=============================================================================
// Function:    rDbgComSocketTargetChannel::Unlock
//=============================================================================
// Description: Mutex unlock
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComSocketTargetChannel::Unlock( void )
{
    radDbgComMutexUnlock( );
}

//=============================================================================
// Function:    rDbgComSocketTargetChannel::Dump
//=============================================================================
// Description: This member is used to display object info
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

#ifdef RAD_DEBUG
void rDbgComSocketTargetChannel::Dump( char* pStringBuffer, unsigned int bufferSize )
{
    Lock( );
    sprintf( pStringBuffer, "Object: [rDbgComSocketTargetChannel] At Memory Location:[%p]\n", this );
    Unlock( );
}

#endif
