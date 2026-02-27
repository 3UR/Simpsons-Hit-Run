//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        hostchannel.Hpp
//
// Subsystem:	Radical Debug Communication
//
// Description:	This file contains all definitions and classes relevant to
//              the debug communicatio Host channel implementation.
//
// Revisions:	08-Nov-2000 Creation
//
// Notes:       This component is only available under Windows 
//
//=============================================================================

#ifndef	HOSTCHANNEL_HPP
#define HOSTCHANNEL_HPP
#ifdef RAD_WIN32

//=============================================================================
// Include Files
//=============================================================================

#include <windows.h>
#include <raddebugcommunication.hpp>
#include "host.hpp"
#include "decitypes.h"
#include "deci2x.h"
#include "protocol.hpp"

//=============================================================================
// Forward Class Declarations
//=============================================================================


//=============================================================================
// Defintions
//=============================================================================

//=============================================================================
// Class Declarations
//=============================================================================

//
// This class implements the debug host channel interface.
//
class rDbgComHostChannel : public IRadDbgComChannel
{
    public:

    //
    // Contructor. Receives pointer to parent host object.
    //
    rDbgComHostChannel( rDbgComHost* pHost, unsigned int targetIndex, unsigned short protocol );

    //
    // Mmeber functions as a result of the bIDebugHostChannel interface.
    //
    virtual void RegisterStatusCallback( IRadDbgComChannelStatusCallback* pCallback );
    virtual void Attach( void );
    virtual void Detach( void );
    virtual void GetStatus( ConnectionState* pConnectionState, char* pMessage );
    virtual void SendAsync( void* pBuffer, unsigned int numBytes, IRadDbgComChannelSendCallback* callback );
    virtual void ReceiveAsync( void* pBuffer, unsigned int numBytes, IRadDbgComChannelReceiveCallback* callback );

    //
    // Member functions implemented for IRefCound
    //
    virtual void AddRef( void );
    virtual void Release( void );

    //
    // Member used to field windows messages sent to this channel
    //
    LRESULT OnWindowMessage( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

    private:
    
    //
    // Destruction done through release.
    //
    ~rDbgComHostChannel( void );
 
    //
    // Helper to set the state and invoke state change callback if it exists.
    //
    enum InternalState
    {
        Idle,                           // Not connected, 
        DNSLoopUp,                      // Looking up host name
        Connecting,                     // Waiting for socket connect to complete
        WaitForNETMPConnectResponse,    // Waiting for NETMP connect response
        WaitForNETMPStatusResponse,     // Waiting for NETMP staus response
        WaitForConnectMessage,          // Waiting for our internal connect message
        Ready                           // Life is good. We can send and receive.
    };

    void SetState( ConnectionState state, InternalState internalState, const char* msg );

    //
    // Internal handlers for the various windows messages.
    //
    void OnDNSNameLoopUp( HANDLE dnsLookUpHandle, unsigned int result );
    void OnReadyToRead( unsigned int result );
    void OnReadyToWrite( unsigned int result );
    void OnConnectComplete( unsigned int result );
    void OnClose( unsigned int result );
    void OnTimerExpired( void );
    void OnSendComplete( void );

    //
    // Internal helper functions to build packets.
    //
    unsigned int MakeNETMPConnectPacket( unsigned char* buffer, unsigned int buffersize, unsigned int protocol );       
    unsigned int MakeNETMPMessagePacket( unsigned char* buffer, unsigned int buffersize );       
    unsigned int MakeNETMPStatusPacket( unsigned char* buffer, unsigned int buffersize );       
    unsigned int MakeNETMPKillPacket( unsigned char* buffer, unsigned int buffersize, unsigned int protocol );       
    unsigned int MakeDataPacket( unsigned char* buffer, unsigned int buffersize, unsigned int m_Protocol,
                                 unsigned int bytesToSend, unsigned char* pData );
    unsigned int MakeInternalConnectReplyPacket( unsigned char* buffer, unsigned int buffersize, unsigned int protocol );
    unsigned int MakeInternalReconnectPacket( unsigned char* buffer, unsigned int buffersize, unsigned int protocol );

    //
    // Helper function to send data synchronously. Needed because there was an oversight in the
    // code and this was a quick fix. Not a problem as only used for small packets and only runs
    // on the host side.
    //
    int SendHelper( int socket, const char* buf, int len, int flags);
 
    //
    // This member maintains the reference count of this object.
    //
    unsigned int m_ReferenceCount;    

    //
    // Maintain protocol and target index ( used to obtain ip address and port )
    //
    unsigned int m_TargetIndex;
    unsigned short m_Protocol;

    //
    // Holds a reference to the parent host.
    //
    rDbgComHost* m_ParentHost;    

    //
    // Maintains our connection state.
    //
    ConnectionState m_ConnectionState;

    //
    // Window handle used for to receive events for windows sockets.
    //
    HWND            m_hWnd;

    //
    // Holds callback to invoke anytime status change occurs.
    //
    IRadDbgComChannelStatusCallback*  m_StatusCallback;

    //  
    // Socket used for communicaiton
    //
    SOCKET  m_Socket;

    //
    // This holds the handle associated with a DNS name lookup and the buffer to
    // receive the data.
    //
    bool    m_UsingDNSLookup;
    HANDLE  m_DNSLookUpHandle;
    char    m_NameLookBuffer[ MAXGETHOSTSTRUCT ];
    
    //
    // This state is our internal state as apposed to our connection state.
    // 
    InternalState m_InternalState;

    // 
    // Contains non-zero value if timer is running.
    //
    unsigned int m_TimerId;

        
    //
    // This buffer is used to textual information to the cleint about the
    // state of things.
    //
    char m_TextInfoBuffer[ 200 ];   

    //  
    // This is used when we are try to connect. It holds the address and port
    // of the target.
    //
	SOCKADDR_IN m_SockAddr;

    //
    // These variables are used to receive data.
    //
    unsigned char   m_RxBuffer[ sizeof(DECI2_HDR) +  sizeof(rDbgComDataPacket) ];
    unsigned int    m_CurrentBytesToRead;
    unsigned char*  m_CurrentReadAddress;
    bool            m_ReadingHeader;

    //
    // This holds info about the client read async request. Non-zero if receive added.
    //
    IRadDbgComChannelReceiveCallback* m_ClientReceiveCallback;
    unsigned int                    m_ClientReceiveSize;
    unsigned char*                  m_ClientReceiveBuffer;
    
    //
    // This is the buffer we use for sending. It must be large enough for our
    // largest packet which is a data packet.
    //
    unsigned char m_TxBuffer[ sizeof(DECI2_HDR) +  sizeof(rDbgComDataPacket) ];
    
    //
    // This variable indicates that the DCMP system has blocked our send.
    //
    bool            m_TxBlocked;
    int             m_LastSendSize;

    //
    // This member contains the number of bytes to send if we could not send due to
    // sockets blocking. This is not the same kind of block that we receive from
    // the DECI system described by the proceeding variables. If this is non-zero
    // and we were blocked.
    //
    unsigned int    m_PendingBytesToSend;
    unsigned char*  m_CurrentSendAddress;


    //
    // This holds info about the client send async request. Non-zero if send added.
    //
    IRadDbgComChannelSendCallback*    m_ClientSendCallback;
    unsigned int                    m_ClientSendSize;
    unsigned char*                  m_ClientSendBuffer;
 
    //
    // This member holds our socket implementation abstraction.
    //
    struct radSocket*     m_SocketImp;

};

#endif
#endif


