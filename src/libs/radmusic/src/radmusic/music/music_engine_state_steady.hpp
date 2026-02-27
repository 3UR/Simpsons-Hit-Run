//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
///
//-----------------------------------------------------------------------------

#ifndef MUSIC_ENGINE_STATE_STEADY_HPP
#define MUSIC_ENGINE_STATE_STEADY_HPP

//-----------------------------------------------------------------------------
// Nested Includes
//-----------------------------------------------------------------------------

#include <radmusic/schema.hpp>

//-----------------------------------------------------------------------------
// Forward References
//-----------------------------------------------------------------------------

namespace radmusic
{
    struct struct_state_fade;
    struct struct_state_stitch;
    struct struct_music_engine;
}

//-----------------------------------------------------------------------------
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Interface Definitions
//-----------------------------------------------------------------------------

namespace radmusic {

void state_steady_construct( struct_music_engine * );
void state_steady_destroy( struct_music_engine * );
void state_steady_initialize( struct_music_engine *, ods::region & ptr_region_current );
void state_steady_enter( struct_music_engine * );

void   state_steady_enter( struct_music_engine * );
bool   state_steady_set_target_region( struct_music_engine *, ods::region & ptr_target, bool reset_current_region );
ods::region state_steady_current_region( struct_music_engine * );
void   state_steady_update( struct_music_engine *, unsigned int ms );
void   state_steady_kill( struct_music_engine * );
void   state_steady_paused( struct_music_engine *, bool );
void   state_steady_set_volume( struct_music_engine *, float );
void   state_steady_set_pitch( struct_music_engine *, float );
void   state_steady_start_layer( struct_music_engine *, const char * );
void   state_steady_stop_layer( struct_music_engine *, const char * );
bool   state_steady_is_idle( struct_music_engine * );

enum enum_state_steady_transition_type {
    state_steady_transition_type_stop,
    state_steady_transition_type_switch,
    state_steady_transition_type_pop,
    state_steady_transition_type_exit };
    
struct struct_state_steady
{
    enum_state_steady_transition_type transition_type;
    bool entered;
    
    struct_state_stitch * p_state_stitch;
    struct_state_fade   * p_state_fade;

    ods::region ptr_region_current;
    ods::region ptr_region_target;
};

}
#endif