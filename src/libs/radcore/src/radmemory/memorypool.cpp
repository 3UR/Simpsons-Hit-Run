//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        memorypool.cpp
//
// Subsystem:	Foundation Technologies - Memory
//
// Description:	This file contains the implementation of the Foundation 
//              Technologies memory pool access.
//
// Date:    	May 02, 2000
//
// Author:      Donnie Gossett
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <stdlib.h>
#include <raddebug.hpp>
#include <radobject.hpp>
#include <radmemory.hpp>
#include <radmemorymonitor.hpp>
#include "memory.hpp"

//=============================================================================
// Local Defintions
//=============================================================================

//=============================================================================
// Static Data Defintions
//=============================================================================

//=============================================================================
// Function:    radMemoryCreatePool
//=============================================================================
// Description: This interface is used to construct a pool object. A pool 
//				object is a memory management scheme whereby objects of a fixed 
//				size can be allocated and freed.  This scheme is can allocate 
//				and free objects extremely fast.
//
// Parameters:  ppPool              Pointer to biMemoryPool object.
//              elementSize			size of individual fixed size memory blocks 
//									in bytes.
//				numberOfElements	number of fixed size memory blocks.
//				isThreadSafe		determines if threadsafe support required.
//				debugLevel			determines degree of debugging support 
//									required.
//
// Returns:     void
//
// Notes:
//------------------------------------------------------------------------------
    
void radMemoryCreatePool
( 
    IRadMemoryPool**        ppPool, 
	unsigned int            elementSize, 
    unsigned int            numberOfElements,
	unsigned int			growBy,
    bool                    isThreadSafe, 
    radMemoryDebugLevel     debugLevel,
	radMemoryAllocator      allocator,
    const char *            pName
)
{
	rAssert( ppPool != NULL );
	rAssert( elementSize > 0 );
	rAssert( numberOfElements > 0 );
	rAssert( !isThreadSafe );
	rAssert( debugLevel <= High );

	MemoryPool* pMemoryPool = new( allocator ) MemoryPool();
	rAssert( pMemoryPool != NULL );

	pMemoryPool->Initialize
	( 
		NULL, 
		elementSize, 
		numberOfElements, 
		growBy,
		isThreadSafe, 
		debugLevel, 
		allocator,
        pName
	);
	*ppPool = pMemoryPool;
}

//=============================================================================
// Function:    radMemoryCreatePool
//=============================================================================
// Description: This interface is used to construct a pool object. A pool 
//				object is a memory management scheme whereby objects of a fixed 
//				size can be allocated and freed.  This scheme is can allocate 
//				and free objects extremely fast.
//
// Parameters:  ppPool              returns Pointer to biMemoryPool object.
//              placement			pointer to pre-defined memory block for
//									pool
//				elementSize			size of individual fixed size memory blocks 
//									in bytes.
//				numberOfElements	number of fixed size memory blocks.
//				isThreadSafe		determines if threadsafe support required.
//				debugLevel			determines degree of debugging support 
//									required.
//
// Returns:     void
//
// Notes:
//------------------------------------------------------------------------------
    
void radMemoryCreatePool
( 
    IRadMemoryPool** ppPool, 
	void*            placement,
	unsigned int     elementSize, 
    unsigned int     numberOfElements,
	unsigned int     growBy,
    bool             isThreadSafe, 
    radMemoryDebugLevel  debugLevel,
	radMemoryAllocator   allocator,
    const char *    pName
)
{
	rAssert( ppPool != NULL );
	rAssert( placement != NULL );
	rAssert( elementSize > 0 );
	rAssert( numberOfElements > 0 );
	rAssert( !isThreadSafe );
	rAssert( debugLevel <= High );

	MemoryPool* pMemoryPool = new( allocator ) MemoryPool();
	rAssert( pMemoryPool != NULL );

	pMemoryPool->Initialize
	( 
		placement, 
		elementSize, 
		numberOfElements, 
		growBy,
		isThreadSafe, 
		debugLevel, 
		(radMemoryAllocator) -1,
        pName
	);
	*ppPool = pMemoryPool;
}

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    MemoryPool::MemoryPool
//=============================================================================
// Description: Constructor for MemoryPool.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

MemoryPool::MemoryPool
( 
)	
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "MemoryPool" );
}

//=============================================================================
// Function:    MemoryPool::Initialize
//=============================================================================
// Description: Constructor for MemoryPool.
//
// Parameters:  placement			pointer to pre-allocated memory buffer.
//				elementSize			size of single element in pool.
//				numberOfElements	number of elements in pool.
//				isThreadSafe		flag to determine re-entrancy support.
//				debugLevel			flag to determine level of debug support.
//				placement			flag to determine whether memory block needs to
//									be freed on exit.
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void MemoryPool::Initialize
(
	void*                      pPlacement,
	unsigned int               elementSize, 
    unsigned int               numberOfElements,
	unsigned int			   growBy,
    bool                       isThreadSafe,
	radMemoryDebugLevel        debugLevel,
	radMemoryAllocator         allocator,
    const char *               pName
)
{			
	rAssert( elementSize > 0 );
	rAssert( numberOfElements > 0 );
	rAssert( !isThreadSafe );
	rAssert( debugLevel <= High );

	m_GrowBy			   = growBy;
	m_pPlacement           = pPlacement;
	m_RequestedElementSize = elementSize;
	m_NumberOfElements	   = numberOfElements;
	m_IsThreadSafe		   = isThreadSafe;
	m_DebugLevel		   = debugLevel;
	m_Chain				   = NULL;
    m_ReferenceCount	   = 1;                  // Initial reference count

	m_Allocator            = allocator;

	m_NumberFree           = m_NumberOfElements;
	m_NumberAllocated      = 0;

	// Calculate and adjust (as appropriate) sizes.
	m_ActualElementSize = RoundUpAlignment( elementSize, STANDARD_ALIGNMENT );
	m_FillSize          = m_ActualElementSize - sizeof( MemList );
	m_FillSize         /= sizeof( unsigned int );
	m_Size              = m_ActualElementSize * numberOfElements;

	if( pPlacement == NULL )
	{
		m_pPlacement = radMemoryAlloc( allocator, m_Size );
        rAssert( m_pPlacement != NULL );
        radMemoryMonitorIdentifyAllocation( m_pPlacement, g_nameFTech, "MemoryPool::m_pPlacement" );
		pPlacement = m_pPlacement;
	}

	pPlacement = (void*)RoundUpAlignment( (uintptr_t)pPlacement, STANDARD_ALIGNMENT );
	rAssert( pPlacement != NULL );

    radMemoryMonitorDeclareSection( pPlacement, m_Size,  IRadMemoryMonitor::MemorySectionType_DynamicData );
    
#ifdef RADMEMORYMONITOR
    {
    char szPoolName[ 128 ];
    sprintf( szPoolName, "%s[MemoryPool]", pName );
    radMemoryMonitorIdentifySection( pPlacement, szPoolName );
    }
#endif

	m_First    = (MemList*)pPlacement;
	m_FreeHead = m_First;

	// Walk through list and initialize free list.
	MemList* curList = m_First;
	unsigned char* pointer = NULL;
	for
	( 
		unsigned int iBlock = 0;
		iBlock < m_NumberOfElements;
		++iBlock
	)
	{
		pointer             = (unsigned char*)curList;
		pointer            += m_ActualElementSize;
	    curList->m_NextFree = (MemList*)pointer;
#ifdef RAD_DEBUG
		FillFreeBlock( curList );
#endif
		curList = curList->m_NextFree;
	}

	pointer             = (unsigned char*)curList;
	pointer            -= m_ActualElementSize;
	curList             = (MemList*)pointer;
	curList->m_NextFree = NULL;
	m_FreeTail          = curList;
	m_Last          = m_FreeTail;

 	rAssert( Validate() );
}
		
//=============================================================================
// Function:    MemoryPool:GetMemory
//=============================================================================
// Description: Use this member to get a memory object. Will return null if the 
//				request cannot be fullfilled. Use delete reference of the object
//				to free the memory.
//
// Parameters:  none
//
// Returns:     Pointer to memory block allocated by this call.
//
// Notes:
//------------------------------------------------------------------------------

void* MemoryPool::GetMemory
( 
	void 
)
{
	rAssert
	( 
		((m_FreeHead == NULL) && (m_FreeHead == NULL))
		||
		((m_FreeHead != NULL) && (m_FreeHead != NULL))
	);

	void* pointer = NULL;

AttemptGet:
	if( m_FreeHead != NULL )
	{
		// Use first entry on free list.
		MemList* curList = m_FreeHead;
		pointer          = (void*)m_FreeHead;

		if( m_FreeHead == m_FreeTail )
		{
			rAssert( m_FreeHead->m_NextFree == NULL );
			m_FreeHead = m_FreeTail = NULL;
		}
		else
		{
			m_FreeHead = m_FreeHead->m_NextFree;
		}
		curList->m_NextFree = NULL;
		AddRef();

		rAssert( m_NumberFree > 0 );
		--m_NumberFree;
		++m_NumberAllocated;
		rAssert( m_NumberAllocated <= m_NumberOfElements );

		rAssert
		(
			FillAndValidate
			( 
				pointer, 
				USED_VALUE, 
				m_ActualElementSize / sizeof( unsigned int ) 
			)
		);
	}
	else
	{
		// Pass on if chained defined.
		if( m_Chain != NULL )
		{
			pointer = m_Chain->GetMemory();
		}
		else
		{
			if ( m_Allocator != (unsigned int)-1 )
			{
				if ( m_GrowBy > 0 )
				{
					rDebugString( "radMemoryPool: Out of pool elements, growing!\n" );

					IRadMemoryPool * pIRadMemoryPool = NULL;

					::radMemoryCreatePool(
						& pIRadMemoryPool,
						m_RequestedElementSize,
						m_NumberOfElements,
						m_GrowBy,
						m_IsThreadSafe,
						m_DebugLevel,
						m_Allocator,
                        "OverGrow" );

					Chain( pIRadMemoryPool );

                    radRelease( pIRadMemoryPool, this );

					goto AttemptGet;
				}
				else
				{
					rDebugString( "radMemoryPool: Out of memory, and grow size is zero\n" );
				}
			}
			else
			{
				rDebugString( "radMemoryPool: Out of memory, can't grow placement pool\n" );
			}
		}
	}

#if defined RADMEMORYMONITOR
    if ( pointer != NULL )
    {
        radMemoryMonitorDeclareAllocation( pointer, m_RequestedElementSize );
    }
#endif // RADMEMORYMONITOR
	return( pointer );
}

//=============================================================================
// Function:    MemoryPool:GetMemory
//=============================================================================
// Description: Same as above function but takes a size parameneter, this is
//      similar to a heap and does debug checking on the size.
//
// Parameters:  unsigned int element size
//
// Returns:     Pointer to memory block allocated by this call.
//
// Notes:
//------------------------------------------------------------------------------

void* MemoryPool::GetMemory( unsigned int size )
{
    rAssertMsg( size <= m_ActualElementSize, "MemoryPool: Error: Memory size requested is larger than the size of memory pool element." );

    void * pMemory = GetMemory( );

    return pMemory;
}

//=============================================================================
// Function:    MemoryPool:FreeMemory
//=============================================================================
// Description: Frees previously allocated memory block.
//
// Parameters:  pMemory		pointer to memory block
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void MemoryPool::FreeMemory
( 
	void* pMemory 
)
{
	rAssert
	( 
		((m_FreeHead == NULL) && (m_FreeHead == NULL))
		||
		((m_FreeHead != NULL) && (m_FreeHead != NULL))
	);

	rAssert( pMemory != NULL );
	if( WithinRange( pMemory ) )
	{
		rAssert( ValidPointer( pMemory ) );

		MemList* curList = (MemList*)pMemory;
		rAssert( WithinRange( curList ) );

		// Add freed block to end of free list.
		if( m_FreeTail == NULL )
		{
			m_FreeHead = curList;
		}
		else
		{
			m_FreeTail->m_NextFree = curList;
		}
		curList->m_NextFree = NULL;
		m_FreeTail          = curList;
		Release();

		++m_NumberFree;
		rAssert( m_NumberFree <= m_NumberOfElements );
		rAssert( m_NumberAllocated > 0 );
		--m_NumberAllocated;

#if defined RADMEMORYMONITOR
        if ( pMemory != NULL )
        {
            radMemoryMonitorRescindAllocation( pMemory );
        }
#endif // RADMEMORYMONITOR

#ifdef RAD_DEBUG
		if( m_DebugLevel >= Moderate )
		{
			FillFreeBlock( curList );
			rAssert( Validate() );
		}
#endif 
	}
	else
	{
		// Pass on to chain if defined.
		if( m_Chain != NULL )
		{
			m_Chain->FreeMemory( pMemory );
		}
		else
		{
			// Complain if request is invalid.
			rAssert( WithinRange( pMemory ) );
		}
	}
}

//=============================================================================
// Function:    MemoryPool:CanFreeMemory
//=============================================================================
// Description:	Can memory be freed from this pool?
//
// Parameters:  pMemory		pointer to memory to be freed
//
// Returns:     n/a
//
// Notes:       true if memory can be freed, false otherwise
//------------------------------------------------------------------------------

bool MemoryPool::CanFreeMemory( void * pMemory )
{
	rAssert
	( 
		((m_FreeHead == NULL) && (m_FreeHead == NULL))
		||
		((m_FreeHead != NULL) && (m_FreeHead != NULL))
	);

	rAssert( pMemory != NULL );
	return WithinRange( pMemory );
}

//=============================================================================
// Function:    MemoryPool:GetMemoryAligned
//=============================================================================
// Description:	Can memory be freed from this pool?
//
// Parameters:  size		size of the memory in byte
//              alignment   alignment in bytes
//
// Returns:     address of the memory block
//
// Notes:       true if memory can be freed, false otherwise
//------------------------------------------------------------------------------

void* MemoryPool::GetMemoryAligned( unsigned int size, unsigned int alignment )
{
    (void)size;
    (void)alignment;
    rAssertMsg( false, "MemoryPool: Error: MemoryPool::GetMemoryAligned() is not implemented yet." );
    return NULL;
}

//=============================================================================
// Function:    MemoryPool:FreeMemoryAligned
//=============================================================================
// Description:	Free memory from memory pool (aligned memory)
//
// Parameters:  pMemory		pointer to memory to be freed
//
// Returns:     n/a
//
// Notes:       true if memory can be freed, false otherwise
//------------------------------------------------------------------------------

void  MemoryPool::FreeMemoryAligned( void * pMemory )
{
    (void)pMemory;
    rAssertMsg( false, "MemoryPool: Error: MemoryPool::FreeMemoryAligned() is not implemented yet." );
}

//=============================================================================
// Function:    MemoryPool:CanFreeMemoryAligned
//=============================================================================
// Description:	Can memory be freed from this pool (aligned memory?
//
// Parameters:  pMemory		pointer to memory to be freed
//
// Returns:     n/a
//
// Notes:       true if memory can be freed, false otherwise
//------------------------------------------------------------------------------

bool  MemoryPool::CanFreeMemoryAligned( void * pMemory )
{
    (void)pMemory;
    rAssertMsg( false, "MemoryPool: Error: MemoryPool::CanFreeMemoryAligned() is not implemented yet." );
    return false;
}

// Function:    MemoryPool:GetStatus
//=============================================================================
// Description:	This member is used to determine the information about pool 
//				object. It reports the element size, the number of free 
//				elements, the number of allocated elements.
//
// Parameters:  elementSize		pointer to size of element in this pool.
//				numberFree		pointer to number of free objects in this pool.
//				numberAllocated	pointer to number of allocated objects in this pool.
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void MemoryPool::GetStatus
( 
	unsigned int* elementSize, 
	unsigned int* numberFree,
    unsigned int* numberAllocated 
)
{

	unsigned int chainElementSize = 0; 
	unsigned int chainNumberFree = 0;
	unsigned int chainNumberAllocated = 0; 

	if( m_Chain != NULL )
	{


		// Get chain status
		m_Chain->GetStatus( & chainElementSize, & chainNumberFree, & chainNumberAllocated );

		// Validate or add chain status to this pool's status
		rAssert( chainElementSize == *elementSize );
		*numberFree      += chainNumberFree;
		*numberAllocated += chainNumberAllocated;
	}


	if ( elementSize != NULL )
	{
		*elementSize = m_RequestedElementSize;
	}

	if ( numberFree != NULL )
	{
		*numberFree      = m_NumberFree + chainNumberFree;
	}

	if ( numberAllocated != NULL )
	{
		*numberAllocated = m_NumberAllocated + chainNumberAllocated;
	}
}
          
//=============================================================================
// Function:    MemoryPool:Chain
//=============================================================================
// Description:	This member is used to chain another memory pool to this memory 
//				pool. It is a way of expanding the first pool object. Once 
//				chained. memory can be allocated from either pool using the 
//				first pool object. The element size of both pools must be 
//				identical.
//
// Parameters:  pMemoryPool	pointer to memory pool object to chain to.
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void MemoryPool::Chain
( 
	IRadMemoryPool* pMemoryPool 
)
{
#ifdef RAD_DEBUG
	if( m_DebugLevel >= Moderate )
	{
		if( pMemoryPool != NULL )
		{
			// Verify that other pool is same size
			unsigned int elementSize;
			unsigned int numberFree;
			unsigned int numberAllocated;

			pMemoryPool->GetStatus( &elementSize, &numberFree, &numberAllocated );
			rAssert( elementSize == m_RequestedElementSize );
		}
	}
#endif //debug

	if( pMemoryPool == NULL )
	{
		if( m_Chain != NULL )
		{
            radRelease( m_Chain, this );
		}
	}
	else
	{
		rAssert( m_Chain == NULL );
        radAddRef( pMemoryPool, this );
	}
	m_Chain = static_cast< MemoryPool* >( pMemoryPool );

	rAssert( (m_Chain == NULL) || m_Chain->Validate() );
}

//=============================================================================
// Function:    MemoryPool:AddRef
//=============================================================================
// Description: Invoke this member anytime another object is given a pointer to 
//				this object. Simply updates the reference count.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void MemoryPool::AddRef
( 
	void 
)
{
    m_ReferenceCount++;
}

//=============================================================================
// Function:    MemoryPool:Release
//=============================================================================
// Description:	This member should be used instead of delete. It will ensure 
//				correct management of the objects lifetime.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void MemoryPool::Release
( 
	void 
)   
{
    // Decrement the reference count. If zero, delete the object.
	rAssert( m_ReferenceCount != 0 );    
    m_ReferenceCount--;
        
    if( m_ReferenceCount == 0 )
    {
        delete this;
    }
}

//=============================================================================
// Function:    MemoryPool:~MemoryPool
//=============================================================================
// Description: Destructor for memory pool object.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

MemoryPool::~MemoryPool
(
)
{
	rAssert( m_NumberAllocated == 0 );
	rAssert( m_ReferenceCount == 0 );
	    
	Chain( NULL );
	if( m_pPlacement != NULL )
	{
        radMemoryMonitorRescindSection( m_First );
		radMemoryFree( m_pPlacement );
		m_pPlacement = NULL;
	}
}

//=============================================================================
// Function:    MemoryPool::operator new
//=============================================================================
// Description: new for allocator system.
//
// Parameters:  size of object to allocate memor for.
//	            type of allocator to use.
//
// Returns:     pointer to allocated object
//
// Notes:
//------------------------------------------------------------------------------

void* MemoryPool::operator new
(
    size_t size,
    radMemoryAllocator allocator
)
{
    return radMemoryAlloc( allocator, size );
}

//=============================================================================
// Function:    MemoryPool::operator delete
//=============================================================================
// Description: new for allocator system.
//
// Parameters:  pointer to memory for object.
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void MemoryPool::operator delete( void * pMemory )
{
    radMemoryFree( pMemory );
}

#ifdef RAD_DEBUG
//=============================================================================
// Function:    MemoryPool:FillAndValidate
//=============================================================================
// Description: This members fills a block and validates the pool.
//
// Parameters:  pMemory	memory block to fill
//				value	value to fill memory block with
//				size	number of "value" elements in block
//
// Returns:     true if pool valid.
//				false if pool NOT valid.
//
// Notes:
//------------------------------------------------------------------------------

bool MemoryPool::FillAndValidate
( 
	void*        pMemory, 
	unsigned int value, 
	unsigned int size 
)
{
	if( m_DebugLevel >= Moderate )
	{
		if( m_DebugLevel == High )
		{
			MemorySet( pMemory, value, size );
		}
		rAssert( Validate() );
	}
	return( true );
}

//=============================================================================
// Function:    MemoryPool:FillFreeBlock
//=============================================================================
// Description: This members fills a free block (except for where the MemList
//				structure lives).
//
// Parameters:  curList	pointer to current block
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void MemoryPool::FillFreeBlock
( 
	MemList* curList 
)
{
	if( m_DebugLevel >= High )
	{
		if( m_ActualElementSize > sizeof( MemList ) )
		{
			MemorySet( curList + 1, FREE_VALUE, m_FillSize );
		}
	}
}

//=============================================================================
// Function:    MemoryPool:Validate
//=============================================================================
// Description: This is equivalent to "heapwalk()" in other standard heap 
//				managers.
//
// Parameters:  none
//
// Returns:     True if current heap is valid.
//				False if current heap is NOT valid.
//
// Notes:
//------------------------------------------------------------------------------

bool MemoryPool::Validate
( 
	void 
)
{
	if( m_DebugLevel < Moderate )
	{
		return( true );
	}
	bool bValidate = true;

	// Walk through pool list to validate each member.
	for
	(
		MemList* curList = m_FreeHead;
		(curList != m_FreeTail) && (curList != NULL);
		curList = curList->m_NextFree
	)
	{
		rAssert( WithinRange( curList ) );
		if( m_DebugLevel == High )
		{
			rAssert
			( 
				MemoryCheck( curList + 1, FREE_VALUE, m_FillSize ) 
			);
		}
	}

	if( m_Chain != NULL )
	{
		bValidate = m_Chain->Validate();
	}
	return( bValidate );
}

//=============================================================================
// Function:    MemoryPool:ValidatePointer
//=============================================================================
// Description: Determines if the given pointer is a valid allocated pointer, 
//				previously allocated by Allocate().
//
// Parameters:  pointer		pointer to validate.
//
// Returns:     True if pointer is valid for this pool.
//				False if pointer is NOT valid for this pool.
//
// Notes:
//------------------------------------------------------------------------------

bool MemoryPool::ValidPointer
( 
	void* pointer 
)
{
	if( m_DebugLevel < Moderate )
	{
		return( true );
	}

	for
	( 
		MemList* curList = m_First; 
		curList <= m_Last; 
		curList = (MemList*)(((unsigned char*)(curList)) + m_ActualElementSize)
	)
	{
		if( (MemList*)pointer == curList )
		{
			return( true );
		}
	}
	return( false );
}
#endif //debug

//=============================================================================
// Function:    MemoryPool:WithinRange
//=============================================================================
// Description: Determines if a given pointer is valid within the range of this
//				pool.
//
// Parameters:  pointer		pointer to validate.
//
// Returns:     True if pointer is valid for this heap.
//				False if pointer is NOT valid for this heap.
//
// Notes:
//------------------------------------------------------------------------------

bool MemoryPool::WithinRange
( 
	void* pointer 
)
{
	rAssert( m_First != NULL );
	rAssert( m_Last != NULL );
	if( pointer >= m_First )
	{
		if( pointer <= m_Last )
		{
			return( true );
		}
	}
	return( false );
}

//=============================================================================
// Function:	MemoryCheck
//=============================================================================
// Description:	Performs memory integrity check for a memory block initialized
//      		to a given value.
//
// Parameters:  block - Pointer to user's block to examine
//				value - value to check for
//				size - Size of user's block to examine
//
// Returns:     0 means no errors (or successful memory check).
//      		>0 is the number of error founds in memory check.
//
// Notes:       
//------------------------------------------------------------------------------

bool MemoryCheck
( 
	void*        block,  // Pointer to user's block to examine
	unsigned int value,  // value to check for
	unsigned int size    // Size of user's block to examine
)
{
	rAssert( block != NULL );
	unsigned int* block32 = (unsigned int*)block;
	for( unsigned int i = 0; i < size; ++i, ++block32 )
	{
		if( *block32 != value )
		{
			return( false );
		}
	}
	return( true );
}

//=============================================================================
// Function:	MemorySet
//=============================================================================
// Description: Initializes memory block to a given value.
//
// Parameters:  block - Pointer to user's block to initialize
//				value - value to initialize to
//				size - Size of user's block to initialize
//
// Returns:     n/a
//
// Notes:       
//------------------------------------------------------------------------------

void MemorySet
( 
	void*        block,  // Pointer to user's block to initialize
	unsigned int value,  // value to initialize to
	unsigned int size    // Size of user's block to initialize
)
{
	rAssert( block != NULL );
	unsigned int* block32 = (unsigned int*)block;
	for( unsigned int i = 0; i < size; ++i, ++block32 )
	{
		*block32 = value;
	}
}

//=============================================================================
// Function:	RoundUpAlignment
//=============================================================================
// Description: Rounds up a number to a given multiple of alignment.
//
// Parameters:  Value to round up
//
// Returns:     Value rounded up to nearest multiple of alignment.
//
// Notes:       
//------------------------------------------------------------------------------

uintptr_t RoundUpAlignment
( 
    uintptr_t value,
    uintptr_t alignment
)
{
	return( value + (alignment - 1) & (~(alignment - 1)) );
}

