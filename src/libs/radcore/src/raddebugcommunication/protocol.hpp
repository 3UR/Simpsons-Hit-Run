//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        protocol.Hpp
//
// Subsystem:	Radical Debug Communication
//
// Description:	This file contains all definition of the packets definitions
//              specific to the debug com protocol.
//              Contains constants shared between the host and the target as
//              well.
//
// Revisions:	14-Mar-2001 Creation
//
// Notes:       
//
//=============================================================================

#ifndef	PROTOCOL_HPP
#define PROTOCOL_HPP

//=============================================================================
// Include Files
//=============================================================================

//=============================================================================
// Forward Class Declarations
//=============================================================================

//=============================================================================
// Defintions
//=============================================================================

//
// This constant governs the maximum number of protocols supported by the
// system.
//
const unsigned int MaxProtocols = 16;

//
// This constant governs the maximum length the computer name string can be.
//
const unsigned int MaxComputerName = 31;

//
// This constant governs the maximum amount of data send in any one packet.
// Make sure it is kept to a multiple of 4.
//
const unsigned int rDbgComMaxDataPacketSize = 31 * 1024;

//=============================================================================
// Class Declarations
//=============================================================================

//
// This enumeration is used to define the packet idenifiers used in our protocol,
//
enum rDbgComCommand
{
    CmdConnectRequest,                     // Indentifies a connect request packet
    CmdConnectReply,                       // Identifies a connent reply packet
    CmdReconnect,                          // Reconnect
    CmdDisconnectRequest,                  // Sent by target
    CmdDataPacket                          // Data packet
};

//
// Packet defintions
//
struct rDbgComConnectRequestPacket
{
    rDbgComCommand  m_Command;
};
   
struct rDbgComConnectReplyPacket
{
    rDbgComCommand  m_Command;
};

struct rDbgComReconnectPacket
{
    rDbgComCommand  m_Command;
};

struct rDbgComDisconnectRequestPacket
{
    rDbgComCommand  m_Command;
};


//
// Data packet defintion
//
struct rDbgComDataPacket
{
    rDbgComCommand  m_Command;
    unsigned int    m_DataSize;         // Actual amount of data
    unsigned char   m_Data[ rDbgComMaxDataPacketSize ];
};     

#endif


