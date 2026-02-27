//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        controllerbuffer.hpp
//
// Subsystem:	Foundation Technologies - Internal Controller Buffering System
//
// Description:	This file contains the header description of the Foundation
//              Technologies controller buffering system. This module provides 
//              services for storing and retrieving controller state information.
//
// Date:    	November 30, 2000
//
//=============================================================================

#ifndef CONTROLLERBUFFER_HPP
#define CONTROLLERBUFFER_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <radobject.hpp>
#include <raddebug.hpp>
#include <radmemory.hpp>
#include "radcontrollerbuffer.hpp"

//=============================================================================
// class ControllerBuffer
//=============================================================================

class ControllerBuffer
    :
    public IRadControllerBuffer,
    public radObject
{
	IMPLEMENT_BASEOBJECT( "ControllerBuffer" )

    public:
        ControllerBuffer( radMemoryAllocator allocator = RADMEMORY_ALLOC_TEMP );
		virtual ~ControllerBuffer( void );

		void AddRef( void );
		void Release( void ); 

        void Initialize( unsigned int queueSize, unsigned int bufferSize );
        void SetQueueSize( unsigned int queueSize );
        void EnqueuePacket( void* pPacket );
        bool PeekNextPacket( void* pBuffer, int peekSize ); 
        bool DequeuePacket( void* pNextPacket );
        void Flush( void );    

    private:
        int m_ReferenceCount;

        //
        // Holds the packets.
        // 
        unsigned char* m_BufferArray;
        
        unsigned int m_QueueSize;
        unsigned int m_PacketSize;
        unsigned int m_HeadIndex;
        unsigned int m_TailIndex;
        
        //
        // Contains pointers to packets in m_BufferArray.
        //
        unsigned char** m_PacketArray;

        radMemoryAllocator m_Allocator;
};

#endif // CONTROLLERBUFFER_HPP_