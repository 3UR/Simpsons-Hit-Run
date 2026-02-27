//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        remotecommand.hpp
//
// Subsystem:	Radical Remote Command Server
//
// Description:	This file contains all definitions and classes relevant to
//              the Remote Command Server
//
// Revisions:	06-Feb-2001	Creation
//
// Notes:       
//
//=============================================================================

#ifndef	REMOTECOMMAND_HPP
#define REMOTECOMMAND_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <raddebugcommunication.hpp>
#include <radremotecommand.hpp>
#include <radobject.hpp>
#include <radmemory.hpp>
#include "../radprotocols/remotecommandprotocol.hpp"

//=============================================================================
// Forward Class Declarations
//=============================================================================

class Functionlist;
class RemoteCommandServer;
class RemoteCommandTarget;

//=============================================================================
// Class Declarations
//=============================================================================

//
// The FunctionList Class is only used by the RemoteCommandServer Class
// It's just a singly linked list of function pointers and data with a 
// few member functions
//
class FunctionList
	:
	public radRefCount
{
	public:
		IMPLEMENT_REFCOUNTED( "FunctionList" )
	
		FunctionList( radMemoryAllocator alloc );
		~FunctionList( void );
		void Add( const char* functionName, RemoteFunction rfptr, void* userData );
		bool Remove( const char* functionName );
		void Get( char* functionName, RemoteFunction* prfptr, void** userData );
    	const char* GetNextName( void );
		void ResetNextName( void );
		void Kill( void );
		unsigned int NumFunctions( void );

	private:

		struct FunctionNode
		{
			char			m_FunctionName[256];
			RemoteFunction	m_Function;
			void*			m_userData;
			FunctionNode*	m_Next;
		};

		FunctionNode* m_Head;
		unsigned int m_FunctionCount;
		FunctionNode* m_Current;
        radMemoryAllocator   m_Allocator;
};


//
// CLASS RemoteCommandServer
//
// This implements the bIRemoteCommandServer interfance
// It contains references to a FunctionList object and a RemoteCommandTarget
// object
//
class RemoteCommandServer 
	: 
	public IRadRemoteCommand,
	public radRefCount
{
    public:
		IMPLEMENT_REFCOUNTED( "RemoteCommandServer" )

		//
		// Constructor. Nothing too interesting.
		//
		RemoteCommandServer( radMemoryAllocator alloc );
		//
		// Destruction done through release.
		//
		~RemoteCommandServer( void );

		//
		// bIRemoteCommandServer Functions
		//
		virtual void RegisterRemoteFunction( const char* functionName, RemoteFunction rfptr, void* userData );
		virtual void UnRegisterRemoteFunction( const char* functionName );

		void Initialize( void );
		void Terminate( void );
		
		//
		// Used by the RemoteCommandTarget class to call
		// a registered remote function
		//
		void CallFunction( char* fname, int argc, char* argv );
		
		//
		// Used by the RemoteCommandTarget class to list
		// all registered functions
		//
		void ListFunctions( void );

    private:
    
		//
		// The linked list of Function Pointers and related data.
		// The list will consist of FunctionNodes
		//
		radRef< FunctionList > m_FunctionList;

		//
		// We need a RemoteCommandTarget for communication purposes
		//
		radRef< RemoteCommandTarget > m_RemoteCommandTarget;

        radMemoryAllocator         m_Allocator;
};


//
// This class represents to object used to communicate with the host.
// It handles all of the DebugComTarget stuff
//
class RemoteCommandTarget : public IRadDbgComChannelStatusCallback,
							public IRadDbgComChannelReceiveCallback,
							public IRadDbgComChannelSendCallback,
							public radRefCount  
{
    public:
  
		IMPLEMENT_REFCOUNTED( "RemoteCommandTarget" )

		//
		// Constructor. Nothing too interesting.
		//
		RemoteCommandTarget( radMemoryAllocator alloc );
		virtual ~RemoteCommandTarget( void );
		
		//
		// Status callback
		//
		virtual void OnStatusChange( IRadDbgComChannel::ConnectionState connectionState, const char* Message );
		virtual void OnReceiveComplete( bool successful, unsigned int bytesReceived );
		virtual void OnSendComplete( bool successful );

		//  
		// Thee next four members are all to be called by the RemoteCommandServer class
		//
		
		// Upon receive a list command from the host. We must reply with SendListReply
		// and state how many functions are registered.  Then, we must send each
		// function name individually. 
		void SendListReply( unsigned int numItems );
		void SendListItemCommand( char* name );

		//
		// Upon receiving a RemoteFunction call from the host, we must report back 
		// the result of that operation
		//
		void SendRemoteFunctionReply( HrcsResultCode result );

		//
		// This gives a reference to the parent server class
		//
		void SetRemoteCommandServer( RemoteCommandServer* rcs );
		void UnSetRemoteCommandServer( void );

    private:
    
		//
		// Private used to kick start transmission,
		//
		void InitiateTransmission( void );

		//
		// Channel used for communication.
		//
		radRef< IRadDbgComChannel >    m_Channel;

		//
		// Pointer to the Server
		//
		RemoteCommandServer* m_RemoteCommandServer;
		//
		// Boolean indicating that we are attached.
		//
		bool    m_Attached;
    
		//
		// Buffer used to receive messages from the target.
		//
		unsigned char m_RxBuffer[ RCS_BUFFER_SIZE ];

		//
		// Buffer used for transmitting data. Double buffered so we can
		// queue information to send.
		//
		unsigned char m_TxBuffer1[ RCS_BUFFER_SIZE ];
		unsigned char m_TxBuffer2[ RCS_BUFFER_SIZE ];

		//
		// Used to control transmit state machine.    
		//
		bool            m_TxOutstanding;
		unsigned int    m_TxBytesQueued;
		unsigned char*  m_CurrentTxAddress;
		bool            m_Buffer1Active;

		//
		// Prevents the display of too many error messages.
		//
		bool            m_OverflowReported;

		//
		// Indicates we want to detached when send completes.
		//
		bool            m_PendingDetach;

        radMemoryAllocator   m_Allocator;
       
};

		
#endif // REMOTECOMMAND_HPP


