//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include <radmemory.hpp>
#include <radsound.hpp>
#include <radmemorymonitor.hpp>
#include "memoryregion.hpp"

/* static */ radMemorySpace radSoundHalMemoryRegion::s_MemorySpace = radMemorySpace_Local;
/* static */ radSoundHalMemoryRegion * radSoundHalMemoryRegion::s_pRadSoundMemoryRegion_Root = NULL;
/* static */ unsigned int radSoundHalMemoryRegion::s_Alignment = 1;

//============================================================================
// radSoundHalMemoryRegion::Initialize
//============================================================================

/* static */ void radSoundHalMemoryRegion::Initialize
(
	void * pStartOfMemory,
	unsigned int rootSize,
    unsigned int maxRootAllocations,
	unsigned int alignment,
    radMemorySpace memorySpace,
	radMemoryAllocator allocator
)
{
	s_Alignment = alignment;
    s_MemorySpace = memorySpace;

	rAssert( s_pRadSoundMemoryRegion_Root == NULL );

	s_pRadSoundMemoryRegion_Root = new ( "radSoundHalMemoryRegion (Root)", allocator ) radSoundHalMemoryRegion( );
	s_pRadSoundMemoryRegion_Root->AddRef( );

	s_pRadSoundMemoryRegion_Root->CreateRoot( pStartOfMemory, rootSize, maxRootAllocations );
}

//============================================================================
// radSoundHalMemoryRegion::Terminate
//============================================================================

/* static */ void radSoundHalMemoryRegion::Terminate( void )
{
	rAssert( s_pRadSoundMemoryRegion_Root != NULL );

	s_pRadSoundMemoryRegion_Root->Release( );
	s_pRadSoundMemoryRegion_Root = NULL;
}

//============================================================================
// radSoundHalMemoryRegion::GetRootRegion
//============================================================================

/* static */ radSoundHalMemoryRegion * radSoundHalMemoryRegion::GetRootRegion( void )
{
	return s_pRadSoundMemoryRegion_Root;
}

//============================================================================
// radSoundHalMemoryRegion::radSoundHalMemoryRegion
//============================================================================

radSoundHalMemoryRegion::radSoundHalMemoryRegion( void )
	:
	m_pMemoryRegion_Next( NULL ),
	m_pMemoryRegion_Prev( NULL ),
	m_pMemoryRegion_ChildHead( NULL )
{
}

//============================================================================
// radSoundHalMemoryRegion::Create
//============================================================================

void radSoundHalMemoryRegion::Create
(
	radSoundHalMemoryRegion * pRadSoundMemoryRegion_Parent,
	IRadMemoryObject * pIRadMemoryObject,
    unsigned int maxAllocations
)
{
	rAssert( pRadSoundMemoryRegion_Parent != NULL );
	
	m_xRadSoundMemoryRegion_Parent = pRadSoundMemoryRegion_Parent;

	rAssert( pIRadMemoryObject != NULL );

	m_xIRadMemoryObject_HeapMemory = pIRadMemoryObject;

	::radMemoryExternalHeapCreate(
		& m_xIRadMemoryHeap,
		m_xIRadMemoryObject_HeapMemory->GetMemoryAddress( ),
		m_xIRadMemoryObject_HeapMemory->GetMemorySize( ),
        s_MemorySpace,
        maxAllocations,
        0,
		GetThisAllocator( )
	);

	m_pMemoryRegion_Next = m_xRadSoundMemoryRegion_Parent->m_pMemoryRegion_ChildHead;

	if ( m_pMemoryRegion_Next != NULL )
	{
		m_pMemoryRegion_Next->m_pMemoryRegion_Prev = this;
	}
	m_pMemoryRegion_Prev = NULL;

	m_xRadSoundMemoryRegion_Parent->m_pMemoryRegion_ChildHead = this;
}

IRadSoundHalMemoryRegion * radSoundHalMemoryRegion::CreateChildRegion
(
	unsigned int size,
    unsigned int maxAllocations,
    const char * pIdentifier
)
{
	radSoundHalMemoryRegion * pRadSoundMemoryRegion = NULL;

	radRef< IRadMemoryObject > xIRadMemoryObject = NULL;

    radMemorySetAllocationName( pIdentifier );

    if ( size > 0 )
    {
	    m_xIRadMemoryHeap->GetMemoryObjectAligned( & xIRadMemoryObject, size, s_Alignment );

	    if ( xIRadMemoryObject != NULL )
	    {
            ::radMemoryMonitorIdentifyAllocation(
                xIRadMemoryObject->GetMemoryAddress( ),
                radSoundDebugChannel,
                pIdentifier,
                NULL,
                s_MemorySpace );

		    rAssert( ((uintptr_t)xIRadMemoryObject->GetMemoryAddress( ) % s_Alignment ) == 0 );
		    rAssert( xIRadMemoryObject->GetMemorySize( ) == size );

		    pRadSoundMemoryRegion = new ( "radSoundHalMemoryRegion", GetThisAllocator( ) ) radSoundHalMemoryRegion( );

		    pRadSoundMemoryRegion->Create( this, xIRadMemoryObject, maxAllocations );

            ::radMemoryMonitorIdentifySection(
                xIRadMemoryObject->GetMemoryAddress( ),
                pIdentifier,
                s_MemorySpace );
	    }
	    else
	    {
		    rTuneString( "radSound: WARNING - Out of memory creating child region\n" );
	    }
    }

	return pRadSoundMemoryRegion;
}

//============================================================================
// radSoundHalMemoryRegion::CreateRoot
//============================================================================

void radSoundHalMemoryRegion::CreateRoot
(
	void * pStartOfMemory,
	unsigned int sizeInBytes,
    unsigned int maxAllocations
)
{
    ::radMemoryMonitorIdentifyAllocation(
        pStartOfMemory,
        radSoundDebugChannel,
        "Sound Data Root Region",
        NULL,
        s_MemorySpace );

	::radMemoryExternalHeapCreate(
		& m_xIRadMemoryHeap,
		pStartOfMemory,
		sizeInBytes,
        s_MemorySpace,
        maxAllocations,
        0,		
		GetThisAllocator( )
	);

    ::radMemoryMonitorIdentifySection(
        pStartOfMemory,
        "Sound Data Root Region",
        s_MemorySpace ); 
}

//============================================================================
// radSoundHalMemoryRegion::radSoundHalMemoryRegion
//============================================================================

/* virtual */ radSoundHalMemoryRegion::~radSoundHalMemoryRegion( void )
{
	if ( m_xRadSoundMemoryRegion_Parent != NULL )
	{
		if ( m_pMemoryRegion_Prev != NULL )
		{
			m_pMemoryRegion_Prev->m_pMemoryRegion_Next = m_pMemoryRegion_Next;
		}
		else
		{
			m_xRadSoundMemoryRegion_Parent->m_pMemoryRegion_ChildHead = m_pMemoryRegion_Next;
		}

		if ( m_pMemoryRegion_Next != NULL )
		{
			m_pMemoryRegion_Next->m_pMemoryRegion_Prev = m_pMemoryRegion_Prev;
		}
	}
}

//============================================================================
// radSoundHalMemoryRegion::CreateMemoryObject
//============================================================================

/* virtual */ void radSoundHalMemoryRegion::CreateMemoryObject
(
	IRadMemoryObject ** ppIRadMemoryObject,
	unsigned int size,
    const char * pIdentifier
)
{
    radMemorySetAllocationName( pIdentifier );

	m_xIRadMemoryHeap->GetMemoryObjectAligned( ppIRadMemoryObject, size, s_Alignment );

	if ( *ppIRadMemoryObject != NULL )
	{
        ::radMemoryMonitorIdentifyAllocation(
            (*ppIRadMemoryObject)->GetMemoryAddress( ),
            radSoundDebugChannel,
            pIdentifier,
            NULL,
            s_MemorySpace );

		rAssert( ((uintptr_t)(*ppIRadMemoryObject)->GetMemoryAddress( ) % s_Alignment ) == 0 );
		rAssert( (*ppIRadMemoryObject)->GetMemorySize( ) == size );
	}
}

//============================================================================
// radSoundHalMemoryRegion::GetSize
//============================================================================

/* virtual */ unsigned int radSoundHalMemoryRegion::GetSize( void )
{
	return m_xIRadMemoryHeap->GetSize( );
}

//============================================================================
// radSoundHalMemoryRegion::GetParent
//============================================================================

/* virtual */ IRadSoundHalMemoryRegion * radSoundHalMemoryRegion::GetParent( void )
{
	return m_xRadSoundMemoryRegion_Parent;
}
//============================================================================
// radSoundHalMemoryRegion::GetFirstChild
//============================================================================

/* virtual */IRadSoundHalMemoryRegion * radSoundHalMemoryRegion::GetFirstChild( void )
{
	return m_pMemoryRegion_ChildHead;
}

//============================================================================
// radSoundHalMemoryRegion::GetNextChild
//============================================================================

/* virtual */ IRadSoundHalMemoryRegion * radSoundHalMemoryRegion::GetNextChild
(
	IRadSoundHalMemoryRegion * pIRadSoundHalMemoryRegion_LastChild
)
{
	rAssert( pIRadSoundHalMemoryRegion_LastChild != NULL );

	return static_cast< radSoundHalMemoryRegion * >(
		pIRadSoundHalMemoryRegion_LastChild )->m_pMemoryRegion_Next;
}

//============================================================================
// radSoundHalMemoryRegion::GetStats
//============================================================================

/* virtual */ void radSoundHalMemoryRegion::GetStats
(
	unsigned int * pFreeMemory,
	unsigned * pNumObjects,
	unsigned int * pLargestBlock,
	bool recurseChildren
)
{
	if ( pFreeMemory != NULL )
	{
		*pFreeMemory = 0;
	}

	if ( pNumObjects != NULL )
	{
		*pNumObjects = 0;
	}

	if ( pLargestBlock != NULL )
	{
		*pLargestBlock = 0;
	}

	GetStats_Recurse( this, pFreeMemory, pNumObjects, pLargestBlock, recurseChildren );
}

//============================================================================
// radSoundHalMemoryRegion::GetStats_Recurse (static)
//============================================================================

/* static */ void radSoundHalMemoryRegion::GetStats_Recurse
(
	radSoundHalMemoryRegion * pRadSoundMemoryRegion,
	unsigned int * pFreeMemory, unsigned int * pNumObjects,
	unsigned int * pLargestBlock,
	bool recurse
)
{
	unsigned int _freeMemory;
	unsigned int _largestBlock;
	unsigned int _numObjects;

	pRadSoundMemoryRegion->m_xIRadMemoryHeap->GetStatus
	(
		pFreeMemory != NULL ? & _freeMemory : NULL,
		pLargestBlock != NULL ? & _largestBlock : NULL,
		pNumObjects != NULL ? & _numObjects : NULL,
		NULL
	);

	if ( pFreeMemory != NULL )
	{
		*pFreeMemory += _freeMemory;
	}

	if ( pLargestBlock != NULL )
	{
		*pLargestBlock += _largestBlock;
	}

	if ( pNumObjects != NULL )
	{
		*pNumObjects += _numObjects;
	}

	if ( recurse == true )
	{
		radSoundHalMemoryRegion * pRsmr = static_cast< radSoundHalMemoryRegion * >(
			pRadSoundMemoryRegion->GetFirstChild( ) );

		while ( pRsmr != NULL )
		{
			GetStats_Recurse( pRsmr, pFreeMemory, pLargestBlock, pNumObjects, recurse );		

			pRsmr = static_cast< radSoundHalMemoryRegion * >( pRadSoundMemoryRegion->GetNextChild( pRsmr) );
		}
	}
}

//========================================================================
// ::radSoundHalBufferCalculateBufferSize
//========================================================================

unsigned int radSoundHalBufferCalculateMemorySize
( 
    IRadSoundHalAudioFormat::SizeType resultType,
    unsigned int requestedSize, 
    IRadSoundHalAudioFormat::SizeType requestedSizeType,
    IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat
)
{
    rAssert( pIRadSoundHalAudioFormat != NULL );

    unsigned int sizeInBytes = pIRadSoundHalAudioFormat->ConvertSizeType(
        IRadSoundHalAudioFormat::Bytes, requestedSize, requestedSizeType );

    // The size of the buffer is 

    sizeInBytes = ::radMemoryRoundUp( sizeInBytes, 
        radMemorySpace_OptimalMultiple * pIRadSoundHalAudioFormat->GetNumberOfChannels( ) );

    unsigned int result = pIRadSoundHalAudioFormat->ConvertSizeType(
        resultType,
        sizeInBytes,
        IRadSoundHalAudioFormat::Bytes );

    return result;
}

#define OPTIMAL_CD_READ_MULTIPLE radFileMaxSectorSize

//========================================================================
// radSoundStreamReadMultipleGet
//========================================================================

unsigned int radSoundHalDataSourceReadAlignmentGet( void )
{
    if ( radFileOptimalMemoryAlignment > radMemorySpace_OptimalAlignment )
    {
        return radFileOptimalMemoryAlignment;
    }

    return radMemorySpace_OptimalAlignment;
}

//========================================================================
// radSoundStreamReadMultipleGet
//========================================================================

unsigned int radSoundHalDataSourceReadMultipleGet( void )
{
	unsigned int mult;
	
    if ( OPTIMAL_CD_READ_MULTIPLE > radMemorySpace_OptimalMultiple )
    {
        mult = OPTIMAL_CD_READ_MULTIPLE;
    }
	else
	{
    	mult = radMemorySpace_OptimalMultiple;
   	}
   	
   	return mult;
}