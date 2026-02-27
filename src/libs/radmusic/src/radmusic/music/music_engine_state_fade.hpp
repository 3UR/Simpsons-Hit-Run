//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
///
//-----------------------------------------------------------------------------

#ifndef MUSIC_ENGINE_STATE_FADE_HPP
#define MUSIC_ENGINE_STATE_FADE_HPP

//-----------------------------------------------------------------------------
// Nested Includes
//-----------------------------------------------------------------------------

#include <radmusic/schema.hpp>

//-----------------------------------------------------------------------------
// Forward References
//-----------------------------------------------------------------------------

namespace radmusic
{
    struct struct_music_engine;
}
    
//-----------------------------------------------------------------------------
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Interface Definitions
//-----------------------------------------------------------------------------

namespace radmusic {

void   state_fade_construct( struct_music_engine * );
void   state_fade_destroy( struct_music_engine * );
void   state_fade_initialize( struct_music_engine *, ods::region & ptr_current_region, bool reset_target, ods::region & ptr_region_target, ods::fade_transition & ptr_fade_transition );
void   state_fade_enter( struct_music_engine * );
void   state_fade_cancel_init( struct_music_engine * );
bool   state_fade_initialized( struct_music_engine * );

bool   state_fade_set_target_region( struct_music_engine *, ods::region & ptr_target, bool reset_current_region );
ods::region state_fade_current_region( struct_music_engine * );
void   state_fade_update( struct_music_engine *, unsigned int ms );
void   state_fade_kill( struct_music_engine * );
void   state_fade_paused( struct_music_engine *, bool );
void   state_fade_set_volume( struct_music_engine *, float );
void   state_fade_set_pitch( struct_music_engine *, float );
void   state_fade_start_layer( struct_music_engine *, const char * );
void   state_fade_stop_layer( struct_music_engine *, const char * );

ods::fade_transition state_fade_fade_transition( struct_music_engine * );
ods::region          state_fade_source_region ( struct_music_engine * );
ods::region          fade_State_target_region ( struct_music_engine * );

enum enum_state_fade_state {
    state_fade_state_idle,
    state_fade_state_init_region,
    state_fade_state_init_stream,
    state_fade_state_initialized,
    state_fade_state_entered };
    
struct struct_state_fade
{
    enum_state_fade_state state;

	float current_source_volume;
	float current_target_volume;

    unsigned int running_time;
	float        last_beat;

    ods::region ptr_region_target;
    ods::region ptr_region_source;
    
    bool reset_target_region;

    ods::fade_transition ptr_fade_transition;
};

    
}
#endif
