//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
/// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "pch.hpp"
#include <radmusic/music/music_performance.hpp>
#include <radmusic/schema/schema_util.hpp>
#include <memory/memory.hpp>

//-----------------------------------------------------------------------------
// Local Constants, Typedefs, and Macros
//-----------------------------------------------------------------------------

using namespace ods;

//-----------------------------------------------------------------------------
// Global Data, Local Data, Local Classes [Optional commment can be deleted]
//-----------------------------------------------------------------------------

namespace
{
    const unsigned int INVALID_INDEX = 0xFFFFFFFFu;

    inline unsigned int select_random_from_mask( unsigned int mask )
    {
        rAssert( mask != 0 );

        unsigned int count = 0;
        unsigned int temp = mask;
        while ( temp )
        {
            temp &= ( temp - 1 );
            count++;
        }

        unsigned int choice = ( count > 0 ) ? ( rand() % count ) : 0;

        for ( unsigned int bit = 0; bit < 32; ++bit )
        {
            if ( mask & ( 1u << bit ) )
            {
                if ( choice == 0 )
                {
                    return bit;
                }
                --choice;
            }
        }

        return 0;
    }

    inline unsigned int full_mask_for( unsigned int count )
    {
        return ( count >= 32u ) ? 0xFFFFFFFFu : ( ( 1u << count ) - 1u );
    }
}

namespace radmusic
{
    performance * g_p_performance_list_head = 0;

    void _do_event_actions( performance * p_performance, unsigned int ei );
    void _do_action_rand_state(  performance * p_performance, const rand_state_action  & );

    unsigned int    _get_state_index( performance * p_performance, const state & ptr_state );
    unsigned int    _get_state_value( performance * p_performance, const state & ptr_state );
}

//-----------------------------------------------------------------------------
// Member Functions
//-----------------------------------------------------------------------------

namespace radmusic {

performance * performance_new(
	composition * p_composition,
	const char * p_search_directory,
	radMemorySpace pre_loaded_stream_space )
{
    performance * p_performance = (performance*)
        memory::malloc( sizeof( performance ), "performance" );
       
    p_performance->p_composition = p_composition;
    p_performance->p_composition->ref_count++;
    
    unsigned int num_states = state_array_num_items(
        comp_states( p_performance->p_composition->ptr_composition ) );

    p_performance->p_state_values = (unsigned char*) memory::calloc(
        sizeof( unsigned char ) * num_states,
                "performance::p_state_values" );

    p_performance->p_state_rand_history = (unsigned int*) memory::calloc(
        sizeof( unsigned int ) * num_states,
                "performance::p_state_rand_history" );

    unsigned int num_events = event_array_num_items(
        comp_events( p_performance->p_composition->ptr_composition ) );

    if ( num_events > 0 )
    {
        p_performance->p_event_last_action_index = (unsigned int*) memory::malloc(
            sizeof( unsigned int ) * num_events,
                "performance::p_event_last_action_index" );

        for ( unsigned int event_index = 0; event_index < num_events; ++event_index )
        {
            p_performance->p_event_last_action_index[ event_index ] = INVALID_INDEX;
        }
    }
    else
    {
        p_performance->p_event_last_action_index = 0;
    }

    p_performance->event_queue_head = 0;
    p_performance->event_queue_num_items = 0;
    
    unsigned int sound_memory_size = 0;
    unsigned int cache_memory_size = 0;
    unsigned int num_sound_resources = 0;
    unsigned int num_cache_resources = 0;
    
    group ptr_root_group = group_array_item_at(
        comp_root_groups( p_performance->p_composition->ptr_composition ), 0 );
    
    
    group_memory_sizes_recurse( ptr_root_group,
        & num_sound_resources,
        & sound_memory_size,
        0,
        & num_cache_resources,
        & cache_memory_size,
        0 );     

        p_performance->p_resource_manager = resource_manager_new(
            p_search_directory,
            memory::allocator( ),
            num_sound_resources, sound_memory_size,
            pre_loaded_stream_space,
            memory::allocator( ),
            num_cache_resources, cache_memory_size );

    music_engine_construct(
        & p_performance->music_engine,
        p_performance ); 
        
    return p_performance;   
}

void performance_delete( performance ** p_performance )
{
    rAssert( (*p_performance)->p_composition->ref_count > 0 );
    
    performance_stop( *p_performance );
    
    memory::free( (*p_performance)->p_state_values );
    memory::free( (*p_performance)->p_state_rand_history );

    if ( (*p_performance)->p_event_last_action_index != 0 )
    {
        memory::free( (*p_performance)->p_event_last_action_index );
    }
    
    resource_manager_delete( & ((*p_performance)->p_resource_manager) );
    music_engine_destroy(  & (*p_performance)->music_engine );    
            
    (*p_performance)->p_composition->ref_count--;
    
    memory::free( *p_performance );
    
    (*p_performance) = 0;
}    

void performance_stop( performance * p_performance )
{
    music_engine_stop( & p_performance->music_engine );
}

void performance_paused( performance * p_performance, bool pause )
{
    music_engine_paused( & p_performance->music_engine, pause );
}

bool performance_paused( performance * p_performance)
{
    return music_engine_paused( & p_performance->music_engine );
}

void performance_volume( performance * p_performance, float volume )
{
    music_engine_volume( & p_performance->music_engine, volume );
}

float performance_volume( performance * p_performance)
{
    return music_engine_volume( & p_performance->music_engine );
}

void  performance_pitch( performance * p_performance, float pitch )
{
    music_engine_pitch( & p_performance->music_engine, pitch );
}

float performance_pitch( performance * p_performance)
{
    return music_engine_pitch( & p_performance->music_engine );
}


unsigned int performance_num_states( performance * p )
{
    return state_array_num_items( comp_states( p->p_composition->ptr_composition ) );

}

void performance_state_name( performance * p, unsigned int index, char * p_string, unsigned int len )
{
    return state_name(
        state_array_item_at(
            comp_states( p->p_composition->ptr_composition ), index ),
                p_string, len );

}

unsigned int performance_num_state_values( performance * p, unsigned int i )
{
    state ptr_state = state_array_item_at(
        comp_states( p->p_composition->ptr_composition ), i );
    
    string_array ptr_string_array = state_values( ptr_state );
    
    return string_array_num_items( ptr_string_array );

}

void performance_state_value_name( performance * p, unsigned int e, unsigned int i, char * p_string, unsigned int len )
{
    state ptr_state = state_array_item_at(
        comp_states( p->p_composition->ptr_composition ), e );
        
    string_array ptr_string_array = state_values( ptr_state );
    
    string_array_item_at( ptr_string_array, i, p_string, len );
}

void performance_state_value( performance * p, unsigned int e, unsigned int i)
{
    p->p_state_values[ e ] = i;
}

unsigned int performance_state_value( performance * p, unsigned int e )
{
    return p->p_state_values[ e ];

}

unsigned int performance_num_events( performance * p_performance )
{    
    return event_array_num_items( comp_events( p_performance->p_composition->ptr_composition ) );
}

void performance_event_name(
    performance * p_performance,
    unsigned int ei,
    char * p_name,
    unsigned int chars )
{
    ::event ptr_event = event_array_item_at(
        comp_events(
            p_performance->p_composition->ptr_composition ),
                ei );
    
    event_name( ptr_event, p_name, chars );
}

void performance_trigger_event( performance * p_performance, unsigned int ei )
{
    rAssert( ei < event_array_num_items(
        comp_events(
            p_performance->p_composition->ptr_composition ) ) );
            
    p_performance->event_queue[
        ( p_performance->event_queue_head + p_performance->event_queue_num_items ) % EVENT_QUEUE_SIZE ] = ei;
    
    if ( p_performance->event_queue_num_items < EVENT_QUEUE_SIZE )
    {
        p_performance->event_queue_num_items++;
    }
    else
    {
        rTunePrintf( "AUDIO: MUSIC: Event Queue Full, dropping events!!!\n" );
        
        p_performance->event_queue_head =
            ( p_performance->event_queue_head + 1 ) % EVENT_QUEUE_SIZE;
    }
}

void performance_update( performance * p_performance, unsigned int ms )
{
	music_engine_update(  & p_performance->music_engine, ms );
    
    event_array ptr_event_array = comp_events( p_performance->p_composition->ptr_composition );
    
    if ( performance_paused( p_performance ) == false )
    {
        while( p_performance->event_queue_num_items > 0 )
        {
            unsigned int eventIndex = p_performance->event_queue[ p_performance->event_queue_head ];
            
            char eventName[ 256 ];
            
            event_name(
                event_array_item_at(
                    ptr_event_array, eventIndex ),
                eventName,
                256 );
                            
            rTunePrintf(
                "AUDIO: Performance Event: [%s] (0x%x)\n", eventName, p_performance );
            
            _do_event_actions( p_performance, eventIndex );
            
            p_performance->event_queue_head = ( p_performance->event_queue_head + 1 ) % EVENT_QUEUE_SIZE;
            p_performance->event_queue_num_items--; 
            

        }

	}
}

void debug_performance_current_region_name(
    performance * p_performance,
    char * p_string,
    unsigned int len )
{
    music_engine_current_region_name( & p_performance->music_engine, p_string, len );
}

bool performance_is_state_steady_idle( performance * p )
{
    return music_engine_is_state_steady_and_idle( & p->music_engine );
}

bool debug_performance_current_beat( performance * p_performance, float * p_beat)
{
    return music_engine_current_beat( & p_performance->music_engine, p_beat );    
}

unsigned int _get_state_index( performance * p_performance, const state & ptr_state )
{
    // revisit -- optimize me
    state_array ptr_state_array = comp_states( p_performance->p_composition->ptr_composition );
    
    for( unsigned int i = 0; i < state_array_num_items( ptr_state_array ); i ++ )
    {
        state ptr_state_test = state_array_item_at( ptr_state_array, i );
        
        if ( ptr_state_test == ptr_state )
        {   
            return i;
        }
    }
    
    rAssert( false );
    
    return 0xFFFFFFFF;
}

unsigned int _get_state_value( performance * p_performance, const state & ptr_state )
{
    unsigned int state_index = _get_state_index( p_performance, ptr_state );
    
    return p_performance->p_state_values[ state_index ];    
}

void _do_action_rand_state( performance * p_performance, const rand_state_action & ptr_rand_state_action )
{
    state ptr_state = rand_state_action_state_ref( ptr_rand_state_action );
    
    unsigned int state_value_index = _get_state_index( p_performance, ptr_state );
    
    string_array ptr_string_array = state_values( ptr_state );
    unsigned int num_state_values = string_array_num_items( ptr_string_array );
    
    if ( num_state_values > 1 )
    {
        unsigned int current_value = p_performance->p_state_values[ state_value_index ];
        unsigned int new_value = current_value;

        if ( num_state_values <= 32 )
        {
            unsigned int full_mask = full_mask_for( num_state_values );
            unsigned int used_mask = p_performance->p_state_rand_history[ state_value_index ];

            used_mask |= ( 1u << current_value );

            if ( used_mask == full_mask )
            {
                used_mask = ( 1u << current_value );
            }

            unsigned int candidate_mask = full_mask & ~used_mask;
            candidate_mask &= ~( 1u << current_value );

            if ( candidate_mask == 0 )
            {
                candidate_mask = full_mask & ~( 1u << current_value );
            }

            if ( candidate_mask != 0 )
            {
                new_value = select_random_from_mask( candidate_mask );
            }
            else
            {
                new_value = ( current_value + 1 ) % num_state_values;
            }

            used_mask |= ( 1u << new_value );
            p_performance->p_state_rand_history[ state_value_index ] = used_mask;
        }
        else
        {
            do
            {
                new_value = rand() % num_state_values;
            } while( new_value == current_value );
        }

        performance_state_value( p_performance, state_value_index, new_value );
    }
}


void _do_event_actions( performance * p_performance, unsigned int i )
{  
    event ptr_event = event_array_item_at( comp_events( p_performance->p_composition->ptr_composition ), i );

    event_matrix ptr_event_matrix = event_event_matrix_ref( ptr_event );
    
    unsigned int action_index;
        
    if ( is_null( ptr_event_matrix ) )
    {
        action_index = 0;
    }
    else
    {
        state_ref_array ptr_state_ref_array = event_matrix_state_ref_array( ptr_event_matrix );
        unsigned int num_states = state_ref_array_num_items( ptr_state_ref_array );
        
        unsigned int d[ 50 ];
        
        for( unsigned int i = 0; i < num_states; i ++ )
        {
            state ptr_state = state_ref_array_item_at( ptr_state_ref_array, i );
                
            d[ i ] = _get_state_value( p_performance, ptr_state );
        }
    
        action_index = calculate_action_offset( ptr_event, d );
    }
    
    action_ref_array_array ptr_action_array_array = event_action_arrays( ptr_event );
    unsigned int num_action_arrays = action_ref_array_array_num_items( ptr_action_array_array );

    if ( num_action_arrays == 0 )
    {
        return;
    }

    if ( action_index >= num_action_arrays )
    {
        action_index = action_index % num_action_arrays;
    }

    if ( num_action_arrays > 1 && p_performance->p_event_last_action_index != 0 )
    {
        unsigned int last_index = p_performance->p_event_last_action_index[ i ];
        if ( last_index != INVALID_INDEX && last_index == action_index )
        {
            action_index = ( action_index + 1 ) % num_action_arrays;
        }
    }

    if ( p_performance->p_event_last_action_index != 0 )
    {
        p_performance->p_event_last_action_index[ i ] = action_index;
    }

    action_ref_array ptr_action_ref_array =
        action_ref_array_array_item_at( ptr_action_array_array, action_index );
    
    unsigned int num_actions = action_ref_array_num_items( ptr_action_ref_array );
    
    for( unsigned int a = 0; a < num_actions; a ++ )
    {
        action ptr_action = action_ref_array_item_at(
            ptr_action_ref_array, a );
            
        switch( (enum_action_type) action_type( ptr_action ) )
        {
            case action_type_push_region:  
            case action_type_pop_region:   
            case action_type_play_region:  
            case action_type_start_layer:  
            case action_type_stop_layer:
                music_engine_do_action( & p_performance->music_engine, ptr_action );
                break;   
            case action_type_rand_state:
                _do_action_rand_state( p_performance, (rand_state_action) (pointer) ptr_action );
                break;
            default:
                rAssert( false );
        };
    }
}


}