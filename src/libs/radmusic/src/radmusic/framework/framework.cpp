//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
/// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "pch.hpp"
#include <radmusic/radmusic.hpp>
#include <radmusic/resource/resource_manager.hpp>
#include <memory/memory.hpp>

//-----------------------------------------------------------------------------
// Local Constants, Typedefs, and Macros [Optional commment can be deleted]
//-----------------------------------------------------------------------------

namespace radmusic {

//-----------------------------------------------------------------------------
// Global Data, Local Data, Local Classes [Optional commment can be deleted]
//-----------------------------------------------------------------------------

const char * debug_channel = "radmusic";

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
///
//-----------------------------------------------------------------------------

void initialize(
    stream_graph_description * p_sgd,
    unsigned int num_sg,
    unsigned int num_cp,
    radMemoryAllocator allocator )
{
    memory::initialize( allocator );
   
    resource_manager_initialize( p_sgd,
        num_sg,
        num_cp );
}

//-----------------------------------------------------------------------------
///
//-----------------------------------------------------------------------------

void terminate( void )
{
    resource_manager_terminate( );
   
    memory::terminate( );
}    

} // namespace radmusic

