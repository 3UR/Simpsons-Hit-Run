//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
///
//-----------------------------------------------------------------------------

#ifndef MUSIC_PERFORMANCE_HPP
#define MUSIC_PERFORMANCE_HPP

//-----------------------------------------------------------------------------
// Nested Includes
//-----------------------------------------------------------------------------

#include <radmusic/music/music_engine.hpp>
#include <radmusic/music/music_composition.hpp>
#include <radmusic/radmusic.hpp>
#include <radsound.hpp>

//-----------------------------------------------------------------------------
// Forward References
//-----------------------------------------------------------------------------

namespace radmusic {

//-----------------------------------------------------------------------------
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Interface Definitions
//-----------------------------------------------------------------------------

void performance_update_all( unsigned int ms );

const unsigned int EVENT_QUEUE_SIZE = 20;

struct performance
{
    struct_music_engine      music_engine;
    resource_manager *       p_resource_manager;

    composition * p_composition;

    unsigned char * p_state_values;

    unsigned int * p_state_rand_history;
    unsigned int * p_event_last_action_index;
                           
    unsigned int    event_queue[ EVENT_QUEUE_SIZE ];
    unsigned int    event_queue_head;
    unsigned int    event_queue_num_items;       
};

}
#endif PERFORMANCE_HPP