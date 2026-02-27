//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radcontrollerbuffer.hpp
//
// Description:	This file contains all the definitions to interact with the
//              ControllerBuffer.
//
//=============================================================================

#ifndef	RADCONTROLLERBUFFER_HPP
#define RADCONTROLLERBUFFER_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radobject.hpp>                     
#include <radmemory.hpp>

//=============================================================================
// Forward Declarations
//=============================================================================

struct IRadControllerBuffer;

//=============================================================================
// Functions
//=============================================================================

void radControllerBufferCreate( IRadControllerBuffer** ppIRadControllerBuffer, 
                                radMemoryAllocator alloc = RADMEMORY_ALLOC_DEFAULT );

//=============================================================================
// Internal Interface IRadControllerBuffer
//=============================================================================

struct IRadControllerBuffer
    :
    public IRefCount
{
    virtual void Initialize( unsigned int queueSize, unsigned int bufferSize ) = 0;

    virtual void SetQueueSize( unsigned int queueSize ) = 0;

    virtual void EnqueuePacket( void* pPacket ) = 0;

    //
    // Fills in pBuffer with peekSize bytes of information contained in the 
    // next packet ( that you would get with DequeuePacket ).  This way, the
    // user could read a chunk of user-defined data at the start of the buffer
    // ( if it is arranged that way ) to determine if the packet should be
    // dequeued and used. This is useful for checking the timestamp of a packet
    // if the timestamp is added to the start of a packet.
    //
    virtual bool PeekNextPacket( void* pBuffer, int peekSize ) = 0; 

    virtual bool DequeuePacket( void* pNextPacket ) = 0; // for copying

    virtual void Flush( void ) = 0;    
};


#endif // IRADCONTROLLERBUFFER_HPP