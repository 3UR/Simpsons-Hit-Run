//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
/// 
//-----------------------------------------------------------------------------

#ifndef RADMUSIC_HPP
#define RADMUSIC_HPP

//-----------------------------------------------------------------------------
// Nested Includes
//-----------------------------------------------------------------------------

#include <radmemory.hpp>
#include <radsound.hpp>
#include <radload/radload.hpp>
#include <radmusic/schema.hpp>

namespace radmusic
{

//-----------------------------------------------------------------------------
// Forward References
//-----------------------------------------------------------------------------

struct resource_manager;
struct stream_graph;
struct sequence_player;        
struct performance;
struct composition;

//-----------------------------------------------------------------------------
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//-----------------------------------------------------------------------------

extern const char * debug_channel;
const radMemorySpace default_preloaded_stream_space = radMemorySpace_Local;

//-----------------------------------------------------------------------------
// Interface Definitions
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// system functions
//----------------------------------------------------------------------------

struct stream_graph_description
{
    unsigned int   channels;
    unsigned int   sampling_rate;
    
    unsigned int   stream_buffer_size_in_ms;
    
    bool           use_buffered_data_source;
    radMemorySpace buffered_data_source_space;    
    unsigned int   buffered_data_source_size_in_ms;
};

void initialize(
    stream_graph_description *,
    unsigned int num_stream_players,
    unsigned int num_clip_players,
    radMemoryAllocator allocator );
    
void terminate( void );

//----------------------------------------------------------------------------
// resource management functions
//----------------------------------------------------------------------------

resource_manager * resource_manager_new(
    const char * p_search_path,
    radMemoryAllocator ai_allocator,    
    unsigned int number_of_sound_resources,
    unsigned int sound_resource_buffer_size,
    radMemorySpace cache_memory_space,
    radMemoryAllocator cache_allocator,
    unsigned int number_of_cache_resources,
    unsigned int cache_resource_buffer_size );

void resource_manager_delete( resource_manager ** );

unsigned int resource_manager_move_to_group(
    resource_manager *,
    const ods::group & ptr_group,
    unsigned int buffer_number );
    
bool resource_manager_group_loaded(
    resource_manager *,
    unsigned int buffer,
    const ods::group & ptr_group );

radRef< IRadSoundClip > resource_manager_get_clip(
    resource_manager *,
    const ods::clip & ptr_clip,
    unsigned int buffer );
    
radRef< IRadSoundHalDataSource > resource_manager_get_stream(
    resource_manager *,
    const ods::stream & ptr_stream,
    unsigned int buffer,
    unsigned int start_frame );
    
//----------------------------------------------------------------------------
// playback resource management/arbitration functions
//----------------------------------------------------------------------------

void capture_stream_graph( stream_graph **, IRadSoundHalAudioFormat * );
void release_stream_graph( stream_graph ** );
void capture_clip_player( IRadSoundClipPlayer ** );
void release_clip_player( IRadSoundClipPlayer ** );

//----------------------------------------------------------------------------
// stream graph functions
//----------------------------------------------------------------------------

enum enum_stream_graph_state {
    stream_graph_state_dequeued,
    stream_graph_state_queueing,
    stream_graph_state_queued,
    stream_graph_state_running,
    stream_graph_state_finished }; 

enum_stream_graph_state   stream_graph_state( stream_graph * );
void                      stream_graph_paused( stream_graph *, bool );
unsigned int              stream_graph_queue_data_source( stream_graph *, IRadSoundHalDataSource *, unsigned int length );
bool                      stream_graph_queue_full( stream_graph * );
void                      stream_graph_queue( stream_graph * );
void                      stream_graph_queue_flush( stream_graph * );
void                      stream_graph_run( stream_graph * );
void                      stream_graph_dequeue( stream_graph * );
void                      stream_graph_volume( stream_graph *, float );
void                      stream_graph_pitch( stream_graph *, float );
void                      stream_graph_aux( stream_graph *, unsigned int, float );
void                      stream_graph_positional_group( stream_graph *, IRadSoundHalPositionalGroup * );
unsigned int              stream_graph_position( stream_graph * );
IRadSoundHalAudioFormat * stream_graph_format( stream_graph * );      

//----------------------------------------------------------------------------
// sequence player functions
//----------------------------------------------------------------------------

enum enum_sequence_player_state {
    sequence_player_state_idle,
    sequence_player_state_queueing,
    sequence_player_state_queued,
    sequence_player_state_playing,
    sequence_player_state_finishing,
    sequence_player_state_finished };

sequence_player * sequence_player_new(
    ods::sequence & ptr_sequence,
    resource_manager *,
    const char * name );
    
void sequence_player_delete( sequence_player** );

enum_sequence_player_state sequence_player_state( sequence_player * );
void                sequence_player_queue( sequence_player *, unsigned int resource_buffer, stream_graph *, bool resetState );
void                sequence_player_dequeue( sequence_player *, stream_graph ** );
void                sequence_player_play( sequence_player * );
void                sequence_player_update( sequence_player * );
void                sequence_player_paused( sequence_player *, bool );
void                sequence_player_volume( sequence_player *, float );
void                sequence_player_pitch(  sequence_player *, float );
void                sequence_player_beat( sequence_player *, float * p_beat );
unsigned int        sequence_player_remaining_time( sequence_player * );

//----------------------------------------------------------------------------
// music composition functions
//----------------------------------------------------------------------------

composition * composition_new_from_file( const char * p_file_name );
void composition_delete( composition ** );

// you can now load compositions via radload.  The composition will appear
// in the inventory as an object of this class.  You can retrieve the
// actual comp from the adapter object.  This object will handle
// creation and destruction of the composition for you, or set this object's
// p_composition to null and delete the composition yourself.

void register_radload_loaders( void );

struct radload_composition_adapter : public radLoadObject {
    ~radload_composition_adapter( );       
    composition * p_composition;
};

//----------------------------------------------------------------------------
// music performance functions
//----------------------------------------------------------------------------
      
performance * performance_new(
    composition *,
    const char * p_search_directory,
    radMemorySpace pre_loaded_stream_space = default_preloaded_stream_space );

void         performance_delete( performance ** );	    

void         performance_update( performance *, unsigned int ms );
void         performance_stop( performance * );
void         performance_paused( performance *, bool );
bool         performance_paused( performance * );
void         performance_volume( performance *, float );
float        performance_volume( performance * );
void         performance_pitch( performance *, float );
float        performance_pitch( performance * );

unsigned int performance_num_events( performance * p );
void         performance_event_name( performance * p, unsigned int, char *, unsigned int size );
void         performance_trigger_event( performance *, unsigned int );
    	    
unsigned int performance_num_states( performance * p );
void         performance_state_name( performance * p, unsigned int, char *, unsigned int );
void         performance_state_value( performance * p, unsigned int, unsigned int);
unsigned int performance_state_value( performance * p, unsigned int );
unsigned int performance_num_state_values( performance * p, unsigned int);
void         performance_state_value_name( performance * p, unsigned int, unsigned int, char *, unsigned int );

bool         performance_is_state_steady_idle( performance * p );

void         debug_performance_current_region_name( performance *, char * p_string, unsigned int size );
bool         debug_performance_current_beat( performance *, float * );
    
}

#endif RADMUSIC_HPP