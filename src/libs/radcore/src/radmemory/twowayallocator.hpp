//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        twowayallocator.hpp
//
// Subsystem:	Foundation Technologies - Two Way Allocator
//
// Description:	This file contains the implementation of the two way allocator.
//              The two way allocator allocates memory from one side of 
//              memory to the other.  The allocator must be reset to allocate
//              from the the beginning again.
//
// Revisions:   June 27, 2002   Creation     RWS
//
//=============================================================================

#ifndef TWOWAYALLOCATOR_HPP
#define TWOWAYALLOCATOR_HPP

//============================================================================
// Include Files
//============================================================================

#include <radmemory.hpp>

//============================================================================
// Forward Declarations
//============================================================================

class radMemoryTwoWayAllocator;

//============================================================================
// radMemoryTwoWayAllocator
//============================================================================

class radMemoryTwoWayAllocator
	:
	public IRadMemoryTwoWayAllocator,
	public radRefCount

{
    public:

		IMPLEMENT_REFCOUNTED( "radMemoryTwoWayAllocator" )

        radMemoryTwoWayAllocator( void );
        virtual ~radMemoryTwoWayAllocator( void );

        void Initialize( unsigned int size );

	    // IRadMemoryAllocator

	    virtual void * GetMemory( unsigned int size );
	    virtual void FreeMemory( void* pMemory );
        virtual bool CanFreeMemory( void * pMemory );

        virtual void * GetMemoryAligned( unsigned int size, unsigned int alignment );
	    virtual void FreeMemoryAligned( void * pMemory );
        virtual bool CanFreeMemoryAligned( void * pMemory );

        // IRadMemoryTwoWayAllocator

        virtual void SetDirection( Direction direction );
        virtual Direction GetDirection( void );
        virtual void Reset( Direction direction );

    private:

        inline void * InternalAllocate( unsigned int size, unsigned int alignment );
        inline void InternalFree( void * pMemory );
        inline bool InternalCanFree( void * pMemory );

        char * m_pTop;
        char * m_pBottom;
        char * m_pCurrentTop;
        char * m_pCurrentBottom;
        unsigned int m_NumTopAllocations;
        unsigned int m_NumBottomAllocations;
        Direction m_Direction;
};

//=============================================================================
// radMemoryTwoWayAllocator::InternalAllocate
//=============================================================================

void * radMemoryTwoWayAllocator::InternalAllocate( unsigned int size, unsigned int alignment )
{
    char * pMemory = NULL;

    if( m_Direction == TopDown )
    {
        pMemory = ( char * ) ::radMemoryRoundUp( ( uintptr_t )  m_pCurrentTop, alignment );

        if( pMemory + size < m_pCurrentBottom )
        {
            m_pCurrentTop = pMemory + size;
            m_NumTopAllocations++;
        }
        else
        {
            rReleasePrintf( "radMemoryTwoWayAllocator: Out of memory.\n" );
            rAssert( pMemory + size < m_pCurrentBottom );
            pMemory = NULL;
        }
    }
    else
    {
        pMemory = ( char * ) ::radMemoryRoundDown( ( uintptr_t ) m_pCurrentBottom - size, alignment );

        if( pMemory > m_pCurrentTop )
        {
            m_pCurrentBottom = pMemory;
            m_NumBottomAllocations++;
        }
        else
        {
            rReleasePrintf( "radMemoryTwoWayAllocator: Out of memory.\n" );
            rAssert( pMemory > m_pCurrentTop );
            pMemory = NULL;
        }
    }

    radMemoryMonitorDeclareAllocation( pMemory, size );

    return pMemory;
}

//=============================================================================
// radMemoryTwoWayAllocator::InternalCanFree
//=============================================================================

bool radMemoryTwoWayAllocator::InternalCanFree( void * pMemory )
{
    return( pMemory >= m_pTop && pMemory < m_pBottom );
}

//=============================================================================
// radMemoryTwoWayAllocator::InternalFree
//=============================================================================

void radMemoryTwoWayAllocator::InternalFree( void * pMemory )
{
    if( pMemory != NULL )
    {
        rAssert( InternalCanFree( pMemory ) == true );

        // Decrement appropriate counter

        if( pMemory < m_pCurrentBottom )
        {
            rAssert( m_NumTopAllocations > 0 );
            m_NumTopAllocations--;
        }
        else
        {
            rAssert( m_NumBottomAllocations > 0 );
            m_NumBottomAllocations--;
        }

        radMemoryMonitorRescindAllocation( pMemory );
    }
}



#endif // TWOWAYALLOCATOR_HPP