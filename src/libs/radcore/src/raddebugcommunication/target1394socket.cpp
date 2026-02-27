//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        target1394socket.cpp
//
// Subsystem:	Radical Debug Communication System
//
// Description:	This file contains the an implementation of sockets using
//              the 1394 FireWire system. It in no way is a true
//              implementation of the socket definition. It makes many
//              assumptions about is use by the debug communication system.
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	Jul 16, 2001
//
// Notes:       This file is intented to be built only for the PS2EE
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#ifdef SN_TCPIP
extern "C"
{
#include "../../../Sdks/sntcpip/inc/snsocket.h"
#include "../../../Sdks/sntcpip/inc/sneeutil.h"
#include "../../../Sdks/sntcpip/inc/snskdefs.h"
}
#endif
#include <string.h>
#include <stdio.h>
#include <raddebug.hpp>
#include <radtime.hpp>
#include <radplatform.hpp>
#include <rad1394.hpp>
#include <radmemorymonitor.hpp>
#include "target1394socket.hpp"
#include <eekernel.h>

//=============================================================================
// Local Definitions
//=============================================================================

//const int INVALID_SOCKET = -1;
//#define WSAEWOULDBLOCK  EWOULDBLOCK
//#define WSAENOTCONN     ENOTCONN

//
// This constant governs how long we will wait before assuming the host is 
// not longer present. Time is express in milliseconds
//
#define HOST_COMMUNICATION_TIMEOUT          10000

//=============================================================================
// Statics
//=============================================================================

//
// These don't need to be statics other than the fact that the compiler has alignment
// problems.
//
static FW1394ChannelControlBlock m_ChannelControlBlock __attribute__((aligned (64)));

static bool g_Initialized = false;
static unsigned int g_SequenceNumber = 1;
static bool         g_LastSequenceNumber = 9542371;

//=============================================================================
// Member Functions
//=============================================================================

//=============================================================================
// Function:    CTarget1394Socket::CTarget1394Socket
//=============================================================================
// Description: Initialize the 1394 target system. 
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

CTarget1394Socket::CTarget1394Socket( void )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "CTarget1394Socket" );
    //
    // Been burned by alignment many times, make sure compiler didn't screw things up.
    //
    rAssert( (((unsigned int) &m_ChannelControlBlock ) % 64 ) == 0 );
    rAssert( (sizeof( m_ChannelControlBlock ) % 64 ) == 0 );

    m_SocketArray = (Socket*) m_SocketArraySpace;
   
    if( (unsigned int) m_SocketArraySpace & 0x3f )
    {
        m_SocketArray = (Socket*) ( ((unsigned int)m_SocketArraySpace & 0xffffffc0) + 0x40 );
    }  

    rAssert( (((unsigned int) m_SocketArray ) % 64 ) == 0 );

    //
    // Initialized our local data structures to all free and indicate that 
    // we are not listening.
    //
    for( unsigned int i = 0 ; i < FW1394_CHANNEL_COUNT ; i++ )
    {
        m_SocketArray[ i ].m_BytesInRxBuffer = 0;      
        m_SocketArray[ i ].m_BytesInTxBuffer = 0;   
        m_SocketArray[ i ].m_State = Free;
    }

    m_ListenSocketIndex = FW1394_CHANNEL_COUNT + 1;
    m_OpenSocketCount = 0;

    //
    // Lets intialize the 1394 shared memory system.
    //
    rad1394Initialize( );
    
    //
    // Get interface pointer to the slave interface. Add ref as we will maintain this pointer.
    //
    m_pIRad1394Slave = rad1394Get( );
    rAssert( m_pIRad1394Slave != NULL );
    radAddRef( m_pIRad1394Slave, this );

    //
    // Set up the shared memory region.
    //
    m_pIRad1394Slave->SetMemorySpace( sizeof( FW1394MemoryMap ) );

    //
    // Set up pointer to access shared 1394 memory.
    //
    m_p1394Memory = (FW1394MemoryMap*) 0;

    //
    // Set up time of last contact.
    //  
    m_TimeOfLastContact = radTimeGetMilliseconds( );
}

//=============================================================================
// Function:    CTarget1394Socket::~CTarget1394Socket( void )
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

CTarget1394Socket::~CTarget1394Socket( void )
{
    radRelease( m_pIRad1394Slave, this );

    rad1394Terminate( );
}

//=============================================================================
// Function:    CTarget1394Socket::socket
//=============================================================================
// Description  Create a socket.
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------
    
int CTarget1394Socket::socket
( 
    int     af, 
    int     type,
    int     protocol
)
{
    //
    // Nothing interesting happens when a socket is created. Just return
    // a socket id that is not one of out valid socket ids.
    //
    m_OpenSocketCount++;

    return( FW1394_CHANNEL_COUNT + 1);
}
    
//=============================================================================
// Function:    CTarget1394Socket::setsockopt
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CTarget1394Socket::setsockopt
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
// Function:    CTarget1394Socket::bind
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CTarget1394Socket::bind
( 
    int     sock,
    struct  sockaddr* addr,
    int     addrlen
)
{
    //
    // Make sure the socket is our listening socket and that we have not already
    // openned our file used for communicaition.
    //
    rAssert( sock == FW1394_CHANNEL_COUNT + 1 );
    
    return( 0 );    
};

//=============================================================================
// Function:    CTarget1394Socket::listen
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CTarget1394Socket::listen( int sock, int backlog)
{
    //
    // Make sure the socket is our listen socket and we are not already listening.
    //
    rAssert( sock == FW1394_CHANNEL_COUNT + 1 );
    rAssert( m_ListenSocketIndex == FW1394_CHANNEL_COUNT + 1);      
    
    //
    // Find a free socket and set state to listening.
    //
    for( unsigned int i = 0 ; i < FW1394_CHANNEL_COUNT ; i++ )
    {
        //
        // Find a free socket.
        //
        if( m_SocketArray[ i ].m_State ==  Free )
        {
            //
            // We have found a free one. Set state to listening and 
            //
            m_SocketArray[ i ].m_State = Listen;
            m_SocketArray[ i ].m_BytesInRxBuffer = 0;      
            m_SocketArray[ i ].m_BytesInTxBuffer = 0;   
            m_ListenSocketIndex = i;
            return( 0 );

        }
    }
    return( -1 );
}

//=============================================================================
// Function:    CTarget1394Socket::accept
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CTarget1394Socket::accept( int sock, struct sockaddr* addr, int* addrlen)
{
    //
    // This routine is polled by the higher levels looking for a connection.
    // Lets use it to drive the host communicaiton.
    //
    Do1394Communication( );

    rAssert( sock == FW1394_CHANNEL_COUNT + 1);
    rAssert( m_ListenSocketIndex != FW1394_CHANNEL_COUNT + 1 );      
    
    //
    // Check if connected or still listening.
    //
    if( m_SocketArray[ m_ListenSocketIndex ].m_State == Listen )
    {
        return( INVALID_SOCKET );
    }
    else if( m_SocketArray[ m_ListenSocketIndex ].m_State == Connected )
    {
        //
        // Here we have acheived a connection. Return socket adjusted by one.
        //
        int connectedSocket = m_ListenSocketIndex + 1;
        m_ListenSocketIndex = FW1394_CHANNEL_COUNT + 1;
        
        //
        // Listen on another socket.
        //
        CTarget1394Socket::listen( sock, 0 );

        m_OpenSocketCount++;

        return( connectedSocket );
    }

    rAssert( false );
       
    return( INVALID_SOCKET );
}    

//=============================================================================
// Function:    CTarget1394Socket::connect
//=============================================================================
// Description: Connect not allowed on target
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CTarget1394Socket::connect( int sock, const struct sockaddr* addr, int addrlen )
{
    return( -1 );
}

//=============================================================================
// Function:    CTarget1394Socket::closesocket
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CTarget1394Socket::closesocket( int sock )
{
    m_OpenSocketCount--;

    //
    // Check if it is a socket we are listening with.
    //
    if( sock != FW1394_CHANNEL_COUNT + 1)
    {
        //  
        // Adjust to compensate for value returned at accept.
        //
        sock--;

        //
        // Check if connected. If not don't worry.
        //
        if( m_SocketArray[ sock ].m_State == Connected )
        {
            //
            // Check it we have data to send. If so set pending disconnect.
            // Else set disconnect.
            //
            if( m_SocketArray[ sock ].m_BytesInTxBuffer == 0 )
            {
                m_SocketArray[ sock ].m_State = Disconnected;
            }
            else
            {
                m_SocketArray[ sock ].m_State = PendingDisconnect;
            }
        }
        else
        {
            m_SocketArray[ sock ].m_State = Free;
        }                   
    }
    else
    {
        //
        // Here we have the listen socket. Lets change state to free.
        //
        m_SocketArray[ m_ListenSocketIndex ].m_State = Free;
        m_ListenSocketIndex = FW1394_CHANNEL_COUNT;
    }


    //
    // Check if we should shutdown. When the open socket count goes to zero,
    // this is our indication we are done.
    //
    if( m_OpenSocketCount == 0 )
    {
        //
        // Lets wait a few moments spinning to ensure the host sees the 
        // final state.
        //
        unsigned int startTime = radTimeGetMilliseconds( );
        while( radTimeGetMilliseconds( ) - startTime < 3000  )
        {
            Do1394Communication( );
        }
    }

    return( 0 );
}

//=============================================================================
// Function:    CTarget1394Socket::lasterror
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CTarget1394Socket::lasterror( int sock )
{
    //  
    // Adjust to compensate for value returned at accept.
    //
    sock--;

    //
    // This routine gets the last error. It assumes that only called when 
    // a send or receive fails. Return would block if connected.
    //
    if( m_SocketArray[ sock ].m_State == Connected )
    {
        return( WSAEWOULDBLOCK );
    }
    else
    {
        return( WSAENOTCONN );
    }
}

//=============================================================================
// Function:    CTarget1394Socket::recv
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CTarget1394Socket::recv( int sock, char* buf, int len, int flags)
{
    //  
    // Adjust to compensate for value returned at accept.
    //
    sock--;

    //
    // This routine is polled. Use it to drive 1394 communication
    //
    Do1394Communication( );

    //
    // Make sure connecteded. If not return -1;
    //
    if( m_SocketArray[ sock ].m_State != Connected )
    {
        return( -1 );
    }
    
    //
    // See if any data. If so return as much as the caller asked for.
    //
    if( m_SocketArray[ sock ].m_BytesInRxBuffer == 0 )
    {
        return( 0 );
    }
    
    if( len >= (int) m_SocketArray[ sock ].m_BytesInRxBuffer )
    {
        len = m_SocketArray[ sock ].m_BytesInRxBuffer;
        memcpy( buf, m_SocketArray[ sock ].m_RxBuffer, len );
        m_SocketArray[ sock ].m_BytesInRxBuffer = 0;
    }
    else
    {
        m_SocketArray[ sock ].m_BytesInRxBuffer -= len;
        memcpy( buf, m_SocketArray[ sock ].m_RxBuffer, len );
        
        //
        // This is lazy, but will revisit if performance is bad.
        //
        memmove( m_SocketArray[ sock ].m_RxBuffer, &m_SocketArray[ sock ].m_RxBuffer[ len ], m_SocketArray[ sock ].m_BytesInRxBuffer );
    }
    return( len );
}

//=============================================================================
// Function:    CTarget1394Socket::send
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CTarget1394Socket::send( int sock, const char* buf, int len, int flags)
{
    //  
    // Adjust to compensate for value returned at accept.
    //
    sock--;

    //
    // Make sure connecteded. If not return -1;
    //
    if( m_SocketArray[ sock ].m_State != Connected )
    {
        return( -1 );
    }
    
    //
    // See if any data. If so return as much as the caller asked for.
    //
    if( m_SocketArray[ sock ].m_BytesInTxBuffer != 0 )
    {
        return( 0 );
    }

    if( len > FW1394_CHANNEL_SIZE )
    {
        len = FW1394_CHANNEL_SIZE;
    }

    memcpy( m_SocketArray[ sock ].m_TxBuffer, buf, len );
    m_SocketArray[ sock ].m_BytesInTxBuffer = len;

    //
    // This routine is polled. Use it to drive host communication.
    //
    Do1394Communication( );

    return( len );
}

//=============================================================================
// Function:    CTarget1394Socket::RoundUpTo16
//=============================================================================
// Description: Member to round number up to multiple of 16
//
// Parameters:  unsinged int
//              
// Returns:     converted usinged int.
//
// Notes:       
//------------------------------------------------------------------------------

unsigned int CTarget1394Socket::RoundUpTo16
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
// Function:    CTarget1394Socket::Do1394Communication 
//=============================================================================
// Description: This routine is invoked perfiodically to drive the communication
//              with the host. 
//              
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

void CTarget1394Socket::Do1394Communication( void )
{
    unsigned int i;

    if( !g_Initialized )
    {
        //
        // First time set state of control block to invalid. This will cause host to 
        // re-initialize.
        //   
        g_Initialized = true;

        for( unsigned int i = 0 ; i < FW1394_CHANNEL_COUNT ; i++ )
        {
            m_ChannelControlBlock.m_ChannelAllocationMap[ i ] = 0;
            m_ChannelControlBlock.m_HostTransmitCount[ i ] = 0;
            m_ChannelControlBlock.m_TargetTransmitCount[ i ] = 0;
            m_ChannelControlBlock.m_MessageMap[ i ] = FW1394ChannelControlBlock::NullMessage;
        }
        
        m_ChannelControlBlock.m_HeadSequenceNumber = g_SequenceNumber;
        m_ChannelControlBlock.m_TailSequenceNumber = g_SequenceNumber;
        g_SequenceNumber++;

        m_ChannelControlBlock.m_MemoryOwner = FW1394ChannelControlBlock::TARGET_RESTARTED;

        m_ChannelControlBlock.m_Crc = 0;
        m_ChannelControlBlock.m_Crc = MakeCrc( &m_ChannelControlBlock, sizeof( m_ChannelControlBlock ) );
    
        m_pIRad1394Slave->WriteMemoryAsync( (unsigned int) &m_p1394Memory->m_ChannelControlBlock,
                                        sizeof( m_ChannelControlBlock ), &m_ChannelControlBlock, true );
    }

ReRead:
    m_pIRad1394Slave->ReadMemoryAsync( (unsigned int) &m_p1394Memory->m_ChannelControlBlock,
                                       sizeof( m_ChannelControlBlock ), &m_ChannelControlBlock, true ); 

    unsigned int oldCrc = m_ChannelControlBlock.m_Crc;
    m_ChannelControlBlock.m_Crc = 0;
    if( oldCrc != MakeCrc( &m_ChannelControlBlock, sizeof( m_ChannelControlBlock ) ) )
    {
        goto ReRead;
    }

    //
    // Lets see if the ownership has been transfered to us. If not try again later provided
    // a time-out has not occurred.
    //
    if( m_ChannelControlBlock.m_MemoryOwner != FW1394ChannelControlBlock::TARGET_OWNS_MEMORY)
    {
        //
        // Here ownership has not been passed to us yet. See if a time-out has occurred.
        //
        if( radTimeGetMilliseconds( ) - m_TimeOfLastContact > HOST_COMMUNICATION_TIMEOUT )
        {
            //
            // Here the host has vanished or is not present. Set the state of
            // all our non-listening sockets to remotely disconnected.
            //
            for( i = 0 ; i < FW1394_CHANNEL_COUNT ; i++ )
            {
                if( (m_SocketArray[ i ].m_State != Listen) && (m_SocketArray[ i ].m_State != Free) )
                {
                    m_SocketArray[ i ].m_BytesInRxBuffer = 0;      
                    m_SocketArray[ i ].m_BytesInTxBuffer = 0;   

                    if( m_SocketArray[ i ].m_State == Connected )
                    {
                        m_SocketArray[ i ].m_State = RemoteDisconnect;
                    }
                    else
                    {
                        m_SocketArray[ i ].m_State = Free;
                    }
                }
            }
        }
        return;
    }
    
    //
    // Check sequence number. If not the same, re-read control block.
    //
    if( m_ChannelControlBlock.m_HeadSequenceNumber != m_ChannelControlBlock.m_TailSequenceNumber )
    {
        goto ReRead;
    }

    if( g_LastSequenceNumber == m_ChannelControlBlock.m_HeadSequenceNumber )
    {
        goto ReRead;
    }

    g_LastSequenceNumber = m_ChannelControlBlock.m_HeadSequenceNumber;
  
    //
    // Here we have the control block. First lets see if we have any data to read.
    //
    m_ReadWriteIndex = 0;

    while( ReadData( ) )
    {
        //
        // Update our local info with the amount received and clear the control block.
        //
        m_SocketArray[ m_ReadWriteIndex ].m_BytesInRxBuffer = m_ChannelControlBlock.m_HostTransmitCount[ m_SocketArray[ m_ReadWriteIndex ].m_1394Channel ];
        m_ChannelControlBlock.m_HostTransmitCount[ m_SocketArray[ m_ReadWriteIndex ].m_1394Channel ] = 0;

        m_ReadWriteIndex++;
    }

    //
    // Here there was not data to read. Try to see if we can send data.
    //
    m_ReadWriteIndex = 0;

    while( WriteData( ) )
    {
        //
        // Update the control block and zero our local transmit count.
        //
        m_ChannelControlBlock.m_TargetTransmitCount[ m_SocketArray[ m_ReadWriteIndex ].m_1394Channel ] = 
                        m_SocketArray[ m_ReadWriteIndex ].m_BytesInTxBuffer;
        m_SocketArray[ m_ReadWriteIndex ].m_BytesInTxBuffer = 0;
        
        m_ReadWriteIndex++;
    }

    //
    // If we get here we cound not read or there was nothing to write. Lets resolve
    // state.
    //
    ResolveState( );

    //
    // Done. 
    //
    m_TimeOfLastContact = radTimeGetMilliseconds( );
}

//=============================================================================
// Function:    CTarget1394Socket::ReadData
//=============================================================================
// Description: This routine is invoked to initiate the receipt to data in
//              the from shared memory. 
//              
// Parameters:  n/a
//              
// Returns:     returns true is receice outstanding.
//                      false if done     
//
//------------------------------------------------------------------------------
     
bool CTarget1394Socket::ReadData( void )
{
    //
    // Iterate through channels, seeing if connected and have room to take received data
    // and that there is data.
    //
    for( unsigned int i = m_ReadWriteIndex ; i < FW1394_CHANNEL_COUNT ; i++ )
    {
        if( m_SocketArray[ i ].m_State == Connected )
        {
            unsigned int bytesInReceiver = m_ChannelControlBlock.m_HostTransmitCount[ m_SocketArray[ i ].m_1394Channel ];
                   
            if( (bytesInReceiver != 0) && (m_SocketArray[ i ].m_BytesInRxBuffer == 0) )  
            {
                //
                // Here we have some data to read and a place to put it.
                // Initiate the read. We don't update state until the read completes. Make sure
                // read is a multiple of 16, always rounding up as necessary.
                //
                m_ReadWriteIndex = i;
            
                m_pIRad1394Slave->ReadMemoryAsync( (unsigned int) &m_p1394Memory->m_Channel[ m_SocketArray[ i ].m_1394Channel ].m_HostTransmitBuffer,
                                                   RoundUpTo16( bytesInReceiver ), m_SocketArray[ i ].m_RxBuffer );

                return( true );
            }
        }
    }

    //
    // We are done. Return false.
    //
    return( false );
}    

//=============================================================================
// Function:    CTarget1394Socket::WriteData
//=============================================================================
// Description: This routine is invoked to initiate the transmit of data into
//              shared memory. 
//              
// Parameters:  n/a
//              
// Returns:     returns true is write outstanding.
//                      false if done     
//
//------------------------------------------------------------------------------

bool CTarget1394Socket::WriteData( void )
{
    for( unsigned int i = m_ReadWriteIndex ; i < FW1394_CHANNEL_COUNT ; i++ )
    {
        if( (m_SocketArray[ i ].m_State == Connected) || (m_SocketArray[ i ].m_State == PendingDisconnect) )
        {
            unsigned int bytesInTransmitter = m_ChannelControlBlock.m_TargetTransmitCount[ m_SocketArray[ i ].m_1394Channel ];
                   
            if( (bytesInTransmitter == 0) && (m_SocketArray[ i ].m_BytesInTxBuffer != 0) )
            {
                //
                // Here we have some data to write and a place to put it.
                // Initiate the write. 
                //
                m_ReadWriteIndex = i;

                m_pIRad1394Slave->WriteMemoryAsync( (unsigned int) &m_p1394Memory->m_Channel[ m_SocketArray[ i ].m_1394Channel ].m_TargetTransmitBuffer,
                                                    RoundUpTo16( m_SocketArray[ i ].m_BytesInTxBuffer ), m_SocketArray[ i ].m_TxBuffer ); 
            
                m_ChannelControlBlock.m_TargetTransmitCrc[ m_SocketArray[ i ].m_1394Channel ] =     
                    MakeCrc( m_SocketArray[ i ].m_TxBuffer, m_SocketArray[ i ].m_BytesInTxBuffer );
                
                     
                return( true );
            }
        }
    }
    return( false );
}

//=============================================================================
// Function:    CTarget1394Socket::ResolveState
//=============================================================================
// Description: This routine is used to resolve the state based on mesages
//              and messages we send.
//              
// Parameters:  
//
//------------------------------------------------------------------------------

void CTarget1394Socket::ResolveState( void )
{
    //
    // First interate over our local channels. We do not initaite connections,
    // so we our local state can only be free, listen, connected, pending disconnect, and
    // disconnect. Lets first deal with established connections.
    //
    for( unsigned int i = 0; i < FW1394_CHANNEL_COUNT ; i++ )
    {
        if( (m_SocketArray[ i ].m_State == Connected) || 
            (m_SocketArray[ i ].m_State == PendingDisconnect) ||
            (m_SocketArray[ i ].m_State == Disconnected) )
        {
            //
            // Here we have a connection or are about to shut one down. We have 
            // a valid index into the message table. The only valid messages from the
            // host are disconnect or null message.
            //
            unsigned int channel = m_SocketArray[ i ].m_1394Channel;
            FW1394ChannelControlBlock::ControlMessage message = m_ChannelControlBlock.m_MessageMap[ channel ];
            rAssert( (message == FW1394ChannelControlBlock::NullMessage) || (message == FW1394ChannelControlBlock::DisconnectMessage ) );
 
            //
            // Lets let the host disconnect message take priority. Set our state to disconnected.
            //
            if( message == FW1394ChannelControlBlock::DisconnectMessage )
            {
                //
                // Clear the message entry, free the channel, and set our local state. 
                // 
                m_ChannelControlBlock.m_MessageMap[ channel ] = FW1394ChannelControlBlock::NullMessage;
                m_ChannelControlBlock.m_ChannelAllocationMap[ channel ] = 0;
                if( m_SocketArray[ i ].m_State == Connected )
                {
                    //
                    // Set it disconnected. The client will free the entry,
                    //
                    m_SocketArray[ i ].m_State = RemoteDisconnect;
                }
                else
                {
                    m_SocketArray[ i ].m_State = Free; 
                }
            }
            else
            {
                //
                // No message from the host. Check our state. If connected, do nothing. If
                // disconnected, send disconnect message. If Pending disconnect, and tx buffer
                // empty, send disconnect message. Host free channel when sees message
                //
                if( (m_SocketArray[ i ].m_State == Disconnected) ||
                    ( (m_SocketArray[ i ].m_State == PendingDisconnect) && (m_SocketArray[ i ].m_BytesInTxBuffer == 0) ) )
                {
                    m_ChannelControlBlock.m_MessageMap[ channel ] = FW1394ChannelControlBlock::DisconnectMessage;
                    m_SocketArray[ i ].m_State = Free; 
                }                                
            }
        }  
    }

    //
    // Here we have dealt with all the established or disconnected sockets. Now lets deal with
    // the listening and connecting sockets. Lets iterate over all allocated shared memory channels.
    //
    for( unsigned int channel = 0 ; channel < FW1394_CHANNEL_COUNT ; channel++ )
    {
        if( m_ChannelControlBlock.m_ChannelAllocationMap[ channel ] != 0 )
        {
            if( FW1394ChannelControlBlock::ConnectRequestMessage == m_ChannelControlBlock.m_MessageMap[ channel ] )
            {
                //
                // Here we have a connect request. Lets check if we have a listen. If so, accept it,
                // Otherwise reject it.
                //
                for( unsigned int j = 0 ; j < FW1394_CHANNEL_COUNT ; j++ )
                {
                    if( m_SocketArray[ j ].m_State == Listen )
                    {
                        //
                        // Here we have a listen. Set state to connected, and update message.
                        //
                        m_SocketArray[ j ].m_State = Connected;
                        m_SocketArray[ j ].m_1394Channel = channel;
                        m_ChannelControlBlock.m_MessageMap[ channel ] = FW1394ChannelControlBlock::ConnectAcceptMessage;

                        break;  
                    }
                }
                    
                //
                // Check the message. If still connect request, did not find listen and reject the connect.
                //
                if( FW1394ChannelControlBlock::ConnectRequestMessage == m_ChannelControlBlock.m_MessageMap[ channel ] )
                {
                    m_ChannelControlBlock.m_MessageMap[ channel ] = FW1394ChannelControlBlock::ConnectRejectMessage;
                }
            }
        }                         
    }

    //
    // Set the state so that the onwer is the host and initiate the write of the control block. Write it
    // in an atomic manner. Set signature as well to ensure valid packet.
    //
    m_ChannelControlBlock.m_HeadSequenceNumber = g_SequenceNumber;
    m_ChannelControlBlock.m_TailSequenceNumber = g_SequenceNumber;
    g_SequenceNumber++;

    m_ChannelControlBlock.m_MemoryOwner = FW1394ChannelControlBlock::HOST_OWNS_MEMORY;
 
    m_ChannelControlBlock.m_Crc = 0;
    m_ChannelControlBlock.m_Crc = MakeCrc( &m_ChannelControlBlock, sizeof( m_ChannelControlBlock ) );


    m_pIRad1394Slave->WriteMemoryAsync( (unsigned int) &m_p1394Memory->m_ChannelControlBlock,
                                        sizeof( m_ChannelControlBlock ), &m_ChannelControlBlock, true );
}

//=============================================================================
// Function:    CTarget1394Socket::MakeCRC
//=============================================================================
// Description: Used to create a CRC. 
//              
// Parameters:  
//
//------------------------------------------------------------------------------

unsigned int CTarget1394Socket::MakeCrc( void* pBuffer, unsigned int size )
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

