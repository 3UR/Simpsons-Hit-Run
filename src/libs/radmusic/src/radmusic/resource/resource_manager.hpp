//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
/// 
//-----------------------------------------------------------------------------

#ifndef RESOURCE_MANAGER_HPP
#define RESOURCE_MANAGER_HPP

//-----------------------------------------------------------------------------
// Nested Includes
//-----------------------------------------------------------------------------

#include <radmusic/sequence/sequence_stream_graph.hpp>
#include <radmusic/radmusic.hpp>
#include <radsound.hpp>

//-----------------------------------------------------------------------------
// Forward References
//-----------------------------------------------------------------------------

namespace radmusic {

//-----------------------------------------------------------------------------
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//-----------------------------------------------------------------------------

const unsigned int NULL_RESOURCE_BUFFER = 0xFFFF0000;

//-----------------------------------------------------------------------------
// Interface Definitions
//-----------------------------------------------------------------------------

void resource_manager_initialize(
    stream_graph_description *,
    unsigned int num_stream_graphs,
    unsigned int num_clips );
    
void resource_manager_terminate( void );

}
#endif // RESOURCELOADER_HPP