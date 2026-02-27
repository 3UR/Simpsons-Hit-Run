//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================



#include "pch.hpp"
#include <raddebug.hpp>
#include "platalloc.hpp"

#if defined WIN32 || defined RAD_UWP
    
    #include <stdlib.h>
    #include <malloc.h>
    
    #if defined MALLOC_DEBUG
        #include <crtdbg.h>
    #endif

#endif

//============================================================================
// ::radMemoryPlatInitialize
//============================================================================
void radMemoryPlatInitialize( void )
{

}

//============================================================================
// ::radMemoryPlatTerminate
//============================================================================

void radMemoryPlatTerminate( void )
{
}

//============================================================================
// ::radMemoryPlatAlloc
//============================================================================

void * radMemoryPlatAlloc( unsigned int numberOfBytes )
{
    void * pMemory;
    //
    // C++ standard says you can allocate 0 byte memory object.
    //
    if ( numberOfBytes == 0 )
    {
        numberOfBytes = 1;
    }

    pMemory = malloc( numberOfBytes );
    
    rWarningMsg( pMemory != NULL, "radMemory: Platform (malloc) allocator failed to allocate memory\n" );
    return pMemory;
}

//============================================================================
// ::radMemoryPlatFree
//============================================================================

void radMemoryPlatFree( void * pMemory )
{
    free( pMemory );
}

//============================================================================
// ::radMemoryPlatAllocAligned
//============================================================================

void * radMemoryPlatAllocAligned( unsigned int numberOfBytes, unsigned int alignment )
{
	#ifndef WIN32

		return ::aligned_alloc( alignment, numberOfBytes );

	#else

        return _aligned_malloc( numberOfBytes, alignment );

	#endif
}

//============================================================================
// ::radMemoryPlatFreeAligned
//============================================================================

void radMemoryPlatFreeAligned( void * pAlignedMemory )
{

	#ifndef WIN32
		
		free( pAlignedMemory );
	
	#else

        _aligned_free( pAlignedMemory );

	#endif
}
