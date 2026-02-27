//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        controllerbuffer.cpp
//
// Subsystem:	Foundation Technologies - Internal Controller Buffering System
//
// Description:	This file contains the implementation of the Foundation
//              Technologies buffering system for the controller system. 
//              This module provides services for storing and retrieving
//              controller state information.
//
// Date:    	November 30, 2000
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================
#include "pch.hpp"
#include "controllerbuffer.hpp"
#include <string.h>
#include <radmemorymonitor.hpp>


void * operator new[]( size_t size, radMemoryAllocator allocator )
{
    return ::radMemoryAlloc( allocator, size );
}

//=============================================================================
// ControllerBuffer Factory
//=============================================================================

void radControllerBufferCreate
( 
    IRadControllerBuffer**  ppIRadControllerBuffer,
    radMemoryAllocator      alloc 
)
{
    *ppIRadControllerBuffer = new( alloc ) ControllerBuffer( alloc );
}

//=============================================================================
// ControllerBuffer::ControllerBuffer
//=============================================================================

ControllerBuffer::ControllerBuffer( radMemoryAllocator allocator )
    :
    m_ReferenceCount( 1 ),
    m_BufferArray( NULL ),
    m_QueueSize( 0 ),
    m_PacketSize( 0 ),
    m_HeadIndex( 0 ),
    m_TailIndex( 0 ),
    m_PacketArray( NULL ),
    m_Allocator( allocator )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "ControllerBuffer" );
}

//=============================================================================
// ControllerBuffer::~ControllerBuffer
//=============================================================================
    
ControllerBuffer::~ControllerBuffer( void )
{
    radMemoryFree( m_Allocator, m_BufferArray );
    radMemoryFree( m_Allocator, m_PacketArray );
}

//=============================================================================
// ControllerBuffer::AddRef
//=============================================================================

void ControllerBuffer::AddRef( void )
{
    m_ReferenceCount++;
}

//=============================================================================
// ControllerBuffer::Release
//=============================================================================

void ControllerBuffer::Release( void )
{
    m_ReferenceCount--;

    if( m_ReferenceCount == 0 )
    {
        delete this;
    }
}

//=============================================================================
// ControllerBuffer::Initialize
//=============================================================================

void ControllerBuffer::Initialize( unsigned int queueSize, unsigned int packetSize )
{
    rAssert( queueSize != 0 && packetSize != 0 );

    //
    // To make sure no one calls initialize twice.
    //
    rAssert( m_QueueSize == 0 );

    m_PacketSize = packetSize;

    SetQueueSize( queueSize );
}

//=============================================================================
// ControllerBuffer::SetQueueSize
//=============================================================================

void ControllerBuffer::SetQueueSize( unsigned int queueSize )
{
    if( m_BufferArray != NULL && m_PacketArray != NULL )
    {   
        delete( m_BufferArray );
        delete( m_PacketArray );
    }

    m_QueueSize = queueSize;

    m_BufferArray = new(m_Allocator) unsigned char[ m_QueueSize * m_PacketSize ];
    m_PacketArray = new(m_Allocator) unsigned char*[ m_QueueSize * sizeof( unsigned char* ) ];

    rAssert( m_BufferArray != NULL );
    rAssert( m_PacketArray != NULL );

    for( unsigned int i=0; i<m_QueueSize; i++ )
    {
        m_PacketArray[ i ] = m_BufferArray + ( m_PacketSize * i );
    }

    m_HeadIndex = 0;
    m_TailIndex = 0;
}

//=============================================================================
// ControllerBuffer::EnqueuePacket
//=============================================================================

void ControllerBuffer::EnqueuePacket( void* pPacket )
{
    memcpy( (void*)m_PacketArray[ m_TailIndex ], pPacket, m_PacketSize );

    m_TailIndex = ( m_TailIndex + 1 ) % m_QueueSize;

    //
    // If the queue is full, move the head index to point to the oldest 
    // item.
    //
    if( m_TailIndex == m_HeadIndex )
    {
        rDebugString( "radControllerSystem: Buffer Overflow\n" );

        m_HeadIndex = ( m_HeadIndex + 1 ) % m_QueueSize;
    }
}

//=============================================================================
// ControllerBuffer::PeekNextPacket
//=============================================================================

bool ControllerBuffer::PeekNextPacket( void* pBuffer, int peekSize )
{
    if( m_HeadIndex == m_TailIndex )
    {
        return false;
    }
    memcpy( pBuffer, (void*)m_PacketArray[ m_HeadIndex ], peekSize );
    return true;
}

//=============================================================================
// ControllerBuffer::DequeuePacket
//=============================================================================

bool ControllerBuffer::DequeuePacket( void* pNextPacket )
{
    if( m_HeadIndex == m_TailIndex )
    {
        return false;
    }

    memcpy( pNextPacket, (void*)m_PacketArray[ m_HeadIndex ], m_PacketSize );
    m_HeadIndex = ( m_HeadIndex + 1 ) % m_QueueSize;

    return true;
}

//=============================================================================
// ControllerBuffer::Flush
//=============================================================================

void ControllerBuffer::Flush( void )
{
    m_HeadIndex = 0;
    m_TailIndex = 0;
}

