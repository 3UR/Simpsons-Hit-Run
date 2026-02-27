//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        rad1394.hpp
//
// Subsystem:	Foundation Technologies - 1394 Fire Wire Communication
//
// Description:	This file contains all definitions and interfaces required to
//              interact with 1394 Fire Wire Server running on the PS2 IOP.
//
//              It also provides the interface for use under Windows 2K.
//
// Revisions:  	Jun 20, 2001
//
//=============================================================================

#ifndef	RAD1394_HPP
#define RAD1394_HPP

//=============================================================================
// Build Configuration Check
//=============================================================================

#if !defined(RAD_WIN32)
    #error 'FTech requires definition of RAD_WIN32'
#endif
#if !defined(RAD_DEBUG) && !defined(RAD_TUNE) && !defined(RAD_RELEASE) 
    #error 'FTech requires definition of RAD_DEBUG, RAD_TUNE, or RAD_RELEASE'
#endif

//=============================================================================
// Include Files
//=============================================================================

#include <radobject.hpp>   
#include <radmemory.hpp>

//=============================================================================
// Forward Class/Struct Declarations
//=============================================================================

struct IRad1394Slave;              // IOP Interface
struct IRad1394Master;             // Windows 2k interface.

//=============================================================================
// Functions
//=============================================================================


//
// Use these to get interfaces. Interfaces vary between Ps2 and Win32
//
#ifdef RAD_WIN32

void rad1394Initialize( unsigned int sharedMemorySize );
void rad1394Terminate( void );
IRad1394Master* rad1394Get( void );

#endif

#ifdef RAD_PS2
//
// These services must frame all other calls to the FireWire system.
//
void rad1394Initialize( radMemoryAllocator alloc = RADMEMORY_ALLOC_DEFAULT );
void rad1394Terminate( void );

IRad1394Slave* rad1394Get( void );

#endif

//=============================================================================
// Interfaces
//=============================================================================

//
// This is the interface to the 1394 FireWire server running on the IOP. Currently
// the iterface is very basic. Just lets the client set up the shared memory region
// and provides asychronous accessor functions to this memory. Higher level protocols
// are left to the reader.
//
struct IRad1394Slave : public IRefCount
{
    //
    // Use this member to set the size of the shared memory region that will be 
    // addressed over the FireWire. The addresses start at 1:0 - 1:size. 
    // This member must be invoked prior to any other calls. 
    //
    virtual void SetMemorySpace( unsigned int size ) = 0;
    virtual unsigned int GetMemorySpace( void ) = 0;
     
    //
    // These members are provided to read and write the shared memory space. The calls
    // are asychoronous. Current implementation is very basic. It only allows one outstanding
    // request. Caller must ensure that only one request is ever outstanding. The caller
    // must also check for completion. The address range to read is byte addressable. Caller
    // should must align source and dest pointers to 16 and size must be a multiple of 16. Caller
    // can provide indication that it wants the read or write to occur in an atomic operation.
    // Use with care as interrupts are disabled on IOP during the read /write. 
    //
    virtual void ReadMemoryAsync( unsigned int address, unsigned int size, void* dest, bool atomic = false ) = 0;
    virtual void WriteMemoryAsync( unsigned int address, unsigned int size, void* source, bool atomic = false, bool localWrite = false ) = 0;

    //
    // Member to check for completion of pending asynch operation. Returns true if no outstanding
    // operation.
    //
    virtual bool CheckForCompletion( void ) = 0; 
};

//
// This interface is provided to access the FireWire from the host running Windows 2K.
// The host is the master and can access any device of the fire wire. Again we have strive
// for simplicity. Can be enhanced as future requirements dictate.
//
struct IRad1394Master : public IRefCount
{
    //
    // Two members are provided. They simply allow reading and writing of the slaves
    // memory. Caller must provide bus and node address. Must also provide offset into
    // slaves memory to read and write along with the number of bytes. If slave not present
    // or com error occurs, returns false. 
    //
    virtual bool ReadMemory( unsigned int busNumber, unsigned int nodeNumber, unsigned int address,
                             unsigned int size, void* dest, bool localRead = true ) = 0;

    virtual bool WriteMemory( unsigned int busNumber, unsigned int nodeNumber, unsigned int address,
                              unsigned int size, void* source ) = 0;

};
#endif