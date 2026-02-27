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
#include <radmusic/music/music_engine.hpp>
#include <radmusic/music/music_performance.hpp>
#include <memory/memory.hpp>

//-----------------------------------------------------------------------------
// Local Constants, Typedefs, and Macros
//-----------------------------------------------------------------------------

using namespace ods;

namespace radmusic {

//-----------------------------------------------------------------------------
// Global Data, Local Data, Local Classes
//-----------------------------------------------------------------------------

void _construct_region_controllers( struct_music_engine * p_se );
void _destroy_region_controllers( struct_music_engine * p_se );
void _do_action_push_region( struct_music_engine *, const push_region_action & );
void _do_action_pop_region(  struct_music_engine *, const pop_region_action  & );
void _do_action_play_region( struct_music_engine *, const play_region_action & );
void _do_action_start_layer( struct_music_engine *, const start_layer_action & );
void _do_action_stop_layer(  struct_music_engine *, const stop_layer_action  & );

typedef void (state_update_fx) (struct_music_engine *, unsigned int ms );
typedef bool (state_set_target_region_fx)(struct_music_engine *, region & ptr_target_region, bool reset_current_region );
typedef void (state_start_layer_fx)(struct_music_engine *, const char * p_layer_name );
typedef void (state_stop_layer_fx)(struct_music_engine *, const char * p_layer_name );
typedef void (state_set_volume_fx)(struct_music_engine *, float );
typedef void (state_set_pitch_fx)(struct_music_engine *, float );
typedef void (state_kill_fx)(struct_music_engine * );
typedef void (state_paused_fx)(struct_music_engine *, bool );
typedef region (state_current_region_fx)(struct_music_engine * );

struct music_engine_state_functions
{
    //state_enter_fx *             p_enter_fx;
    state_update_fx *            p_update_fx;
    state_set_target_region_fx * p_set_target_region_fx;
    state_start_layer_fx *       p_start_layer_fx;
    state_stop_layer_fx *        p_stop_layer_fx;
    state_set_volume_fx *        p_set_volume_fx;
    state_set_pitch_fx *         p_set_pitch_fx;
    state_kill_fx *              p_kill_fx;
    state_paused_fx *            p_paused_fx;
    state_current_region_fx *    p_current_region_fx;
} g_state_functions[ music_engine_state_max ] = {

    { state_stopped_update,
      state_stopped_set_target_region,
      state_stopped_start_layer,
      state_stopped_stop_layer,      
      state_stopped_set_volume,
      state_stopped_set_pitch,
      state_stopped_kill,
      state_stopped_paused,
      state_stopped_current_region },
      
    { state_steady_update,
      state_steady_set_target_region,
      state_steady_start_layer,
      state_steady_stop_layer,      
      state_steady_set_volume,
      state_steady_set_pitch,
      state_steady_kill,
      state_steady_paused,
      state_steady_current_region },
      
    { state_fade_update,
      state_fade_set_target_region,
      state_fade_start_layer,
      state_fade_stop_layer,      
      state_fade_set_volume,
      state_fade_set_pitch,
      state_fade_kill,
      state_fade_paused,
      state_fade_current_region },
      
    { state_stitch_update,
      state_stitch_set_target_region,
      state_stitch_start_layer,
      state_stitch_stop_layer,      
      state_stitch_set_volume,
      state_stitch_set_pitch,
      state_stitch_kill,
      state_stitch_paused,
      state_stitch_current_region } };

//-----------------------------------------------------------------------------
// Member Functions
//-----------------------------------------------------------------------------

void music_engine_construct(
    struct_music_engine * p_se,
    performance     * p_performance )
{
    p_se->volume = 1.0f;
    p_se->pitch  = 1.0f;
    p_se->paused = false;
    p_se->p_performance = p_performance;
    p_se->region_stack_pointer = 0;
    p_se->current_state = music_engine_state_stopped;

    state_fade_construct( p_se );    
    state_stopped_construct( p_se );
    state_steady_construct( p_se );
    state_stitch_construct( p_se );
    
    _construct_region_controllers( p_se );
    
    pointer_construct( (pointer*) & p_se->ptr_action_on_deck );
    
    for( unsigned int i = 0; i < REGION_STACK_SIZE; i ++ )
    {
        pointer_construct( (pointer*) & ( p_se->region_stack[ i ].ptr_region ) );
    }
};

void music_engine_destroy( struct_music_engine * p_se )
{
    for( unsigned int i = 0; i < REGION_STACK_SIZE; i ++ )
    {
        pointer_destroy( (pointer*) & ( p_se->region_stack[ i ].ptr_region ) );
    }

    pointer_destroy( (pointer*) & p_se->ptr_action_on_deck );
        
    _destroy_region_controllers( p_se );
    
    state_fade_destroy( p_se );
    state_stitch_destroy( p_se );
    state_steady_destroy( p_se );
    state_stopped_destroy( p_se );
}

void _construct_region_controllers( struct_music_engine * p_se )
{
    unsigned int num_regions = region_array_num_items(
        comp_regions( p_se->p_performance->p_composition->ptr_composition ) );
        
    p_se->p_region_controllers = (struct_region_controller*)
        memory::malloc( sizeof(struct_region_controller) *
            num_regions, "region controller array" );
            
    for( unsigned int i = 0; i < num_regions; i ++ )
    {
        region_controller_construct(
            p_se->p_region_controllers + i,
            p_se->p_performance->p_resource_manager,
            region_array_item_at(
                comp_regions( p_se->p_performance->p_composition->ptr_composition ),
                i ) );
    }
}

void _destroy_region_controllers( struct_music_engine * p_se )
{
    unsigned int num_regions = region_array_num_items(
        comp_regions( p_se->p_performance->p_composition->ptr_composition ) );
           
    for( unsigned int i = 0; i < num_regions; i ++ )
    {
        region_controller_destroy( p_se->p_region_controllers + i );
    }

    memory::free( p_se->p_region_controllers );    
    
}

struct_region_controller * music_engine_region_controller( struct_music_engine * p_se, const region & ptr_region )
{
    // revisit: this indexable now!
    
    unsigned int num_regions = region_array_num_items(
        comp_regions( p_se->p_performance->p_composition->ptr_composition ) );
            
    for( unsigned int i = 0; i < num_regions; i ++ )
    {
        if ( region_controller_region( p_se->p_region_controllers + i ) == ptr_region )
        {
            return p_se->p_region_controllers + i;
        }
    }
    
    rAssert( false );
    return 0;
}

void music_engine_paused( struct_music_engine * p_se, bool paused )
{
    if ( p_se->paused != paused )
    {
        p_se->paused = paused;
        
        g_state_functions[ p_se->current_state ].p_paused_fx( p_se, paused );
        
        unsigned int num_regions =
            region_array_num_items(
                comp_regions(
                    p_se->p_performance->p_composition->ptr_composition ) );
                
        for( unsigned int i = 0; i < num_regions; i ++ )
        {
            region_controller_paused( p_se->p_region_controllers + i, paused );
        }        
    }
}



region music_engine_peek_region( struct_music_engine * p_se, enum_region_resume_type * p_rt )
{
    if ( p_se->region_stack_pointer > 0 )
    {
        *p_rt = p_se->region_stack[ p_se->region_stack_pointer - 1 ].region_resume_type;
        return p_se->region_stack[ p_se->region_stack_pointer - 1 ].ptr_region;
    }

    return (region) pointer_null;
}

void music_engine_flush_region_stack( struct_music_engine * p_se )
{
    region ptr_region;

    while ( true )
    {
        enum_region_resume_type rt;
        
        music_engine_pop_region( p_se, & ptr_region, & rt );

        if ( is_null( ptr_region ) )
        {
            break;
        }
    }
}

void music_engine_push_region(
    struct_music_engine *p_se,
    region & ptr_region,
    enum_region_resume_type region_resume_type )
{
    rAssert( p_se->paused == false );

    if ( p_se->region_stack_pointer < REGION_STACK_SIZE )
    {
        char region_nom[ 256 ];
        region_name( ptr_region, region_nom, 256 );
        
        rDebugChannelPrintf(
            debug_channel,
            "radmusic: Pushing struct_region_controller: [%s]\n", region_nom );

        p_se->region_stack[ p_se->region_stack_pointer ].ptr_region = ptr_region;
        p_se->region_stack[ p_se->region_stack_pointer ].region_resume_type = region_resume_type;
        p_se->region_stack_pointer++;
    }
    else
    {
        rDebugString( "radmusic: Warning ignoring push operation out of stack space\n" );
    } 
}

void music_engine_pop_region(
    struct_music_engine * p_se,
    region * p_ptr_region,
    enum_region_resume_type * p_region_resume_type )
{
    rAssert( p_se->paused == false );

    if ( p_se->region_stack_pointer > 0 )
    {
        p_se->region_stack_pointer--;
        *p_ptr_region         = p_se->region_stack[ p_se->region_stack_pointer ].ptr_region;
        *p_region_resume_type = p_se->region_stack[ p_se->region_stack_pointer ].region_resume_type;

        p_se->region_stack[ p_se->region_stack_pointer ].ptr_region = (region) pointer_null;       

        char region_nom[ 256 ];
        region_name( *p_ptr_region, region_nom, 256 );

        rDebugChannelPrintf(
            debug_channel,
            "radmusic: Popping struct_region_controller: [%s]\n", region_nom );
    }
    else
    {
        *p_ptr_region = (region) pointer_null;
    }
}

void music_engine_current_region_name( struct_music_engine * p_se, char * p_string, unsigned int len )
{
    if ( p_se->current_state == music_engine_state_steady )
    {
        region ptr_region = g_state_functions[ p_se->current_state ].p_current_region_fx( p_se );
    
        region_name( ptr_region, p_string, len );
    }
    else if ( p_se->current_state == music_engine_state_stopped )
    {
        strncpy( p_string, "stopped", len );
    }
    else
    {
        strncpy( p_string, "transitioning", len );
    }

}

bool music_engine_is_state_steady_and_idle( struct_music_engine * p_me )
{
    if ( p_me->current_state == music_engine_state_steady )
    {
        return state_steady_is_idle( p_me );
    }
    else if ( p_me->current_state == music_engine_state_stopped )
    {
        return true;
    }

    return false;

}

bool music_engine_current_beat( struct_music_engine * p_se, float * p_beat )
{
    region ptr_region_current = g_state_functions[ p_se->current_state ].p_current_region_fx( p_se );
    
    if ( not_null( ptr_region_current ) )
    {
         sequence_player_beat(
            region_controller_primary_sequence(
                music_engine_region_controller( p_se, ptr_region_current ) ),
                    p_beat );
        return true;
    }
    else
    {
        *p_beat = 0.0f;
        
        return false;
    }    
}

void music_engine_update( struct_music_engine * p_se, unsigned int ms )
{
    g_state_functions[ p_se->current_state ].p_update_fx( p_se, ms );   
    
    if ( not_null( p_se->ptr_action_on_deck ) )
    {
        action ptr_action = p_se->ptr_action_on_deck;
        p_se->ptr_action_on_deck = (action) pointer_null;
        
        music_engine_do_action( p_se, ptr_action );
    }
    
};

void music_engine_stop( struct_music_engine * p_se )
{
    if ( p_se->current_state != music_engine_state_stopped )
    {
        g_state_functions[ p_se->current_state ].p_kill_fx( p_se );
        state_stopped_enter( p_se );
        p_se->current_state = music_engine_state_stopped;
    }
}

bool music_engine_paused( struct_music_engine * p_se )
{
    return p_se->paused;
}

void music_engine_volume( struct_music_engine * p_se, float v )
{
    p_se->volume = v;

    g_state_functions[ p_se->current_state ].p_set_volume_fx( p_se, v );    
}

float music_engine_volume( struct_music_engine * p_se )
{
    return p_se->volume;
}

void music_engine_pitch( struct_music_engine * p_se, float pitch )
{
    p_se->pitch = pitch;
    
    g_state_functions[ p_se->current_state ].p_set_pitch_fx( p_se, pitch );
}

float music_engine_pitch( struct_music_engine * p_se )
{
    return p_se->pitch;
}

void _do_action_push_region(
    struct_music_engine * p_se,
    const push_region_action & ptr_push_region_action )
{
    region ptr_region_target = push_region_action_region_ref( ptr_push_region_action );
    enum_region_resume_type crrt =
        (enum_region_resume_type) push_region_action_current_region_resume_type( ptr_push_region_action );
    enum_region_resume_type trrt =
        (enum_region_resume_type) push_region_action_target_region_resume_type( ptr_push_region_action );

    region ptr_region_current = g_state_functions[ p_se->current_state ].p_current_region_fx( p_se );
                
    if ( ptr_region_current == ptr_region_target )
    {
        char region_nom[ 256 ];
        region_name( ptr_region_current, region_nom, 256 );
        
        rDebugChannelPrintf(
            debug_channel,
            "radmusic: ignoring push operation, already playing: [%s]\n", region_nom );
        return;
    }

    bool did_set_region =
        g_state_functions[ p_se->current_state ].p_set_target_region_fx(
            p_se, ptr_region_target,
                trrt == region_resume_type_restart );
    
    if ( did_set_region )
    {
        if ( not_null( ptr_region_current ) )
        {
            enum_region_resume_type rrt;
            
            if ( ptr_region_current != music_engine_peek_region( p_se, & rrt ) )
            {
                music_engine_push_region( p_se, ptr_region_current, crrt );
            }
            else
            {
                char region_nom[ 256 ];
                region_name( ptr_region_current, region_nom, 256 );
                    
                rDebugChannelPrintf(
                    debug_channel,
                    "radmusic: region already on top of stack: [%s]\n",
                    region_nom );
            }
        }

        return;
    }

    p_se->ptr_action_on_deck = (action) ptr_push_region_action;
        
}

void _do_action_pop_region(
    struct_music_engine * p_se,
    const pop_region_action & ptr_pop_region_action )
{
    region ptr_region_target = pop_region_action_region_ref( ptr_pop_region_action );

    enum_region_resume_type rrt_tos;
    
    region ptr_region_top_of_stack = music_engine_peek_region( p_se, & rrt_tos );

    if ( is_null( ptr_region_top_of_stack ) )
    {
        rDebugChannelPrintf( debug_channel,
            "struct_music_engine: Stack is empty, can't perform pop operation\n" );

        return;
    }

    region ptr_region_current = g_state_functions[ p_se->current_state ].p_current_region_fx( p_se );
    
    if ( not_null( ptr_region_target ) )
    {
        if ( ptr_region_current != ptr_region_target )
        {
            char region_nom[ 256 ];
            region_name( ptr_region_target, region_nom, 256 );
            
            rDebugChannelPrintf( debug_channel,
                "struct_music_engine: Not playing: [%s], can't pop\n",
                    region_nom );

            return;
        }
    }
    
    bool did_set_region =
        g_state_functions[ p_se->current_state ].p_set_target_region_fx(
            p_se, ptr_region_top_of_stack, rrt_tos == region_resume_type_restart );

    if ( did_set_region == true )
    {
        char region_nom[ 256 ];
        region_name( ptr_region_top_of_stack, region_nom, 256 );
            
        rDebugChannelPrintf( debug_channel,
            "struct_music_engine: Popped of region: [%s]\n", region_nom );

        region my_region;

        music_engine_pop_region( p_se, & my_region, & rrt_tos );

        return;
    }
    
    p_se->ptr_action_on_deck = (action) ptr_pop_region_action;
}

void _do_action_play_region(
    struct_music_engine * p_se,
    const play_region_action & ptr_play_region_action )
{
    region ptr_region_target = play_region_action_region_ref( ptr_play_region_action );
    enum_region_resume_type rt =
        (enum_region_resume_type) play_region_action_region_resume_type( ptr_play_region_action );

    region ptr_region_current = g_state_functions[ p_se->current_state ].p_current_region_fx( p_se );
    
    if ( ptr_region_target == ptr_region_current )
    {
        char region_nom[ 256 ];
        region_name( ptr_region_current, region_nom, 256 );
        
        rDebugChannelPrintf(
            debug_channel,
            "radmusic: Ignoring switch, already playing: [%s]\n", region_nom ); 
        return;    
    }

    bool region_was_set =
        g_state_functions[ p_se->current_state ].p_set_target_region_fx(
            p_se, ptr_region_target, rt == region_resume_type_restart );
    
    if ( region_was_set == true )
    {
        music_engine_flush_region_stack( p_se );
    
        rDebugChannelPrintf(
            debug_channel, "radmusic: Flushing stack...\n" );

        return;
    }
    
    p_se->ptr_action_on_deck = (action) ptr_play_region_action;    
}

void _do_action_start_layer( struct_music_engine * p_se, const start_layer_action & ptr_start_layer_action )
{
    char layer_nom[ 256 ];
    start_layer_action_layer_name( ptr_start_layer_action, layer_nom, 256 );
 
    g_state_functions[ p_se->current_state ].p_start_layer_fx( p_se, layer_nom );
}

void _do_action_stop_layer( struct_music_engine * p_se, const stop_layer_action & ptr_stop_layer_action )
{
    char layer_nom[ 256 ];
    stop_layer_action_layer_name( ptr_stop_layer_action, layer_nom, 256 );
 
    g_state_functions[ p_se->current_state ].p_stop_layer_fx( p_se, layer_nom );
}

void music_engine_do_action( struct_music_engine * p_se, const action & ptr_action )
{
    switch( (enum_action_type) action_type( ptr_action ) )
    {
        case action_type_push_region:  _do_action_push_region( p_se, (push_region_action) ptr_action );  break;
        case action_type_pop_region:   _do_action_pop_region(  p_se, (pop_region_action) ptr_action );  break;
        case action_type_play_region:  _do_action_play_region( p_se, (play_region_action) ptr_action );  break;
        case action_type_start_layer:  _do_action_start_layer( p_se, (start_layer_action) ptr_action );  break;
        case action_type_stop_layer:   _do_action_stop_layer(  p_se, (stop_layer_action) ptr_action );  break;
        default:
            rAssert( false );
    };    
}


} // namespace radmusic