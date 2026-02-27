//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radmemory.hpp
//
// Subsystem:	Foundation Technologies - Memory Services
//
// Description:	This file contains all definitions and interfaces required to
//              interact with the Foundation Technologies Memory Services.
//
//              The services provided by this subsystem include memory management
//              and allocation services.
//
// Revisions:   Mar 4, 2001         Creation
//
//=============================================================================

#ifndef	RADMEMORY_HPP
#define RADMEMORY_HPP

//=============================================================================
// Build Configuration Check
//=============================================================================

#if !defined(RAD_UWP) && !defined(RAD_WIN32)
    #error 'FTech requires definition of RAD_UWP, or RAD_WIN32'
#endif

//=============================================================================
// Include Files
//=============================================================================

#include <radobject.hpp>

//=============================================================================
// Defintions
//=============================================================================

// This typedef is used for specifiy the type that represents an allocator
// array slot.  Users can define their own allocation slots. The defualt
// behavior for all allocator slots is to use the system's built in malloc
// and free.

typedef int radMemoryAllocator;

#define RADMEMORY_ALLOC_DEFAULT         ((radMemoryAllocator)0)
#define RADMEMORY_ALLOC_TEMP            ((radMemoryAllocator)1)
#define RADMEMORY_ALLOC_PERSISTENT      ((radMemoryAllocator)3)

#define ALLOCATOR_TABLE_SIZE 30
#define ALLOCATOR_SEARCH ALLOCATOR_TABLE_SIZE

//=============================================================================
// Forward Declarations
//=============================================================================

struct IRadMemoryHeap;
struct IRadMemoryPool;
struct IRadMemoryObject;
struct IRadMemoryAllocator;

//=============================================================================
// Platform Alignment and Caching Macros
//=============================================================================

#if defined (RAD_WIN32) || defined (RAD_UWP)
#define STANDARD_ALIGNMENT  4
#define UNCACHE_BIT         0x00000000
#endif

//=============================================================================
// L O C A L  M E M O R Y  F U N C T I O N S
//=============================================================================

//-----------------------------------------------------------------------------
// Initialization Functions
//
// Because the C runtime calls global constructors, which often perform dynamic
// memory allocation, BEFORE MAIN, you must initialize the memory system in
// the FIRST new().
//
// Your game's new()'s MUST look something like the following for FTech and
// Pure3D to function correctly:
//
// bool g_MemoryInitialized = false;
//
// void * operator new ( size_t size )
// {
//     if ( g_MemoryInitialized == false)
//     {
//          // game should initialize their allocators and install them
//          // into ftech here before the fist allocation (below).
//
//          radMemoryInitialize( );
//
//          gameInstalAllocators( ); // The game's memory init function.
//
//          g_MemoryInitialized = true;
//     }
//     
//     return radMemoryAlloc( radMemoryGetCurrentAllocator( ), size );
//  }
//-----------------------------------------------------------------------------


void radMemoryInitialize( void );
void radMemoryTerminate( void );

//-----------------------------------------------------------------------------
// Allocator management functions
//
// The FTech memory heap components support the IRadMemoryAllocator interface,
// or you can write your on IRadMemoryAllocator derived classes to implement
// your own memory allocation algorithms.
//
// Allocators are registered into an allocator array.  The FTech libraries
// define the semantics of the first two array elements: RADMEMORY_ALLOC_DEFAULT
// (0), and RADMEMORY_ALLOC_TEMP (1).  The FoundationTech libraries use only
// RADMEMORY_ALLOC_TEMP internally.  RADMEMORY_ALLOC_DEFAULT is used only as
// default parameters to creation functions.  You can define your own constants
// for the remaining array elements.
//
// You must specify the PARENT allocator when registering an allocator into
// the array.  This allows the ftech libraries to create an allocator tree
// structure which will speed up the radMemoryFree() that searches for the
// allocator that allocated the memory.
//-----------------------------------------------------------------------------
void SetMemoryIdentification( const char* id );
void radMemoryRegisterAllocator( radMemoryAllocator allocator,
    radMemoryAllocator parent, IRadMemoryAllocator* pIRadMemoryAllocator );

void radMemoryUnregisterAllocator( radMemoryAllocator allocator );

IRadMemoryAllocator * radMemoryGetAllocator( radMemoryAllocator allocator );
radMemoryAllocator radMemoryGetAllocatorID( IRadMemoryAllocator* allocator );
IRadMemoryAllocator * radMemoryFindAllocator( void * pMemory );

// These functions are used to maintain a global "current allocator" state
// for EACH THREAD.  It is currently used only by the Pure3d loading system
// but not the FTech libraries.  By default the current allocator is set to
// RADMEMORY_ALLOC_DEFAULT.

radMemoryAllocator radMemoryGetCurrentAllocator( void );

// This function sets the current allocator by storing it in the calling
// thread's local storage area.  The return value is the previous allocator--
// use the return value to restore the state after an allocation or
// set of allocation operations like so:
//
// radMemoryAllocator old = radMemorySetCurrentAllocator( new );
// ... allocate some stuff using new
// radMemorySetCurrentAllcoator( old );

radMemoryAllocator radMemorySetCurrentAllocator( radMemoryAllocator allocator );

// This provides a mechanism for overriding the default behaviour of the
// above two functions.  That is, if the callbacks are specified, they will be
// called instead of radMemorySetCurrentAllocator and radMemoryGetCurrentAllocator
// doing what they normally do.  Note that if the callback will then be responsible
// for maintaining state in a thread-local way, if that is desired.

class IRadMemorySetAllocatorCallback
{
public:
    virtual radMemoryAllocator GetCurrentAllocator () = 0;
    virtual radMemoryAllocator SetCurrentAllocator ( radMemoryAllocator allocator ) = 0;
};
void radMemorySetAllocatorCallback ( IRadMemorySetAllocatorCallback* callback );

// This provides a mechanism for being notified whenever any alloc or free takes place
//

class IRadMemoryActivityCallback
{
public:
    virtual void MemoryAllocated( radMemoryAllocator allocator, void* address, unsigned int size ) = 0;
    virtual void MemoryFreed( radMemoryAllocator allocator, void* address ) = 0;
};
void radMemorySetActivityCallback ( IRadMemoryActivityCallback* callback );

void radMemorySetUsableAllocators( radMemoryAllocator* allocs, unsigned int numAllocs );

//-----------------------------------------------------------------------------
// Allocate/Free Functions
//-----------------------------------------------------------------------------

// Unaligned allocation functions: the first parameter is the allocator array
// element to use to perform the allocation.  This array element is mapped to
// an IRadMemoryAllocator() derived object such as the ftech-provied Doug Lea
// heap implementation.

void* radMemoryAlloc( radMemoryAllocator allocator, unsigned int numberOfBytes );
void  radMemoryFree( radMemoryAllocator allocator, void* pMemory );

// This free() searches the allocator tree for the allocator that did the
// allocation, thus it is somewhat slower than the version above.

void  radMemoryFree( void* pMemory );

// These functions allocate aligned memory.  You MUST match
// radMemoryAllocAligned() with radMemoryFreeAligned( ).

void* radMemoryAllocAligned( radMemoryAllocator allocator,
    unsigned int numberOfBytes, unsigned int alignment );

void  radMemoryFreeAligned(  radMemoryAllocator allocator, void * pMemory );

void  radMemoryFreeAligned( void * pMemory );

//-----------------------------------------------------------------------------
// Out of memory hook
//-----------------------------------------------------------------------------

//
// This function allows you to install handlers for trapping out of
// memory errors globally
//

typedef void (radMemoryOutOfMemoryCallback)( void * pUserData, radMemoryAllocator allocator, const unsigned int size );
void radMemorySetOutOfMemoryCallback( radMemoryOutOfMemoryCallback * pCallback, void * pUserData );

//-----------------------------------------------------------------------------
// Helper Functions
//-----------------------------------------------------------------------------

//
// Helper functions for aligning sizes
//

size_t radMemoryRoundUp( size_t value, size_t alignment );
size_t radMemoryRoundDown( size_t value, size_t alignemnt );
bool radMemoryIsAligned( size_t value, size_t alignment );

//=============================================================================
// R E M O T E  M E M O R Y  F U N C T I O N S
//
// Remote memory functions are use to manged memory spaces outside of the
// memory space on which the calling code is being run.  Because the transfer
// to this memory is achieved by performing a hardware DMA operation, the
// copy functions are asynchronous.  Local memory is also supported for
// the the convenience of abstracting all memory spaces.
//=============================================================================

//
// Forward Declarations
//

struct IRadMemorySpaceCopyRequest;

//
// This enumeration is used to control memory-to-memory copy operations 
// involving memory that is not addressable by the CPU running the program,
// for example a program wanting to manipulate IOP memory. The enum
// values can be used to identify source and destination memory types.
//
enum radMemorySpace
{
    radMemorySpace_Null,
#if defined (RAD_WIN32) || defined (RAD_UWP)
    radMemorySpace_Main         // Win/UWP main memory.
#endif
};

//
// LocalMemory always refers to the memory space that is addressable by the 
// processor running the program.
//
#define radMemorySpace_Local radMemorySpace_Main

extern unsigned int radMemorySpace_OptimalMultiple;
extern unsigned int radMemorySpace_OptimalAlignment;

//
// Copies memory from one space to another (or within one space) asynchronously
//

IRadMemorySpaceCopyRequest * radMemorySpaceCopyAsync(
	void * pDest, radMemorySpace spaceDest,
	const void * pSrc, radMemorySpace spaceSrc, unsigned int bytes );

//
// Allocates memory in specified memory space
//

void * radMemorySpaceAlloc( 
	radMemorySpace space, radMemoryAllocator allocator,	unsigned int numBytes );

//
// Allocates aligned memory in specified memory space
//

void * radMemorySpaceAllocAligned( 
	radMemorySpace space, radMemoryAllocator allocator,	
	unsigned int numBytes, unsigned int alignment );

IRadMemoryAllocator * radMemorySpaceGetAllocator( radMemorySpace space, radMemoryAllocator allocator );

//
// Frees memory in specified memory space
//

void radMemorySpaceFree( radMemorySpace space, radMemoryAllocator allocator, void * pMemory );

//
// Frees aligned memory in specified memory space
//

void radMemorySpaceFreeAligned( radMemorySpace space, radMemoryAllocator allocator,	void * pMemory );

//
// This interface is returned by asynchronous memory operations.
// Clients use it to poll the state of their requested operation.
//

struct IRadMemorySpaceCopyRequest : public IRefCount
{
	virtual bool IsDone( void ) = 0;
};

//=============================================================================
// I N T E R F A C E S
//=============================================================================

//
// This interface is used for the memory allocation system. Implement this 
// interface if your are going to serve up memory.  Objects that support this
// interface can "plug" into the memory management system.
//

struct IRadMemoryAllocator : public IRefCount
{
    virtual void* GetMemory( unsigned int size ) = 0;
	virtual void  FreeMemory( void* pMemory ) = 0;
    virtual bool  CanFreeMemory( void * pMemory ) = 0;

    virtual void* GetMemoryAligned( unsigned int size, unsigned int alignment ) = 0;
	virtual void  FreeMemoryAligned( void * pMemory ) = 0;
    virtual bool  CanFreeMemoryAligned( void * pMemory ) = 0;

	//
	// Memory statistics
	//

    virtual void GetStatus(
		unsigned int * totalFreeMemory,
		unsigned int * largestBlock,
		unsigned int * numberOfObjects,
		unsigned int * highWaterMark );

    virtual unsigned int GetSize( void );
};


//
// This is the interface definition used to interace with a heap object. A heap
// object supports allocations of variabled sized objects.
//

struct IRadMemoryHeap : public IRadMemoryAllocator
{
	//
	// Allocatos a "memory object".  This is a container for a address and a
	// size.  When it is Release()d it frees the memory.
	//

    virtual void GetMemoryObject(
		IRadMemoryObject ** pIRadMemoryObject,
		unsigned int bytes ) = 0;
	
	virtual void GetMemoryObjectAligned(
		IRadMemoryObject ** pIRadMemoryObject,
		unsigned int bytes, unsigned int alignment ) = 0;

    virtual void AllowFreeing( const bool freeingAllowed )
    {
        rAssertMsg( false, "function not implemented for this class" );
    };

	//
	// Performs a validty check on the heap contents, free-lists, etc.
	//

	virtual bool ValidateHeap( void ) = 0;
};

//============================================================================
// Interface: IRadMemoryBinAllocator
//
// Description: The component that supports this interface provides a very
//      specialized, user-customizable heap.  The heap uses fix-sized bins
//      with NO reuse of fragments.  The user can "tune" the bin sizes to
//      work best with the types of allocations used in the application.
//
// You can define the following preprocessor directive in radoptions.hpp:
//
//      #define RADMEMORY_BIN_ALLOCATOR_STATS
//          Keep high water mark and current free statistics - these
//          calculations are extremely lightweight but do add a few cycles
//          to each allocation.
//
//      #define RADMEMORY_BIN_ALLOCATOR_DEBUG
//          Do runtime debug checks.
//
//      #define RADMEMORY_BIN_ALLOCATOR_DEBUG_PAD
//          Pad pre & post, check fill.
//
//      #define RADMEMORY_BIN_ALLOCATOR_WARN_BYTE_WASTE 100 
//          Warn if wasting more that x bytes in a single allocation
//
//      #define RADMEMORY_BIN_ALLOCATOR_WARN_PERCENTAGE_WASTE 20.0f
//           Warn if wasting x % in a single allocation
//============================================================================

struct IRadMemoryBinAllocator : public IRadMemoryAllocator
{
    struct Bin
    {
        unsigned int m_ElementSizeInBytes;
        unsigned int m_NumberOfElements;
    };

    struct BinStatus : public Bin
    {
        unsigned int m_FreeElements;       // Number of elements currently free.
        unsigned int m_HighWaterMark;      // Most elements simultaneously allocated in this bin.
    };

    // Use these functions to iterate through each bin, getting its status.

    virtual unsigned int GetNumberOfBins( void ) = 0;
    virtual bool GetBinStatus(    // Returns false if index >= numberOfBins
        unsigned int binIndex,                 
        IRadMemoryBinAllocator::BinStatus * pBin ) = 0;       
    
    // Perform a consistency check on the heap.

    virtual void Validate( void ) = 0;
};

//
// This is the interface definition used to interace with a pool object. A pool object
// supports allocations of fixed sized objects.
//
struct IRadMemoryPool : public IRadMemoryAllocator
{
    //
    // Use this member to get a memory object. Will return null if the 
    // request cannot be fullfilled. Use delete reference of the object to 
    // free the memory.
    //

    // This function checks the size of the allocation, which is useful for
    // debugging.  Also it is more consistent with a heap interface.

    virtual void* GetMemory( unsigned int size ) = 0;
    virtual void  FreeMemory( void* pMemory ) = 0;
    virtual bool  CanFreeMemory( void * pMemory ) = 0;

    virtual void* GetMemoryAligned( unsigned int size, unsigned int alignment ) = 0;
    virtual void  FreeMemoryAligned( void * pMemory ) = 0;
    virtual bool  CanFreeMemoryAligned( void * pMemory ) = 0;

    virtual void* GetMemory( void ) = 0;

    //
    // This member is used to determine the information about pool object. It
    // reports the element size, the number of free elements, the number of allocated
    // elements.
    //
    virtual void GetStatus( unsigned int* elementSize, unsigned int* numberFree,
                            unsigned int* numberAllocated ) = 0;
};

//
// IRadMemoryTwoWayAllocator:
//
// * Allocations occur one after the other starting from 
//   either the top or bottom.
// * No internal headers or lists are maintained
// * Free must be called.  It internally performs no operations
//   (except maintain a counter in debug mode)
// * Only allocates each location once until reset is called.
//   Reset allows locations to be allocated again.  In debug
//   mode, reset will verify that all previous allocations
//   have been freed.
// * Note: Because locations are allocated once, this allocator
//   will eventually fill up even if free is called.  
//
struct IRadMemoryTwoWayAllocator : public IRadMemoryAllocator
{
    enum Direction { TopDown, BottomUp };

    // SetDirection: - sets the direction of suceeding allocations
    // GetDirection: - gets the curretion direction of allocations
    //               - default direction is TopDown

    virtual void SetDirection( Direction direction ) = 0;
    virtual Direction GetDirection( void ) = 0;

    // Reset: - allows memory locations to be allocated again

    virtual void Reset( Direction direction ) = 0;
};

//
// This is the iterface used a represent a reference counted piece of memory. 
// Obtained from the memory heap. To free, simbly delete the reference.
//
struct IRadMemoryObject : public IRefCount
{
    //
    // This member is used to retrieve the actual address of memory that
    // is backed by this object. The address returned is just a pointer
    // not an interface. Do not attempt to delete the object using this
    // address. Not that this memory will not fail as no alloction is
    // performed.
    //    
    virtual void* GetMemoryAddress( void ) = 0;
    
    //
    // Use this member to get the size of the memory represented by 
    // this object.
    //
    virtual unsigned int GetMemorySize( void ) = 0;
};

//=============================================================================
// Functions
//=============================================================================
//
// This factory function creates a bin allocator. 
//

void radMemoryBinAllocatorCreate(
    IRadMemoryBinAllocator ** ppIRadMemoryBinAllocator, // "Returns" the Bin Allocator object
    IRadMemoryBinAllocator::Bin * pBinArray,            // Array of Bin Structures (see above)
    unsigned int sizeOfBinArray,                        // Number of Bin Structures
    radMemoryAllocator allocator,                       // Allocator to get memory to manage 
    const char * pName = NULL );

//
// Company standard proven "best algorithm", writen by Doug Lea himself (gcc).
//

IRadMemoryHeap * radMemoryCreateDougLeaHeap( unsigned int size,
	radMemoryAllocator allocator,
    const char * pName = NULL );

IRadMemoryHeap * radMemoryCreateDougLeaHeap( void *pMem, unsigned int size,
	radMemoryAllocator allocator,
    const char * pName = NULL );

IRadMemoryHeap * radMemoryCreateStaticHeap( unsigned int size,
	radMemoryAllocator allocator,
    const char * pName = NULL );

IRadMemoryHeap * radMemoryCreateTrackingHeap( unsigned int size,
	radMemoryAllocator allocator,
    const char * pName = NULL );


//
// This interface is used to construct a pool object. A pool object is a memory
// management scheme whereby objects of a fixed size can be allocated and freed.
// This scheme is can allocate and free objects extremey fast.
//
//
// This enumeration is used to control memory integrity checks.
//
enum radMemoryDebugLevel
{
    None,                           // No intergrity checking performed.
    Moderate,                       // Moderate number of checks performed.
    High                            // A high level of checking performed.
};

void radMemoryCreatePool( IRadMemoryPool** ppMemoryPool,
                          unsigned int elementSize, 
                          unsigned int numberOfElements,
						  unsigned int growBy,
                          bool isThreadSafe = false, 
                          radMemoryDebugLevel debugLevel = Moderate, 
   					      radMemoryAllocator alloc = RADMEMORY_ALLOC_DEFAULT,
                          const char * pName = NULL );

void radMemoryCreatePool( IRadMemoryPool** ppMemoryPool,
                          void* placement,
                          unsigned int elementSize, 
                          unsigned int numberOfElements,
						  unsigned int growBy,
                          bool isThreadSafe = false, 
                          radMemoryDebugLevel debugLevel = Moderate, 
   					      radMemoryAllocator alloc = RADMEMORY_ALLOC_DEFAULT,
                          const char * pName = NULL );

//
// This interface is used to create an external memory heap. This object is used to manage
// memory in a way such that none of the memory it manages is used by the heap itself.
//
void radMemoryExternalHeapCreate( IRadMemoryHeap** ppExternalMemoryHeap,
                                  void * pStartOfExternalMemory,
                                  unsigned int sizeOfExternalMemory,
                                  radMemorySpace memorySpace,
                                  unsigned int maxAllocations, // size of allocation pool
                                  unsigned int growAllocationsBy, // grow allocation pool by
           					      radMemoryAllocator allocator,
                                  const char * pName = NULL ); 

//
// Creation of two way allocator
//

void radMemoryTwoWayAllocatorCreate( IRadMemoryTwoWayAllocator** ppTwoWayAllocator, 
                                     unsigned int size,
                                     radMemoryAllocator allocator,
                                     const char * pName );
                              

//
// Temporary!
//

void radMemorySetAllocationName( const char * pName );
const char * radMemoryGetAllocationName( void );

#endif
