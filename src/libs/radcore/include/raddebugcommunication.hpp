//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        raddebugcomminication.hpp
//
// Subsystem:	Radical Fountation Tech - Debug Communication Interfaces.
//
// Description:	This file contains the interface used for communication between
//              the target and host during debug developement. They are intended
//              for use by during development and not intended for general communication
//              purposes (i.e. in a networked game ). 
//
// Revisions:	Mar 4, 2000     Creation
//
// Notes:       These interfaces are not symetric. The application is either
//              a target (ie, the game ) or a host ( a tool running on the host
//              computer ). 
//
//=============================================================================

#ifndef	RADDEBUGCOMMUNICATION_HPP
#define RADDEBUGCOMMUNICATION_HPP

//=============================================================================
// Build Configuration Check
//=============================================================================

#if !defined(RAD_UWP) && !defined(RAD_WIN32)
    #error 'FTech requires definition of RAD_UWP or RAD_WIN32'
#endif

//=============================================================================
// Include Files
//=============================================================================

//#include <stddef.h>
#include <radobject.hpp>
#include <radmemory.hpp>

//=============================================================================
// Defintions
//=============================================================================

//
// This definition governs the default IP port used for communication between
// the target and the host applications.
//
const unsigned int radDbgComDefaultPort = 8510;

//
// These constants govern the allowable values for protocols. They are inclusive.
//
const unsigned short radDbgComMinProtocol = 0xE000;
const unsigned short radDbgComMaxProtocol = 0xEFFF;

//
// This enumeration is used for control the type of communication used by the 
// target.
//
enum radDbgComType
{
  #if defined (RAD_WIN32) || defined (RAD_UWP)
    WinSocket,                          // Sockets
    FileSockets                         // Socket emulation using files (obsolete)
  #endif
};

//
// This structure is used to convery USB Tcpip configuration information.
//
struct radDbgComUsbTcpIpInitInfo
{
    char    m_IPAddress[ 15 + 1 ];  // Null terminated string of form aaa.bbb.ccc.ddd
    char    m_SubMask[ 15 + 1 ];    // Submask using the same formate
    char    m_Gateway[ 15 + 1 ];    // Gateway
};

//
// These used used to define strings for the target name and IP address.
//
typedef char radDbgComTargetName[ 32 + 1 ];
typedef char radDbgComIPAddress[ 64 + 1 ];

//=============================================================================
// Forward Reference Declarations
//=============================================================================

struct IRadDbgComTargetTable;
struct IRadDbgComChannel;
struct IRadDbgComChannelStatusCallback;
struct IRadDbgComChannelSendCallback;
struct IRadDbgComChannelReceiveCallback;

//=============================================================================
// Functions Definitions
//=============================================================================

//
// Use this function to get an interface to the debug host object. This objects
// is a signelton and manages target definitions. Can be accessed accross process
// boundaries. Currently this interface is only avaibable under windows. Use
// the second function to obtain a communincaiton channel for communication with
// the 
//
#if defined (RAD_WIN32) || defined (RAD_UWP)

void radDbgComHostGetTargetTable( IRadDbgComTargetTable** pIRadDbgComTargetTable );
void radDbgComHostCreateChannel( const radDbgComTargetName pName, unsigned short protocol, 
                                 IRadDbgComChannel** ppChannel );

//
// Explicitly service the dbgcom host channel, neccessary if only if doing sync data
// transfer.
//
void radDbgComHostService( void );

#endif

//
// These functions are provided for the target side of the communication. The 
// game must initialize the system proior to creating communicaiton channels. If
// the game does not initialize it, all channel creations will fail. The game,
// at initialization time must specify what type of communication to use 
// with the host. Terminate must be invoked at the end of the game.
//
void radDbgComTargetInitialize( radDbgComType targetType,
                                unsigned short port = radDbgComDefaultPort,
                                void* pInitInfo = NULL,
                                radMemoryAllocator alloc = RADMEMORY_ALLOC_DEFAULT );

void radDbgComTargetTerminate( void );

//
// This service must be periodically invoked by the game for the debug communication
// system to work. Add a call to this function in your game loop.
//
void radDbgComService( void );

//
// Use this interface to obtain a target comminication channel. The channel is initially 
// not connected. If the specified protocol is already in use on the specified target,
// then NULL is returned or if the system has not been initialized.
//
void radDbgComTargetCreateChannel( unsigned short protocol, 
                                   IRadDbgComChannel** ppChannel,
                                   radMemoryAllocator alloc = RADMEMORY_ALLOC_DEFAULT );

//
// To make debugcommunication system thread safe, user must call this function before they
// want to handle anything from debugcommunication. All subsystem uses debugcommunication
// should use those two functions to make sure thread sync problem.
//
// In short word, these two functions make multi-threaded debugcommunication system behave
// the same way as single-threaded system.
//
// - Nov 15th 2002 James Tan -
// This is a short term solution empolyed to fix multi-thread sync problem in debugcommunication
// system. And a long term solution should be found and put in-place when more time is avariable.
//
void radDbgComMutexLock( );

void radDbgComMutexUnlock( );

//=============================================================================
// Interface Definitions
//=============================================================================

//
// The following interface is used to interact with the debug communication host
// target definition table.. This interface interacts with a singleton object. The singleton is
// valid accross process boundaries.
//
struct IRadDbgComTargetTable : public IRefCount
{
    //
    // Use this member to add a target definition. Target name is case insensitive.
    // The name must not exist and returns true if sucessfull.
    //
    virtual bool AddTargetDefinition( const radDbgComTargetName pName, 
                                      const radDbgComIPAddress pIpAddress,
                                      unsigned short port = radDbgComDefaultPort ) = 0;
        
    //
    // This member allows a target definition to be deleted. It is invalid to delete
    // a target that is currently in use.
    //
    virtual bool DeleteTargetDefinition( const radDbgComTargetName pName ) = 0;
 
    //
    // This member is used to enumerate targets. Callback will be invoked for each target
    // defined. If no targets defined, then the callback will not be invoked. The callback
    // is invoked as an extension of calling thread. Caller must copy data as pointers cannot
    // be held by caller. Caller also receives a boolean indicating weather or not any applications
    // on the host computer are using the indicated target.
    //
    virtual void EnumerateTargets( void* context, void (*pEnumerationCallback)( void* context, const radDbgComTargetName pName,
                                   const radDbgComIPAddress pIpAddress, unsigned short port, bool InUse ) ) = 0;
    
    //
    // These two members are used to set and get the default target. They do nothing more than
    // flag the specified target as the default. Will return false if target not found or no 
    // default target exists respectively.
    //
    virtual bool SetDefaultTarget( const radDbgComTargetName pName ) = 0;
    virtual bool GetDefaultTarget( radDbgComTargetName pName ) = 0;
};     

//
// Interface used to communicate bwteen host and target. 
//
struct IRadDbgComChannel : public IRefCount
{
    //
    // The following enumeration reflects the state a channel can be in.
    //
    enum ConnectionState
    {
        Detached,                   // Not currently attached or attemping to attach.
        Attaching,                  // Attaching, ( waiting to attached )
        Attached,                   // Attached, can send and receive,
        Detaching                   // Caller detached, waiting for completion. 
    };
            
    //
    // These services are used to be notified when the connection state of a channnel
    // changes. Set it to NULL to unregister.
    //
    virtual void RegisterStatusCallback( IRadDbgComChannelStatusCallback* pCallback ) = 0;
        
    //
    // Issue this member to attach to the target. This service will continue indefinitely
    // to try and reach the host/target. It will only stop trying if is in use by another
    // host, or the caller issues an explicit detach.
    //
    virtual void Attach( void ) = 0;
    
    //
    // Issue this call to detach the channel. Due to internal considerations, make
    // sure this member is invoked, prior to releasing refernece to channel.
    //
    virtual void Detach( void ) = 0;

    //
    // This member can be used to poll for changes in the connnection status. Caller
    // can provied a buffer which will receive a text message indicating what is happening.
    //
    virtual void GetStatus( ConnectionState* pConnectionState, char* pMessage ) = 0;
      
    //
    // Use this member to send. This service is asynchronous and callback is invokeed
    // when send completes or an error occurs. Caller is responsible for maintaining
    // buffer until call completes. Current implentation only allows a single send
    // outstanding on a given channel. Also system is packet based, meaning a receive must
    // exist capaable of receiving this send.
    //
    virtual void SendAsync( void* pBuffer, unsigned int numBytes, IRadDbgComChannelSendCallback* callback ) = 0;

    //
    // Use this member to issue a receive asynchronously. Callback will be invoked when
    // receive completes. If connection is not connected, or is disconnected, then the callback
    // will be invoked with an error status.
    //
    virtual void ReceiveAsync( void* pBuffer, unsigned int numBytes, IRadDbgComChannelReceiveCallback* callback ) = 0;

};

//
// Implement this interface to be notified of status changes to the communication channel. A
// string is provided to provide user with info,
//
struct IRadDbgComChannelStatusCallback
{
    virtual void OnStatusChange( IRadDbgComChannel::ConnectionState connectionState,
                                  const char* Message ) = 0;
};

//
// Implement this interface to receive notification of a send complete. This call will
// return a boolean indicating if the send was successful or not. Note that the 
// caller will be informed of a status change if a send fails.
// 
struct IRadDbgComChannelSendCallback : public IRefCount 
{
    virtual void OnSendComplete( bool Successful ) = 0;
};

//
// Implement this interface to receive notifcation of a receive complete. Returns a boolean
// indicating if the receive was successful or not. Will only fail it status is not attached.
//
struct IRadDbgComChannelReceiveCallback : public IRefCount 
{
    virtual void OnReceiveComplete( bool Successful, unsigned int bytesReceived ) = 0;
};

#endif

