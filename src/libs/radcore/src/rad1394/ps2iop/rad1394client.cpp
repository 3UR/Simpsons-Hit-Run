//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        rad1394client.cpp
//
// Subsystem:	Radical Fire Wire EE Client
//
// Description:	This file contains the implementation PS2 EE client for accessing
//              the rad 1394 Fire Wire server running on IOP.
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	June 20, 2001
//
// Notes:       This file is intented to be built only for the PS2 EE
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include <stdlib.h>
#include <eekernel.h>
#include <sifrpc.h>
#include "rad1394client.hpp"
#include "rad1394server.h"
#include <radplatform.hpp>
#include <radmemorymonitor.hpp>

//=============================================================================
// Local Definitions
//=============================================================================


//=============================================================================
// Statics
//=============================================================================

static rad1394Client*  s_the1394Client = NULL;

//
// Maintains stuff for the RCP link with the IOP server.
//
static sceSifClientData    s_RpcClientData __attribute__((aligned (16)));

//
// Buffer used for get/set memory space requests.
//
static RPCGetSetMemorySize s_RpcSizeRequest __attribute__((aligned (16)));

//
// Buffer used for read and write of data. 
//
static RPCReadWriteInfo    s_RpcReadWriteInfo __attribute__((aligned (16)));

//=============================================================================
// Public Functions
//=============================================================================

//=============================================================================
// Function:    rad1394Initialize
//=============================================================================
// Description: This routine must be invoke prior to accessing any other 1394
//              functions. This routine ensures that IOP IRX has been loaded
//              and binds to the appropiate functions.
//
// Parameters:  memory allocator
//              
// Returns:     n/a
//
// Notes:       
//------------------------------------------------------------------------------

void rad1394Initialize
( 
    radMemoryAllocator alloc
)
{
    //
    // Assert that this subsystem has not allready been initialized.
    //
    rAssertMsg( s_the1394Client == NULL, "rad1394 client already initialized\n");
    
    //
    // Construct the object.
    //
    s_the1394Client = new( alloc ) rad1394Client( alloc );
}

//=============================================================================
// Function:    rad1394Terminate
//=============================================================================
// Description: This routine will shut down things.
//
// Parameters:  n/a
//              
// Returns:     n/a
//
// Notes:       
//------------------------------------------------------------------------------

void rad1394Terminate( void )
{
    //
    // Make sure has been initailzed. Release reference.
    //
    rAssertMsg( s_the1394Client != NULL, "rad1394 client not initialized\n");
    
    radRelease( s_the1394Client, NULL );

    rAssertMsg( s_the1394Client == NULL, "rad1394 client still in use\n");

}

//=============================================================================
// Function:    rad1394Get
//=============================================================================
// Description: This routine obtains an interface pointer to the singleton 
//              client object.
//
// Parameters:  
//              
// Returns:     IRad1394
//
// Notes:       Client is reponsible for adding a reference if he will hold the
//              pointer.
//------------------------------------------------------------------------------

IRad1394Slave* rad1394Get( void )
{
    rAssertMsg( s_the1394Client != NULL, "rad1394 client not initialized\n");
    return( s_the1394Client );
};

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    rad1394Client::rad1394Client
//=============================================================================
// Description: Constructor for the client object.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

rad1394Client::rad1394Client( radMemoryAllocator alloc )
    :
    m_ReferenceCount( 1 ),
    m_Allocator( alloc ),
    m_OutstandingRequest( 0 )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "rad1394Client" );
    //
    // Lets first see if the IRX is already running and loaded on the IOP. 
    // This may be the case, if we have used the bootloader.
    //
    sceSifInitRpc( 0 ); 
    
    //
    // Spin here a few times, trying to see if the server is there.
    //
    unsigned int attempts = 3;
    while( attempts != 0 )
    {
        attempts--;
            
        sceSifBindRpc( &s_RpcClientData, rad1394FunctionId, 0 );
    
        //
        // If it is not there, the documentation recommends, that you delay
        // a bit to allow the IOP to run.
        //
        if( s_RpcClientData.serve != 0 )
        {
            //
            // Here the server is present. We can simply return as we 
            // are connected.
            //
            return;
        }

        unsigned int spinCount = 5000;
        while( spinCount != 0 )
        {
            spinCount--;
        }
    }

    //
    // If we get here, we assume that the server has not been loaded. Lets do so now.
    // Need our module and the Sony ILink Driver.
    //
    radPlatformGet( )->LoadIrxModule( "ILink.irx" );
    radPlatformGet( )->LoadIrxModule( "rad1394.irx" );

    //
    // Lets try to bind. Do so until we succeed.
    //
    while( true )
    {
        sceSifBindRpc( &s_RpcClientData, rad1394FunctionId, 0 );
    
        //
        // If it is not there, the documentation recommends, that you delay
        // a bit to allow the IOP to run.
        //
        if( s_RpcClientData.serve != 0 )
        {
            //
            // Here the server is present. We can simply return as we 
            // are connected.
            //
            return;
        }

        unsigned int spinCount = 5000;
        while( spinCount != 0 )
        {
            spinCount--;
        }
    }
}

//=============================================================================
// Function:    rad1394Client::~rad1394Client
//=============================================================================
// Description: Destructor for the client object.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

rad1394Client::~rad1394Client( void )
{
    //
    // Set the memory space to zero, effectively talking the device off line.
    //
    SetMemorySpace( 0 );

    s_the1394Client = NULL;
}

//=============================================================================
// Function:    rad1394Client::SetMemorySpace
//=============================================================================
// Description: This member is invoked to establish the size of the memory space
//              used for communication over the 1394 firewire. Setting the value
//              to zero, takes the server "off-line"
//
//  Parameters: size = size of the memory in bytes
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rad1394Client::SetMemorySpace( unsigned int size )
{
    //
    // Can't have an oustanding request.
    //
    rAssert( !m_OutstandingRequest );

    //
    // Set up request and issue the RPC call. This one is done synchronously.
    //
    s_RpcSizeRequest.m_Size = size;

    sceSifCallRpc( &s_RpcClientData, rad1394SetMemorySpace, 0, &s_RpcSizeRequest, sizeof( s_RpcSizeRequest ),
                   NULL, 0, NULL, NULL );

}

//=============================================================================
// Function:    rad1394Client::GetMemorySpace
//=============================================================================
// Description: This member is invoked to obtain the current memory space
//              size.
//
// Parameters:  n/a
//
// Returns:     size
//
// Notes:
//------------------------------------------------------------------------------

unsigned int rad1394Client::GetMemorySpace( void )
{
    //
    // Can't have an oustanding request.
    //
    rAssert( !m_OutstandingRequest );

    //
    // Issue the RPC call. This one is done synchronously.
    //
    sceSifCallRpc( &s_RpcClientData, rad1394GetMemorySpace, 0, &s_RpcSizeRequest, sizeof( s_RpcSizeRequest ),
                   &s_RpcSizeRequest, sizeof( s_RpcSizeRequest ), NULL, NULL );

    return( s_RpcSizeRequest.m_Size );

}

//=============================================================================
// Function:    rad1394Client::ReadMemoryAsync
//=============================================================================
// Description: This member is invoked to read from the 1394 memory space
//              asychronously.
//
// Parameters:  addresss - 0 based offest of were to read. Must be aligned to 4
//              size     - number of bytes to read. Must be multiple of 16
//              dest     - where to copy, must be aligned to 16
//              atomic   - true to disable interrupts during read
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rad1394Client::ReadMemoryAsync
( 
    unsigned int address,
    unsigned int size,
    void*        dest,
    bool         atomic
)
{
    //
    // Can't have an oustanding request.
    //
    rAssert( !m_OutstandingRequest );

    //
    // Validate paramaters.
    //
    rAssert( (address & 0x03) == 0 );
    rAssert( (size & 0x0f) == 0 );
    rAssert( ((unsigned int) dest & 0x0f) == 0 );
    rAssert( size <= RPCMaxReadWriteSize );

    //
    // First send the address of where to read synchnorously accross.
    //
    s_RpcReadWriteInfo.m_Address = address;
    s_RpcReadWriteInfo.m_Size = size;
    s_RpcReadWriteInfo.m_Atomic = atomic;

    //
    // Issue the RPC call. This one is done synchronously.
    //
    sceSifCallRpc( &s_RpcClientData, rad1394ReadWriteInfo, 0, &s_RpcReadWriteInfo, sizeof( s_RpcReadWriteInfo ),
                   NULL, 0, NULL, NULL );

    //
    // Now lets do the actual transfer. Do it asychronously. Note:: Doing a read asynch did not
    // work well. Even with the flush cache, it ran very slow.
    //
//    m_OutstandingRequest = true;

//    SyncDCache( dest, (unsigned char*) dest + size  );

//    sceSifCallRpc( &s_RpcClientData, rad1394ReadAsync,SIF_RPCM_NOWAIT | SIF_RPCM_NOWBDC, NULL, 0,
//                 dest, size, NULL, NULL );

    sceSifCallRpc( &s_RpcClientData, rad1394ReadAsync,0, NULL, 0,
                   dest, size, NULL, NULL );

}

//=============================================================================
// Function:    rad1394Client::WriteMemoryAsync
//=============================================================================
// Description: This member is invoked to wrtie to the 1394 memory space
//              asychronously.
//
// Parameters:  addresss - 0 based offest of were to write. Must be aligned to 4
//              size     - number of bytes to write. Must be multiple of 16
//              source   - where to from copy, must be aligned to 16
//              atomic   - true to disable interrupts during write
//              localWrite -  true if write local, false write remote
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rad1394Client::WriteMemoryAsync
( 
    unsigned int address,
    unsigned int size,
    void*        source,
    bool         atomic,
    bool         localWrite
)
{
    //
    // Can't have an oustanding request.
    //
    rAssert( !m_OutstandingRequest );

    //
    // Validate paramaters.
    //
    rAssert( (address & 0x03) == 0 );
    rAssert( (size & 0x0f) == 0 );
    rAssert( ((unsigned int) source & 0x0f) == 0 );
    rAssert( size <= RPCMaxReadWriteSize );

    //
    // First send the address of where to write synchnorously accross.
    //
    s_RpcReadWriteInfo.m_Address = address;
    s_RpcReadWriteInfo.m_Size = size;
    s_RpcReadWriteInfo.m_Atomic = atomic;
    s_RpcReadWriteInfo.m_LocalWrite = localWrite;

    //
    // Issue the RPC call. This one is done synchronously.
    //
    sceSifCallRpc( &s_RpcClientData, rad1394ReadWriteInfo, 0, &s_RpcReadWriteInfo, sizeof( s_RpcReadWriteInfo ),
                   NULL, 0, NULL, NULL );

    //
    // Now lets do the actual transfer. Do it synchronously as no-wait made performance worse.
    //
    //    m_OutstandingRequest = true;

    sceSifCallRpc( &s_RpcClientData, rad1394WriteAsync, 0 /*SIF_RPCM_NOWAIT*/, source, size,
                   NULL, 0, NULL, NULL );
 
}

//=============================================================================
// Function:    rad1394Client::CheckForCompletion
//=============================================================================
// Description: Check for the completion of a previously issued asych operation.
//
// Parameters:  n/a
//
// Returns:     true - if complete, false otherwise
//
// Notes:
//------------------------------------------------------------------------------

bool rad1394Client::CheckForCompletion( void )
{
    //
    // Always return complete as the async version did not perform well.
    //
    return( true );

    if( !m_OutstandingRequest )
    {
        return( true );
    }

    if( 0 == sceSifCheckStatRpc( (sceSifRpcData*) &s_RpcClientData ) )   
    {
        m_OutstandingRequest = false;

        return( true );
    }

    return( false );
}

//=============================================================================
// Function:    rad1394Client::AddRef
//=============================================================================
// Description: Reference Management.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rad1394Client::AddRef
(
	void
)
{
	m_ReferenceCount++;
}

//=============================================================================
// Function:    rad1394Client::Release
//=============================================================================
// Description: Reference Management.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rad1394Client::Release
(
	void
)
{
	m_ReferenceCount--;

	if ( m_ReferenceCount == 0 )
	{
		delete this;
	}
}

//=============================================================================
// Function:    rad1394Client::Dump
//=============================================================================
// Description: This member is used to display object info
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

#ifdef RAD_DEBUG

void rad1394Client::Dump( char* pStringBuffer, unsigned int bufferSize )
{
    sprintf( pStringBuffer, "Object: [rad1394Client] At Memory Location:[%p]\n", this );
}

#endif


