//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
/// 
//-----------------------------------------------------------------------------

#ifndef SEQUENCE_PLAYER_HPP
#define SEQUENCE_PLAYER_HPP

//-----------------------------------------------------------------------------
// Nested Includes
//-----------------------------------------------------------------------------

#include <radmusic/radmusic.hpp>
#include <radmusic/sequence/sequence_stream_graph.hpp>
#include <radmusic/resource/resource_manager.hpp>
#include <radobjectlist.hpp>

//-----------------------------------------------------------------------------
// Forward References
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Interface Definitions
//-----------------------------------------------------------------------------

namespace radmusic {

struct sequence_player_command
{     
    ods::sequence_event ptr_sequence_event;
  
    bool enter;
    
    union {                   
        
        struct
        {           
            unsigned int index;
        } and_info;              
        
        struct
        {
            unsigned int times;                    
            unsigned int target_times;                
        } repeat_info;             
    };
};
         
struct sequence_player_event
{
    ods::sequence_event ptr_sequence_event;

    union
    {
        struct
        {
            bool           started;
            unsigned int   current_sample;
        } clip;
           
        struct
        {
            unsigned int   start_sample;
            unsigned int   queued_sample;
            unsigned int   current_sample;                    
        } stream;
        
        struct
        {
            bool         started; 
            unsigned int current_time;          
            float        time;
            unsigned int last_time;           
        } silence;
    };
    
};

typedef unsigned char event_queue_type;
typedef unsigned char command_stack_type;

const event_queue_type   g_event_queue_size   = 8;

#ifdef SEQUENCE_PLAYER_NAMES
    const unsigned char       g_sequence_player_name_chars = 64;
#endif

struct sequence_player
{
    #ifdef SEQUENCE_PLAYER_NAMES
        char                    name[ g_sequence_player_name_chars ];
    #endif
    
    sequence_player_command * p_command_stack;    
    command_stack_type      command_stack_pointer;    
    
    sequence_player_event   event_queue[ g_event_queue_size ];
    event_queue_type        event_queue_items;
    event_queue_type        event_queue_head;
    event_queue_type        event_queue_tail;
    event_queue_type        event_queue_window_items;
    
    float                   beat;
    
    unsigned int            resource_buffer;
    
    unsigned char           master_volume;
    unsigned short          master_pitch;
 
    unsigned char           volume_rand_current;     
    unsigned short          pitch_rand_current;
                        
    unsigned char           var_volume;
    unsigned short          var_pitch;
    unsigned char           var_volume_rand_min;
    unsigned char           var_volume_rand_max;
    unsigned short          var_pitch_rand_min;
    unsigned short          var_pitch_rand_max;    
    unsigned char           var_aux_gains[ 2 ];    
    bool                    var_positional;
    float                   var_pos_fall_off;
    float                   var_pos_dist_min;
    float                   var_pos_dist_max;


    enum_sequence_player_state     state;
    bool                           paused;       

    IRadSoundClipPlayer *         p_clip_player;
    stream_graph *                p_stream_graph;
    IRadSoundHalPositionalGroup * p_positional_group;         

    ods::sequence                 ptr_sequence;
    resource_manager *     p_resource_manager;
};

}

#endif