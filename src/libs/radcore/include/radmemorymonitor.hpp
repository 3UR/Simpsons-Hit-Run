//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radmemorymonitor.hpp
//
// Subsystem:	Foundation Technologies - Memory Monitoring Component
//
// Description:	This file contains all definitions and interfaces required to
//              interact with the Foundation Technologies Memory Monitoring 
//				component. This system works in conjunction with the Host Memory
//				Monitor tool.
//
//              The memory monitor provides a view of memory. It should only 
//              be used for diagnostic and debugging purposes. To enable system,
//              the preprocess directive RADMEMORYMONITOR must be defined before 
//              this file is encountered.
//
// Revisions:   Feb 14, 2002
//
//=============================================================================

#ifndef	RADMEMORYMONITOR_HPP
#define RADMEMORYMONITOR_HPP

//=============================================================================
// Include Files
//=============================================================================
#include <radoptions.hpp>

#include <radobject.hpp>
#include <radmemory.hpp>

//=============================================================================
// External Defintions
//=============================================================================
//
// max depth of call stack
//
#define MM_MAX_CALLSTACK_DEPTH  24

//=============================================================================
// Forward Class/Struct Declarations
//=============================================================================

struct IRadMemoryMonitor;
struct IRadMemoryTraverse;

//=============================================================================
// Functions
//=============================================================================

//
// SyncMode - should system enforce send previous data before trying send next
//            data
// ReportRefCount - should system send reference count information, it will slow
//                  transmission down a lot. But if information is useful, then
//                  you need to enable it.
void radMemoryMonitorInitialize( unsigned int bufferSize, radMemoryAllocator alloc = RADMEMORY_ALLOC_DEFAULT, bool SyncMode = false, bool ReportRefCount = false );
void radMemoryMonitorTerminate( void );

//
// Use this function to obtain a reference to the memory monitor. This function
// should not be used directly by applications. They should use the inline functions
// instead as they will vanish in a release build.
//
IRadMemoryMonitor* radMemoryMonitorGet( void );

//
// call this function to service the memory monitor
//
void radMemoryMonitorService( void );

//=============================================================================
// Interfaces
//=============================================================================

//
// This is the interface definition to the memory monitor object. Again it
// is recommended that this interface definition not be used directly by applications.
// They should use the inline functions following this definition.
//
struct IRadMemoryMonitor : public IRefCount
{
    //
    // This routine must be invoked before any memory allocation is started, and
    // must be called right after radMemoryInitialize( ). Otherwise assert will happen.
    //
    virtual void Initialize( unsigned int bufferSize, bool SyncMode, bool ReportRefCount ) = 0;

    //
    // This routine should be invoked when the game as finished with the memory monitor.
    //
    virtual void Terminate( void ) = 0;

    //
    // The following enumeration is used to specify the type of memory section.
    //
    enum MemorySectionType
    {
        MemorySectionType_Code,                     // Code, 
        MemorySectionType_StaticData,               // Initialized and uninitialized data
        MemorySectionType_DynamicData,              // Heaps
        MemorySectionType_Stack                     // Stacks
    };

    //
    // These services are used to declare and rescind a section. Basically the game should
    // use the services any time a memory section is created or destroyed. For the allocation
    // caller must specify address and size of section. Additional information is the memory space,
    // memory type and name of the section. Caller can also provide (if dynamic memory heap) an
    // memory traverser which allows contection after the fact.
    //
    virtual void DeclareSection( void* address, unsigned int size, MemorySectionType sectionType, 
                                 radMemorySpace memorySpace, IRadMemoryTraverse* traverse ) = 0;

    virtual void RescindSection( void* address, radMemorySpace memorySpace ) = 0;

    virtual void IdentifySection( void* address, const char* name, radMemorySpace memorySpace ) = 0;
    //
    // These services are used to declare and rescind memory allcations. Basically, these functions
    // should be invoked anytime and allocation or free of memory occurs. Caller needs to specify
    // address, size of allocation, and depth of call statck to transfer. If more detailed memory
    // information is required, such as group( used for display purposes ), name, address of
    // reference count, IdentifyAllocation( ) should be called.
    //
    virtual void DeclareAllocation( void* address, unsigned int size, unsigned int callStackDepth,
                                    radMemorySpace memorySpace ) = 0;
       
    virtual void RescindAllocation( void* address, radMemorySpace memorySpace ) = 0;

    virtual void IdentifyAllocation( void* address, const char* group, const char* name,
                                     unsigned int* pReferenceCount, radMemorySpace memorySpace ) = 0;

    //
    // This member can be used by the game to flag memory states. These flags show up in the host
    // memory monitor tool and the user can "go to" these flags. Call just specifies a string to
    // idenify his flag.
    //
    virtual void IssueFlag( const char* flag ) = 0;
    
    //
    // The member allows the game to be suspended and the memory monitor put into a suspended state.
    // To resume the game, the user of the host memory monitor is required to intervene.
    //
    virtual void Suspend( void ) = 0;

    //
    // These two members can be used by reference counted objects to report which objects are holding
    // references to another object. Whenever an an addref or release is performed, the following two
    // methods should be invoked. Caller must provide the address of where the reference is stored and
    // the address of the referenced object.
    //
    virtual void ReportAddRef( void* pReference, void* pObject, radMemorySpace memorySpaceObject ) = 0;
    virtual void ReportRelease( void* pReference, void* pObject, radMemorySpace memorySpaceObject ) = 0;

    //
    // To track stack usage, stack should be filled initally with a specific character,
    // so memory monitor can identify the stack high water mark later.
    //
    virtual void SetStackInitialFillChar( char c ) = 0;
};

//
// The following interface is used by memory systems that can report their own 
// allocation state. By implementing and registering this interface when a memory section is
// defined, the memory monitor can obtain a picture of memory even if the memory monitor is 
// started after the game has run.
//
struct IRadMemoryTraverse
{
    //
    // This member will be invoked by the monitor to inform the memory system that it 
    // should ready itself for a new memory traversal.
    //
    virtual void Reset( void ) = 0; 

    //
    // This member is invoked by the monitor to get the next memory block defined in a 
    // heap. When heap is traversed, the implementer should return false to inidcate
    // no more blocks. Implementer can return the group and name if have maintains it.
    // Can return null if not.
    //
    virtual bool GetNext( void** address, unsigned int* size, unsigned int* group, char* name ) = 0;
};

//=============================================================================
// Inline Functions
//=============================================================================
inline void radMemoryMonitorDeclareSection( void* address, unsigned int size, IRadMemoryMonitor::MemorySectionType sectionType,
                                            radMemorySpace memorySpace = radMemorySpace_Local,
                                            IRadMemoryTraverse* traverse = NULL )
{
#if defined RADMEMORYMONITOR
    if ( radMemoryMonitorGet( ) != NULL )
    {
        radMemoryMonitorGet( )->DeclareSection( address, size, sectionType, memorySpace, traverse );
    }
#else
    (void) address; (void) size; (void) sectionType; (void) memorySpace; (void) traverse;
#endif
}

inline void radMemoryMonitorRescindSection( void* address, radMemorySpace memorySpace = radMemorySpace_Local )
{
#if defined RADMEMORYMONITOR
    if ( radMemoryMonitorGet( ) != NULL )
    {
        radMemoryMonitorGet( )->RescindSection( address, memorySpace );
    }
#else
    (void) address; (void) memorySpace;
#endif
}

inline void radMemoryMonitorIdentifySection( void* address, const char* name, radMemorySpace memorySpace = radMemorySpace_Local )
{
#if defined RADMEMORYMONITOR
    if ( radMemoryMonitorGet( ) != NULL )
    {
        radMemoryMonitorGet( )->IdentifySection( address, name, memorySpace );
    }
#else
    (void) address; (void) name; (void) memorySpace;
#endif
}

inline void radMemoryMonitorDeclareAllocation( void* address, unsigned int size, unsigned int callStackDepth = MM_MAX_CALLSTACK_DEPTH,
                                               radMemorySpace memorySpace = radMemorySpace_Local )
{
#if defined RADMEMORYMONITOR
    if ( radMemoryMonitorGet( ) != NULL )
    {
        radMemoryMonitorGet( )->DeclareAllocation( address, size, callStackDepth, memorySpace );
    }
#else
    (void) address; (void) size; (void) callStackDepth, (void) memorySpace;
#endif
}

inline void radMemoryMonitorRescindAllocation( void* address, radMemorySpace memorySpace = radMemorySpace_Local )
{
#if defined RADMEMORYMONITOR
    if ( radMemoryMonitorGet( ) != NULL )
    {
        radMemoryMonitorGet( )->RescindAllocation( address, memorySpace );
    }
#else
    (void) address; (void) memorySpace;
#endif
}

inline void radMemoryMonitorIdentifyAllocation( void* address, const char * group = NULL, const char* name = NULL,
                         unsigned int* pReferenceCount = NULL, radMemorySpace memorySpace = radMemorySpace_Local )
{
#if defined RADMEMORYMONITOR
    if ( radMemoryMonitorGet( ) != NULL )
    {
        radMemoryMonitorGet( )->IdentifyAllocation( address, group, name, pReferenceCount, memorySpace );
    }
#else
    (void) address; (void) group; (void) name, (void) pReferenceCount, (void) memorySpace;
#endif
}

inline void radMemoryMonitorIssueFlag( const char* flag )
{
#if defined RADMEMORYMONITOR
    if ( radMemoryMonitorGet( ) != NULL )
    {
        radMemoryMonitorGet( )->IssueFlag( flag );
    }
#else
    (void) flag;
#endif
}

inline void radMemoryMonitorSuspend( void )
{
#if defined RADMEMORYMONITOR
    if ( radMemoryMonitorGet( ) != NULL )
    {
    	radMemoryMonitorGet( )->Suspend( );
    }
#endif
}

inline void radMemoryMonitorReportAddRef( void* pReference, void* pObject, radMemorySpace memorySpaceObject = radMemorySpace_Local )
{
#if defined RADMEMORYMONITOR
    if ( radMemoryMonitorGet( ) != NULL )
    {
        radMemoryMonitorGet( )->ReportAddRef( pReference, pObject, memorySpaceObject );
    }
#else
    (void) pReference; (void) pObject;
#endif
}

inline void radMemoryMonitorReportRelease( void* pReference, void* pObject, radMemorySpace memorySpaceObject = radMemorySpace_Local )
{
#if defined RADMEMORYMONITOR
    if ( radMemoryMonitorGet( ) != NULL )
    {
        radMemoryMonitorGet( )->ReportRelease( pReference, pObject, memorySpaceObject );
    }
#else
    (void) pReference; (void) pObject;
#endif
}


#if !defined RADMEMORYMONITOR
//
// if RADMEMORYMONITOR is not defined, radMemoryMonitorInitialize does nothing
//
inline void radMemoryMonitorInitialize( unsigned int bufferSize, radMemoryAllocator alloc, bool SyncMode, bool ReportRefCount )
{
    (void)bufferSize;
    (void)alloc;
    (void)SyncMode;
}

//
// if RADMEMORYMONITOR is not defined, radMemoryMonitorTerminate does nothing
//
inline void radMemoryMonitorTerminate( void )
{
}

//
// if RADMEMORYMONITOR is not defined, radMemoryMonitorService does nothing
//
inline void radMemoryMonitorService( void )
{
}
#endif


#endif // RADMEMORYMONITOR_HPP