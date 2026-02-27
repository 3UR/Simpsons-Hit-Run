//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
/// 
/// This is the comment from the template source file.  If you can't
/// think if anything useful to add to what you put in the header file,
/// to describe this class then delete this comment.
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "pch.hpp"
#include <radmusic/music/music_engine_state_stopped.hpp>
#include <radmusic/music/music_performance.hpp>
#include <radmusic/music/music_engine.hpp>


//-----------------------------------------------------------------------------
// Local Constants, Typedefs, and Macros
//-----------------------------------------------------------------------------

using namespace ods;

//-----------------------------------------------------------------------------
// Global Data, Local Data, Local Classes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Member Functions
//-----------------------------------------------------------------------------

namespace radmusic {

void state_stopped_construct( struct_music_engine * p_se )
{
    pointer_construct( (pointer*) & p_se->state_stopped.ptr_region_target );

    p_se->state_stopped.reset_target_region = true;
    p_se->state_stopped.state               = state_state_stopped_idle;
}

void state_stopped_destroy( struct_music_engine * p_se )
{
    pointer_destroy( (pointer*) & p_se->state_stopped.ptr_region_target );
}

bool state_stopped_set_target_region(
    struct_music_engine * p_se,
    region & ptr_region_target,
    bool reset_target_region )
{
    if ( p_se->state_stopped.state != state_state_stopped_idle )
    {
        return false; // busy
    }

    p_se->state_stopped.ptr_region_target = ptr_region_target; 
    p_se->state_stopped.reset_target_region = reset_target_region;
    
    char region_nom[ 256 ];
    region_name( ptr_region_target, region_nom, 256 );
    
    rDebugChannelPrintf( debug_channel,
        "Stopped: Loading region: [%s]\n", region_nom );

    region_controller_resource_buffer(
        music_engine_region_controller(
            p_se,
            p_se->state_stopped.ptr_region_target ),
        resource_manager_move_to_group(
            p_se->p_performance->p_resource_manager,
            region_group_ref( p_se->state_stopped.ptr_region_target ),
            0 ) );

    p_se->state_stopped.state = state_state_stopped_loading;            

    return true;
 
};

region state_stopped_current_region( struct_music_engine * p_se )
{
    return (region) pointer_null;
}

void state_stopped_update( struct_music_engine * p_se, unsigned int ms )
{
    if ( p_se->paused == false )
    {
        if ( p_se->state_stopped.state == state_state_stopped_idle )
        {
            // waiting for target
        }
        else if ( p_se->state_stopped.state == state_state_stopped_loading )
        {
            if ( resource_manager_group_loaded(
                p_se->p_performance->p_resource_manager,
                region_controller_resource_buffer(
                    music_engine_region_controller(
                        p_se,
                        p_se->state_stopped.ptr_region_target ) ),
                region_group_ref( p_se->state_stopped.ptr_region_target ) ) )
            {
                region_controller_queue_stream(
                    music_engine_region_controller(
                        p_se,
                        p_se->state_stopped.ptr_region_target ),
                    0, p_se->state_stopped.reset_target_region,
                    p_se->volume,
                    p_se->pitch );
                
                p_se->state_stopped.state = state_state_stopped_queueing;                                
            }        
        }
        else if ( p_se->state_stopped.state == state_state_stopped_queueing )
        {
            region_controller_update(
                music_engine_region_controller(
                    p_se,
                    p_se->state_stopped.ptr_region_target ) );
               
            if ( region_controller_is_queued(
                music_engine_region_controller( p_se, p_se->state_stopped.ptr_region_target ) ) )
            {
                rDebugChannelPrintf( debug_channel,
                    "Stopped: Next state ready transitioning\n");
                    
                state_steady_initialize( p_se, p_se->state_stopped.ptr_region_target );
                
                region_controller_play(
                    music_engine_region_controller(
                        p_se,
                        p_se->state_stopped.ptr_region_target
                    )
                );
                
                p_se->state_stopped.ptr_region_target = (region) pointer_null;
                p_se->state_stopped.reset_target_region = true;
                p_se->state_stopped.state = state_state_stopped_idle;

                p_se->current_state = music_engine_state_steady;
                
                state_steady_enter( p_se );                

            }
        }
    }
}

void state_stopped_enter( struct_music_engine * p_se )
{
    rDebugChannelPrintf( debug_channel,
        "radmusic: STATE: Enter Stopped\n" );
}

void state_stopped_kill( struct_music_engine * p_se )
{
     rDebugChannelPrintf( debug_channel, "radmusic: STATE: Stopped: Kill(), no action taken\n" );
}

void state_stopped_paused( struct_music_engine * p_se, bool )
{
    rDebugChannelPrintf( debug_channel, "radmusic: STATE: Stopped: Pause(), no action taken\n" );
}

void state_stopped_set_pitch( struct_music_engine * p_se, float pitch )
{
}

void state_stopped_set_volume( struct_music_engine * p_se, float volume )
{
}

void  state_stopped_start_layer( struct_music_engine * p_se, const char * p_layer_name )
{
}

void  state_stopped_stop_layer( struct_music_engine * p_se, const char * p_layer_name )
{
}
}