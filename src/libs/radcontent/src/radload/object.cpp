//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <radload/utility/object.hpp>
#include <raddebug.hpp>
#include <radmemorymonitor.hpp>
#include <string.h>
#include <typeinfo>


#ifdef RADLOAD_HEAP_DEBUGGING
static const unsigned int MAX_ADDRESSES_PER_HASH = 256;
static const unsigned int MAX_HASHES = 256;
static unsigned int maxUsed[MAX_HASHES];
static const void* addresses[MAX_HASHES][MAX_ADDRESSES_PER_HASH];
static bool initialized = false;

unsigned int HashPointer( const void* ptr )
{
    unsigned int hash = (uintptr_t)ptr;
    hash = (hash & 0x0000FFFF) ^ ((hash >> 16) & 0x0000FFFF);
    hash = (hash & 0x000000FF) ^ ((hash >> 8) & 0x000000FF);
    return hash;
}

void InitializeHeapDebugging()
{
    if( !initialized )
    {
        memset( maxUsed, 0, MAX_HASHES*sizeof(unsigned int) );
        memset( addresses, 0, MAX_HASHES*MAX_ADDRESSES_PER_HASH*sizeof(void*) ); 
        initialized = true;
    }
}

// check if an address is in the table
bool ValidAddress( const void* rawAddress )
{
    unsigned int hash = HashPointer( rawAddress );
    for( unsigned int i = 0; i < maxUsed[hash]; i++ )
    {
        if( addresses[hash][i] == rawAddress )
        {
            return true;
        }
    }
    return false;
}

void radLoadHeapDebugAddAddress( const void* rawAddress )
{
    unsigned int hash = HashPointer( rawAddress );

    for( unsigned int i = 0; i < maxUsed[hash]; i++ )
    {
        if( !addresses[hash][i] )
        {
            addresses[hash][i++] = rawAddress;
            return;
        }
    }
    rAssertMsg( maxUsed[hash] < MAX_ADDRESSES_PER_HASH, "Out of space for radLoad heap debugging.\nEither disable it, or increase the space\n" );
    addresses[hash][maxUsed[hash]++] = rawAddress;
}

void radLoadHeapDebugRemoveAddress( const void* rawAddress )
{
    unsigned int hash = HashPointer( rawAddress );
    for( unsigned int i = 0; i < maxUsed[hash]; i++ )
    {
        if( addresses[hash][i] == rawAddress )
        {
            addresses[hash][i] = NULL;
            if( i == (maxUsed[hash] - 1) )
            {
                while( (addresses[hash][i] == NULL ) && i > 0 )
                {
                    i--;
                }
                maxUsed[hash] = (addresses[hash][i] == NULL) ? i : i + 1;
            }
            return;
        }
    }
    rDebugString( "Tried to remove an invalid addresss from radLoad heap debugging\n" );
    rAssert( false );
}

#endif

radMemoryAllocator radLoadObject::s_allocator = RADLOAD_ALLOC_DEFAULT;

radLoadObject::radLoadObject() :
m_refCount( 0 )
#ifdef RADLOAD_USE_RADMEMORYMONITOR
,m_bMemMonitorInitialized( false )
#endif
{
#ifdef RADLOAD_HEAP_DEBUGGING
    rAssertMsg( ValidAddress( this ), "Failed to verify address in radLoadObject constructor.\n Object is probably allocated on the stack, part of an array, or part of a multiply inherited class\n" );
#endif
}

radLoadObject::~radLoadObject()
{
#ifdef RADLOAD_HEAP_DEBUGGING
    rAssertMsg( !ValidAddress( this ), "Failed to verify address in radLoadObject destructor.\nObject was probably directly deleted.\n" );
#endif
}

void radLoadObject::AddRef()
{
#ifdef RADLOAD_USE_RADMEMORYMONITOR
    if( !m_bMemMonitorInitialized )
    {
        m_bMemMonitorInitialized = true;
        const type_info& t = typeid( *this );
        radMemoryMonitorIdentifyAllocation( this, "radLoad", t.name(), &m_refCount );
    }
    radMemoryMonitorReportAddRef( &m_refCount, this );
#endif
#ifdef RADLOAD_HEAP_DEBUGGING
    rAssertMsg( ValidAddress( static_cast<const void*>(this) ), "Failed to verify address in radLoadObject::AddRef().\nPossible bad cast, or already deleted object.\n" );
#endif
    m_refCount++;
}

void radLoadObject::Release()
{
#ifdef RADLOAD_USE_RADMEMORYMONITOR
    radMemoryMonitorReportRelease( &m_refCount, this );
#endif
#ifdef RADLOAD_HEAP_DEBUGGING
#ifdef RADLOAD_HEAP_DEBUGGING_PARANOID
    rAssertMsg( ValidAddress( static_cast<const void*>(this) ), "Failed to verify address in radLoadObject::Release.\nPossible double delete\n" );
#endif
#endif
    if( m_refCount < 2 )
    {
#ifdef RADLOAD_HEAP_DEBUGGING
    rAssertMsg( ValidAddress( static_cast<const void*>(this) ), "Failed to verify address in radLoadObject::Release.\nPossible double delete\n" );
    radLoadHeapDebugRemoveAddress( this );
#endif
        // Set the reference counting to a large number
        // in order to avoid a recursive delete.
        m_refCount = 0x08000000;
        delete this;
        return;
    }
    m_refCount--;
}

void radLoadObject::ReleaseVerified()
{
    rAssert( m_refCount < 2 );
    Release();
}

void* radLoadObject::operator new( size_t size )
{
    void* mem = radMemoryAlloc( radMemoryGetCurrentAllocator(), size );

#ifdef RADLOAD_USE_RADMEMORYMONITOR
    radMemoryMonitorIdentifyAllocation( mem, "radLoad", "radLoadObject" );
#endif
#ifdef RADLOAD_HEAP_DEBUGGING
    radLoadHeapDebugAddAddress( mem );
#endif
    return mem;
}

void* radLoadObject::operator new( size_t size, radMemoryAllocator alloc )
{
    void* mem = radMemoryAlloc( alloc, size );

#ifdef RADLOAD_USE_RADMEMORYMONITOR
    radMemoryMonitorIdentifyAllocation( mem, "radLoad", "radLoadObject" );
#endif
#ifdef RADLOAD_HEAP_DEBUGGING
    radLoadHeapDebugAddAddress( mem );
#endif
    return mem;
}

void* radLoadObject::operator new[]( size_t size )
{
    void* mem = radMemoryAlloc( radMemoryGetCurrentAllocator(), size );

#ifdef RADLOAD_USE_RADMEMORYMONITOR
    radMemoryMonitorIdentifyAllocation( mem, "radLoad", "radLoadObject" );
#endif
#ifdef RADLOAD_HEAP_DEBUGGING
    radLoadHeapDebugAddAddress( mem );
#endif
    return mem;
}

void* radLoadObject::operator new[]( size_t size, radMemoryAllocator alloc )
{
    void* mem = radMemoryAlloc( alloc, size );

#ifdef RADLOAD_USE_RADMEMORYMONITOR
    radMemoryMonitorIdentifyAllocation( mem, "radLoad", "radLoadObject" );
#endif
#ifdef RADLOAD_HEAP_DEBUGGING
    radLoadHeapDebugAddAddress( mem );
#endif
    return mem;
}

void radLoadObject::operator delete( void* ptr )
{
#ifdef RADLOAD_USE_RADMEMORYMONITOR
    radMemoryMonitorRescindAllocation( ptr );
#endif
#ifdef RADLOAD_HEAP_DEBUGGING
    rAssertMsg( !ValidAddress( ptr ), "Failed to verify address in radLoadObject::delete.\nObject was probably directly deleted\n");
#endif
    radMemoryFree( ptr );
}

void radLoadObject::operator delete[]( void* ptr )
{
#ifdef RADLOAD_USE_RADMEMORYMONITOR
    radMemoryMonitorRescindAllocation( ptr );
#endif
#ifdef RADLOAD_HEAP_DEBUGGING
    rAssertMsg( !ValidAddress( ptr ), "Failed to verify address in radLoadObject::delete.\nObject was probably directly deleted\n");
#endif
    radMemoryFree( ptr );
}