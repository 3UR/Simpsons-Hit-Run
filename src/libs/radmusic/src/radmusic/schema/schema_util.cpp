//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
/// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "pch.hpp"
#include <radmusic/schema/schema_util.hpp>
#include <memory/memory.hpp>
#include <radload/loader.hpp>
#include <radsoundfile.hpp>
#include <radmemory.hpp>

//============================================================================
// include files
//============================================================================

using namespace ods;

//-----------------------------------------------------------------------------
// Local Constants, Typedefs, and Macros
//-----------------------------------------------------------------------------

namespace radmusic {

//-----------------------------------------------------------------------------
// Global Data, Local Data, Local Classes
//-----------------------------------------------------------------------------

#define VAG_SAMPLES_PER_FRAME 28
#define VAG_MONO_FRAME_SIZE 16

struct audio_format_info
{
    unsigned int mono_8_bit_frame_size_bytes;
    unsigned int frame_size_samples;
};
		
static const unsigned int RADICAL_ADPCM_FRAME_SIZE = 20;
static const unsigned int RADICAL_ADPCM_SAMPLES_PER_FRAME = 32;

audio_format_info g_audio_format_info[] = 
{
    { 1, 1 }, // PCM
    { VAG_MONO_FRAME_SIZE / 2, VAG_SAMPLES_PER_FRAME }, //VAG
    { 1, 1 },   // PCM_BIGENDIAN,
    { 36 / 2, 64 }, // XBOXADPCM,
    { 0, 0 }, // GCN - REMOVED
    { RADICAL_ADPCM_FRAME_SIZE / 2, RADICAL_ADPCM_FRAME_SIZE } 
};

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

bool composition_check_stream_constraints(
    const comp & ptr_composition,
    char * p_error_message,
    unsigned int len );
    
void group_memory_sizes_recurse(
    const group & ptr_group,
    unsigned int * p_num_sound_resources,
    unsigned int * p_sound_memory_size,
    group * p_ptr_group_largest_sound_memory,
    unsigned int * p_num_cache_resources,
    unsigned int * p_cache_memory_size,
    group * p_ptr_group_largest_cache_memory )
{
    group_array ptr_children = group_children( ptr_group );
    unsigned int num_children = group_array_num_items( ptr_children );

    if ( num_children == 0 )
    {
        // Only calculate leaf nodes

        unsigned int soundMemorySize = 0;
        unsigned int cacheMemorySize = 0;
        unsigned int numberOfCacheMemoryResources = 0;
        unsigned int numberOfSoundMemoryResources = 0;

        group_memory_sizes( ptr_group,
            & numberOfSoundMemoryResources, & soundMemorySize,
            & numberOfCacheMemoryResources, & cacheMemorySize );

        if ( soundMemorySize >= *p_sound_memory_size )
        {
            *p_sound_memory_size = soundMemorySize;

            if ( p_ptr_group_largest_sound_memory != 0 )
            {
                *p_ptr_group_largest_sound_memory = ptr_group;
            }
        }

        if ( cacheMemorySize >= *p_cache_memory_size )
        {
            *p_cache_memory_size = cacheMemorySize;

            if ( p_ptr_group_largest_cache_memory != 0 )
            {
                *p_ptr_group_largest_cache_memory = ptr_group;
            }
        }

        if ( numberOfSoundMemoryResources >= *p_num_sound_resources )
        {
            *p_num_sound_resources += numberOfSoundMemoryResources;
        }

        if ( numberOfCacheMemoryResources >= *p_num_cache_resources )
        {
            *p_num_cache_resources += numberOfCacheMemoryResources;
        }

        char group_nom[ 256 ];
        group_name( ptr_group, group_nom, 256 );
        
        rDebugChannelPrintf( debug_channel,
            "Leaf Group: [%20s] SoundMemory: [0x%x][%d][%.2f]K CacheMemory: [0x%x][%d][%.2f]K\n",
                group_nom,
                soundMemorySize,
                soundMemorySize,
                radSoundFloatToDouble( soundMemorySize / 1024.0f ),
                cacheMemorySize,
                cacheMemorySize,
                radSoundFloatToDouble( (float)cacheMemorySize / 1024.0f ) );
    }
    else
    {
        for( unsigned int c = 0; c < num_children; c ++ )
        {
            group_memory_sizes_recurse(
                group_array_item_at( ptr_children, c ),
                p_num_sound_resources, p_sound_memory_size, p_ptr_group_largest_sound_memory,
                p_num_cache_resources, p_cache_memory_size, p_ptr_group_largest_cache_memory );
        }
    }
}

bool composition_check_constraints( const comp & ptr_composition, char * p_error_msg, unsigned int len )
{
    bool ok = true;
    char sound_msg[ 2048 ];
    char cache_msg[ 2048 ];

    unsigned int soundMemorySize = 0;
    unsigned int cacheMemorySize = 0;
    unsigned int numSoundResources = 0;
    unsigned int numCacheResources = 0;

    group_array ptr_group_array = comp_root_groups( ptr_composition );
    group ptr_group_1 = group_array_item_at( ptr_group_array, 0 );
    
    group_memory_sizes_recurse( ptr_group_1,
        & numSoundResources, & soundMemorySize, 0,
        & numCacheResources, & cacheMemorySize, 0 );
            
    if ( soundMemorySize > comp_sound_memory_max( ptr_composition ) )
    {
        sprintf( sound_msg, "Composition uses more sound memory than is budgeted:\n    Budget: H:[0x%x]B D:[%d]B [%.2f]K [%.2f]M\n    Used: H:[0x%x]B D:[%d]B [%.2f]K [%.2f]M\n",
            comp_sound_memory_max( ptr_composition ),
            comp_sound_memory_max( ptr_composition ),
            radSoundFloatToDouble( (float) comp_sound_memory_max( ptr_composition ) / 1024.0f ),
            radSoundFloatToDouble( (float) comp_sound_memory_max( ptr_composition ) / ( 1024.0f * 1024.0f ) ),
            soundMemorySize,
            soundMemorySize,
            radSoundFloatToDouble( (float) soundMemorySize / 1024.0f ),
            radSoundFloatToDouble( (float) soundMemorySize / ( 1024.0f * 1024.0f ) ) );
            
        ok = false;           
    }
    else
    {
        strcpy( sound_msg, "" );
    }

    if ( cacheMemorySize > comp_cache_memory_max( ptr_composition ) )
    {
        sprintf( cache_msg, "Composition uses more cache memory than is budgeted:\n    Budget: H:[0x%x]B D:[%d]B [%.2f]K [%.2f]M\n    Used: H:[0x%x]B D:[%d]B [%.2f]K [%.2f]M\n",
            comp_cache_memory_max( ptr_composition ),
            comp_cache_memory_max( ptr_composition ),
            radSoundFloatToDouble( comp_cache_memory_max( ptr_composition ) / 1024.0f ),
            radSoundFloatToDouble( (float) comp_cache_memory_max( ptr_composition ) / ( 1024.0f * 1024.0f ) ),
            cacheMemorySize,
            cacheMemorySize,
            radSoundFloatToDouble( (float) cacheMemorySize / 1024.0f ),
            radSoundFloatToDouble( (float) cacheMemorySize / ( 1024.0f * 1024.0f ) ) );

        ok = false;
    }
    else
    {
        strcpy( cache_msg, "" );
    }

    char stream_error_msg[ 4096 ];
    
    bool streamsOk = composition_check_stream_constraints(
        ptr_composition, stream_error_msg, 4096 );

    if ( ! ok || ! streamsOk )
    {
        strcpy( p_error_msg, ok && streamsOk ? "" : "Your comp does not meet minimum memory and/or disk constraints:\n" );
        strcat( p_error_msg, ok ? "" : "\n" );
        strcat( p_error_msg, sound_msg );
        strcat( p_error_msg, cache_msg );
        strcat( p_error_msg, streamsOk ? "" : "\nThe following files are too short to ods::stream:\n" );
        strcat( p_error_msg, stream_error_msg );    
    }
    else
    {
        strcpy( p_error_msg, "" );
    }

    return ok && streamsOk;
}

void group_check_stream_constraints_recursive(
    const group & ptr_group,
    unsigned int min_length_in_ms,
    char * p_error_string,
    unsigned int len,
    bool * ok )
{
    stream_ref_array ptr_stream_ref_array = group_stream_refs( ptr_group );
    
    unsigned int num_streams = stream_ref_array_num_items( ptr_stream_ref_array );
    
    for( unsigned int s = 0; s < num_streams; s ++ )
    {
        ods::stream ptr_stream = stream_ref_array_item_at( ptr_stream_ref_array, s );
        
        if ( stream_streamed( ptr_stream ) )
        {
            rsd_file ptr_rsd_file =  stream_rsd_file_ref( ptr_stream );
            
            unsigned int length_in_ms = rsd_file_length_in_ms( ptr_rsd_file );

            if ( length_in_ms < min_length_in_ms )
            {
                char file_name[ 256 ];
                rsd_file_file_name( ptr_rsd_file, file_name, 256 );
                
                char error_msg[ 512 ];
                sprintf( error_msg, "    %s.rsd: %dms\n", file_name, length_in_ms ); 

                strncat( p_error_string, error_msg, len );

                *ok = false;
            }
        }
    }

    group_array ptr_group_array = group_children( ptr_group );

    unsigned int num_children = group_array_num_items( ptr_group_array );
    
    for( unsigned int c = 0; c < num_children; c ++ )
    {
        group ptr_group_new = group_array_item_at( ptr_group_array, c );
        
        group_check_stream_constraints_recursive( ptr_group_new, min_length_in_ms, p_error_string, len, ok );
    }
}

unsigned int rsd_file_length_in_ms( const rsd_file & ptr_rsd_file )
{
    audio_format ptr_audio_format = rsd_file_audio_format( ptr_rsd_file );
    
    return ( rsd_file_length_in_samples( ptr_rsd_file ) * 1000 ) / audio_format_sampling_rate( ptr_audio_format );
}

unsigned int rsd_file_length_in_samples( const rsd_file & ptr_rsd_file )
{
    audio_format ptr_audio_format = rsd_file_audio_format( ptr_rsd_file );
    IRadSoundHalAudioFormat::Encoding e = (IRadSoundHalAudioFormat::Encoding) audio_format_encoding( ptr_audio_format );
 
    if ( e == IRadSoundHalAudioFormat::RadicalAdpcm )
    {
        e = IRadSoundHalAudioFormat::PCM;   
    } 
     
    unsigned int num_frames = rsd_file_length_in_frames( ptr_rsd_file );
        
    unsigned int num_samples = g_audio_format_info[ e ].frame_size_samples  * num_frames;
    
    return num_samples;
}

unsigned int rsd_file_length_in_frames( const rsd_file & ptr_rsd_file )
{
    audio_format ptr_audio_format = rsd_file_audio_format( ptr_rsd_file );

    IRadSoundHalAudioFormat::Encoding e = (IRadSoundHalAudioFormat::Encoding) audio_format_encoding( ptr_audio_format );
    
    if ( e == IRadSoundHalAudioFormat::RadicalAdpcm )
    {
        e = IRadSoundHalAudioFormat::PCM;    
    }
    
    unsigned int num_bytes              = rsd_file_length_in_bytes( ptr_rsd_file );
    unsigned int num_channels           = audio_format_channels( ptr_audio_format );
    unsigned int byte_resolution        = audio_format_bit_resolution( ptr_audio_format ) / 8;
    unsigned int frame_size             = g_audio_format_info[ e ].mono_8_bit_frame_size_bytes * num_channels * byte_resolution;

   
    rAssert( (num_bytes % frame_size) == 0 );
    
    unsigned int num_frames  = num_bytes / frame_size;
        
    return num_frames;    
   
}

unsigned int audio_format_frames_to_samples( const audio_format & ptr_audio_format, unsigned int frames )
{
    IRadSoundHalAudioFormat::Encoding e = (IRadSoundHalAudioFormat::Encoding) audio_format_encoding( ptr_audio_format );

    if ( e == IRadSoundHalAudioFormat::RadicalAdpcm )
    {
        e = IRadSoundHalAudioFormat::PCM;   
    }    
    
    unsigned int samples = g_audio_format_info[ e ].frame_size_samples * frames;
    
    return samples;
}


unsigned int audio_format_samples_to_frames( const audio_format & ptr_audio_format, unsigned int samples )
{
    IRadSoundHalAudioFormat::Encoding e = (IRadSoundHalAudioFormat::Encoding) audio_format_encoding( ptr_audio_format );

    if ( e == IRadSoundHalAudioFormat::RadicalAdpcm )
    {
        e = IRadSoundHalAudioFormat::PCM;   
    } 
    
    unsigned int frames = samples / g_audio_format_info[ e ].frame_size_samples;
        
    return frames;
}


unsigned int rsd_file_length_in_bytes( const rsd_file & ptr_rsd_file )
{
    audio_format af = rsd_file_audio_format( ptr_rsd_file );
    
    IRadSoundHalAudioFormat::Encoding e = (IRadSoundHalAudioFormat::Encoding) audio_format_encoding( af );
    
    unsigned int size = rsd_file_size( ptr_rsd_file ) - RSD_FILE_DATA_OFFSET;

    if ( e == IRadSoundHalAudioFormat::RadicalAdpcm )
    {
        unsigned int channels = audio_format_channels( af );
    
        unsigned int frames =  size / ( RADICAL_ADPCM_FRAME_SIZE * channels );
        
        unsigned int samples = frames * RADICAL_ADPCM_SAMPLES_PER_FRAME;
        
        unsigned int bytes = samples * channels * 2;
        
        size = bytes;
    } 
    
    return size;
}

bool composition_check_stream_constraints(
    const comp & ptr_composition,
    char * p_error_message,
    unsigned int len )
{
    strncpy( p_error_message, "", len );
    
    group ptr_group_1 = group_array_item_at( comp_root_groups( ptr_composition ), 0 );

    bool ok = true;
        
    group_check_stream_constraints_recursive( ptr_group_1, comp_stream_size_min( ptr_composition ), p_error_message, len, & ok );
    
    return ok;
}

radRef< IRadSoundHalAudioFormat > construct_audio_format( const audio_format & ptr_audio_format, bool targetFormat )
{
    radRef< IRadSoundHalAudioFormat > ref_af =
        radSoundHalAudioFormatCreate( memory::allocator( ) );

    IRadSoundHalAudioFormat::Encoding e = (IRadSoundHalAudioFormat::Encoding) audio_format_encoding( ptr_audio_format );
    
    if ( e == IRadSoundHalAudioFormat::RadicalAdpcm && targetFormat )
    {
        e = IRadSoundHalAudioFormat::PCM;
    }
                
    ref_af->Initialize(
        e,
        0,
        audio_format_sampling_rate( ptr_audio_format ),
        audio_format_channels( ptr_audio_format ),
        audio_format_bit_resolution( ptr_audio_format ) );
            
    return ref_af;
}

unsigned int audio_format_samples_to_milliseconds(
    const audio_format & ptr_audio_format,
    unsigned int samples )
{
    unsigned int sampling_rate = audio_format_sampling_rate( ptr_audio_format );
    
    float f_sampling_rate = radSoundUIntToFloat( sampling_rate );
    float f_samples       = radSoundUIntToFloat( samples );
    
    float f_ms = ( f_samples * 1000.0f ) / sampling_rate;
    
    return radSoundFloatToUInt( f_ms );
}


void group_memory_sizes( const group & ptr_group,
    unsigned int * p_num_sound_resources, unsigned int * p_sound_memory,
    unsigned int * p_num_cache_resources, unsigned int * p_cache_memory )
{
    clip_ref_array ptr_clip_ref_array = group_clip_refs( ptr_group );
    
    unsigned int num_clips = clip_ref_array_num_items( ptr_clip_ref_array );
    
    for( unsigned int c = 0; c < num_clips; c ++ )
    {
        ods::clip ptr_clip = clip_ref_array_item_at( ptr_clip_ref_array, c );
        
        (*p_num_sound_resources)++;
        unsigned int clip_size_in_bytes = rsd_file_length_in_bytes( clip_rsd_file_ref( ptr_clip ) );
        
        audio_format ptr_audio_format = rsd_file_audio_format( clip_rsd_file_ref( ptr_clip ) );
        
        unsigned int adjusted_size_in_bytes = ::radSoundHalBufferCalculateMemorySize(
                IRadSoundHalAudioFormat::Bytes,
                clip_size_in_bytes,
                IRadSoundHalAudioFormat::Bytes,
                construct_audio_format( ptr_audio_format, true ) );

        // Memory is aligned to this number, so we must allocate this much extra

        adjusted_size_in_bytes += radSoundHalDataSourceReadAlignmentGet( );

        *p_sound_memory += adjusted_size_in_bytes;
    }

    stream_ref_array ptr_stream_ref_array = group_stream_refs( ptr_group );
    unsigned int num_streams = stream_ref_array_num_items( ptr_stream_ref_array );

    for( unsigned int s = 0; s < num_streams; s ++ )
    {
        ods::stream ptr_stream = stream_ref_array_item_at( ptr_stream_ref_array, s );
        
        if ( false == stream_streamed( ptr_stream ) )
        {
            rsd_file ptr_rsd_file = stream_rsd_file_ref( ptr_stream );
            
            unsigned int cachedSize = rsd_file_length_in_bytes( ptr_rsd_file );

            *p_cache_memory += cachedSize + radSoundHalDataSourceReadAlignmentGet( );

            (*p_num_cache_resources)++;
        }
    }

    // Include parent resources!

    group ptr_group_parent = group_parent_ref( ptr_group );
    
    if ( not_null( ptr_group_parent ) )
    {
        group_memory_sizes( ptr_group_parent,
            p_num_sound_resources,
            p_sound_memory,
            p_num_cache_resources,
            p_cache_memory );
    }
}

fade_transition composition_find_fade_transition(
    const comp & ptr_composition,
    const region & ptr_region_source_in,
    const region & ptr_region_target_in )
{
    fade_transition_array ptr_fade_transition_array = comp_fade_transitions( ptr_composition );
    unsigned num_fade_transitions = fade_transition_array_num_items( ptr_fade_transition_array );
    
    fade_transition ptr_fade_transition;
    region ptr_region_source;
    region ptr_region_target;

    // Find Exact Transition
        
    for( unsigned int f = 0; f < num_fade_transitions; f ++ )
    {
        ptr_fade_transition = fade_transition_array_item_at( ptr_fade_transition_array, f );
        ptr_region_source   = fade_transition_source_region_ref( ptr_fade_transition );
        ptr_region_target   = fade_transition_target_region_ref( ptr_fade_transition );
        
        if ( ptr_region_source == ptr_region_source_in &&
             ptr_region_target == ptr_region_target_in )
        {
            return ptr_fade_transition;
        }
    }

    // Find Target Transition

    for( unsigned int f = 0; f < num_fade_transitions; f ++ )
    {
        ptr_fade_transition = fade_transition_array_item_at( ptr_fade_transition_array, f );
        ptr_region_source   = fade_transition_source_region_ref( ptr_fade_transition );
        ptr_region_target   = fade_transition_target_region_ref( ptr_fade_transition );
        
        if ( is_null( ptr_region_source ) && ptr_region_target == ptr_region_target_in )
        {
            return ptr_fade_transition;
        }
    }

    // Find Source Transition

    for( unsigned int f = 0; f < num_fade_transitions; f ++ )
    {
        ptr_fade_transition = fade_transition_array_item_at( ptr_fade_transition_array, f );
        ptr_region_source   = fade_transition_source_region_ref( ptr_fade_transition );
        ptr_region_target   = fade_transition_target_region_ref( ptr_fade_transition );
        
        if ( ptr_region_source == ptr_region_source_in && is_null( ptr_region_target ) )
        {
            return ptr_fade_transition;
        }
    }

    // Find generic
    
    for( unsigned int f = 0; f < num_fade_transitions; f ++ )
    {
        ptr_fade_transition = fade_transition_array_item_at( ptr_fade_transition_array, f );
        ptr_region_source   = fade_transition_source_region_ref( ptr_fade_transition );
        ptr_region_target   = fade_transition_target_region_ref( ptr_fade_transition );
        
        if ( is_null( ptr_region_source ) && is_null( ptr_region_target ) )
        {
            return ptr_fade_transition;
        }
    }
    
    return (fade_transition) pointer_null;
}

stitch_transition composition_find_stitch_transition(
    const comp & ptr_composition,
    const region & ptr_region_source_in,
    const region & ptr_region_target_in )
{
    stitch_transition_array ptr_stitch_transition_array = comp_stitch_transitions( ptr_composition );
    unsigned num_stitch_transitions = stitch_transition_array_num_items( ptr_stitch_transition_array );
    
    stitch_transition ptr_stitch_transition;
    region ptr_region_source;
    region ptr_region_target;

    // Find Exact Transition
        
    for( unsigned int f = 0; f < num_stitch_transitions; f ++ )
    {
        ptr_stitch_transition = stitch_transition_array_item_at( ptr_stitch_transition_array, f );
        ptr_region_source   = stitch_transition_source_region_ref( ptr_stitch_transition );
        ptr_region_target   = stitch_transition_target_region_ref( ptr_stitch_transition );
        
        if ( ptr_region_source == ptr_region_source_in &&
             ptr_region_target == ptr_region_target_in )
        {
            return ptr_stitch_transition;
        }
    }

    // Find Target Transition

    for( unsigned int f = 0; f < num_stitch_transitions; f ++ )
    {
        ptr_stitch_transition = stitch_transition_array_item_at( ptr_stitch_transition_array, f );
        ptr_region_source   = stitch_transition_source_region_ref( ptr_stitch_transition );
        ptr_region_target   = stitch_transition_target_region_ref( ptr_stitch_transition );
        
        if ( is_null( ptr_region_source ) && ptr_region_target == ptr_region_target_in )
        {
            return ptr_stitch_transition;
        }
    }

    // Find Source Transition

    for( unsigned int f = 0; f < num_stitch_transitions; f ++ )
    {
        ptr_stitch_transition = stitch_transition_array_item_at( ptr_stitch_transition_array, f );
        ptr_region_source   = stitch_transition_source_region_ref( ptr_stitch_transition );
        ptr_region_target   = stitch_transition_target_region_ref( ptr_stitch_transition );
        
        if ( ptr_region_source == ptr_region_source_in && is_null( ptr_region_target ) )
        {
            return ptr_stitch_transition;
        }
    }

    // Find generic
    
    for( unsigned int f = 0; f < num_stitch_transitions; f ++ )
    {
        ptr_stitch_transition = stitch_transition_array_item_at( ptr_stitch_transition_array, f );
        ptr_region_source   = stitch_transition_source_region_ref( ptr_stitch_transition );
        ptr_region_target   = stitch_transition_target_region_ref( ptr_stitch_transition );
        
        if ( is_null( ptr_region_source ) && is_null( ptr_region_target ) )
        {
            return ptr_stitch_transition;
        }
    }
    
    return (stitch_transition) pointer_null;
}

/* struct_region_controller * group_find_region_in_sub_tree( group * ptr_group const char * p_region_name )
{
    struct_region_controller * pRmr = m_refRadMusicRegion_Head;

    while ( pRmr != 0 )
    {
        if ( strcmp( pRmr->GetName( ), pName ) == 0 )
        {
            return pRmr;
        }

        pRmr = pRmr->GetGroupNext( );
    }

    Group * pGroup = m_refRadMusicGroup_Children_Head;  

    while ( pGroup != 0 )
    {
        pRmr = pGroup->FindRegionInSubTree( pName );

        if ( pRmr != 0 )
        {
            return pRmr;
        }

        pGroup = pGroup->GetGroupNext( );
    }

    return 0;
} */

bool group_is_child_of( const group & ptr_group, const group & ptr_parent )
{
    group ptr_group_search = ptr_group;   

    while( not_null( ptr_group_search ) )
    {
        if ( ptr_group_search == ptr_parent )
        {
            return true;
        }

        ptr_group_search = group_parent_ref( ptr_group_search );
    }

    return false;
}

bool beat_in_range( float beat, float last_beat, float current_beat )
{
    bool yes;

    if ( last_beat < current_beat )
    {
        yes = ( last_beat <= beat ) && ( current_beat >= beat );
    }
    else if ( last_beat > current_beat ) // wrap
    {
        if ( last_beat < beat )
        {                
            yes = ( last_beat <= beat ) && ( current_beat <= beat );
        }
        else
        {
            yes = ( last_beat >= beat ) && ( current_beat >= beat );
        }
    }
    else
    {
        yes = false;
    }

	return yes;
}


bool beat_set_beat_in_range(
    const beat_set & ptr_beat_set,
    float last_beat,
    float current_beat )
{
    float_array ptr_float_array = beat_set_beats( ptr_beat_set );
    
    unsigned int num_beats = float_array_num_items( ptr_float_array );
    
    if ( num_beats == 0 )
    {
        return true;
    }

    for( unsigned int i = 0; i < num_beats; i ++ )
    {
        float beat = float_array_item_at( ptr_float_array, i );
        
        if ( beat_in_range( beat, last_beat, current_beat ) )
        {   
            return true;
        }
    }

    return false;

}


float tempo_track_beat(
    const tempo_track & ptr_tempo_track,
    unsigned int int_current_sample,
    unsigned int int_sampling_rate )
{
    #if defined( RAD_WIN32 )
        static float game_loop_latency_ms = 60.0f;
    #elif defined( RAD_UWP )
        static float game_loop_latency_ms = 10.0f;
    #endif
    
    float current_sample = radSoundUIntToFloat( int_current_sample );
    float sampling_rate = radSoundUIntToFloat( int_sampling_rate );
    float bpm = tempo_track_bpm( ptr_tempo_track );  
      
    float denom =
        radSoundUIntToFloat(
            tempo_track_time_sig_denominator(
                ptr_tempo_track ) );
                
    float numerator =
        radSoundUIntToFloat(
            tempo_track_time_sig_numerator(
                ptr_tempo_track ) );
                
    bpm = ( bpm * denom ) / 4.0f;

    float game_loop_latency_in_samples = game_loop_latency_ms * sampling_rate / 1000.0f;

    float samples_per_beat = ( sampling_rate * 60.0f ) / ( bpm );                                            
    
    current_sample += game_loop_latency_in_samples;

    float beat = current_sample / samples_per_beat;
    beat += tempo_track_start_beat( ptr_tempo_track );
    beat = fmodf( beat, numerator );
   
    return beat;
}

unsigned int calculate_action_offset(
    const event & ptr_event,
    unsigned int * p_dimension_values )
{
    event_matrix ptr_event_matrix = event_event_matrix_ref( ptr_event );
    
    if ( is_null( ptr_event_matrix ) )
    {
        return 0;
    }
    
    state_ref_array ptr_state_ref_array = event_matrix_state_ref_array( ptr_event_matrix );
    unsigned int num_dims = state_ref_array_num_items( ptr_state_ref_array );
    
    unsigned int index = p_dimension_values[ 0 ];
    unsigned int dim = 1;

    for ( unsigned int i = 1; i < num_dims; i++ )
    {
        state ptr_state = state_ref_array_item_at( ptr_state_ref_array, i - 1);
        string_array ptr_string_array = state_values( ptr_state );
        unsigned int items = string_array_num_items( ptr_string_array );

        dim *= items;
        index += p_dimension_values[ i ] * dim;
    }
    
    return index;
}

} // namespace radmusic