//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "pch.hpp"
#include <radmusic/radmusic.hpp>
#include <radmusic/sequence/sequence_region.hpp>
#include <radmusic/schema/schema_util.hpp>
#include <memory/memory.hpp>

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
    
struct layer_controller
{
    sequence_player * p_sequence_player;
    bool triggered;    
};

void region_controller_construct(
    struct_region_controller * p_rc,
    resource_manager * p_rm,
    const region & ptr_region )
{
    pointer_construct( (pointer*) & p_rc->ptr_region );
    
    p_rc->ptr_region = ptr_region;
    p_rc->current_resource_buffer = 0x0000FFFF;
    p_rc->last_beat = 0.0f;
    p_rc->volume = 0.0f;
    p_rc->has_been_played = false;

    p_rc->p_layer_controllers = (layer_controller*)
        memory::malloc(
            sizeof( layer_controller ) *
                layer_ref_array_num_items(
                    region_layer_refs( ptr_region ) ), "struct_region_controller sequence players" );
    
    for( unsigned int t = 0; t < layer_ref_array_num_items( region_layer_refs( p_rc->ptr_region ) ); t ++ )
    {
        layer    ptr_layer    = layer_ref_array_item_at( region_layer_refs( ptr_region ), t );
        sequence ptr_sequence = layer_sequence_ref( ptr_layer );

        char layer_nom[ 256 ];
        char region_nom[ 256 ];
        
        layer_name(  ptr_layer, layer_nom, 256 );
        region_name( ptr_region, region_nom, 256 );
        
        char sequence_player_name[ 256 ];
        sprintf( sequence_player_name, "struct_region_controller: [%s] layer: [%s]", region_nom, layer_nom );
        
        p_rc->p_layer_controllers[ t ].p_sequence_player = sequence_player_new(
            ptr_sequence,
            p_rm,
            sequence_player_name );
       
        p_rc->p_layer_controllers[ t ].triggered = false;
     
    }
}

void region_controller_destroy( struct_region_controller * p_rc )
{
    for( unsigned int s = 0; s < layer_ref_array_num_items( region_layer_refs( p_rc->ptr_region ) ); s ++ )
    {
        sequence_player_delete( & p_rc->p_layer_controllers[ s ].p_sequence_player );
    }

    memory::free( p_rc->p_layer_controllers );
    
    pointer_destroy( (pointer*) & p_rc->ptr_region );
}

region region_controller_region( struct_region_controller * p_rc )
{
    return p_rc->ptr_region;
}

void region_controller_start_layer( struct_region_controller * p_rc, const char * p_layer_name )
{ 
    layer_ref_array ptr_layer_ref_array = region_layer_refs( p_rc->ptr_region );
       
    for( unsigned int s = 0; s < layer_ref_array_num_items( ptr_layer_ref_array ); s ++ )
    {
        layer ptr_layer = layer_ref_array_item_at( ptr_layer_ref_array, s );
        
        if ( false == layer_constant( ptr_layer ) )
        {
            char layer_nom[ 256 ];
            layer_name( ptr_layer, layer_nom, 256 );
         
            if ( strcmp( layer_nom, p_layer_name ) == 0 )
            {
                p_rc->p_layer_controllers[ s ].triggered = true;
            }
        }
    }
}

void region_controller_stop_layer( struct_region_controller * p_rc, const char * p_layer_name )
{ 
    layer_ref_array ptr_layer_ref_array = region_layer_refs( p_rc->ptr_region );
       
    for( unsigned int s = 0; s < layer_ref_array_num_items( ptr_layer_ref_array ); s ++ )
    {
        layer ptr_layer = layer_ref_array_item_at( ptr_layer_ref_array, s );
        
        if ( false == layer_constant( ptr_layer ) )
        {
            char layer_nom[ 256 ];
            layer_name( ptr_layer, layer_nom, 256 );
             
            if ( strcmp( layer_nom, p_layer_name ) == 0 )
            {
                p_rc->p_layer_controllers[ s ].triggered = false;
                
                sequence_player * p_sp = p_rc->p_layer_controllers[ s ].p_sequence_player;
                
                if ( sequence_player_state( p_sp ) != sequence_player_state_idle )
                {   
                    sequence_player_dequeue( p_sp, 0 );
                    sequence_player_queue( p_sp, p_rc->current_resource_buffer, 0, true );
                }
            }
        }
    }
}

void region_controller_queue_stream(
    struct_region_controller * p_rc,
    stream_graph * p_sg,
    bool reset_state,
    float volume,
    float pitch )
{
    if ( p_rc->has_been_played == false )
    {
        reset_state = true;
        p_rc->has_been_played = true;
    }
       
    if ( layer_ref_array_num_items( region_layer_refs( p_rc->ptr_region ) ) > 0 )
    {
        sequence_player_queue(
            p_rc->p_layer_controllers[ 0 ].p_sequence_player,
            p_rc->current_resource_buffer, p_sg,
            reset_state );    
    }
           
    for( unsigned i = 1; i < layer_ref_array_num_items( region_layer_refs( p_rc->ptr_region ) ); i ++ )
    {
        sequence_player_queue(
            p_rc->p_layer_controllers[ i ].p_sequence_player,
            p_rc->current_resource_buffer,
            0,
            reset_state );
    }
    
    if ( reset_state == true )
    {
        p_rc->last_beat = 0.0f;
    }
    
    region_controller_volume( p_rc, volume );
    region_controller_pitch( p_rc, pitch );
}

void region_controller_kill( struct_region_controller * p_rc )
{
    for ( unsigned int l = 0; l < layer_ref_array_num_items( region_layer_refs( p_rc->ptr_region ) ); l ++ )
    {
        sequence_player * p_sp = p_rc->p_layer_controllers[ l ].p_sequence_player;
        
        if ( sequence_player_state( p_sp ) != sequence_player_state_idle )
        {
            sequence_player_dequeue( p_sp, 0 );
        }
    }
}

void region_controller_paused( struct_region_controller * p_rc, bool pause )
{
    for( unsigned int t = 0; t < layer_ref_array_num_items( region_layer_refs( p_rc->ptr_region ) ); t ++ )
    {
        sequence_player * p_sp = p_rc->p_layer_controllers[ t ].p_sequence_player;    
        
        sequence_player_paused( p_sp, pause );
    }
}

void region_controller_play( struct_region_controller * p_rc )
{
    layer_ref_array ptr_layer_ref_array = region_layer_refs( p_rc->ptr_region );
    
    for( unsigned int t = 0; t < layer_ref_array_num_items( ptr_layer_ref_array ); t ++ )
    {       
        layer ptr_layer = layer_ref_array_item_at( ptr_layer_ref_array, t );
        
        bool constant = layer_constant( ptr_layer );
             
        if ( constant == true )
        {
            sequence_player * p_sp = p_rc->p_layer_controllers[ t ].p_sequence_player;
                
            if ( sequence_player_state( p_sp ) == sequence_player_state_queued )
            {
                sequence_player_play( p_sp );
            }
        }
    }
}

void region_controller_update( struct_region_controller * p_rc )
{   
    float this_beat;
    
    if ( layer_ref_array_num_items( region_layer_refs( p_rc->ptr_region ) ) > 0 )
    {
        sequence_player * p_sp = p_rc->p_layer_controllers[ 0 ].p_sequence_player;

        sequence_player_update( p_sp );
        sequence_player_beat( p_sp, & this_beat );
    }
    
    for ( unsigned int l = 1; l < layer_ref_array_num_items( region_layer_refs( p_rc->ptr_region ) ); l ++ )
    {
        layer_controller * p_lc = & ( p_rc->p_layer_controllers[ l ] );
        sequence_player * p_sp = p_rc->p_layer_controllers[ l ].p_sequence_player;
            
        sequence_player_update( p_sp );
                    
        layer_ref_array ptr_layer_ref_array = region_layer_refs( p_rc->ptr_region );
        layer ptr_layer = layer_ref_array_item_at( ptr_layer_ref_array, l );
        
        if ( false == layer_constant( ptr_layer ) )
        {
            if ( sequence_player_state( p_sp ) == sequence_player_state_finished )
            {
                sequence_player_dequeue( p_sp, 0 );
                sequence_player_queue( p_sp, p_rc->current_resource_buffer, 0, true );
                
                p_lc->triggered = false;
            }
            
            if ( sequence_player_state( p_sp ) == sequence_player_state_queued )
            {            
                if ( p_lc->triggered == true )
                {
                    beat_set ptr_beat_set = layer_beat_set( ptr_layer );

                    if ( beat_set_beat_in_range( ptr_beat_set, p_rc->last_beat, this_beat ) )
                    {
                        sequence_player_play( p_sp );
                    }
                }
            }
        }
    }
        
    p_rc->last_beat = this_beat;  
}

void region_controller_volume( struct_region_controller * p_rc, float volume )
{
    p_rc->volume = volume;

    layer_ref_array ptr_layer_ref_array = region_layer_refs( p_rc->ptr_region );
    
    for ( unsigned int l = 0; l < layer_ref_array_num_items( ptr_layer_ref_array ); l ++ )
    {
        layer ptr_layer = layer_ref_array_item_at( ptr_layer_ref_array, l );
        
        float vol = layer_volume( ptr_layer ) *
                    p_rc->volume *
                    region_volume( p_rc->ptr_region );
                    
        sequence_player * p_sp = p_rc->p_layer_controllers[ l ].p_sequence_player;                 
        
        sequence_player_volume(
            p_sp,
            vol );
    } 
}

float region_controller_volume( struct_region_controller * p_rc )
{
    return p_rc->volume;
}

void region_controller_pitch( struct_region_controller * p_rc, float pitch )
{
    for ( unsigned int l = 0; l < layer_ref_array_num_items( region_layer_refs( p_rc->ptr_region ) ); l ++ )
    {
        sequence_player * p_sp = p_rc->p_layer_controllers[ l ].p_sequence_player;
            
        sequence_player_pitch( p_sp, pitch );
    } 
}

void region_controller_dequeue_stream( struct_region_controller * p_rc, stream_graph ** pp_sg )
{
    // revisit
    
    unsigned int num_layers = layer_ref_array_num_items( region_layer_refs( p_rc->ptr_region ) );
    
    if ( num_layers > 0 )
    {
        sequence_player * p_sp = p_rc->p_layer_controllers[ 0 ].p_sequence_player; 
            
        sequence_player_dequeue( p_sp, pp_sg );

        for( unsigned int s = 1; s < layer_ref_array_num_items( region_layer_refs( p_rc->ptr_region ) ); s ++ )
        {
            layer_controller * p_lc = &(p_rc->p_layer_controllers[ s ]);
            
            p_lc->triggered = false;
            
            sequence_player * p_sp = p_lc->p_sequence_player; 
                
            sequence_player_dequeue( p_sp, 0 );
        }
    }
}

void region_controller_resource_buffer( struct_region_controller * p_rc, unsigned int buffer )
{
    p_rc->current_resource_buffer = buffer;
}

unsigned int region_controller_resource_buffer( struct_region_controller * p_rc )
{
    return p_rc->current_resource_buffer;
}

sequence_player * region_controller_primary_sequence( struct_region_controller * p_rc )
{
    sequence_player * p_sp = p_rc->p_layer_controllers[ 0 ].p_sequence_player; 
        
    return p_sp;
}

bool region_controller_is_queued( struct_region_controller * p_rc )
{
    for ( unsigned int s = 0; s < layer_ref_array_num_items( region_layer_refs( p_rc->ptr_region ) ); s ++ )
    {
        sequence_player * p_sp = p_rc->p_layer_controllers[ s ].p_sequence_player;
            
        enum_sequence_player_state state = sequence_player_state( p_sp );
                
        if ( state != sequence_player_state_queued && state != sequence_player_state_finished )
        {
            return false;
        }
    }

    return true;
}

    
}




