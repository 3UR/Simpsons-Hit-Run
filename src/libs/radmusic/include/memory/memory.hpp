//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
/// 
//-----------------------------------------------------------------------------

#ifndef MEMORY_HPP
#define MEMORY_HPP

//-----------------------------------------------------------------------------
// Nested Includes
//-----------------------------------------------------------------------------

#include <radmemory.hpp>

//-----------------------------------------------------------------------------
// Forward References
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Interface Definitions
//-----------------------------------------------------------------------------

namespace memory {

void               initialize( radMemoryAllocator allocator );
void               terminate( void );
radMemoryAllocator allocator( void );

void *             calloc( size_t, const char * p_name );
void *             malloc( size_t, const char * p_name );
void               free( void * p_mem );
void               free( void * p_mem, size_t size );
void *             realloc( void *, size_t old_size, size_t new_size, const char * p_name );

}
#endif // MEMORY_HPP