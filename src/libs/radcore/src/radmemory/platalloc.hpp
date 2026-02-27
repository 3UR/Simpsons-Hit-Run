//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef PLATALLOC_HPP
#define PLATALLOC_HPP

void   radMemoryPlatInitialize( void );
void   radMemoryPlatTerminate( void );

void * radMemoryPlatAlloc( unsigned int size );
void * radMemoryPlatAllocAligned( unsigned int size, unsigned int alignment );
void   radMemoryPlatFree( void * pMemory );
void   radMemoryPlatFreeAligned( void * pMemory );

#endif // PLATALLOC_HPP