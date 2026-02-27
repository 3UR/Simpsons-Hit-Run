//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radRemoteCommand.hpp
//
// Subsystem:	Radical Remote Command Server
//
// Description:	This file contains all definitions and interfaces
//				required to use the Radical Remote Command Server. 
//
// Revisions:	06-Feb-2001	    Creation
//
//=============================================================================

#ifndef RADREMOTECOMMAND_HPP
#define RADREMOTECOMMAND_HPP

//=============================================================================
// Build Configuration Check
//=============================================================================

#if !defined(RAD_DEBUG) && !defined(RAD_TUNE) && !defined(RAD_RELEASE) 
    #error 'FTech requires definition of RAD_DEBUG, RAD_TUNE, or RAD_RELEASE'
#endif

//=============================================================================
// Include Files
//=============================================================================

#include <radobject.hpp>
#include <radmemory.hpp>

//=============================================================================
// Forward Declarations
//=============================================================================

struct IRadRemoteCommand;

//=============================================================================
// Type Definition
//=============================================================================

//
// All functions available remotely must follow the conventions of the 
// RemoteFunction typedef.  The functions will be statically defined, and can,
// for example, use the userData argument to determine which instance of a class
// must react to the function call.
//
enum HrcsResultCode
{
	HrcsSuccess,
	HrcsFail
};

typedef HrcsResultCode (*RemoteFunction)( int argc, char* argv[], void* userData );

//=============================================================================
// Functions
//=============================================================================

//
// Use these functions to intialize and terminate the system. The debug
// communication system must have been previously initialized. Note that cleints
// should use the inline definitions of these functions as they compile to nothing
// in a release build. 	
// 
void radRemoteCommandInitialize( radMemoryAllocator alloc );
void radRemoteCommandTerminate( void );

//
// Use this member to obtain the iterface to the remote functions. The inlines defintions
// should normally be used. If you hold the pointer returned, remember to add ref it. 
//
IRadRemoteCommand* radRemoteCommandGet( void );

//=============================================================================
// Interface: bIRemoteCommmand
//=============================================================================

struct IRadRemoteCommand : public IRefCount
{
	//
	// The functionName will be used for identification purposes.
	// The UserData will be stored witht the function pointer and passed to the 
	// function when the function is called.
	//
	virtual void RegisterRemoteFunction( const char* functionName, RemoteFunction rfptr, void* userData ) = 0;
	
	//
	// This is self-explanatory.  Clients are responsible for UnRegistering
	//
	virtual void UnRegisterRemoteFunction( const char* functionName ) = 0;
};


//=============================================================================
// Inline Function Definitions
//=============================================================================

//
// These inline functions vanish in a release build. The Initialize and 
// Terminate functions should fram all other calls to the remote command system.  
//

//=============================================================================
// Inline Function:    RcsRemoteCommandInitialize
//=============================================================================

inline void RcsRemoteCommandInitialize( radMemoryAllocator alloc = RADMEMORY_ALLOC_DEFAULT )
{
#ifdef RAD_DEBUG
    radRemoteCommandInitialize( alloc );    
#else
    (void) alloc;
#endif
}

//=============================================================================
// Inline Function:    RcsRemoteCommandTerminate
//=============================================================================

inline void RcsRemoteCommandTerminate( void )
{
#ifdef RAD_DEBUG
    radRemoteCommandTerminate( );
#endif
}

//=============================================================================
// Inline Function:    RcsRegisterRemoteFunction
//
// Description: Inline function function for registering remote functions
//=============================================================================
inline void RcsRegisterRemoteFunction( char* functionName, RemoteFunction rfptr, void* userData )
{
#ifdef RAD_DEBUG
    radRemoteCommandGet( )->RegisterRemoteFunction( functionName, rfptr, userData );
#else
	(void) functionName;
	(void) rfptr;
	(void) userData;
#endif
}

//=============================================================================
// Inline Function:    RcsRegisterRemoteFunction
//
// Description: Inline function function for unregistering remote functions
//=============================================================================
inline void RcsUnRegisterRemoteFunction( char* functionName )
{
#ifdef RAD_DEBUG
	radRemoteCommandGet( )->UnRegisterRemoteFunction( functionName );
#else
	(void) functionName;
#endif 
}

#endif