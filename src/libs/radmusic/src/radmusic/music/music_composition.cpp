//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
/// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "pch.hpp"
#include <radmusic/music/music_composition.hpp>
#include <radmusic/radmusic.hpp>
#include <memory/memory.hpp>
#include <raddebug.hpp>
#include <radload/loader.hpp>

//-----------------------------------------------------------------------------
// Local Constants, Typedefs, and Macros
//-----------------------------------------------------------------------------

using namespace ods;

//-----------------------------------------------------------------------------
// Global Data, Local Data, Local Classes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Member Functions
//-----------------------------------------------------------------------------

namespace radmusic {

composition * composition_new( void)
{
    composition * p_c =
        (composition*) memory::malloc(
            sizeof( composition ), "composition" );
        
    p_c->ref_count = 0;
      
    pointer_construct( (pointer*) & p_c->ptr_project );
    pointer_construct( (pointer*) & p_c->ptr_composition ); 
    
    return p_c;       
}

void composition_delete( composition ** p_c )
{
    rAssert( (*p_c)->ref_count == 0 );
    
    pointer_destroy( (pointer*) & (*p_c)->ptr_composition );
    
#ifdef CLEAN_UP_PROJECT // ok to just dump the memory...
    
    if ( not_null( (*p_c)->ptr_project ) )
    {
        project_destroy( (*p_c)->ptr_project );
    }
    
#endif
    
    pointer_destroy( (pointer*) & (*p_c)->ptr_project );
    block_destroy( & (*p_c)->blk );
    
    memory::free( *(p_c) );    
    
    *p_c = 0;
}

const unsigned int composition_chunk_id = 0x43634211;

struct radmusic_file_loader : public radLoadFileLoader
{
    virtual void LoadFile(
        radLoadOptions* options,
        radLoadUpdatableRequest* request );
};

struct composition_data_loader : public radLoadDataLoader
{
    virtual IRefCount * LoadData(
        radLoadStream * p_stream, 
        radMemoryAllocator alloc, 
        radLoadInventory* resolver = NULL, 
        IRefCount* originalObject = NULL );
};

struct radload_radmusic_inventory : public radLoadInventory
{
    IRefCount * Find( radLoadInventory::SafeCastBase & sc, const radKey id )
    {
        rAssert( sc.safe_cast( m_ref_adapter ) );
        
        return m_ref_adapter;
    }  
        
    radRef< radload_composition_adapter > m_ref_adapter;
};

IRefCount * composition_data_loader::LoadData(
        radLoadStream * p_stream, 
        radMemoryAllocator alloc, 
        radLoadInventory* resolver, 
        IRefCount* originalObject )
{
    radload_composition_adapter * p_rca = new ( alloc )
        radload_composition_adapter( );
        
    p_rca->p_composition = composition_new(  );

    memory_hooks h;
    h.p_realloc = memory::realloc;
    h.p_free    = memory::free;
    
    stream_info si;
    si.p_stream = (void*) p_stream;
    si.p_sf     = & radload_stream_functions;
    
    if ( block_construct_from_stream_synch(
        & p_rca->p_composition->blk,
        & si,
        & p_rca->p_composition->ptr_project,
        & h,
        false ) )
    {
        
        if( not_null( p_rca->p_composition->ptr_project ) )
        {            
            p_rca->p_composition->ptr_composition = (comp) project_find_instance(
                p_rca->p_composition->ptr_project, "my_comp", 0 );
                
            if ( not_null( p_rca->p_composition->ptr_composition ) )
            {
                return p_rca;
            }
        }
    }    

    p_rca->Release( );
    p_rca = 0;
        
    return p_rca;
}

radload_composition_adapter::~radload_composition_adapter( void )
{
    if ( p_composition )
    {
        composition_delete( & p_composition );
    }
}

void radmusic_file_loader::LoadFile(
        radLoadOptions* options,
        radLoadUpdatableRequest* request )
{
    radLoadStream * p_radload_stream;
    
    rTunePrintf( "AUDIO: Performace Load: [%s]\n", options->filename );
    
    ::radLoadCreateFileStream(
        & p_radload_stream,
        options->filename );
        
            
    radLoadDataLoader * p_loader =
        radLoadInstance( )->GetDataLoader( composition_chunk_id );

    radload_radmusic_inventory * p_rmi = new ( options->allocator )
        radload_radmusic_inventory( ); 
  
    p_rmi->m_ref_adapter  = (radload_composition_adapter*) p_loader->LoadData(
        p_radload_stream,
        options->allocator,
        options->resolverInventory,
        NULL );
        
    request->SetInventory( p_rmi );

    p_radload_stream->Release( );
}

void register_radload_loaders( void )
{
    radLoadInstance( )->AddFileLoader( new radmusic_file_loader, "rms" );
    radLoadInstance( )->AddDataLoader( new composition_data_loader, composition_chunk_id );
}

composition * composition_new_from_file(
    const char * p_file_name )
{
    composition * p_c = composition_new( );

    memory_hooks h;
    h.p_realloc = memory::realloc;
    h.p_free    = memory::free;
    
    stream_info si;
    si.p_sf = & radfile_stream_functions;
    si.p_stream = radfile_stream_open( p_file_name, false );
    
    if ( si.p_stream == 0 )
    {
        return 0;
    }   
            
    if ( block_construct_from_stream_synch(
        & p_c->blk,
        & si,
        & p_c->ptr_project,
        & h,
        false ) )
    {
        if( not_null( p_c->ptr_project ) )
        {
            p_c->ptr_composition = (comp) project_find_instance(
                p_c->ptr_project, "my_comp", 0 );
                    
            if ( not_null( p_c->ptr_composition ) )
            {
                radfile_stream_close( si.p_stream );                          
                return p_c;
            }
        }
    }
    
    radfile_stream_close( si.p_stream );    

    composition_delete( & p_c );
    
    return p_c;
}

}