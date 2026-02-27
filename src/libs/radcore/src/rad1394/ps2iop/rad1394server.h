//=============================================================================
//
// File:        rad1394server.h
//
// Subsystem:	Radical 1394 server exports
//
// Description:	This file contains all definitions and classes relevant to
//              the radical 1394 iop server
//
// Revisions:	20-June-2001 Creation
//
// Notes:       
//
//=============================================================================

#ifndef	RAD1394SERVER_H
#define RAD1394SERVER_H

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
// This is the id of the function provided by this RPC server.
//
#define rad1394FunctionId  0x07893434

// 
// These define the function numbers supported by server.
//
#define rad1394SetMemorySpace  0
#define rad1394GetMemorySpace  1
#define rad1394ReadWriteInfo   2
#define rad1394ReadAsync       3
#define rad1394WriteAsync      4

//
// Defines the size of the largest read write that can occur in a single transaction.
//
#define RPCMaxReadWriteSize   (32 * 1024)

//
// This structure is the format of our RPC read/write data requests. Used for communication
// with the host EE
//
struct RPCReadWriteInfo
{
    unsigned int    m_Address;                  // Where is shared memory to access
    unsigned int    m_Size;                     // Size of transfer
    unsigned int    m_Atomic;                   // Used to indicate interrups should be disabled
    unsigned int    m_LocalWrite;               // Indicates local write (boolean)
};

//
// Used for get and set size requests.
//
struct RPCGetSetMemorySize
{
    unsigned int    m_Size;
    unsigned int    m_Filler[ 3 ];
};

#endif


