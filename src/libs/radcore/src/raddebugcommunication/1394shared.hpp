//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        1394shared.hpp
//
// Subsystem:	Radical Debug Communication System
//
// Description:	This file contains the data definitions used to communicated over
//              the 1394 FireWire It is used to create a sockets 
//              implementation. This file is included by the host (win32 ) and 
//              targer ( PS2 ). 
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	Jul 16, 2001
//
// Note:        
//
//=============================================================================

#ifndef	_1394SHARED_HPP
#define _1394SHARED_HPP

//=============================================================================
// Include Files
//=============================================================================

//=============================================================================
// Defintions
//=============================================================================

//
// This defines the maximum number of communincation channels are partion the shared
// memory into. Currently set to three based on understanding of the Foundation Tech
// tool requirements.
//
#define FW1394_CHANNEL_COUNT   4
#define FW1394_CHANNEL_SIZE    (32 * 1024)

//
// We partion the shared memory into channels, each channel consisting of a transmit
// and receive buffer. Each channel is index, zero relative. The following control
// block is designed so that all information about the shared memory can be obtained
// in a single read/write. 
//
struct FW1394ChannelControlBlock
{
    //
    // We need to read and write this automically. Add sequence number to both ends.
    // If they don't match, re-read data.
    //
    unsigned int m_HeadSequenceNumber;

    //
    // Embed CRC of the channel control block. Re-read if invalid.
    //
    unsigned int m_Crc;

    //
    // Keep things aligned to multiple of 64.
    //  
    unsigned int m_Pad[ 4 ];

    //
    // This array maintains a whether the channel is allocated or free. The enrty is
    // set to non-zero, if the channel is allocated, and zero when free. Channels 
    // are allocated when a connect request is processed. 
    //
    unsigned int m_ChannelAllocationMap[ FW1394_CHANNEL_COUNT ];
    
    //
    // Buffers are named from the perspective of the transmitter. A buffer called the host transmit
    // buffer is equivalent to the target receiver buffer. The following contains the number
    // of bytes present in the various buffers. The writer of data can only place data in 
    // the transmitter if the count is zero. Once doing so, the count is set to the non-zero
    // value. The receiver can remove the data when it desires and is responsible for clearing
    // the count.
    //
    unsigned int m_HostTransmitCount[ FW1394_CHANNEL_COUNT ];
    unsigned int m_TargetTransmitCount[ FW1394_CHANNEL_COUNT ];
    
    unsigned int m_HostTransmitCrc[ FW1394_CHANNEL_COUNT ];
    unsigned int m_TargetTransmitCrc[ FW1394_CHANNEL_COUNT ];
    

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
    ControlMessage m_MessageMap[ FW1394_CHANNEL_COUNT ];


    //
    // This member is used to control who owns the shared memory. 
    //
    enum SharedMemoryOwner
    {
        HOST_OWNS_MEMORY    = 0x11111111,
        TARGET_OWNS_MEMORY  = 0x22222222,
        TARGET_RESTARTED    = 0x33333333
    };

    SharedMemoryOwner m_MemoryOwner;

    unsigned int m_TailSequenceNumber;
};

//
// This is the structure of a channel, containing buffers for transmission is each direction.
//
struct FW1394Channel
{
    unsigned char m_HostTransmitBuffer[ FW1394_CHANNEL_SIZE ];
    unsigned char m_TargetTransmitBuffer[ FW1394_CHANNEL_SIZE ];
};

//
// This is the memory map of the shared 1394 memory. 
//
struct FW1394MemoryMap
{
    FW1394ChannelControlBlock  m_ChannelControlBlock;
    FW1394Channel              m_Channel[ FW1394_CHANNEL_COUNT ];   
};
  
#endif