//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        memoryutilities.h
//
// Description: some funcitons for checking the amount of free memory, etc
//
// History:     2002/12/03 + Created -- Ian Gipson
//
//=============================================================================

//========================================
// System Includes
//========================================

#if defined(RAD_WIN32) || defined(RAD_UWP)
    #include <crtdbg.h>
    #include <windows.h>
#endif

//========================================
// Project Includes
//========================================
#include <radmemorymonitor.hpp>
#include <memory/createheap.h>
#include <memory/memoryutilities.h>

namespace Memory
{
//******************************************************************************
//
// Public Member Functions
//
//******************************************************************************

size_t AllocateLargestFreeBlock( IRadMemoryAllocator* allocator, void*& returnMemory )
{
    //
    // Find out the largest block of memory I can grab
    //
    size_t lo = 0;
    size_t hi = 1024*1024*256; //this is just less than 256mb
    int pivot;
    void* memory = NULL;
    do
    {
        pivot = ( hi + lo ) / 2;
        if( memory != NULL )
        {
            allocator->FreeMemory( memory );
            memory = NULL;
        }
        memory = allocator->GetMemory( pivot );
        if( memory != NULL )
        {
            lo = pivot;
        }
        else
        {
            memory = allocator->GetMemory( lo );
            hi = pivot;
        }
    } while( ( hi - lo ) > 1 );
    returnMemory = memory;
    return lo;
}

//==============================================================================
// InitializeMemoryUtilities
//==============================================================================
//
// Description: does any setup required for the memory utility system.
//
// Parameters:	None.
//
// Return:      total amount of free memory.
//
// Constraints: None
//
//==============================================================================
void InitializeMemoryUtilities()
{
    static bool alreadyCalled = false;
    if( alreadyCalled )
    {
        return;
    }
    alreadyCalled = true;
}

//==============================================================================
// GetFreeMemoryEntropy
//==============================================================================
//
// Description: attempt at getting a measurement of fragmentation
//
// Parameters:	None.
//
// Return:      fragmentation metric
//
// Constraints: None
//
//==============================================================================
float GetFreeMemoryEntropy( IRadMemoryAllocator* allocator )
{
    //
    // if it's a tracking heap, just quit
    //
    GameMemoryAllocator which = WhichAllocator( allocator );
    HeapType type = AllocatorType( which );
    if( type != HEAP_TYPE_DOUG_LEA )
    {
        return 0.0f;
    }

    unsigned int totalFreeMemory;
    allocator->GetStatus( &totalFreeMemory, NULL, NULL, NULL );

    //
    // Get the largest N blocks of memory
    //
    const int N = 10;
    size_t largestSize[ N ];
    void*  memory     [ N ];

    int i;
    for( i = 0; i < N; ++i )
    {
        void* pointer = NULL;
        unsigned int freeMemory;
        unsigned int largestBlock;
        allocator->GetStatus( &freeMemory, &largestBlock, NULL, NULL );

        largestSize[ i ] = AllocateLargestFreeBlock( allocator, pointer );
        memory[ i ] = pointer;
    }

    //
    // Compute the entropy
    //
    float totalEntropy = 0.0f;
    float remainingPercentage = 1.0f;
    for( i = 0; i < N; ++i )
    {
        float size = static_cast< float >( largestSize[ i ] );
        float percentage = size / static_cast< float >( totalFreeMemory );
        float entropy = -percentage * logf( percentage ) / logf( 2.0f );
        totalEntropy += entropy;
        remainingPercentage -= percentage;
    }

    //
    //
    //

    //
    // free the memory
    //
    for( i = 0; i < N; ++i )
    {   
        allocator->FreeMemory( memory[ i ] );
        memory[ i ] = NULL;
    }

    return totalEntropy;
}

//==============================================================================
// GetFreeMemoryProfile
//==============================================================================
//
// Description: Tries to fill up availiable memory and reports back on how much
//              it got
//
// Parameters:	None.
//
// Return:      total amount of free memory.
//
// Constraints: None
//
//==============================================================================
size_t GetFreeMemoryProfile()
{
    static int numberOfTimesCalled = 0;
    ++numberOfTimesCalled;

#if defined(RAD_WIN32) || defined(RAD_UWP)
    return 0;
#endif
    const int size = 256;
    void* pointers[ size ];
    size_t sizes[ size ];

    int index = 0;
    int i;
    for( i = 0; i < size; i++ )
    {
        pointers[ i ] = NULL;
        sizes[ i ] = 0;
    }

    int retrys = 0;
    do
    {
        //
        // Find out the largest block of memory I can grab
        //
        int lo = 0;
        int hi = 1024*1024*256; //this is just less than 256mb
        int pivot;
        void* memory = NULL;
        do
        {
            pivot = ( hi + lo ) / 2;
            if( memory != NULL )
            {
                free( memory );
                memory = NULL;
            }
            memory = malloc( pivot );
            if( memory != NULL )
            {
                lo = pivot;
            }
            else
            {
                memory = malloc( lo );
                hi = pivot;
            }
        } while( ( hi - lo ) > 1 );

        if( ( memory == NULL ) && ( retrys < 2 ) )
        {
            ++retrys;
        }
        else
        {
            sizes[ index ] = lo;
            pointers[ index ] = memory;
            memory = NULL;
            ++index;
        }
    } while( ( pointers[ index - 1 ] != NULL ) && ( index < size ) );

    //
    // Need to sum the memory
    //
    size_t total = 0;
    for( i = 0; i < size; i++ )
    {
        total += sizes[ i ];
    }

    //
    // Need to delete the memory
    //
    for( i = 0; i < size; i++ )
    {
        void* pointer = pointers[ i ];
        if( pointer != NULL )
        {
            free( pointer );
            pointers[ i ] = NULL;
        }
        else
        {
            break;
        }
    }

#ifndef FINAL
    //rReleasePrintf( "Free Memory Profile - %d\n", total );
#endif
    return total;
}

//==============================================================================
// GetLargestFreeBlock
//==============================================================================
//
// Description: Attempts via a binary search to determine the largest free 
//              block of main memory (uses malloc)
//
// Parameters:	None.
//
// Return:      total amount of free memory.
//
// Constraints: None
//
//==============================================================================
size_t GetLargestFreeBlock()
{
    #if defined(RAD_WIN32) || defined(RAD_UWP)
        return 0;
    #endif

    //
    // Find out the largest block of memory I can grab
    //
    size_t lo = 0;
    size_t hi = 1024*1024*256; //this is just less than 256mb
    int pivot;
    void* memory = NULL;
    do
    {
        pivot = ( hi + lo ) / 2;
        if( memory != NULL )
        {
            free( memory );
            memory = NULL;
        }
        memory = malloc( pivot );
        if( memory != NULL )
        {
            lo = pivot;
        }
        else
        {
            memory = malloc( lo );
            hi = pivot;
        }
    } while( ( hi - lo ) > 1 );
    free( memory );
    return hi;
}

//==============================================================================
// GetLargestFreeBlock
//==============================================================================
//
// Description: Attempts via a binary search to determine the largest free 
//              block of main memory (uses malloc)
//
// Parameters:	allocator - the allocator to test
//
// Return:      total amount of free memory.
//
// Constraints: None
//
//==============================================================================
size_t GetLargestFreeBlock( IRadMemoryAllocator* allocator )
{
    //
    // Can't call this on the persistent heap
    //
    IRadMemoryAllocator* persistent = GetAllocator( GMA_PERSISTENT );
    if( allocator == persistent )
    {
        unsigned int totalFreeMemory;
        unsigned int largestBlock;
        unsigned int numberOfObjects;
        unsigned int highWater;
        allocator->GetStatus( &totalFreeMemory, &largestBlock, &numberOfObjects, &highWater );
        return largestBlock;
    }

    //
    // Find out the largest block of memory I can grab
    //
    void* memory = NULL;
    size_t size = AllocateLargestFreeBlock( allocator, memory );
    allocator->FreeMemory( memory );
    return size;
}

//==============================================================================
// GetLargestNFreeBlocks
//==============================================================================
//
// Description: Attempts via a binary search to determine the largest free 
//              block of main memory (uses malloc)
//
// Parameters:	allocator - the allocator to test
//
// Return:      total amount of free memory.
//
// Constraints: None
//
//==============================================================================
void GetLargestNFreeBlocks( IRadMemoryAllocator* allocator, const int n, size_t blocks[] )
{
}

//==============================================================================
// GetMaxFreeMemory
//==============================================================================
//
// Description: Returns the maximum amount of memory that has ever been free
//              in the lifetime of the game
//
// Parameters:	None.
//
// Return:      total amount of free memory.
//
// Constraints: None
//
//==============================================================================
size_t GetMaxFreeMemory()
{
    rAssertMsg( false, "GetMaxFreeMemory - this doesnt work on any platform but the PS2\n" );
    return 0;
}

//=============================================================================
// GetTotalMemoryFree
//=============================================================================
//
// Description: Returns the total amount of free memory
//
// Parameters:	None.
//
// Return:      total amount of free memory.
//
// Constraints: None
//
//=============================================================================
size_t GetTotalMemoryFree()
{
    #if defined(RAD_WIN32) || defined(RAD_UWP)
        MEMORYSTATUSEX status;
        GlobalMemoryStatusEx (&status);
        return status.ullAvailPhys;
    #else
        #pragma error( "CMemoryTracker::GetTotalMemoryFree - What platform is this then?");
    #endif
    return 0;
}

//=============================================================================
// GetTotalMemoryFreeLowWaterMark
//=============================================================================
//
// Description: Returns the low water mark of free memory. Must be called at
//              the frequency at which you want the memory sampled
//
// Parameters:	None.
//
// Return:      total amount of free memory at the worst point in the game
//
// Constraints: None
//
//=============================================================================
size_t GetTotalMemoryFreeLowWaterMark()
{
    static size_t lowWaterMark = GetTotalMemoryFree();
    size_t currentFree = GetTotalMemoryFree();
    if( currentFree < lowWaterMark )
    {
        lowWaterMark = currentFree;
    }
    return lowWaterMark;
}

//=============================================================================
// GetTotalMemoryUnavailable
//=============================================================================
//
// Description: Returns the total amount of free memory
//
// Parameters:	None.
//
// Return:      total amount of free memory.
//
// Constraints: None
//
//=============================================================================
size_t GetTotalMemoryUnavailable()
{
    #if defined(RAD_WIN32) || defined(RAD_UWP)
        //IAN didn't bother writing this yet
        return 0;
    #else
        #pragma error( "CMemoryTracker::GetTotalMemoryFree - What platform is this then?");
    #endif
}

//==============================================================================
// GetTotalMemoryUsed
//==============================================================================
//
// Description: query the OS with regard to the amount of memory used
//
// Parameters:	None.
//
// Return:      total amount of memory used
//
// Constraints: None
//
//==============================================================================
size_t GetTotalMemoryUsed()
{
#if (defined(RAD_WIN32) || defined(RAD_UWP)) && defined(RAD_DEBUG)
        _CrtMemState state;
        _CrtMemCheckpoint( &state );
        int total = 0;
        int i;
        for( i = 0; i < _MAX_BLOCKS; i++ )
        {
            total += state.lSizes[ i ];
        }

        //double check against the MEMORYSTATUS numbers
        MEMORYSTATUSEX status;
        GlobalMemoryStatusEx (&status);
        size_t doubleCheck =  status.ullTotalPhys - status.ullAvailPhys;
        //double check these numbers - they should match on the XBOX!
        return total;
#else
        return 0;
#endif
};

//=============================================================================
// PrintMemoryStatsToTty
//=============================================================================
//
// Description: Prints the total free memory to the TTY
//
// Parameters:	None.
//
// Return:      None
//
// Constraints: None
//
//=============================================================================
void PrintMemoryStatsToTty()
{
    size_t totalFree = GetTotalMemoryFree();
    rReleasePrintf( "MEMSTATS\t%d\t\n", totalFree );
}

}   //namespace MEMORY
