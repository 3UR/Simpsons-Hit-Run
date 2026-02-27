//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        remotecommandserver.cpp
//
// Subsystem:   Radical Remote Command Server
//
// Description:	This file contains the implementation of the RCS.
//
// Revisions:	06-Feb-2001    Creation
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <string.h>
#include <radmemorymonitor.hpp>
#include "remotecommand.hpp"

//=============================================================================
// Local Defintions
//=============================================================================

//=============================================================================
// Static Data Defintions
//=============================================================================

static RemoteCommandServer* s_theRemoteCommandServer = NULL;


//=============================================================================
// Function:    radRemoteCommandInitialize
//=============================================================================
// Description: Constructor for the server object.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radRemoteCommandInitialize( radMemoryAllocator alloc )
{
    //
    // Assert that this subsystem has not allready been initialized.
    //
    rAssertMsg( s_theRemoteCommandServer == NULL, "Remote Command System already initialized");
    
    //
    // Construct the object.
    //
    s_theRemoteCommandServer = new( alloc ) RemoteCommandServer( alloc );
        
    s_theRemoteCommandServer->Initialize( );
    
}

//=============================================================================
// Function:    radRemoteCommandTerninate
//=============================================================================
// Description: Shutdown
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radRemoteCommandTerminate( void )
{
    rAssertMsg( s_theRemoteCommandServer != NULL, "Remote Command System not initialized");

    s_theRemoteCommandServer->Terminate( );

    radRelease( s_theRemoteCommandServer, NULL );

    rAssertMsg( s_theRemoteCommandServer == NULL, "Remote Command System is still in use by someone");
}

//=============================================================================
// Function:    radRemoteCommandGet
//=============================================================================
// Description: Obtain interface to remote command server object
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

IRadRemoteCommand* radRemoteCommandGet( void )
{
    rAssertMsg( s_theRemoteCommandServer != NULL, "Remote Command System not initialized");
    
    return( s_theRemoteCommandServer );
}

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    RemoteCommandServer::RemoteCommandServer
//=============================================================================
// Description: Constructor for the server object.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

RemoteCommandServer::RemoteCommandServer( radMemoryAllocator alloc )
	:
	radRefCount( 1 ),
	m_FunctionList( NULL ),
	m_RemoteCommandTarget( NULL ),
    m_Allocator( alloc )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "RemoteCommandServer" );
}

//=============================================================================
// Function:    RemoteCommandServer::~RemoteCommandServer
//=============================================================================
// Description: Destructor for the server object. 
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

RemoteCommandServer::~RemoteCommandServer( void )
{
    s_theRemoteCommandServer = NULL;
}    

//=============================================================================
// Function:    RemoteCommandServer::Initialize
//=============================================================================
// Description: Initializes everything 
//
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void RemoteCommandServer::Initialize(  )
{
	//
	// Initialize a FunctionList
	//
	m_FunctionList = new( m_Allocator ) FunctionList( m_Allocator );
	
    //
	// Initialize the RemoteCommandTarget
	//
	m_RemoteCommandTarget = new( m_Allocator ) RemoteCommandTarget( m_Allocator );
	m_RemoteCommandTarget->SetRemoteCommandServer( this );
}

//=============================================================================
// Function:    RemoteCommandServer::Terminate
//=============================================================================
// Description: Terminates everything that needs it
//
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void RemoteCommandServer::Terminate( void )
{
	m_RemoteCommandTarget->UnSetRemoteCommandServer();
	m_RemoteCommandTarget = NULL;	

	m_FunctionList->Kill();
	m_FunctionList = NULL;
}	




//=============================================================================
// Function:    RemoteCommandServer::RegisterRemotFunction
//=============================================================================
// Description: Allows the game to register static functions to be called remotely
//
// Parameters:  char*			functionName	- The name of the function to be 
//												  published to remote callers 
//				RemoteFunction	rfptr			- The pointer to the static function
//				void*			userData
//
// Returns:     n/a
//
// Notes:		The registered function must match type definition, RemoteFunction
//------------------------------------------------------------------------------
void RemoteCommandServer::RegisterRemoteFunction( const char* functionName, RemoteFunction rfptr, void* userData )
{
	//
	// Some preventative medicine
	//
	rAssert( rfptr != NULL );
	rAssert( functionName != NULL );
	rAssert( functionName[0] != '\0' );
	//
	// Add the new function to the list
	//
	m_FunctionList->Add( functionName, rfptr, userData );
}

//=============================================================================
// Function:    RemoteCommandServer::UnRegisterRemotFunction
//=============================================================================
// Description: Allows the game to unregister static functions to be called remotely
//
// Parameters:  char*	functionName	- The name of the function to unregister
//												  
//
// Returns:     n/a
//
// Notes:		
//------------------------------------------------------------------------------
void RemoteCommandServer::UnRegisterRemoteFunction( const char* functionName )
{
	rAssert( functionName != NULL );
	rAssert( functionName[0] != '\0' );
	//
	// Remove the function from the list
	//
	m_FunctionList->Remove( functionName );
}

//=============================================================================
// Function:    RemoteCommandServer::CallFunction
//=============================================================================
// Description: Used by the RemoteCommandTarget to call a remote function
//
// Parameters:  char*  fname	- Name of the function to call
//				int    argc     - Number of arguments to pass to the function
//				char*  argv     - A parsed string of arguments
//												  
// Returns:     n/a
//
// Notes:		
//------------------------------------------------------------------------------
void RemoteCommandServer::CallFunction( char* fname, int argc, char* argv )
{
	RemoteFunction rf = NULL;
	void* userData = NULL;;

	//
	// Try to find the requested remote function
	//
	m_FunctionList->Get( fname, &rf, &userData );

	//
	// If a function pointer wasn't returned, we reply to the request with a failure message
	//
	if( ( rf == NULL ) )
	{
		rDebugString( "\nREMOTE FUNCTION NOT FOUND\n" );
		m_RemoteCommandTarget->SendRemoteFunctionReply( HrcsFail );
		return;
	}

	//
	// Get tokens from the argument string and then put them into a nicer array
	//
    char ** argvNew = (char**) radMemoryAlloc( GetThisAllocator( ), argc * sizeof(char*) );

    for( int i = 0; i < argc; i ++ )
    {
        argvNew[ i ] = argv;
		//
		// Iterate through until we reach the end of a token or the end of the string
		//
        while ( *argv != '\0' && *argv != '/' )
        {
            argv++;
        }
		//
		// Terminate the string
		//
        *argv = 0;
		argv++;
    }

	//
	// If a function pointer was returned, we call it and hope it exists
	// If something goes wrong, this function should return a failure message.
	//
	if( rf( argc, argvNew, userData ) == HrcsSuccess )
	{
		m_RemoteCommandTarget->SendRemoteFunctionReply( HrcsSuccess );
	}
	else
	{
		rDebugString( "\nREMOTE FUNCTION RETURNED FAIL\n" );
		m_RemoteCommandTarget->SendRemoteFunctionReply( HrcsFail );
	}
	
	radMemoryFree( GetThisAllocator( ), argvNew );
}

//=============================================================================
// Function:    RemoteCommandServer::ListFunctions
//=============================================================================
// Description: Used by the RemoteCommandTarget to list the registered function
//
// Parameters:  n/a
//												  
// Returns:     n/a
//
// Notes:		
//------------------------------------------------------------------------------
void RemoteCommandServer::ListFunctions( void )
{
	//
	// We ask the RemoteCommandTarget to tell the host how many functions there are
	//
	m_RemoteCommandTarget->SendListReply( m_FunctionList->NumFunctions() );
	
	m_FunctionList->ResetNextName();
	char name[40];
	for( unsigned int i = 0; i < m_FunctionList->NumFunctions(); ++i )
	{
		//
		// We ask the RemoteCommandTarget to send the name of each function in the list
		//
		strcpy( name, m_FunctionList->GetNextName() );
		if( strcmp( name, "" ) != 0 )
		{
			m_RemoteCommandTarget->SendListItemCommand( name );
		}
	}
} 
