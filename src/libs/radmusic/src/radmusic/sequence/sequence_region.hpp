//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
///
//-----------------------------------------------------------------------------

#ifndef SEQUENCE_REGION_HPP
#define SEQUENCE_REGION_HPP

//-----------------------------------------------------------------------------
// Nested Includes
//-----------------------------------------------------------------------------

#include <radmusic/schema/schema_util.hpp>
#include <radmusic/schema.hpp>
#include <radmusic/sequence/sequence_player.hpp>
#include <radmusic/resource/resource_manager.hpp>

//-----------------------------------------------------------------------------
// Forward References
//-----------------------------------------------------------------------------

namespace radmusic
{
    struct struct_region_controller;
}

//-----------------------------------------------------------------------------
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Interface Definitions
//-----------------------------------------------------------------------------

namespace radmusic {

void                     region_controller_construct( struct_region_controller *, resource_manager *, const ods::region & );
void                     region_controller_destroy( struct_region_controller * );
void                     region_controller_resource_buffer( struct_region_controller *, unsigned int );
unsigned int             region_controller_resource_buffer( struct_region_controller * );
sequence_player*         region_controller_primary_sequence( struct_region_controller * );
void                     region_controller_queue_stream( struct_region_controller *, stream_graph *, bool reset_state, float volume, float pitch );
bool                     region_controller_is_queued( struct_region_controller * );
void                     region_controller_play( struct_region_controller * );
void                     region_controller_dequeue_stream( struct_region_controller *, stream_graph ** );
void                     region_controller_kill( struct_region_controller * );
void                     region_controller_paused( struct_region_controller *, bool );
void                     region_controller_volume( struct_region_controller *, float );
float                    region_controller_volume( struct_region_controller * );
void                     region_controller_pitch( struct_region_controller *, float );
void                     region_controller_start_layer( struct_region_controller *, const char * );
void                     region_controller_stop_layer( struct_region_controller *, const char * );
void                     region_controller_update( struct_region_controller * );
ods::region              region_controller_region( struct_region_controller * );

struct layer_controller;

struct struct_region_controller
{
    ods::region              ptr_region;
    unsigned int             current_resource_buffer;
    float                    volume;
    float                    last_beat;
    bool                     has_been_played;
    
    layer_controller *       p_layer_controllers;


};

}

#endif 