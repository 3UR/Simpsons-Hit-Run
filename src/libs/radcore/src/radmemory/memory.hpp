//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        memory.hpp
//
// Subsystem:	Foundation Technologies - Memory Services
//
// Description:	This file contains all definitions and interfaces required to
//              implement the Foundation Technologies Memory Services.
//
//				For complete description of Manager, refer to the Foundation
//              Technologies technical reference manual.
//
// Revisions:   May 2, 2000     Creation     DEG
//
//=============================================================================

#ifndef	MEMORY_HPP
#define MEMORY_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radobject.hpp>
#include <radmemory.hpp>

//=============================================================================
// Defintions
//=============================================================================

#ifdef NOT_ENCODE
#else
#define MASK_FREE        0x80000000
#define MASK_ROUNDED     0x40000000
#define MASK_ALIGNMENT   0x20000000
#define MASK_SIZE        0x1FFFFFFF
#endif // NOT_ENCODE

#define FREE_VALUE    0xFEEDF00D
#define USED_VALUE    0x12345678

// Debug flags for bitmask
#define INITIALIZE_MEMORY     0x00000001
#define ALWAYS_VALIDATE_HEAP  0x00000002
#define ALWAYS_VALIDATE_PTRS  0x00000004

//=============================================================================
// Forward Class/Struct Declarations
//=============================================================================

class MemoryHeap;
class MemoryPool;

//=============================================================================
// Interface Declarations
//=============================================================================

//
// This is the interface used a represent a piece of memory. They are obtained
// from the memory heap. These memory objects are reference
// counted. To free the memory object, simply delete the reference.
//
class MemoryObject : public IRadMemoryObject
{
public:

	MemoryObject( unsigned int size, void* memoryAddress, IRadMemoryHeap* pMemoryHeap );

    //
    // This member is used to retrieve the actual address of memory that
    // is backed by this object. The address returned is just a pointer
    // not an interface. Do not attempt to delete the object using this
    // address. Not that this memory will not fail as no alloction is
    // performed.
    //    
    void* GetMemoryAddress( void );
    
    //
    // Use this member to get the size of the memory represented by 
    // this object.
    //
    unsigned int GetMemorySize( void );

    //
    // Invoke this member anytime another object is given a pointer to this object.
    // Simply updates the reference count.
    //
    void AddRef( void );

    //
    // This member should be used instead of delete. It will ensure correct management
    // of the objects lifetime.
    //
    void Release( void );   

  	void* operator new( size_t size, void* placement ) { return( placement ); };
	virtual ~MemoryObject();

private:

    // This data member contains a count of objects referencing this object.
        
	unsigned int    m_Size;
	void*		    m_MemoryAddress;
	IRadMemoryHeap* m_MemoryHeap;
    int	            m_ReferenceCount;
};

//
// This is the interface definition used to interace with a pool object. A pool object
// supports allocations of fixed sized objects.
//
class MemoryPool : public IRadMemoryPool
{
public:

	MemoryPool();

    //
    // Inhirtance from IRadMemoryAllocator
    //
    void* GetMemory( unsigned int size );
    void  FreeMemory( void* pMemory );
    bool  CanFreeMemory( void * pMemory );

    void* GetMemoryAligned( unsigned int size, unsigned int alignment );
    void  FreeMemoryAligned( void * pMemory );
    bool  CanFreeMemoryAligned( void * pMemory );

    void* GetMemory( void );

    //
    // This member is used to determine the information about pool object. It
    // reports the element size, the number of free elements, the number of allocated
    // elements.
    //
    void GetStatus( unsigned int* elementSize, unsigned int* numberFree,
                    unsigned int* numberAllocated );         
     
   
    //
    // Invoke this member anytime another object is given a pointer to this object.
    // Simply updates the reference count.
    //
    void AddRef( void );

    //
    // This member should be used instead of delete. It will ensure correct management
    // of the objects lifetime.
    //
    void Release( void );   

	struct MemList
	{
		MemList* m_NextFree;
	};

	virtual ~MemoryPool();

    void * operator new( size_t size, radMemoryAllocator allocator );
    void operator delete( void * pMemory );

	void Initialize( void*                      pPlacement,
					 unsigned int               elementSize, 
					 unsigned int               numberOfElements,
					 unsigned int               growSize,
					 bool                       isThreadSafe,
					 radMemoryDebugLevel        debugLevel,
					 radMemoryAllocator         allocator,
                     const char *               pName );

private:

	void Chain ( IRadMemoryPool * pIRadMemoryPool );

#ifdef RAD_DEBUG
	bool FillAndValidate( void* pMemory, unsigned int value, unsigned int size );

	void FillFreeBlock( MemList* curList );

	// This is equivalent to "heapwalk()" in other standard heap managers.

	bool Validate( void );

	// Determines if the given pointer is a valid allocated pointer, previously
	// allocated by Allocate().

	bool ValidPointer( void* pointer );
#endif //debug

	// Determines if a given pointer is valid within the range of this
	// heap.

    bool WithinRange( void* pointer );

	// Pointers to the start and finish blocks of the heap.  First never 
	// moves but last moves depending on the size of the last block.

    MemList* m_First;
    MemList* m_Last;

	// Pointers to the head and tail of the free memory block list.

	MemList* m_FreeHead;
	MemList* m_FreeTail;

	// Pointer to the "chained" pool.

    MemoryPool* m_Chain;

	unsigned int m_Allocator;
    int          m_ReferenceCount;
	unsigned int m_RequestedElementSize;
	unsigned int m_ActualElementSize;
	unsigned int m_FillSize;
	unsigned int m_NumberOfElements;
	unsigned int m_GrowBy;
	bool         m_IsThreadSafe;

	radMemoryDebugLevel m_DebugLevel;

	unsigned int  m_Size;
	void*         m_pPlacement;

    unsigned int m_NumberFree;
    unsigned int m_NumberAllocated;
};

// Validates a given memory block to have a known pattern.

bool MemoryCheck( void* block, unsigned int value, unsigned int size );

// Writes a known pattern into a given memory block.

void MemorySet( void* block, unsigned int value, unsigned int size );

// Takes any given value and rounds it up to the next multiple of 4
// in order to cause values to be aligned to 32-bits.

uintptr_t RoundUpAlignment( uintptr_t value, uintptr_t alignment );

#endif
