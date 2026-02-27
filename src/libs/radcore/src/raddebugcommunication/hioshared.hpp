//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        hioshared.hpp
//
// Subsystem:	Radical Debug Communication System
//
// Description:	This file contains the data definitions used to communicated over
//              the gamecube host i/o system. It is used to create a sockets 
//              implementation. This file is included by the host (win32 ).
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	May 24, 2001
//
// Note:        It is the responsibility of the target to convert to and
//              from little endian to big endien, as the shared memory is defined
//              to use little endien.
//
//=============================================================================

#ifndef	HIOSHARED_HPP
#define HIOSHARED_HPP

//=============================================================================
// Include Files
//=============================================================================

//=============================================================================
// Defintions
//=============================================================================

//
// This defines the maximum number of communincation channels are partion the shared
// memory into. Currently set to six based on understanding of the Foundation Tech
// tool requirements.
//
#define HIO_CHANNEL_COUNT   6

//
// This size is arrive at trying to maximimize the packet size while maintaining
// the limit of the shared memory. The size is equal to the largest packet
// possible to send by the upper layers. 31K + 16 bytes. Make it 16 bytes bigger to
// satisfy HIO multiple of 64 bytes size requirement,

#define HIO_CHANNEL_SIZE    ((31 * 1024) + 64 )

//
// This constant governs the number of both transmit and receive buffers.
//
#define HIO_BUFFER_COUNT 2

//
// Flag used to indicate target owns shared memory.
//
#define HIO_TARGETOWNSMEMORY		0x11111111
#define HIO_HOSTOWNSMEMORY			0x22222222


//
// The Host I/O shared memory is 128k ( less 1280 bytes reserved for the hio system). 
// We partion the shared memory into channels, each channel consisting of a transmit
// and receive buffer. Each channel is index, zero relative. The following control
// block is designed so that all information about the shared memory can be obtained
// in a single read/write. 
//
struct HioChannelControlBlock
{

    //
    // This array maintains a whether the channel is allocated or free. The enrty is
    // set to non-zero, if the channel is allocated, and zero when free. Channels 
    // are allocated when a connect request is processed. We do not worry about
    // endianess of this field as it is zero or non-zero only.
    //
    unsigned int m_ChannelAllocationMap[ HIO_CHANNEL_COUNT ];
    
    //
    // Buffers are named from the perspective of the transmitter. A buffer called the host transmit
    // buffer is equivalent to the target receiver buffer. The following contains the number
    // of bytes present in the various buffers. The writer of data can only place data in 
    // the transmitter if the count is zero. Once doing so, the count is set to the non-zero
    // value. The receiver can remove the data when it desires and is responsible for clearing
    // the count.
    //
    unsigned int m_HostTransmitCount[ HIO_CHANNEL_COUNT ];
    unsigned int m_TargetTransmitCount[ HIO_CHANNEL_COUNT ];
    
    //
    // This is used to arbrite the the tranmit and receive buffers. There are two of each 
    // and when used, the channel index of the user is placed in it. If not in use, -1
    // is stored. If there are more than one channel wanting to send data, they are
    // arbritrated round robin,
    //
    unsigned int m_HostTransitBufferOwner[ HIO_BUFFER_COUNT ];
    unsigned int m_TargetTransitBufferOwner[ HIO_BUFFER_COUNT ];
     
    //
    // This data structure is used to transit control messages. They must always be processed
    // 
    enum ControlMessage
    {
        NullMessage,                    // No message
        ConnectRequestMessage,          // Connect Message
        ConnectAcceptMessage,           // Connection accepted
        ConnectRejectMessage,           // Connection rejected 
        DisconnectMessage               // Disconnect
    };
    ControlMessage m_MessageMap[ HIO_CHANNEL_COUNT ];

    //
    // This member is used to control who owns the shared memory.
    //
    unsigned int m_TargetOwnsMemory;

    //
    // Padding keeping size aligned.
    //
    unsigned int m_Padding[ 3 ];
};

//
// This is the memory map of the shared hio memory. The HIO api dictates that reads occur at
// multiples of 32 bytes, aligned to 64. 
//
struct HioMemoryMap
{
    unsigned char           m_SystemReserved[ 0x500 ];
    HioChannelControlBlock  m_ChannelControlBlock;
    unsigned char m_HostTransmitBuffer[HIO_BUFFER_COUNT] [ HIO_CHANNEL_SIZE ];
    unsigned char m_TargetTransmitBuffer[HIO_BUFFER_COUNT] [ HIO_CHANNEL_SIZE ];
};
  
#endif