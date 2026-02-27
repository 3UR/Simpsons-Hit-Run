//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include <radmemory.hpp>

//============================================================================
// ::radMemoryRoundUp
//============================================================================

size_t radMemoryRoundUp( size_t size, size_t alignment )
{
	if ( alignment != 0 )
	{
		if ( ( size % alignment ) != 0 )
		{
			return size + ( alignment - ( size % alignment ) );
		}
	}

    return size;
}

//============================================================================
// ::radMemoryRoundDown
//============================================================================

size_t radMemoryRoundDown( size_t size, size_t alignment )
{
    if ( alignment != 0 )
    {
        return ( size / alignment ) * alignment;
    }

    return size;
}

//============================================================================
// ::radMemoryIsAligned
//============================================================================

bool radMemoryIsAligned( size_t value, size_t alignment )
{
    return( value == radMemoryRoundUp( value, alignment ) );
}