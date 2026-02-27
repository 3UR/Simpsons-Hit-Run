//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
/// 
//-----------------------------------------------------------------------------

#ifndef SEQUENCE_STREAM_GRAPH_HPP
#define SEQUENCE_STREAM_GRAPH_HPP

//-----------------------------------------------------------------------------
// Nested Includes
//-----------------------------------------------------------------------------

#include <radmusic/radmusic.hpp>

//-----------------------------------------------------------------------------
// Forward References
//-----------------------------------------------------------------------------

namespace radmusic
{
    struct stream_graph;
}

//-----------------------------------------------------------------------------
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Interface Definitions
//-----------------------------------------------------------------------------

namespace radmusic {
       
void stream_graph_construct(
    stream_graph *,
    IRadSoundHalAudioFormat *,
    bool use_bufered_data_source,
    radMemorySpace bufferedDatSourceMemorySpace,
    IRadMemoryAllocator * pIRadMemoryAllocator_FileBuffer,
    unsigned int fileBufferSize,
    unsigned int streamBufferSize,
    radMemoryAllocator allocator );

void stream_graph_destroy( stream_graph * );
                
//============================================================================
// "private" types
//============================================================================

const unsigned int STREAM_GRAPH_QUEUE_SIZE = 3;

struct struct_queue_item
{
    IRadSoundHalDataSource * p_data_source;
    unsigned int length;
    unsigned int stitch_pos;
};
            
struct stream_graph
{
    enum_stream_graph_state state;
    bool                    paused;

    IRadSoundStreamPlayer       * p_stream_player;
    IRadSoundStitchedDataSource * p_stitched_data_source;
    IRadSoundBufferedDataSource * p_buffered_data_source;
    	
    struct struct_queue_item queue[ STREAM_GRAPH_QUEUE_SIZE ];   
        
    unsigned char             queue_items;
    unsigned char             queue_head;
    
    unsigned int              next_stitch_position;             
};

}

#endif 
