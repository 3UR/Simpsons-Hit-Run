//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        targetx.Hpp
//
// Subsystem:	Radical Debug Communication
//
// Description:	This file contains all definitions and classes relevant to
//              the debug communication Target implementation.
//
// Revisions:	16-Nov-2000 Creation
//
// Notes:       
//
//=============================================================================

#ifndef	TARGET_HPP
#define TARGET_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radtime.hpp>
#include <raddebug.hpp>
#include <radobject.hpp>
#include <radmemory.hpp>
#include <raddispatch.hpp>
#include <radthread.hpp>
#include <raddebugcommunication.hpp>

#if defined(RAD_WIN32) || defined(RAD_UWP)
#include <windows.h>
#endif 

#include "protocol.hpp"
#include "socket.hpp"

//=============================================================================
// Forward Class Declarations
//=============================================================================

class rDbgComTargetConnection;
class rDbgComSocketTargetChannel;

//=============================================================================
// Defintions
//=============================================================================

#ifndef RAD_WIN32
#define SOCKET int
#endif

//
// This structure is used to manage the state of protocols on the target. It
// is only used when we are using sockets on the target as opposed to 
// the DECI system.
//
struct ProtocolInfoEntry
{
    enum State
    {
        Free,                   // Entry, free
        ChannelPending,         // Protocol valid, target channel valid, computer name invalid
        ConnectionPending,      // Protocol valid, connection valid, computer name valid.
        Connected               // Protocol valid, name valid.
    };
    State                       m_State;
    unsigned short              m_Protocol;
    char                        m_HostComputerName[ MaxComputerName + 1 ];
    rDbgComTargetConnection*    m_pConnection;      
    rDbgComSocketTargetChannel* m_pChannel;
};

//=============================================================================
// Class Declarations
//=============================================================================

//
// This class implements the debug target interface. We only support a singleton
// instance of this object.
//
class rDbgComTarget : public IRadTimerCallback,
                      public radObject,
                      public IRadThreadMutex
{
    public:

    //
    // Constructor. Nothing too interesting.
    //
    rDbgComTarget( void );

    //
    // Destruction done through release.
    //
    ~rDbgComTarget( void );


    void Initialize( radDbgComType targetType, unsigned short port, void* InitInfo, radMemoryAllocator alloc );
    void Terminate( void );
    void CreateChannel( unsigned short protocol, IRadDbgComChannel** ppChannel, radMemoryAllocator alloc );
    void Service( void );

    //
    // Members implemented for the timer event handler
    //
    virtual void OnTimerDone( unsigned int elapsedtime, void* pUserData );

    //
    // lock/unlock the mutex
    //
    virtual void Lock( );

    virtual void Unlock( );

    #ifdef RAD_DEBUG
    virtual void Dump( char* pStringBuffer, unsigned int bufferSize );
    #endif

    //
    // Member functions implemented for IRefCount
    //
    virtual void AddRef( void );
    virtual void Release( void );

    //  
    // Members to assist in managing protocol information.
    //
    ProtocolInfoEntry* FindProtocol( unsigned short protocol );
    ProtocolInfoEntry* FindFreeProtocol( void );

    //
    // This member our local dispatcher.
    //
    IRadDispatcher*         m_Dispatcher;
    IRadTimerList*          m_TimerList;

    private:
    
    //
    // This member maintains the reference count of this object.
    //
    unsigned int m_ReferenceCount;    

    //
    // Maintains current target type and port.
    //
    radDbgComType   m_TargetType;
    unsigned short  m_Port;

    //
    // The remaining members are used when we are initialized using
    // sockets. We use these to emulate behavior similar to that done
    // by the PS2 Deci System.
  
    //
    // Holds the socket we use to listen for connections on.
    //
    SOCKET                  m_ListeningSocket;

    //
    // This timer is used for check for socket events.
    //
    IRadTimer*              m_Timer;

    //
    // Used for memory management;
    //
    radMemoryAllocator      m_Allocator;

    //
    // This data structure is used to manage allocated protocols on this target. If is
    // manipulated by both connection and channel objects.
    //
    ProtocolInfoEntry       m_ProtocolTable[ MaxProtocols ];

    //
    // This member holds our socket implementation abstraction.
    //
    public:
    struct radSocket*     m_SocketImp;
     
};

#endif


