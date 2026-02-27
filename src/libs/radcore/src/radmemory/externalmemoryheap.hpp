//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        externalmemoryheap.hpp
//
// Subsystem:	Foundation Technologies - External Memory Services
//
// Description:	This file contains the implementation of the external memory
//              manager.  The external memory manages memory spaces which
//              is different from the heap of the calling process.
//
// Revisions:   June 20, 2000     Creation     TEH
//
//=============================================================================

#ifndef EXTERNALMEMORYHEAP_HPP
#define EXTERNALMEMORYHEAP_HPP

//============================================================================
// Include Files
//============================================================================

#include <radmemory.hpp>

//============================================================================
// Forward Declarations
//============================================================================

class ExternalMemoryObject;

//============================================================================
// ExternalMemoryHeap
//============================================================================

class ExternalMemoryHeap
	:
	public IRadMemoryHeap,
	public radObject

{
    public:

		IMPLEMENT_BASEOBJECT( "ExternalMemoryHeap" )

        ExternalMemoryHeap( void );

        // IRefCount

        virtual void AddRef( void );
        virtual void Release( void );

        // IRadMemoryHeap

        virtual void GetMemoryObject( IRadMemoryObject** ppMemoryObject, unsigned int size );
		virtual void GetMemoryObjectAligned( IRadMemoryObject ** ppIRadMemoryObject,
											 unsigned int size,
										     unsigned int alignment);

        virtual void* GetMemory( unsigned int size );
        virtual void FreeMemory( void* pMemory );
        virtual bool CanFreeMemory( void * pMemory );

        virtual void* GetMemoryAligned( unsigned int size, unsigned int alignment = 4 );
		virtual void FreeMemoryAligned( void * pMemory );      
        virtual bool CanFreeMemoryAligned( void * pMemory );

        virtual void GetStatus( unsigned int* totalFreeMemory, unsigned int* largestBlock,
			unsigned int* numberOfObjects, unsigned int * lowWaterMark );

		virtual unsigned int GetSize( void );

		virtual bool ValidateHeap( void );

        void Initialize(
			void * pStartOfExternalMemory,
            unsigned int sizeOfExternalMemory,
            radMemorySpace memorySpace,
            unsigned int maxAllocations, // size of allocation pool
            unsigned int growAllocationsBy, // grow allocation pool by
			radMemoryAllocator allocator,
            const char * pName );

	    void * operator new( size_t size, radMemoryAllocator allocator );
		void operator delete( void * pMemory );


		virtual void Chain( IRadMemoryHeap* pMemoryHeap ) { rAssert( 0 ); }

    private:

        inline unsigned int CalculateUnalignment( unsigned int val, unsigned int align );

		bool CanFree( void * pMemory );
        void Map( void );

        virtual ~ExternalMemoryHeap( void );

        ExternalMemoryObject * Allocate( unsigned int size, unsigned int alignment );
        ExternalMemoryObject * AllocateFirstFit( unsigned int size, unsigned int alignment );
        ExternalMemoryObject * AllocateLastFit( unsigned int size, unsigned int alignment );
        ExternalMemoryObject * AllocateClosestFit( unsigned int size, unsigned int alignment );
        ExternalMemoryObject * SplitBlockTop( ExternalMemoryObject * pEmo_FreeBlock, unsigned int size, unsigned int alignment );
        ExternalMemoryObject * SplitBlockBottom( ExternalMemoryObject * pEmo_FreeBlock, unsigned int size, unsigned int alignment );

        void Free( ExternalMemoryObject * pEmo_Free );

        unsigned int m_ReferenceCount;
		unsigned int m_HeapSize;
        radMemorySpace m_RadMemorySpace;

        ExternalMemoryObject * m_pEmo_First;
        ExternalMemoryObject * m_pEmo_Last;

        radRef< IRadMemoryPool > refIRadMemoryPool_Emo;

    friend class MemorySystem;
    friend class ExternalMemoryObject;
};

inline unsigned int ExternalMemoryHeap::CalculateUnalignment( unsigned int val, unsigned int align )
{
    unsigned int mod = val % align;

    if ( mod == 0 )
    {
        return 0;
    }

    return align - mod;
}



#endif // EXTERNALMEMORYHEAPIMPL_HPP