//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        target1394socket.hpp
//
// Subsystem:	Radical Debug Communication System
//
// Description:	This file contains the target side definition of sockets using
//              the PS2 1394 FireWire interface. It is not a true sockets 
//              implementation but meets the requirements of the Debug Communication
//              system. We make many assumptions about its use. If a more fully
//              functioning sockets implemetation is required, we will do so.
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	Jul 16, 2001
//
//=============================================================================

#ifndef	TARGET1394SOCKET_HPP
#define TARGET1394SOCKET_HPP

//=============================================================================
// Include Files
//=============================================================================

#include "socket.hpp"
#include "1394shared.hpp"
#include <rad1394.hpp>
#include <radmemory.hpp>

//=============================================================================
// Defintions
//=============================================================================

//=============================================================================
// Class Declaration
//=============================================================================

//
// Definition for the target side socket implementation using the PS2 1394 FireWire
// communcation system.
//
class CTarget1394Socket : public radSocket
{
    public:

    CTarget1394Socket( void );
    ~CTarget1394Socket( void );

    //
    // Implement the various socket functions. Note this is not a true sockets implementation.
    //
    virtual int socket( int af, int type, int protocol);
    virtual int closesocket( int socket );
    virtual int setsockopt( int socket, int level, int optname, const char* optval, int optlen);
    virtual int accept( int socket, struct sockaddr* addr, int* addrlen);
    virtual int bind( int socket, struct sockaddr* addr, int addrlen);
    virtual int listen( int socket, int backlog);
    virtual int connect( int sock, const struct sockaddr* addr, int addrlen );
    virtual int recv( int socket, char* buf, int len, int flags);
    virtual int send( int socket, const char* buf, int len, int flags);
    virtual int lasterror( int socket );

    private:

    //
    // Member to perform the host communication
    //
    void Do1394Communication( void );

    //
    // Members to read, write and resolve state.
    //
    bool ReadData( void );
    bool WriteData( void );
    void ResolveState( void );

    //
    // Helper to round up a number to a multiple of 16
    //
    unsigned int RoundUpTo16( unsigned int value );    

    //
    // Helper to create a crc.
    //
    unsigned int MakeCrc( void* pBuffer, unsigned int size );

    public:

    void* operator new( size_t size, radMemoryAllocator alloc )
    {
        return( radMemoryAlloc( alloc, size ) );
    }    

    void operator delete( void * pMemory )
    {
	    radMemoryFree( pMemory );
    }

    private:
    
    // 
    // Interface pointer to the underlyng 1394 shared memory system running on the IOP.
    //
    IRad1394Slave*      m_pIRad1394Slave;

    //
    // These data structure are used to manage socket information 
    // maintained in memory.
    //
    enum SocketState
    {
        Free,               // Socket is not in use
        Listen,             // Socket is allocated and listening
        Connecting,         // Socket is connecting ( not currently used on target )
        Connected,          // Socket is connected,
        PendingDisconnect,  // Waiting for data to be sent before disconnect.
        Disconnected,       // Local Disconnected
        RemoteDisconnect    // Remote Disconnect
    };

    //
    // This structure is cooked to keep alignment to 64 bytes.
    //
  	struct Socket
	{
		SocketState     m_State;                          // Maintains state of the connection.
        unsigned int    m_1394Channel;                    // Shared memory index
		unsigned int    m_BytesInTxBuffer;                // Bytes queued for transmision.
		unsigned int    m_BytesInRxBuffer;                // Bytes received
        unsigned int    m_Pad[ 12 ];                        
		unsigned char   m_TxBuffer[ FW1394_CHANNEL_SIZE ];  // Buffer for sending data
		unsigned char   m_RxBuffer[ FW1394_CHANNEL_SIZE ];  // Buffer for receiving data.
	};

    //
    // Make the space for this array 64 bytes bigger so we can ensure it is aligned.
    //
    unsigned char  m_SocketArraySpace[ (sizeof( Socket ) * FW1394_CHANNEL_COUNT) + 64 ];

    Socket* m_SocketArray;

    //
    // This is used to indentiy the socket we are waiting for a connection on.
    //
    unsigned int        m_ListenSocketIndex;

    //
    // This member indicates the time we last heard from the host.
    //
    unsigned int        m_TimeOfLastContact;

    //
    // Maintains a count of open sockets. Used to determine shutdown condition.
    //
    unsigned int        m_OpenSocketCount;

    //
    // Need an index to determine where we are in the current read/write process.
    //
    unsigned int        m_ReadWriteIndex;  

    //
    // Address of shared memory.
    //
    FW1394MemoryMap*    m_p1394Memory;
};

#endif 
