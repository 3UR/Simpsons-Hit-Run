
//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
///
//-----------------------------------------------------------------------------

#ifndef MUSIC_ENGINE_STATE_STOPPED_HPP
#define MUSIC_ENGINE_STATE_STOPPED_HPP

//-----------------------------------------------------------------------------
// Nested Includes
//-----------------------------------------------------------------------------

#include <radmusic/schema.hpp>

//-----------------------------------------------------------------------------
// Forward References
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constants, Typedefs, and Macro Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Interface Definitions
//-----------------------------------------------------------------------------

#include <radmusic/schema.hpp>

namespace radmusic {

struct struct_music_engine;

void   state_stopped_construct( struct_music_engine * );
void   state_stopped_destroy( struct_music_engine * );

void   state_stopped_enter( struct_music_engine * );
bool   state_stopped_set_target_region( struct_music_engine *, ods::region & ptr_target, bool reset_current_region );
ods::region state_stopped_current_region( struct_music_engine * );
void   state_stopped_update( struct_music_engine *, unsigned int ms );
void   state_stopped_kill( struct_music_engine * );
void   state_stopped_paused( struct_music_engine *, bool );
void   state_stopped_set_volume( struct_music_engine *, float );
void   state_stopped_set_pitch( struct_music_engine *, float );
void   state_stopped_start_layer( struct_music_engine *, const char * );
void   state_stopped_stop_layer( struct_music_engine *, const char * );

enum enum_state_stopped_state {
    state_state_stopped_idle,
    state_state_stopped_loading,
    state_state_stopped_queueing };
    
struct struct_state_stopped
{
    enum_state_stopped_state state;
    bool reset_target_region;
    ods::region ptr_region_target;
};

}
#endif