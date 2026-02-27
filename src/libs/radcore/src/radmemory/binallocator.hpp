//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//============================================================================
// File: binallocator.hpp
//
// Created April 10, 2002 - Tim Hinds
//============================================================================

//============================================================================
// Include Files 
//============================================================================

#include <radmemory.hpp>
#include <radoptions.hpp>

//============================================================================
// Macros
//============================================================================

//#define RADMEMORY_BIN_ALLOCATOR_DEBUG_PAD
// #define RADMEMORY_BIN_ALLOCATOR_DEBUG
//#define RADMEMORY_BIN_ALLOCATOR_WARN_BYTE_WASTE 100
//#define RADMEMORY_BIN_ALLOCATOR_WARN_PERCENTAGE_WASTE 2.0f

#define PRE_FREE_FILL    0xDEADFACE
#define POST_FREE_FILL   0xDEADFADE
#define BODY_FREE_FILL   0xDEADBEEF

#define PRE_ALLOC_FILL   0xA10CFACE
#define POST_ALLOC_FILL  0xA10CFADE
#define BODY_ALLOC_FILL 0xA10CA10C

#ifdef RADMEMORY_BIN_ALLOCATOR_DEBUG_PAD
    #define RADMEMORY_BIN_ALLOCATOR_DEBUG
    #define RADMEMORY_BIN_ALLOCATOR_PRE_PAD  ( STANDARD_ALIGNMENT )
    #define RADMEMORY_BIN_ALLOCATOR_POST_PAD ( STANDARD_ALIGNMENT )
#else
    #define RADMEMORY_BIN_ALLOCATOR_PRE_PAD  0
    #define RADMEMORY_BIN_ALLOCATOR_POST_PAD 0
#endif

#ifdef RADMEMORY_BIN_ALLOCATOR_DEBUG
    #define RADMEMORY_BIN_ALLOCATOR_STATS 
#endif

typedef unsigned short FREE_BLOCK_INDEX_TYPE;
#define NULL_INDEX ((FREE_BLOCK_INDEX_TYPE)-1)

//============================================================================
// Component: radMemoryBinAllocator
//
// Description:  This is the internal implementation of the Bin Allocator.
//      This class is not visible to clients of radmemory.  Clients
//      communicate with this object through the IRadMemoryBinAllocator
//      interface (see radmemory.hpp)
//============================================================================

struct radMemoryBinAllocator
	:
	public IRadMemoryBinAllocator,
	public radRefCount
{
	// IRefCount

	IMPLEMENT_REFCOUNTED( "radMemoryBinAllocator" );

	// IRadMemoryAllocator

	virtual void* GetMemory( unsigned int size );
	virtual void  FreeMemory( void* pMemory );
    virtual bool  CanFreeMemory( void * pMemory );

    virtual void* GetMemoryAligned( unsigned int size, unsigned int alignment );
	virtual void  FreeMemoryAligned( void * pMemory );
    virtual bool  CanFreeMemoryAligned( void * pMemory );

	// IRadMemoryBinAllocator

    virtual unsigned int GetNumberOfBins( void );
    virtual bool GetBinStatus( 
        unsigned int binIndex,                 
        IRadMemoryBinAllocator::BinStatus * pBin );     
    
    virtual void Validate( void );

    // Internal Implementation

    inline void * GetMemoryUnAligned( unsigned int size );

    struct InternalBin
    {
        struct FreeBlock
        {
            FREE_BLOCK_INDEX_TYPE m_NextFreeIndex;
            FREE_BLOCK_INDEX_TYPE m_PrevFreeIndex;      
        };

        unsigned int m_ElementSizeInBytes;
        unsigned int m_NumberOfElements;
        
        char * m_pMemoryStart;
        char * m_pMemoryEnd; // Exclusive

        FREE_BLOCK_INDEX_TYPE m_FreeHeadIndex;        

        // These members are only calcualted if RADMEMORY_BIN_ALLOCATOR_STATS
        // is defined.

        unsigned int m_NumberAllocated;
        unsigned int m_HighWaterMark;

        // Functions

        inline void SetUpFreeList( void );
        inline void Validate( void );

        inline void * GetMemory( void );
        inline void FreeMemory( void * pMemory );
        inline void FreeMemoryAligned( void * pMemory );

        inline void RemoveFromFreeList( void * pMemory );
        inline void AddToFreeList( void * pMemory );

        // Free


        inline void FillFreeBlock( void * pMemory ); // Does next two fills
        inline void FillFreeBody( void * pMemory );
        inline void FillFreePad( void * pMemory );
        
        inline void CheckFreeBlock( void * pMemory ); // Does next three checks
        inline void CheckFreePad( void * pMemory );
        inline void CheckFreeBlockPtrs( void * pMemory );
        inline void CheckFreeBlockFill( void * pMemory );

        inline void FillAllocBlock( void * pMemory );
        inline void FillAllocPad( void * pMemory );
        inline void FillAllocBody( void * pMemory );

        inline void CheckAllocBlock( void * pMemory );

        inline FreeBlock * IndexToPtr( FREE_BLOCK_INDEX_TYPE index );
        inline FREE_BLOCK_INDEX_TYPE PtrToIndex( void * pMemory );

        inline void * UnAlignMemory( void * pMemory );
        inline unsigned int GetPaddedElementSize( void );

        inline char * GetUnPaddedMemoryStart( void );
        inline char * GetUnPaddedMemoryEnd( void );
        
    };

    inline radMemoryBinAllocator( void );
    virtual ~radMemoryBinAllocator( void );

    void Initialize(
        IRadMemoryBinAllocator::Bin * pBinArray,            // Array of Bin Structures (see above)
        unsigned int sizeOfBinArray,                        // Number of Bin Structures
        radMemoryAllocator allocator,
        const char * pName );   
    
    static int QSortCompareFunction( const void * pElement1, const void * pElement2 );

    inline InternalBin * FindBin( unsigned int size );
    inline InternalBin * FindBin( void * pMemory );
 
    // Member variables.
    
    unsigned int m_NumberOfBins;
    InternalBin * m_pBinArray;
};

//============================================================================
// radMemoryBinAllocator::radMemoryBinAllocator
//============================================================================

inline radMemoryBinAllocator::radMemoryBinAllocator( void )
    :
    radRefCount( 0 ),
    m_NumberOfBins( 0 ),
    m_pBinArray( NULL )
{
}

//============================================================================
// radMemoryBinAllocator::InternalBin:GetPaddedElementSize
//
// Description: Returns the size of a memory block INCLUDING PADDING.
//============================================================================

inline unsigned int radMemoryBinAllocator::InternalBin::GetPaddedElementSize( void )
{
    return m_ElementSizeInBytes + RADMEMORY_BIN_ALLOCATOR_PRE_PAD + RADMEMORY_BIN_ALLOCATOR_POST_PAD;
}

//============================================================================
// radMemoryBinAllocator::InternalBin:GetUnPaddedMemoryStart
//
// Description: Returns the address of the first block (not including the
//      pre-pad;
//============================================================================

inline char* radMemoryBinAllocator::InternalBin::GetUnPaddedMemoryStart( void )
{
    return m_pMemoryStart + RADMEMORY_BIN_ALLOCATOR_PRE_PAD;
}

//============================================================================
// radMemoryBinAllocator::InternalBin:GetUnPaddedMemoryEnd
//
// Description: Returns the end of the last block (not including the
//      post pad).
//============================================================================

inline char * radMemoryBinAllocator::InternalBin::GetUnPaddedMemoryEnd( void )
{
    return m_pMemoryEnd - RADMEMORY_BIN_ALLOCATOR_POST_PAD;
}

//============================================================================
// radMemoryBinAllocator::InternalBin::FreeMemoryAligned
//
// Description: Move pointer to start of block, it was ajusted when the
//      memory was allocated.
//============================================================================

inline void * radMemoryBinAllocator::InternalBin::UnAlignMemory( void * pMemory )
{
    unsigned int offset = (( (char*)pMemory - GetUnPaddedMemoryStart( ) ) / GetPaddedElementSize( ) ) * GetPaddedElementSize( );

    char * pUlMemory = ( GetUnPaddedMemoryStart( ) + offset );

    return pUlMemory;
}

//============================================================================
// radMemoryBinAllocator::InternalBin::IndexToPtr
//
// Description: This function converts a bin's free block index to a ptr.
//      The free blocks are linked together using indexes instead of pointers
//      because the size can be implied by the bin size.  This allows us
//      to achieve 4 byte free blocks for WIN32 platforms.
//============================================================================

inline radMemoryBinAllocator::InternalBin::FreeBlock * radMemoryBinAllocator::InternalBin::IndexToPtr(
    FREE_BLOCK_INDEX_TYPE index )
{
    
    radMemoryBinAllocator::InternalBin::FreeBlock * pFreeBlock;
    
    if ( index == NULL_INDEX )
    {
        pFreeBlock = NULL;
    }
    else
    {
        pFreeBlock = (radMemoryBinAllocator::InternalBin::FreeBlock * )
            ( GetUnPaddedMemoryStart( ) + ( GetPaddedElementSize( ) * index ) );

        rAssert( (char*) pFreeBlock >= GetUnPaddedMemoryStart( ) && (char*) pFreeBlock < GetUnPaddedMemoryEnd( ) );
    }

    return pFreeBlock;
}

//============================================================================
// radMemoryBinAllocator::InternalBin::IndexToPtr
//
// Description: This function convers a bin's free block ptr to an index.
//      The free blocks are linked together using indexes instead of pointers
//      because the size can be implied by the bin size.  This allows us
//      to achieve 4 byte free blocks for WIN32 platforms.
//============================================================================

inline FREE_BLOCK_INDEX_TYPE radMemoryBinAllocator::InternalBin::PtrToIndex( void * pMemory )
{
    FREE_BLOCK_INDEX_TYPE index = ( (char*) pMemory - GetUnPaddedMemoryStart( ) ) / GetPaddedElementSize( );
    
    rAssert( ( GetUnPaddedMemoryStart( ) + ( GetPaddedElementSize( ) * index ) ) == pMemory );
    
    return index; 
}

//============================================================================
// radMemoryBinAllocator::FillFreeBody
//
// Description: Fill a block of memory with the free fill patern (skipping the
//      first sizeof(FreeBlock) prev/next indexes at the begginging
//============================================================================

inline void radMemoryBinAllocator::InternalBin::FillFreeBody( void * pMemory )
{
    #ifdef RADMEMORY_BIN_ALLOCATOR_DEBUG
        
        unsigned int numInts = m_ElementSizeInBytes / 4;
        
        for( unsigned int f = sizeof( FreeBlock ) / 4; f < numInts; f ++ )
        {
            ((unsigned int*) pMemory)[ f ] = BODY_FREE_FILL;
        }

    #endif // RADMEMORY_BIN_ALLOCATOR_DEBUG
}

//============================================================================
// radMemoryBinAllocator::radMemoryBinAllocator::InternalBin::FillFreePad
//
// Description: This function fills a free block's pad areas.
//============================================================================

inline void radMemoryBinAllocator::InternalBin::FillFreePad( void * pMemory )
{
    #ifdef RADMEMORY_BIN_ALLOCATOR_DEBUG_PAD

        unsigned int * pPrePad = (unsigned int*)( ((char*)pMemory) - RADMEMORY_BIN_ALLOCATOR_PRE_PAD);
        unsigned int * pPostPad = (unsigned int*)( ((char*)pMemory) + m_ElementSizeInBytes );

        for( unsigned int pr = 0; pr < ( RADMEMORY_BIN_ALLOCATOR_PRE_PAD / 4 ) ; pr ++ )
        {
            pPrePad[ pr ] = PRE_FREE_FILL;
        }
        
        for( unsigned int po = 0; po < ( RADMEMORY_BIN_ALLOCATOR_PRE_PAD / 4 ) ; po ++ )
        {
            pPostPad[ po ] = POST_FREE_FILL;
        }

    #endif // RADMEMORY_BIN_ALLOCATOR_DEBUG_PAD
}

//============================================================================
// radMemoryBinAllocator::FillFreeBlock
//
// Description: Fill a free block and padding areas.
//============================================================================

inline void radMemoryBinAllocator::InternalBin::FillFreeBlock( void * pMemory )
{
    FillFreeBody( pMemory );
    FillFreePad( pMemory );
}

//============================================================================
// radMemoryBinAllocator::CheckFreeBlockFill
//
// Description: Check that the free block fill has not been corrupted.
//============================================================================

inline void radMemoryBinAllocator::InternalBin::CheckFreeBlockFill( void * pMemory )
{
    #ifdef RADMEMORY_BIN_ALLOCATOR_DEBUG
       
        unsigned int numInts = m_ElementSizeInBytes / 4;
        
        for( unsigned int f = sizeof( FreeBlock ) / 4 ; f < numInts; f ++ )
        {
            rReleaseAssert( ((unsigned int*) pMemory)[ f ] == BODY_FREE_FILL );
        }

    #endif // RADMEMORY_BIN_ALLOCATOR_DEBUG
}

//============================================================================
// radMemoryBinAllocator::CheckFreeBlockPtrs
//
// Description: This function checks to make sure that a free-block's list
//      pointers have not be written over.  It checks that these numbers are
//      indeed inside the bin's memory.
//============================================================================

inline void radMemoryBinAllocator::InternalBin::CheckFreeBlockPtrs( void * pMemory )
{
    #ifdef RADMEMORY_BIN_ALLOCATOR_DEBUG

        FreeBlock * pFreeBlock = (FreeBlock*) pMemory;

        // IndexToPtr checks the range

        rAssert( pFreeBlock->m_NextFreeIndex == NULL_INDEX || IndexToPtr( pFreeBlock->m_NextFreeIndex ) );
        rAssert( pFreeBlock->m_PrevFreeIndex == NULL_INDEX || IndexToPtr( pFreeBlock->m_PrevFreeIndex ) );

    #endif
}

//============================================================================
// radMemoryBinAllocator::radMemoryBinAllocator::InternalBin::CheckFreePad
//
// Description: This function checks an free block's pad areas.
//============================================================================

inline void radMemoryBinAllocator::InternalBin::CheckFreePad( void * pMemory )
{
    #ifdef RADMEMORY_BIN_ALLOCATOR_DEBUG_PAD

        unsigned int * pPrePad = (unsigned int*)( ((char*)pMemory) - RADMEMORY_BIN_ALLOCATOR_PRE_PAD);
        unsigned int * pPostPad = (unsigned int*)( ((char*)pMemory) + m_ElementSizeInBytes );

        for( unsigned int pr = 0; pr < ( RADMEMORY_BIN_ALLOCATOR_PRE_PAD / 4 ) ; pr ++ )
        {
            rAssert( pPrePad[ pr ] == PRE_FREE_FILL );
        }
        
        for( unsigned int po = 0; po < ( RADMEMORY_BIN_ALLOCATOR_POST_PAD / 4 ) ; po ++ )
        {
            rAssert( pPostPad[ po ] == POST_FREE_FILL );
        }

    #endif // RADMEMORY_BIN_ALLOCATOR_DEBUG_PAD    
}

//============================================================================
// radMemoryBinAllocator::CheckFreeBlock
//
// Description: Check that a free block has not been corrupted
//============================================================================

inline void radMemoryBinAllocator::InternalBin::CheckFreeBlock( void * pMemory )
{
    CheckFreeBlockPtrs( pMemory );
    CheckFreeBlockFill( pMemory );
    CheckFreePad( pMemory );
}

//============================================================================
// radMemoryBinAllocator::FillAllocBody
//
// Description: Fill a block of memory with the allocated fill patern.
//      This function fills the entire block including the FreeBlock
//      next & prev indexes.
//============================================================================

inline void radMemoryBinAllocator::InternalBin::FillAllocBody( void * pMemory )
{
    #ifdef RADMEMORY_BIN_ALLOCATOR_DEBUG
        
        unsigned int numInts = m_ElementSizeInBytes / 4;
        
        for( unsigned int f = 0; f < numInts; f ++ )
        {
            ((unsigned int*) pMemory)[ f ] = BODY_ALLOC_FILL;
        }

    #endif // RADMEMORY_BIN_ALLOCATOR_DEBUG
}

//============================================================================
// radMemoryBinAllocator::radMemoryBinAllocator::InternalBin::FillAllocPad
//
// Description: This function fills a allocated block's pad areas.
//============================================================================

inline void radMemoryBinAllocator::InternalBin::FillAllocPad( void * pMemory )
{
    #ifdef RADMEMORY_BIN_ALLOCATOR_DEBUG_PAD

        unsigned int * pPrePad = (unsigned int*)( ((char*)pMemory) - RADMEMORY_BIN_ALLOCATOR_PRE_PAD);
        unsigned int * pPostPad = (unsigned int*)( ((char*)pMemory) + m_ElementSizeInBytes );

        for( unsigned int pr = 0; pr < ( RADMEMORY_BIN_ALLOCATOR_PRE_PAD / 4 ) ; pr ++ )
        {
            pPrePad[ pr ] = PRE_ALLOC_FILL;
        }
        
        for( unsigned int po = 0; po < ( RADMEMORY_BIN_ALLOCATOR_PRE_PAD / 4 ) ; po ++ )
        {
            pPostPad[ po ] = POST_ALLOC_FILL;
        }

    #endif // RADMEMORY_BIN_ALLOCATOR_DEBUG_PAD
}

//============================================================================
// radMemoryBinAllocator::radMemoryBinAllocator::InternalBin::FillAllocBlock
//
// Description: This function fills an allocated block
//============================================================================

inline void radMemoryBinAllocator::InternalBin::FillAllocBlock( void * pMemory )
{
    FillAllocBody( pMemory );
    FillAllocPad( pMemory );
}

//============================================================================
// radMemoryBinAllocator::radMemoryBinAllocator::InternalBin::CheckAllocPad
//
// Description: This function checks an allocated block's pad areas.
//============================================================================

inline void radMemoryBinAllocator::InternalBin::CheckAllocBlock( void * pMemory )
{
    #ifdef RADMEMORY_BIN_ALLOCATOR_DEBUG_PAD

        unsigned int * pPrePad = (unsigned int*)( ((char*)pMemory) - RADMEMORY_BIN_ALLOCATOR_PRE_PAD);
        unsigned int * pPostPad = (unsigned int*)( ((char*)pMemory) + m_ElementSizeInBytes );

        for( unsigned int pr = 0; pr < ( RADMEMORY_BIN_ALLOCATOR_PRE_PAD / 4 ) ; pr ++ )
        {
            rAssert( pPrePad[ pr ] == PRE_ALLOC_FILL );
        }
        
        for( unsigned int po = 0; po < ( RADMEMORY_BIN_ALLOCATOR_POST_PAD / 4 ) ; po ++ )
        {
            rAssert( pPostPad[ po ] == POST_ALLOC_FILL );
        }

    #endif // RADMEMORY_BIN_ALLOCATOR_DEBUG_PAD    
}

//============================================================================
// radMemoryBinAllocator::AddToFreeList
//
// Description: This function adds a block of memory to a bin's free list.
//      A doubly linked list of free blocks is used.  The pointers are
//      indexes into the bin's element array.
//============================================================================

inline void radMemoryBinAllocator::InternalBin::AddToFreeList( void * pMemory )
{
    rAssert( pMemory != NULL );
    rAssert( pMemory >= m_pMemoryStart && pMemory < m_pMemoryEnd );

    unsigned int blockIndex = PtrToIndex( pMemory );
    
    CheckAllocBlock( pMemory );          
    FillFreeBlock( pMemory );

    FreeBlock * pFreeBlock = (FreeBlock*) pMemory;
    
    pFreeBlock->m_NextFreeIndex = m_FreeHeadIndex;
    pFreeBlock->m_PrevFreeIndex = NULL_INDEX;

    if( m_FreeHeadIndex != NULL_INDEX )
    {
        IndexToPtr( m_FreeHeadIndex )->m_PrevFreeIndex = blockIndex;
    }

    m_FreeHeadIndex = blockIndex;
}

//============================================================================
// radMemoryBinAllocator::RemoveFromFreeList
//
// Description: This function removes a block of memory from a bin's free
//      list.
//============================================================================

inline void radMemoryBinAllocator::InternalBin::RemoveFromFreeList( void * pMemory )
{
    rAssert( pMemory != NULL );

    CheckFreeBlock( pMemory );
    
    FreeBlock * pFreeBlock = (FreeBlock*) pMemory;

    if ( pFreeBlock->m_NextFreeIndex != NULL_INDEX )
    {
        IndexToPtr( pFreeBlock->m_NextFreeIndex )->m_PrevFreeIndex = pFreeBlock->m_PrevFreeIndex;
    }

    if ( pFreeBlock->m_PrevFreeIndex != NULL_INDEX )
    {
        IndexToPtr( pFreeBlock->m_PrevFreeIndex )->m_NextFreeIndex = pFreeBlock->m_NextFreeIndex;
    }
    else
    {
        m_FreeHeadIndex = pFreeBlock->m_NextFreeIndex;
    }

    FillAllocBlock( pMemory );
}

//============================================================================
// radMemoryBinAllocator::FindBin
//
// Description: This method finds the memory bin that allocated a piece of
//              memory.  The bin array is sorted, so a binary search is
//              used.
//============================================================================

inline radMemoryBinAllocator::InternalBin * radMemoryBinAllocator::FindBin( void * pMemory )
{
    rAssert( pMemory );

    unsigned int low = 0;
    unsigned int high  = m_NumberOfBins - 1;

    InternalBin * pBin = NULL;

    while ( 1 )
    {        
        unsigned int search = ( low + high ) / 2;

        rAssert( search < m_NumberOfBins );
    
        pBin = & ( m_pBinArray[ search ] );
        
        if ( pMemory < pBin->m_pMemoryStart )
        {
            high = search;
        }
        else if ( pMemory >= pBin->m_pMemoryEnd )
        {
            low = search;
        }
        else
        {
            break;
        }
        
        if ( low == high )
        {
            pBin = NULL;
            break;
        }
        else if ( search == low )
        {
            low++;
        }                                
    }

    return pBin;
}

//============================================================================
// radMemoryBinAllocator::FindBin
//
// Description: This method finds the best bin to allocate a certain size.
//      The bins are sorted so a binary search is used.
//============================================================================

inline radMemoryBinAllocator::InternalBin * radMemoryBinAllocator::FindBin(
    unsigned int size )
{
    rAssert( size > 0 );

    unsigned int low = 0;
    unsigned int high  = m_NumberOfBins - 1;

    InternalBin * pBin = NULL;

    while ( 1 )
    {        
        unsigned int search = ( low + high ) / 2;

        rAssert( search < m_NumberOfBins );
    
        pBin = & ( m_pBinArray[ search ] );

        if ( search < m_NumberOfBins - 1 )
        {
            InternalBin * pBin_Next = & ( m_pBinArray[ search + 1 ] );
        
            if ( size <= pBin_Next->m_ElementSizeInBytes )
            {
                low = search;
            }
            else if ( size > pBin->m_ElementSizeInBytes )
            {
                high = search;
            }
            else
            {
                // rDebugPrintf( "Found Bin: Size: [%d] for Size: [%d]\n", pBin->m_ElementSizeInBytes, size );

                rAssert( size <= pBin->m_ElementSizeInBytes );
                break;
            }
        
            if ( low == high )
            {
                pBin = NULL;
                break;
            }
            else if ( search == low )
            {
                low++;
            }
        }
        else
        {
            // Last bin
            
            if ( size <= pBin->m_ElementSizeInBytes )
            {
                break;
            }
            else
            {
                rAssert( false ); // No bin big enough
                pBin = NULL;
            }
        }                              
    }

    return pBin;
}

//============================================================================
// radMemoryBinAllocator::radMemoryBinAllocator::InternalBin::GetMemory
//
// Description: Allocate memory from this bin.
//============================================================================

void * radMemoryBinAllocator::InternalBin::GetMemory( void )
{
    if ( m_FreeHeadIndex != NULL_INDEX )
    {
        void * pMemory = IndexToPtr( m_FreeHeadIndex );

        RemoveFromFreeList( pMemory );

        #ifdef RADMEMORY_BIN_ALLOCATOR_STATS
                        
            m_NumberAllocated++;

            if ( m_NumberAllocated > m_HighWaterMark )
            {
                m_HighWaterMark = m_NumberAllocated;
            }

        #endif // RADMEMORY_BIN_ALLOCATOR_STATS

        return pMemory;
    }

    return NULL;
}

//============================================================================
// radMemoryBinAllocator::InternalBin::FreeMemory
//
// Description: Free unaligned memory from this bin
//============================================================================

void radMemoryBinAllocator::InternalBin::FreeMemory( void * pMemory )
{
    AddToFreeList( pMemory );

    #ifdef RADMEMORY_BIN_ALLOCATOR_STATS
                    
        m_NumberAllocated--;

    #endif // RADMEMORY_BIN_ALLOCATOR_STATS
}

//============================================================================
// radMemoryBinAllocator::InternalBin::FreeMemoryAligned
//
// Description: Free aligned memory from this bin
//============================================================================

void radMemoryBinAllocator::InternalBin::FreeMemoryAligned( void * pMemory )
{
    pMemory = UnAlignMemory( pMemory );

    AddToFreeList( pMemory );

    #ifdef RADMEMORY_BIN_ALLOCATOR_STATS
                    
        m_NumberAllocated--;

    #endif // RADMEMORY_BIN_ALLOCATOR_STATS
}

//============================================================================
// radMemoryBinAllocator::InternalBin::Validate
//
// Description: Validates that thee free list ad padding are intact
//============================================================================

inline void radMemoryBinAllocator::InternalBin::Validate( void )
{
    if ( m_FreeHeadIndex != NULL_INDEX )
    {
        FreeBlock * pFreeBlock = IndexToPtr( m_FreeHeadIndex );

        while( pFreeBlock )
        {
            CheckFreeBlock( pFreeBlock );

            pFreeBlock = IndexToPtr( pFreeBlock->m_NextFreeIndex );
        }
    }
}

//============================================================================
// radMemoryBinAllocator::InternalBin::SetUpFreeList
//
// Description: Add every block to the free list
//============================================================================

inline void radMemoryBinAllocator::InternalBin::SetUpFreeList( void )
{
    m_FreeHeadIndex = 0;

    FillFreeBlock( IndexToPtr( 0 ) );
    IndexToPtr( 0 )->m_PrevFreeIndex = NULL_INDEX;
    IndexToPtr( 0 )->m_NextFreeIndex = 1;


    for( FREE_BLOCK_INDEX_TYPE element = 1; element < m_NumberOfElements - 1; element ++ )
    {
        FreeBlock * pFb = IndexToPtr( element );

        FillFreeBlock( pFb );

        pFb->m_NextFreeIndex = element + 1;
        pFb->m_PrevFreeIndex = element - 1;  
    }

    FillFreeBlock( IndexToPtr( m_NumberOfElements - 1  ) );
    IndexToPtr( m_NumberOfElements - 1 )->m_PrevFreeIndex = m_NumberOfElements - 2;
    IndexToPtr( m_NumberOfElements - 1 )->m_NextFreeIndex = NULL_INDEX;

    rDebugPrintf( "Bin: Element Size: [0x%x] Num Elements: [0x%x] Start: [0x%x] End: [0x%x]\n",
        m_ElementSizeInBytes,
        m_NumberOfElements,
        m_pMemoryStart,
        m_pMemoryEnd );

        
}

//============================================================================
//  radMemoryBinAllocator::GetMemoryUnAligned
//
// Description: Internal function to get memory from a bin.
//============================================================================

inline void * radMemoryBinAllocator::GetMemoryUnAligned( unsigned int size )
{
    // We don't allocate zero size blocks with a valid pointer.

    void * pMemory = NULL;

    if ( size > 0 )
    {
        InternalBin * pIb = FindBin( size );

        // Dod we find a bin large enough for size, if not return NULL

        if ( pIb != NULL )
        {

            #ifdef RADMEMORY_BIN_ALLOCATOR_WARN_BYTE_WASTE

                unsigned int bwaste = pIb->m_ElementSizeInBytes - size;

                if ( bwaste > RADMEMORY_BIN_ALLOCATOR_WARN_BYTE_WASTE )
                {
                    rReleasePrintf( "radMemoryBinAllocator: Waste warning: size: [%d], closest: [%d], waste: [%d] bytes\n",
                        size, pIb->m_ElementSizeInBytes, bwaste );
                }
            #endif

            #ifdef RADMEMORY_BIN_ALLOCATOR_WARN_PERCENTAGE_WASTE
                
                float waste = (float) size / (float) (pIb->m_ElementSizeInBytes );
                waste = 1.0f - waste;

                if ( waste >= (RADMEMORY_BIN_ALLOCATOR_WARN_PERCENTAGE_WASTE / 100.0f ) )
                {   
                    rReleasePrintf( "radMemoryBinAllocator: Waste warning: size: [%d], closest: [%d], waste: [%.2f] percent\n",
                        size, pIb->m_ElementSizeInBytes, waste * 100.0f);
                }

            #endif

            pMemory = pIb->GetMemory( );

            if ( pMemory == NULL )
            {
                rDebugPrintf( "Out of memory in bin size: [%d] for allocation size: [%d]\n",
                    pIb->m_ElementSizeInBytes, size );
                rAssertMsg( false, "radMemoryBinAllocator: Out of memory in bin" );
            }
        }
        else
        {
            rDebugPrintf( "No bin big enough for [%d] size allocation\n", size );
            rAssertMsg( false, "radMemoryBinAllocator: No bin big enough for allocation" );
        }
    }
    else
    {
        pMemory = NULL;
    }

    return pMemory;

}
