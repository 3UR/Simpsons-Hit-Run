//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
/// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "pch.hpp"
#include <radmusic/music/music_engine_state_fade.hpp>
#include <radmusic/music/music_performance.hpp>
#include <radmusic/music/music_engine.hpp>


//-----------------------------------------------------------------------------
// Local Constants, Typedefs, and Macros
//-----------------------------------------------------------------------------

using namespace ods;

//-----------------------------------------------------------------------------
// Global Data, Local Data, Local Classes
//-----------------------------------------------------------------------------

namespace radmusic
{
    void _state_fade_update_fade_volumes( struct_music_engine * p_se );
}

//-----------------------------------------------------------------------------
// Member Functions
//-----------------------------------------------------------------------------

namespace radmusic {

void state_fade_construct( struct_music_engine * p_se )
{
    
    p_se->state_fade.state = state_fade_state_idle;
    p_se->state_fade.current_source_volume = 0.0f;
    p_se->state_fade.current_target_volume = 0.0f;
    p_se->state_fade.last_beat = 0.0f;
    p_se->state_fade.reset_target_region = false;
    p_se->state_fade.running_time = 0;
    
    pointer_construct( (pointer*) & p_se->state_fade.ptr_region_source );
    pointer_construct( (pointer*) & p_se->state_fade.ptr_region_target );
    pointer_construct( (pointer*) & p_se->state_fade.ptr_fade_transition );
}

void state_fade_destroy( struct_music_engine * p_se )
{
    pointer_destroy( (pointer*) &p_se->state_fade.ptr_fade_transition );
    pointer_destroy( (pointer*) &p_se->state_fade.ptr_region_source );
    pointer_destroy( (pointer*) &p_se->state_fade.ptr_region_target );
}

void state_fade_kill( struct_music_engine * p_se )        
{
    rDebugChannelPrintf( debug_channel,
        "STATE: Fade: Kill( )\n" );
        
    region_controller_kill(
        music_engine_region_controller(
            p_se,
            p_se->state_fade.ptr_region_source ) );
 
     region_controller_kill(
        music_engine_region_controller(
            p_se,
            p_se->state_fade.ptr_region_target ) );
               
    p_se->state_fade.ptr_region_source = (region) pointer_null;
    p_se->state_fade.ptr_region_target = (region) pointer_null;

    p_se->state_fade.state = state_fade_state_idle;        
}

void state_fade_paused( struct_music_engine * p_se, bool paused )
{
    rDebugChannelPrintf( debug_channel,
        "STATE: Fade: Paused( %d )\n", paused );

    region_controller_paused(
        music_engine_region_controller(
            p_se,
            p_se->state_fade.ptr_region_source ),
        paused );
            
    region_controller_paused(
        music_engine_region_controller(
            p_se,
            p_se->state_fade.ptr_region_target ),
        paused );
}

void state_fade_update( struct_music_engine * p_se, unsigned int ms )
{
    if ( p_se->state_fade.state != state_fade_state_idle )
    {
        region_controller_update(
            music_engine_region_controller(
                p_se,
                p_se->state_fade.ptr_region_source ) );
            
        region_controller_update(
            music_engine_region_controller(
                p_se,
                p_se->state_fade.ptr_region_target ) );
    }
    
    if ( p_se->state_fade.state == state_fade_state_init_region )
    {
        bool group_loaded = resource_manager_group_loaded(
            p_se->p_performance->p_resource_manager,
            region_controller_resource_buffer(
                music_engine_region_controller(
                    p_se,
                    p_se->state_fade.ptr_region_target ) ),            
            region_group_ref( p_se->state_fade.ptr_region_target ) );

            
        if ( group_loaded == true )
        {
            region_controller_queue_stream(
                music_engine_region_controller(
                    p_se,
                    p_se->state_fade.ptr_region_target ),
                0,
                p_se->state_fade.reset_target_region,
                p_se->volume,
                p_se->pitch );       
                   
            p_se->state_fade.state = state_fade_state_init_stream;
        }
    }
    else if ( p_se->state_fade.state == state_fade_state_init_stream )
    {
        bool target_queued = region_controller_is_queued(
            music_engine_region_controller(
                p_se,
                p_se->state_fade.ptr_region_target ) );
                
        if ( target_queued )
        {
            p_se->state_fade.state = state_fade_state_initialized;
        }
    }
    else if ( p_se->state_fade.state == state_fade_state_initialized )
    {
        // waiting
    }
    else if ( p_se->state_fade.state == state_fade_state_entered )
    {
    
        struct_region_controller * p_region_controller_source =
            music_engine_region_controller(
                p_se,
                p_se->state_fade.ptr_region_source );
                
        struct_region_controller * p_region_controller_target =
            music_engine_region_controller(
                p_se,
                p_se->state_fade.ptr_region_target );                
                
        sequence_player * p_sequence_player_source =
            region_controller_primary_sequence( p_region_controller_source );
 
        sequence_player * p_sequence_player_target =
            region_controller_primary_sequence( p_region_controller_target );
                               
        if ( p_se->paused == true )
        {
            // No time has elapsed if we are paused;

            ms = 0;
        }

        // Waiting for target to queue...

	    if ( p_se->state_fade.running_time == 0xFFFFFFFF )
	    {
            // Ok here we are ready to start fading, but we might have to wait
            // for a specific beat.
            
            if ( float_array_num_items(
                beat_set_beats(
                    fade_transition_beat_set( p_se->state_fade.ptr_fade_transition ) ) )== 0 )
            {
                // Here we don't wait for a beat, so just start counting.
                p_se->state_fade.running_time = 0;
            }
            else
            {
                // We have to wait for the correct beat, if the ods::stream is
                // finished, just switch now.

                if ( sequence_player_state( p_sequence_player_source ) == sequence_player_state_finished )
                {
                    p_se->state_fade.running_time = 0;
                }
                else
                {
                    float this_beat;

                    sequence_player_beat( p_sequence_player_source, & this_beat );
                
                    bool in_range = beat_set_beat_in_range(
                        fade_transition_beat_set(
                            p_se->state_fade.ptr_fade_transition ),
                            p_se->state_fade.last_beat, this_beat );
                    
                    p_se->state_fade.last_beat = this_beat;
                    
                    if ( in_range == true )
                    {
                        p_se->state_fade.running_time = 0;
                    }
                    else
                    {
                        return;
                    }
                }
            }
	    }

        // Here target is queued

	    p_se->state_fade.running_time += ms;


	    //
	    // Fade out source
	    //
	    
	    unsigned int sourceStartTime = radSoundFloatToUInt( fade_transition_source_start( p_se->state_fade.ptr_fade_transition ) );
	    unsigned int sourceFadeTime  = radSoundFloatToUInt( fade_transition_source_time( p_se->state_fade.ptr_fade_transition ) );
	    unsigned int targetStartTime = radSoundFloatToUInt( fade_transition_target_start( p_se->state_fade.ptr_fade_transition ) );
	    unsigned int targetFadeTime  = radSoundFloatToUInt( fade_transition_target_time( p_se->state_fade.ptr_fade_transition ) );

        if ( targetFadeTime <= radSoundMinFadeTime )
        {
            targetFadeTime = radSoundMinFadeTime;
        }
        
        if ( sourceFadeTime <= radSoundMinFadeTime )
        {
            sourceFadeTime = radSoundMinFadeTime;
        }
        
        if ( p_se->state_fade.running_time >= sourceStartTime )
        {
	        if ( p_se->state_fade.current_source_volume >= 0.0f )
	        {
                rAssert( sourceFadeTime > 0 );
                
                float decrement = radSoundIntToFloat( ms ) / sourceFadeTime;
                
		        p_se->state_fade.current_source_volume -= decrement;
		    
		        if ( p_se->state_fade.current_source_volume < 0.0f )
		        {
			        p_se->state_fade.current_source_volume = 0.0f;
		        }

	        }
        }
	    
	    //
	    // Fade Target
	    //

	    if ( region_controller_is_queued( p_region_controller_target) &&
             ( p_se->state_fade.running_time >= targetStartTime ) )
	    {
            region_controller_play( p_region_controller_target );
	    }
	    
	    if( sequence_player_state( p_sequence_player_target ) == sequence_player_state_finished ||
	        sequence_player_state( p_sequence_player_target ) == sequence_player_state_finishing ||
	        sequence_player_state( p_sequence_player_target ) == sequence_player_state_playing )
	    {
		    if ( p_se->state_fade.current_target_volume < 1.0f )
		    {
                rAssert( targetFadeTime > 0 );

                float increment = radSoundIntToFloat( ms ) / targetFadeTime;
                
			    p_se->state_fade.current_target_volume += increment;
			
			    if ( p_se->state_fade.current_target_volume > 1.0f )
			    {
				    p_se->state_fade.current_target_volume = 1.0f;
			    }

		    }
	    }

        _state_fade_update_fade_volumes( p_se );

	    if ( region_controller_volume( p_region_controller_target ) == p_se->volume &&
	         region_controller_volume( p_region_controller_source ) == 0.0f )
	    {
	        state_steady_initialize( p_se, p_se->state_fade.ptr_region_target );

            region_controller_dequeue_stream(
                p_region_controller_source, 0 );  

            p_se->state_fade.ptr_region_source = (region) pointer_null;
            p_se->state_fade.ptr_region_target = (region) pointer_null;
            p_se->state_fade.state = state_fade_state_idle;
            
            p_se->current_state = music_engine_state_steady;
            
            state_steady_enter( p_se );
   	    }  
    }				 
}

void state_fade_initialize(
    struct_music_engine * p_se,
    region & ptr_region_current,
    bool reset_target_region,
    region & ptr_region_target,
    fade_transition & ptr_fade_transition )
{

    rAssert( p_se->state_fade.state == state_fade_state_idle );

    p_se->state_fade.state = state_fade_state_init_region;
    p_se->state_fade.ptr_fade_transition = ptr_fade_transition;
    p_se->state_fade.reset_target_region = reset_target_region;
    p_se->state_fade.ptr_region_source = ptr_region_current;
    p_se->state_fade.ptr_region_target = ptr_region_target;

    region_controller_resource_buffer(
        music_engine_region_controller(
            p_se,
            p_se->state_fade.ptr_region_target ),
        resource_manager_move_to_group(
            p_se->p_performance->p_resource_manager,
            region_group_ref( p_se->state_fade.ptr_region_target ),
                region_controller_resource_buffer(
                    music_engine_region_controller(
                        p_se,
                        ptr_region_current ) ) ) );   
 
	p_se->state_fade.current_source_volume = 1.0f;
	p_se->state_fade.current_target_volume = fade_transition_target_time( p_se->state_fade.ptr_fade_transition ) == 0.0f ? 1.0f : 0.0f;
	p_se->state_fade.running_time = 0xFFFFFFFF;
	
	state_fade_update( p_se, 0 );
}

region state_fade_current_region( struct_music_engine * p_se )
{
    return p_se->state_fade.ptr_region_target;
}

bool state_fade_initialized( struct_music_engine * p_se )
{
    return p_se->state_fade.state == state_fade_state_initialized;
}

void state_fade_cancel_init( struct_music_engine * p_se )
{
    rAssert( p_se->state_fade.state == state_fade_state_init_stream ||
             p_se->state_fade.state == state_fade_state_init_region ||
             p_se->state_fade.state == state_fade_state_initialized );
    
    if ( p_se->state_fade.state == state_fade_state_init_stream || p_se->state_fade.state == state_fade_state_initialized )
    {
        region_controller_dequeue_stream(
            music_engine_region_controller(
                p_se,
                p_se->state_fade.ptr_region_target ),
            0 );
    }

    p_se->state_fade.ptr_region_target = (region) pointer_null;
    p_se->state_fade.ptr_region_source = (region) pointer_null;
         
    p_se->state_fade.state = state_fade_state_idle;
}

void state_fade_enter( struct_music_engine * p_se )
{
    rAssert( p_se->state_fade.state == state_fade_state_initialized );

    p_se->state_fade.state = state_fade_state_entered;
    
    sequence_player_beat(
        region_controller_primary_sequence(
            music_engine_region_controller(
                p_se,
                p_se->state_fade.ptr_region_source ) ),
        & p_se->state_fade.last_beat );
    
    char source_region_nom[ 256 ];
    char target_region_nom[ 256 ];
    
    region_name( p_se->state_fade.ptr_region_target, source_region_nom, 256 );           
    region_name( p_se->state_fade.ptr_region_source, target_region_nom, 256 );
    
    rDebugChannelPrintf( debug_channel,
        ": STATE: Fade: [%s] --> [%s]\n",
        source_region_nom,
        target_region_nom );
        
    state_fade_update( p_se, 0 );
}

void state_fade_start_layer( struct_music_engine * p_se, const char * p_layer_name )
{
    region_controller_start_layer(
        music_engine_region_controller(
            p_se,
            p_se->state_fade.ptr_region_source ),
        p_layer_name );
        
    region_controller_start_layer(
        music_engine_region_controller(
            p_se,
            p_se->state_fade.ptr_region_target ),
        p_layer_name );
}

void state_fade_stop_layer( struct_music_engine * p_se, const char * p_layer_name )
{
    region_controller_stop_layer(
        music_engine_region_controller(
            p_se,
            p_se->state_fade.ptr_region_source ),
        p_layer_name );
        
    region_controller_stop_layer(
        music_engine_region_controller(
            p_se,
            p_se->state_fade.ptr_region_target ),
        p_layer_name );
}

void state_fade_set_volume( struct_music_engine * p_se, float volume )
{
    _state_fade_update_fade_volumes( p_se );
}

void state_fade_set_pitch( struct_music_engine * p_se, float pitch )
{
    region_controller_pitch(
        music_engine_region_controller(
            p_se,
            p_se->state_fade.ptr_region_target ),
        pitch );        
            
    region_controller_pitch(
        music_engine_region_controller(
            p_se,
            p_se->state_fade.ptr_region_source ),
        pitch );    
}

float correct_vol( float oldVol, float newVol )
{
    float ret = newVol;
    
    if ( newVol > oldVol )
    {
        float dif = newVol - oldVol;
        
        if ( dif > radSoundVolumeChangeThreshold )
        {
            ret = oldVol + radSoundVolumeChangeThreshold;
        }
    }
    else
    {
        float dif = oldVol - newVol;
        
        if ( dif > radSoundVolumeChangeThreshold )
        {
            ret = oldVol - radSoundVolumeChangeThreshold;
        }
    }
    
    return ret;
}

void _state_fade_update_fade_volumes( struct_music_engine * p_se )
{
    struct_region_controller * pTargetRc = music_engine_region_controller( p_se, p_se->state_fade.ptr_region_target );
    struct_region_controller * pSourceRc = music_engine_region_controller( p_se, p_se->state_fade.ptr_region_source );
    
    float sv_current = region_controller_volume( pSourceRc );
    float tv_current  = region_controller_volume( pTargetRc );
    
    float sv_target = p_se->volume * sqrtf( p_se->state_fade.current_source_volume );
    float tv_target = p_se->volume * sqrtf( p_se->state_fade.current_target_volume );
    
    float sv_actual = correct_vol( sv_current, sv_target );
    float tv_actual = correct_vol( tv_current, tv_target );

    region_controller_volume( pTargetRc, tv_actual );    
            
    region_controller_volume( pSourceRc, sv_actual );  
}

fade_transition state_fade_fade_transition( struct_music_engine * p_se )
{
    return p_se->state_fade.ptr_fade_transition;
}

region state_fade_source_region( struct_music_engine * p_se )
{
    return p_se->state_fade.ptr_region_source;
}

region state_fade_target_region( struct_music_engine * p_se )
{
    return p_se->state_fade.ptr_region_target;
}

bool state_fade_set_target_region(
    struct_music_engine * p_se,
    region & ptr_region,
    bool reset_target_region )
{
    return false;
}


}