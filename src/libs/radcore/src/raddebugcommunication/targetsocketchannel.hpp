//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        targetsocketchannel.Hpp
//
// Subsystem:	Radical Debug Communication
//
// Description:	This file contains all definitions and classes relevant to
//              the debug communication socket target Channel.
//
// Revisions:	28-Nov-2000 Creation
//
// Notes:       
//
//=============================================================================

#ifndef	TARGETSOCKETCHANNEL_HPP
#define TARGETSOCKETCHANNEL_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <raddebugcommunication.hpp>              
#include <radtime.hpp>
#include <raddispatch.hpp>
#include <radthread.hpp>
#include "targetx.hpp"
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
// This class implements the debug target channel interface. This implementation
// is for use with sockets under Windows and the PS2
//
class rDbgComSocketTargetChannel : public IRadDbgComChannel,
                                   public IRadDispatchCallback,
                                   public IRadThreadMutex,
                                   public radObject
{
    public:

    //
    // Contructor. Receives pointer to parent host object.
    //
    rDbgComSocketTargetChannel( rDbgComTarget* pTarget, unsigned short protocol );

    //
    // Destruction done through release.
    //
    ~rDbgComSocketTargetChannel( void );
 
    //
    // Mmeber functions as a result of the bIDebugChannel interface.
    //
    virtual void RegisterStatusCallback( IRadDbgComChannelStatusCallback* pCallback );
    virtual void Attach( void );
    virtual void Detach( void );
    virtual void GetStatus( ConnectionState* pConnectionState, char* pMessage );
    virtual void SendAsync( void* pBuffer, unsigned int numBytes, IRadDbgComChannelSendCallback* callback );
    virtual void ReceiveAsync( void* pBuffer, unsigned int numBytes, IRadDbgComChannelReceiveCallback* callback );

    //
    // Member implemented for the dispatch event handler
    //
    virtual void OnDispatchCallack( void* userData );

    //
    // Member functions implemented for bIObject
    //
    virtual void AddRef( void );
    virtual void Release( void );

    virtual void Lock( void );

    virtual void Unlock( void );

    #ifdef RAD_DEBUG
    virtual void Dump( char* pStringBuffer, unsigned int bufferSize );
    #endif

    //
    // Member used to set the socket to use for communication,
    //
    void SetSocket( SOCKET socket );
 
 
    private:
    
    //
    // Helper to set the state and invoke state change callback if it exists.
    //
    enum InternalState
    {
        Idle,                           // Not connected, 
        WaitingForSocket,               // Waiting for socket
        WaitingForConnectResponse,      // Waiting for our connect response
        Connected,                      // OK. Can send and receive
        WaitToSendDisconnectMessage     // Waiting for disconnect to be sent.    
    };

    void SetState( ConnectionState state, InternalState internalState, const char* msg );

    //
    // Internal members to deal with send and receive completes and errors
    //
    bool ProcessReceiveComplete( void );
    bool ProcessSendComplete( void );
    void ProcessError( );
    

    //
    // Helpers to build up packets.
    //
    unsigned int MakeInternalConnectPacket( void );
    unsigned int MakeDataPacket( unsigned int bytesToSend, unsigned char* pData );
    unsigned int MakeInternalDisconnectPacket( void );

    //
    // This member maintains the reference count of this object.
    //
    unsigned int m_ReferenceCount;    

    //
    // Maintain protocol
    //
    unsigned short m_Protocol;

    //
    // Holds a reference to the parent target
    //
    rDbgComTarget* m_ParentTarget;    

    //
    // Maintains our connection state.
    //
    ConnectionState m_ConnectionState;

    //
    // Holds callback to invoke anytime status change occurs.
    //
    IRadDbgComChannelStatusCallback*  m_StatusCallback;

    //  
    // Socket used for communicaiton
    //
    SOCKET  m_Socket;

    //
    // This state is our internal state as apposed to our connection state.
    // 
    InternalState m_InternalState;

    //
    // This buffer is used to textual information to the cleint about the
    // state of things.
    //
    char m_TextInfoBuffer[ 200 ];   

    //
    // These variables are used to receive data.
    //
    unsigned char   m_RxBuffer[ sizeof(DECI2_HDR) +  sizeof(rDbgComDataPacket)];
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
    unsigned char   m_TxBuffer[ sizeof(DECI2_HDR) +  sizeof(rDbgComDataPacket)];
    unsigned int    m_CurrentBytesToWrite;
    unsigned char*  m_CurrentWriteAddress;
    
    //
    // This holds info about the client send async request. Non-zero if send added.
    //
    IRadDbgComChannelSendCallback*    m_ClientSendCallback;
    unsigned int                    m_ClientSendSize;
    unsigned char*                  m_ClientSendBuffer;

    //
    // Boolean used to control the queuing of dispatch events.
    //
    bool                m_Active; 
};

#endif



