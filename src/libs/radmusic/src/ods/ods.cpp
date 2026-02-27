//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "pch.hpp"
#include <ods/ods.hpp>
#include <raddebug.hpp>
#include <string.h>
#include <stdio.h>

//-----------------------------------------------------------------------------
// Local Constants, Typedefs, and Macros
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Global Data, Local Data, Local Classes
//-----------------------------------------------------------------------------

namespace ods
{
    pointer      _cls_type_attribute_create( block * p_m, const char * p_string, const pointer & ptr_type );
    void         _cls_type_attribute_destroy( const pointer & ptr_attribute );
    void         _cls_type_attribute_name( const pointer & ptr_attribute, char *, unsigned int );
    pointer      _cls_type_attribute_type( const pointer & ptr_attribute );
    void         indent( stream_info * p_si, unsigned int );
    bool         primitive_assemble_name ( const pointer & ptr_project, const pointer & ptr_type, const pointer & ptr_object, const pointer & ptr_instance, char * p_string, unsigned int max_chars );

    bool         log_error( const char * p_msg );
    
    static const unsigned int INDENT = 4;
}

//-----------------------------------------------------------------------------
// Member Functions
//-----------------------------------------------------------------------------

namespace ods
{

//
// type functions
//

typedef void         ( type_create_fx )        ( const pointer & ptr_type, const pointer & ptr_inplace );
typedef void         ( type_destroy_fx )       ( const pointer & ptr_type, const pointer & ptr_inplace );
typedef unsigned int ( type_size_fx )          ( const pointer & ptr_type );
typedef void         ( type_format_name_fx )   ( const pointer & ptr_type, char * p_string, unsigned int size );
typedef bool         ( type_write_text_fx )    ( const pointer & ptr_project, const pointer & ptr_type, const pointer & ptr_instance, stream_info *, unsigned int indent );
typedef bool         ( type_read_text_fx )     ( const pointer & ptr_type, const pointer & ptr_instance, Parser & parser);
typedef bool         ( type_compare_text_fx )  ( const pointer & ptr_type, const pointer & ptr_instance, Parser & parser, bool * p_same );
typedef bool         ( type_assemble_name_fx ) ( const pointer & ptr_project, const pointer & ptr_type, const pointer & ptr_object, const pointer & ptr_instance, char * p_string, unsigned int max_chars );

struct type_functions
{
    type_create_fx        * p_construct_fx;
    type_destroy_fx       * p_destroy_fx;
    type_size_fx          * p_size_fx;
    type_format_name_fx   * p_format_name_fx;
    type_write_text_fx    * p_write_text_fx;
    type_read_text_fx     * p_read_text_fx;
    type_compare_text_fx  * p_compare_text_fx;
    type_assemble_name_fx * p_assemble_name_fx;   
    
};

type_functions g_type_functions[ g_num_types ] =
{
    { uint32_instance_construct,    uint32_instance_destroy, uint32_instance_size, uint32_type_format_name, uint32_instance_write_text, uint32_instance_read_text, uint32_instance_compare_text, primitive_assemble_name },
    { bool_instance_construct,      bool_instance_destroy,   bool_instance_size,   bool_type_format_name,   bool_instance_write_text,   bool_instance_read_text,   bool_instance_compare_text,   primitive_assemble_name },
    { float_instance_construct,     float_instance_destroy,  float_instance_size,  float_type_format_name,  float_instance_write_text,  float_instance_read_text,  float_instance_compare_text,  primitive_assemble_name },
    { string_instance_construct,    string_instance_destroy, string_instance_size, string_type_format_name, string_instance_write_text, string_instance_read_text, string_instance_compare_text, primitive_assemble_name },
    { char_instance_construct,      char_instance_destroy,   char_instance_size,   char_type_format_name,   char_instance_write_text,   char_instance_read_text,   char_instance_compare_text,   primitive_assemble_name },
    { array_instance_construct,     array_instance_destroy,  array_instance_size,  array_type_format_name,  array_instance_write_text,  array_instance_read_text,  array_instance_compare_text,  array_instance_assemble_name },
    { ref_instance_construct,       ref_instance_destroy,    ref_instance_size,    ref_type_format_name,    ref_instance_write_text,    ref_instance_read_text,    ref_instance_compare_text,    primitive_assemble_name },
    { cls_instance_construct,       cls_instance_destroy,    cls_instance_size,    cls_type_format_name,    cls_instance_write_text,    cls_instance_read_text,    cls_instance_compare_text,    cls_instance_assemble_name }
};

bool log_error( const char * p_msg )
{
    return false;
}
    
const offset_t _project_name_offset               = 0;
const offset_t _project_uint32_offset             = _project_name_offset                  + ( sizeof( offset_t ) * 8 );
const offset_t _project_bool_offset               = _project_uint32_offset                + ( sizeof( unsigned char )  * 8 );
const offset_t _project_float_offset              = _project_bool_offset                  + ( sizeof( unsigned char )  * 8 );
const offset_t _project_string_offset             = _project_float_offset                 + ( sizeof( unsigned char )  * 8 );
const offset_t _project_character_offset          = _project_string_offset                + ( sizeof( unsigned char )  * 8 );
const offset_t _project_array_array_offset        = _project_character_offset             + ( sizeof( unsigned char )  * 8 );
const offset_t _project_ref_array_offset          = _project_array_array_offset           + ( sizeof( offset_t ) * 8 );
const offset_t _project_cls_array_offset          = _project_ref_array_offset             + ( sizeof( offset_t ) * 8 );
const offset_t _project_named_object_array_offset = _project_cls_array_offset             + ( sizeof( offset_t ) * 8 );
const offset_t _project_end                       = _project_named_object_array_offset    + ( sizeof( offset_t ) * 8 );

const offset_t _named_object_name_ref_offset      = 0;
const offset_t _named_object_object_ref_offset    = _named_object_name_ref_offset +   ( sizeof( offset_t ) * 8 );
const offset_t _named_object_type_ref_offset      = _named_object_object_ref_offset + ( sizeof( offset_t ) * 8 );
const offset_t _named_object_end                  = _named_object_type_ref_offset +   ( sizeof( offset_t ) * 8 );

pointer project_construct( block * p_m, const char * p_name )
{
    pointer ptr_project = block_alloc( p_m, _project_end, "project" );
    
    block_write( true, ptr_project, _project_uint32_offset, & g_uint32_id, sizeof( g_uint32_id ) * 8 );
    block_write( true, ptr_project, _project_bool_offset, & g_bool_id, sizeof( g_bool_id ) * 8 );
    block_write( true, ptr_project, _project_float_offset, & g_float_id, sizeof( g_float_id ) * 8 );
    block_write( true, ptr_project, _project_string_offset, & g_string_id, sizeof( g_string_id ) * 8 );
    block_write( true, ptr_project, _project_character_offset, & g_character_id, sizeof( g_character_id ) * 8 );
    
    pointer ptr_array_type_array   = reference_array_construct( p_m );
    pointer ptr_ref_type_array     = reference_array_construct( p_m );
    pointer ptr_cls_type_array     = reference_array_construct( p_m );
    pointer ptr_named_object_array = array_construct( p_m, _named_object_end );
    pointer ptr_name               = string_construct( p_m, p_name );
    
    block_write_reference( ptr_project, _project_name_offset,               ptr_name.offset( ) );
    block_write_reference( ptr_project, _project_array_array_offset,        ptr_array_type_array.offset( ) );
    block_write_reference( ptr_project, _project_ref_array_offset,          ptr_ref_type_array.offset( ) );
    block_write_reference( ptr_project, _project_cls_array_offset,          ptr_cls_type_array.offset( ) );
    block_write_reference( ptr_project, _project_named_object_array_offset, ptr_named_object_array.offset( ) );                  
 
    return ptr_project;
 }

void project_destroy( pointer & ptr_project )
{   
    pointer ptr_array_type_array   = block_read_reference( ptr_project, _project_array_array_offset );
    pointer ptr_ref_type_array     = block_read_reference( ptr_project, _project_ref_array_offset );
    pointer ptr_cls_type_array     = block_read_reference( ptr_project, _project_cls_array_offset );
    pointer ptr_named_object_array = block_read_reference( ptr_project, _project_named_object_array_offset );    
    pointer ptr_name               = block_read_reference( ptr_project, _project_name_offset );

    for( unsigned int n = 0; n < array_size( ptr_named_object_array ); n++ )
    {
        pointer ptr_n = array_get_pointer_at( ptr_named_object_array, n );
        
        pointer ptr_string =      block_read_reference( ptr_n, _named_object_name_ref_offset );
        pointer ptr_object =      block_read_reference( ptr_n, _named_object_object_ref_offset );
        pointer ptr_object_type = block_read_reference( ptr_n, _named_object_type_ref_offset );
            
        block_revoke_reference( ptr_n.memory( ), ptr_n.offset( ) + _named_object_name_ref_offset );
        block_revoke_reference( ptr_n.memory( ), ptr_n.offset( ) + _named_object_object_ref_offset );
        block_revoke_reference( ptr_n.memory( ), ptr_n.offset( ) + _named_object_type_ref_offset );
        
        unsigned int size = cls_instance_size( ptr_object_type );
        cls_instance_destroy( ptr_object_type, ptr_object );
        block_free( ptr_object, size );
        string_destroy( ptr_string );
    } 
            
    for( unsigned int a = 0; a < reference_array_num_items( ptr_array_type_array ); a ++ )
    {
        pointer ptr_a = reference_array_item_at( ptr_array_type_array, a );
        
        array_type_destroy( ptr_a );
    }

    for( unsigned int r = 0; r < reference_array_num_items( ptr_ref_type_array ); r ++ )
    {
        pointer ptr_r = reference_array_item_at( ptr_ref_type_array, r );
        
        ref_type_destroy( ptr_r );
    }   

    for( unsigned int c = 0; c < reference_array_num_items( ptr_cls_type_array ); c ++ )
    {
        pointer ptr_c = reference_array_item_at( ptr_cls_type_array, c );
        
        cls_type_destroy( ptr_c);
    }
    
    block_revoke_reference( ptr_project.memory( ), ptr_project.offset( ) + _project_array_array_offset );
    block_revoke_reference( ptr_project.memory( ), ptr_project.offset( ) + _project_cls_array_offset );
    block_revoke_reference( ptr_project.memory( ), ptr_project.offset( ) + _project_ref_array_offset );
    block_revoke_reference( ptr_project.memory( ), ptr_project.offset( ) + _project_named_object_array_offset );
    block_revoke_reference( ptr_project.memory( ), ptr_project.offset( ) + _project_name_offset );
    
    reference_array_destroy( ptr_cls_type_array );
    reference_array_destroy( ptr_ref_type_array );
    reference_array_destroy( ptr_array_type_array );
    array_destroy( ptr_named_object_array );
    string_destroy( ptr_name );
    
    block_free( ptr_project, _project_end );    

    ptr_array_type_array = pointer_null;
    ptr_ref_type_array = pointer_null; 
    ptr_cls_type_array = pointer_null;
    ptr_project        = pointer_null;

}

void project_name( const pointer & ptr_project, char * p_name, unsigned int chars )
{
    pointer ptr_name = block_read_reference( ptr_project, _project_name_offset );
    
    string_get( ptr_name, p_name, chars );
}

pointer project_get_built_in_type( const pointer & ptr_project, unsigned char id )
{
    offset_t offset;
    
    switch( id )
    {
        case g_uint32_id:       offset = _project_uint32_offset;    break;
        case g_bool_id:         offset = _project_bool_offset;      break;  
        case g_float_id:        offset = _project_float_offset;     break;
        case g_string_id:       offset = _project_string_offset;    break;
        case g_character_id:    offset = _project_character_offset; break;
        default:
            offset = 0xFFFFFFFF;
            rAssert(false);
    }
    
    return pointer( ptr_project.memory( ), ptr_project.offset( ) + offset );
}

void project_add_cls_type( const pointer & ptr_project, const pointer & ptr_cls_type)
{
    pointer ptr_cls_type_array = block_read_reference( ptr_project, _project_cls_array_offset );
    
    reference_array_add_tail( ptr_cls_type_array, ptr_cls_type );
}

pointer project_cls_type_at( const pointer & ptr_project, unsigned int i )
{
    pointer ptr_cls_type_array = block_read_reference( ptr_project, _project_cls_array_offset );
    
    return reference_array_item_at( ptr_cls_type_array, i );
}

unsigned int project_num_cls_types( const pointer & ptr_project )
{
    pointer ptr_cls_type_array = block_read_reference( ptr_project, _project_cls_array_offset );
    
    return reference_array_num_items( ptr_cls_type_array );
}

pointer project_cls_type_find_or_alloc( const pointer & ptr_project, const char * p_name )
{
    pointer cls_ptr = project_cls_type_find( ptr_project, p_name );
    
    if ( cls_ptr == pointer_null )
    {
        cls_ptr = cls_type_construct( ptr_project.memory( ), p_name );
        
        pointer ptr_cls_type_array = block_read_reference( ptr_project, _project_cls_array_offset );
        
        reference_array_add_tail( ptr_cls_type_array, cls_ptr );
    }
    
    return cls_ptr;
}

pointer project_cls_type_find( const pointer & ptr_project, const char * p_name )
{
    pointer ptr_cls_type_array = block_read_reference( ptr_project, _project_cls_array_offset );
       
    for( unsigned int i = 0; i < reference_array_num_items( ptr_cls_type_array ); i ++ )
    {
        pointer ptr_cls_type = reference_array_item_at( ptr_cls_type_array, i );
        
        char name[ 256 ];
        cls_type_name( ptr_cls_type, name, 256 );
        
        if ( strcmp( name, p_name ) == 0 )
        {
            return ptr_cls_type;
        }
    }
    
    return pointer_null;
}

unsigned int project_num_ref_types( const pointer & ptr_project )
{
    pointer ptr_ref_array = block_read_reference( ptr_project, _project_ref_array_offset );
    
    return reference_array_num_items( ptr_ref_array );
}

pointer project_ref_type_at( const pointer & ptr_project, unsigned int i )
{
    pointer ptr_ref_type_array = block_read_reference( ptr_project, _project_ref_array_offset );
    
    pointer ptr_ref_type = reference_array_item_at( ptr_ref_type_array, i );
    
    return ptr_ref_type;
}

pointer project_ref_type_find_or_alloc( const pointer & ptr_project, const pointer & ptr_ref_type )
{
    pointer ptr_ref = project_ref_type_find( ptr_project, ptr_ref_type );
    
    if ( ptr_ref == pointer_null )
    {
        ptr_ref = ref_type_construct( ptr_project.memory( ), ptr_ref_type );
        
        pointer ptr_ref_type_array = block_read_reference( ptr_project, _project_ref_array_offset );
        
        reference_array_add_tail( ptr_ref_type_array, ptr_ref );
    }
    
    return ptr_ref;
}

pointer project_ref_type_find( const pointer & ptr_project, const pointer & ptr_type )
{
    pointer ptr_ref_type_array = block_read_reference( ptr_project, _project_ref_array_offset );
    
    unsigned int size = reference_array_num_items( ptr_ref_type_array );
    
    for( unsigned int i = 0; i < size; i ++ )
    {
        pointer ptr_ref_type = reference_array_item_at( ptr_ref_type_array, i );

        if ( ref_type_get_type( ptr_ref_type ) == ptr_type )
        {
            return ptr_ref_type;
        }
        
    }
    
    return pointer_null;
}

unsigned int project_num_array_types( const pointer & ptr_project )
{
    pointer ptr_array_list = block_read_reference( ptr_project, _project_array_array_offset );
    
    return reference_array_num_items( ptr_array_list );
}

pointer project_array_type_at( const pointer & ptr_project, unsigned int i )
{
    pointer ptr_array_list = block_read_reference( ptr_project, _project_array_array_offset );
    
    return reference_array_item_at( ptr_array_list, i );
    
}


pointer project_array_type_find_or_alloc( const pointer & ptr_project, const pointer & ptr_array_type )
{
    pointer ptr_array = project_array_type_find( ptr_project, ptr_array_type );
    
    if ( ptr_array == pointer_null )
    {
        ptr_array = array_type_construct( ptr_project.memory( ), ptr_array_type );
        
        pointer ptr_array_type_array = block_read_reference( ptr_project, _project_array_array_offset );
        
        reference_array_add_tail( ptr_array_type_array, ptr_array );
    }
    
    return ptr_array;
}

pointer project_array_type_find( const pointer & ptr_project, const pointer & ptr_type )
{
    pointer ptr_array_type_array = block_read_reference( ptr_project, _project_array_array_offset );
    
    unsigned int size = reference_array_num_items( ptr_array_type_array );
    
    for( unsigned int i = 0; i < size; i ++ )
    {
        pointer ptr_array_type = reference_array_item_at( ptr_array_type_array, i );
        pointer ptr_array_type_type = array_type_get_type( ptr_array_type );
        
        if ( ptr_array_type_type == ptr_type )
        {
            return ptr_array_type;
        }
        
    }
    
    return pointer_null;
}

void project_add_instance( const pointer & ptr_project, const char * p_name, const pointer & ptr_object, const pointer & ptr_type )
{
    pointer ptr_named_object_array = block_read_reference( ptr_project, _project_named_object_array_offset );

    pointer ptr_named_object = array_add_tail( ptr_named_object_array );

    pointer ptr_string       = string_construct( ptr_project.memory( ), p_name );
    
    block_write_reference( ptr_named_object, _named_object_name_ref_offset, ptr_string.offset( ) );
    block_write_reference( ptr_named_object, _named_object_object_ref_offset, ptr_object.offset( ) );
    block_write_reference( ptr_named_object, _named_object_type_ref_offset, ptr_type.offset( ) );
}

unsigned int project_num_instances( const pointer & ptr_project )
{
    pointer ptr_named_object_array = block_read_reference( ptr_project, _project_named_object_array_offset );

    unsigned int size = array_size( ptr_named_object_array );
    
    return size;
}

pointer project_instance_at( const pointer & ptr_project, unsigned int i, char * p_string, unsigned int len, pointer * p_ptr_type )
{
    pointer ptr_named_object_array = block_read_reference( ptr_project, _project_named_object_array_offset );

    pointer ptr_named_object       = array_get_pointer_at( ptr_named_object_array, i );
    
    if ( p_string != 0 )
    {
        pointer ptr_string             = block_read_reference( ptr_named_object, _named_object_name_ref_offset );        
        string_get( ptr_string, p_string, len );
    }
    
    if ( p_ptr_type != 0 )
    {
        *p_ptr_type                     = block_read_reference( ptr_named_object, _named_object_type_ref_offset );    
    }

    pointer ptr_object             = block_read_reference( ptr_named_object, _named_object_object_ref_offset );    
    return ptr_object;
}

pointer project_find_instance( const pointer & ptr_project, const char * p_name, pointer * p_ptr_type )
{
    pointer ptr_named_object_array = block_read_reference( ptr_project, _project_named_object_array_offset );
    
    unsigned int items = reference_array_num_items( ptr_named_object_array );
    
    for( unsigned int i = 0; i < items; i ++ )
    {
        pointer ptr_named_object = array_get_pointer_at( ptr_named_object_array, i );
        pointer ptr_string = block_read_reference( ptr_named_object, _named_object_name_ref_offset );
        
        char name[ 256 ];
        string_get( ptr_string, name, 256 );
        
        if ( strcmp( p_name, name ) == 0 )
        {
            pointer ptr_object = block_read_reference( ptr_named_object, _named_object_object_ref_offset );
            
            if ( p_ptr_type != 0 )
            {
                *p_ptr_type        = block_read_reference( ptr_named_object, _named_object_type_ref_offset );
            }
            return ptr_object;
        }
    }
    
    return pointer_null;
}
        
void project_assemble_instance_name(
    const pointer & ptr_project,
    const pointer & ptr_instance,
    char * p_string,
    unsigned int max_chars )
{
    pointer ptr_named_object_array = block_read_reference( ptr_project, _project_named_object_array_offset );
    
    unsigned int items = reference_array_num_items( ptr_named_object_array );
    
    for( unsigned int i = 0; i < items; i ++ )
    {
        pointer ptr_named_object = array_get_pointer_at( ptr_named_object_array, i );
        
        pointer ptr_object =
            block_read_reference(
                ptr_named_object, _named_object_object_ref_offset );

        pointer ptr_string = block_read_reference( ptr_named_object, _named_object_name_ref_offset );
            
        if ( ptr_instance == ptr_object )
        {
            string_get( ptr_string, p_string, max_chars );
            return;        
        }
        else
        {
            pointer ptr_type = block_read_reference( ptr_named_object, _named_object_type_ref_offset );

            unsigned char id = type_id( ptr_type );
            
            string_get( ptr_string, p_string, max_chars );
                    
            if ( g_type_functions[ id ].p_assemble_name_fx(
                ptr_project,
                ptr_type,
                ptr_object,
                ptr_instance,
                p_string,
                max_chars ) )
            {
                return;
            }
        }
    }
    
    rAssert( false );
}

void project_find_object_name(
    const pointer & ptr_project,
    const pointer & ptr_object_in,
    char * p_string,
    unsigned int max_chars )
{
    pointer ptr_named_object_array = block_read_reference( ptr_project, _project_named_object_array_offset );
    
    unsigned int items = reference_array_num_items( ptr_named_object_array );
    
    for( unsigned int i = 0; i < items; i ++ )
    {
        pointer ptr_named_object = array_get_pointer_at( ptr_named_object_array, i );
        pointer ptr_object = block_read_reference( ptr_named_object, _named_object_object_ref_offset );   

        if ( ptr_object ==  ptr_object_in )
        {                  
            pointer ptr_string = block_read_reference( ptr_named_object, _named_object_name_ref_offset );
        
            string_get( ptr_string, p_string, max_chars );
        
            return;
         }
    }
    
    rAssert( false );
}

bool project_write_objects_text(
    pointer & ptr_project,
    stream_info * p_si )
{

    
    unsigned int objects = project_num_instances( ptr_project );

    for( unsigned int o = 0; o < objects; o ++ )
    {
        char object_name[ 256 ];
        pointer ptr_object_type;        
        pointer ptr_object = project_instance_at( ptr_project, o, object_name, 256, & ptr_object_type );

        char type_name[ 256 ];
        
        type_format_name( ptr_object_type, type_name, 256 );
        stream_printf( p_si,  "%s %s =", type_name, object_name );                

        unsigned int indent = 0;
        
        unsigned char id = type_id( ptr_object_type );
        
        g_type_functions[ id ].p_write_text_fx( ptr_project, ptr_object_type, ptr_object, p_si, indent );
    }
    
    return true;
}

bool project_read_objects_text(
    ods::pointer & ptr_project,
    stream_info * p_si )
{
    char * p_mem = 0;
    size_t size = 0;
    
    char c;
    
    while ( stream_read( p_si, & c, sizeof( c ) ) )
    {
        p_mem = (char*) ptr_project.memory( )->mem_hooks.p_realloc( p_mem, size, size +1, "...temp..." );
        p_mem[ size ] = c;
        size += 1;
    }
    
    Parser parser( p_mem, size );
    
    bool ok = true;
    
    while( ! parser.IsEof( ) )
    {
        if ( false == parser.SkipToken( "class" ) )
        {
            ok = log_error( 0 );
            break;
        }

        char class_name[ 256 ];
        char instance_name[ 256 ];
        
        if ( false == parser.GetVariableName( class_name, 256 ) )
        {
            ok = log_error( 0 );
            break;
        }
        
        if ( false == parser.GetVariableName( instance_name, 256 ) )
        {
            ok = log_error( 0 );
            break;
        }
        
        pointer ptr_type = project_cls_type_find( ptr_project, class_name );
        
        if ( is_null( ptr_type ) )
        {
            ok = log_error( 0 );
            break;
        }
         
        char id = type_id( ptr_type );
        
        pointer ptr_instance = instance_create( ptr_type, ptr_project.memory( ) );
        
        project_add_instance( ptr_project, instance_name, ptr_instance, ptr_type );
        
        if ( ! parser.SkipToken( "=" ) )
        {
            ok = log_error( 0 );
            break;
        }
        
        if ( false == g_type_functions[ id ].p_read_text_fx( ptr_type, ptr_instance, parser ) )
        {
            ok = false;
            break;
        }

    }
    
    ptr_project.memory( )->mem_hooks.p_free( p_mem );
    
    return ok;
}

// types

void type_format_name( const pointer & ptr_type, char * p_string, unsigned int size )
{
    unsigned char id = type_id( ptr_type );
    
    g_type_functions[ id ].p_format_name_fx( ptr_type, p_string, size );
}

// uint32 

void uint32_type_format_name( const pointer & ptr_type, char * p_string, unsigned int size )
{
    sprintf( p_string, "uint32" );
}

// bool


void bool_type_format_name( const pointer & ptr_type, char * p_string, unsigned int size )
{
    sprintf( p_string, "bool" );
}

void float_type_format_name( const pointer & ptr_type, char * p_string, unsigned int size )
{
    sprintf( p_string, "float" );
}

void string_type_format_name( const pointer & ptr_type, char * p_string, unsigned int size )
{
    sprintf( p_string, "string" );
}

void char_type_format_name( const pointer & ptr_type, char * p_string, unsigned int size )
{
    sprintf( p_string, "char" );
}


// class

const offset_t _cls_id_offset = 0;
const offset_t _cls_string_offset = sizeof( g_cls_id ) * 8;
const offset_t _cls_attribute_array_offset = _cls_string_offset + ( sizeof( offset_t ) * 8 );
const offset_t _cls_end                    = _cls_attribute_array_offset + ( sizeof( offset_t ) * 8 );

pointer cls_type_construct( block * p_m, const char * p_string )
{
    char buf[ 256 ];
    sprintf( buf, "class_type:[%s]", p_string );
    
    pointer ptr_cls = block_alloc( p_m, _cls_end, buf );
    
    pointer ptr_string          = string_construct( p_m, p_string );
    pointer ptr_attribute_array = reference_array_construct(p_m );

    block_write(           true, ptr_cls, _cls_id_offset, & g_cls_id, sizeof( g_cls_id ) * 8);    
    block_write_reference( ptr_cls, _cls_string_offset, ptr_string.offset( ) );
    block_write_reference( ptr_cls, _cls_attribute_array_offset, ptr_attribute_array.offset( ) );
    
    return ptr_cls;
              
}

void cls_type_destroy( const pointer & p )
{
    pointer ptr_string          = block_read_reference( p, _cls_string_offset );
    pointer ptr_attribute_array = block_read_reference( p, _cls_attribute_array_offset );
    
    block_revoke_reference( p.memory( ), p.offset( ) + _cls_string_offset );
    block_revoke_reference( p.memory( ), p.offset( ) + _cls_attribute_array_offset );
    
    string_destroy( ptr_string );
    
    for( unsigned int i = 0; i < reference_array_num_items( ptr_attribute_array ); i ++ )
    {
        pointer p = reference_array_item_at( ptr_attribute_array, i );
        
        _cls_type_attribute_destroy( p );
    }
    
    reference_array_destroy( ptr_attribute_array );

    block_free( p, _cls_end );    
}

void cls_type_add_attribute( const pointer & ptr_cls_type, const char * p_name, const pointer & ptr_type )
{
    pointer ptr_attribute      = _cls_type_attribute_create( ptr_cls_type.memory( ), p_name, ptr_type );
           
    pointer ptr_attribute_list = block_read_reference( ptr_cls_type, _cls_attribute_array_offset );
    
    reference_array_add_tail( ptr_attribute_list, ptr_attribute );
}

void cls_type_name( const pointer & ptr_cls_type, char * p_string, unsigned int buf_size )
{
    pointer ptr_string = block_read_reference( ptr_cls_type, _cls_string_offset );
    
    string_get( ptr_string, p_string, buf_size );
}

unsigned int cls_type_num_attributes( const pointer & ptr_cls_type )
{
    pointer ptr_attribute_list = block_read_reference( ptr_cls_type, _cls_attribute_array_offset );
    
    return reference_array_num_items( ptr_attribute_list );
}

void cls_type_attribute_name( const pointer & ptr_cls_type, unsigned int a, char * p_string, unsigned int size )
{
    pointer ptr_attribute_list = block_read_reference( ptr_cls_type, _cls_attribute_array_offset );
    
    pointer ptr_attribute      = reference_array_item_at( ptr_attribute_list, a );
    
    _cls_type_attribute_name( ptr_attribute, p_string, size );
}

unsigned int cls_type_find_attribute( const pointer & ptr_cls_type, const char * p_name )
{
    pointer ptr_attribute_list = block_read_reference( ptr_cls_type, _cls_attribute_array_offset );
    
    unsigned int num_attributes = reference_array_num_items( ptr_attribute_list );
    
    for( unsigned int a = 0; a < num_attributes; a ++ )
    {
        char name[ 256 ];
        
        cls_type_attribute_name( ptr_cls_type, a,  name, 256 );
        
        if ( strcmp( p_name , name ) == 0 )
        {
            return a;
        }
    }
    
    return 0xFFFFFFFF;
}

pointer cls_type_attribute_type( const pointer & ptr_cls_type, unsigned int a )
{
    pointer ptr_attribute_list = block_read_reference( ptr_cls_type, _cls_attribute_array_offset );
    
    pointer ptr_attribute      = reference_array_item_at( ptr_attribute_list, a );
    
    return _cls_type_attribute_type( ptr_attribute );
}

offset_t cls_type_attribute_offset( const pointer & ptr_cls_type, unsigned int attrib )
{
    unsigned int num_attributes = cls_type_num_attributes( ptr_cls_type );
    rAssert( attrib < num_attributes );
    
    unsigned int size = 0;
    
    for( unsigned int a = 0; a < attrib; a ++ )
    {
        pointer ptr_attrib_type = cls_type_attribute_type( ptr_cls_type, a ); 
        size += instance_size( ptr_attrib_type );
    }
    
    return size;
}


unsigned char type_id( const pointer & ptr_cls_type )
{
    unsigned char t;
    block_read( true, ptr_cls_type, 0, & t, sizeof( t ) * 8 );
    return t;
}


// cls attribute

const offset_t _cls_attribute_name_offset = 0;
const offset_t _cls_attribute_type_offset = ( sizeof( offset_t ) * 8 );
const offset_t _cls_attribute_end         = _cls_attribute_type_offset + ( sizeof( offset_t ) * 8 );

void cls_type_format_name( const pointer & ptr_type, char * p_string, unsigned int size )
{
    char type_name[ 256 ];
    
    cls_type_name( ptr_type, type_name, 256 );
    
    sprintf( p_string, "class %s", type_name );

}

pointer _cls_type_attribute_create( block * m_p, const char * p_string, const pointer & ptr_type )
{
    char buf[ 256 ];
    sprintf( buf, "attribute:[%s]", p_string );
    
    pointer ptr_attribute = block_alloc( m_p, _cls_attribute_end, buf );  
     
    pointer ptr_string    = string_construct( m_p, p_string );
    
    block_write_reference( ptr_attribute, _cls_attribute_name_offset, ptr_string.offset( ) );
    block_write_reference( ptr_attribute, _cls_attribute_type_offset, ptr_type.offset( ) );
    
    return ptr_attribute;
}

void _cls_type_attribute_destroy( const pointer & ptr_attribute )
{
    pointer ptr_string = block_read_reference( ptr_attribute, _cls_attribute_name_offset );
        
    string_destroy( ptr_string );
    
    block_revoke_reference( ptr_attribute.memory( ), ptr_attribute.offset( ) + _cls_attribute_name_offset );
    block_revoke_reference( ptr_attribute.memory( ), ptr_attribute.offset( ) + _cls_attribute_type_offset );    
    
    block_free( ptr_attribute, _cls_attribute_end );

}

void _cls_type_attribute_name( const pointer & ptr_attribute, char * p_string, unsigned int  buffer_size)
{
    pointer ptr_string = block_read_reference( ptr_attribute, _cls_attribute_name_offset );
    
    string_get( ptr_string, p_string, buffer_size );
}

pointer _cls_type_attribute_type( const pointer & ptr_attribute )
{
    pointer ptr_type = block_read_reference( ptr_attribute, _cls_attribute_type_offset );
    
    return ptr_type;
}


// ref

const offset_t _reftype_id_offset = 0;
const offset_t _ref_type_type_offset = sizeof( g_ref_id ) * 8;
const offset_t _ref_type_end = _ref_type_type_offset + ( sizeof( offset_t ) * 8 );

void ref_type_format_name( const pointer & ptr_type, char * p_string, unsigned int size )
{
    char type_name[ 256 ];
    
    type_format_name( ref_type_get_type( ptr_type ), type_name, 256 );
    
    sprintf( p_string, "radRef<%s>", type_name );
}

pointer ref_type_construct( block * p_mem, const pointer & ptr_type )
{
    pointer pointer = block_alloc( p_mem, _ref_type_end, "ref" );
        
    block_write( true, pointer, _reftype_id_offset, & g_ref_id, sizeof( g_ref_id ) * 8 );
    block_write_reference( pointer, _ref_type_type_offset, ptr_type.offset( ) );
    
    return pointer;       
}

pointer ref_type_get_type( const pointer & ptr_ref )
{
    pointer ptr_type = block_read_reference( ptr_ref, _ref_type_type_offset );
    
    return ptr_type;
}

void ref_type_destroy( const pointer & ptr_ref_type )
{
    block_revoke_reference( ptr_ref_type.memory( ), ptr_ref_type.offset( ) + _ref_type_type_offset );
    block_free( ptr_ref_type, _ref_type_end );
}

// array

const offset_t _arraytype_id_offset = 0;
const offset_t _array_type_type_offset = sizeof( g_array_id ) * 8;
const offset_t _array_type_end = _array_type_type_offset + ( sizeof( offset_t ) * 8 );

void array_type_format_name( const pointer & ptr_type, char * p_string, unsigned int size )
{
    char type_name[ 256 ];
    
    type_format_name( array_type_get_type( ptr_type ), type_name, 256 );
    
    sprintf( p_string, "array<%s>", type_name );
}

pointer array_type_construct( block * p_m, const pointer & ptr_type )
{
    pointer ptr_array = block_alloc( p_m, _array_type_end, "array" );
    
    block_write( true, ptr_array, _arraytype_id_offset, & g_array_id, sizeof( g_array_id ) * 8 );
    block_write_reference( ptr_array, _array_type_type_offset, ptr_type.offset( ) );
    
    return ptr_array;
    
    
}

pointer array_type_get_type( const pointer & ptr_array )
{
    pointer ptr_type = block_read_reference( ptr_array, _array_type_type_offset );
    
    return ptr_type;
}

void array_type_destroy( const pointer & ptr_array_type )
{
    block_revoke_reference( ptr_array_type.memory( ), ptr_array_type.offset( ) + _array_type_type_offset );
    block_free( ptr_array_type, _array_type_end );
}

//============================================================================
//
//
//
//
// Instances
//
//
//
//
//============================================================================

// instance
            
void instance_destroy( const pointer & ptr_instance_type, const pointer & ptr_instance )
{
    unsigned char id = type_id( ptr_instance_type );
    
    g_type_functions[ id ].p_destroy_fx( ptr_instance_type, ptr_instance );
 
}

void instance_construct( const pointer & ptr_type, const pointer & ptr_inplace )
{
    unsigned char id = type_id( ptr_type );
    
    g_type_functions[ id ].p_construct_fx( ptr_type, ptr_inplace ); 
}

pointer instance_create( const pointer & ptr_type, block * p_m )
{
    unsigned char id = type_id( ptr_type );
    unsigned int size = g_type_functions[ id ].p_size_fx( ptr_type);
    
    pointer ptr_instance = block_alloc( p_m, size, "type (a&c)" );
    g_type_functions[ id ].p_construct_fx( ptr_type, ptr_instance );
    
    return ptr_instance;
}

unsigned int instance_size( const pointer & ptr_type )
{
    unsigned char id = type_id( ptr_type );
    
    unsigned int size = g_type_functions[ id ].p_size_fx( ptr_type );
    
    return size;
}

bool instance_write_text( const pointer & ptr_project, const pointer & ptr_type, const pointer & ptr_instance, stream_info * p_si, unsigned int indent)
{
    unsigned char id = type_id( ptr_type );
    
    return g_type_functions[ id ].p_write_text_fx( ptr_project, ptr_type, ptr_instance, p_si, indent );
}

bool instance_read_text( const pointer & ptr_type, const pointer & ptr_instance, Parser & parser )
{
    unsigned char id = type_id( ptr_type );
    
    return g_type_functions[ id ].p_read_text_fx( ptr_type, ptr_instance, parser );
}

bool instance_compare_text( const pointer & ptr_type, const pointer & ptr_instance, Parser & parser, bool * p_same )
{
    unsigned char id = type_id( ptr_type );
    
    return g_type_functions[ id ].p_compare_text_fx( ptr_type, ptr_instance, parser, p_same );
}


// uint32 instance

bool uint32_instance_write_text( const pointer & ptr_project, const pointer & ptr_type, const pointer & ptr_instance, stream_info * p_si, unsigned int indent)
{
    unsigned int value = uint32_instance_read( ptr_instance );
    
    return stream_printf( p_si, "%u", value );
}

bool uint32_instance_read_text( const pointer & ptr_type, const pointer & ptr_instance, Parser & parser)
{

    if ( ! parser.IsNumber( ) )
    {
        return log_error( 0 );
    }
    
    unsigned int i = parser.GetInteger( );
    
    ods::uint32_instance_write( ptr_instance, i );
    
    return  true;
}

bool uint32_instance_compare_text( const pointer & ptr_type, const pointer & ptr_instance, Parser & parser, bool * p_same)
{
    if ( ! parser.IsNumber( ) )
    {
        *p_same = false;    
        return log_error( 0 );
    }
    
    unsigned int i = parser.GetInteger( );
    
    unsigned int val = ods::uint32_instance_read( ptr_instance );
    
    *p_same = ( val == i );
    
    return  true;
}


void uint32_instance_write( pointer ptr_uint32, unsigned int v )
{
    block_write( true, ptr_uint32, 0, & v, sizeof( v ) * 8 );
}

unsigned int uint32_instance_read( pointer ptr_uint32 )
{
    unsigned int i;
    
    block_read( true, ptr_uint32, 0, & i, sizeof( i ) * 8 );
    
    return i;
}

void uint32_instance_construct( const pointer & ptr_type, const pointer & ptr_inplace )
{
    uint32_instance_construct( ptr_inplace );

}

void uint32_instance_construct( const pointer & ptr_inplace )
{
    unsigned int zero = 0;
    
    block_write( true, ptr_inplace, & zero, sizeof( zero ) * 8 );
}

void uint32_instance_destroy( const pointer & ptr_instance )
{
}

void uint32_instance_destroy( const pointer & ptr_type, const pointer & ptr_instance )
{
}

unsigned int uint32_instance_size( const pointer & ptr_type )
{
    return sizeof( unsigned int ) * 8;
}


// string instance

bool string_instance_write_text( const pointer & ptr_project, const pointer & ptr_type, const pointer & ptr_instance, stream_info * p_si, unsigned int indent )
{
    char string_val[ 256 ];
    string_instance_read( ptr_instance, string_val, 256 );
    return stream_printf( p_si,  "\"%s\"", string_val );
}

bool string_instance_read_text( const pointer & ptr_type, const pointer & ptr_instance, Parser & parser )
{
    char s[ 256 ];
    
    if ( ! parser.GetQuotedString( s, 256 ) )
    {
        return log_error( 0 );
    }
    
    string_instance_write( ptr_instance, s );
    
    return true;
}

bool string_instance_compare_text( const pointer & ptr_type, const pointer & ptr_instance, Parser & parser, bool * p_same )
{
    char buf[ 2048 ];
    
    if ( ! parser.GetQuotedString( buf, 2048 ) )
    {
        *p_same = false;
        return log_error( 0 );
    }
    
    char string_val[ 256 ];
    string_instance_read( ptr_instance, string_val, 256 );

    *p_same = ( strcmp( string_val, buf ) == 0 );
    
    return true;
}

void string_instance_construct( const pointer & ptr_inplace )
{
    pointer ptr_string = string_construct( ptr_inplace.memory( ), "null" );
    
    block_write_reference( ptr_inplace, 0, ptr_string.offset( ) );
}

void string_instance_construct( const pointer & ptr_type, const pointer & ptr_inplace )
{
    string_instance_construct(  ptr_inplace );
}

void string_instance_write( const pointer & ptr_string_ref, const char * p_string )
{    
    pointer ptr_string = block_read_reference( ptr_string_ref, 0 );
    
    if ( not_null( ptr_string ) )
    {
        block_revoke_reference( ptr_string.memory( ), ptr_string_ref.offset() );
        string_destroy( ptr_string );
    }
    
    ptr_string = string_construct( ptr_string.memory( ), p_string );

    block_write_reference( ptr_string_ref, 0, ptr_string.offset( ) );
}

void string_instance_read( const pointer & ptr_string_ref, char * p_string, unsigned int size )
{
    pointer ptr_string = block_read_reference( ptr_string_ref, 0 );
    
    string_get( ptr_string, p_string, size );
}

void string_instance_destroy( const pointer & ptr_instance )
{
    pointer ptr_string = block_read_reference( ptr_instance, 0 );
    
    if ( not_null( ptr_string ))
    {
        block_revoke_reference( ptr_instance.memory( ), ptr_instance.offset( ) + 0 );
    
        string_destroy( ptr_string );
    }
}

void string_instance_destroy( const pointer & ptr_type, const pointer & ptr_instance )
{
    string_instance_destroy( ptr_instance );
}

unsigned int string_instance_size( const pointer & ptr_type )
{
    return sizeof( offset_t ) * 8;
}


// ref instance

bool ref_instance_write_text(
    const pointer & ptr_project,
    const pointer & ptr_type,
    const pointer & ptr_instance,
    stream_info * p_si,
    unsigned int indent )
{
    pointer ptr_object = ref_instance_read( ptr_instance );
    
    if ( ptr_object.offset( ) == 0xFFFFFFFF )
    {
        return stream_printf( p_si, "" );
    }
    else
    {
        char name[ 256 ];
        strcpy( name, "" );
        
        project_assemble_instance_name( ptr_project, ptr_object, name, 256 );
        
        return stream_printf( p_si, "[%s]", name );
    }   
    
}

bool ref_instance_read_text( const pointer & ptr_type, const pointer & ptr_instance, Parser & parser )
{
    rAssert( false );
    
    return log_error( 0 );
}

bool ref_instance_compare_text( const pointer & ptr_type, const pointer & ptr_instance, Parser & parser, bool * p_same )
{
    rAssert( false );
    
    *p_same = false;    
    return log_error( 0 );
}

void ref_instance_construct( const pointer & ptr_type, const pointer & ptr_inplace )
{
    ref_instance_construct( ptr_inplace );
}

void ref_instance_construct( const pointer & ptr_inplace )
{
    offset_t o = 0xFFFFFFFF;
    
    block_write( true, ptr_inplace, 0, & o, sizeof( o ) * 8 );
}

void ref_instance_write( const pointer & ptr_ref, const pointer & ptr_value )
{
    pointer ptr_current_ref = block_read_reference( ptr_ref, 0 );
    
    if ( not_null( ptr_current_ref ) )
    {
        block_revoke_reference( ptr_ref.memory( ), ptr_ref.offset( ) );
    }
    
    if ( not_null( ptr_value ) )
    {    
        block_write_reference( ptr_ref, 0, ptr_value.offset( ) );
    }
}

pointer ref_instance_read( const pointer & ptr_ref )
{
    return block_read_reference( ptr_ref, 0 );
}


void ref_instance_destroy( const pointer & ptr_instance )
{
    pointer ptr_ref = block_read_reference( ptr_instance, 0 );
    
    if ( not_null( ptr_ref ) )
    {
        block_revoke_reference( ptr_instance.memory( ), ptr_instance.offset( ) + 0 );
    }
}

void ref_instance_destroy( const pointer & ptr_type, const pointer & ptr_instance )
{
    ref_instance_destroy( ptr_instance );
}

unsigned int ref_instance_size( const pointer & ptr_type )
{
    return sizeof( offset_t ) * 8;
}

// bool instance

bool bool_instance_write_text(
    const pointer & ptr_project,
    const pointer & ptr_type,
    const pointer & ptr_instance,
    stream_info * p_si, unsigned int indent )
{
    bool b = bool_instance_read( ptr_instance );
    return stream_printf( p_si,  "%s", b ? "true" : "false" );
}

bool bool_instance_read_text( const pointer & ptr_type, const pointer & ptr_instance, Parser & parser )
{
    bool val;
    
    if ( parser.SkipToken( "true" ) )
    {
        val = true;
    }
    else if ( parser.SkipToken( "false" ) )
    {
        val = false;
    }
    else
    {
        return log_error( 0 );
    }
    
    bool_instance_write( ptr_instance, val );
    
    return true;
}

bool bool_instance_compare_text(
    const pointer & ptr_type,
    const pointer & ptr_instance,
    Parser & parser,
    bool * p_same )
{
    bool c;
    
    if ( ! parser.SkipToken( "true" ) )
    {
        c = true;
    }
    else if ( parser.SkipToken( "false" ) )
    {
        c = false;
    }
    else
    {
        *p_same = false;
        return log_error( 0 );
    }
    
    bool b = bool_instance_read( ptr_instance );
    
    *p_same = ( b == c );
    
    return true;
}


void bool_instance_construct( const pointer & ptr_type, const pointer & ptr_inplace )
{
    bool_instance_construct( ptr_inplace );
}

void bool_instance_construct( const pointer & ptr_inplace )
{
    unsigned char f = 0;
    
    block_write( true, ptr_inplace, & f, sizeof( f ) * 8 );
}

void bool_instance_destroy( const pointer & ptr_instance )
{
}

void bool_instance_destroy( const pointer & ptr_type, const pointer & ptr_instance )
{
}

unsigned int bool_instance_size( const pointer & ptr_type )
{
    return sizeof( unsigned char ) * 8;
}

void bool_instance_write( const pointer & ptr_instance, bool value )
{
    unsigned char b = value ? 1 : 0;
    
    block_write( true, ptr_instance, & b, sizeof( b ) * 8 );
}

bool bool_instance_read( const pointer & ptr_instance )
{
    rAssert( not_null( ptr_instance ) );
    
    unsigned char b;
    
    block_read( true, ptr_instance, & b, sizeof( b ) * 8 );
    
    return b == 1;
}

// float instance

bool float_instance_write_text( const pointer & ptr_project, const pointer & ptr_type, const pointer & ptr_instance, stream_info * p_si, unsigned int indent )
{
    float f = float_instance_read( ptr_instance );
    return stream_printf( p_si,  "%f", f );
}

bool float_instance_read_text( const pointer & ptr_type, const pointer & ptr_instance, Parser & parser )
{
    if ( ! parser.IsNumber( ) )
    {
        return log_error( 0 );
    }
    
    float f = parser.GetFloat( );
    
    float_instance_write( ptr_instance, f );
    
    return true;
}

bool float_instance_compare_text(
    const pointer & ptr_type,
    const pointer & ptr_instance, 
    Parser & parser,
    bool * p_same )
{
    if ( ! parser.IsNumber( ) )
    {
        *p_same = false;
        return log_error( 0 );
    }
    
    float c = parser.GetFloat( );
    
    float f = float_instance_read( ptr_instance );

    *p_same = ( f == c );
    
    return true;
}

void float_instance_construct( const pointer & ptr_inplace )
{
    float zero = 0.0f;
    
    block_write( true, ptr_inplace, & zero, sizeof( zero ) * 8 );
}

void float_instance_construct( const pointer & ptr_type, const pointer & ptr_inplace )
{
    float_instance_construct( ptr_inplace );
}

unsigned int float_instance_size( const pointer & ptr_type )
{
    return sizeof( float ) * 8;
}

void float_instance_destroy( const pointer & ptr_instance )
{
}


void float_instance_destroy( const pointer & ptr_type, const pointer & ptr_instance )
{
}

void  float_instance_write( const pointer & ptr_instance, float value )
{
    block_write( true, ptr_instance, & value, sizeof( value ) * 8 );
}

float float_instance_read( const pointer & ptr_instance )
{
    float f;
    
    block_read( true, ptr_instance, & f, sizeof( f ) * 8 );
    
    return f;
}

// character instance

bool char_instance_write_text( const pointer & ptr_project, const pointer & ptr_type, const pointer & ptr_instance, stream_info * p_si, unsigned int indent )
{
    char c = char_instance_read( ptr_instance );
            
    return stream_printf( p_si,  "%d", c );
}

bool char_instance_read_text( const pointer & ptr_type, const pointer & ptr_instance, Parser & parser )
{
    char c;
    
    if ( ! parser.GetChar( & c ) )
    {
        return log_error( 0 );
    }
    
    char_instance_write( ptr_instance, c );
    
    return true;
}

bool char_instance_compare_text(
    const pointer & ptr_type,
    const pointer & ptr_instance,
    Parser & parser,
    bool * p_same )
{
    char c;
    
    if ( ! parser.GetChar( & c ) )
    {
        return log_error( 0 );
    }
    
    char ch = char_instance_read( ptr_instance );
    
    *p_same = ( ch == c );
    
    return true;
}

void char_instance_construct( const pointer & ptr_inplace )
{
    char c = 0;
    
    block_write( true, ptr_inplace, & c, sizeof( c ) * 8 );
}

void char_instance_construct( const pointer & ptr_type, const pointer & ptr_inplace )
{
    char_instance_construct( ptr_inplace );
}

void char_instance_destroy( const pointer & ptr_instance )
{
}

void char_instance_destroy( const pointer & ptr_type, const pointer & ptr_instance )
{
}

unsigned int char_instance_size( const pointer & ptr_type )
{
    return sizeof( char ) * 8;
}

void char_instance_write( const pointer & ptr_instance, char c )
{
    block_write( true, ptr_instance, & c, sizeof( c ) * 8 );
}

char char_instance_read( const pointer & ptr_instance )
{
    char c;
    
    block_read( true, ptr_instance, & c, sizeof( c ) * 8 );
    
    return c;
}



// array instance

bool array_instance_write_text( const pointer & ptr_project, const pointer & ptr_type, const pointer & ptr_instance, stream_info * p_si, unsigned int ind )
{
    pointer ptr_array_item_type = array_type_get_type( ptr_type );
    unsigned int items = array_instance_num_items( ptr_instance );
    stream_printf( p_si, "\n" );
    indent( p_si, ind );
    stream_printf( p_si,  "[\n" );
    for( unsigned int i = 0; i < items; i ++ )
    {
        pointer ptr_item = array_instance_item_at( ptr_instance, instance_size( ptr_type ), i );
        indent( p_si, ind + INDENT );
        instance_write_text( ptr_project, ptr_array_item_type, ptr_item, p_si, ind + INDENT );
        stream_printf( p_si,  "\n" );
    }
    
    indent( p_si, ind );
    stream_printf( p_si,  "]" );
    
    return true;
}

bool array_instance_read_text( const pointer & ptr_type, const pointer & ptr_instance, Parser & parser )
{
    if ( ! parser.SkipToken( "[" ) )
    {
        return log_error( 0 );
    }

    pointer ptr_array_type = ods::array_type_get_type( ptr_type );
            
    while ( ! parser.SkipToken( "]" ) )
    {
        pointer ptr_new_item =
            array_instance_add_item( ptr_instance, instance_size( ptr_array_type ) );
            
        instance_construct( ptr_array_type, ptr_new_item );
        
        if ( ! instance_read_text( ptr_array_type, ptr_new_item, parser ) )
        {
            return log_error( 0 );
        }
        
    }
       
    return true;
}

bool array_instance_compare_text(
    const pointer & ptr_type,
    const pointer & ptr_instance,
    Parser & parser,
    bool * p_same )
{
    if ( ! parser.SkipToken( "[" ) )
    {
        return log_error( 0 );
    }

    pointer ptr_array_type = ods::array_type_get_type( ptr_type );

    unsigned int num_items = ods::array_instance_num_items( ptr_instance );

    unsigned int item = 0;
                    
    while ( ! parser.SkipToken( "]" ) )
    {
        if ( item >= num_items )
        {
            *p_same = false;
            return false;
        }
        
        pointer ptr_item =
            array_instance_item_at(
                ptr_instance,
                instance_size( ptr_array_type ),
                item );  
                
        item++;                          
        
        bool same;
        
        if ( ! instance_compare_text( ptr_array_type, ptr_item, parser, & same ) )
        {
            return log_error( 0 );
        }
        
        if ( same == false )
        {
            *p_same = false;
            return true;
        }        
    }
    
    return false;
}

bool array_instance_assemble_name( 
    const pointer & ptr_project,
    const pointer & ptr_parent_type,
    const pointer & ptr_parent_object,
    const pointer & ptr_object_search,
    char * p_string,
    unsigned int max_chars )
{
    unsigned int items = array_instance_num_items( ptr_parent_object );
    unsigned int item_size = instance_size( array_type_get_type( ptr_parent_type ) );
    
    pointer ptr_array_type_type = array_type_get_type( ptr_parent_type );
    unsigned int id = type_id( ptr_array_type_type );
    
    for( unsigned int i = 0; i < items; i ++ )
    {
        pointer ptr_array_item = array_instance_item_at( ptr_parent_object, item_size, i );
        
        if ( ptr_array_item == ptr_object_search )
        {
            char buf[ 256 ];
            sprintf( buf, ".%d", i );
            strncat( p_string, buf, max_chars );
            return true;
        }
        else
        {
            char buf[ 256 ];
            sprintf( buf, ".%d", i );        
            if ( g_type_functions[ id ].p_assemble_name_fx( ptr_project, ptr_array_type_type, ptr_array_item, ptr_object_search, buf, 256 ) )
            {
                strncat( p_string, buf, max_chars );
                return true;
            }
        }
            
    
    }
    
    return false;
    
}

void array_instance_construct( const pointer & ptr_inplace, unsigned int item_size )
{
    pointer ptr_array = array_construct( ptr_inplace.memory( ), item_size );
    
    block_write_reference( ptr_inplace, 0, ptr_array.offset( ) );
}

void array_instance_construct( const pointer & ptr_type, const pointer & ptr_inplace )
{
    pointer ptr_array_type = array_type_get_type( ptr_type );
    
    array_instance_construct( ptr_inplace, instance_size( ptr_array_type ) );
}

void array_instance_destroy( const pointer & ptr_type, const pointer & ptr_instance )
{
    pointer ptr_array = block_read_reference( ptr_instance, 0 );
    pointer ptr_array_item_type = array_type_get_type( ptr_type );
    
    block_revoke_reference( ptr_instance.memory( ), ptr_instance.offset( ) + 0 );
        
    unsigned int size =array_size( ptr_array );
    
    for( unsigned int i = 0; i < size; i ++ )
    {
        pointer ptr_item = array_get_pointer_at( ptr_array, i );
        
        instance_destroy( ptr_array_item_type, ptr_item );
    }
    
    array_destroy( ptr_array );
}

void array_instance_destroy( const pointer & ptr_array, unsigned int item_size_in_bits )
{
    block_revoke_reference( ptr_array.memory( ), ptr_array.offset( ) + 0 );
    
    array_destroy( block_read_reference( ptr_array ) );
}

unsigned int array_instance_size( const pointer & ptr_type )
{
    return sizeof( offset_t ) * 8;
}

unsigned int array_instance_num_items( const pointer & ptr_instance )
{
    pointer ptr_array = block_read_reference( ptr_instance, 0 );
    
    return array_size( ptr_array );
}

pointer array_instance_add_item( const pointer & ptr_array_instance, unsigned int item_size_in_bits )
{
    pointer ptr_array = block_read_reference( ptr_array_instance, 0 );
    
    pointer ptr_item  = array_add_tail( ptr_array );
    
    return ptr_item;
}

void array_instance_remove_item( const ods::pointer & ptr_array_instance, unsigned int item_size, unsigned int index )
{
    pointer ptr_array = block_read_reference( ptr_array_instance, 0 );
    
    array_remove_at( ptr_array, index );
}


pointer array_instance_item_at( const pointer & ptr_array_instance, unsigned int item_size, unsigned int index )
{
    pointer ptr_array = block_read_reference( ptr_array_instance, 0 );
    
    pointer ptr_at    = array_get_pointer_at( ptr_array, index );
    
    return ptr_at;
}


// cls instance

bool cls_instance_write_text( const pointer & ptr_project, const pointer & ptr_type, const pointer & ptr_instance, stream_info * p_si, unsigned int ind )
{
    stream_printf( p_si,  "\n" );            
    indent( p_si, ind );
    stream_printf( p_si,  "[\n" );
    unsigned int num_attributes = cls_type_num_attributes( ptr_type );

    for( unsigned int i = 0; i < num_attributes; i ++ )
    {
        char type_name[ 256 ];
        char attrib_name[ 256 ];
        cls_type_attribute_name( ptr_type, i, attrib_name, 256 );
        pointer ptr_attrib_type = cls_type_attribute_type( ptr_type, i );
        type_format_name( ptr_attrib_type, type_name, 256 );
        
        unsigned char id = type_id( ptr_attrib_type );

        pointer ptr_attribute_instance(
            ptr_instance.memory( ),
            ptr_instance.offset( ) + cls_type_attribute_offset( ptr_type, i ) );

        indent( p_si, ind + INDENT);
        stream_printf( p_si,  "%s = ", attrib_name );
        
        instance_write_text( ptr_project, ptr_attrib_type, ptr_attribute_instance, p_si, ind + INDENT );
        stream_printf( p_si,  "\n" );
    }
    indent( p_si, ind );                
    stream_printf( p_si,  "]\n" );
    
    return true;    
}

bool cls_instance_compare_text( const pointer & ptr_type, const pointer & ptr_instance, Parser & parser, bool * p_same )
{
    if ( ! parser.SkipToken( "[" ) )
    {
        return log_error( 0 );
    }
    
    while( ! parser.SkipToken( "]" ) )
    {
        char attribute_name[ 256 ];
        
        if ( ! parser.GetVariableName( attribute_name, 256 ) )
        {
            return log_error( 0 );
        }
        
        unsigned int attrib_number = ods::cls_type_find_attribute( ptr_type, attribute_name );
        
        if ( 0xFFFFFFFF == attrib_number )
        {
            return log_error( 0 );
        }

        pointer   ptr_attribute_type = cls_type_attribute_type( ptr_type, attrib_number );                
        offset_t attrib_offset = cls_type_attribute_offset( ptr_type, attrib_number );        
        
        pointer ptr_attribute( ptr_type.memory( ), ptr_instance.offset( ) + attrib_offset );
        
        if ( ! parser.SkipToken( "==" ) )
        {
            return log_error( 0 );
        }
        
        bool same = false;
        
        if ( ! ods::instance_compare_text( ptr_attribute_type, ptr_attribute, parser, & same ) )
        {
            return log_error( 0 );
        }
        
        if ( same == false )
        {
            *p_same = false;
            return true;
        }
    }

    *p_same = true;
    return true;
}

bool cls_instance_read_text( const pointer & ptr_type, const pointer & ptr_instance, Parser & parser )
{
    if ( ! parser.SkipToken( "[" ) )
    {
        return log_error( 0 );
    }
    
    while( ! parser.SkipToken( "]" ) )
    {
        char attribute_name[ 256 ];
        
        if ( ! parser.GetVariableName( attribute_name, 256 ) )
        {
            return log_error( 0 );
        }
        
        unsigned int attrib_number = ods::cls_type_find_attribute( ptr_type, attribute_name );
        
        if ( 0xFFFFFFFF == attrib_number )
        {
            return log_error( 0 );
        }

        pointer   ptr_attribute_type = cls_type_attribute_type( ptr_type, attrib_number );                
        offset_t attrib_offset = cls_type_attribute_offset( ptr_type, attrib_number );        
        
        pointer ptr_attribute( ptr_type.memory( ), ptr_instance.offset( ) + attrib_offset );
        
        if ( ! parser.SkipToken( "=" ) )
        {
            return log_error( 0 );
        }
        
        if ( ! ods::instance_read_text( ptr_attribute_type, ptr_attribute, parser ) )
        {
            return log_error( 0 );
        }
    }

    return true;
}

bool cls_instance_assemble_name(
    const pointer & ptr_project,
    const pointer & ptr_parent_type,
    const pointer & ptr_parent_object,
    const pointer & ptr_object_search,
    char * p_string,
    unsigned int max_chars )
{
    unsigned int num_attributes = cls_type_num_attributes( ptr_parent_type );
            
    unsigned int offset = 0;
    
    for( unsigned int i = 0; i < num_attributes; i ++ )
    {
        pointer ptr_attrib_type = cls_type_attribute_type( ptr_parent_type, i ); 
    
        char buf[ 256 ];
        char attrib_name[ 256 ];
        cls_type_attribute_name( ptr_parent_type, i, attrib_name, 256 );
        
        strcpy( buf, "." );
        strcat( buf, attrib_name );
            
        if ( ptr_object_search.offset() == offset )
        {
            strcat( p_string, buf );

            return true;        
        }
        else
        {
            pointer ptr_attrib = pointer( ptr_parent_object.memory( ), ptr_parent_object.offset( ) + offset );
            
            unsigned char id = type_id( ptr_attrib_type );
            
            if ( g_type_functions[ id ].p_assemble_name_fx( ptr_project, ptr_attrib_type, ptr_attrib, ptr_object_search, buf, 256 ) )
            {
                strcat( p_string, buf );            
                return true;
            }
       
        }
        
        offset += instance_size( ptr_attrib_type );
    }
    
    return false;

}




void cls_instance_construct( const pointer & ptr_type, const pointer & ptr_inplace )
{        
    unsigned int num_attributes = cls_type_num_attributes( ptr_type );  
    
    unsigned int pos = 0;
    
    for( unsigned int a = 0; a < num_attributes; a ++ )
    {
        pointer ptr_attribute_type = cls_type_attribute_type( ptr_type, a );
        
        instance_construct( ptr_attribute_type, pointer( ptr_inplace.memory( ), ptr_inplace.offset( ) + pos ) );
        
        pos += instance_size( ptr_attribute_type );
    }
}

void cls_instance_destroy( const pointer & ptr_object_type, const pointer & ptr_object )
{
    
    unsigned int num_attributes = cls_type_num_attributes( ptr_object_type );
    
    unsigned int pos = 0;
    
    for( unsigned int a = 0; a < num_attributes; a ++ )
    {
        pointer ptr_attribute_type = cls_type_attribute_type( ptr_object_type, a );
        
        instance_destroy( ptr_attribute_type, pointer( ptr_object.memory( ), ptr_object.offset( ) + pos ) );        
        
        pos += instance_size( ptr_attribute_type );
    }
}

pointer cls_instance_find_attribute(
    const pointer & ptr_object_type,
    const pointer & ptr_object,
    const char * p_name,
    const pointer & ptr_expected_attrib_type )
{
    unsigned int num_attributes = cls_type_num_attributes( ptr_object_type );
    
    unsigned int size = 0;
    
    for( unsigned int a = 0; a < num_attributes; a ++ )
    {
        char name[ 256 ];
        cls_type_attribute_name( ptr_object_type, a, name, 256 );
        pointer ptr_attrib_type = cls_type_attribute_type( ptr_object_type, a );  
        
        if ( strcmp( name, p_name ) == 0 )
        {
            rAssert( ptr_attrib_type == ptr_expected_attrib_type );
            
            return pointer( ptr_object.memory( ), ptr_object.offset( ) + size );
        }
        
        size += instance_size( ptr_attrib_type );
    }
    
    return pointer_null;    
}

unsigned int cls_instance_size( const pointer & ptr_cls_type )
{
    pointer ptr_attribute_list = block_read_reference( ptr_cls_type, _cls_attribute_array_offset );

    unsigned int num_attributes = reference_array_num_items( ptr_attribute_list );
    
    unsigned int size = 0;
    
    for( unsigned int i = 0; i < num_attributes; i ++ )
    {
        pointer ptr_attribute = reference_array_item_at( ptr_attribute_list, i );

        pointer ptr_type = _cls_type_attribute_type( ptr_attribute );
        
        size += instance_size( ptr_type );
    }
    
    return size;    
}

void indent( stream_info * p_si, unsigned int ind )
{
    for( unsigned int i = 0; i < ind; i ++ )
    {
        p_si->p_sf->p_write( p_si->p_stream, " ", 1 );
    }
}

bool primitive_assemble_name ( const pointer & ptr_project, const pointer & ptr_type, const pointer & ptr_object, const pointer & ptr_instance, char * p_string, unsigned int max_chars )
{
    return false;
}

}