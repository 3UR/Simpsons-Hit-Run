//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        fileprotocol.hpp
//
// Subsystem:	Radical File System - Host Communication Protocol
//
// Description:	This file contains all definitions shared between the host
//				file server and the runtime system.
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	Mar 26, 2001
//
//=============================================================================

#ifndef	FILEPROTOCOL_HPP
#define FILEPROTOCOL_HPP

//=============================================================================
// Include Files
//=============================================================================

//=============================================================================
// Defintions
//=============================================================================

//
// This is the protocol number we use for connect with the host via the IOP
//
#define HOST_FILE_PROTOCOL (0xE123)         

//
// Defines the size of the packet we break reads and writes into. On the Game
// Cube this is set to a smaller value due to limitations in the host i/o services
//

#define HFP_MAX_READWRITE (30 * 1024)

//
// This enumeration defines the various commands we use in the protocol, The
// protocol is very simple. Basically the host PC is the server and the game is 
// the client. All commands are initiate from the client. All transactions consist
// of command/reply pairs.
//
enum HfpCommand
{
    HfsOpen,                    // Open File
    HfsClose,                   // Close File
    HfsDestroy,                 // Destroy File
    HfsRead,                    // Read Data
    HfsWrite,                   // Write Data
    HfsFindFirst,               // Find First matching file
    HfsFindNext,                // Find next matching file
    HfsFindClose                // End a find search (issued automatically)
};

//=============================================================================
// Stucture Declarations
//=============================================================================

//
// These are the open command and reply structures.
//
struct HfpOpenCmd
{
    HfpCommand      m_Command;
    unsigned int    m_Flags;                // Controls creation
    unsigned int    m_WriteAccess;            
    char            m_FileName[ 256 + 1 ];  // file to open, null terminated.
};

struct HfpOpenRpy
{
    HfpCommand      m_Command;
    void*           m_Handle;               // 0 = open on target, 0xffffffff = failed, others are valid
    unsigned int    m_Size;                 // Size of file
};

//
// These are used to close command and replys.
//
struct HfpCloseCmd
{
    HfpCommand      m_Command;
    void*           m_Handle;               // File handle
};

struct HfpCloseRpy
{
    HfpCommand      m_Command;
};

//
// This is the read command and reply structures.
//
struct HfpReadCmd
{
    HfpCommand      m_Command;
    void*           m_Handle;               // File handle
    unsigned int    m_Position;             // Where to read
    unsigned int    m_NumBytes;             // Number of bytes to read
};

struct HfpReadRpy
{
    HfpCommand      m_Command;
    unsigned int    m_NumBytes;             // Number of bytes read
    unsigned int    m_Pad[ 2 ];             // Aligns to 16 for DMA
    unsigned char   m_Data[ HFP_MAX_READWRITE ];
};

//
// This is the write command and reply structures.
//
struct HfpWriteCmd
{
    HfpCommand      m_Command;
    void*           m_Handle;               // File handle
    unsigned int    m_Position;             // Where to Write
    unsigned int    m_NumBytes;             // Number of bytes to write
    unsigned char   m_Data[ HFP_MAX_READWRITE ]; // Data
};

struct HfpWriteRpy
{
    HfpCommand      m_Command;
    unsigned int    m_NumBytes;             // Number of bytes written
    unsigned int    m_NewSize;              
};

struct HfpDestroyCmd
{
    HfpCommand      m_Command;
    char            m_FileName[ 256 + 1 ];  // file to destroy, null terminated.
};

struct HfpDestroyRpy
{
    HfpCommand      m_Command;
    unsigned int    m_Result;               // Non zero error
};

//
// These are the directory enumeration command and reply structures.
//
struct HfpFindFirstCmd
{
    HfpCommand      m_Command;
    char            m_SearchSpec[ 256 + 1 ];  // Path with wildcards
};

struct HfpFindFirstRpy
{
    HfpCommand      m_Command;
    unsigned int    m_Handle;               // Search handle, 0xffffffff = failed
    unsigned int    m_Type;                 // IsDirectory, IsFile or IsDone
    char            m_Name[ 256 + 1 ];      // Matching file name
};

struct HfpFindNextCmd
{
    HfpCommand      m_Command;
    unsigned int    m_Handle;               // Search handle
};

struct HfpFindNextRpy
{
    HfpCommand      m_Command;
    unsigned int    m_Result;               // 0xffffffff = failed, anything else = success
    unsigned int    m_Type;                 // IsDirectory, IsFile or IsDone
    char            m_Name[ 256 + 1 ];      // Matching file name
};

struct HfpFindCloseCmd
{
    HfpCommand      m_Command;
    unsigned int    m_Handle;               // Search handle
};

struct HfpFindCloseRpy
{
    HfpCommand      m_Command;
    unsigned int    m_Result;               // 0xffffffff = failed, anything else = success
};

#endif 
