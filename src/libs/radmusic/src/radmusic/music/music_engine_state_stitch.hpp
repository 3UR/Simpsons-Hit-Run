//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
///
//-----------------------------------------------------------------------------

#ifndef MUSIC_ENGINE_STATE_STITCH_HPP
#define MUSIC_ENGINE_STATE_STITCH_HPP

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
// Constants, Typedefs, and Macro Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Interface Definitions
//-----------------------------------------------------------------------------

namespace radmusic {

void state_stitch_construct( struct_music_engine * );
void state_stitch_destroy( struct_music_engine * );

void state_stitch_initialize( struct_music_engine *, ods::region & ptr_region_current, ods::region & ptr_region_target, ods::stitch_transition & ptr_stitch_transition );
bool state_stitch_initialized( struct_music_engine * );
void state_stitch_enter( struct_music_engine * );

void   state_stitch_enter( struct_music_engine * );
bool   state_stitch_set_target_region( struct_music_engine *, ods::region & ptr_target, bool reset_current_region );
ods::region state_stitch_current_region( struct_music_engine * );
void   state_stitch_update( struct_music_engine *, unsigned int ms );
void   state_stitch_kill( struct_music_engine * );
void   state_stitch_paused( struct_music_engine *, bool );
void   state_stitch_set_volume( struct_music_engine *, float );
void   state_stitch_set_pitch( struct_music_engine *, float );
void   state_stitch_start_layer( struct_music_engine *, const char * );
void   state_stitch_stop_layer( struct_music_engine *, const char * );

struct struct_state_stitch
{
    bool entered;
    
    ods::region ptr_region_transition;
    ods::region ptr_region_current;
    ods::region ptr_region_target;
    
    ods::stitch_transition ptr_stitch_transition;
};

}
#endif 