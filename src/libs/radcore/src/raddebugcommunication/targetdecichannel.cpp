//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        TargetDeciChannel.cpp
//
// Subsystem:   Radical Debug Communication Deci Target Channel
//
// Description:	This file contains the implementation of the Debug Communication
//              DECI target Channel
//
// Author:		PGM
//
// Revisions:	Nov 21, 2000    Creation
//
// Notes:       This contains the PS2EE target side implementation using the DECI
//              system.
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include <eekernel.h>
#include <deci2.h>
#include <sif.h>
#include <raddebug.hpp>               
#include <radobject.hpp>
#include <raddebugcommunication.hpp>
#include <string.h>              
#include <radmemorymonitor.hpp>
#include "targetx.hpp"          
#include "protocol.hpp"

//=============================================================================
// Local Defintions
//=============================================================================

//
// This macro is critical to getting the transfers to work. It appears the
// Deci2 system uses DMA. DMA does not work with cache, hence we must make
// sure our packets are written and read from main memory (bipassing the cache).
//
#define UNCACHED(p) ((unsigned char *)((unsigned int)p | 0x20000000))

//
// This contains the time we wait for a reply to our connect request.
//
const unsigned int ReplyTimeout = 1500;     // 1.5 seconds


//=============================================================================
// Static Data Defintions
//=============================================================================


//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    rDbgComDECITargetChannel::rDbgComDECITargetChannel
//=============================================================================
// Description: Constructor for the DECI target channel object. Initialize things and
//              set state.
//
// Parameters:  Parent Target Object
//              protocol
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

rDbgComDECITargetChannel::rDbgComDECITargetChannel
( 
    rDbgComTarget*    pTarget, 
    unsigned short  protocol
)
    :
    m_ReferenceCount( 1 ),
    m_Protocol( protocol ),
    m_ConnectionState( Detached ),
    m_StatusCallback( NULL ),
    m_Socket( -1 ),
    m_InternalState( Idle ),
    m_ClientReceiveCallback( NULL ),
    m_ClientReceiveSize( 0 ),
    m_ClientSendCallback( NULL ),
    m_ClientSendSize( 0 ),
    m_Timer( NULL ),
    m_EventMask( 0 ),
    m_UnderInterrupt( false )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "rDbgComDECITargetChannel" );
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
    // Lets set up our transmit and receive buffers. Make sure they are aligned to
    // 64 and a multiple of 64 bytes.
    //
    rAssert( (sizeof( m_RxBufferSpace ) % 64) == 0 );
    m_RxBuffer = UNCACHED( m_RxBufferSpace );
    if( (unsigned int) m_RxBuffer & 0x3f )
    {
        m_RxBuffer = (unsigned char*)((unsigned int)m_RxBuffer & 0xffffffc0);
        m_RxBuffer += 0x40;
    }  
    m_CurrentBytesToRead = 0;

    rAssert( (sizeof( m_TxBufferSpace ) % 64) == 0 );
    m_TxBuffer = UNCACHED( m_TxBufferSpace ); 
    if( (unsigned int) m_TxBuffer & 0x3f )
    {
        m_TxBuffer = (unsigned char*)((unsigned int)m_TxBuffer & 0xffffffc0);
        m_TxBuffer += 0x40;
    }  
    m_CurrentBytesToWrite = 0;

    //
    // Do a flush cache to ensure no one will touch our transfer buffer.
    //
   	FlushCache( 0 );

}

//=============================================================================
// Function:    rDbgComDECITargetChannel::~rDbgComDECITargetChannel
//=============================================================================
// Description: Destructor for the host channel object. Simply unintialize. 
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

rDbgComDECITargetChannel::~rDbgComDECITargetChannel( void )
{
    if( m_ConnectionState != Detached )
    {
        Detach( );
    }

    radRelease( m_ParentTarget, this );
}

//=============================================================================
// Function:    rDbgComDECITargetChannel::RegisterStatusCallback
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

void rDbgComDECITargetChannel::RegisterStatusCallback
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
// Function:    rDbgComDECITargetChannel::GetStatus
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

void rDbgComDECITargetChannel::GetStatus
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
// Function:    rDbgComDECITargetChannel::Attach
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

void rDbgComDECITargetChannel::Attach( void )
{
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
    // Lets make up a connect packet and initiate the send of it.
    //
    m_CurrentBytesToWrite = MakeInternalConnectPacket( );
    m_CurrentWriteAddress = m_TxBuffer;

    m_CurrentBytesToRead = sizeof( DECI2_HDR) + sizeof( rDbgComConnectReplyPacket );
    m_CurrentReadAddress = m_RxBuffer;    

    //
    // Create a timer which we will use to time-out if we do not get a reply.
    //
    m_ParentTarget->m_TimerList->CreateTimer( &m_Timer, ReplyTimeout, this );
    rAssert( m_Timer != NULL );

    //
    // Set our state.
    //
    SetState( Attaching, WaitingForConnectResponse, "Waiting for host reply");

    //
    // First open the DECI system.
    //
    m_Socket = sceDeci2Open( m_Protocol, (void*) this, DeciEventHandler );
    rAssert( m_Socket > 0 );

    //
    // Finally initiate the send.
    //
    if( 0 > sceDeci2ReqSend( m_Socket, DECI2_NODE_HOST) )
	{
    	//
        // Here something is wrong. Close socket, reset things
	    //
        sceDeci2Close( m_Socket );
        m_Socket = -1;
        m_Timer->Release( );
        m_Timer = NULL;

        SetState( Detached, Idle, "Failed to send message to host");
	}
}

//=============================================================================
// Function:    rDbgComDECITargetChannel::Detach
//=============================================================================
// Description: This routine is invoked to detach the channel.
//
// Parameters:  n/a
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComDECITargetChannel::Detach( void )
{
    //
    // If already detached, do nothing.
    //
    if( (m_ConnectionState == Detached) || (m_ConnectionState == Detaching) )
    {
        return;
    }

    //
    // If attached, need to send the kill packet.
    //
    if( m_ConnectionState == Attaching )        
    {
        //
        // Close the socket. Kill timer if running and set new state.
        //
        sceDeci2Close( m_Socket );
        m_Socket = -1;

        if( m_Timer != NULL )
        {
            m_Timer->Release( );
            m_Timer = NULL;

        }

        SetState( Detached, Idle, "Detached");
    }
    else
    {
        SetState( Detaching, WaitToSendDisconnectMessage, "Detaching");

        //
        // Send the shutdown message.
        //
        m_CurrentBytesToWrite = MakeInternalDisconnectPacket( );
        m_CurrentWriteAddress = m_TxBuffer;

        m_CurrentBytesToRead = 0;
   
        if( 0 > sceDeci2ReqSend( m_Socket, DECI2_NODE_HOST) )
	    {   
            //
            // Here something is wrong. Close socket, reset things
	        //
            sceDeci2Close( m_Socket );
            m_Socket = -1;
    	}
        else
        {
            //
            // Spin here waiting for the TX Send event to occur.
            //
            while( (m_EventMask & EventTxComplete) == 0 ) { ; }

            //
            // Close the socket.
            //
            sceDeci2Close( m_Socket );
            m_Socket = -1;
        }

        SetState( Detached, Idle, "Detached");
    }
}   

//=============================================================================
// Function:    rDbgComDECITargetChannel::ReceiveAsync
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

void rDbgComDECITargetChannel::ReceiveAsync
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
// Function:    rDbgComDECITargetChannel::SendAsync
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

void rDbgComDECITargetChannel::SendAsync
( 
    void*                       pBuffer, 
    unsigned int                numBytes,
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
    // Finally initiate the send.
    //
    if( 0 > sceDeci2ReqSend( m_Socket, DECI2_NODE_HOST) )
	{
    	//
        // Here something is wrong. Close socket, reset things
	    //
        sceDeci2Close( m_Socket );
        m_Socket = -1;

        SetState( Detached, Idle, "Failed to send message to host");
	}
}

//=============================================================================
// Function:    rDbgComDECITargetChannel::OnTimerDone
//=============================================================================
// Description: This member is invoked periodically to re-issue the connect
//              request. If it will be stopped once the connection is established.
//
// Parameters:  not used
//           
// Returns:     n/a
//
// Notes:       
//------------------------------------------------------------------------------

void rDbgComDECITargetChannel::OnTimerDone
( 
    unsigned int    elapsedtime,
    void*           userData
)
{
    (void) userData;
    (void) elapsedtime;

    //
    // Assert that we are infact in the correct state.
    //
    rAssert( m_InternalState == WaitingForConnectResponse );

    //
    // Lets make up a connect packet and initiate the send of it.
    //
    m_CurrentBytesToWrite = MakeInternalConnectPacket( );
    m_CurrentWriteAddress = m_TxBuffer;

    m_CurrentBytesToRead = sizeof( DECI2_HDR) + sizeof( rDbgComConnectReplyPacket );
    m_CurrentReadAddress = m_RxBuffer;    
   
    if( 0 > sceDeci2ReqSend( m_Socket, DECI2_NODE_HOST) )
	{
    	//
        // Here something is wrong. Close socket, reset things
	    //
        sceDeci2Close( m_Socket );
        m_Socket = -1;
        m_Timer->Release( );
        m_Timer = NULL;

        SetState( Detached, Idle, "Failed to send message to host");
	}
}

//=============================================================================
// Function:    rDbgComDECITargetChannel::DeciEventHandler
//=============================================================================
// Description: This static private member is a used to field events issued
//              by the Deci2 subsystem. This routine is driven under interrupts
//              and should be used with care.
//
// Parameters:  event - what happenned.
//              param - data relavent to the event
//              rDbgComDECITargetChannel* Object.
//
// Returns:     n/a
//
// Notes:       
//------------------------------------------------------------------------------

void rDbgComDECITargetChannel::DeciEventHandler
( 
    int         event,
    int         param,
    void*       Object
)
{
    //
    // Switch on the event and determine what to do. 
    //
    rDbgComDECITargetChannel* pChannel = (rDbgComDECITargetChannel*) Object;

    //
    // Set flag indicating we are under interrupts.
    //
    pChannel->m_UnderInterrupt = true;

    switch( event )
    {
        case DECI2_WRITE :
        {
            //
            // Here we have beed informed we can write. Check if we have data 
            // to send and if attempt to send it.
            //
            if( pChannel->m_CurrentBytesToWrite != 0 )
            {
                //
                // Here we have data. Send what we have.
                //
                int bytesSent = sceDeci2ExSend( pChannel->m_Socket, pChannel->m_CurrentWriteAddress,
                                                pChannel->m_CurrentBytesToWrite );
                if( bytesSent > -1 )
                {
                    //
                    // Here things are fine. Update the numbfer of bytes to sent and
                    // the pointer.
                    //
                    pChannel->m_CurrentBytesToWrite -= bytesSent;
                    pChannel->m_CurrentWriteAddress += bytesSent;
                }
                else
                {
                    //
                    // We have an error.            
                    //
                    pChannel->SignalEvent( EventError );
                }                
            }

            break;
        }

        case DECI2_WRITEDONE :
        {
            //
            // Here we have been informed of a write done. The current count
            // should be zero or he have an error.
            //
            if( pChannel->m_CurrentBytesToWrite != 0 )
            {
                //
                // We have an error.            
                //
                pChannel->SignalEvent( EventError );
            }
            else
            {
                //
                // Check the internal state. If connected, it means we are sending
                // the clients data. Check if we have more to send and if so, get it
                // ready.
                //
                if( pChannel->m_InternalState == Connected )
                {
                    if( pChannel->m_ClientSendSize != 0 )
                    {
                        //
                        // More data to send. Build up the packet.
                        //
                        unsigned int bytesToSend;
                        if( pChannel->m_ClientSendSize > rDbgComMaxDataPacketSize )
                        {                   
                            bytesToSend = rDbgComMaxDataPacketSize;
                        }
                        else
                        {
                            bytesToSend = pChannel->m_ClientSendSize;
                        }

                        pChannel->m_CurrentBytesToWrite = pChannel->MakeDataPacket( bytesToSend, pChannel->m_ClientSendBuffer );
                        pChannel->m_CurrentWriteAddress = pChannel->m_TxBuffer;
        
                        //
                        // Update the amount of client data to send and its pointer.
                        //
                        pChannel->m_ClientSendSize -= bytesToSend;
                        pChannel->m_ClientSendBuffer += bytesToSend;

                        //
                        // Finally initiate the send.
                        //
                        if( 0 > sceDeci2ExReqSend( pChannel->m_Socket, DECI2_NODE_HOST) )
	                    {
    	                    //
                            // Here something is wrong. 
	                        //
                            pChannel->SignalEvent( EventError );
                        }
                    }
	                else
                    {
                        //
                        // Finished send. 
                        //
                        pChannel->SignalEvent( EventTxComplete );
                    }     
                }
                else if( pChannel->m_InternalState == WaitToSendDisconnectMessage )
                {
                   //
                   // Here we finished sending the disconnect message. Just set the event directly.
                   // Prevents wierd reference counting problems.
                   //        
                   pChannel->m_EventMask |= EventTxComplete;
                           
                   // pChannel->SignalEvent( EventTxComplete );
                }          
            }

            break;        
        }
        
        case DECI2_READ :
        {
            //
            // See if we have room. If not signal error.
            //
            if( param > (int) pChannel->m_CurrentBytesToRead )
            {
                //
                // We must read the data, or things go bad.
                //
                sceDeci2ExRecv( pChannel->m_Socket, pChannel->m_RxBuffer, param );

                pChannel->SignalEvent( EventError );
                break;
            }            

            //
            // Read up as much as we were informed of.
            //        
            int bytesRead = sceDeci2ExRecv( pChannel->m_Socket, pChannel->m_CurrentReadAddress,
                                            param );
            
            if( bytesRead > -1 )
            {
                //
                // Here things are fine. Update the numbfer of bytes to receive and
                // the pointer.
                //
                pChannel->m_CurrentBytesToRead -= bytesRead;    
                pChannel->m_CurrentReadAddress += bytesRead;
            }

            break;
        }

        case DECI2_READDONE :
        {
            //
            // Here we have been informed that we have received a packet.
            // Check what it is. It can be a connect reply or data
            // packet.
            //
            if( (rDbgComCommand) pChannel->m_RxBuffer[ sizeof( DECI2_HDR ) ] == CmdConnectReply ) 
            {
                //
                // Here we have a connect reply. Reset the buffer and signal that we
                // have received this message.
                //
                pChannel->SignalEvent( EventRxConnectReply );

                pChannel->m_CurrentBytesToRead = sizeof( DECI2_HDR) + sizeof( rDbgComDataPacket );
                pChannel->m_CurrentReadAddress = pChannel->m_RxBuffer;    
            }
            else if( (rDbgComCommand) pChannel->m_RxBuffer[ sizeof( DECI2_HDR ) ] == CmdDataPacket ) 
            {
                //
                // Zero the receiver and inform cleint of received data.
                //
                pChannel->m_CurrentBytesToRead = 0;

                pChannel->SignalEvent( EventRxPacket );

            }
            else if((rDbgComCommand) pChannel->m_RxBuffer[ sizeof( DECI2_HDR ) ] == CmdReconnect ) 
            {
                //
                // Here we have a reconnect message. If we are connected, report error. Else simply
                // reset the receiver.
                //
                if( pChannel->m_InternalState == Connected )
                {
                    pChannel->m_CurrentBytesToRead = 0;
                    pChannel->SignalEvent( EventError );
                }
                else
                { 
                    //
                    // Just reset buffer.
                    //
                    pChannel->m_CurrentBytesToRead = sizeof( DECI2_HDR) + sizeof( rDbgComConnectReplyPacket );
                    pChannel->m_CurrentReadAddress = pChannel->m_RxBuffer;    
                }
            }
            else
            {
                //
                // Got something we did not want.
                //
                pChannel->SignalEvent( EventError );
  
            }
            
            break;
        }
 
        case DECI2_CHSTATUS :
        {
            //
            // Not sure what to do about status change.
            //
            break;
        }            

        case DECI2_ERROR:
        {
            //
            // Inform system, that an error occurred. Note if we are trying to
            // attach and waiting for the response, let the time-out system
            // handle the error.
            //
            if( pChannel->m_InternalState != WaitingForConnectResponse )
            {
                pChannel->SignalEvent( EventError );
            }
                    
            break;
        }
  
        default :
        {
            break;
        }    
    }

    pChannel->m_UnderInterrupt = false;
}

//=============================================================================
// Function:    rDbgComDECITargetChannel::OnDispatchEvent
//=============================================================================
// Description: This member is invoked when we have received an event from 
//              the ISR.
//
// Parameters:  
//
// Returns:    
//------------------------------------------------------------------------------

void rDbgComDECITargetChannel::OnDispatchCallack( void* userData )
{
    (void) userData;

    //
    // This function is invoked when we receive an event from the
    // ISR. First check if error.
    //
    if( m_EventMask & EventError )
    {
        sceDeci2Close( m_Socket );
        m_Socket = -1;

        if( m_Timer != NULL )
        {
            m_Timer->Release( );
            m_Timer = NULL;
        }
    
        ClearMask( 0xffffffff );

        SetState( Detached, Idle, "Protocol Error occurred");
            
        //
        // Attach again;
        //
        Attach( );
    }
    else if( m_EventMask & EventRxConnectReply )
    {
        //  
        // Here we have been informed of a connect reply, Stop
        // the timer and set new state to connect.
        //  
        if( m_Timer != NULL )
        {
            m_Timer->Release( );
            m_Timer = NULL;
        }
        
        SetState( Attached, Connected, "Connected");
        
        ClearMask( EventRxConnectReply );
    }
    else if(m_EventMask & EventRxPacket )
    {   
        //
        // Here we have received a packet. We better have receive buffers to receive it.
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

        ClearMask( EventRxPacket );

        //
        // Reset the receiver.
        //
        m_CurrentReadAddress = m_RxBuffer;    
        m_CurrentBytesToRead = sizeof( DECI2_HDR) + sizeof( rDbgComDataPacket );

    }
    else if( m_EventMask & EventTxComplete )
    {
        ClearMask( EventTxComplete );

        if( m_InternalState == Connected )
        {
            //
            // Invoke caller that the send is done.
            //
            IRadDbgComChannelSendCallback* pcallback = m_ClientSendCallback;
            m_ClientSendCallback = 0;
            pcallback->OnSendComplete( true );
            pcallback->Release( );
        }
    }
}

//=============================================================================
// Function:    rDbgComDECITargetChannel::SignalEvent
//=============================================================================
// Description: This member is invoked form the event handler to convey an event
//              into the main thread. 
//
// Parameters:  event mask
//
// Returns:     n/a
//
// Notes:       This routine is invoked under interrupts and should be modified
//              with care.
//------------------------------------------------------------------------------

void rDbgComDECITargetChannel::SignalEvent
( 
    unsigned int event
)
{
    //
    // If the socket is equal to -1, we have been closed. Just return.
    //
    if( m_Socket == -1 )
    {
        return;
    }

    //
    // If we already have the event signaled, then don't bother. This would
    // likely only occur if errors were being reported.
    //
    if( (m_EventMask & event) != 0 )
    {
        return;
    }

    m_EventMask |= event;

    m_ParentTarget->m_Dispatcher->QueueCallbackFromInterrupt( this, NULL );    
}

//=============================================================================
// Function:    rDbgComDECITargetChannel::Clear Mask
//=============================================================================
// Description: This function clears the bits specified in a interrupt safe
//              manner.
//
// Parameters:  mask
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComDECITargetChannel::ClearMask
( 
    unsigned int mask 
)
{
    //
    // Bitwise not of the mask.
    //
    mask = ~mask;

    DI();

    m_EventMask &= mask;

    EI( );
}
    
//=============================================================================
// Function:    rDbgComDECITargetChannel::SetState
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

void rDbgComDECITargetChannel::SetState
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
// Function:    rDbgComDECITargetChannel::MakeInternalDisconnectPacket
//=============================================================================
// Description: Helper to build up a an internal disconnect packet.
//
// Parameters:  n/a
//
// Returns:     size of packet to send.
//
// Notes:
//------------------------------------------------------------------------------

unsigned int rDbgComDECITargetChannel::MakeInternalDisconnectPacket
( 
    void
)
{
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
    pPacket->m_Deci2.length = sizeof( DisconnectPacket );
    pPacket->m_Deci2.reserved = 0;
    pPacket->m_Deci2.protocol = m_Protocol;
    pPacket->m_Deci2.source = DECI2_NODE_EE;
    pPacket->m_Deci2.destination = DECI2_NODE_HOST;            
    
    pPacket->m_Request.m_Command = CmdDisconnectRequest;
        
    return( sizeof( DisconnectPacket ) );
}

//=============================================================================
// Function:    rDbgComDECITargetChannel::MakeInternalConnectPacket
//=============================================================================
// Description: Helper to build up a an internal connect packet.
//
// Parameters:  n/a
//
// Returns:     size of packet to send.
//
// Notes:
//------------------------------------------------------------------------------

unsigned int rDbgComDECITargetChannel::MakeInternalConnectPacket
( 
    void
)
{
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
    pPacket->m_Deci2.length = sizeof( ConnectPacket );
    pPacket->m_Deci2.reserved = 0;
    pPacket->m_Deci2.protocol = m_Protocol;
    pPacket->m_Deci2.source = DECI2_NODE_EE;
    pPacket->m_Deci2.destination = DECI2_NODE_HOST;            
    
    pPacket->m_Request.m_Command = CmdConnectRequest;
        
    return( sizeof( ConnectPacket ) );
}

//=============================================================================
// Function:    rDbgComDECITargetChannel::MakeDataPacket
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

unsigned int rDbgComDECITargetChannel::MakeDataPacket
( 
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
   
    DataPacket* pPacket = (DataPacket*) m_TxBuffer;  
        
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
    pPacket->m_Deci2.source = DECI2_NODE_EE;
    pPacket->m_Deci2.destination = DECI2_NODE_HOST;            
    
    pPacket->m_Data.m_Command = CmdDataPacket;
    pPacket->m_Data.m_DataSize = bytesToSend;
    memcpy( pPacket->m_Data.m_Data, pData, bytesToSend );

    return( packetSize );
}

//=============================================================================
// Function:    rDbgComDECITargetChannel::AddRef
//=============================================================================
// Description: This member is used to update the reference count of the object.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
    
void rDbgComDECITargetChannel::AddRef( void )
{
    //
    // This can be called indirectly by the dispacther when we add an event under
    // interrupts. 
    //
    if( m_UnderInterrupt )
    {
        m_ReferenceCount++;
    }
    else
    {
        DI( );
        m_ReferenceCount++;
        EI( );
    }
}

//=============================================================================
// Function:    rDbgComDECITargetChannel::Release
//=============================================================================
// Description: This member is used to update the reference count of the object.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
    
void rDbgComDECITargetChannel::Release( void )
{
    DI( );
    m_ReferenceCount--;

    if( m_ReferenceCount == 0 )
    {
        EI( );

        delete this;
    }   
    else
    {
        EI( );
    }
}

//=============================================================================
// Function:    rDbgComDECITargetChannel::Dump
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
void rDbgComDECITargetChannel::Dump( char* pStringBuffer, unsigned int bufferSize )
{
    sprintf( pStringBuffer, "Object: [rDbgComDECITargetChannel] At Memory Location:[%p]\n", this );
}

#endif
