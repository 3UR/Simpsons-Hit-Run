//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
/// 
/// This is the comment from the template source file.  If you can't
/// think if anything useful to add to what you put in the header file,
/// to describe this class then delete this comment.
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "pch.hpp"
#include <radmusic/sequence/sequence_stream_graph.hpp>

//-----------------------------------------------------------------------------
// Local Constants, Typedefs, and Macros
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Global Data, Local Data, Local Classes
//-----------------------------------------------------------------------------

namespace radmusic 
{
    static unsigned int _dequeue(     stream_graph *, IRadSoundHalDataSource ** );
    static void         _flush_queue( stream_graph * );
    static void         _on_stitch(   stream_graph *, IRadSoundHalDataSource **, unsigned int );
}

//-----------------------------------------------------------------------------
// Member Functions
//-----------------------------------------------------------------------------

namespace radmusic {

//============================================================================
// global objects
//============================================================================

//----------------------------------------------------------------------------
// struct : stream_graph_callback
//
// description: this object handles all stitch callbacks for all ods::stream graphs
//
//----------------------------------------------------------------------------

struct stream_graph_callback : public IRadSoundStitchCallback
{
    virtual void AddRef( void ) { }
    virtual void Release( void ) { }
    
    virtual void OnStitch( IRadSoundHalDataSource ** pp_ds, unsigned int frame_count,  void * p_user_data )
    {
        stream_graph * p_sg = (stream_graph*) p_user_data;
        _on_stitch( p_sg, pp_ds, frame_count );
    }
} g_stream_graph_stitch_callback;

//============================================================================
//
// public function definitions
//
//============================================================================

//----------------------------------------------------------------------------

void stream_graph_construct(
    stream_graph * p_sg,
    IRadSoundHalAudioFormat * p_audio_format,
    bool                      use_bufered_data_source,
    radMemorySpace            bds_memory_space,
    IRadMemoryAllocator *     p_bds_allocator,
    unsigned int              bds_size_ms,
    unsigned int              stream_buffer_size_ms,
    radMemoryAllocator allocator )
{
    p_sg->queue_items              = 0;
    p_sg->queue_head               = 0;
    p_sg->state                    = stream_graph_state_dequeued;
    p_sg->paused                   = false;
    p_sg->next_stitch_position     = 0;
  
    p_sg->p_stitched_data_source = ::radSoundStitchedDataSourceCreate( allocator );
    p_sg->p_stitched_data_source->AddRef( );
	p_sg->p_stitched_data_source->InitializeFromAudioFormat( p_audio_format );
	p_sg->p_stitched_data_source->SetStitchCallback( & g_stream_graph_stitch_callback, p_sg );

    if ( use_bufered_data_source == true )
    {
	    p_sg->p_buffered_data_source = ::radSoundBufferedDataSourceCreate( allocator );
        p_sg->p_buffered_data_source->AddRef( );	    
	    p_sg->p_buffered_data_source->Initialize(
		    bds_memory_space,
		    p_bds_allocator,
		    bds_size_ms,
		    IRadSoundHalAudioFormat::Milliseconds,
            p_audio_format,
            "ods::stream graph buffered data source" );
    }
    else
    {
        p_sg->p_buffered_data_source = 0;
    }

	p_sg->p_stream_player = ::radSoundStreamPlayerCreate( allocator );
    p_sg->p_stream_player->AddRef( );
	p_sg->p_stream_player->Initialize(
        p_audio_format,
        stream_buffer_size_ms,
		IRadSoundHalAudioFormat::Milliseconds,
		::radSoundHalSystemGet( )->GetRootMemoryRegion( ),
        "ods::stream graph ods::stream player" );
        
    p_sg->p_stream_player->SetPriority( 10 );
}

//----------------------------------------------------------------------------

void stream_graph_destroy( stream_graph * p_sg )
{
    _flush_queue( p_sg );
    
    p_sg->p_stitched_data_source->SetStitchCallback( 0, 0 );
    p_sg->p_stitched_data_source->Release( );
    p_sg->p_stream_player->Release( );
    
    if ( p_sg->p_buffered_data_source != 0 )
    {
        p_sg->p_buffered_data_source->Release( );
    }
}

//----------------------------------------------------------------------------

enum_stream_graph_state stream_graph_state( stream_graph * p_sg )
{
    if ( p_sg->state == stream_graph_state_queueing )
    {
        if ( p_sg->p_stream_player->GetState( ) == IRadSoundStreamPlayer::Paused )
        {
            p_sg->state = stream_graph_state_queued;
        }
        else
        {
            rAssert( p_sg->p_stream_player->GetState( ) == IRadSoundStreamPlayer::Queueing );
        }
    }
    else if ( p_sg->state == stream_graph_state_running )
    {
        if ( p_sg->p_stream_player->GetState( ) == IRadSoundStreamPlayer::NoSource )
        {
            p_sg->state = stream_graph_state_finished;
        }
        else
        {
            rAssert( p_sg->p_stream_player->GetState( ) == IRadSoundStreamPlayer::Paused ||
                     p_sg->p_stream_player->GetState( ) == IRadSoundStreamPlayer::Playing );
        }
    }
    
    return p_sg->state;
}

//----------------------------------------------------------------------------

void stream_graph_paused( stream_graph * p_sg, bool paused )
{
    if ( paused != p_sg->paused )
    {
        p_sg->paused = paused;
        
        if ( p_sg->paused == true )
        {
            p_sg->p_stream_player->Stop( );        
        }
        else
        {
            p_sg->p_stream_player->Play( );
        }
    }
}

//----------------------------------------------------------------------------

unsigned int stream_graph_queue_data_source(
    stream_graph * p_sg,
    IRadSoundHalDataSource * p_data_source,
    unsigned int length )
{
    rAssert( p_sg->queue_items < STREAM_GRAPH_QUEUE_SIZE );
 
    unsigned int stitch_pos = p_sg->next_stitch_position;
    
    unsigned char tail_pos = ( p_sg->queue_head + p_sg->queue_items ) % STREAM_GRAPH_QUEUE_SIZE;
    
    p_sg->queue[ tail_pos ].p_data_source = p_data_source;
    p_sg->queue[ tail_pos ].stitch_pos    = stitch_pos;  
    p_sg->queue[ tail_pos ].length        = length;
    
    p_sg->next_stitch_position += length;
    
    if ( p_data_source != 0 )
    {
        p_data_source->AddRef( );
    }

    p_sg->queue_items ++;
    
    return stitch_pos;
}

//----------------------------------------------------------------------------

bool stream_graph_queue_full( stream_graph * p_sg )
{
    return p_sg->queue_items >= STREAM_GRAPH_QUEUE_SIZE;
}

//----------------------------------------------------------------------------

void stream_graph_queue_flush( stream_graph * p_sg )
{
    return _flush_queue( p_sg );
}

//----------------------------------------------------------------------------

void stream_graph_queue( stream_graph * p_sg )
{
    p_sg->state = stream_graph_state_queueing;

	p_sg->p_stitched_data_source->Reset( );

    if ( p_sg->p_buffered_data_source != 0 )
    {
    	p_sg->p_buffered_data_source->SetInputDataSource( p_sg->p_stitched_data_source );
        p_sg->p_stream_player->SetDataSource( p_sg->p_buffered_data_source );
    }
    else
    {
        p_sg->p_stream_player->SetDataSource( p_sg->p_stitched_data_source );
    }
    
    radSoundHalSystemGet( )->Service( );
}

//----------------------------------------------------------------------------

void stream_graph_run( stream_graph * p_sg )
{
    rAssert( p_sg->state == stream_graph_state_queued );
    
    p_sg->state = stream_graph_state_running;

    if ( p_sg->paused == false )
    {
        p_sg->p_stream_player->Play( );
    }

}

//----------------------------------------------------------------------------

void stream_graph_dequeue( stream_graph * p_sg )
{
    p_sg->state = stream_graph_state_dequeued;
       
    p_sg->p_stream_player->SetDataSource( 0 );
    p_sg->p_stitched_data_source->Reset( );

    if ( p_sg->p_buffered_data_source != 0 )
    {
        p_sg->p_buffered_data_source->SetInputDataSource( 0 );
    }
        
    _flush_queue( p_sg );

    p_sg->next_stitch_position = 0;   
}

//----------------------------------------------------------------------------

void stream_graph_volume( stream_graph * p_sg, float volume )
{
    p_sg->p_stream_player->SetVolume( volume );
}

//----------------------------------------------------------------------------

void stream_graph_pitch( stream_graph * p_sg, float pitch )
{
    p_sg->p_stream_player->SetPitch( pitch );
}

//----------------------------------------------------------------------------

void stream_graph_aux( stream_graph * p_sg, unsigned int an, float g )
{
    p_sg->p_stream_player->SetAuxGain( an, g );
}

//----------------------------------------------------------------------------

void stream_graph_positional_group(
    stream_graph * p_sg, IRadSoundHalPositionalGroup * p_pg )
{
    p_sg->p_stream_player->SetPositionalGroup( p_pg );
}


//----------------------------------------------------------------------------

unsigned int stream_graph_position( stream_graph * p_sg )
{
    rAssert( p_sg->state != stream_graph_state_dequeued );

    return p_sg->p_stream_player->GetPlaybackTimeInSamples( );
}

//----------------------------------------------------------------------------

IRadSoundHalAudioFormat * stream_graph_format( stream_graph * p_sg )
{
    return p_sg->p_stream_player->GetFormat( );
}

//============================================================================
//
// private function definitions
//
//============================================================================

//----------------------------------------------------------------------------

static void _on_stitch( stream_graph * p_sg, IRadSoundHalDataSource ** pp_ds, unsigned int frame_count)
{
    rAssert( p_sg->state != stream_graph_state_finished &&
             p_sg->state != stream_graph_state_dequeued );

    if ( p_sg->queue_items > 0 )
    {    
        unsigned int pos = _dequeue( p_sg, pp_ds );
        
        rAssert( pos == frame_count );
    }
    else
    {
        *pp_ds = 0;
    }
    
    if ( *pp_ds == 0 )
    {
        p_sg->next_stitch_position = 0;
    }
}

//----------------------------------------------------------------------------

static unsigned int  _dequeue( stream_graph * p_sg, IRadSoundHalDataSource ** pp_data_source )
{
    if ( p_sg->queue_items == 0 )
    {
        *pp_data_source = 0;
    }
    
    *pp_data_source  = p_sg->queue[ p_sg->queue_head ].p_data_source;
    unsigned int pos = p_sg->queue[ p_sg->queue_head ].stitch_pos;
    
    p_sg->queue[ p_sg->queue_head ].p_data_source = 0;
    p_sg->queue[ p_sg->queue_head ].length = 0;
    p_sg->queue[ p_sg->queue_head ].stitch_pos = 0;    
    
    p_sg->queue_head ++;
    
    if ( p_sg->queue_head >= STREAM_GRAPH_QUEUE_SIZE )
    {
        p_sg->queue_head = 0;
    }
    
    p_sg->queue_items --;
    
    return pos;
        
    
}

//----------------------------------------------------------------------------

static void _flush_queue( stream_graph * p_sg )
{
    while( p_sg->queue_items > 0 )
    {
        struct_queue_item * p_item =
            p_sg->queue + ( ( p_sg->queue_head + p_sg->queue_items - 1) % STREAM_GRAPH_QUEUE_SIZE );
        
        if ( p_sg->next_stitch_position > 0 )
        {
            rAssert( p_sg->next_stitch_position >= p_item->length );
            p_sg->next_stitch_position -= p_item->length;
        }
        
        p_item->length = 0;
        p_item->stitch_pos = 0;
        
        if ( p_item->p_data_source != 0 )
        {
            p_item->p_data_source->Release( );
            p_item->p_data_source = 0;
        }
 
        p_sg->queue_items--;
    }
}
                
}