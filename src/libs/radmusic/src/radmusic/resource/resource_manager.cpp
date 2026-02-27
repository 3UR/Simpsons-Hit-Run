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
#include <radmusic/resource/resource_manager.hpp>
#include <radmusic/schema/schema_util.hpp>
#include <memory/memory.hpp>
#include <radmemorymonitor.hpp>

//-----------------------------------------------------------------------------
// Local Constants, Typedefs, and Macros
//-----------------------------------------------------------------------------

using namespace ods;

namespace radmusic {

enum enum_resource_buffer_state {
    resource_buffer_state_idle,
    resource_buffer_state_unloading,
    resource_buffer_state_loading };
    
//-----------------------------------------------------------------------------
// Global Data, Local Data, Local Classes
//-----------------------------------------------------------------------------

#if defined( RAD_WIN32 ) || defined( RAD_UWP )
	#define PLATFORM_AUDIO_FORMAT IRadSoundHalAudioFormat::PCM
#endif
	
const unsigned char g_max_stream_graphs = 64;

struct struct_stream_graph_node
{
    radmusic::stream_graph stream_graph;
    bool free;
} g_p_stream_graph_nodes[ g_max_stream_graphs ];

struct clip_player_node
{
    IRadSoundClipPlayer * p_clip_player;
    bool free;    
} * g_p_clip_player_nodes;

struct resource_buffer_clip
{
    IRadSoundClip * p_IRadSoundclip;
    ods::clip            ptr_clip;
};

struct resource_buffer_data_cache
{
    IRadSoundDataCache * p_IRadSoundDataCache;
    ods::stream               ptr_stream;
};
    
struct resource_buffer
{
    IRadSoundMemorySpaceObject * p_IRadSoundMemorySpaceObject_CacheHeap;
    IRadSoundHalMemoryRegion   * p_IRadSoundHalMemoryRegion;
    IRadMemoryHeap             * p_IRadMemoryHeap_Cache;
    
    ods::group ptr_group_current;
    ods::group ptr_group_on_deck;

    resource_buffer_clip       * p_clips;
    resource_buffer_data_cache * p_data_caches;
    
    unsigned int clip_stack_current_pointer;
    unsigned int clip_stack_unload_pointer;    
    unsigned int clip_stack_load_pointer;

    unsigned int cache_stack_current_pointer;
    unsigned int cache_stack_unload_pointer;
    unsigned int cache_stack_load_pointer;
    
    enum_resource_buffer_state state;
};
    
struct resource_manager
{
    radMemoryAllocator ai_allocator;
    
    char search_path[ 128 ];

    unsigned int number_of_sound_resources;
    unsigned int number_of_cache_resources;
  
    resource_buffer resource_buffers[ 2 ];
};


unsigned char g_num_stream_graphs;
unsigned char g_num_clip_players;

//-----------------------------------------------------------------------------
// Member Functions
//-----------------------------------------------------------------------------

void _resource_buffer_move_to_group(
    resource_manager * p_rm,
    unsigned int buffer_number,
    const group & ptr_group_target );

void resource_manager_initialize(
    stream_graph_description * p_sgd,
    unsigned int num_sg,
    unsigned int num_cp )
{
    rAssert( g_num_stream_graphs <= g_max_stream_graphs );
        
    g_num_stream_graphs  = num_sg;
    g_num_clip_players   = num_cp;
    
    g_p_clip_player_nodes = (clip_player_node*)
        memory::malloc( sizeof( clip_player_node) * g_num_clip_players, "clip_player_nodes" );
    
    for( unsigned int sg = 0; sg < g_num_stream_graphs; sg ++ )
    {
        stream_graph_description * p_d = p_sgd + sg;
        struct_stream_graph_node        * p_n = g_p_stream_graph_nodes + sg;
        
        p_n->free = true;
        
	    IRadSoundHalAudioFormat * p_af = ::radSoundHalAudioFormatCreate( memory::allocator( ) );
	    p_af->AddRef( );
    
        p_af->Initialize(
            PLATFORM_AUDIO_FORMAT, 
		    0,
		    p_d->sampling_rate,
		    p_d->channels,
		    16 );
		    
        stream_graph_construct( & p_n->stream_graph,
            p_af,
            p_d->use_buffered_data_source,
            p_d->buffered_data_source_space,
            radMemorySpaceGetAllocator( p_d->buffered_data_source_space, memory::allocator( ) ),
            p_d->buffered_data_source_size_in_ms,
            p_d->stream_buffer_size_in_ms,
            memory::allocator( ) );
            
        p_af->Release( );
    }
    
    for( unsigned int cp = 0; cp < g_num_clip_players; cp ++ )
    {
        clip_player_node * p_n = g_p_clip_player_nodes + cp;
        
        p_n->free = true;
        p_n->p_clip_player = radSoundClipPlayerCreate( memory::allocator( ) );
        p_n->p_clip_player->AddRef( );
        
    }
}

void resource_manager_terminate( void )
{    
    for( unsigned int sg = 0; sg < g_num_stream_graphs; sg ++ )
    {
        rAssert( g_p_stream_graph_nodes[ sg ].free == true );   
        stream_graph_destroy( & g_p_stream_graph_nodes[ sg ].stream_graph );
    }
    
    for( unsigned int cp = 0; cp < g_num_clip_players; cp ++ )
    {
        rAssert( g_p_clip_player_nodes[ cp ].free == true );
        g_p_clip_player_nodes[ cp ].p_clip_player->Release( );
    }
 
    memory::free( g_p_clip_player_nodes );
    
    g_num_stream_graphs = 0;
    g_num_clip_players  = 0; 
}

void capture_stream_graph(
    stream_graph ** pp_sg, IRadSoundHalAudioFormat * p_af )
{
    rAssert( *pp_sg == 0 );
    
    for( unsigned int sg = 0; sg < g_num_stream_graphs; sg ++ )
    {
        if ( g_p_stream_graph_nodes[ sg ].free == true )
        {
            IRadSoundHalAudioFormat * p_sgaf =
                stream_graph_format( & g_p_stream_graph_nodes[ sg ].stream_graph );
                
            if ( p_sgaf->Matches( p_af ) )
            {
                g_p_stream_graph_nodes[ sg ].free = false;     
        
                *pp_sg = & g_p_stream_graph_nodes[ sg ].stream_graph;
                return;
            }
        }
    }
    
    rDebugPrintf( "radmusic [warning]: Dynamically creating stream_graph!\n" );
    
    rAssert( g_num_stream_graphs < g_max_stream_graphs );
    
    g_p_stream_graph_nodes[ g_num_stream_graphs ].free = false;
	        
    stream_graph_construct(
        & g_p_stream_graph_nodes[ g_num_stream_graphs ].stream_graph,
        p_af,
        false,
        radMemorySpace_Local,
        radMemoryGetAllocator( memory::allocator( ) ),
        0,
        6000,
        memory::allocator( ) );
    
    *pp_sg = & g_p_stream_graph_nodes[ g_num_stream_graphs ].stream_graph;
    
    g_num_stream_graphs++;
}

void release_stream_graph( stream_graph ** pp_sg )
{
    rAssert( *pp_sg != 0 );
    
    struct_stream_graph_node * p_sgn =
        (struct_stream_graph_node *) *pp_sg;
    
    rAssert( p_sgn->free == false );
    p_sgn->free = true;
    stream_graph_positional_group( & p_sgn->stream_graph, 0 );
    stream_graph_paused( & p_sgn->stream_graph, false );    
    *pp_sg = 0;
    
}

void capture_clip_player( IRadSoundClipPlayer ** pp_cp )
{
    rAssert( *pp_cp == 0 );
    
    for( unsigned int sg = 0; sg < g_num_clip_players; sg ++ )
    {
        if ( g_p_clip_player_nodes[ sg ].free == true )
        {
            g_p_clip_player_nodes[ sg ].free = false;     
        
            *pp_cp = g_p_clip_player_nodes[ sg ].p_clip_player;
            return;
        }
    }
    
    rAssertMsg( false, "Out of ods::stream graphs!" );
    
    *pp_cp = 0;
}

void release_clip_player( IRadSoundClipPlayer ** pp_cp)
{
    rAssert( *pp_cp != 0 );
    
    for( unsigned int sg = 0; sg < g_num_clip_players; sg ++ )
    {
        if ( g_p_clip_player_nodes[ sg ].p_clip_player == *pp_cp )
        {
            rAssert( g_p_clip_player_nodes[ sg ].free == false );
            g_p_clip_player_nodes[ sg ].free = true; 
            g_p_clip_player_nodes[ sg ].p_clip_player->SetPositionalGroup( 0 );
            g_p_clip_player_nodes[ sg ].p_clip_player->SetClip( 0 );
            g_p_clip_player_nodes[ sg ].p_clip_player->Stop( );
                      
            *pp_cp = 0;
            return;
        }
    }
    
    rAssert( false );    
}


resource_manager * resource_manager_new(
    const char * p_search_path,    
    radMemoryAllocator ai_allocator,
    unsigned int number_of_sound_resources,
    unsigned int sound_resource_buffer_size,
    radMemorySpace cache_memory_space,
    radMemoryAllocator cache_allocator,
    unsigned int number_of_cache_resources,
    unsigned int cache_resource_buffer_size )
{
    resource_manager * p_rm = (resource_manager*)
        memory::malloc(
            sizeof( resource_manager ),
            "resource_manager" );
    
    strncpy( p_rm->search_path, p_search_path, 128 );
    p_rm->ai_allocator = ai_allocator;
    p_rm->number_of_sound_resources = number_of_sound_resources;
    p_rm->number_of_cache_resources = number_of_cache_resources;    
           
    for( unsigned int b = 0; b < 2; b ++ )
    {
        resource_buffer * p_rb = & p_rm->resource_buffers[ b ];

        pointer_construct( (pointer*) & p_rb->ptr_group_current );
        pointer_construct( (pointer*) & p_rb->ptr_group_on_deck );
    
        p_rb->p_clips = (resource_buffer_clip*) memory::malloc(
            sizeof( resource_buffer_clip ) * number_of_sound_resources,
                "resource_buffer_clip_stack" );
        
        for( unsigned int c = 0; c < number_of_sound_resources; c ++ )
        {
            p_rb->p_clips[ c ].p_IRadSoundclip = 0;
            pointer_construct( (pointer*) & p_rb->p_clips[ c ].ptr_clip );
        }
        
        p_rb->clip_stack_current_pointer = 0;        
        p_rb->clip_stack_unload_pointer  = 0;
        p_rb->clip_stack_load_pointer    = 0;
        
        p_rb->p_data_caches = (resource_buffer_data_cache*) memory::malloc(
            sizeof( resource_buffer_data_cache ) * number_of_cache_resources,
                "resource_buffer_data_cache_stack" );
        
        for( unsigned int c = 0; c < number_of_cache_resources; c ++ )
        {
            p_rb->p_data_caches[ c ].p_IRadSoundDataCache = 0;
            pointer_construct( (pointer*) & p_rb->p_data_caches[ c ].ptr_stream );
        }
        
        p_rb->cache_stack_current_pointer = 0;
        p_rb->cache_stack_unload_pointer  = 0;        
        p_rb->cache_stack_load_pointer    = 0;
            
        p_rb->state = resource_buffer_state_idle;
                
        // Don't need a sound region if no memory used.

        if ( sound_resource_buffer_size > 0 )
        {
            p_rb->p_IRadSoundHalMemoryRegion =
                ::radSoundHalSystemGet( )->GetRootMemoryRegion( )->CreateChildRegion(
                    sound_resource_buffer_size, number_of_sound_resources,
                        "radmusic resource_buffer" );
            p_rb->p_IRadSoundHalMemoryRegion->AddRef( );
        }
        else
        {
            p_rb->p_IRadSoundHalMemoryRegion = 0;
        }

        // Don't need a cache heap if no preloaded streams.

        if ( cache_resource_buffer_size > 0 )
        {
            p_rb->p_IRadSoundMemorySpaceObject_CacheHeap =
                ::radSoundMemorySpaceObjectCreate( ai_allocator );
                
            p_rb->p_IRadSoundMemorySpaceObject_CacheHeap->AddRef( );

            p_rb->p_IRadSoundMemorySpaceObject_CacheHeap->Initialize(
                cache_memory_space,                
                ::radMemorySpaceGetAllocator( cache_memory_space, cache_allocator ),
                cache_resource_buffer_size,
                radSoundHalDataSourceReadAlignmentGet( ),
                " PreLoaded ods::stream cache" );

            ::radMemoryExternalHeapCreate(
                & p_rb->p_IRadMemoryHeap_Cache,
                p_rb->p_IRadSoundMemorySpaceObject_CacheHeap->GetAddress( ),
                p_rb->p_IRadSoundMemorySpaceObject_CacheHeap->GetSizeInBytes( ),
                cache_memory_space,
                number_of_cache_resources,
                0, // Don't grow
                ai_allocator );
            
            radMemoryMonitorIdentifySection(
                p_rb->p_IRadSoundMemorySpaceObject_CacheHeap->GetAddress( ),
                " Pre-loaded ods::stream cache",
                cache_memory_space );
        }
        else
        {
            p_rb->p_IRadSoundMemorySpaceObject_CacheHeap = 0;
            p_rb->p_IRadMemoryHeap_Cache = 0;
        }
        
    }
    
    return p_rm;
}

void resource_manager_delete(
    resource_manager ** pp_rm )
{
    resource_manager * p_rm = *pp_rm;
    
    for( unsigned int b = 0; b < 2; b ++ )
    {
        resource_buffer * p_rb = & p_rm->resource_buffers[ b ];

        pointer_destroy( (pointer*) & p_rb->ptr_group_current );
        pointer_destroy( (pointer*) & p_rb->ptr_group_on_deck );  
           
        for( unsigned int c = 0; c < p_rm->number_of_sound_resources; c ++ )
        {
            if ( p_rb->p_clips[ c ].p_IRadSoundclip != 0 )
            {
                rAssert( p_rb->p_clips[ c ].p_IRadSoundclip->WillRelease( ) );
                p_rb->p_clips[ c ].p_IRadSoundclip->Release( );
                p_rb->p_clips[ c ].p_IRadSoundclip = 0;
            }
            
            pointer_destroy( (pointer*) & p_rb->p_clips[ c ].ptr_clip );
        }
        
        memory::free( p_rb->p_clips );
        
        for( unsigned int c = 0; c < p_rm->number_of_cache_resources; c ++ )
        {
            if ( p_rb->p_data_caches[ c ].p_IRadSoundDataCache != 0 )
            {
                rAssert( p_rb->p_data_caches[ c ].p_IRadSoundDataCache->WillRelease( ) );            
                p_rb->p_data_caches[ c ].p_IRadSoundDataCache->Release( );            
                p_rb->p_data_caches[ c ].p_IRadSoundDataCache = 0;
            }
            
            pointer_destroy( (pointer*) & p_rb->p_data_caches[ c ].ptr_stream );
        }
        
        memory::free( p_rb->p_data_caches );
                
        if ( p_rb->p_IRadSoundHalMemoryRegion != 0 )
        {
            p_rb->p_IRadSoundHalMemoryRegion->Release( );
        }
        
        if ( p_rb->p_IRadSoundMemorySpaceObject_CacheHeap != 0 )
        {
            p_rb->p_IRadSoundMemorySpaceObject_CacheHeap->Release( );
        }
        
        if ( p_rb->p_IRadMemoryHeap_Cache != 0 )
        {
            p_rb->p_IRadMemoryHeap_Cache->Release( );
        }
    }
    
    memory::free( p_rm );
    *pp_rm = 0;
}

radRef< IRadSoundHalDataSource >
    _construct_file_data_source(
        resource_manager * p_rm,
        const rsd_file & ptr_rsd_file,
        unsigned int start_frame )
{
    char file_name[ 256 ];
    
    rsd_file_file_name( ptr_rsd_file, file_name, 256 );
    
    char file_path[ 256 ];
    strncpy( file_path, p_rm->search_path, 256 );
    strncat( file_path, file_name, 256 );
    strncat( file_path, ".rsd", 256 );

    radRef< IRadSoundRsdFileDataSource > refIRadSoundRsdFileDataSource =
        radSoundRsdFileDataSourceCreate( p_rm->ai_allocator );
            
    // revisit reading header!
    


    #ifdef USE_ADPCM
    
        start_frame /= 32;
            
        refIRadSoundRsdFileDataSource->InitializeFromFileName(
            file_path,
            true,
            start_frame, IRadSoundHalAudioFormat::Frames,
            construct_audio_format( rsd_file_audio_format( ptr_rsd_file ), false ) );    
    
        radRef< IRadSoundAdpcmDecodeStream > refDs =
                radSoundAdpcmDecodeStreamCreate( RADMEMORY_ALLOC_DEFAULT );
                
        refDs->Initialize( refIRadSoundRsdFileDataSource );
        
        return refDs.m_pInterface;

    #else

    refIRadSoundRsdFileDataSource->InitializeFromFileName(
        file_path,
        true,
        start_frame, IRadSoundHalAudioFormat::Frames,
        construct_audio_format( rsd_file_audio_format( ptr_rsd_file ), false ) );  
                 
        return refIRadSoundRsdFileDataSource.m_pInterface;
        
    #endif
}

                        
void resource_manager_update( resource_manager * p_rm )
{
    for( unsigned int b = 0; b < 2; b ++ )
    {
        resource_buffer * p_rb = & p_rm->resource_buffers[ b ];
        
        if ( p_rb->state == resource_buffer_state_unloading )
        {
            while ( p_rb->clip_stack_current_pointer > p_rb->clip_stack_unload_pointer )
            {
                if ( p_rb->p_clips[ p_rb->clip_stack_current_pointer - 1].p_IRadSoundclip->WillRelease( ) )
                {
                    p_rb->p_clips[ p_rb->clip_stack_current_pointer - 1 ].p_IRadSoundclip->Release( );
                    p_rb->p_clips[ p_rb->clip_stack_current_pointer - 1 ].p_IRadSoundclip = 0;
                    rAssert( p_rb->clip_stack_current_pointer > 0 );                    
                    p_rb->clip_stack_current_pointer--;
                }
                else
                {
                    rAssert( false );
                }
            }
            
            while( p_rb->cache_stack_current_pointer > p_rb->cache_stack_unload_pointer )
            {
                if ( p_rb->p_data_caches[ p_rb->cache_stack_current_pointer - 1 ].p_IRadSoundDataCache->WillRelease( ) )
                {
                    p_rb->p_data_caches[ p_rb->cache_stack_current_pointer - 1 ].p_IRadSoundDataCache->Release( );
                    p_rb->p_data_caches[ p_rb->cache_stack_current_pointer - 1 ].p_IRadSoundDataCache = 0;
                    rAssert( p_rb->cache_stack_current_pointer > 0 );
                    p_rb->cache_stack_current_pointer--;
                }
                else
                {
                    rAssert( false );
                    return;
                }                    
            }
            
            p_rb->state = resource_buffer_state_loading;
        }
        
        if ( p_rb->state == resource_buffer_state_loading )
        {
            while( p_rb->clip_stack_current_pointer < p_rb->clip_stack_load_pointer )
            {
                if ( p_rb->p_clips[ p_rb->clip_stack_current_pointer ].p_IRadSoundclip == 0 )
                {
                    IRadSoundClip ** pp_IRadSoundclip =
                        & p_rb->p_clips[ p_rb->clip_stack_current_pointer ].p_IRadSoundclip;
                    
                    ods::clip ptr_clip = p_rb->p_clips[ p_rb->clip_stack_current_pointer ].ptr_clip;
                    
                    char clip_nom[ 256 ];
                    clip_name( ptr_clip, clip_nom, 256 );
                                      
                    *pp_IRadSoundclip = radSoundClipCreate( p_rm->ai_allocator );
                    (*pp_IRadSoundclip)->AddRef( );
                    
                    (*pp_IRadSoundclip)->Initialize(
                        _construct_file_data_source(
                            p_rm,
                            clip_rsd_file_ref( ptr_clip ), 0 ),
                        p_rb->p_IRadSoundHalMemoryRegion,
                        false,
                        clip_nom
                        );
                }
                
                if ( p_rb->p_clips[ p_rb->clip_stack_current_pointer ].p_IRadSoundclip->GetState( ) != IRadSoundClip::Initialized )
                {
                    return;
                }
                
                rAssert( p_rb->p_clips[ p_rb->clip_stack_current_pointer ].p_IRadSoundclip->GetState( ) == IRadSoundClip::Initialized);
                
                p_rb->clip_stack_current_pointer++;
            }
            
            while( p_rb->cache_stack_current_pointer < p_rb->cache_stack_load_pointer )
            {
                IRadSoundDataCache ** pp_IRadSoundDataCache =
                    & ( p_rb->p_data_caches[ p_rb->cache_stack_current_pointer ].p_IRadSoundDataCache );
                
                ods::stream ptr_stream = p_rb->p_data_caches[ p_rb->cache_stack_current_pointer ].ptr_stream;
                    
                if ( *pp_IRadSoundDataCache == 0 )
                {
                    char data_cache_nom[ 256 ];
                    stream_name( ptr_stream, data_cache_nom, 256 );
                    
                    *pp_IRadSoundDataCache = radSoundDataCacheCreate( p_rm->ai_allocator );
                    (*pp_IRadSoundDataCache)->AddRef( );
                    (*pp_IRadSoundDataCache)->InitializeAsync(
                        p_rb->p_IRadSoundMemorySpaceObject_CacheHeap->GetMemorySpace( ),
                        p_rb->p_IRadMemoryHeap_Cache,
                        _construct_file_data_source( p_rm, stream_rsd_file_ref( ptr_stream ), 0 ),
                        data_cache_nom );
                }
                
                if ( (*pp_IRadSoundDataCache)->GetState( ) != IRadSoundDataCache::Initialized ) 
                {
                    return;
                }
                
                rAssert( (*pp_IRadSoundDataCache)->GetState( ) == IRadSoundDataCache::Initialized );
                
                p_rb->cache_stack_current_pointer++;
            }
            
            p_rb->state = resource_buffer_state_idle;            
        }
        
        rAssert( p_rb->clip_stack_load_pointer == p_rb->clip_stack_current_pointer );
        rAssert( p_rb->cache_stack_load_pointer == p_rb->cache_stack_current_pointer );         
    }

   
}

unsigned int resource_manager_move_to_group(
    resource_manager * p_rm,
    const group & ptr_group_target,
    unsigned int playing_buffer )
{
    group ptr_group_current = p_rm->resource_buffers[ playing_buffer ].ptr_group_current;
    
    if ( not_null( ptr_group_current ) )
    {
        if ( group_is_child_of( ptr_group_target, ptr_group_current ) ||
             group_is_child_of( ptr_group_current, ptr_group_target ) ||
             ptr_group_current == ptr_group_target )
        {
            playing_buffer = playing_buffer;
        }
        else
        {
            playing_buffer = ( playing_buffer == 0 ? 1 : 0 );
        }
    }
    else
    {
        playing_buffer = playing_buffer;
    }
    
    _resource_buffer_move_to_group(
        p_rm,
        playing_buffer,
        ptr_group_target );
        
    resource_manager_update( p_rm );
    
    return playing_buffer;
}

// set load pointer to parent before calling this function

void _queue_items_to_load( resource_buffer * p_rb, const group & ptr_group_child, const group & ptr_group_parent )
{
    // calculate how many items to load, then work backwards, we must fill up
    // the stack in the correct order.
    
    unsigned int num_clips_to_load = 0;
    unsigned int num_data_caches_to_load = 0;

    group ptr_group_search = ptr_group_child;
    
    while( ptr_group_search != ptr_group_parent && ( not_null( ptr_group_search ) ) )
    {
        num_clips_to_load += clip_ref_array_num_items( group_clip_refs( ptr_group_search ) );
        
        stream_ref_array ptr_stream_ref_array = group_stream_refs( ptr_group_search );
        
        for( unsigned int s = 0; s < stream_ref_array_num_items( ptr_stream_ref_array ); s ++ )
        {
            ods::stream ptr_stream = stream_ref_array_item_at( ptr_stream_ref_array, s );
            
            if ( false == stream_streamed( ptr_stream ) )
            {
                num_data_caches_to_load++;
            }
        }
        
        ptr_group_search = group_parent_ref( ptr_group_search );
    }

    ptr_group_search = ptr_group_child;

    unsigned int clips_queued  = 0;    
    unsigned int caches_queued = 0;
    
    while( ptr_group_search != ptr_group_parent && ( not_null( ptr_group_search ) ) )
    {
        //
        // queue clips.
        //
        
        clip_ref_array ptr_clip_ref_array = group_clip_refs( ptr_group_search );
        
        for( unsigned int c = 0; c < clip_ref_array_num_items( ptr_clip_ref_array ); c++ )
        {
            ods::clip ptr_clip = clip_ref_array_item_at( ptr_clip_ref_array, c );                    
            
            p_rb->p_clips[ p_rb->clip_stack_load_pointer + ( num_clips_to_load - 1 - clips_queued ) ].ptr_clip = ptr_clip;
            clips_queued++;
        }
        
        // queue caches.
        
        stream_ref_array ptr_stream_ref_array = group_stream_refs( ptr_group_search );
        
        for( unsigned int s = 0; s < stream_ref_array_num_items( ptr_stream_ref_array ); s ++ )
        {
            ods::stream ptr_stream = stream_ref_array_item_at( ptr_stream_ref_array, s );
            
            if ( false == stream_streamed( ptr_stream ) )
            {
                p_rb->p_data_caches[ p_rb->cache_stack_load_pointer + ( num_data_caches_to_load - 1 - caches_queued ) ].ptr_stream = ptr_stream;
                caches_queued++;
            }
        }
        
        ptr_group_search = group_parent_ref( ptr_group_search );
    }
    
    rAssert( caches_queued == num_data_caches_to_load );
    rAssert( clips_queued  == num_clips_to_load );
    
    p_rb->clip_stack_load_pointer += num_clips_to_load;
    p_rb->cache_stack_load_pointer += num_data_caches_to_load;
}

void _resource_buffer_move_to_group( resource_manager * p_rm, unsigned int buffer_number, const group & ptr_group_target )
{
    resource_buffer * p_rb = p_rm->resource_buffers + buffer_number;
    
    char target_name[ 256 ];
    char source_name[ 256 ];
    
    group_name( ptr_group_target, target_name, 256 );
    
    if( not_null( p_rb->ptr_group_current ) )
    {
        group_name( p_rb->ptr_group_current, source_name, 256 ); 
    }
    else
    {
        strcpy( source_name, "nothing loaded" );
    }
    
    rDebugChannelPrintf( debug_channel,
        "radmusic: GROUP: Request To Move Buffer: [%d] To Group: [%s] from [%s]\n",
        buffer_number,
        target_name, source_name );

    if ( p_rb->state != resource_buffer_state_idle )
    {
        p_rb->ptr_group_on_deck = ptr_group_target;
        return;
    }
    
    rAssert( p_rb->clip_stack_load_pointer == p_rb->clip_stack_current_pointer );
    rAssert( p_rb->cache_stack_load_pointer == p_rb->cache_stack_current_pointer );
 
    if ( ptr_group_target == p_rb->ptr_group_current )
    {
        rDebugChannelPrintf( debug_channel,
            "radmusic: GROUP: Buffer: [%d] Already in group, nothing to do.\n", buffer_number );
            
        return;
    }
    if ( is_null( p_rb->ptr_group_current ) || group_is_child_of( ptr_group_target, p_rb->ptr_group_current ) )
    {
        rDebugChannelPrintf( debug_channel,
            "radmusic: GROUP: Buffer: [%d] No need to unload, "
            "moving towards leaves of tree, issuing load of more specific group\n",
            buffer_number );

        _queue_items_to_load( p_rb, ptr_group_target, p_rb->ptr_group_current );
        
        p_rb->state = resource_buffer_state_loading;                
        p_rb->ptr_group_current = ptr_group_target;            
    }
    else if ( group_is_child_of( p_rb->ptr_group_current, ptr_group_target ) )
    {
        rDebugChannelPrintf( debug_channel,
            "radmusic: GROUP: Buffer: [%d] No need to unload, moving towards root, "
            "keeping more specific group in memory\n", buffer_number );
    }
    else
    {
        rDebugChannelPrintf( debug_channel,
            "radmusic: GROUP: Buffer: [%d] Moving to new branch ...\n", buffer_number );

        group ptr_group_common_parent;
        group ptr_group_to_move_up    = ptr_group_target;
        group ptr_group_to_move_up_to = p_rb->ptr_group_current;
        
        if ( group_tree_depth( ptr_group_to_move_up ) != group_tree_depth( ptr_group_to_move_up_to ) )
        {
            // Get to common depth

            if ( group_tree_depth( ptr_group_target ) > group_tree_depth( p_rb->ptr_group_current ) )
            {
                ptr_group_to_move_up     = ptr_group_target;
                ptr_group_to_move_up_to  = p_rb->ptr_group_current;
            }
            else
            {
                ptr_group_to_move_up_to = ptr_group_target;
                ptr_group_to_move_up    = p_rb->ptr_group_current;
            }

            unsigned int target_tree_depth =
                group_tree_depth( ptr_group_to_move_up ) -
                    group_tree_depth( ptr_group_to_move_up_to );

            for( unsigned int m = 0; m < target_tree_depth; m ++ )
            {
                ptr_group_to_move_up = group_parent_ref( ptr_group_to_move_up );
            }
        }

        // Find common parent

        while( ptr_group_to_move_up_to != ptr_group_to_move_up )
        {
            ptr_group_to_move_up_to = group_parent_ref( ptr_group_to_move_up_to );
            ptr_group_to_move_up    = group_parent_ref( ptr_group_to_move_up );
        }
        
        p_rb->cache_stack_unload_pointer = p_rb->cache_stack_current_pointer;
        p_rb->clip_stack_unload_pointer  = p_rb->clip_stack_current_pointer;

        while( p_rb->ptr_group_current != ptr_group_to_move_up_to )
        {
            char group_nom[ 256 ];
            group_name( p_rb->ptr_group_current, group_nom, 256 );
            
            rDebugChannelPrintf( debug_channel,
                "radmusic: GROUP: Buffer: [%d] Unloading group: [%s]\n",
                    buffer_number,
                    group_nom );

            clip_ref_array ptr_clip_ref_array = group_clip_refs( p_rb->ptr_group_current );
            unsigned int num_clips    = clip_ref_array_num_items( ptr_clip_ref_array );

            for( unsigned int c = 0; c < num_clips; c ++ )
            {
                rAssert( p_rb->clip_stack_unload_pointer > 0 );
                p_rb->clip_stack_unload_pointer--;                
                p_rb->p_clips[ p_rb->clip_stack_unload_pointer ].ptr_clip = (ods::clip) pointer_null;
            }
            
            stream_ref_array ptr_stream_ref_array = group_stream_refs( p_rb->ptr_group_current );
            unsigned int num_streams      = stream_ref_array_num_items( ptr_stream_ref_array );
            unsigned int num_preloaded_streams = 0;
            
            for( unsigned int i = 0; i < num_streams; i ++ )
            {
                ods::stream ptr_stream = stream_ref_array_item_at( ptr_stream_ref_array, i );
                
                if ( stream_streamed( ptr_stream ) == false )
                {
                    num_preloaded_streams++;
                }
            }
            
            for( unsigned int s = 0; s < num_preloaded_streams; s ++ )
            {
                rAssert( p_rb->cache_stack_unload_pointer > 0 );
                p_rb->cache_stack_unload_pointer--;                 
                p_rb->p_data_caches[ p_rb->cache_stack_unload_pointer ].ptr_stream = (ods::stream) pointer_null;
            }

            p_rb->ptr_group_current = group_parent_ref( p_rb->ptr_group_current );
        }
        
        p_rb->cache_stack_load_pointer = p_rb->cache_stack_unload_pointer;
        p_rb->clip_stack_load_pointer  = p_rb->clip_stack_unload_pointer;
        
        _queue_items_to_load( p_rb, ptr_group_target, ptr_group_to_move_up_to );

        p_rb->ptr_group_current = ptr_group_target;
        p_rb->state = resource_buffer_state_unloading;
    }
    
    p_rb->ptr_group_current = ptr_group_target;
    
    resource_manager_update( p_rm );    
}

    
bool resource_manager_group_loaded( resource_manager * p_rm, unsigned int buffer, const group & ptr_group )
{
    rAssert( ( p_rm->resource_buffers[ buffer ].ptr_group_current == ptr_group ) ||
               p_rm->resource_buffers[ buffer ].ptr_group_on_deck == ptr_group );
                   
    if ( not_null( p_rm->resource_buffers[ buffer ].ptr_group_on_deck ) )
    {
        rAssert( p_rm->resource_buffers[ buffer ].ptr_group_on_deck == ptr_group );
        return false;
    }

    rAssert( p_rm->resource_buffers[ buffer ].ptr_group_current == ptr_group );

    resource_manager_update( p_rm ); 
        
    if ( p_rm->resource_buffers[ buffer ].state == resource_buffer_state_idle )
    {
        return true;
    }
    
    return false;
}

radRef< IRadSoundClip > resource_manager_get_clip(
    resource_manager * p_rm,
    const ods::clip & ptr_clip,
    unsigned int buffer )
{
    resource_buffer * p_rb = & p_rm->resource_buffers[ buffer ];
    
    rAssert( p_rb->state == resource_buffer_state_idle );
    
    for( unsigned int c = 0; c < p_rb->clip_stack_current_pointer; c ++ )
    {
        if ( p_rb->p_clips[ c ].ptr_clip == ptr_clip )
        {
            return p_rb->p_clips[ c ].p_IRadSoundclip;
        }
    }
    
    rAssert( false );
    
    return 0;
}

radRef< IRadSoundHalDataSource > resource_manager_get_stream(
    resource_manager * p_rm,
    const ods::stream & ptr_stream,
    unsigned int buffer,
    unsigned int start_frame )
{
    resource_buffer * p_rb = & p_rm->resource_buffers[ buffer ];
    
    rAssert( p_rb->state == resource_buffer_state_idle );
    
    radRef< IRadSoundHalDataSource > ref_IRadSoundHalDataSource;
    
    if ( stream_streamed( ptr_stream ) == true )
    {
        ref_IRadSoundHalDataSource = _construct_file_data_source(
            p_rm,
            stream_rsd_file_ref( ptr_stream ),
            start_frame );
    }
    else
    {   
        unsigned int c;
         
        for( c = 0; c < p_rb->cache_stack_current_pointer; c ++ )
        {
            if ( p_rb->p_data_caches[ c ].ptr_stream == ptr_stream )
            {
                ref_IRadSoundHalDataSource =
                    p_rb->p_data_caches[ c ].p_IRadSoundDataCache->CreateDataSource(
                        start_frame, p_rm->ai_allocator );
                        
                break;
            }
        }
        
        if ( c >= p_rb->cache_stack_current_pointer )
        {    
            rAssert( false );
        }
    }
    
    return ref_IRadSoundHalDataSource;
}

}