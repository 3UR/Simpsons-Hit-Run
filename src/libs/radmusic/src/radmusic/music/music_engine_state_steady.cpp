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
#include <radmusic/music/music_engine_state_steady.hpp>
#include <radmusic/music/music_performance.hpp>
#include <radmusic/music/music_engine.hpp>

//-----------------------------------------------------------------------------
// Local Constants, Typedefs, and Macros
//-----------------------------------------------------------------------------

using namespace ods;

//-----------------------------------------------------------------------------
// Global Data, Local Data, Local Classes [Optional commment can be deleted]
//-----------------------------------------------------------------------------

namespace radmusic
{
    void _state_steady_init_transition( struct_music_engine *, region & region_target, bool reset_target );
    void _state_steady_clear_transition( struct_music_engine * );
    void _state_steady_do_transition( struct_music_engine * );
    void _state_steady_check_for_transition( struct_music_engine * );
    void _state_steady_reset( struct_music_engine * );
}

//-----------------------------------------------------------------------------
// Member Functions
//-----------------------------------------------------------------------------

namespace radmusic {

void state_steady_construct( struct_music_engine * p_se )
{
    pointer_construct( (pointer*) & p_se->state_steady.ptr_region_current );
    pointer_construct( (pointer*) & p_se->state_steady.ptr_region_target );

    _state_steady_reset( p_se );
}    

void state_steady_destroy( struct_music_engine * p_se )
{
    _state_steady_reset( p_se );
    
    pointer_destroy( (pointer*) & p_se->state_steady.ptr_region_current );
    pointer_destroy( (pointer*) & p_se->state_steady.ptr_region_target );    
}

void _state_steady_reset( struct_music_engine * p_se )
{
    p_se->state_steady.entered = false;
    p_se->state_steady.p_state_fade = 0;
    p_se->state_steady.p_state_stitch = 0;
    p_se->state_steady.ptr_region_current = (region) pointer_null;
    p_se->state_steady.ptr_region_target  = (region) pointer_null;
    p_se->state_steady.transition_type    = state_steady_transition_type_stop;
}

bool state_steady_is_idle( struct_music_engine * p_se )
{
    if ( p_se->state_steady.p_state_stitch != NULL )
    {
        return state_stitch_initialized( p_se );
    }
    else if ( p_se->state_steady.p_state_fade != NULL )
    {
        return state_fade_initialized( p_se );
    }
    
    return true;
}

void state_steady_initialize(
    struct_music_engine * p_se,
    region & ptr_region_current )
{
    char region_nom[ 256 ];
    region_name( ptr_region_current, region_nom, 256 );
    
    rDebugChannelPrintf( debug_channel,
        ": Init State Steady: region: [%s]\n", region_nom );

    p_se->state_steady.ptr_region_current = ptr_region_current;
}

region state_steady_current_region( struct_music_engine * p_se )
{
    region ptr_region_current;

    if ( p_se->state_steady.transition_type == state_steady_transition_type_switch )
    {
        if ( p_se->state_steady.p_state_fade != 0 )
        {
            ptr_region_current = state_fade_current_region( p_se );
        }
        else
        {
            rAssert( p_se->state_steady.p_state_stitch != 0 );

            ptr_region_current = state_stitch_current_region( p_se );
        }
    }
    else
    {
        ptr_region_current = p_se->state_steady.ptr_region_current;
    }

    return ptr_region_current;
}

void state_steady_enter( struct_music_engine * p_se )
{
    char region_nom[ 256 ];
    region_name( p_se->state_steady.ptr_region_current, region_nom, 256 );
    
    rDebugChannelPrintf( debug_channel,
        ": STATE: Steady: region: [%s]\n", region_nom );

    rAssert( p_se->state_steady.entered == false );
    rAssert( p_se->state_steady.transition_type == state_steady_transition_type_stop );

    region ptr_exit_region = region_exit_region_ref( p_se->state_steady.ptr_region_current );

    p_se->state_steady.entered = true;    
    
    if ( not_null( ptr_exit_region ) )
    {
        p_se->state_steady.transition_type = state_steady_transition_type_exit;
        
        _state_steady_init_transition( p_se, ptr_exit_region, true );
    }
}

void state_steady_set_volume( struct_music_engine * p_se, float volume )
{
    region_controller_volume(
        music_engine_region_controller(
            p_se,
            p_se->state_steady.ptr_region_current ),
        volume );
}

void state_steady_set_pitch( struct_music_engine * p_se, float pitch )
{
    region_controller_pitch(
        music_engine_region_controller(
            p_se,
            p_se->state_steady.ptr_region_current ),
        pitch );
}
void state_steady_kill( struct_music_engine * p_se )
{
    rDebugChannelPrintf( debug_channel,
        "STATE: Steady: Kill( )\n" );

    if ( p_se->state_steady.p_state_stitch != 0 )
    {
        state_stitch_kill( p_se );
    }
    else if ( p_se->state_steady.p_state_fade != 0 )
    {
        state_fade_kill( p_se );
    }
    else
    {
        region_controller_kill(
            music_engine_region_controller(
                p_se,
                p_se->state_steady.ptr_region_current ) );
    }

    _state_steady_reset( p_se );
}

void state_steady_paused( struct_music_engine * p_se, bool paused )
{
    rDebugChannelPrintf( debug_channel,
        "STATE: Steady: Pause( )\n" );

    region_controller_paused(
        music_engine_region_controller(
            p_se,
            p_se->state_steady.ptr_region_current ),
        paused );  
}

void state_steady_start_layer( struct_music_engine * p_se, const char * p_layer_name )
{
    region_controller_start_layer(
        music_engine_region_controller(
            p_se,
            p_se->state_steady.ptr_region_current ),
        p_layer_name );    
}

void state_steady_stop_layer( struct_music_engine * p_se, const char * p_layer_name )
{
    region_controller_stop_layer(
        music_engine_region_controller(
            p_se,
            p_se->state_steady.ptr_region_current ),
        p_layer_name );    
}


bool state_steady_set_target_region(
    struct_music_engine * p_se,
    region & ptr_region_target,
    bool reset_target_region )
{
    // If we are waiting for our next state to initialize, do nothing

    if ( p_se->state_steady.transition_type == state_steady_transition_type_switch )
    {
        return false; // Didn't handle request, busy
    }

    if ( ptr_region_target == p_se->state_steady.ptr_region_current )
    {
        rDebugChannelPrintf( debug_channel, "region: ignoring request to play region we are already playing\n" );
        return true;
    }
    
    if ( ( ptr_region_target == p_se->state_steady.ptr_region_target ) &&
         ( p_se->state_steady.transition_type == state_steady_transition_type_pop ) &&
         ( reset_target_region == false ) )
    {
        p_se->state_steady.transition_type = state_steady_transition_type_switch;
    }
    else
    {
        _state_steady_clear_transition( p_se );
        p_se->state_steady.transition_type = state_steady_transition_type_switch;
        _state_steady_init_transition( p_se, ptr_region_target, reset_target_region );
    }
    
    return true;					
}

void _state_steady_clear_transition( struct_music_engine * p_se )
{
    

    if ( p_se->state_steady.p_state_stitch != 0 )
    {
        // Kill ?
        p_se->state_steady.p_state_stitch = 0;
    }
    else if ( p_se->state_steady.p_state_fade != 0 )
    {
        state_fade_cancel_init( p_se );
        p_se->state_steady.p_state_fade = 0;
    }       

    p_se->state_steady.transition_type = state_steady_transition_type_stop;
}

void _state_steady_init_transition(
    struct_music_engine * p_se,
    region & ptr_region_target,
    bool reset_target_region )
{
    p_se->state_steady.ptr_region_target = ptr_region_target;
	    
    stitch_transition ptr_stitch_transition =
        composition_find_stitch_transition(
            p_se->p_performance->p_composition->ptr_composition,
            p_se->state_steady.ptr_region_current,
            p_se->state_steady.ptr_region_target );
	    
    if ( not_null( ptr_stitch_transition ) )
    {
        p_se->state_steady.p_state_stitch = & p_se->state_stitch;
        
        state_stitch_initialize(
            p_se,
            p_se->state_steady.ptr_region_current,
            p_se->state_steady.ptr_region_target,
            ptr_stitch_transition );
        
        _state_steady_check_for_transition( p_se );
    }
    else
    {
        fade_transition ptr_fade_transition =
            composition_find_fade_transition(
                p_se->p_performance->p_composition->ptr_composition,
                p_se->state_steady.ptr_region_current,
                p_se->state_steady.ptr_region_target );
            
        if ( not_null( ptr_fade_transition ) )
        {
            p_se->state_steady.p_state_fade = & p_se->state_fade;
                
            state_fade_initialize(
                p_se,
                p_se->state_steady.ptr_region_current,
                reset_target_region,
                p_se->state_steady.ptr_region_target,
                ptr_fade_transition );
                
            _state_steady_check_for_transition( p_se );                
    
        }
        else
        {
		    rDebugChannel( debug_channel, "Error: No Default transition defined. Can't transition.\n" );        
        }
    }
}

void _state_steady_do_transition( struct_music_engine * p_se )
{
    if ( p_se->state_steady.p_state_stitch != 0 )
    {
        if ( state_stitch_initialized( p_se ) )
        {
            if ( p_se->state_steady.transition_type == state_steady_transition_type_pop )
            {   
                enum_region_resume_type rrt;
                region ptr_region;
                music_engine_pop_region( p_se, & ptr_region, & rrt );
            }

            _state_steady_reset( p_se );
            
            p_se->current_state = music_engine_state_stitch;
            
            state_stitch_enter( p_se );            
        }
    }
    else
    {
        if ( state_fade_initialized( p_se ) )
        {
            if ( p_se->state_steady.transition_type == state_steady_transition_type_pop )
            {
                enum_region_resume_type rrt;              
                region ptr_region;
                music_engine_pop_region( p_se, & ptr_region, & rrt );
            }

            _state_steady_reset( p_se );
            
            p_se->current_state = music_engine_state_fade;
            
            state_fade_enter( p_se );            
        }
    }
}

void _state_steady_check_for_transition( struct_music_engine * p_se )
{
    //
    // Here we are waiting for the target region to load
    //

    if ( p_se->state_steady.p_state_stitch != 0 )
    {
        if ( state_stitch_initialized( p_se ) )
        {        
            if ( p_se->state_steady.transition_type == state_steady_transition_type_switch )
            {
                _state_steady_do_transition( p_se );
            }
            else if ( ( p_se->state_steady.transition_type == state_steady_transition_type_exit ) ||
                    ( p_se->state_steady.transition_type == state_steady_transition_type_pop ) )
            {
                sequence_player * p_sequence_player_primary =
                    region_controller_primary_sequence(
                        music_engine_region_controller(
                            p_se,
                            p_se->state_steady.ptr_region_current ) );
                            
                if ( sequence_player_state( p_sequence_player_primary ) == sequence_player_state_finishing ||
                    sequence_player_state( p_sequence_player_primary ) == sequence_player_state_finished )
                {
                    _state_steady_do_transition( p_se );
                }
            }
        }
    }
    else if ( p_se->state_steady.p_state_fade != 0 )
    {
        if ( state_fade_initialized( p_se ) )
        {
            if ( p_se->state_steady.transition_type == state_steady_transition_type_switch )
            {
                _state_steady_do_transition( p_se );
            }
            else if ( ( p_se->state_steady.transition_type == state_steady_transition_type_exit ) ||
                    ( p_se->state_steady.transition_type == state_steady_transition_type_pop  ) )
            {
                fade_transition ptr_fade_transition = state_fade_fade_transition( p_se );
                
                unsigned int source_time       = radSoundFloatToUInt( fade_transition_source_time( ptr_fade_transition ) );
                unsigned int target_start_time = radSoundFloatToUInt( fade_transition_target_start( ptr_fade_transition ) );
                    
                unsigned int pre_roll_time = source_time + target_start_time;                     

                sequence_player * p_sequence_player_primary =
                    region_controller_primary_sequence(
                        music_engine_region_controller(
                            p_se,
                            p_se->state_steady.ptr_region_current ) );
                            
                unsigned int remaining_time = sequence_player_remaining_time( p_sequence_player_primary );
                
                if ( remaining_time <= pre_roll_time )                
                {
                    _state_steady_do_transition( p_se );
                }
            }
        }
    }
    else
    {
        rAssert( p_se->state_steady.transition_type == state_steady_transition_type_stop );

        enum_region_resume_type rrt;
        region ptr_region_peek = music_engine_peek_region( p_se, & rrt );
        
        if ( not_null( ptr_region_peek ) )
        {
            if ( ptr_region_peek != p_se->state_steady.ptr_region_current )
            {
                p_se->state_steady.transition_type = state_steady_transition_type_pop;
                _state_steady_init_transition( p_se, ptr_region_peek, false );
            }
        }
        else
        {
            sequence_player * p_sequence_player_primary =
                region_controller_primary_sequence(
                    music_engine_region_controller(
                        p_se,
                        p_se->state_steady.ptr_region_current ) );
                                
            if ( sequence_player_state( p_sequence_player_primary ) == sequence_player_state_finished )
            {
                region_controller_dequeue_stream(
                    music_engine_region_controller(
                        p_se,
                        p_se->state_steady.ptr_region_current ),
                    0 );

                _state_steady_reset( p_se );
                
                p_se->current_state = music_engine_state_stopped;
                
                state_stopped_enter( p_se );
            }
        }
    }

}

void state_steady_update( struct_music_engine * p_se, unsigned int ms )
{
    if ( p_se->state_steady.entered == true )
    {
        if ( p_se->paused == false )
        {
            region_controller_update(
                music_engine_region_controller(
                    p_se,
                    p_se->state_steady.ptr_region_current ) );
                    
            if ( p_se->state_steady.p_state_stitch != 0 )
            {
                state_stitch_update( p_se, ms );
            }
            else if ( p_se->state_steady.p_state_fade != 0 )
            {
                state_fade_update( p_se, ms );
            }

           _state_steady_check_for_transition( p_se );
        }
    }
}

}