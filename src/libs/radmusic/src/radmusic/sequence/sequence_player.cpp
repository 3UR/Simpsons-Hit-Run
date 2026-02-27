//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
/// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "pch.hpp"
#include <radmusic/sequence/sequence_player.hpp>
#include <radmusic/schema/schema_util.hpp>
#include <memory/memory.hpp>

//-----------------------------------------------------------------------------
// Local Constants, Typedefs, and Macros
//-----------------------------------------------------------------------------

using namespace ods;

namespace radmusic
{
    const unsigned int NULL_SAMPLE = 0xFFFFFFFF;
}

//-----------------------------------------------------------------------------
// Global Data, Local Data, Local Classes [Optional commment can be deleted]
//-----------------------------------------------------------------------------

namespace radmusic
{       

inline unsigned char vol_f_to_c( float f )
{
    return (unsigned char) radSoundFloatToUInt( f * 255.0f );
}

inline float vol_c_to_f( unsigned char c )
{
    return radSoundUIntToFloat( (unsigned int) c ) / 255.0f;
}

inline unsigned short pitch_f_to_s( float f )
{
    return (unsigned short) radSoundFloatToUInt( f * 6553.0f );
}

inline float pitch_s_to_f( unsigned short c )
{
    return radSoundUIntToFloat( (unsigned short) c ) / 6553.0f;
}


static void             _clip_player_volume( sequence_player * );
static void             _stream_graph_volume( sequence_player * );
static void             _clip_player_pitch( sequence_player * );
static void             _stream_graph_pitch( sequence_player * );
static void             _clip_player_aux( sequence_player * );
static void             _stream_graph_aux( sequence_player * );
static void             _reset( sequence_player * );
static void             _reset_variables( sequence_player * );
static void             _randomize_positional_settings( sequence_player * p_sp );
static void             _randomize_volume( sequence_player * p_sp );
static void             _randomize_pitch( sequence_player * p_sp );
static void             _randomize_clip_player( sequence_player * p_sp );
static void             _randomize_stream_player( sequence_player * p_sp );

static void               _command_stack_push( sequence_player *, const sequence_event & ptr_sequence_event );
static sequence_player_command *          _command_stack_peek( sequence_player * );
static void               _command_stack_pop( sequence_player * );
static void               _command_stack_reset( sequence_player * );
static command_stack_type _command_stack_items( sequence_player* );

static event_queue_type _event_queue_wrap( event_queue_type i );

static void              _event_queue_enqueue( sequence_player *, sequence_player_event * p_e );
static sequence_player_event *           _event_queue_peek( sequence_player * );
static void              _event_queue_dequeue( sequence_player * );
static bool              _event_queue_full( sequence_player * );
static void              _event_queue_reset( sequence_player * );
static bool              _event_queue_empty( sequence_player * );
static event_queue_type  _event_queue_items( sequence_player * );
static sequence_player_event *           _event_queue_item_at( sequence_player *, event_queue_type );


static void              _event_queue_window_enqueue( sequence_player * );
static void              _event_queue_window_dequeue( sequence_player * );
static sequence_player_event *           _event_queue_window_peek( sequence_player * );
static event_queue_type  _event_queue_window_unqueued_items( sequence_player * );
static event_queue_type  _event_queue_window_queued_items( sequence_player * );

static void             _update_command_stack( sequence_player * );
static void             _update_event_queue( sequence_player * );        

static void             _command_clip( sequence_player *, sequence_player_command * p_c );
static void             _command_silence( sequence_player *, sequence_player_command * p_c );
static void             _command_logic_and( sequence_player *, sequence_player_command * p_c );
static void             _command_logic_or( sequence_player *, sequence_player_command * p_c );
static void             _command_logic_repeat( sequence_player *, sequence_player_command * p_c );
static void             _command_stream( sequence_player *, sequence_player_command * p_c );
static void             _command_var( sequence_player *, sequence_player_command * p_c );

static void             _command_callback( sequence_player *, sequence_player_command * p_c );

static bool             _event_clip( sequence_player *, sequence_player_event * p_e );
static bool             _event_stream( sequence_player *, sequence_player_event * p_e );
static bool             _event_silence( sequence_player *, sequence_player_event * p_e );
static bool             _event_var_volume( sequence_player *, sequence_player_event * p_e );
static bool             _event_var_pitch( sequence_player *, sequence_player_event * p_e );
static bool             _event_var_volume_rand_min( sequence_player *, sequence_player_event * p_e );
static bool             _event_var_volume_rand_max( sequence_player *, sequence_player_event * p_e );
static bool             _event_var_pitch_rand_min( sequence_player *, sequence_player_event * p_e );
static bool             _event_var_pitch_rand_max( sequence_player *, sequence_player_event * p_e );
static bool             _event_var_aux_gain( sequence_player *, sequence_player_event * p_e );
static bool             _event_var_positional( sequence_player *, sequence_player_event * p_e );
static bool             _event_var_pos_fall_off( sequence_player *, sequence_player_event * p_e );
static bool             _event_var_pos_dist_min( sequence_player *, sequence_player_event * p_e );
static bool             _event_var_pos_dist_max( sequence_player *, sequence_player_event * p_e );
static bool             _event_var_callback( sequence_player *, sequence_player_event * p_e );

static void             _event_reset_clip( sequence_player *, sequence_player_event * p_e );
static void             _event_reset_stream( sequence_player *, sequence_player_event * p_e );
static void             _event_reset_silence( sequence_player *, sequence_player_event * p_e );

static unsigned int     _event_remaining_time_clip( sequence_player *, sequence_player_event * p_e );
static unsigned int     _event_remaining_time_stream( sequence_player *, sequence_player_event * p_e );
static unsigned int     _event_remaining_time_silence( sequence_player *, sequence_player_event * p_e );

static void             _event_dump_clip( sequence_player *, sequence_player_event * p_e );
static void             _event_dump_stream( sequence_player *, sequence_player_event * p_e );
static void             _event_dump_silence( sequence_player *, sequence_player_event * p_e );
static void             _event_dump_var_volume( sequence_player *, sequence_player_event * p_e );
static void             _event_dump_var_pitch( sequence_player *, sequence_player_event * p_e );
static void             _event_dump_var_volume_rand_min( sequence_player *, sequence_player_event * p_e );
static void             _event_dump_var_volume_rand_max( sequence_player *, sequence_player_event * p_e );
static void             _event_dump_var_pitch_rand_min( sequence_player *, sequence_player_event * p_e );
static void             _event_dump_var_pitch_rand_max( sequence_player *, sequence_player_event * p_e );
static void             _event_dump_var_aux_gain( sequence_player *, sequence_player_event * p_e );
static void             _event_dump_var_positional( sequence_player *, sequence_player_event * p_e );
static void             _event_dump_var_pos_fall_off( sequence_player *, sequence_player_event * p_e );
static void             _event_dump_var_pos_dist_min( sequence_player *, sequence_player_event * p_e );
static void             _event_dump_var_pos_dist_max( sequence_player *, sequence_player_event * p_e );
static void             _event_dump_var_callback( sequence_player *, sequence_player_event * p_e );

typedef bool         (event_handler_fx)( sequence_player *, sequence_player_event * p_e );
typedef void         (event_dump_fx)( sequence_player *, sequence_player_event * p_e );
typedef void         (event_reset_fx)( sequence_player *, sequence_player_event * p_e );
typedef unsigned int (event_remaining_time_fx)( sequence_player *, sequence_player_event * p_e );
typedef void         (command_handler_fx)(sequence_player *, sequence_player_command * p_c );

struct event_handler
{
    command_handler_fx      * p_command_handler_fx;
    event_handler_fx        * p_handler_fx;
    event_dump_fx           * p_dump_fx;
    event_reset_fx          * p_reset_fx;
    event_remaining_time_fx * p_remaining_time_fx;
};

event_handler g_p_event_handlers[ sequence_event_max ] =
{
    /* ods::clip       */             { _command_clip,           _event_clip,                _event_dump_clip,                   _event_reset_clip,      _event_remaining_time_clip      },
    /* ods::stream     */             { _command_stream,         _event_stream,              _event_dump_stream,                 _event_reset_stream,    _event_remaining_time_stream    },
    /* silence    */             { _command_silence,        _event_silence,             _event_dump_silence,                _event_reset_silence,   _event_remaining_time_silence   },
    /* var_volume */             { _command_var,            _event_var_volume,          _event_dump_var_volume,             0,                      0,                              },
    /* var_pitch  */             { _command_var,            _event_var_pitch,           _event_dump_var_pitch,              0,                      0,                              },
    /* var_volume_rand_min */    { _command_var,            _event_var_volume_rand_min, _event_dump_var_volume_rand_min,    0,                      0,                              },
    /* var_volume_rand_max */    { _command_var,            _event_var_volume_rand_max, _event_dump_var_volume_rand_max,    0,                      0,                              },
    /* var_pitch_rand_min */     { _command_var,            _event_var_pitch_rand_min,  _event_dump_var_pitch_rand_min,     0,                      0,                              },
    /* var_pitch_rand_max */     { _command_var,            _event_var_pitch_rand_max,  _event_dump_var_pitch_rand_max,     0,                      0,                              },
    /* var_aux_gain */           { _command_var,            _event_var_aux_gain,        _event_dump_var_aux_gain,           0,                      0,                              },
    /* var_positional */         { _command_var,            _event_var_positional,      _event_dump_var_positional,         0,                      0,                              },
    /* var_pos_fall_off */       { _command_var,            _event_var_pos_fall_off,    _event_dump_var_pos_fall_off,       0,                      0,                              },
    /* var_pos_dist_min */       { _command_var,            _event_var_pos_dist_min,    _event_dump_var_pos_dist_min,       0,                      0,                              },
    /* var_pos_dist_max */       { _command_var,            _event_var_pos_dist_max,    _event_dump_var_pos_dist_max,       0,                      0,                              },
    /* callback */               { _command_var,            _event_var_callback,        _event_dump_var_callback,           0,                      0,                              },
    /* logic_and */              { _command_logic_and,      0,                          0,                                  0,                      0,                              },
    /* logic_or */               { _command_logic_or,       0,                          0,                                  0,                      0,                              },
    /* logic_repeat */           { _command_logic_repeat,   0,                          0,                                  0,                      0,                              }
};

}

//-----------------------------------------------------------------------------
// Member Functions
//-----------------------------------------------------------------------------

namespace radmusic
{

//============================================================================
//
// radmusic sequence_player public functions
//
//============================================================================

//----------------------------------------------------------------------------

sequence_player * sequence_player_new(
    sequence & ptr_sequence,
    resource_manager * p_rm,
    const char * name )
{
    sequence_player * p_sp = (sequence_player*)
        memory::malloc(
            sizeof( sequence_player ),
            name );
            
    pointer_construct( (pointer*) & p_sp->ptr_sequence );
    
    p_sp->ptr_sequence = ptr_sequence;
    p_sp->p_resource_manager = p_rm;   
    p_sp->master_volume         = vol_f_to_c( 1.0f );
    p_sp->master_pitch          = vol_f_to_c( 1.0f );
    
    p_sp->resource_buffer       = NULL_RESOURCE_BUFFER;
    p_sp->state                 = sequence_player_state_idle;
    p_sp->paused                = false;
    
    #ifdef SEQUENCE_PLAYER_NAMES          
    strncpy( p_sp->name, name, g_sequence_player_name_chars );
    #endif    
   
    p_sp->p_clip_player               = 0;
    p_sp->p_stream_graph              = 0;
    p_sp->p_positional_group          = 0;
    
    p_sp->p_command_stack = (sequence_player_command*) memory::malloc(
        sizeof( sequence_player_command) * sequence_stack_size( p_sp->ptr_sequence ),
        "sequence player command stack" );
        
    for( unsigned int i = 0; i < sequence_stack_size( p_sp->ptr_sequence ); i ++ )
    {
        pointer_construct( (pointer*) & p_sp->p_command_stack[ i ].ptr_sequence_event );
    }
    
    for( unsigned int i = 0; i < g_event_queue_size; i ++ )
    {
        pointer_construct( (pointer*) & p_sp->event_queue[ i ].ptr_sequence_event );
    }
        
    _reset( p_sp );
    
    return p_sp;
}

//----------------------------------------------------------------------------

void sequence_player_delete( sequence_player ** pp_sp )
{
    sequence_player * p_sp = *pp_sp;
    
    for( unsigned int i = 0; i < sequence_stack_size( p_sp->ptr_sequence ); i ++ )
    {
        pointer_destroy( (pointer*) & p_sp->p_command_stack[ i ].ptr_sequence_event );
    }
    
    for( unsigned int i = 0; i < g_event_queue_size; i ++ )
    {
        pointer_destroy( (pointer*) & p_sp->event_queue[ i ].ptr_sequence_event );
    }
    
    if ( p_sp->p_clip_player != 0 )
    {   
        release_clip_player( & p_sp->p_clip_player );
    }
    
    if ( p_sp->p_stream_graph != 0 )
    {
        release_stream_graph( & p_sp->p_stream_graph );
    }
    
    if ( p_sp->p_positional_group != 0 )
    {
        p_sp->p_positional_group->Release( );
    }
    
    memory::free( p_sp->p_command_stack );
    
    p_sp->ptr_sequence = pointer_null;
    
    memory::free( p_sp );
    
    *pp_sp = 0;
}

//----------------------------------------------------------------------------

enum_sequence_player_state sequence_player_state( sequence_player * p_sp )
{
    return p_sp->state;
}

//----------------------------------------------------------------------------

void sequence_player_paused( sequence_player * p_sp, bool pause )
{
    if ( pause != p_sp->paused )
    {
        p_sp->paused = pause;
        
        if ( p_sp->paused == true )
        {
            if ( p_sp->state == sequence_player_state_playing ||
                 p_sp->state == sequence_player_state_finishing )
            {
                if( p_sp->p_clip_player != 0 )
                {
                    p_sp->p_clip_player->Stop( );
                }
                
                if ( p_sp->p_stream_graph != 0 )
                {
                    stream_graph_paused( p_sp->p_stream_graph, true );
                }
            }
        }
        else
        {
            if ( p_sp->state == sequence_player_state_playing ||
                 p_sp->state == sequence_player_state_finishing )            
             {
                if ( p_sp->p_clip_player != 0 )
                {
                    p_sp->p_clip_player->Play( );
                }
                
                if ( p_sp->p_stream_graph != 0 )
                {
                    stream_graph_paused( p_sp->p_stream_graph, false );           
                }
            }
       }
    }
}

//----------------------------------------------------------------------------

void sequence_player_queue(
    sequence_player * p_sp,
    unsigned int resource_buffer,
    stream_graph * p_sg,
    bool reset_state )
{    
    rAssert( p_sp->state == sequence_player_state_idle );
    
    // revisit ods::clip player pool
    
    p_sp->p_positional_group = ::radSoundHalPositionalGroupCreate( memory::allocator( ) );
    p_sp->p_positional_group->AddRef( ); 
    
    p_sp->p_stream_graph = p_sg;   

    p_sp->state = sequence_player_state_queueing;

    p_sp->resource_buffer = resource_buffer;
    
    if ( reset_state == true )
    {
        _reset( p_sp );
        
        sequence_event ptr_root_event = sequence_root_event( p_sp->ptr_sequence );
        
        if ( not_null( ptr_root_event ) )
        {
            _command_stack_push( p_sp, ptr_root_event );
        }
    }
    
    sequence_player_update( p_sp );
}

//----------------------------------------------------------------------------

void sequence_player_dequeue( sequence_player * p_sp, stream_graph ** pp_sg )
{       
    rAssert( p_sp->state != sequence_player_state_idle );
    
    if ( pp_sg != 0 )
    {
        *pp_sg = 0;
    }
    
    if ( p_sp->p_clip_player != 0 )
    {
        release_clip_player( & p_sp->p_clip_player );
    }
    
    if ( p_sp->p_stream_graph != 0 )
    {
        if ( pp_sg == 0 )
        {
            stream_graph_dequeue( p_sp->p_stream_graph );
            release_stream_graph( & p_sp->p_stream_graph );
        }
        else
        {
            stream_graph_queue_flush( p_sp->p_stream_graph );
            *pp_sg = p_sp->p_stream_graph;
            p_sp->p_stream_graph = 0;
        } 
    } 
    
    if ( p_sp->p_positional_group != 0 )
    {
        p_sp->p_positional_group->Release( );
        p_sp->p_positional_group = 0;
    }  
              

    p_sp->resource_buffer = NULL_RESOURCE_BUFFER;        
    p_sp->state = sequence_player_state_idle;   
   
    // reset queue window to queue head.
   
    while( _event_queue_window_queued_items( p_sp ) > 0 )
    {
        _event_queue_window_dequeue( p_sp );
        
        sequence_player_event * p_e = _event_queue_window_peek( p_sp );
        
        enum_sequence_event_type id = (enum_sequence_event_type) sequence_event_type( p_e->ptr_sequence_event  );
        
        if ( g_p_event_handlers[ id ].p_reset_fx != 0 )
        {
            g_p_event_handlers[ id ].p_reset_fx( p_sp, p_e );
        }                
    }
}

//----------------------------------------------------------------------------
   
void sequence_player_beat(
    sequence_player * p_sp,
    float * p_beat )
{
    sequence_player_update( p_sp );
    
    *p_beat = p_sp->beat;
}

//----------------------------------------------------------------------------

unsigned int sequence_player_remaining_time( sequence_player * p_sp )
{
    command_stack_type cs_items = _command_stack_items( p_sp );
    
    if ( cs_items > 0 )
    {
        return 0xFFFFFFFF;
    }
    else
    {

        event_queue_type    eq_items = _event_queue_items( p_sp );
    
        unsigned int rt = 0;
                
        for( unsigned int i =0; i < eq_items; i ++ )
        {
            sequence_player_event * p_e = _event_queue_item_at( p_sp, i );
            
            enum_sequence_event_type id = (enum_sequence_event_type) sequence_event_type( p_e->ptr_sequence_event );
            
            if ( g_p_event_handlers[ id ].p_remaining_time_fx != 0 )
            {
                rt += g_p_event_handlers[ id ].p_remaining_time_fx( p_sp, p_e );
            }
        }
        
        return rt;
    }
}

//----------------------------------------------------------------------------

void sequence_player_volume( sequence_player * p_sp, float volume )
{
    p_sp->master_volume = vol_f_to_c( volume );
    
    if ( p_sp->state != sequence_player_state_idle )
    {
        if ( p_sp->p_clip_player != 0 )
        {
            _clip_player_volume( p_sp );
        }
        
        if ( p_sp->p_stream_graph != 0 )
        {
            _stream_graph_volume( p_sp );
        }
    }
}

//----------------------------------------------------------------------------

void sequence_player_pitch( sequence_player * p_sp, float pitch )
{
    p_sp->master_pitch = pitch_f_to_s( pitch );
        
    if ( p_sp->state != sequence_player_state_idle )
    {
        if ( p_sp->p_clip_player != 0 )
        {
            _clip_player_pitch( p_sp );
        }
        
        if ( p_sp->p_stream_graph != 0 )
        {
            _stream_graph_pitch( p_sp );
        }
    }
}

//----------------------------------------------------------------------------

void sequence_player_play( sequence_player * p_sp )
{
    rAssert( p_sp->state == sequence_player_state_queued );
    
    p_sp->state = sequence_player_state_playing;
}

//============================================================================
//
// radmusic sequence_player private functions
//
//============================================================================

//----------------------------------------------------------------------------

void _reset( sequence_player * p_sp )
{
    _reset_variables( p_sp );
    _event_queue_reset( p_sp );
    _command_stack_reset( p_sp );
}

//----------------------------------------------------------------------------

void _reset_variables( sequence_player * p_sp )
{
    p_sp->volume_rand_current   = vol_f_to_c( 1.0f );
    p_sp->pitch_rand_current    = pitch_f_to_s( 1.0f );
    
    p_sp->var_volume            = vol_f_to_c( 1.0f );
    p_sp->var_pitch             = pitch_f_to_s( 1.0f );
    p_sp->var_volume_rand_min   = vol_f_to_c( 1.0f );
    p_sp->var_volume_rand_max   = vol_f_to_c( 1.0f );
    p_sp->var_pitch_rand_min    = pitch_f_to_s( 1.0f );
    p_sp->var_pitch_rand_max    = pitch_f_to_s( 1.0f );
    p_sp->var_aux_gains[ 0 ]    = vol_f_to_c( 0.0f );
    p_sp->var_aux_gains[ 1 ]    = vol_f_to_c( 0.0f );
    p_sp->var_positional = false;
    p_sp->var_pos_fall_off = 100.0f;
    p_sp->var_pos_dist_min = 0.0f;
    p_sp->var_pos_dist_max = 50.0f;
    p_sp->beat = 0.0f;
}

//----------------------------------------------------------------------------

void _clip_player_volume( sequence_player * p_sp )
{
    float vol =
        vol_c_to_f( p_sp->var_volume ) *
        vol_c_to_f( p_sp->master_volume ) *
        vol_c_to_f( p_sp->volume_rand_current );
        
    p_sp->p_clip_player->SetVolume( vol );
}

//----------------------------------------------------------------------------

void _clip_player_pitch( sequence_player * p_sp )
{
    float pitch =
        pitch_s_to_f( p_sp->var_pitch ) *
        pitch_s_to_f( p_sp->master_pitch ) *
        pitch_s_to_f( p_sp->pitch_rand_current );
    
    p_sp->p_clip_player->SetPitch( pitch );
}

//----------------------------------------------------------------------------

void _clip_player_aux( sequence_player * p_sp )
{
    unsigned int num_system_sends = radSoundHalSystemGet( )->GetNumAuxSends( );
    bool environmental_effects_enabled = radSoundHalListenerGet( )->GetEnvEffectsEnabled( );
   
    for( unsigned int s = 0; (s < 2) && (s < num_system_sends); s ++ )
    {
        if ( environmental_effects_enabled == true )
        {
            unsigned int environmental_effects_send = radSoundHalListenerGet( )->GetEnvironmentAuxSend( );
                
            if ( s != environmental_effects_send )
            {
                p_sp->p_clip_player->SetAuxGain( s, p_sp->var_aux_gains[ s ] );
            }
        }
        else
        {
            p_sp->p_clip_player->SetAuxGain( s, p_sp->var_aux_gains[ s ] );
        }       
    }
};

//----------------------------------------------------------------------------

void _stream_graph_volume( sequence_player * p_sp )
{
    float vol =
        vol_c_to_f( p_sp->var_volume ) *
        vol_c_to_f( p_sp->master_volume ) *
        vol_c_to_f( p_sp->volume_rand_current );
    
    stream_graph_volume( p_sp->p_stream_graph, vol );
}

//----------------------------------------------------------------------------

void _stream_graph_pitch( sequence_player * p_sp )
{
    float pitch =
        pitch_s_to_f( p_sp->var_pitch ) *
        pitch_s_to_f( p_sp->master_pitch ) *
        pitch_s_to_f( p_sp->pitch_rand_current );
    
    stream_graph_pitch( p_sp->p_stream_graph, pitch );
}

//----------------------------------------------------------------------------

void _stream_graph_aux( sequence_player * p_sp )
{
    unsigned int num_system_sends = radSoundHalSystemGet( )->GetNumAuxSends( );
    unsigned int environmental_effects_enabled = radSoundHalListenerGet( )->GetEnvEffectsEnabled( );
    
    for( unsigned int s = 0; (s < 2) && (s < num_system_sends); s ++ )
    {
        unsigned int environmental_effects_send = radSoundHalListenerGet( )->GetEnvironmentAuxSend( );
        
        if ( s != environmental_effects_send )
        {
            stream_graph_aux( p_sp->p_stream_graph, s, p_sp->var_aux_gains[ s ] );
        }
    }
};

//----------------------------------------------------------------------------

static void _randomize_stream_player( sequence_player * p_sp )
{
    _randomize_volume( p_sp );
    _randomize_pitch( p_sp );
    
    _stream_graph_volume( p_sp );
    _stream_graph_pitch( p_sp );
    _stream_graph_aux( p_sp );
    
    if ( p_sp->var_positional == true )
    {
        _randomize_positional_settings( p_sp );
        _randomize_volume( p_sp );
        _randomize_pitch( p_sp );
        
        if ( stream_graph_format( p_sp->p_stream_graph )->GetNumberOfChannels( ) == 1 )
        {  
            stream_graph_positional_group( p_sp->p_stream_graph, p_sp->p_positional_group );
        }
        else
        {
            rAssertMsg( false, "Positional streams must be mono" );
        }        
    }
    else
    {
        stream_graph_positional_group( p_sp->p_stream_graph, 0 );
    }
}

//----------------------------------------------------------------------------

static void _randomize_clip_player( sequence_player * p_sp )
{
    _randomize_volume( p_sp );
    _randomize_pitch( p_sp );
    
    _clip_player_volume( p_sp );
    _clip_player_pitch( p_sp );
    _clip_player_aux( p_sp );
        
    if ( p_sp->var_positional == true )
    {
        _randomize_positional_settings( p_sp );
        
        if ( p_sp->p_clip_player->GetClip( )->GetFormat( )->GetNumberOfChannels( ) == 1 )
        {
            p_sp->p_clip_player->SetPositionalGroup( p_sp->p_positional_group );
        }
        else
        {
            rAssertMsg( false, "Positional clips must be mono" );
        }
    }
    else
    {
        p_sp->p_clip_player->SetPositionalGroup( 0 );
    }
    
}

//----------------------------------------------------------------------------

static void _randomize_positional_settings( sequence_player * p_sp )
{             
    p_sp->p_clip_player->SetPositionalGroup( p_sp->p_positional_group );
        
    radSoundVector listenerPos;
    ::radSoundHalListenerGet( )->GetPosition( & listenerPos );                    
                                    
    radSoundVector v(
        radSoundRandMinMax( -1.0f, 1.0f ),
        radSoundRandMinMax( -1.0f, 1.0f ),
        0.0f );
            
    v.Normalize( );

    v *= radSoundRandMinMax( p_sp->var_pos_dist_min, p_sp->var_pos_dist_max );

    v = v + listenerPos;
                                        
    p_sp->p_positional_group->SetPosition( & v );
    p_sp->p_positional_group->SetMinMaxDistance( 1.0f, p_sp->var_pos_fall_off );    
}

static void _randomize_volume( sequence_player * p_sp )
{
    p_sp->volume_rand_current = vol_f_to_c(
        radSoundRandMinMax(
            vol_c_to_f( p_sp->var_volume_rand_min ),
            vol_c_to_f( p_sp->var_volume_rand_max ) ) );
}

static void _randomize_pitch( sequence_player * p_sp )
{
    p_sp->pitch_rand_current = pitch_f_to_s( radSoundRandMinMax(
        pitch_s_to_f( p_sp->var_pitch_rand_min ),
        pitch_s_to_f( p_sp->var_pitch_rand_max ) ) );
}

//----------------------------------------------------------------------------

void _update_command_stack( sequence_player * p_sp )
{
    while ( 1 )
    {
        sequence_player_command * p_c = _command_stack_peek( p_sp );
                
        if ( p_c == 0 )
        {
            // no more commands
            
            break;
        }
        
        if ( _event_queue_full( p_sp ) )
        {
            break;
        }
           
        enum_sequence_event_type id = (enum_sequence_event_type) sequence_event_type( p_c->ptr_sequence_event );
        
        g_p_event_handlers[ id ].p_command_handler_fx( p_sp, p_c );
    }
   
}

//----------------------------------------------------------------------------

void sequence_player_update( sequence_player * p_sp )
{
    if ( p_sp->state != sequence_player_state_idle )
    {
        _update_command_stack( p_sp );
        
        if ( p_sp->paused == false )
        {    
            _update_event_queue( p_sp );
        }
    }
}

//----------------------------------------------------------------------------

void _update_event_queue( sequence_player * p_sp )
{
    if ( p_sp->state == sequence_player_state_idle )
    {
        return;
    }
    
    if ( p_sp->state == sequence_player_state_queued )
    {
        return;
    }

    while( true )
    {
        sequence_player_event *p_e_head = _event_queue_peek( p_sp );
    
        if ( p_e_head == 0 )
        {            
            p_sp->state = sequence_player_state_finished;
                
            return;
        }

        if ( _event_queue_window_unqueued_items( p_sp ) == 0 && 
             p_sp->state == sequence_player_state_playing )
        {
            p_sp->state = sequence_player_state_finishing;
        }
            
        enum_sequence_event_type id_head = (enum_sequence_event_type) sequence_event_type( p_e_head->ptr_sequence_event );

        bool event_finished = g_p_event_handlers[ id_head ].p_handler_fx( p_sp, p_e_head );
        
        if ( event_finished == false )
        {
            break;
        }
      
        _event_queue_dequeue( p_sp );
    }        
}

//============================================================================
//
// radmusic sequence_player sequence_player_command handlers
//
//============================================================================

//----------------------------------------------------------------------------

void _command_silence( sequence_player * p_sp, sequence_player_command * p_c )
{
    float        min = event_silence_min_time( (event_silence) p_c->ptr_sequence_event );
    float        max = event_silence_max_time( (event_silence) p_c->ptr_sequence_event );
    
    float random_silence = radSoundRandMinMax( min,max );
    
    sequence_player_event e;
    e.ptr_sequence_event          = p_c->ptr_sequence_event;
    e.silence.time         = random_silence;
    e.silence.current_time = 0;
    e.silence.started      = false;
        
    _event_queue_enqueue( p_sp, & e );
    
    _command_stack_pop( p_sp );
}

//----------------------------------------------------------------------------

void _command_clip( sequence_player * p_sp, sequence_player_command * p_c )
{
    sequence_player_event e;
    e.ptr_sequence_event = p_c->ptr_sequence_event;
    e.clip.started = false;
    e.clip.current_sample = 0;
    
    _event_queue_enqueue( p_sp, & e );
    
    _command_stack_pop( p_sp );    
}

//----------------------------------------------------------------------------

void _command_stream( sequence_player * p_sp, sequence_player_command * p_c )
{
    sequence_player_event e;
    e.ptr_sequence_event = p_c->ptr_sequence_event;
    e.stream.start_sample = NULL_SAMPLE;
    e.stream.queued_sample = NULL_SAMPLE;
    e.stream.current_sample = 0;
    
    _event_queue_enqueue( p_sp, & e );
    
    _command_stack_pop( p_sp );    
}

//----------------------------------------------------------------------------

void _command_logic_and( sequence_player * p_sp, sequence_player_command * p_c )
{
    unsigned int items = sequence_event_ref_array_num_items(
        event_logic_and_events(
            (event_logic_and) p_c->ptr_sequence_event ) );
        
    if ( p_c->enter )
    {
        p_c->and_info.index = 0;
        p_c->enter = false;
    }
    else
    {
        if ( p_c->and_info.index < items )
        {
            sequence_event ptr_sequence_event =
                sequence_event_ref_array_item_at(
                    event_logic_and_events(
                        (event_logic_and) p_c->ptr_sequence_event ),
                p_c->and_info.index );     
            
            _command_stack_push( p_sp, ptr_sequence_event );  
                         
            p_c->and_info.index++;    
        }
        else
        {
            _command_stack_pop( p_sp );
        }    
    }
}

//----------------------------------------------------------------------------

void _command_logic_or( sequence_player * p_sp, sequence_player_command * p_c )
{
    _command_stack_pop( p_sp );
    
    unsigned int items =
        sequence_event_ref_array_num_items(
            event_logic_or_events(
                (event_logic_or) p_c->ptr_sequence_event ) );
    
    if ( items > 0 )
    {
        items = rand() % items;
        
        sequence_event ptr_sequence_event =
            sequence_event_ref_array_item_at(
                event_logic_or_events(
                    (event_logic_or) p_c->ptr_sequence_event ),
            items );                   
       
        _command_stack_push( p_sp, ptr_sequence_event );
    }
}

//----------------------------------------------------------------------------

void _command_logic_repeat( sequence_player * p_sp, sequence_player_command * p_c )
{

    unsigned int min_times =
        event_logic_repeat_min_times( (event_logic_repeat) p_c->ptr_sequence_event );
    unsigned int max_times =
        event_logic_repeat_max_times( (event_logic_repeat) p_c->ptr_sequence_event );
    
    if ( p_c->enter )
    {
        unsigned int times;
                
        if ( min_times == event_logic_repeat_infinite )
        {
            times = event_logic_repeat_infinite;
        }
        else
        {
            times = radSoundRandMinMax(
                min_times,
                max_times );
        }
        
        p_c->repeat_info.times = 0;
        p_c->repeat_info.target_times = times;          
        p_c->enter = false;
    }
    else
    {
        if ( p_c->repeat_info.target_times == event_logic_repeat_infinite )
        {
            _command_stack_push(
                p_sp,
                event_logic_repeat_event_ref(
                    (event_logic_repeat) p_c->ptr_sequence_event ) );
        }
        else
        {        
            if ( p_c->repeat_info.times >= p_c->repeat_info.target_times )
            {
                _command_stack_pop( p_sp );
            }
            else
            {
                p_c->repeat_info.times++; 
                          
                _command_stack_push(
                    p_sp,
                    event_logic_repeat_event_ref(
                        (event_logic_repeat) p_c->ptr_sequence_event ) );
            }
        }
    }
}

//----------------------------------------------------------------------------

void _command_var( sequence_player * p_sp, sequence_player_command * p_c )
{   
    sequence_player_event e;
    e.ptr_sequence_event = p_c->ptr_sequence_event;
    
    _event_queue_enqueue( p_sp, & e );
    
    _command_stack_pop( p_sp );
}

//----------------------------------------------------------------------------

void _command_callback( sequence_player * p_sp, sequence_player_command * p_c )
{
    sequence_player_event e;
    e.ptr_sequence_event = p_c->ptr_sequence_event;
    
    _event_queue_enqueue( p_sp, & e );
    
    _command_stack_pop( p_sp );
}

//============================================================================
//
// sequence_player_event hanling functions
//
//============================================================================

//----------------------------------------------------------------------------

bool _event_clip( sequence_player * p_sp, sequence_player_event * p_e_head )
{
    bool ret = false;
    
    ods::clip ptr_clip = event_clip_clip_ref(
        (event_clip) p_e_head->ptr_sequence_event );
    
    if ( p_sp->p_clip_player == 0 )
    {
        capture_clip_player(  & p_sp->p_clip_player );
        
        p_sp->p_clip_player->SetClip(
            resource_manager_get_clip(
                p_sp->p_resource_manager, ptr_clip, p_sp->resource_buffer ) );        
        
        _randomize_clip_player( p_sp );
        
        p_e_head->clip.started = false;        
        
         _event_queue_window_enqueue( p_sp );        
    }
    else
    {
        if ( p_sp->state == sequence_player_state_queueing )
        {
            p_sp->state = sequence_player_state_queued;
        }
        else if ( p_sp->state == sequence_player_state_playing ||
                p_sp->state == sequence_player_state_finishing )
        {
            if ( p_e_head->clip.started == false )
            {
                p_sp->p_clip_player->SetPlaybackPosition(
                    p_e_head->clip.current_sample,
                    IRadSoundHalAudioFormat::Samples );            
                    
                p_sp->p_clip_player->Play( );
                p_e_head->clip.started = true;
            }
            else
            {
                if ( p_sp->p_clip_player->GetState( ) == IRadSoundClipPlayer::Stopped )
                {
                    ret = true;
                    release_clip_player( & p_sp->p_clip_player );                     
                }
                else
                {
                    p_e_head->clip.current_sample = p_sp->p_clip_player->GetPlaybackTimeInSamples( );
                }
            }          
            
            p_sp->beat = tempo_track_beat(
                clip_tempo_track( ptr_clip ),
                p_e_head->clip.current_sample,
                audio_format_sampling_rate(
                    rsd_file_audio_format(
                        clip_rsd_file_ref( ptr_clip ) ) ) );
                            
        }
        else
        {
            rAssert( p_sp->state == sequence_player_state_queued );
        }        
    }
    
    return ret;

}

//----------------------------------------------------------------------------

bool _event_stream( sequence_player * p_sp, sequence_player_event * p_e_head )
{  
    bool ret = false;
    bool eos;
      
    if ( _event_queue_window_unqueued_items( p_sp ) > 0 )
    {
        sequence_player_event * p_e_window = _event_queue_window_peek( p_sp );
        
        enum_sequence_event_type id_window = (enum_sequence_event_type) sequence_event_type( p_e_window->ptr_sequence_event );
        
        if ( id_window == sequence_event_stream )
        {
            eos = false;
        }
        else
        {
            eos = true;
        }
    }
    else
    {
        //if ( p_sp->p_stream_graph == 0 || stream_graph_queue_full( p_sp->p_stream_graph )== true )
        //{
            //eos = false;
        //}
        //else
        //{
            eos = true;
        //}
    }

    if ( eos )
    {
        enum_stream_graph_state sgs = stream_graph_state( p_sp->p_stream_graph );
            
        if ( sgs == stream_graph_state_dequeued )
        {
            stream_graph_queue_data_source( p_sp->p_stream_graph, 0, 0 );
        }
    }
    else
    {
        sequence_player_event * p_e_window = _event_queue_window_peek( p_sp );

        ods::stream ptr_stream_window = event_stream_stream_ref( (event_stream) p_e_window->ptr_sequence_event );
        
        if ( p_e_window->stream.queued_sample == NULL_SAMPLE )
        {
            if ( p_sp->p_stream_graph == 0 || stream_graph_queue_full( p_sp->p_stream_graph ) == false )
            {
                audio_format ptr_audio_format = rsd_file_audio_format( stream_rsd_file_ref( ptr_stream_window ) );
            
                unsigned int stream_start_frame =
                    audio_format_samples_to_frames(
                        ptr_audio_format,
                        p_e_window->stream.current_sample );

                // must round samples back to nearest frame, or counter will be get out!
                
                p_e_window->stream.current_sample =
                    audio_format_frames_to_samples( ptr_audio_format, stream_start_frame );
                                    
                radRef< IRadSoundHalDataSource > ref_ds =
                    resource_manager_get_stream(
                        p_sp->p_resource_manager,
                        ptr_stream_window,
                        p_sp->resource_buffer,
                        stream_start_frame );
                                
                unsigned int stream_total_length_in_frames =
                    rsd_file_length_in_frames( stream_rsd_file_ref( ptr_stream_window ) );
                
                rAssert( stream_start_frame < stream_total_length_in_frames );
   
                unsigned int stream_frames_to_play = stream_total_length_in_frames - stream_start_frame;                            
                
                if ( p_sp->p_stream_graph == 0 )
                {
                    capture_stream_graph(
                        & p_sp->p_stream_graph,
                        construct_audio_format(
                            rsd_file_audio_format( stream_rsd_file_ref( ptr_stream_window ) ),
                            true ) );
                }                

                unsigned int queue_frame =
                    stream_graph_queue_data_source(
                        p_sp->p_stream_graph,
                        ref_ds,
                        stream_frames_to_play );
                                                                   
                p_e_window->stream.queued_sample =
                    audio_format_frames_to_samples(
                        rsd_file_audio_format(
                            stream_rsd_file_ref( ptr_stream_window ) ),
                        queue_frame );
                        
                p_e_window->stream.start_sample = p_e_window->stream.current_sample;
                
                _event_queue_window_enqueue( p_sp );
            }
        }
    }
            
    if ( stream_graph_state( p_sp->p_stream_graph ) == stream_graph_state_dequeued )
    {
        if ( eos || stream_graph_queue_full( p_sp->p_stream_graph ) == true )
        {
            stream_graph_queue( p_sp->p_stream_graph );
        }
    }   
    
    if ( stream_graph_state( p_sp->p_stream_graph ) == stream_graph_state_queued )
    {
        if ( p_sp->state == sequence_player_state_queueing )
        {        
            p_sp->state = sequence_player_state_queued;
        }
        else if ( p_sp->state == sequence_player_state_playing ||
                  p_sp->state == sequence_player_state_finishing )
        {
            if ( stream_graph_state( p_sp->p_stream_graph ) != stream_graph_state_running )
            {
                _randomize_stream_player( p_sp );                    
                stream_graph_run( p_sp->p_stream_graph );
            }
        }
    }
    
    if ( stream_graph_state( p_sp->p_stream_graph ) == stream_graph_state_running )
    {
        // revisit, add to get stitching to work
        
        if ( p_sp->state == sequence_player_state_queueing )
        {
            p_sp->state = sequence_player_state_queued;
        }
        
        ods::stream ptr_stream_head = event_stream_stream_ref(
            (event_stream) p_e_head->ptr_sequence_event );
        
        unsigned int current_pos = stream_graph_position( p_sp->p_stream_graph );
                
        if ( current_pos >= p_e_head->stream.queued_sample ) // not stitching old samples?
        {
            unsigned int stream_total_samples   = rsd_file_length_in_samples( stream_rsd_file_ref( ptr_stream_head ) );
            unsigned int stream_samples_to_play = stream_total_samples - p_e_head->stream.start_sample;           
                 
            if ( current_pos >= p_e_head->stream.queued_sample + stream_samples_to_play )
            {
                unsigned int next_head = p_sp->event_queue_head + 1;
                bool         next_full = true;
                
                if ( next_head >= g_event_queue_size )
                {
                    next_head = 0;
                }
                
                if ( next_head == p_sp->event_queue_tail )
                {
                    next_full = false;
                }
                
                bool end;                            
            
                if ( ( next_head != p_sp->event_queue_tail ) ||
                        ( next_head == p_sp->event_queue_tail ) && next_full == true )
                {                
                    sequence_player_event * p_e_head   = p_sp->event_queue + next_head;
                    
                    enum_sequence_event_type id = (enum_sequence_event_type) sequence_event_type( p_e_head->ptr_sequence_event );
                    
                    if ( id == sequence_event_stream )
                    {
                        end = false;
                    }
                    else
                    {
                        end = true;
                    }
                }
                else
                {
                    end = true;
                }
                                
                if ( end == false )
                {
                    ret = true;
                }
            }
            else
            {
            
                ods::stream ptr_stream_head = event_stream_stream_ref(
                    (event_stream) p_e_head->ptr_sequence_event );
                    
                p_e_head->stream.current_sample = current_pos - p_e_head->stream.queued_sample + p_e_head->stream.start_sample;
                 
                p_sp->beat = tempo_track_beat(
                    stream_tempo_track( ptr_stream_head ),
                    p_e_head->stream.current_sample,
                    audio_format_sampling_rate( rsd_file_audio_format( stream_rsd_file_ref( ptr_stream_head ) ) ) );
          
            }
        }
    }
    
    if ( stream_graph_state( p_sp->p_stream_graph ) == stream_graph_state_finished )
    {
        stream_graph_dequeue( p_sp->p_stream_graph );
        release_stream_graph( & p_sp->p_stream_graph );  
            
        ret = true;
    }
    
    return ret;
}          

//----------------------------------------------------------------------------

bool _event_silence( sequence_player * p_sp, sequence_player_event * p_e_head )
{    
    if ( p_e_head->silence.started == false )
    {
        if ( p_sp->state == sequence_player_state_queueing )
        {
            p_sp->state = sequence_player_state_queued;
        }
        else if ( p_sp->state == sequence_player_state_playing ||
                  p_sp->state == sequence_player_state_finishing )
        {
            p_e_head->silence.started     = true;
            p_e_head->silence.last_time   = ::radTimeGetMilliseconds( );
            _event_queue_window_enqueue( p_sp );              
        }
        
      
    }
    else
    {
        unsigned int now = radTimeGetMilliseconds( );
        unsigned int elapsed = now - p_e_head->silence.last_time;
        
        p_e_head->silence.current_time += elapsed;
        p_e_head->silence.last_time = now;
        
        p_sp->beat = ( 120.0f * radSoundIntToFloat( p_e_head->silence.current_time ) ) / 60000.0f;
        p_sp->beat = fmodf( p_sp->beat, 4.0 );        
        
        if ( p_e_head->silence.current_time >= p_e_head->silence.time )
        {
            return true;
        }
    }
    
    return false;
} 

//----------------------------------------------------------------------------
         
bool _event_var_volume( sequence_player * p_sp, sequence_player_event * p_e )
{
    p_sp->var_volume =
        vol_f_to_c(
            event_var_volume_volume(
                (event_var_volume) p_e->ptr_sequence_event ) );
        
    return true;
}

//----------------------------------------------------------------------------

static bool _event_var_pitch( sequence_player * p_sp, sequence_player_event * p_e )
{
    p_sp->var_pitch =
        pitch_f_to_s(
            event_var_pitch_pitch(
                (event_var_pitch) p_e->ptr_sequence_event ) ); 
    
    return true;  
}

//----------------------------------------------------------------------------

static bool _event_var_volume_rand_min( sequence_player * p_sp, sequence_player_event * p_e )
{
    p_sp->var_volume_rand_min =
        vol_f_to_c(
            event_var_volume_rand_min_volume_rand_min(
                (event_var_volume_rand_min) p_e->ptr_sequence_event ) );
     
    return true;
}

//----------------------------------------------------------------------------

static bool _event_var_volume_rand_max( sequence_player * p_sp, sequence_player_event * p_e )
{
    p_sp->var_volume_rand_max =
        vol_f_to_c(
            event_var_volume_rand_max_volume_rand_max(
                (event_var_volume_rand_max) p_e->ptr_sequence_event ) );
        
    return true;           
} 

//----------------------------------------------------------------------------

static bool _event_var_pitch_rand_min( sequence_player * p_sp, sequence_player_event * p_e )
{
    p_sp->var_pitch_rand_min =
        pitch_f_to_s(
            event_var_pitch_rand_min_pitch_rand_min(
                (event_var_pitch_rand_min) p_e->ptr_sequence_event ) );
    
    return true;
}  

//----------------------------------------------------------------------------

static bool _event_var_pitch_rand_max( sequence_player * p_sp, sequence_player_event * p_e )
{
    p_sp->var_pitch_rand_max =
        pitch_f_to_s(
            event_var_pitch_rand_max_pitch_rand_max(
                (event_var_pitch_rand_max) p_e->ptr_sequence_event ) );
    
    return true;
}

//----------------------------------------------------------------------------
  
static bool _event_var_aux_gain( sequence_player * p_sp, sequence_player_event * p_e )
{
    float         aux_gain = event_var_aux_gain_aux_gain( (event_var_aux_gain) p_e->ptr_sequence_event );
    unsigned char aux_number =event_var_aux_gain_aux_number( (event_var_aux_gain) p_e->ptr_sequence_event );
        
    p_sp->var_aux_gains[ aux_number ] = vol_f_to_c( aux_gain );
    
    return true;
}

//----------------------------------------------------------------------------
      
bool _event_var_positional( sequence_player * p_sp, sequence_player_event * p_e )
{   
    p_sp->var_positional =
        event_var_positional_positional(
            (event_var_positional) p_e->ptr_sequence_event ) ;
        
    return true; 
} 

//----------------------------------------------------------------------------

bool _event_var_pos_fall_off( sequence_player * p_sp, sequence_player_event * p_e )
{    
    p_sp->var_pos_fall_off =
        event_var_pos_fall_off_pos_fall_off(
            (event_var_pos_fall_off) p_e->ptr_sequence_event );
        
    return true;
}

//----------------------------------------------------------------------------

bool _event_var_pos_dist_min( sequence_player * p_sp, sequence_player_event * p_e )
{
    p_sp->var_pos_dist_min =
        event_var_pos_dist_min_pos_dist_min(
            (event_var_pos_dist_min) p_e->ptr_sequence_event );
    
    return true;   
}

//----------------------------------------------------------------------------

bool _event_var_pos_dist_max( sequence_player * p_sp, sequence_player_event * p_e )
{   
    p_sp->var_pos_dist_max =
        event_var_pos_dist_max_pos_dist_max(
            (event_var_pos_dist_max) p_e->ptr_sequence_event );
    
    return true;
}

//----------------------------------------------------------------------------
 
bool _event_var_callback( sequence_player * p_sp, sequence_player_event * p_e_head )
{
    char name[ 64 ];
    
    event_callback_callback_name( (event_callback) p_e_head->ptr_sequence_event,
        name, 64 );
    
    rDebugPrintf( "Please implement callbacks! [%s]\n",
        name );
        
    return true;
}  

//============================================================================
//
// radmusic sequence_player sequence_player_event reset functions
//
//============================================================================

static void _event_reset_clip( sequence_player *, sequence_player_event * p_e )
{
    p_e->clip.started          = false;
}

static void _event_reset_stream( sequence_player *, sequence_player_event * p_e )
{
    p_e->stream.start_sample   = NULL_SAMPLE;
    p_e->stream.queued_sample  = NULL_SAMPLE;
}

static void _event_reset_silence( sequence_player *, sequence_player_event * p_e )
{
    p_e->silence.started = false;
}

//============================================================================
//
// radmusic sequence_player sequence_player_event reset functions
//
//============================================================================

//----------------------------------------------------------------------------

static unsigned int _event_remaining_time_clip( sequence_player * p_sp, sequence_player_event * p_e )
{
    ods::clip ptr_clip = event_clip_clip_ref( (event_clip) p_e->ptr_sequence_event );
        
    unsigned int length_in_samples = rsd_file_length_in_samples( clip_rsd_file_ref( ptr_clip ) );
    
    unsigned int samples_left = length_in_samples - p_e->clip.current_sample;
    unsigned int ms_left = audio_format_samples_to_milliseconds(
        rsd_file_audio_format( clip_rsd_file_ref( ptr_clip ) ),
        samples_left );
    
    return ms_left;
    
}

//----------------------------------------------------------------------------

static unsigned int _event_remaining_time_stream( sequence_player * p_sp, sequence_player_event * p_e )
{
    ods::stream ptr_stream = event_stream_stream_ref( (event_stream) p_e->ptr_sequence_event );

    unsigned int length_in_samples = rsd_file_length_in_samples( stream_rsd_file_ref( ptr_stream ) );
    unsigned int samples_left      = length_in_samples - p_e->clip.current_sample;
    unsigned int ms_left =
        audio_format_samples_to_milliseconds(
            rsd_file_audio_format( stream_rsd_file_ref( ptr_stream ) ), samples_left );
    
    return ms_left;
}

//----------------------------------------------------------------------------

static unsigned int _event_remaining_time_silence( sequence_player *, sequence_player_event * p_e )
{
    unsigned int ms_left = radSoundFloatToUInt( p_e->silence.time - p_e->silence.current_time );
    return ms_left;
}


//============================================================================
//
// radmusic sequence_player sequence_player_command stack management functions
//
//============================================================================

//----------------------------------------------------------------------------

void _command_stack_reset( sequence_player * p_sp )
{
    p_sp->command_stack_pointer = 0;
}

//----------------------------------------------------------------------------

void _command_stack_push( sequence_player * p_sp, const sequence_event & ptr_sequence_event )
{
    rAssert( p_sp->command_stack_pointer < sequence_stack_size( p_sp->ptr_sequence ) );
    
    sequence_player_command * p_new = p_sp->p_command_stack + p_sp->command_stack_pointer;
    p_new->ptr_sequence_event = ptr_sequence_event;
    p_new->enter = true;
    
    p_sp->command_stack_pointer++;
}

//----------------------------------------------------------------------------

sequence_player_command * _command_stack_peek( sequence_player * p_sp )
{
    if ( p_sp->command_stack_pointer > 0 )
    {
        return p_sp->p_command_stack + ( p_sp->command_stack_pointer - 1 );
    }
    
    return 0;
}

//----------------------------------------------------------------------------

static command_stack_type _command_stack_items( sequence_player* p_sp )
{
    return p_sp->command_stack_pointer;
}

//----------------------------------------------------------------------------

void _command_stack_pop( sequence_player * p_sp )
{
    rAssert( p_sp->command_stack_pointer > 0 );
    p_sp->command_stack_pointer--;
}

//============================================================================
//
// radmusic sequence_player sequence_player_event queue management functions
//
//============================================================================

//----------------------------------------------------------------------------

event_queue_type _event_queue_wrap( event_queue_type i )
{
    i = i % g_event_queue_size;
    
    return i;
}

//----------------------------------------------------------------------------

void _event_queue_reset( sequence_player * p_sp )
{
    p_sp->event_queue_tail         = 0;
    p_sp->event_queue_head         = 0;
    p_sp->event_queue_items        = 0;
    p_sp->event_queue_window_items = 0;
}

//----------------------------------------------------------------------------
        
void _event_queue_enqueue( sequence_player * p_sp, sequence_player_event * p_e )
{
    rAssert( _event_queue_full( p_sp ) == false );
             
    p_sp->event_queue[ p_sp->event_queue_tail ] = *p_e;
    
    p_sp->event_queue_tail = _event_queue_wrap( p_sp->event_queue_tail + 1 );
    p_sp->event_queue_items++;
       
    enum_sequence_event_type id = (enum_sequence_event_type) sequence_event_type( p_e->ptr_sequence_event );  
    
    // rDebugPrintf( "sequence_player_event: enqueue" );
    
    // g_p_event_handlers[ id ].p_dump_fx( p_sp, p_e );
     
}

//----------------------------------------------------------------------------

sequence_player_event * _event_queue_peek( sequence_player * p_sp )
{
    if ( p_sp->event_queue_items <= 0 )
    {
        return 0;        
    }
    
    return p_sp->event_queue + p_sp->event_queue_head;
}

//----------------------------------------------------------------------------

void _event_queue_dequeue( sequence_player * p_sp )
{  
    rAssert( _event_queue_empty( p_sp ) == false );

    // rDebugPrintf( "sequence_player_event: dequeue" );
    // sequence_player_event * p_e = p_sp->event_queue + p_sp->event_queue_head;
    // g_p_event_handlers[ command_type( p_sp->ptr_sequence.p_mb, p_e->ptr_sequence_event ) ].p_dump_fx( p_sp, p_e );      
 
    p_sp->event_queue_head = _event_queue_wrap( p_sp->event_queue_head + 1 );  
    p_sp->event_queue_items--;
    
    if ( p_sp->event_queue_window_items > 0 )
    {
        p_sp->event_queue_window_items--;
    }
}

//----------------------------------------------------------------------------

bool _event_queue_full( sequence_player * p_sp )
{
    bool full = p_sp->event_queue_items >= g_event_queue_size;
    
    return full;
}

//----------------------------------------------------------------------------

static bool _event_queue_empty( sequence_player * p_sp )
{
    bool empty = p_sp->event_queue_items <= 0;
    
    return empty;
}

//----------------------------------------------------------------------------

static event_queue_type _event_queue_items( sequence_player * p_s )
{
    return p_s->event_queue_items;
}

//----------------------------------------------------------------------------

static sequence_player_event * _event_queue_item_at( sequence_player * p_sp, event_queue_type i )
{
    rAssert( p_sp->event_queue_items >= i );
    
    i = _event_queue_wrap( p_sp->event_queue_head + i );
    
    sequence_player_event * p_e = p_sp->event_queue + i;
    
    return p_e;
}

//----------------------------------------------------------------------------

sequence_player_event * _event_queue_window_peek( sequence_player * p_sp )
{
    if ( p_sp->event_queue_window_items == p_sp->event_queue_items )
    {
        return 0;
    }
    else
    {
        event_queue_type i = _event_queue_wrap( p_sp->event_queue_head + p_sp->event_queue_window_items );
        
        sequence_player_event * p_e = p_sp->event_queue + i; 
        return p_e;
    }
}

//----------------------------------------------------------------------------

void _event_queue_window_enqueue( sequence_player * p_sp )
{
    rAssert( p_sp->event_queue_window_items < p_sp->event_queue_items );

    p_sp->event_queue_window_items++;
}

//----------------------------------------------------------------------------

void _event_queue_window_dequeue( sequence_player * p_sp )
{
    rAssert( p_sp->event_queue_window_items > 0 );
    p_sp->event_queue_window_items--;
}

//----------------------------------------------------------------------------

event_queue_type _event_queue_window_unqueued_items( sequence_player * p_sp )
{
    event_queue_type items = p_sp->event_queue_items - p_sp->event_queue_window_items;

    return items;
}

//----------------------------------------------------------------------------

static event_queue_type _event_queue_window_queued_items( sequence_player * p_sp )
{
    return p_sp->event_queue_window_items;
}    
 
//============================================================================
//
// sequence_player_event dumping functions
//
//============================================================================

//----------------------------------------------------------------------------

void _event_dump_clip( sequence_player * p_sp, sequence_player_event * p_e )
{    
    ods::clip ptr_clip = event_clip_clip_ref( (event_clip) p_e->ptr_sequence_event );    
   
    char clip_nom[ 256 ];
    char file_nom[ 256 ];
    
    clip_name( ptr_clip, clip_nom, 256 );
    rsd_file_file_name( clip_rsd_file_ref( ptr_clip ), file_nom, 256 );
    
    rDebugPrintf( "sequence_player_event: ods::clip: [%s]:[%s]\n",
        clip_nom,
        file_nom );
}

//----------------------------------------------------------------------------

void _event_dump_stream( sequence_player * p_sp, sequence_player_event * p_e )
{
    ods::stream ptr_stream = event_stream_stream_ref( (event_stream) p_e->ptr_sequence_event );
    
    char stream_nom[ 256 ];
    char file_name[ 256 ];
    
    stream_name( ptr_stream, stream_nom, 256 );
    rsd_file_file_name( stream_rsd_file_ref( ptr_stream ), file_name, 256 );
    
    rDebugPrintf( "sequence_player_event: ods::stream: [%s]:[%s]\n",
        stream_nom,
        file_name );
}

//----------------------------------------------------------------------------

void _event_dump_silence( sequence_player * p_sp, sequence_player_event * p_e )
{
    float        min = event_silence_min_time( (event_silence) p_e->ptr_sequence_event );
    float        max = event_silence_max_time( (event_silence) p_e->ptr_sequence_event );
        
    rDebugPrintf( "sequence_player_event: silence: min:[%f] max:[%f] rand:[%f]\n",
        min, max, p_e->silence.time );
}

//----------------------------------------------------------------------------
         
void _event_dump_var_volume( sequence_player * p_sp, sequence_player_event * p_e )
{    
    float vol = event_var_volume_volume( (event_var_volume) p_e->ptr_sequence_event );
   
    rDebugPrintf( "sequence_player_event: var_volume: [%f]\n", vol );
}

static void _event_dump_var_pitch( sequence_player * p_sp, sequence_player_event * p_e )
{
    float pitch = event_var_pitch_pitch( (event_var_pitch) p_e->ptr_sequence_event );
    
    rDebugPrintf( "sequence_player_event: var_pitch: [%f]\n", pitch );
}

static void _event_dump_var_volume_rand_min( sequence_player * p_sp, sequence_player_event * p_e )
{
    float volume_rand_min =
        event_var_volume_rand_min_volume_rand_min(
            (event_var_volume_rand_min) p_e->ptr_sequence_event );
    
    rDebugPrintf( "sequence_player_event: var_volume_rand_min: [%f]\n", volume_rand_min );
}

static void _event_dump_var_volume_rand_max( sequence_player * p_sp, sequence_player_event * p_e )
{
    float volume_rand_max =
        event_var_volume_rand_max_volume_rand_max(
            (event_var_volume_rand_max) p_e->ptr_sequence_event );
    
    rDebugPrintf( "sequence_player_event: var_volume_rand_max: [%f]\n", volume_rand_max );
}

static void _event_dump_var_pitch_rand_min( sequence_player * p_sp, sequence_player_event * p_e )
{
    float pitch_rand_min =
        event_var_pitch_rand_min_pitch_rand_min(
            (event_var_pitch_rand_min) p_e->ptr_sequence_event );
    
    rDebugPrintf( "sequence_player_event: var_pitch_rand_min: [%f]\n", pitch_rand_min );
}

static void _event_dump_var_pitch_rand_max( sequence_player * p_sp, sequence_player_event * p_e )
{
    float pitch_rand_max =
        event_var_pitch_rand_max_pitch_rand_max(
            (event_var_pitch_rand_max) p_e->ptr_sequence_event );
    
    rDebugPrintf( "sequence_player_event: var_pitch_rand_max: [%f]\n", pitch_rand_max );
}

static void _event_dump_var_aux_gain( sequence_player * p_sp, sequence_player_event * p_e )
{
    float         aux_gain   = event_var_aux_gain_aux_gain( (event_var_aux_gain) p_e->ptr_sequence_event );
    unsigned char aux_number = event_var_aux_gain_aux_number( (event_var_aux_gain) p_e->ptr_sequence_event );
    
    rDebugPrintf( "sequence_player_event: var_aux_gain: [%d][%f]\n", aux_gain, aux_number );
}

//----------------------------------------------------------------------------
       
void _event_dump_var_positional( sequence_player * p_sp, sequence_player_event * p_e )
{    
    bool positional = event_var_positional_positional( (event_var_positional) p_e->ptr_sequence_event );
    
    rDebugPrintf( "sequence_player_event: var_positional: [%d]\n", positional );
} 

//----------------------------------------------------------------------------

void _event_dump_var_pos_fall_off( sequence_player * p_sp, sequence_player_event * p_e )
{    
    float pos_fall_off = event_var_pos_fall_off_pos_fall_off( (event_var_pos_fall_off) p_e->ptr_sequence_event );
   
    rDebugPrintf( "sequence_player_event: var_pos_fall_off: [%f]\n", pos_fall_off );
}

//----------------------------------------------------------------------------
 
void _event_dump_var_pos_dist_min( sequence_player * p_sp, sequence_player_event * p_e )
{    
    float pos_dist_min = event_var_pos_dist_min_pos_dist_min( (event_var_pos_dist_min) p_e->ptr_sequence_event );
    
    rDebugPrintf( "sequence_player_event: var_pos_dist_min: [%f]\n", pos_dist_min );
}

//----------------------------------------------------------------------------

void _event_dump_var_pos_dist_max( sequence_player * p_sp, sequence_player_event * p_e )
{    
    float pos_dist_max = event_var_pos_dist_max_pos_dist_max( (event_var_pos_dist_max) p_e->ptr_sequence_event );
    
    rDebugPrintf( "sequence_player_event: var_pos_dist_max: [%f]\n", pos_dist_max );
}

//----------------------------------------------------------------------------
 
void _event_dump_var_callback( sequence_player * p_sp, sequence_player_event * p_e )
{    
    char name[ 64 ];
    
    event_callback_callback_name( (event_callback) p_e->ptr_sequence_event, name, 64 );

    rDebugPrintf( "sequence_player_event: callback: [%s]\n", name );
}
        
} // namespace