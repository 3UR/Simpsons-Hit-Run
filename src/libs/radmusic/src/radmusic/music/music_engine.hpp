//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
///
//-----------------------------------------------------------------------------

#ifndef MUSIC_ENGINE_HPP
#define MUSIC_ENGINE_HPP

//-----------------------------------------------------------------------------
// Nested Includes
//-----------------------------------------------------------------------------

#include <radsound.hpp>
#include "radmusic/music/music_engine.hpp"
#include "radmusic/music/music_engine_state_stopped.hpp"
#include "radmusic/music/music_engine_state_fade.hpp"
#include "radmusic/music/music_engine_state_stitch.hpp"
#include "radmusic/music/music_engine_state_steady.hpp"
#include "radmusic/schema/schema_util.hpp"
#include "radmusic/sequence/sequence_region.hpp"
#include "radmusic/sequence/sequence_stream_graph.hpp"

//-----------------------------------------------------------------------------
// Forward References
//-----------------------------------------------------------------------------

namespace radmusic {

struct performance;

//-----------------------------------------------------------------------------
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Interface Definitions
//-----------------------------------------------------------------------------

void  music_engine_construct( struct_music_engine *, performance * );
void  music_engine_destroy( struct_music_engine * );
void  music_engine_stop( struct_music_engine * );
bool  music_engine_paused( struct_music_engine * );
void  music_engine_paused( struct_music_engine *, bool );
void  music_engine_update( struct_music_engine *, unsigned int ms );
void  music_engine_volume( struct_music_engine *, float );
float music_engine_volume( struct_music_engine *);
void  music_engine_pitch( struct_music_engine *, float );
float music_engine_pitch( struct_music_engine * );

void  music_engine_do_action( struct_music_engine *, const ods::action & );

void  music_engine_do_region_switch( struct_music_engine *, ods::region & ptr_region, enum_region_resume_type );
void  music_engine_do_region_push( struct_music_engine *, ods::region & ptr_region, enum_region_resume_type current, enum_region_resume_type target);
void  music_engine_do_region_pop( struct_music_engine *, ods::region & ptr_region );

void   music_engine_flush_region_stack( struct_music_engine * );
void   music_engine_push_region( struct_music_engine *, ods::region & ptr_region, enum_region_resume_type * );
void   music_engine_pop_region( struct_music_engine *, ods::region * ptr_region, enum_region_resume_type * );
ods::region music_engine_peek_region( struct_music_engine *, enum_region_resume_type * );

void  music_engine_current_region_name( struct_music_engine *, char * p_string, unsigned int len );
bool  music_engine_current_beat( struct_music_engine *, float * p_last_beat );

bool music_engine_is_state_steady_and_idle( struct_music_engine * );


performance * music_engine_performance( struct_music_engine * );
struct_region_controller * music_engine_region_controller( struct_music_engine *, const ods::region & ptr_region );

enum enum_music_engine_state
{
    music_engine_state_stopped,
    music_engine_state_steady,
    music_engine_state_fade,
    music_engine_state_stitch,
    music_engine_state_max
};

struct struct_region_stack_item
{
    ods::region ptr_region;
    enum_region_resume_type region_resume_type;
};

struct struct_music_engine
{
    performance *         p_performance;
    
    struct_state_fade            state_fade;       
    struct_state_stopped         state_stopped;
    struct_state_steady          state_steady;
    struct_state_stitch          state_stitch;
    enum_music_engine_state current_state;

	float volume;
    float pitch;
	bool  paused;
        
    #define REGION_STACK_SIZE 100

    struct_region_stack_item region_stack[ REGION_STACK_SIZE ];
    unsigned int             region_stack_pointer;
    
    ods::action ptr_action_on_deck;

    struct_region_controller * p_region_controllers;		
};



}

#endif // SEQUENCEENGINE_HPP