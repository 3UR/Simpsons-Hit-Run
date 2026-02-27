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
#include <radmusic/music/music_engine_state_stitch.hpp>
#include <radmusic/music/music_engine_state_fade.hpp>
#include <radmusic/music/music_performance.hpp>
#include <radmusic/music/music_engine.hpp>


//-----------------------------------------------------------------------------
// Local Constants, Typedefs, and Macros [Optional commment can be deleted]
//-----------------------------------------------------------------------------

using namespace ods;

//-----------------------------------------------------------------------------
// Global Data, Local Data, Local Classes [Optional commment can be deleted]
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Member Functions
//-----------------------------------------------------------------------------

namespace radmusic {

void state_stitch_construct( struct_music_engine * p_se )
{
    p_se->state_stitch.entered = false;  

    pointer_construct( (pointer*) & p_se->state_stitch.ptr_stitch_transition );
    pointer_construct( (pointer*) & p_se->state_stitch.ptr_region_transition );
    pointer_construct( (pointer*) & p_se->state_stitch.ptr_region_current );
    pointer_construct( (pointer*) & p_se->state_stitch.ptr_region_target );
}

void state_stitch_destroy( struct_music_engine * p_se )
{
    pointer_destroy( (pointer*) & p_se->state_stitch.ptr_stitch_transition );
    pointer_destroy( (pointer*) & p_se->state_stitch.ptr_region_transition );
    pointer_destroy( (pointer*) & p_se->state_stitch.ptr_region_current );
    pointer_destroy( (pointer*) & p_se->state_stitch.ptr_region_target );
}


void state_stitch_initialize(
    struct_music_engine * p_se,
    region & ptr_region_current,
    region & ptr_region_target,
    stitch_transition & ptr_stitch_transition )
{
    p_se->state_stitch.entered = false;
    p_se->state_stitch.ptr_stitch_transition = ptr_stitch_transition;
    p_se->state_stitch.ptr_region_current = ptr_region_current;
    p_se->state_stitch.ptr_region_target  = ptr_region_target;
    p_se->state_stitch.ptr_region_transition = stitch_transition_transition_region_ref( ptr_stitch_transition );

    if ( not_null( p_se->state_stitch.ptr_region_transition ) )
    {
        region_controller_resource_buffer(
            music_engine_region_controller(
                p_se,
                p_se->state_stitch.ptr_region_transition ),
                    resource_manager_move_to_group(
                        p_se->p_performance->p_resource_manager,
                        region_group_ref( p_se->state_stitch.ptr_region_transition ),
                        region_controller_resource_buffer(
                            music_engine_region_controller( p_se, ptr_region_current ) ) ) );
            
    }
    else
    {
        region_controller_resource_buffer(
            music_engine_region_controller(
                p_se,
                p_se->state_stitch.ptr_region_target ),
                    resource_manager_move_to_group(
                        p_se->p_performance->p_resource_manager,
                        region_group_ref( p_se->state_stitch.ptr_region_target ),
                        region_controller_resource_buffer(
                            music_engine_region_controller( p_se, ptr_region_current ) ) ) );                        
    }
}

bool state_stitch_initialized( struct_music_engine * p_se )
{
    bool loaded;
    
    if ( not_null( p_se->state_stitch.ptr_region_transition ) )
    {
        loaded = resource_manager_group_loaded(
            p_se->p_performance->p_resource_manager,
            region_controller_resource_buffer(
                music_engine_region_controller(
                    p_se, p_se->state_stitch.ptr_region_transition ) ),
            region_group_ref( p_se->state_stitch.ptr_region_transition ) );
    }
    else
    {
        loaded = resource_manager_group_loaded(
            p_se->p_performance->p_resource_manager,
            region_controller_resource_buffer(
                music_engine_region_controller(
                    p_se, p_se->state_stitch.ptr_region_target ) ),
            region_group_ref( p_se->state_stitch.ptr_region_target ) );
    }
    
    return loaded;
}

region state_stitch_current_region( struct_music_engine * p_se )
{
    return p_se->state_stitch.ptr_region_target;
}

void state_stitch_enter( struct_music_engine * p_se )
{
    char target_region_name[ 256 ];
    char current_region_name[ 256 ];
    char transition_region_name[ 256 ];
    
    region_name( p_se->state_stitch.ptr_region_current, current_region_name, 256 );
    region_name( p_se->state_stitch.ptr_region_target, target_region_name, 256 );
   
    if ( not_null( p_se->state_stitch.ptr_region_transition ) )
    {
        region_name( p_se->state_stitch.ptr_region_transition, transition_region_name, 256 );
    }
    else
    {
        strncpy( transition_region_name, "none", 256 );
    }
    
    rDebugChannelPrintf( debug_channel,
        ": STATE: Stitch: [%s] --> [%s] --> [%s]\n",
            current_region_name,
            transition_region_name,
            target_region_name );

    p_se->state_stitch.entered = true;

    stream_graph * p_sg;
    
    region_controller_dequeue_stream(
        music_engine_region_controller(
            p_se,
            p_se->state_stitch.ptr_region_current ),
            & p_sg );
 
    if ( not_null( p_se->state_stitch.ptr_region_transition ) )
    {
        rAssert( resource_manager_group_loaded(
            p_se->p_performance->p_resource_manager,
            region_controller_resource_buffer(
                music_engine_region_controller(
                    p_se,
                    p_se->state_stitch.ptr_region_transition ) ),
            region_group_ref( p_se->state_stitch.ptr_region_transition ) ) );
            
        region_controller_queue_stream(
            music_engine_region_controller(
                p_se,
                p_se->state_stitch.ptr_region_transition ),
                p_sg,
                true,
                p_se->volume,
                p_se->pitch );
    }
    else if ( not_null( p_se->state_stitch.ptr_region_target ) )
    {
        rAssert( resource_manager_group_loaded(
            p_se->p_performance->p_resource_manager,
            region_controller_resource_buffer(
                music_engine_region_controller(
                    p_se,
                    p_se->state_stitch.ptr_region_target ) ),
            region_group_ref( p_se->state_stitch.ptr_region_target ) ) );
            
        region_controller_queue_stream(
            music_engine_region_controller(
                p_se,
                p_se->state_stitch.ptr_region_target ),
                p_sg,
                true,
                p_se->volume,
                p_se->pitch );
    }
}

void state_stitch_kill( struct_music_engine * p_se )
{
    rDebugChannelPrintf( debug_channel,
        "STATE: Stitch: Kill( )\n" );

    region_controller_kill(
        music_engine_region_controller(
            p_se,
            p_se->state_stitch.ptr_region_current ) );
            
    if ( not_null( p_se->state_stitch.ptr_region_transition ) )
    {
        region_controller_kill(
            music_engine_region_controller(
                p_se,
                p_se->state_stitch.ptr_region_transition ) );
    }

    region_controller_kill(
        music_engine_region_controller(
            p_se,
            p_se->state_stitch.ptr_region_target ) );

    p_se->state_stitch.ptr_region_current    = (region) pointer_null;
    p_se->state_stitch.ptr_region_target     = (region) pointer_null;
    p_se->state_stitch.ptr_region_transition = (region) pointer_null;
    p_se->state_stitch.entered = false;
}

void state_stitch_paused( struct_music_engine * p_se, bool pause )
{
    rDebugChannelPrintf( debug_channel,
        "STATE: Stitch: Pause( )\n" );

    if ( not_null( p_se->state_stitch.ptr_region_current ) )
    {
        region_controller_paused(
            music_engine_region_controller(
                p_se,
                p_se->state_stitch.ptr_region_current ),
            pause );
    }
    
    if ( not_null( p_se->state_stitch.ptr_region_transition ) )
    {
        region_controller_paused(
            music_engine_region_controller(
                p_se,
                p_se->state_stitch.ptr_region_transition ),
            pause );
    }
    
    if ( not_null( p_se->state_stitch.ptr_region_target ) )
    {
        region_controller_paused(
            music_engine_region_controller(
                p_se,
                p_se->state_stitch.ptr_region_target ),
            pause );
    }
}

void state_stitch_start_layer( struct_music_engine * p_se, const char * p_layer_name )
{
    region_controller_start_layer(
        music_engine_region_controller(
            p_se,
            p_se->state_stitch.ptr_region_current ),
        p_layer_name );

    if ( not_null( p_se->state_stitch.ptr_region_transition ) )
    {
        region_controller_start_layer(
            music_engine_region_controller(
                p_se,
                p_se->state_stitch.ptr_region_transition ),
            p_layer_name );
    }
    else
    {
        region_controller_start_layer(
            music_engine_region_controller(
                p_se,
                p_se->state_stitch.ptr_region_target ),
            p_layer_name );
    }
}

void state_stitch_stop_layer( struct_music_engine * p_se, const char * p_layer_name )
{
    region_controller_stop_layer(
        music_engine_region_controller(
            p_se,
            p_se->state_stitch.ptr_region_current ),
        p_layer_name );

    if ( not_null( p_se->state_stitch.ptr_region_transition ) )
    {
        region_controller_stop_layer(
            music_engine_region_controller(
                p_se,
                p_se->state_stitch.ptr_region_transition ),
            p_layer_name );
    }
    else
    {
        region_controller_stop_layer(
            music_engine_region_controller(
                p_se,
                p_se->state_stitch.ptr_region_target ),
            p_layer_name );
    }
}

void state_stitch_update( struct_music_engine * p_se, unsigned int ms )
{
    region_controller_update(
        music_engine_region_controller(
            p_se,
            p_se->state_stitch.ptr_region_current ) );

    region_controller_update(
        music_engine_region_controller(
            p_se,
            p_se->state_stitch.ptr_region_target ) );
                        
    if ( not_null( p_se->state_stitch.ptr_region_transition ) )
    {
        region_controller_update(
            music_engine_region_controller(
                p_se,
                p_se->state_stitch.ptr_region_transition ) );
    }

    if ( p_se->state_stitch.entered == false )
    {
        return;
    }

    if ( not_null( p_se->state_stitch.ptr_region_transition ) )
    {
        if ( region_controller_is_queued(
                music_engine_region_controller(
                    p_se,
                    p_se->state_stitch.ptr_region_transition ) ) )
        {
            region_controller_play(
                music_engine_region_controller(
                    p_se,
                    p_se->state_stitch.ptr_region_transition ) );
                    
            enum_sequence_player_state sp_state =
                sequence_player_state(
                    region_controller_primary_sequence(
                        music_engine_region_controller(
                            p_se,
                            p_se->state_stitch.ptr_region_transition ) ) );
                            
           
            if( sp_state == sequence_player_state_finished ||
                sp_state == sequence_player_state_finishing )
            {            
                stream_graph * p_sg;
                
                region_controller_dequeue_stream(
                    music_engine_region_controller(
                        p_se,
                        p_se->state_stitch.ptr_region_transition ),
                    & p_sg );

                region_controller_resource_buffer(
                    music_engine_region_controller(
                        p_se,
                        p_se->state_stitch.ptr_region_target ),
                    resource_manager_move_to_group(
                        p_se->p_performance->p_resource_manager,
                        region_group_ref( p_se->state_stitch.ptr_region_target ),
                        region_controller_resource_buffer(
                            music_engine_region_controller(
                                p_se,
                                p_se->state_stitch.ptr_region_transition ) ) ) );                        
                        
                region_controller_queue_stream(
                    music_engine_region_controller(
                        p_se,
                        p_se->state_stitch.ptr_region_target ),
                    p_sg, true,
                    p_se->volume,
                    p_se->pitch );
                   
                p_se->state_stitch.ptr_region_transition = (region) pointer_null;
            }            
        }
    }
    else
    {
        bool target_controller_queued =
            region_controller_is_queued(
                music_engine_region_controller(
                    p_se,
                    p_se->state_stitch.ptr_region_target ) );
                
        if( target_controller_queued )
        {
            region_controller_play(
                music_engine_region_controller(
                    p_se,
                    p_se->state_stitch.ptr_region_target ) );
                    
            state_steady_initialize( p_se, p_se->state_stitch.ptr_region_target );

            p_se->state_stitch.ptr_region_current = (region) pointer_null;
            p_se->state_stitch.ptr_region_target = (region) pointer_null;
            p_se->state_stitch.ptr_region_transition = (region) pointer_null;
            p_se->state_stitch.entered = false;
    
            p_se->current_state = music_engine_state_steady;
            
            state_steady_enter( p_se );            
        }
    }
}

void state_stitch_set_volume( struct_music_engine * p_se, float volume )
{
    
    if ( not_null( p_se->state_stitch.ptr_region_transition ) )
    {
        region_controller_volume(
            music_engine_region_controller(
                p_se,
                p_se->state_stitch.ptr_region_transition ),
            volume );
    }

    region_controller_volume(
        music_engine_region_controller(
            p_se,
            p_se->state_stitch.ptr_region_target ),
        volume );
        
    region_controller_volume(
        music_engine_region_controller(
            p_se,
            p_se->state_stitch.ptr_region_current ),
        volume );        
}

void state_stitch_set_pitch( struct_music_engine * p_se, float pitch )
{    
    if ( not_null( p_se->state_stitch.ptr_region_transition ) )
    {
        region_controller_pitch(
            music_engine_region_controller(
                p_se,
                p_se->state_stitch.ptr_region_transition ),
            pitch );
    }

    region_controller_pitch(
        music_engine_region_controller(
            p_se,
            p_se->state_stitch.ptr_region_target ),
        pitch );
        
    region_controller_pitch(
        music_engine_region_controller(
            p_se,
            p_se->state_stitch.ptr_region_current ),
        pitch );   
}

bool state_stitch_set_target_region(
    struct_music_engine * p_se,
    region & ptr_region,
    bool reset_target_region )
{
    return false;
}

}

