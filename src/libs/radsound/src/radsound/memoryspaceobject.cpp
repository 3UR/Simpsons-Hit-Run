//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include <radmemorymonitor.hpp>
#include "memoryspaceobject.hpp"

radSoundMemorySpaceObject::radSoundMemorySpaceObject( void )
{
    ::radStringCreate( & m_xIRadString_Identifier, GetThisAllocator( ) );
    
}

radSoundMemorySpaceObject::~radSoundMemorySpaceObject( void )
{
    m_xIRadMemoryAllocator->FreeMemoryAligned( m_pMemory );
    rDebugChannelPrintf(
        radSoundDebugChannel,
        "radsound: radSoundMemorySpaceObject destroyed: [%s]\n",
        m_xIRadString_Identifier->GetChars( ) );
}

void radSoundMemorySpaceObject::Initialize(
        radMemorySpace space,
        IRadMemoryAllocator * pIRadMemoryAllocator,
        unsigned int numberOfBytes,
        unsigned int alignment,
        const char * pIdentifier )
{
    rAssert( pIRadMemoryAllocator != NULL );
    rAssert( numberOfBytes > 0 );
    rAssertMsg( pIdentifier != NULL, "You MUST name your sound memory space objects so we can track memory" );
    
    m_xIRadString_Identifier->Copy( pIdentifier );
	m_xIRadMemoryAllocator = pIRadMemoryAllocator;
	m_RadMemorySpace = space;
	m_NumberOfBytes  = numberOfBytes;

    radMemorySetAllocationName( pIdentifier );

	m_pMemory = m_xIRadMemoryAllocator->GetMemoryAligned(
        m_NumberOfBytes, alignment );

    rAssert( m_pMemory != NULL );

    if ( m_pMemory == NULL )
    {
        rDebugPrintf( "radSoundMemorySpaceObject::OutOfMemory allocating [0x%x] Bytes.\n",
            numberOfBytes );

    }
    else
    {
        ::radMemoryMonitorIdentifyAllocation( m_pMemory, radSoundDebugChannel,
            pIdentifier, NULL, space );
    }
}
    
void * radSoundMemorySpaceObject::GetAddress( void )
{
	return m_pMemory;
}

IRadMemoryAllocator * radSoundMemorySpaceObject::GetAllocator( void )
{
	return m_xIRadMemoryAllocator;
}

radMemorySpace radSoundMemorySpaceObject::GetMemorySpace( void )
{
	return m_RadMemorySpace;
}

unsigned int radSoundMemorySpaceObject::GetSizeInBytes( void )
{
	return m_NumberOfBytes;
}

const char * radSoundMemorySpaceObject::GetIdentifier( void )
{
    return m_xIRadString_Identifier->GetChars( );
}
IRadSoundMemorySpaceObject * radSoundMemorySpaceObjectCreate( radMemoryAllocator allocator )
{
    return new ( "radSoundMemorySpaceObject", allocator ) radSoundMemorySpaceObject( );
}


