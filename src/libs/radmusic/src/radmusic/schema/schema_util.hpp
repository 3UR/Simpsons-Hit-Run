//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
/// 
/// Utility functions for manipulating, retrieving data from the radmusic
/// database schema.
///
//-----------------------------------------------------------------------------

#ifndef SCHEMA_UTIL_HPP
#define SCHEMA_UTIL_HPP

//-----------------------------------------------------------------------------
// Nested Includes
//-----------------------------------------------------------------------------

#include <radmusic/radmusic.hpp>
#include <ods/ods.hpp>
#include <radstring.hpp>

//-----------------------------------------------------------------------------
// Forward References
//-----------------------------------------------------------------------------

namespace radmusic {

//-----------------------------------------------------------------------------
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Interface Definitions
//-----------------------------------------------------------------------------

ods::stitch_transition composition_find_stitch_transition(
    const ods::comp & ptr_composition,
    const ods::region & ptr_source_region,
    const ods::region & ptr_target_region );
    
ods::fade_transition composition_find_fade_transition(
    const ods::comp & ptr_composition,
    const ods::region & ptr_source_region,
    const ods::region & ptr_target_region );

void group_memory_sizes_recurse(
    const ods::group & ptr_group,
    unsigned int * pNumSoundResources,
    unsigned int * pSoundMemorySize,
    ods::group * p_ptr_group_largest_sound_memory,
    unsigned int * pNumCacheResources,
    unsigned int * pCacheMemorySize,
    ods::group * p_ptr_group_largest_cache_memory );
    
void group_memory_sizes(
    const ods::group & ptr_group,
    unsigned int * p_number_of_sound_memory_resources,
    unsigned int * p_sound_memory_size,
    unsigned int * p_number_of_cache_memory_readouds,
    unsigned int * p_cache_memory_size );

void group_check_stream_constraints_recursive(
    const ods::group & ptr_group,
    unsigned int min_length_in_ms,
    char * p_error_string,
    unsigned int len,
    bool * ok );

bool group_is_child_of( const ods::group & ptr_group, const ods::group & ptr_parent );
ods::region group_find_region_in_sub_tree( const ods::group & ptr_group, const char * p_region_name );
    
bool composition_check_constraints(
    const ods::comp & ptr_composition,
    char * p_error_message,
    unsigned int len );

radRef< IRadSoundHalAudioFormat > construct_audio_format( const ods::audio_format &, bool targetFormat );

unsigned int audio_format_samples_to_milliseconds( const ods::audio_format &, unsigned int samples );
unsigned int audio_format_samples_to_frames( const ods::audio_format &, unsigned int samples );
unsigned int audio_format_frames_to_samples( const ods::audio_format & ptr_audio_format, unsigned int frames );

unsigned int rsd_file_length_in_ms( const ods::rsd_file & ptr_rsd_file );
unsigned int rsd_file_length_in_samples ( const ods::rsd_file & ptr_rsd_file );
unsigned int rsd_file_length_in_bytes( const ods::rsd_file & ptr_rsd_file );
unsigned int rsd_file_length_in_frames( const ods::rsd_file & ptr_rsd_file );
    
ods::fade_transition composition_find_fade_transition(
    const ods::comp & ptr_composition,
    const ods::region & ptr_region_source,
    const ods::region & ptr_region_target );
    
bool  beat_in_range( float beat, float last_beat, float current_beat );
bool  beat_set_beat_in_range( const ods::beat_set & ptr_beat_set, float last_beat, float current_beat );
float tempo_track_beat( const ods::tempo_track & ptr_tempo_track, unsigned int current_sample, unsigned int sampling_rate );

unsigned int calculate_action_offset(
    const ods::event & ptr_event,
    unsigned int * p_dimension_values );
    
enum enum_region_resume_type
{    
    region_resume_type_resume,
    region_resume_type_restart
};

enum enum_sequence_event_type
{
    sequence_event_clip,
    sequence_event_stream,                 
    sequence_event_silence,
    sequence_event_var_volume,
    sequence_event_var_pitch,
    sequence_event_var_volume_rand_min,
    sequence_event_var_volume_rand_max,
    sequence_event_var_pitch_rand_min,
    sequence_event_var_pitch_rand_max,
    sequence_event_var_aux_gain,
    sequence_event_var_positional,
    sequence_event_var_pos_fall_off,
    sequence_event_var_pos_dist_min,
    sequence_event_var_pos_dist_max,
    sequence_event_callback,
    sequence_event_logic_and,
    sequence_event_logic_or,
    sequence_event_logic_repeat,
    sequence_event_max
};

enum enum_action_type
{
    action_type_push_region,
    action_type_pop_region,
    action_type_play_region,
    action_type_start_layer,
    action_type_stop_layer,
    action_type_rand_state
};

const unsigned int event_logic_repeat_infinite = 0xFFFFFFFF;

}
#endif // COMPOSITION_HPP