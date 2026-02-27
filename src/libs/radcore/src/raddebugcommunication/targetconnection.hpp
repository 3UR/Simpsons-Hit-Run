//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        targetconnection.Hpp
//
// Subsystem:	Radical Debug Communication
//
// Description:	This file contains all definitions and classes relevant to
//              the debug communication connection class. This class is used
//              during the initial formation of a connection. It is used only
//              on the target and simulates much of the PS2EE Deci behavior.
//              
// Revisions:	20-Nov-2000 Creation
//
// Notes:       
//
//=============================================================================

#ifndef	TARGETCONNECTION_HPP
#define TARGETCONNECTION_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radtime.hpp>
#include <raddispatch.hpp>

#include "decitypes.h"

#include "deci2x.h"

#include "protocol.hpp"

#include "targetx.hpp"



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
// This class is used during the formation of a connection. It is used by the
// target when we are using sockets.
//
class rDbgComTargetConnection : public IRadDispatchCallback,
                                public radObject,
                                public IRadThreadMutex
{
    public:

    //
    // Constructor. Nothing too interesting. Just receives the socket and parent
    // target object.
    //
    rDbgComTargetConnection( SOCKET socket, rDbgComTarget* pTarget );

    //
    // Destruction done through release.
    //
    ~rDbgComTargetConnection( void );

    //
    // Member function implemented for dispatch event.
    //
    virtual void OnDispatchCallack( void* userData );

    virtual void AddRef( void );
    virtual void Release( void );

    virtual void Lock( void );
    virtual void Unlock( void );

    #ifdef RAD_DEBUG
    virtual void Dump( char* pStringBuffer, unsigned int bufferSize );
    #endif

    //
    // These members are used internally. The first is used when we shutdown
    // the target system to close any pending connections. The other is used
    // by a pending channel to get a socket.
    //
    void Deactivate( void );
    
    SOCKET GetSocket( void );
    
    private:
    

    //
    // Internal members to deal with send and receive completes.
    //
    bool ProcessReceiveComplete( void );
    bool ProcessSendComplete( void );

    //
    // Helper functions to make packets.
    //
    unsigned int MakeNETMPConnectReply( bool success );
    unsigned int MakeNETMPStatusReply( char* hostOwnerName );
    unsigned int MakeNETMPMessageReply( void );
 
    //
    // This member maintains the reference count of this object.
    //
    unsigned int m_ReferenceCount;    

    //
    // Socket.
    //
    SOCKET  m_Socket;

    //  
    // Parent target object.
    //
    rDbgComTarget* m_pParentTarget; 

    //
    // This enumeration reflects the state of this object. Not this object simulates
    // must of the bahavior implemeted by the Sony DECI protocol.
    // 
    enum ConnectionState
    {
        WaitForNETMPConnect,                // Waiting for connect.
        WaitForNETMPStatus,                 // Waiting for status message
        WaitForNETMPMessage,                // Waiting for message message
        WaitForNETMPStatusReplySend,        // Waiting to finish send of status reply   
        WaitForInternalReconnect,           // Waiting for internal reconnect
        Connected                           // Have a DECI connection established   
    };

    ConnectionState m_ConnectionState;

    //
    // These member variables are used to receive data.
    //
    unsigned char   m_RxBuffer[ 256 ];
    unsigned int    m_CurrentBytesToRead;
    unsigned char*  m_CurrentReadAddress;

    //
    // Used to monitor for time-outs.
    //
    bool            m_TimerActive;
    unsigned int    m_StartTime;

    //
    // These member variables are used to write data.
    //
    unsigned char   m_TxBuffer[ 256 ];
    unsigned int    m_CurrentBytesToWrite;
    unsigned char*  m_CurrentWriteAddress;

    //
    // This variable contains the name of the computer which already owned the
    // protocol or the name which currently owns it.
    //
    char            m_HostOwnerName[ MaxComputerName + 1 ];

    //  
    // Contains the protocol for this connection.
    //
    unsigned short  m_Protocol;    

    //
    // Boolean used to object alive.
    //
    bool    m_Active;


};

#endif


