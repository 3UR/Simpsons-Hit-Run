//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        externalmemoryheap.cpp
//
// Subsystem:   Foundation Technologies - External Memory Services
//
// Description: This file contains the implementation of the external memory
//              manager.  The external memory manages memory spaces which
//              is different from the heap of the calling process.
//
// Revisions:   June 20, 2000     Creation     TEH
//
//=============================================================================

//============================================================================
// Include Files
//============================================================================

#include "pch.hpp"
#include <stdlib.h> 
#include <raddebug.hpp>
#include <radmemory.hpp>
#include <radmemorymonitor.hpp>
#include "externalmemoryobject.hpp"
#include "externalmemoryheap.hpp"

//=============================================================================
// Function:    radMemoryExternalHeapCreate
//=============================================================================
// Description: This interface is used to construct a external memory heap
//              object. An external memory heap is memory that is not part of
//              the processes' memory. (i.e., Sound memory ).
//
// Parameters:  ppExternalHeap      returns heap
//              pStartAddress       Address of the first valid memory address
//              size                Number of bytes in the heap.
//              heapManagementSize  Bytes to use in normal process heap for
//                                  managing the external memory.
//
// Returns:     void
//
// Notes:
//------------------------------------------------------------------------------

void radMemoryExternalHeapCreate
(
    IRadMemoryHeap** ppExternalHeap, 
    void*          pStartOfExternalMemory,
    unsigned int sizeOfExternalMemory,
    radMemorySpace memorySpace,
    unsigned int maxAllocations, // size of allocation pool
    unsigned int growAllocationsBy, // grow allocation pool by
    radMemoryAllocator   allocator,
    const char * pName
)
{
    *ppExternalHeap = NULL;

    ExternalMemoryHeap * pExternalMemoryHeap = new( allocator ) ExternalMemoryHeap( );

    pExternalMemoryHeap->Initialize
    (
        pStartOfExternalMemory,
        sizeOfExternalMemory,
        memorySpace,
        maxAllocations,
        growAllocationsBy,
        allocator,
        pName
    );

    *ppExternalHeap = pExternalMemoryHeap;

}

//=============================================================================
// Function:    ExternalMemoryHeap::operator new
//=============================================================================
// Description: new for allocator system.
//
// Parameters:  size of object to allocate memor for.
//              type of allocator to use.
//
// Returns:     pointer to allocated object
//
// Notes:
//------------------------------------------------------------------------------

void* ExternalMemoryHeap::operator new
(
    size_t size,
    radMemoryAllocator allocator
)
{
    return radMemoryAlloc( allocator, size );
}

//=============================================================================
// Function:    ExternalMemoryHeap::operator delete
//=============================================================================
// Description: new for allocator system.
//
// Parameters:  pointer to memory for object.
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void ExternalMemoryHeap::operator delete( void * pMemory )
{
    radMemoryFree( pMemory );
}

//============================================================================
// ExternalMemoryHeap::AddRef
//============================================================================

/* virtual */ void ExternalMemoryHeap::AddRef
(
    void
)
{
    m_ReferenceCount++;
}

//============================================================================
// ExternalMemoryHeap::Release
//============================================================================

/* virtual */ void ExternalMemoryHeap::Release
(
    void
)
{
    m_ReferenceCount--;

    //
    // If no clients are holding a reference to us, we can't destroy ourselves
    // until all of our memory objects are freed.  These hold week references
    // on us.  So when we delete our last used object, we must delete ourselves.
    //

    if
    (
        ( m_ReferenceCount == 0 ) &&
        ( m_pEmo_First == m_pEmo_Last ) &&
        ( m_pEmo_First->m_ReferenceCount == 0 )
    )
    {
        delete this;
    }
}

//============================================================================
// ExternalMemoryHeap::GetMemoryObject
//============================================================================

/* virtual */ void ExternalMemoryHeap::GetMemoryObject
(
    IRadMemoryObject** ppMemoryObject,
    unsigned int size
)
{
    *ppMemoryObject = Allocate( size, 4 );
}

//============================================================================
// ExternalMemoryHeap::GetMemoryObjectAligned
//============================================================================

/* virtual */ void ExternalMemoryHeap::GetMemoryObjectAligned
(
    IRadMemoryObject ** ppIRadMemoryObject, unsigned int size,
    unsigned int alignment
)
{
    *ppIRadMemoryObject = Allocate( size, alignment );
}


//============================================================================
// ExternalMemoryHeap::GetMemory
//============================================================================

/* virtual */ void* ExternalMemoryHeap::GetMemory
(
    unsigned int size
)
{
    IRadMemoryObject * pIMemoryObject = Allocate( size, 4 );
   
    //
    // We just return the memory pointer.  We don't delete reference because
    // a reference count of > 0 means that the memory is used.  We will find
    // the block later and delete it ourselves.
    //

    if ( pIMemoryObject != NULL )
    {
        return pIMemoryObject->GetMemoryAddress( );
    }

    return NULL;
}

//============================================================================
// ExternalMemoryHeap::GetMemory
//============================================================================
/* virtual */ void* ExternalMemoryHeap::GetMemoryAligned
( 
    unsigned int size, 
    unsigned int alignment 
)
{
    ExternalMemoryObject * pMemoryObject = Allocate( size, alignment );
   
    //
    // We just return the memory pointer.  We don't delete reference because
    // a reference count of > 0 means that the memory is used.  We will find
    // the block later and delete it ourselves.
    //

    if ( pMemoryObject != NULL )
    {
        return pMemoryObject->GetMemoryAddress( );
    }

    return NULL;
}

//============================================================================
// ExternalMemoryHeap::CanMemory
//============================================================================

/* virtual */ bool ExternalMemoryHeap::CanFree
(
    void * pMemory
)
{
    //
    // Find the memory object and release it
    //

    ExternalMemoryObject * pEmo_Free = m_pEmo_First;

    while ( ( pEmo_Free != NULL ) &&
            ( pEmo_Free->m_ClientAddress != reinterpret_cast< uintptr_t >( pMemory ) ) )
    {
        pEmo_Free = pEmo_Free->m_pEmo_Next;
    }

    if
    (
        ( m_ReferenceCount == 0 ) &&
        ( m_pEmo_First == m_pEmo_Last ) &&
        ( m_pEmo_First->m_ReferenceCount == 0 )
    )
    {
        return( false );
    }
    else
    {
        return( true );
    }
}

//============================================================================
// ExternalMemoryHeap::FreeMemory
//============================================================================

/* virtual */ void ExternalMemoryHeap::FreeMemory
(
    void * pMemory
)
{
    rAssert( CanFree( pMemory ) );



    //
    // Find the memory object and release it
    //

    ExternalMemoryObject * pEmo_Free = m_pEmo_First;

    while( ( pEmo_Free != NULL ) && ( pEmo_Free->m_ClientAddress != reinterpret_cast< uintptr_t >( pMemory ) ) )
    {
        pEmo_Free = pEmo_Free->m_pEmo_Next;
    }

    if
    (
        ( m_ReferenceCount == 0 ) &&
        ( m_pEmo_First == m_pEmo_Last ) &&
        ( m_pEmo_First->m_ReferenceCount == 0 )
    )
    {
        rAssertMsg( false, "ExternalMemoryHeap::FreeMemory - Block Not Allocated." );
    }
    else
    {
        radRelease( pEmo_Free, this );
    }
}

void ExternalMemoryHeap::FreeMemoryAligned
( 
    void* pMemory 
)
{
    
    rAssert( CanFree( pMemory ) );

    FreeMemory( pMemory );
}

bool ExternalMemoryHeap::CanFreeMemory( void * pMemory )
{
    return CanFree( pMemory );
}

bool ExternalMemoryHeap::CanFreeMemoryAligned( void * pMemory )
{
    return CanFree( pMemory );
}

//============================================================================
// ExternalMemoryHeap::GetStatus
//============================================================================

/* virtual */ void ExternalMemoryHeap::GetStatus
(
    unsigned int * totalFreeMemory,
    unsigned int * largestBlock,
    unsigned int * numberOfObjects,
    unsigned int * lowWaterMark
)
{
    unsigned int _totalFreeMemory = 0;
    unsigned int _largestBlock = 0;
    unsigned int _numberOfObjects = 0;
    
    ExternalMemoryObject * pEmo = m_pEmo_First;

    while( pEmo != NULL )
    {
        if ( pEmo->m_ReferenceCount == 0 )
        {
            (_totalFreeMemory) += pEmo->m_Size;
            if ( pEmo->m_Size > (_largestBlock) )
            {
                (_largestBlock) = pEmo->m_Size;
            }
        }
        else
        {
            (_numberOfObjects)++;
        }

        pEmo = pEmo->m_pEmo_Next;
    }

    if ( totalFreeMemory != NULL )
    {
        *totalFreeMemory = _totalFreeMemory;
    }

    if ( largestBlock != NULL )
    {
        *largestBlock = _largestBlock;
    }

    if ( numberOfObjects != NULL )
    {
        *numberOfObjects = _numberOfObjects;
    }

    if ( lowWaterMark != NULL )
    {
        *lowWaterMark = 0;
    }
}

void ExternalMemoryHeap::Map( void )
{
    ExternalMemoryObject * pSearch = m_pEmo_First;

    rDebugPrintf( "External Dump:\n" );
    rDebugPrintf( "    Size: Space:[%d]Size:[0x%x]B[%d]B[%.2f]K[%.2f]Meg\n",
            m_RadMemorySpace,
            m_HeapSize,
            m_HeapSize,
            (float) m_HeapSize / 1024.0f,
            (float) m_HeapSize / ( 1024.0f * 1024.0f ) );
    
    while( pSearch )
    {
        rDebugPrintf( "    [%s][%20s]Addr:[0x%6x]Size:[0x%6x]B[%6d]B[%6.2f]K[%6.2f]M\n",
            pSearch->m_ReferenceCount == 0 ? "-" : "*",
            pSearch->GetName( ),
            pSearch->m_Address, pSearch->m_Size,
            pSearch->m_Size,
            (float) pSearch->m_Size / 1024.0f,
            (float) pSearch->m_Size / ( 1024.0f * 1024.0f ) );
        pSearch = pSearch->m_pEmo_Next;
    }
}

//============================================================================
// ExternalMemoryHeap::Initialize
//============================================================================

void ExternalMemoryHeap::Initialize
(
    void * pStartOfExternalMemory,
    unsigned int sizeOfExternalMemory,
    radMemorySpace memorySpace,
    unsigned int maxAllocations, // size of allocation pool
    unsigned int growAllocationsBy, // grow allocation pool by
    radMemoryAllocator allocSlot,
    const char * pName
)
{


    m_HeapSize = sizeOfExternalMemory;
    m_RadMemorySpace = memorySpace;

    ::radMemoryCreatePool( & refIRadMemoryPool_Emo,
        sizeof( ExternalMemoryObject ),
        maxAllocations + 1, // Extra one for big free block
        growAllocationsBy,
        false,
        None,
        allocSlot );

    //
    // Create one big free block
    //

    ExternalMemoryObject * pEmo_FreeMem = ExternalMemoryObject::Create( refIRadMemoryPool_Emo );

    rAssert( pEmo_FreeMem != NULL );
        
    pEmo_FreeMem->Initialize(
        this,
        0,
        reinterpret_cast< uintptr_t >( pStartOfExternalMemory ),
        sizeOfExternalMemory,
        NULL,
        NULL
    );

    rAssert( pEmo_FreeMem != NULL );
 
    //
    // Set up our doubly linked list
    //

    m_pEmo_First = pEmo_FreeMem;
    m_pEmo_Last = pEmo_FreeMem;

    radMemoryMonitorDeclareSection(
        (void*) m_pEmo_First->m_Address,
        m_pEmo_First->m_Size,
        IRadMemoryMonitor::MemorySectionType_DynamicData,
        memorySpace,
        NULL );

#ifdef RADMEMORYMONITOR
    {
    char szFullName[ 128 ];
    sprintf( szFullName, "%s[ExternalMemoryHeap]", pName );
    radMemoryMonitorIdentifySection( (void*) m_pEmo_First->m_Address, szFullName, memorySpace );
    }
#endif // RADMEMORYMONITOR
}

//============================================================================
// ExternalMemoryHeap::bIExternalMemoryHeap
//============================================================================

ExternalMemoryHeap::ExternalMemoryHeap
(
    void
)
    :
    m_ReferenceCount( 1 ),
    m_HeapSize( 0 ),
    m_pEmo_First( NULL ),
    m_pEmo_Last( NULL )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "ExternalMemoryHeap" );
}

//============================================================================
// ExternalMemoryHeap::~ExternalMemoryHeap
//============================================================================

/* virtual */ ExternalMemoryHeap::~ExternalMemoryHeap
(
    void
)
{
    //
    // If there is more than one block, or the one block isn't free
    // we !
    //

    rAssert( m_pEmo_First->m_pEmo_Next == NULL );
    rAssert( m_pEmo_First->m_pEmo_Previous == NULL );
    rAssert( m_pEmo_First->m_ReferenceCount == 0 );

    radMemoryMonitorRescindSection(
        (void*) m_pEmo_First->m_Address,
        m_RadMemorySpace );

    ExternalMemoryObject::Destroy( refIRadMemoryPool_Emo, m_pEmo_First );
}

//============================================================================
// ExternalMemoryHeap::Allocate
//============================================================================
ExternalMemoryObject * ExternalMemoryHeap::Allocate
(
    unsigned int size,
    unsigned int alignment
)
{
    rAssert( radMemoryGetAllocationName( ) );

    ExternalMemoryObject * pEmo_New = NULL;

    if ( size > 0 )
    {
        pEmo_New = AllocateFirstFit( size, alignment );

        if ( pEmo_New == NULL )
        {
            unsigned int totalFreeMemory;
            unsigned int largestBlock;
            unsigned int numberOfObjects;
            unsigned int lowWaterMark;

            GetStatus( & totalFreeMemory, & largestBlock, & numberOfObjects, & lowWaterMark );

            rDebugPrintf( "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );

            rDebugPrintf( "Out of memory or external nodes allocating: [%s][0x%x]B[%d]B[%.2f]K[%.2f]M\n",
                radMemoryGetAllocationName( ),
                size,
                size,
                (float) size / 1024.0f,
                (float) size / ( 1024.0f * 1024.0f ) );

            rDebugPrintf( "    totalFreeMemory:         [0x%x]B[%d]B[%.2f]K[%.2f]M\n",
                totalFreeMemory,
                totalFreeMemory,
                (float) totalFreeMemory / 1024.0f,
                (float) totalFreeMemory / ( 1024.0f * 1024.0f ) );

            rDebugPrintf( "    largestBlock:      [0x%x]B[%d]B[%.2f]K[%.2f]M\n",
                largestBlock,
                largestBlock,
                (float) largestBlock / 1024.0f,
                (float) largestBlock / ( 1024.0f * 1024.0f ) );

            rDebugPrintf( "    numberOfObjects (allocated): [%d]\n", numberOfObjects );

            unsigned int numberFree = 0;
            unsigned int numberAllocated = 0;

            refIRadMemoryPool_Emo->GetStatus( NULL, & numberFree, & numberAllocated );


            rDebugPrintf( "    max nodes (free and allocated): [%d]\n", numberFree + numberAllocated );
                
            rDebugPrintf( "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
            Map( );
            rDebugPrintf( "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
        }
        else
        {
            rAssert( ((uintptr_t) pEmo_New->GetMemoryAddress( ) % alignment ) == 0 );

            pEmo_New->SetName( radMemoryGetAllocationName( ) );

            radMemoryMonitorDeclareAllocation(
                (void*) pEmo_New->m_ClientAddress,
                pEmo_New->m_ClientSize,
                MM_MAX_CALLSTACK_DEPTH,
                m_RadMemorySpace );
        }
    }

    radMemorySetAllocationName( NULL );
        
    // Map( );
    
    return pEmo_New;
}

//============================================================================
// ExternalMemoryHeap::AllocateFirstFit
//============================================================================

ExternalMemoryObject * ExternalMemoryHeap::AllocateFirstFit
(
    unsigned int size,
    unsigned int alignment
)
{
    ExternalMemoryObject * pEmo_Ptr = m_pEmo_First;      

    //
    // Just loop through until we find a free block that fits
    //

    while ( pEmo_Ptr != NULL )
    {
        if ( pEmo_Ptr->m_ReferenceCount == 0 )
        {
            //
            // We have to adjust this block's size for aligning this memory block
            //

            unsigned int unalignmentval = CalculateUnalignment( pEmo_Ptr->m_Address, alignment );

            if( pEmo_Ptr->m_Size >= size + unalignmentval )
            {
                break;
            }
        }

        pEmo_Ptr = pEmo_Ptr->m_pEmo_Next;
    }

    if ( pEmo_Ptr != NULL )
    {
        pEmo_Ptr = SplitBlockBottom( pEmo_Ptr, size, alignment );
    }     

    return pEmo_Ptr;
}

//============================================================================
// ExternalMemoryHeap::AllocateClosestFit
//============================================================================

ExternalMemoryObject * ExternalMemoryHeap::AllocateClosestFit
(
    unsigned int size,
    unsigned int alignment
)
{
    ExternalMemoryObject * pEmo_Ptr = m_pEmo_First;
    ExternalMemoryObject * pEmo_ClosestFit = NULL;      

    //
    // Just loop through until we find a free block that fits
    //
    
    while ( pEmo_Ptr != NULL )
    {
        unsigned int unalignmentval = CalculateUnalignment( pEmo_Ptr->m_Address, alignment );

        if( ( pEmo_Ptr->m_ReferenceCount == 0 ) && ( pEmo_Ptr->m_Size >= ( size + unalignmentval ) ) )
        {
            if
            (
                ( ( pEmo_ClosestFit != NULL ) && ( pEmo_ClosestFit->m_Size > pEmo_Ptr->m_Size ) ) ||
                ( pEmo_ClosestFit == NULL )
            )
            {
                pEmo_ClosestFit = pEmo_Ptr;
            }           
        }

        pEmo_Ptr = pEmo_Ptr->m_pEmo_Next;
    }

    if ( pEmo_ClosestFit != NULL )
    {
        pEmo_Ptr = SplitBlockBottom( pEmo_ClosestFit, size, alignment );
    }     

    return pEmo_Ptr;
}

//============================================================================
// ExternalMemoryHeap::AllocateLastFit
//============================================================================

ExternalMemoryObject * ExternalMemoryHeap::AllocateLastFit
(
    unsigned int size,
    unsigned int alignment
)
{
    ExternalMemoryObject * pEmo_Ptr = m_pEmo_Last;      

    //
    // Just loop through until we find a free block that fits
    //

    while ( pEmo_Ptr != NULL )
    {
        unsigned int unalignmentval = CalculateUnalignment( pEmo_Ptr->m_Address, alignment );

        if ( ( pEmo_Ptr->m_ReferenceCount == 0 ) && ( pEmo_Ptr->m_Size >= ( size + unalignmentval ) ) )
        {
            break;
        }

        pEmo_Ptr = pEmo_Ptr->m_pEmo_Previous;
    }

    if ( pEmo_Ptr != NULL )
    {
        pEmo_Ptr = SplitBlockTop( pEmo_Ptr, size, alignment );
    }     

    return pEmo_Ptr;
}

//============================================================================
// ExternalMemoryHeap::SplitBlockBottom
//============================================================================

ExternalMemoryObject * ExternalMemoryHeap::SplitBlockBottom
(
    ExternalMemoryObject * pEmo_OldFreeBlock,
    unsigned int size,
    unsigned int alignment
)
{
    // How much is this block out of alignment ?

    unsigned int unalignmentval = CalculateUnalignment( pEmo_OldFreeBlock->m_Address, alignment );

    if ( pEmo_OldFreeBlock->m_Size == ( size + unalignmentval )  )
    {
        //
        // The size is exactly what we need so allocate the whole thing
        //
        pEmo_OldFreeBlock->m_ReferenceCount = 1;

        pEmo_OldFreeBlock->m_ClientSize = size;
        pEmo_OldFreeBlock->m_ClientAddress =
            pEmo_OldFreeBlock->m_Address + unalignmentval;
    }
    else
    {
        //
        // The free block is big enough so just allocate part of it
        //

        //
        // Allocate a new free block block, the old free block will
        // become the allocated block.
        //

        ExternalMemoryObject * pEmo_NewFreeBlock = ExternalMemoryObject::Create( refIRadMemoryPool_Emo );

        if ( pEmo_NewFreeBlock == NULL )
        {
            return NULL;
        }

        pEmo_NewFreeBlock->Initialize(
            this,
            0, // referenceCount
            pEmo_OldFreeBlock->m_Address + ( size + unalignmentval ),
            pEmo_OldFreeBlock->m_Size - ( size + unalignmentval ),
            pEmo_OldFreeBlock->m_pEmo_Next,
            pEmo_OldFreeBlock
        );

        rAssert( pEmo_NewFreeBlock->m_Size > 0 );

        if ( pEmo_NewFreeBlock != NULL )
        {

            if ( pEmo_OldFreeBlock->m_pEmo_Next != NULL )
            {
                //
                // We are at the end of the list so we must fix up our last pointer
                //
                pEmo_OldFreeBlock->m_pEmo_Next->m_pEmo_Previous = pEmo_NewFreeBlock;
            }
            pEmo_OldFreeBlock->m_pEmo_Next = pEmo_NewFreeBlock;
            pEmo_OldFreeBlock->m_ReferenceCount = 1;
            pEmo_OldFreeBlock->m_Size = ( size + unalignmentval );
            pEmo_OldFreeBlock->m_ClientSize = size;
            pEmo_OldFreeBlock->m_ClientAddress = pEmo_OldFreeBlock->m_Address + unalignmentval;

            if ( m_pEmo_Last == pEmo_OldFreeBlock )
            {
                m_pEmo_Last = pEmo_NewFreeBlock;
            }
        }
        else
        {
            return NULL;
        }
    }
    
    rAssert( pEmo_OldFreeBlock->GetMemoryAddress( ) != NULL );

    return pEmo_OldFreeBlock;
}

//============================================================================
// ExternalMemoryHeap::SplitBlockTop
//============================================================================

ExternalMemoryObject * ExternalMemoryHeap::SplitBlockTop
(
    ExternalMemoryObject * pEmo_OldFreeBlock,
    unsigned int size,
    unsigned int alignment
)
{

    unsigned int unalignmentval = CalculateUnalignment( pEmo_OldFreeBlock->m_Address, alignment );

    if ( pEmo_OldFreeBlock->m_Size == ( size + unalignmentval ) )
    {
        //
        // The size is exactly what we need so allocate the whole thing
        //
        pEmo_OldFreeBlock->m_ReferenceCount = 1;
        pEmo_OldFreeBlock->m_ClientSize = size;
        pEmo_OldFreeBlock->m_ClientAddress = pEmo_OldFreeBlock->m_Address + unalignmentval;
    }
    else
    {
        //
        // The free block is big enough so just allocate part of it
        //

        //
        // Allocate a new free block block, the old free block will
        // become the allocated block.
        //

        
        ExternalMemoryObject * pEmo_NewFreeBlock = ExternalMemoryObject::Create( refIRadMemoryPool_Emo );

        if ( pEmo_NewFreeBlock == NULL )
        {
            return NULL;
        }

        pEmo_NewFreeBlock->Initialize(
            this,
            0, // referenceCount
            pEmo_OldFreeBlock->m_Address,
            pEmo_OldFreeBlock->m_Size - ( size + unalignmentval ),
            pEmo_OldFreeBlock,
            pEmo_OldFreeBlock->m_pEmo_Previous
        );

        if ( pEmo_NewFreeBlock != NULL )
        {
            if ( pEmo_OldFreeBlock->m_pEmo_Previous != NULL )
            {
                pEmo_OldFreeBlock->m_pEmo_Previous->m_pEmo_Next = pEmo_NewFreeBlock;
            }

            pEmo_OldFreeBlock->m_Address = pEmo_NewFreeBlock->m_Address + pEmo_NewFreeBlock->m_Size;
            pEmo_OldFreeBlock->m_pEmo_Previous = pEmo_NewFreeBlock;
            pEmo_OldFreeBlock->m_ReferenceCount = 1;
            pEmo_OldFreeBlock->m_Size = size + unalignmentval;
            pEmo_OldFreeBlock->m_ClientSize = size;
            pEmo_OldFreeBlock->m_ClientAddress = pEmo_OldFreeBlock->m_Address + unalignmentval;

            if ( m_pEmo_First == pEmo_OldFreeBlock )
            {
                m_pEmo_First = pEmo_NewFreeBlock;
            }
        }
        else
        {
            return NULL;
        }
    }
    
    return pEmo_OldFreeBlock;
}


//============================================================================
// ExternalMemoryHeap::Free
//============================================================================

void ExternalMemoryHeap::Free
(
    ExternalMemoryObject * pEmo_Free
)
{
    rAssert( pEmo_Free != NULL );
    
    radMemoryMonitorRescindAllocation(
        (void*) pEmo_Free->m_ClientAddress,
        m_RadMemorySpace );

    pEmo_Free->SetName( "Free" );
    
    pEmo_Free->m_ReferenceCount = 0;
    pEmo_Free->m_ClientAddress = 0;
    pEmo_Free->m_ClientSize = 0;

    ExternalMemoryObject * pEmo_Ptr = pEmo_Free->m_pEmo_Next;

    if ( ( pEmo_Ptr != NULL ) && ( pEmo_Ptr->m_ReferenceCount == 0 ) )
    {
        pEmo_Free->m_Size = pEmo_Free->m_Size + pEmo_Ptr->m_Size;
        pEmo_Free->m_pEmo_Next = pEmo_Ptr->m_pEmo_Next;

        if ( pEmo_Ptr->m_pEmo_Next != NULL )
        {
            pEmo_Ptr->m_pEmo_Next->m_pEmo_Previous = pEmo_Free;
        }

        if ( pEmo_Ptr == m_pEmo_Last )
        {
            m_pEmo_Last = pEmo_Free;
        }

        ExternalMemoryObject::Destroy( refIRadMemoryPool_Emo, pEmo_Ptr );
    }

    pEmo_Ptr = pEmo_Free->m_pEmo_Previous;

    if ( (pEmo_Ptr != NULL ) && ( pEmo_Ptr->m_ReferenceCount == 0 ) )
    {
        pEmo_Free->m_Size = pEmo_Free->m_Size + pEmo_Ptr->m_Size;
        pEmo_Free->m_pEmo_Previous = pEmo_Ptr->m_pEmo_Previous;
        pEmo_Free->m_Address = pEmo_Ptr->m_Address;

        if ( pEmo_Ptr->m_pEmo_Previous != NULL )
        {
            pEmo_Ptr->m_pEmo_Previous->m_pEmo_Next = pEmo_Free;
        }
       
        if ( pEmo_Ptr == m_pEmo_First )
        {
            m_pEmo_First = pEmo_Free;
        }

        ExternalMemoryObject::Destroy( refIRadMemoryPool_Emo, pEmo_Ptr );
    }

    //
    // If freeing this block caused us to have one big free block and our refernce
    // count is zero, it is not possible for anyone to be holding a valid
    // memory object, so we must destroy ourslves.
    //

    if ( ( m_ReferenceCount == 0 ) && ( m_pEmo_First == m_pEmo_Last ) )
    {
        delete this;
    }
}

//============================================================================
// ExternalMemoryHeap::GetSize
//============================================================================

/* virtual */ unsigned int ExternalMemoryHeap::GetSize( void )
{
    return m_HeapSize;
}

//============================================================================
// ExternalMemoryHeap::ValidateHeap
//============================================================================

/* virtual */ bool ExternalMemoryHeap::ValidateHeap( void )
{
    return true;
}
