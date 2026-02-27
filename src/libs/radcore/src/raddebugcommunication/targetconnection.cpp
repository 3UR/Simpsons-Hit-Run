//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        TargetConnection.cpp
//
// Subsystem:   Radical Debug Communication 
//
// Description:	This file contains the implementation of the Debug Communication
//              connection object. This object simulates much of the behavior
//              that is present using the Sony DECI system.
//
// Author:		PGM
//
// Revisions:	Nov 20, 2000    Creation
//
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
#include "targetx.hpp"               // Target
#include "targetconnection.hpp"     // Connection objects
#include "protocol.hpp"             // Protocol
#include "targetsocketchannel.hpp"
#include "decitypes.h"
#include "deci2x.h"
#include "netmp.h"
#include "dcmp.h"

//=============================================================================
// Local Defintions
//=============================================================================

//
// Constant governing time-out we wait for expected messages.
//
const unsigned int MessageTimeout = 2000;       // Milliseconds

//=============================================================================
// Static Data Defintions
//=============================================================================

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    rDbgComTargetConnection::rDbgComTargetConnection
//=============================================================================
// Description: Constructor for the target connection object. This object is
//              used on the target when we are using sockets for communication.
//              This is not used when we are using the DECI system
//
// Parameters:  socket.
//              parent object.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

rDbgComTargetConnection::rDbgComTargetConnection
( 
    SOCKET lsocket,
    rDbgComTarget* pTarget
)
    :
    m_ReferenceCount( 1 ),
    m_Socket( lsocket ),
    m_pParentTarget( pTarget ),
    m_ConnectionState( WaitForNETMPConnect ),
    m_TimerActive( false ),
    m_Active( true )
{
    Lock( );

    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "rDbgComTargetConnection" );
    //  
    // Make sure we update the parent reference.
    //

    m_pParentTarget->AddRef( );

    //  
    // Set socket option to linger so unsent data is sent when we close. Set time-out
    // to zero as this was the  only way the close was detected by the host.
    //
    linger  Linger;
    Linger.l_onoff = 0;
    Linger.l_linger = 0;
    int result = m_pParentTarget->m_SocketImp->setsockopt( m_Socket, SOL_SOCKET, SO_LINGER, (const char*) &Linger, sizeof( linger ) );
    rAssert( result == 0 );    

    //  
    // Set socket option to send keep alive messages
    //
    int KeepAlive = 1;
    result = m_pParentTarget->m_SocketImp->setsockopt( m_Socket, SOL_SOCKET, SO_KEEPALIVE, (const char*) &KeepAlive, sizeof( int ) );
    rAssert( result == 0 );    
 
    //  
    // Lets set up our receiver. We are expectin teh DECI connect message in a timely
    // manner.
    //
    m_CurrentReadAddress = m_RxBuffer;
    m_CurrentBytesToRead = sizeof(DECI2_HDR) + sizeof(NETMP_HDR) + sizeof( NETMP_PROTOS ); 
    rAssert( m_CurrentBytesToRead <= sizeof( m_RxBuffer ) );
    m_StartTime = radTimeGetMilliseconds( );
    m_TimerActive = true;

    m_CurrentBytesToWrite = 0;

    //
    // Finally queue to the dispatcher. This is what keeps this object reference.
    //
    m_pParentTarget->m_Dispatcher->QueueCallback( this, NULL );

    Unlock( );
}

//=============================================================================
// Function:    rDbgComTargetConnection::~rDbgComTargetConnection
//=============================================================================
// Description: Destructor for the connection object. 
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

rDbgComTargetConnection::~rDbgComTargetConnection( void )
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

    //
    // Check if connected. If so remove ourself from the table.
    //
    if( m_ConnectionState == Connected )
    {
        ProtocolInfoEntry* pEntry = m_pParentTarget->FindProtocol( m_Protocol );
        
        rAssert( pEntry != NULL );
        rAssert( pEntry->m_State == ProtocolInfoEntry::ConnectionPending );       
        rAssert( pEntry->m_pConnection == this );

        pEntry->m_pConnection = NULL;
        pEntry->m_State = ProtocolInfoEntry::Free;
    }

    //
    // close the socket if non zero.
    //
    if( m_Socket != 0 )
    {
        m_pParentTarget->m_SocketImp->closesocket( m_Socket );
    }
 
    //
    // Lets drop the references to things we hold.
    //
    m_pParentTarget->Release( );
}            

//=============================================================================
// Function:    rDbgComTargetConnection::OnDispatchEvent( )
//=============================================================================
// Description: We use this member to interact with the socket. 
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComTargetConnection::OnDispatchCallack( void* userData )
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
    // First check for time-outs if the timer is active.
    //
    if( m_TimerActive )
    {
        //
        // Get the current time and check it against the time-out.
        //
        if( (radTimeGetMilliseconds( ) - m_StartTime) > MessageTimeout )
        {
            //
            // Here we have a time-out. All we need to do is return. This will
            // cause this object to be deleted as the dispatcher will no longer be
            // holding a reference.
 
            return;
        }
    }
    
    //
    // Check if we are receiving data. If so attempt to receive it.
    //
    if( m_CurrentBytesToRead != 0 )
    {
        //
        // Here we are receiving data. Attempt to read the data.
        //
        int bytesRead = m_pParentTarget->m_SocketImp->recv( m_Socket, (char*) m_CurrentReadAddress, m_CurrentBytesToRead, 0 );
        if( bytesRead <= 0 )
        {
            //
            // Check last error. If it is not a would block indication, things are not
            // go and we just return.
            //
            if( WSAEWOULDBLOCK != m_pParentTarget->m_SocketImp->lasterror( m_Socket ) )
            {
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
                // Here we have received data. Process the packet. Will return false
                // if problem.
                //
                if( !ProcessReceiveComplete( ) )
                {
                    //
                    // Return. Things are not good.
                    //

                    return;
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
        int bytesWritten = m_pParentTarget->m_SocketImp->send( m_Socket, (char*) m_CurrentWriteAddress, m_CurrentBytesToWrite, 0 );
        if( bytesWritten <= 0 )
        {
            //
            // Check last error. If it is not a would block indication, things are not
            // go and we just return.
            //
            if( WSAEWOULDBLOCK != m_pParentTarget->m_SocketImp->lasterror( m_Socket ) )
            {
                //
                // Return. Things are not good.
                //
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
                    return;
                }
            }
        }
    }
        
    //
    // Here we dispatch ourselves.
    //
    m_pParentTarget->m_Dispatcher->QueueCallback( this, NULL );
}

//=============================================================================
// Function:    rDbgComTargetConnection::ProcessReceiveComplete
//=============================================================================
// Description: This member is invoked when the current number of expected bytes
//              has been received.
//
// Parameters:  n/a
//
// Returns:     true - no error detected, false error detected. Controls
//              whether the channel is connection dropped.
//
// Notes:
//------------------------------------------------------------------------------

bool rDbgComTargetConnection::ProcessReceiveComplete( void )
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

    //
    // Check the state.
    //
    if( m_ConnectionState == WaitForNETMPConnect )
    {
        //
        // Here we are waiting for a connection packet. Make sure we 
        // got one. If not return false.
        //
        struct ConnectPacket
        {
	        DECI2_HDR       m_Deci2;
		    NETMP_HDR       m_Netmp;
		    NETMP_PROTOS    m_Protos;
        };
        ConnectPacket* pPacket = (ConnectPacket*) m_RxBuffer;  
    
        if( (radPlatformEndian16( pPacket->m_Deci2.protocol ) != DECI2_PROTO_NETMP ) ||
            ( pPacket->m_Netmp.code  != NETMP_CODE_CONNECT) )
        {
            //
            // Fail the operation.  
            //
            return( false );
        }
                          
        //
        // Check if the requested protocol has already been obtained. 
        //
        ProtocolInfoEntry* pEntry = m_pParentTarget->FindProtocol( radPlatformEndian16( pPacket->m_Protos.proto ) );

        if( (pEntry != NULL) && (pEntry->m_State != ProtocolInfoEntry::ChannelPending ) )
        {
            //
            // Here we found the protocol already in use. Build up a send packet and
            // report that the protocol is in use. Save the name that was using it as well.
            //
            strcpy( m_HostOwnerName, pEntry->m_HostComputerName );

            m_CurrentBytesToWrite = MakeNETMPConnectReply( false );
            m_CurrentWriteAddress = m_TxBuffer;
          
            //
            // Set the new state and reset the receiver, and timeout.
            // We are now waiting for the message to check for the connected computer.
            //
            m_ConnectionState = WaitForNETMPStatus;
                
            m_CurrentReadAddress = m_RxBuffer;
            m_CurrentBytesToRead = sizeof(DECI2_HDR) + sizeof(NETMP_HDR); 
            rAssert( m_CurrentBytesToRead <= sizeof( m_RxBuffer ) );
            m_StartTime = radTimeGetMilliseconds( );
            m_TimerActive = true;
          
        }
        else
        {
            //
            // Here the protocol is not in use. Send a success message and set the
            // state waiting for the computer message. Save the protocol.
            //
            m_Protocol = radPlatformEndian16( pPacket->m_Protos.proto );            

            m_CurrentBytesToWrite = MakeNETMPConnectReply( true );
            m_CurrentWriteAddress = m_TxBuffer;
          
            m_ConnectionState = WaitForNETMPMessage;
                
            m_CurrentReadAddress = m_RxBuffer;
            m_CurrentBytesToRead = sizeof(DECI2_HDR) + sizeof(NETMP_HDR) + MaxComputerName + 1; 
            rAssert( m_CurrentBytesToRead <= sizeof( m_RxBuffer ) );
            m_StartTime = radTimeGetMilliseconds( );
            m_TimerActive = true;
        }
    }
    else if( m_ConnectionState == WaitForNETMPStatus )
    {
        //
        // Here we are waiting for a status message. Make sure the received packet is
        // correct.
        //
        struct StatusPacket
        {
	        DECI2_HDR       m_Deci2;
		    NETMP_HDR       m_Netmp;
        };
        StatusPacket* pPacket = (StatusPacket*) m_RxBuffer;  
    
        if( (radPlatformEndian16( pPacket->m_Deci2.protocol) != DECI2_PROTO_NETMP ) ||
            (pPacket->m_Netmp.code != NETMP_CODE_STATUS) )
        {
            //
            // Fail the operation.  
            //
            return( false );
        }
    
        //
        // Lets build up the reply. We have the name of the owner.
        //
        m_CurrentBytesToWrite = MakeNETMPStatusReply( m_HostOwnerName );
        m_CurrentWriteAddress = m_TxBuffer;
          
        //
        // We just want the send to complete. Then we close the socket.
        //
        m_ConnectionState = WaitForNETMPStatusReplySend;
        m_CurrentBytesToRead = 0;

        m_TimerActive = false;
    }
    else if( m_ConnectionState == WaitForNETMPMessage )
    {
        //
        // Here we are waiting for a computer name message. Make sure the received packet is
        // correct.
        //
        struct MessagePacket
        {
	        DECI2_HDR       m_Deci2;
		    NETMP_HDR       m_Netmp;
            char            m_ComputerName[ MaxComputerName + 1 ];
        };
        MessagePacket* pPacket = (MessagePacket*) m_RxBuffer;  
    
        if( (radPlatformEndian16( pPacket->m_Deci2.protocol) != DECI2_PROTO_NETMP ) ||
            (pPacket->m_Netmp.code != NETMP_CODE_MESSAGE) )
        {
            //
            // Fail the operation.  
            //
            return( false );
        }
                
        //
        // Save the computer name.
        //
        strcpy( m_HostOwnerName, pPacket->m_ComputerName );

        //
        // Lets build up the reply. We have the name of the owner.
        //
        m_CurrentBytesToWrite = MakeNETMPMessageReply(  );
        m_CurrentWriteAddress = m_TxBuffer;

        //
        // Set up the receive and time-out. We are waiting for 
        // the reconnect mesasga.e
        //
        m_ConnectionState = WaitForInternalReconnect;

        m_CurrentReadAddress = m_RxBuffer;
        m_CurrentBytesToRead = sizeof(DECI2_HDR) + sizeof(rDbgComReconnectPacket); 
        rAssert( m_CurrentBytesToRead <= sizeof( m_RxBuffer ) );
        m_StartTime = radTimeGetMilliseconds( );
        m_TimerActive = true;
    }
    else if( m_ConnectionState == WaitForInternalReconnect )
    {
        //
        // Here we are waiting for a internal reconnect. If we get it, things
        // are in good shape.
        //
        struct ReconnectPacket
        {
	        DECI2_HDR               m_Deci2;
            rDbgComReconnectPacket  m_ReconnectPacket;
        };
        ReconnectPacket* pPacket = (ReconnectPacket*) m_RxBuffer;  
    
        if( (radPlatformEndian16( pPacket->m_Deci2.protocol) != m_Protocol ) ||
            (radPlatformEndian32(pPacket->m_ReconnectPacket.m_Command ) != CmdReconnect) )
        {
            //
            // Fail the operation.  
            //
            return( false );
        }
        
        //
        // Here everything is good. We want to check if a pending connection
        // exists
        //
        ProtocolInfoEntry* pEntry = m_pParentTarget->FindProtocol( m_Protocol );
        
        if( pEntry == NULL ) 
        {
            //
            // Here there is no entry with this protocol. Attempt to find a free entry.
            //
            pEntry = m_pParentTarget->FindFreeProtocol( );
            if( pEntry == NULL )
            {
                //
                // If we have no free entries, just return false. This will close things
                // up and host will receive error.
                //
                return( false );
            }
            else
            {
                //
                // Here we have a free entry and have established a connection with host.
                // The client on the target has yet to create the target channel however.
                // Add our self to the table, setting state up.
                //
                pEntry->m_pConnection = this;           // Weak reference
                pEntry->m_Protocol = m_Protocol;
                pEntry->m_State = ProtocolInfoEntry::ConnectionPending;
                strcpy( pEntry->m_HostComputerName, m_HostOwnerName ) ;
                
                //
                // Set up the to read a byte. If we get anything, we shut down
                // the connection as we.
                //
                m_CurrentReadAddress = m_RxBuffer;
                m_CurrentBytesToRead = 1;

                m_ConnectionState = Connected;
                m_TimerActive = false;
            }
        }
        else
        {
            //
            // Here we have an entry for this protocol. If it does not reflect the
            // pending channel state, then should just disconnect.
            //
            if( pEntry->m_State != ProtocolInfoEntry::ChannelPending )
            {
                return( false );
            }
            else 
            {
                //
                // Lets give the pending channel its socket to use for communication.
                // First set up the state of the table.
                //                                              
                strcpy( pEntry->m_HostComputerName, m_HostOwnerName );
                pEntry->m_State = ProtocolInfoEntry::Connected;
                pEntry->m_Protocol = m_Protocol;
      
                //
                // Give the pending connection the socket.
                //
                pEntry->m_pChannel->SetSocket( m_Socket );

                //
                // We zero our socket, so when this object goes out of scope,
                // the socket is not closed.
                //
                pEntry->m_pChannel = NULL;

                m_Socket = 0;
    
                return( false );
            }    

        }
    }
    else
    {
        //
        // Received something unexpected. Return false.
        //
        return( false );

    }      

    //
    // Everything ok if we get here.
    //
    return( true );
}

//=============================================================================
// Function:    rDbgComTargetConnection::ProcessSendComplete
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

bool rDbgComTargetConnection::ProcessSendComplete( void )
{   
    radSingleLock< IRadThreadMutex > singleLock( this, true );

    //
    // Check the state and act accordingly.
    //
    if( m_ConnectionState == WaitForNETMPStatusReplySend )
    {
        //
        // Here we have finished the send of the reply message indicating that
        // the protocol was already in use. Return false and an the connection 
        // will be closed.
        //
        return( false );
    }

    //
    // All other cases of sends, keep the channel alive.
    //

    return( true );
}

//=============================================================================
// Function:    rDbgComTargetConnection::MakeNETMPConnectReply
//=============================================================================
// Description: Helper to build up a NETMP connect reply packet.
//
// Parameters:  bool, true for success, false otherwise.
//
// Returns:     size of packet to send.
//
// Notes:
//------------------------------------------------------------------------------

unsigned int rDbgComTargetConnection::MakeNETMPConnectReply
( 
    bool    success
)
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

    //
    // Define the packet structure for a connect request.
    //
    struct ConnectReplyPacket
    {
	    DECI2_HDR       m_Deci2;
		NETMP_HDR       m_Netmp;
    };
   
    rAssert( sizeof( m_TxBuffer ) >= sizeof( ConnectReplyPacket ) );
    ConnectReplyPacket* pPacket = (ConnectReplyPacket*) m_TxBuffer;  
        
    //
    // Build the DECI part.
    //
    pPacket->m_Deci2.length = radPlatformEndian16( sizeof( ConnectReplyPacket ) );
    pPacket->m_Deci2.reserved = 0;
    pPacket->m_Deci2.protocol = radPlatformEndian16( DECI2_PROTO_NETMP );
    pPacket->m_Deci2.source = DECI2_NODE_HOST;
    pPacket->m_Deci2.destination = DECI2_NODE_HOST;            
  
    //
    // Build up the netmp part.
    //
    pPacket->m_Netmp.code = NETMP_CODE_CONNECTR;
    if( success )
    {
        pPacket->m_Netmp.result = NETMP_ERR_OK;
    }
    else
    {
        pPacket->m_Netmp.result = NETMP_ERR_BUSY;
    }

    return( sizeof( ConnectReplyPacket ) );
}

//=============================================================================
// Function:    rDbgComTargetConnection::MakeNETMPStatusReply
//=============================================================================
// Description: Helper to build up a NETMP status reply packet.
//
// Parameters:  name of computer owner.
//
// Returns:     size of packet to send.
//
// Notes:
//------------------------------------------------------------------------------

unsigned int rDbgComTargetConnection::MakeNETMPStatusReply
( 
    char*   name
)
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

    //
    // Define the packet structure for a connect request.
    //
    struct StatusReplyPacket
    {
	    DECI2_HDR       m_Deci2;
		NETMP_HDR       m_Netmp;
        unsigned short  m_Reserved[ 5 ];
        char            m_Name[ MaxComputerName + 1 ];
    };
   
    rAssert( sizeof( m_TxBuffer ) >= sizeof( StatusReplyPacket ) );
    StatusReplyPacket* pPacket = (StatusReplyPacket*) m_TxBuffer;  
        
    //
    // Build the DECI part.
    //
    pPacket->m_Deci2.length = radPlatformEndian16( sizeof( StatusReplyPacket ) );
    pPacket->m_Deci2.reserved = 0;
    pPacket->m_Deci2.protocol = radPlatformEndian16( DECI2_PROTO_NETMP) ;
    pPacket->m_Deci2.source = DECI2_NODE_HOST;
    pPacket->m_Deci2.destination = DECI2_NODE_HOST;            
  
    //
    // Build up the netmp part.
    //
    pPacket->m_Netmp.code = NETMP_CODE_STATUSR;
    pPacket->m_Netmp.result = NETMP_ERR_OK;
    
    strcpy(  pPacket->m_Name, name );

    return( sizeof( StatusReplyPacket ) );
}

//=============================================================================
// Function:    rDbgComTargetConnection::MakeNETMPMessgaeReply
//=============================================================================
// Description: Helper to build up a NETMP message reply packet.
//
// Parameters:  n/a
//
// Returns:     size of packet to send.
//
// Notes:
//------------------------------------------------------------------------------

unsigned int rDbgComTargetConnection::MakeNETMPMessageReply
( 
    void
)
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

    //
    // Define the packet structure for a connect request.
    //
    struct MessageReplyPacket
    {
	    DECI2_HDR       m_Deci2;
		NETMP_HDR       m_Netmp;
    };
   
    rAssert( sizeof( m_TxBuffer ) >= sizeof( MessageReplyPacket ) );
    MessageReplyPacket* pPacket = (MessageReplyPacket*) m_TxBuffer;  
        
    //
    // Build the DECI part.
    //
    pPacket->m_Deci2.length = radPlatformEndian16( sizeof( MessageReplyPacket ) );
    pPacket->m_Deci2.reserved = 0;
    pPacket->m_Deci2.protocol = radPlatformEndian16( DECI2_PROTO_NETMP );
    pPacket->m_Deci2.source = DECI2_NODE_HOST;
    pPacket->m_Deci2.destination = DECI2_NODE_HOST;            
  
    //
    // Build up the netmp part.
    //
    pPacket->m_Netmp.code = NETMP_CODE_MESSAGER;
    pPacket->m_Netmp.result = NETMP_ERR_OK;

    return( sizeof( MessageReplyPacket ) );
}

//=============================================================================
// Function:    rDbgComTargetConnection::AddRef
//=============================================================================
// Description: This member is used to update the reference count of the object.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
    
void rDbgComTargetConnection::AddRef( void )
{
    Lock( );
    m_ReferenceCount++;
    Unlock( );
}

//=============================================================================
// Function:    rDbgComTargetConnection::Release
//=============================================================================
// Description: This member is used to update the reference count of the object.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
    
void rDbgComTargetConnection::Release( void )
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
// Function:    rDbgComTargetConnection::Lock
//=============================================================================
// Description: Lock mutex
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComTargetConnection::Lock( void )
{
    radDbgComMutexLock( );
}

//=============================================================================
// Function:    rDbgComTargetConnection::Unlock
//=============================================================================
// Description: Unlock mutex
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComTargetConnection::Unlock( void )
{
    radDbgComMutexUnlock( );
}

//=============================================================================
// Function:    rDbgComTargetConnection::Deactivate
//=============================================================================
// Description: This member simply sets a boolean which stops this object from
//              being dispatched.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComTargetConnection::Deactivate( void )
{
    Lock( );
    m_Active = false;
    Unlock( );
}

//=============================================================================
// Function:    rDbgComTargetConnection::GetSocket
//=============================================================================
// Description: This member simply gets our socket. We return our connected
//              socket and de-activate ourself causing this object to deconstruct.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

SOCKET rDbgComTargetConnection::GetSocket( void )
{
    Lock( );

    SOCKET s = m_Socket ;    
    m_Socket = 0;
    m_Active = false;

    // Alter the connection state since this object will be destructed.
    // and we do not want it to think it is connected.   

    m_ConnectionState = WaitForNETMPConnect;
    Unlock( );

    return( s );
}

//=============================================================================
// Function:    rDbgComTargetConnection::Dump
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

void rDbgComTargetConnection::Dump( char* pStringBuffer, unsigned int bufferSize )
{
    Lock( );
    sprintf( pStringBuffer, "Object: [rDbgComTargetConnection] At Memory Location:[%p]\n", this );
    Unlock( );
}

#endif
