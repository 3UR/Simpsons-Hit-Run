//----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "pch.hpp"

#include <ods/ods.hpp>
#include <radload/loader.hpp>
#include <radfile.hpp>
#include <raddebug.hpp>

#include <string.h>

//-----------------------------------------------------------------------------
// Local Constants, Typedefs, and Macros
//-----------------------------------------------------------------------------

namespace ods
{
    const char g_str_block_reference_table[] = "block_reference_table";
    const char g_str_block_data_blob[]       = "block_data_blob";
    
    const null_ptr_type pointer_null = { 4 };
    
    #ifdef TAG_ALLOCS
        const unsigned int block_alloc_head_pad_bytes = 64;
        const unsigned int block_alloc_head_pad_bits  = block_alloc_head_pad_bytes * 8; 
    #else
        const unsigned int block_alloc_head_pad_bytes = 0;
        const unsigned int block_alloc_head_pad_bits  = block_alloc_head_pad_bytes * 8;
    #endif 
    
}

//-----------------------------------------------------------------------------
// Global Data, Local Data, Local Classes [Optional commment can be deleted]
//-----------------------------------------------------------------------------

namespace ods
{
    offset_t _get_reference_value( block * p_m, unsigned int index );
    void     _set_reference_value( block * p_m, unsigned int index, offset_t );    
}

//-----------------------------------------------------------------------------
// Member Functions
//-----------------------------------------------------------------------------

namespace ods
{

void block_construct( block * p_m, memory_hooks * p_mem_hooks, bool reverse_endian )
{
    memset( p_m, 0, sizeof( *p_m ) );
    memcpy( & (p_m->mem_hooks), p_mem_hooks, sizeof( *p_mem_hooks ) );
        
    p_m->reverse_endian = reverse_endian;
}

void block_destroy( block * p_m )
{
    // ok to do a bulk dump here
    
    //rAssert( p_m->size == 0 );    
    //rAssert( p_m->num_references == 0 );
    
    rAssert( p_m->p_pointer_head == 0 );
   
    p_m->mem_hooks.p_free( p_m->p_mem );
    p_m->mem_hooks.p_free( p_m->p_references );
}


bool block_reverse_endian( block * p_m )
{
    return p_m->reverse_endian;
}

bool block_save_to_stream_synch( const block * p_m, stream_info * p_si )
{

    unsigned int num_references = reverse_endian( p_m->reverse_endian, p_m->num_references );
    unsigned int size           = reverse_endian( p_m->reverse_endian, p_m->size );
    
    bool ok =
        p_si->p_sf->p_write(
            p_si->p_stream, & num_references, sizeof( num_references ) );
            
    if( false == ok  ) { return false; }
    
    for( unsigned int i = 0; i < p_m->num_references; i ++ )
    {
        offset_t ref_offset = reverse_endian( p_m->reverse_endian, p_m->p_references[ i ] );
        
        ok = p_si->p_sf->p_write( p_si->p_stream, & ref_offset, sizeof( ref_offset ) );
        
        if( false == ok  ) { return false; }
    }
    
    ok = p_si->p_sf->p_write( p_si->p_stream, & size, sizeof( size ));
    
    if( false == ok ) { return false; }
    
    ok = p_si->p_sf->p_write( p_si->p_stream, p_m->p_mem, p_m->size / 8 );
    
    if( false == ok  ) { return false; }
    
    return true; 
}

offset_t block_size( block * p_m )
{
    return p_m->reverse_endian, p_m->size / 8;
}

bool block_construct_from_stream_synch(
    block * p_m,
    stream_info * p_si,
    pointer * p_pointer,
    memory_hooks * p_mem_hooks,
    bool reverse_endian )
{
    block_construct( p_m, p_mem_hooks, reverse_endian );
             
    if ( true != p_si->p_sf->p_read( p_si->p_stream, & p_m->num_references, sizeof( p_m->num_references ) ) ) { return false; }

    p_m->num_references = ods::reverse_endian( p_m->reverse_endian, p_m->num_references );
    
    p_m->p_references = (offset_t*)
        p_m->mem_hooks.p_realloc(
            0, 0, p_m->num_references * sizeof( offset_t ), g_str_block_reference_table );
    
    if ( true != p_si->p_sf->p_read( p_si->p_stream, p_m->p_references, sizeof( offset_t ) * p_m->num_references ) )
    {
        return false;
    }
    
    for( unsigned int r = 0; r < p_m->num_references; r ++ )
    {
        p_m->p_references[ r ] = ods::reverse_endian( p_m->reverse_endian, p_m->p_references[ r ] );
    }
    
    if ( true != p_si->p_sf->p_read( p_si->p_stream, & p_m->size, sizeof( p_m->size ) ) ) { return false; }
    
    p_m->size = ods::reverse_endian( p_m->reverse_endian, p_m->size );
    
    p_m->p_mem = (char*) p_m->mem_hooks.p_realloc( 0, 0, p_m->size / 8, g_str_block_data_blob );
    
    if ( true != p_si->p_sf->p_read( p_si->p_stream, p_m->p_mem, p_m->size / 8 ) ) { return false; }
    
    p_m->p_pointer_head = 0;
    
    *p_pointer = pointer( p_m, block_alloc_head_pad_bits );

    return true;
}

pointer block_alloc( block * p_m, unsigned int bits, const char * p_id )
{   
    rAssert( bits % 8 == 0 );
    
    p_m->p_mem = (char*) p_m->mem_hooks.p_realloc(
        p_m->p_mem,
        p_m->size / 8 + block_alloc_head_pad_bytes,
        (( p_m->size + bits ) / 8) + block_alloc_head_pad_bytes,
        g_str_block_data_blob );    
    
    ::memset( p_m->p_mem + ( p_m->size / 8 ), '-', block_alloc_head_pad_bytes );
    ::memset( p_m->p_mem + ( p_m->size / 8 ) + block_alloc_head_pad_bytes, '?', bits / 8 );
    
    strncpy( p_m->p_mem + ( p_m->size / 8 ), p_id, block_alloc_head_pad_bytes );
    
    pointer p( p_m, p_m->size + block_alloc_head_pad_bits );    
   
    p_m->size += bits + block_alloc_head_pad_bits;
            
    return p;    
}

void block_write( bool check_endian, const pointer & p, const void * p_data, offset_t bits )
{
    block_write( check_endian, p, 0, p_data, bits );
}

void block_write(
    bool check_endian,
    pointer p,
    offset_t bit_offset,
    const void * p_bits,
    offset_t num_bits )
{
    rAssert( num_bits % 8 == 0 );
    rAssert( bit_offset % 8 == 0 );
    
    unsigned int bytes_to_write       = num_bits / 8;
    unsigned int total_byte_offset = ( bit_offset + p.offset( ) ) / 8;
    
    if ( check_endian & p.memory()->reverse_endian )
    {
        const char * p_src = ((const char*)p_bits) + bytes_to_write - 1;
        char       * p_dst = p.memory( )->p_mem + total_byte_offset;
        
        while( p_src >= p_bits )
        {
            *p_dst = *p_src;

            p_src--;
            p_dst++;
        }
        
    }
    else
    {
        ::memcpy( p.memory( )->p_mem + total_byte_offset, p_bits, bytes_to_write );
    }
  
}

void memoryfill( const pointer & p, unsigned char fill, unsigned int bits )
{
    rAssert( bits % 8  == 0 );
    ::memset( p.memory( )->p_mem + ( p.offset( ) / 8 ), fill, bits / 8 );
}

offset_t _get_reference_value( block * p_m, unsigned int index )
{
    unsigned int bit_offset  = p_m->p_references[ index ];
    rAssert( (bit_offset % 8) == 0 );    
    unsigned int byte_offset = bit_offset / 8;
    char *       p_addr     = p_m->p_mem + byte_offset;
    
    offset_t value;
    ::memcpy( & value, p_addr, sizeof( value ) );
    
    value = reverse_endian( p_m->reverse_endian, value );
    
    return value;
}

void _set_reference_value( block * p_m, unsigned int index, offset_t value)
{
    unsigned int bit_offset  = p_m->p_references[ index ];
    rAssert( (bit_offset % 8) == 0 );
    unsigned int byte_offset = bit_offset / 8;
    char *       p_addr     = p_m->p_mem + byte_offset;
    
    value = reverse_endian( p_m->reverse_endian, value );
    
    ::memcpy( p_addr, & value, sizeof( value ) );
}

void block_add_reference( block * p_m, offset_t bit_offset )
{
    rAssert ( bit_offset < p_m->size );
    
    for( unsigned int i = 0; i < p_m->num_references; i ++ )
    {
        if ( p_m->p_references[ i ] == bit_offset )
        {
            rAssert( false );
        } 
    }
    
    p_m->p_references = (offset_t*) p_m->mem_hooks.p_realloc(
        p_m->p_references,
        ( p_m->num_references ) * sizeof( offset_t ),        
        ( p_m->num_references + 1 ) * sizeof( offset_t ),
        g_str_block_reference_table );
    
    p_m->p_references[ p_m->num_references ] = bit_offset;
    
    p_m->num_references++; 
}


void block_revoke_reference(block * p_m, offset_t bit_offset )
{    
    for( unsigned int i = 0; i < p_m->num_references; i ++ )
    {
        if ( p_m->p_references[ i ] == bit_offset )
        {
            ::memmove( p_m->p_references + i,
                p_m->p_references + i + 1,
                (p_m->num_references - i - 1) * sizeof( offset_t ) );
                
            p_m->p_references = (offset_t*) p_m->mem_hooks.p_realloc(
                p_m->p_references,
                p_m->num_references * sizeof( offset_t ),                
                (p_m->num_references - 1) * sizeof( offset_t ),
                g_str_block_reference_table );

            p_m->num_references--;
            
            return;
        } 
    }
    
    rAssert( false );
}
    
void block_write_reference( const pointer & p, offset_t bit_offset, offset_t pointer_bit_offset )
{
    rAssert( bit_offset % 8 == 0 );
    
    unsigned int total_byte_offset = ( bit_offset + p.offset( ) ) / 8;
    
    pointer_bit_offset = reverse_endian( p.memory( )->reverse_endian, pointer_bit_offset );
        
    ::memcpy( p.memory( )->p_mem + total_byte_offset, & pointer_bit_offset, sizeof( offset_t ) );
        
    block_add_reference( p.memory( ), p.offset( ) + bit_offset  );
}

void block_write_reference( const pointer & ptr, const pointer & ptr_ref )
{
    block_write_reference( ptr, 0, ptr_ref.offset( ) );
}

pointer block_read_reference( const pointer & p, offset_t bit_offset )
{
    offset_t offset;
    
    block_read( true, p, bit_offset, & offset, sizeof( offset ) * 8 );
    
    return pointer( p.memory(), offset );
}

pointer block_read_reference( const pointer & p )
{
    return block_read_reference( p, 0 );
}

void block_read( bool check_endian, const pointer & p, void * p_bits, offset_t num_bits )
{
    block_read( check_endian, p, 0, p_bits, num_bits );
}

void block_read( bool check_endian, const pointer & p, offset_t bit_offset, void * p_bits, offset_t bits )
{
    rAssert( bit_offset % 8 == 0 );
    rAssert( bits       % 8 == 0 );
    
    unsigned int total_byte_offset = ( p.offset( ) + bit_offset ) / 8;
    unsigned int bytes_to_read       = bits / 8;
    
    if ( ( check_endian == true ) && ( p.memory( )->reverse_endian == true ) )
    {
        char * p_dst = (char*) p_bits + bytes_to_read - 1;
        char * p_src = p.memory( )->p_mem + total_byte_offset;
        
        while ( p_dst >= p_bits )
        {
            *p_dst = *p_src;
            
            p_src++;
            p_dst--;
        }
    }
    else
    {
        ::memcpy( p_bits, p.memory( )->p_mem + total_byte_offset, bytes_to_read );
    }
}

void block_insert( bool check_endian, const pointer & p, const void * p_memory, offset_t bits)
{
    block_insert( check_endian, p, 0, p_memory, bits );
}

void block_insert( const pointer & p, offset_t bit_offset, offset_t bits )
{
    rAssert( bit_offset % 8 == 0 );
    rAssert( bits % 8 == 0 );
    
    unsigned int bytes_to_insert = bits / 8;
    
    unsigned int total_bit_offset = ( p.offset( ) + bit_offset );
    unsigned int total_byte_offset = ( total_bit_offset ) / 8;
        
    for( unsigned int i = 0; i < p.memory( )->num_references; i ++ )
    {
        offset_t offset = _get_reference_value( p.memory( ), i );
        
        rAssert( offset < p.memory( )->size );
        
        if ( offset >= total_bit_offset )
        {
            _set_reference_value( p.memory( ), i, offset + bits );
        }
        
        if ( p.memory( )->p_references[ i ] >= total_bit_offset )
        {
            p.memory( )->p_references[ i ] += bits;
        }
    }
    
    const pointer * p_ptr = p.memory( )->p_pointer_head;
    
    while( p_ptr != 0 )
    {
        if ( not_null( *p_ptr ) )
        {
            if ( p_ptr->offset( ) >= total_bit_offset )
            {
                p_ptr->offset( p_ptr->offset( ) + bits );
            }
        }
        
        p_ptr = p_ptr->next;
    }
    
    p.memory( )->p_mem = (char*) p.memory( )->mem_hooks.p_realloc(
        p.memory( )->p_mem,
        (p.memory( )->size / 8),
        (p.memory( )->size / 8) + bytes_to_insert,
        g_str_block_data_blob );

    ::memmove( p.memory( )->p_mem + total_byte_offset + bytes_to_insert,
               p.memory( )->p_mem + total_byte_offset, (p.memory( )->size / 8) - total_byte_offset );

    p.memory( )->size += bits;
    

}

void block_insert( bool check_endian, const pointer & p, offset_t bit_offset, const void * p_bits, offset_t bits )
{
    block_insert( p, bit_offset, bits );
    block_write( check_endian, p, bit_offset, p_bits, bits );
}

void block_delete( pointer p, unsigned int size_in_bits )
{
    rAssert( size_in_bits % 8 == 0 );
    
    for( unsigned int i = 0; i < p.memory( )->num_references; i ++ )
    {
        offset_t offset = _get_reference_value( p.memory( ), i );
        
        if ( offset >= p.offset( ) + size_in_bits )
        {
            _set_reference_value( p.memory( ), i, offset - size_in_bits );
        }
                
        if ( p.memory( )->p_references[ i ] >= p.offset( ) &&
             p.memory( )->p_references[ i ] < ( p.offset( ) + size_in_bits ) )
        {                
             rAssert( false ); // revisit
        }
        else if ( p.memory( )->p_references[ i ] >= ( p.offset( ) + size_in_bits ) )
        {
            p.memory( )->p_references[ i ] -= size_in_bits;
        }
    }

    unsigned int start_offset_in_bytes = p.offset( ) / 8;
    unsigned int number_of_bytes_to_free = size_in_bits / 8;
    
    ::memmove( p.memory( )->p_mem + start_offset_in_bytes,
               p.memory( )->p_mem + start_offset_in_bytes + number_of_bytes_to_free,
               ( p.memory( )->size / 8 ) - ( start_offset_in_bytes + number_of_bytes_to_free ) );

    p.memory( )->p_mem = (char*) p.memory( )->mem_hooks.p_realloc(
        p.memory( )->p_mem,
        ( p.memory( )->size / 8 ),        
        ( p.memory( )->size / 8 ) - number_of_bytes_to_free,
        g_str_block_data_blob );

    const pointer * p_pointer = p.memory( )->p_pointer_head;
    
    unsigned int start_of_free_in_bits = p.offset( );
    unsigned int end_of_free_in_bits = start_of_free_in_bits + size_in_bits;
    
    while( p_pointer )
    {
        if ( not_null( *p_pointer ) )
        {            
            if ( p_pointer->offset( ) >= start_of_free_in_bits &&
                p_pointer->offset( ) <  end_of_free_in_bits )
            {
                p_pointer->offset( -1 );
            }
            else if ( p_pointer->offset( ) >= end_of_free_in_bits )
            {
                p_pointer->offset( p_pointer->offset( ) - size_in_bits );
            }
        }
            
        p_pointer = p_pointer->next;
    }
        
    p.memory( )->size -= size_in_bits;
}

void block_free( const pointer & p, offset_t size_in_bits )
{
    offset_t o = p.offset( );
    block * p_mem  = p.memory( );
    
    block_delete( p, size_in_bits );
    block_delete( pointer( p_mem, o - block_alloc_head_pad_bits ), block_alloc_head_pad_bits );
}

// pointer

void pointer::_add_to_list( const pointer * p )
{
    if ( p->p_mem != 0 )
    {
        if ( p->p_mem->p_pointer_head  != 0 )
        {
            p->p_mem->p_pointer_head->prev = p;
            p->next = p->p_mem->p_pointer_head;
        }
        else
        {
            p->next = 0;
        }

        p->p_mem->p_pointer_head = p;
        p->prev = 0;
    }
    else
    {
        p->prev = 0;
        p->next = 0;
    }
}

void pointer::_remove_from_list( const pointer * p )
{
    if ( p->p_mem != 0 )
    {
        if ( p->prev != 0 )
        {
            p->prev->next = p->next;
        }
        else
        {
            p->p_mem->p_pointer_head = p->next;
        }

        if ( p->next != 0 )
        {
            p->next->prev = p->prev;
        }
        
        p->next = 0;
        p->prev = 0;
    }
    else
    {
        rAssert( p->prev == 0 );
        rAssert( p->next == 0 );
        rAssert( p->o == 0xFFFFFFFF );        
    }        
}

void pointer_construct( pointer * p_pointer )
{
    pointer::construct( p_pointer );
}

void pointer_destroy( pointer * p_pointer )
{
    pointer::destroy( p_pointer );
}

void pointer::construct( pointer * p_pointer )
{
    p_pointer->p_mem = 0;
    p_pointer->o = 0xFFFFFFFF;
    
    pointer::_add_to_list( p_pointer );
}

void pointer::destroy( pointer * p_pointer )
{
    pointer::_remove_from_list( p_pointer );
}
                
pointer::pointer( )
    :
    p_mem( 0 ),    
    o( 0xFFFFFFFF )

{
    _add_to_list( this );
}

pointer::pointer( const block * p_m, unsigned int o )
    :
    p_mem( (block*) p_m ),
    o( o )
{
    _add_to_list( this );
}

pointer::pointer( const pointer & o )
    :
    p_mem( o.p_mem ),
    o( o.o )
{
    _add_to_list( this );
}

pointer::pointer( const null_ptr_type & n )
    :
    prev( 0 ),
    next( 0 )
{
    o = 0xFFFFFFFF;
    p_mem = 0;
}    
                
/* const pointer & pointer::operator=( const null_ptr_type & ) const
{
    _remove_from_list( this );
    
    o = 0xFFFFFFFF;
    p_mem = 0;
    
    return *this;
} */

/* const pointer & pointer::operator=( const pointer & other ) const
{
    _remove_from_list( this );
    
    o = other.o;
    p_mem = other.p_mem;
    
    _add_to_list( this );
    
    return *this;
} */

pointer & pointer::operator=( const null_ptr_type & )
{
    _remove_from_list( this );
    
    o = 0xFFFFFFFF;
    p_mem = 0;
    
    return *this;
}

pointer & pointer::operator=( const pointer & other )
{
    _remove_from_list( this );
    
    o = other.o;
    p_mem = other.p_mem;
    
    _add_to_list( this );
    
    return *this;
}

bool pointer::operator==( const pointer & other ) const
{
    return o == other.o && p_mem == other.p_mem;
}

bool pointer::operator!=( const pointer & other ) const
{
    return o != other.o || p_mem != other.p_mem;
}

const offset_t * pointer::p_offset( ) const
{
    return & o;
}

offset_t pointer::offset( ) const
{
    return o;
}

void pointer::offset( offset_t m ) const
{
    rAssert( m % 8 == 0 || m == 0xFFFFFFFF);
    o = m;
}

block * pointer::memory( ) const
{ 
    return p_mem;
}
    
pointer::~pointer( void )
{
    _remove_from_list( this );
    
}


// array

const offset_t _array_num_items_offset = 0;
const offset_t _array_item_size_offset = _array_num_items_offset + (sizeof(unsigned int)*8);
const offset_t _array_start_offset     = _array_item_size_offset + (sizeof(unsigned int)*8);

pointer array_construct( block * p_m, unsigned int item_size )
{
    pointer p = block_alloc( p_m,_array_start_offset, "array" );
    
    unsigned int zero = 0;
        
    block_write( true, p, 0, & zero, sizeof( zero ) * 8 );
    block_write( true, p, _array_item_size_offset, & item_size, sizeof( item_size ) * 8 );
    
    return p;
}

void array_destroy( pointer p )
{
    unsigned int num_items;
    unsigned int list_item_size_in_bits;
    
    block_read( true, p, _array_num_items_offset, & num_items, (sizeof(num_items)*8) );
    block_read( true, p, _array_item_size_offset, & list_item_size_in_bits, (sizeof(list_item_size_in_bits)*8) );
    
    block_free( p, _array_start_offset + ( list_item_size_in_bits * num_items )  );
    p = pointer_null;
}

unsigned int array_size( pointer p )
{
    unsigned int num_items;
    block_read( true, p, _array_num_items_offset, & num_items, (sizeof(num_items)*8) );
    
    return num_items;
}

pointer array_get_pointer_at( pointer ptr_a, unsigned int index )
{
    unsigned int list_item_size_in_bits;
    
    block_read( true, ptr_a, _array_item_size_offset, & list_item_size_in_bits, (sizeof(list_item_size_in_bits)*8) );

    return pointer( ptr_a.memory( ), ptr_a.offset( ) + _array_start_offset + (list_item_size_in_bits * index) );
}

pointer array_add_tail( const pointer & ptr_array )
{
    unsigned int num_items;
    unsigned int list_item_size_in_bits;
    
    block_read( true, ptr_array, _array_num_items_offset, & num_items, (sizeof(num_items)*8) );
    block_read( true, ptr_array, _array_item_size_offset, & list_item_size_in_bits, (sizeof(num_items)*8) );
    
    block_insert(
        ptr_array, _array_start_offset + ( list_item_size_in_bits * num_items ),
        list_item_size_in_bits );
        
    pointer ptr_new_item( ptr_array.memory( ), ptr_array.offset( ) + _array_start_offset + ( list_item_size_in_bits * num_items ) );
    
    num_items++;
    
    block_write( true, ptr_array, _array_num_items_offset, & num_items, (sizeof(num_items)*8) );
    
    return ptr_new_item;     
}

void array_remove_at( const pointer & ptr_array, unsigned int index )
{
    unsigned int num_items;
    unsigned int list_item_size_in_bits;
    
    block_read( true, ptr_array, _array_num_items_offset, & num_items, (sizeof(num_items)*8) );
    block_read( true, ptr_array, _array_item_size_offset, & list_item_size_in_bits, (sizeof(num_items)*8) );
    
    block_delete(
        pointer(
            ptr_array.memory( ),
            ptr_array.offset( ) + _array_start_offset + ( index * list_item_size_in_bits ) ),
        list_item_size_in_bits );
        
    num_items--;
    
    block_write( true, ptr_array, _array_num_items_offset, & num_items, (sizeof(num_items)*8) );
}


// pointer array

pointer reference_array_construct( block * p_m )
{
    pointer p = block_alloc( p_m, sizeof( unsigned int ) * 8, "reference array" );
    
    unsigned int zero = 0;
        
    block_write( true, p, 0, & zero, sizeof( zero ) * 8 );
    
    return p;
}

void reference_array_destroy( pointer p )
{
    unsigned int items;
    
    block_read( true, p, 0, & items, sizeof( items ) * 8 );

    unsigned int i;
           
    for( i= 0; i < items; i ++ )
    {
        block_revoke_reference(
            p.memory( ),
            p.offset( ) +
                (( sizeof( items ) + (sizeof(offset_t ) * i )) * 8) );
    }
    
    block_free( p, ( sizeof( items ) + sizeof( offset_t ) * i ) * 8 );
}

void reference_array_add_tail( pointer p_ra, pointer p )
{
    unsigned int items;
    
    block_read( true, p_ra, 0, & items, sizeof( items ) * 8);
    
    offset_t offset = p.offset( );
    
    block_insert( true, p_ra, ( sizeof( items ) + ( sizeof(offset_t) * items ) ) * 8 ,
        p.p_offset( ), sizeof( offset_t) * 8 );
        
    block_add_reference( p_ra.memory( ),
        p_ra.offset( ) + (( sizeof( items ) + (items * sizeof( offset_t ))) * 8 ) );
        
    items++;
    
    block_write( true, p_ra, 0, & items, sizeof( items ) * 8 );
    
}

/*void  _reference_array_add_head( pointer p_ra, pointer p )
{
    unsigned int items;
    block_read( true, p_ra, 0, & items, sizeof( items ) * 8 );

    offset_t offset = p.offset( );
        
    block_insert( p_ra, sizeof( items ) * 8, & offset, sizeof( offset ) * 8 );
    
    block_add_reference( p_ra.memory(), p_ra.offset( ) + (sizeof(items)*8) );
        
    items++;
    
    block_write( true, p_ra, 0, & items, sizeof( items ) * 8 );
 
    
} */

pointer reference_array_item_at( pointer p_ra, unsigned int index )
{
    unsigned int items;
    block_read( true, p_ra, 0, & items, sizeof( items ) * 8 );
    
    rAssert( index < items );

    offset_t offset;
    
    block_read( true, p_ra, ( sizeof( items ) + ((sizeof(offset_t) * index))) *8 ,
        & offset, sizeof( offset ) * 8 );
        
     rAssert( offset < p_ra.memory( )->size );
     
     return pointer( p_ra.memory( ), offset );
}

void _reference_array_remove_at( pointer p_ra, unsigned int index )
{
    unsigned int items;
    block_read( true, p_ra, 0, & items, sizeof( items ) * 8 );
    
    rAssert( index < items );
    
    block_revoke_reference(
        p_ra.memory(),
            p_ra.offset() +( ( sizeof( items ) + (sizeof(offset_t) * index))*8) );
    
    pointer ptr_free(
        p_ra.memory( ),
            p_ra.offset() +( ( sizeof( items ) + (sizeof(offset_t) * index))*8 )  );
    
    block_delete( ptr_free, sizeof( offset_t ) * 8 );
    
    items--;
    
    block_write( true, p_ra, 0, & items, sizeof( items ) * 8 );
    
}

unsigned int reference_array_num_items( pointer p_ra )
{
    unsigned int items;
    block_read( true, p_ra, 0, & items, sizeof( items ) * 8 );
    
    return items;
}


// string

pointer string_construct( block * p_m, const char * p_string )
{
    unsigned int size = (unsigned int)( strlen( p_string ) + 1 ) * 8;
    
    pointer p = block_alloc( p_m, size, "string" );
    
    block_write( false, p, 0, p_string, size );
    
    return p;
}

void string_get( pointer p , char * p_string, unsigned int buffer_size )
{
    if ( p.offset( ) == 0xFFFFFFFF )
    {
        strncpy( p_string, "null", buffer_size );
    }
    else
    {
        unsigned int len = string_len( p ) + ( sizeof( char ) * 8 );
        
        rAssert( buffer_size * 8 >= len );
        
        block_read( false, p, 0, p_string, len );
    }
}

unsigned int string_len( pointer p  )
{
    char c;
    
    unsigned int i = 0;
    
    do
    {
        block_read( false, p, i, & c, sizeof( c ) * 8 );
        i += ( sizeof( c ) * 8 );
    } while ( c != 0 );
    
    return i - ( sizeof( char ) * 8 );
}

void string_destroy( pointer p )
{
    block_free( p, string_len( p ) + ( sizeof( char ) * 8 ) );
    p = pointer_null;
    
}

// stream wrappers (stdio)

void * stdio_stream_open(  const char * filename, bool write )
{
    return fopen( filename, write ? "wb" : "rb" );
}

bool stdio_stream_write( void * stream, const void * buffer, unsigned int bytes )
{
    return 1 == fwrite( buffer, bytes, 1, (FILE*) stream );
}

bool stdio_stream_read( void * stream, void * buffer, unsigned int bytes )
{
    return 1 == fread( buffer, bytes, 1, (FILE*) stream );
}

bool stdio_stream_close( void * stream )
{
    return fclose( (FILE*) stream ) == 0;
}

stream_functions stdio_stream_functions   = { stdio_stream_write, stdio_stream_read };

// stream wrappers (radfile)

void * radfile_stream_open(  const char * filename, bool write )
{
    IRadFile * pIRadFile;
    
    radFileOpen(
        & pIRadFile,
        filename,
        write );
        
    return pIRadFile;
}

bool  radfile_write( void * stream, const void * buffer, unsigned int size )
{
    ((IRadFile*)stream)->WriteSync( buffer, radMemorySpace_Local );
    
    return true;
}

bool radfile_read( void * stream, void * buffer, unsigned int size )
{
    ((IRadFile*)stream)->ReadSync( buffer, size, radMemorySpace_Local );
    
    return true;
}

bool radfile_stream_close( void * stream )
{
    ((IRadFile*)stream)->Release( );
    
    return true;
}

stream_functions radfile_stream_functions   = { radfile_write, radfile_read };

// file wrappers (radfile)

void * radload_open( const char * filename, bool write )
{
    radLoadStream * p_radload_stream;

    rAssert( false == write );
            
    radLoadCreateFileStream(
        & p_radload_stream,
        filename,
        NULL ); // ?

        
    return p_radload_stream;
}

bool radload_write( void * stream, const void *buffer, unsigned int bytes )
{   
    return false;
}

bool radload_read( void * stream, void * buffer, unsigned int size )
{
    bool ok = ((radLoadStream*)stream)->Read( buffer, size );
    
    return ok;
}

bool radload_close( void * stream )
{
    ((radLoadStream*)stream)->Release( );
    
    return true;
}

stream_functions radload_stream_functions = { radload_write, radload_read };

// file wrappers (memory)



void memory_stream_open ( memory_stream * p_ms, char * p_memory, unsigned int bytes, bool write )
{
    p_ms->p_memory = p_memory;
    p_ms->bytes = bytes;
    p_ms->p_current = p_memory;
    p_ms->write = write;
}

bool memory_stream_write( void * stream, const void *buffer, unsigned int bytes )
{   
    memory_stream * p_ms = (memory_stream*) stream;
    
    if ( ! p_ms->write )
    {
        return false;
    }
    
    unsigned int bytes_left = ( p_ms->p_memory + p_ms->bytes )- p_ms->p_current;
    
    if ( bytes_left < bytes )
    {
        return false;
    }
    
    ::memcpy( p_ms->p_current, buffer, bytes );
    p_ms->p_current += bytes;
    
    return true;
}

bool memory_stream_read(  void * stream, void * buffer, unsigned int size  )
{
    memory_stream * p_ms = (memory_stream*) stream;
    
    if ( p_ms->write )
    {
        return false;
    }
    
    unsigned int bytes_left = p_ms->p_memory - p_ms->p_current;
    
    if ( bytes_left >= size )
    {
        return false;
    }
    
    ::memcpy( buffer, p_ms->p_current, size );
    p_ms->p_current += size;
    
    return true;
}

void  memory_stream_close( memory_stream * p_ms )
{
    ::memset( p_ms, 0, sizeof( *p_ms ) );
}

stream_functions memory_stream_functions =
    { memory_stream_write, memory_stream_read };


// file wrappers (debug printf)

void* debug_printf_open(  const char * filename, bool write )
{
    if ( write ) 
    {
        return (void*) 0x12345678;
    }
    
    return 0;
}

#define PRINTF_BUFFER_SIZE 4096 
bool debug_printf_write( void * stream, const void *buffer, unsigned int size  )
{   
    char buf[ PRINTF_BUFFER_SIZE + 1 ];
    
    unsigned full_block = size / PRINTF_BUFFER_SIZE;
    unsigned extra_chars = size % PRINTF_BUFFER_SIZE;

    unsigned int b;

    for( b = 0; b < full_block; b )
    {
        ::memcpy( buf, ((const char*) buffer) + ( b * PRINTF_BUFFER_SIZE ), PRINTF_BUFFER_SIZE );
        buf[ PRINTF_BUFFER_SIZE ] = 0;
        
        rDebugString( buf );
    }
    
    ::memcpy( buf, ((const char*)buffer) + ( b * PRINTF_BUFFER_SIZE ), extra_chars);
    buf[extra_chars] = 0;
        
    rDebugString( buf );    
    
    return true;
}

bool debug_printf_read( void * stream, void * buffer, unsigned int size )
{
    return false;
}

bool debug_printf_close( void * stream )
{
    return stream != 0;
}

stream_functions debug_printf_stream_functions = { debug_printf_write, debug_printf_read };

int VsnPrintf( char *buffer, size_t count, const char *format, va_list argptr )
{
    return vsnprintf(buffer, count, format, argptr);
}
    
bool stream_printf( stream_info * p_si, const char * fmt, ... )
{
    va_list va_alist;
    char    printfstr[ RAD_DEBUG_PRINT_MAX_LENGTH ];

    va_start( va_alist, fmt ); 

    VsnPrintf( printfstr, RAD_DEBUG_PRINT_MAX_LENGTH, fmt, va_alist );

    printfstr[ RAD_DEBUG_PRINT_MAX_LENGTH - 1 ] = '\0';

    va_end( va_alist ); 

    return p_si->p_sf->p_write( p_si->p_stream, printfstr, strlen( printfstr ) );
}

bool stream_write( stream_info * p_si, const void * buffer, unsigned int chars )
{
    return p_si->p_sf->p_write( p_si->p_stream, buffer, chars );
}

bool stream_read( stream_info * p_si, void * buffer, unsigned int chars )
{
    return p_si->p_sf->p_read( p_si->p_stream, buffer, chars );
}


} // namespace ods