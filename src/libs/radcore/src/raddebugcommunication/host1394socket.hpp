//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        host1394socket.hpp
//
// Subsystem:	Radical Debug Communication System
//
// Description:	This file contains the host side definition of sockets using
//              the PS2 1394 FireWire interface. This is note a true implementation
//              of sockets but meets the requirements of the host communication
//              system.
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	Jul 16, 2001
//
// Notes:       
//
//=============================================================================

#ifndef	HOST1394SOCKET_HPP
#define HOST1394SOCKET_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <rad1394.hpp>
#include "socket.hpp"
#include "1394shared.hpp"

//=============================================================================
// Defintions
//=============================================================================


//=============================================================================
// Class Declaration
//=============================================================================

//
// Definition for the host side socket implementation using the rad1394 FireWire system. 
// 
class CHost1394Socket : public radSocket
{
    public:
    CHost1394Socket( void );
    ~CHost1394Socket( void );

    //
    // Implement the various socket functions.
    //
    virtual int socket( int af, int type, int protocol);
    virtual int closesocket( int socket );
    virtual int setsockopt( int socket, int level, int optname, const char* optval, int optlen);
    virtual int bind( int socket, struct sockaddr* addr, int addrlen);
    virtual int connect( int socket, const struct sockaddr* addr, int addrlen);
    virtual int recv( int socket, char* buf, int len, int flags);
    virtual int send( int socket, const char* buf, int len, int flags);
    virtual int lasterror( int socket );
    virtual int ioctlsocket( SOCKET s, long cmd, u_long *argp );
    virtual int AsyncSelect( SOCKET s, HWND Wnd, unsigned int wMsg, long levent );
    virtual int accept( int socket, struct sockaddr* addr, int* addrlen);
    virtual int listen( int socket, int backlog);

    private:

    //
    // These data structure are used to manage socket information 
    // maintained in shared memory.
    //
    enum SocketState
    {
        Free,               // Socket is not in use
        ToBeFreed,          // Will be freed.
        Idle,               // In use, not yet connecting ( only on host );
        Listen,             // Socket is allocated and listening (not used by host)
        Connecting,         // Socket is connecting ( not currently used on target )
        Connected,          // Socket is connected,
        PendingDisconnect,  // Waiting for data to be sent before disconnect.
        Disconnected,       // Local Disconnected
        RemoteDisconnect    // Remote Disconnect
    };

    //
    // This is the definition of a socket data struct. It is access by the host
    // i/o thread.
    //
  	struct Socket
	{
        //
        // The first part of this structure is alter by the background thread performing
        // i/o for all processes and the owner of the socket.
        //
        SocketState     m_State;                            // Maintains state of the connection.
        unsigned int    m_1394Channel;                      // Shared memory index
		unsigned int    m_BytesInTxBuffer;                  // Bytes queued for transmision.
		unsigned int    m_BytesInRxBuffer;                  // Bytes received
		unsigned char   m_TxBuffer[ FW1394_CHANNEL_SIZE ];  // Buffer for sending data
		unsigned char   m_RxBuffer[ FW1394_CHANNEL_SIZE ];  // Buffer for receiving data.
        unsigned int    m_ConnectTime;                      // Used to time-out pending connections

        HWND            m_hWnd;                             // To send socket messages to
        unsigned int    m_WndMsg;                           // Message
        Socket*         m_Socket;                           // Socket in process address space.

        HANDLE          m_ThreadHandle;                   // Thread handle
        DWORD           m_ThreadID;                       // Corresponding id       

        //
        // This member is a little weak. It is used by the thread which needs
        // to see its parent socket class. Essentially these objects are one to
        // one but may change in future.
        //
        CHost1394Socket* m_pHost1394Socket;

	};

    //
    // This is the data structure visible to all sockets, in all processes, along
    // with their background threads. It contains the array of socket information
    // along with flag indicating that a thread is currently performing the host
    // I/o operations. When a thread is started it checks this flag and will try
    // to gain it. 
    //
    struct SharedMemoryMap
    {
        Socket          m_SocketArray[ FW1394_CHANNEL_COUNT ];  // Sccket data
        bool            m_1394ThreadActive;                     // Is someone doing I/O
        bool            m_1394DataInitialized;                  // Has 1394 memory ever been initialzied
    };

    //
    // Static member is the thread enrty point.
    //
    static DWORD WINAPI SocketThread( Socket* pSocket );

    //
    // Functions to perform communication with target on behalf of all processes.
    // Returns true if it is doing the communication. It receives indication if
    // if was previously doing communicaiton.
    //
    bool DoTargetCommunication( bool isDoingCommunication );
    
    //
    // Cleans up if was doing communicaiton/
    //
    void DoTargetCommunicationCleanUp( void );

    //
    // Helper to round up a number to a multiple of 16
    //
    unsigned int RoundUpTo16( unsigned int value );    

    //
    // Helper to create CRCs.
    //
    unsigned int MakeCrc( void* pBuffer, unsigned int size );

    //
    // These data structures are used to access the shared memory and serialize
    // access to it when necessary.
    //
    HANDLE m_MutexHandle;
    
    //
    // Holds handle to shared memory object containing target information table.
    //
    HANDLE m_MemoryFileHandle;
    
    //
    // Holds the address of the shared memory image.
    //
    SharedMemoryMap*    m_pSharedData;

    //
    // Interface pointer to the rad1394 master.
    //
    IRad1394Master*     m_pIRad1394Master;      

    //
    // Stuff used to maintain communication state over the i/o interface.
    //
    unsigned int        m_TimeOfLastContact;

    FW1394MemoryMap*       m_p1394Memory;

    //
    // This specifies the time the host will wait before disconnect from the target.
    //
    unsigned int        m_SocketConnectionTimeoutValue;
    
};

#endif 
