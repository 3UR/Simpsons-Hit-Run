//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        rad1394client.hpp
//
// Subsystem:	Radical 1394 Firewire Client
//
// Description:	This file contains all definitions and classes relevant to
//              the radical 1394 client
//
// Revisions:	20-June-2001 Creation
//
// Notes:       
//
//=============================================================================

#ifndef	RAD1394CLIENT_HPP
#define RAD1394CLIENT_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <rad1394.hpp>
#include <radobject.hpp>
#include <radmemory.hpp>
#include <eekernel.h>
#include <sifrpc.h>
#include "rad1394server.h"

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
// This class represents the object used to communicated with 1394 server
// running on the PS2 IOP
//
class rad1394Client : public IRad1394Slave,
					  public radObject
{
    public:

    //
    // Constructor. Nothing too interesting.
    //
    rad1394Client( radMemoryAllocator alloc );
    virtual ~rad1394Client( void );
  
	//
	// IRad1394 Interfaces
	//
    void SetMemorySpace( unsigned int size );
    unsigned int GetMemorySpace( void );
    void ReadMemoryAsync( unsigned int address, unsigned int size, void* dest, bool atomic );
    void WriteMemoryAsync( unsigned int address, unsigned int size, void* source, bool atomic, bool localWrite );
    bool CheckForCompletion( void ); 
    
    virtual void AddRef( void );
    virtual void Release( void );

    #ifdef RAD_DEBUG
    virtual void Dump( char* pStringBuffer, unsigned int bufferSize );
    #endif

    private:

    //
    // This member maintains the reference count of this object.
    //
    unsigned int m_ReferenceCount;    

    //
    // USed to peform memory allocations.
    //
    radMemoryAllocator  m_Allocator;

    //
    // Flag indicating if we have an outstanding operation.
    //
    bool m_OutstandingRequest;
};

#endif


