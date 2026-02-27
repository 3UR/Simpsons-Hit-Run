#ifndef STATICHEAP_H_
#define STATICHEAP_H_
//-----------------------------------------------------------------------------
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// staticheap.h
//
// Description: a low overhead heap that cannot be freed from
//
// Modification History:
//
//  + Created Mar 21, 2003 Ian Gipson
//-----------------------------------------------------------------------------

#include <radmemory.hpp>

//-----------------------------------------------------------------------------
// Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// StaticHeap
//-----------------------------------------------------------------------------
class StaticHeap:
	public IRadMemoryHeap,
    public radRefCount
{
public:
    void   AddRef();
    void   AllowFreeing( const bool freeingAllowed );   //ONLY use this function when forcing some type of system shutdown and you don't want to see the asserts
    bool   Contains( const void* pMemory );
    bool   CanFreeMemory( void* pMemory );
    bool   CanFreeMemoryAligned( void* pMemory );
    void   CreateHeap( size_t size );
    void   FreeMemory( void* pMemory );
    void   FreeMemoryAligned( void* pMemory );
    size_t FreeMemorySize() const;
    void*  GetMemory ( unsigned int size );
    void*  GetMemoryAligned( unsigned int size, unsigned int align );
    void   GetMemoryObject( IRadMemoryObject** ppMemoryObject, unsigned int size );
	void   GetMemoryObjectAligned( IRadMemoryObject ** ppIRadMemoryObject, unsigned int size, unsigned int alignment );
    unsigned int GetSize();
    void GetStatus(
		unsigned int* totalFreeMemory,
		unsigned int* largestBlock,
		unsigned int* numberOfObjects,
		unsigned int* highWaterMark );

    void   Release();
    bool   ValidateHeap();
protected:
    static size_t Align( const size_t size, const size_t alignment );
    static char*  Align( const char*  ptr , const size_t alignment );
    void   PrintOutOfMemoryMessage( size_t requested );
    char*  m_BasePointer;
    char*  m_CurrentPointer;
    char*  m_End;
    size_t m_TotalSize;
    size_t m_Overflow;
    int    m_TotalAllocations;
    bool   m_FreeingAllowed;
};

IRadMemoryHeap* radMemoryCreateStaticHeap( unsigned int size,
	radMemoryAllocator allocator,
    const char * pName );

#endif // STATICHEAP_H_

