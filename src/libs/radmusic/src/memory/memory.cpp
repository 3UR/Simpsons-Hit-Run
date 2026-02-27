//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "pch.hpp"
#include <radmemorymonitor.hpp>
#include <string.h>

//-----------------------------------------------------------------------------
// Local Constants, Typedefs, and Macros
//-----------------------------------------------------------------------------

namespace memory
{
    #ifdef TAG_ALLOCS
        const size_t g_allocation_header_size = 64;
    #else
        const size_t g_allocation_header_size = 0;
    #endif
}

//-----------------------------------------------------------------------------
// Global Data, Local Data, Local Classes [Optional commment can be deleted]
//-----------------------------------------------------------------------------

namespace memory
{
    radMemoryAllocator g_allocator = RADMEMORY_ALLOC_DEFAULT;
    const char * g_p_tag = "radmusic_memory";
}

//-----------------------------------------------------------------------------
// Member Functions
//-----------------------------------------------------------------------------

namespace memory {

void initialize( radMemoryAllocator allocator )
{
    g_allocator = allocator;
}

void terminate( void )
{
}

radMemoryAllocator allocator( void )
{
    return g_allocator;
}

void * malloc( size_t s, const char * p_name )
{
    char * p = (char*) ::radMemoryAlloc( g_allocator, s + g_allocation_header_size );
    
    ::radMemoryMonitorIdentifyAllocation(
        p, g_p_tag, p_name, 0, radMemorySpace_Local );
                
    if ( p != 0 )
    {
        strncpy( p, p_name, g_allocation_header_size );
        p += g_allocation_header_size;
    } 
    
    return p;  
}

void * calloc( size_t s, const char * p_name )
{
    void * p = memory::malloc( s, p_name );
    
    ::memset( p, 0, s );
    
    return p;
}

void free( void * p_mem)
{
    if ( p_mem != 0 )
    {
        char * p = (char*) p_mem;
        
        p -= g_allocation_header_size;
        
        radMemoryFree( g_allocator, p );
    }
}

void free( void * p_mem, size_t size )
{
    memory::free( p_mem );
}

void * realloc( void * p, size_t old_size, size_t new_size, const char * p_name )
{
    char* p_old_mem = (char*) p;
    
    if ( p_old_mem != 0 )
    {
        p_old_mem -= g_allocation_header_size;
    }
    
    char * p_new_mem;
    
    if ( new_size > 0 )
    {
         p_new_mem = (char*) radMemoryAlloc( g_allocator, new_size + g_allocation_header_size );
         
         radMemoryMonitorIdentifyAllocation(
            p_new_mem, g_p_tag, p_name, 0, radMemorySpace_Local );
        
        if ( p_new_mem != 0 )
        {
            strncpy( p_new_mem, p_name, g_allocation_header_size );
        
            if ( p_old_mem != 0 )
            {
                unsigned int copy_size = ( new_size > old_size  ) ? old_size : new_size;
                     
                memcpy( p_new_mem, p_old_mem, copy_size + g_allocation_header_size );
            }
            
            p_new_mem += g_allocation_header_size;
        }
    }
    else
    {
        p_new_mem = 0;
    }
    
    if ( p_old_mem != 0 )
    {
        radMemoryFree( g_allocator, p_old_mem );
    }
    
    return p_new_mem;
}

void * realloc( void * p, size_t old_size, size_t new_size)
{
    return memory::realloc( p, old_size, new_size, "unnamed realloc" );
}

}

