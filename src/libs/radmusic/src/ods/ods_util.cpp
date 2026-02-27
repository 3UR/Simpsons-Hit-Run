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
#include <stdio.h>
#include <string.h>
#include <raddebug.hpp>
#include <memory/memory.hpp>

//-----------------------------------------------------------------------------
// Local Constants, Typedefs, and Macros
//-----------------------------------------------------------------------------

namespace ods
{
    const unsigned int STACK_SIZE = 256;
}

//-----------------------------------------------------------------------------
// Global Data, Local Data, Local Classes
//-----------------------------------------------------------------------------

namespace ods
{
    block g_memory;
    pointer ptr_current_type;
    pointer ptr_current_project;

    pointer ptr_current_object[ STACK_SIZE ];
    pointer ptr_current_object_type[ STACK_SIZE ];

    pointer ptr_current_array[ STACK_SIZE ];
    pointer ptr_current_array_type[ STACK_SIZE ];

    unsigned int object_stack_pointer = 0;
    unsigned int array_stack_pointer  = 0;
}


//-----------------------------------------------------------------------------
// Member Functions
//-----------------------------------------------------------------------------

namespace ods
{

void push_object( pointer ptr_object, pointer ptr_object_type )
{
    rAssert( object_stack_pointer < STACK_SIZE );
    
    ptr_current_object[ object_stack_pointer ]      = ptr_object;
    ptr_current_object_type[ object_stack_pointer ] = ptr_object_type;
    
    object_stack_pointer++;
}

bool object_stack_empty( void )
{
    return object_stack_pointer == 0;
}

void pop_object( )
{
    rAssert( object_stack_pointer > 0 );
    
    ptr_current_object[ object_stack_pointer - 1 ] = pointer_null;
    ptr_current_object_type[ object_stack_pointer -1 ] = pointer_null;
    
    object_stack_pointer--;
}

pointer peek_object_type( void )
{
    rAssert( object_stack_pointer > 0 );
    
    return ptr_current_object_type[ object_stack_pointer -1 ];
}

pointer peek_object( void )
{
    rAssert( object_stack_pointer > 0 );
    
    return ptr_current_object[ object_stack_pointer -1 ];
}
 
 
 
 void push_array( pointer ptr_array, pointer ptr_array_type )
{
    rAssert( array_stack_pointer < STACK_SIZE );
    
    ptr_current_array[ array_stack_pointer ]      = ptr_array;
    ptr_current_array_type[ array_stack_pointer ] = ptr_array_type;
    
    array_stack_pointer++;
}

bool array_stack_empty( void )
{
    return array_stack_pointer == 0;
}

void pop_array( )
{
    rAssert( array_stack_pointer > 0 );
    
    ptr_current_array[ array_stack_pointer - 1 ] = pointer_null;
    ptr_current_array_type[ array_stack_pointer -1 ] = pointer_null;
    
    array_stack_pointer--;
}

pointer peek_array_type( void )
{
    rAssert( array_stack_pointer > 0 );
    
    return ptr_current_array_type[ array_stack_pointer -1 ];
}

pointer peek_array( void )
{
    rAssert( array_stack_pointer > 0 );
    
    return ptr_current_array[ array_stack_pointer -1 ];
}

pointer create_schema( const char * p_name, bool reverse_endian )
{
    memory_hooks h;
    h.p_realloc = memory::realloc;
    h.p_free    = memory::free;
    
    block_construct( & g_memory, & h, reverse_endian );
    
    ptr_current_project = project_construct( & g_memory, p_name );
    
    return ptr_current_project;
}

void begin_schema( const pointer & ptr_project )
{
    ptr_current_project = ptr_project;
}

void end_schema( void )
{
    ptr_current_project = pointer_null;
}


/* bool load_schema(
    stream_functions * p_sf,
    const char * p_file_name, )
{
    return block_construct_from_file_synch(
        & g_memory, p_sf, p_file_name, & ptr_current_project, open_realloc );
} */


void destroy_schema( void )
{
    project_destroy( ptr_current_project );
    
    block_destroy( & g_memory );
}

bool save_schema( stream_info * p_si )
{
    return block_save_to_stream_synch( & g_memory, p_si );
}

void begin_type( const char * p_name )
{
    rAssert( ptr_current_type == pointer_null );
    
    ptr_current_type = project_cls_type_find_or_alloc( ptr_current_project, p_name );
}

void end_type( )
{
    ptr_current_type = pointer_null;
}

void attribute( const char* p_name, pointer type )
{
    cls_type_add_attribute( ptr_current_type, p_name, type );
}

pointer uint( )
{   
    return project_get_built_in_type( ptr_current_project, g_uint32_id );
}

pointer string( )
{
    return project_get_built_in_type( ptr_current_project, g_string_id );
}

pointer booln( )
{   
    return project_get_built_in_type( ptr_current_project, g_bool_id );
}

pointer flt( )
{   
    return project_get_built_in_type( ptr_current_project, g_float_id );
}


pointer character( )
{
    return project_get_built_in_type( ptr_current_project, g_character_id );
}


pointer cls( const char * p_name )
{
    return project_cls_type_find_or_alloc( ptr_current_project, p_name );
}


pointer rf( pointer & ptr_type )
{
    return project_ref_type_find_or_alloc( ptr_current_project, ptr_type );
}

 pointer array( pointer & ptr_type )
{
    return project_array_type_find_or_alloc( ptr_current_project, ptr_type );
}

void dump_schema( void )
{     
    unsigned int cls_items   = project_num_cls_types( ptr_current_project ); 
    unsigned int ref_items   = project_num_ref_types( ptr_current_project ); 
    unsigned int array_items = project_num_array_types( ptr_current_project ); 

    char schema_name[ 256 ];
    project_name( ptr_current_project, schema_name, 256 );
    
    rDebugPrintf( "schema: [%s]\n", schema_name );
    rDebugPrintf( "class types:[%d] ref_types:[%d] array_types[%d]\n\n", cls_items, ref_items, array_items );
    
    for( unsigned int i = 0; i < cls_items; i ++ )
    {
        pointer ptr_class = project_cls_type_at( ptr_current_project, i );
        
        char name[ 256 ];
        cls_type_name( ptr_class, name, 256 );
        
        rDebugPrintf( "class %s \n{\n", name );
        
        unsigned int num_attributes = cls_type_num_attributes( ptr_class );
        
        for( unsigned int a = 0; a < num_attributes; a ++ )
        {
            cls_type_attribute_name( ptr_class, a, name, 256 );
            pointer ptr_attribute_type = cls_type_attribute_type( ptr_class, a );
            
            char type_name[256];
            type_format_name( ptr_attribute_type, type_name, 256 );
            
            rDebugPrintf( "    %s %s\n", type_name, name );
        }
        
        rDebugPrintf( "}\n" );      
    }
}

void begin_object( const char * p_name, pointer ptr_type )
{
    pointer ptr_object      = instance_create( ptr_type, ptr_type.memory( ) );
    
    rAssert( not_null( ptr_object ) );
    
    project_add_instance( ptr_current_project, p_name, ptr_object, ptr_type );
    
    push_object( ptr_object, ptr_type );
}

void set_attrib( const char * p_name, unsigned int i )
{
    pointer ptr_int = project_get_built_in_type( ptr_current_project, g_uint32_id );
    pointer attrib = cls_instance_find_attribute( peek_object_type( ), peek_object( ), p_name, ptr_int );
    rAssert( not_null( attrib ) );
    uint32_instance_write( attrib, i );
}

void set_attrib( const char * p_name, bool b )
{
    pointer ptr_bool_type = project_get_built_in_type( ptr_current_project, g_bool_id );
    pointer ptr_attrib = cls_instance_find_attribute( peek_object_type( ), peek_object( ), p_name, ptr_bool_type );
    rAssert( not_null(ptr_attrib) );
    bool_instance_write( ptr_attrib, b );
}

void set_attrib( const char * p_name, float f )
{
    pointer ptr_float_type = project_get_built_in_type( ptr_current_project, g_float_id );
    pointer ptr_attrib    = cls_instance_find_attribute( peek_object_type( ), peek_object( ), p_name, ptr_float_type );
    rAssert( not_null( ptr_attrib ) );
    float_instance_write( ptr_attrib, f );
}

void set_attrib( const char * p_name, const char * s )
{
    pointer ptr_int    = project_get_built_in_type( ptr_current_project, g_string_id );
    pointer ptr_attrib = cls_instance_find_attribute( peek_object_type( ), peek_object( ), p_name, ptr_int );
    string_instance_write( ptr_attrib, s );
}

void set_attrib( const char * p_name, pointer & ptr_object )
{   
    unsigned int index = cls_type_find_attribute( peek_object_type( ), p_name );
    
    rAssert( index != 0xFFFFFFFF );

    pointer ptr_attrib_type = cls_type_attribute_type( peek_object_type( ), index );   

    rAssert( type_id( ptr_attrib_type ) == g_ref_id );
    
    pointer ptr_attrib = cls_instance_find_attribute( peek_object_type( ), peek_object( ), p_name, ptr_attrib_type );
    ref_instance_write( ptr_attrib, ptr_object );
}

void end_object( )
{
    pop_object( );
}

pointer null_ref( void )
{
    return pointer( 0, 0xFFFFFFFF );
}

pointer object( const char * p_name )
{   
    pointer ptr_type;
    return project_find_instance( ptr_current_project, p_name, & ptr_type );
}

void begin_attrib_object( const char * p_attrib_name )
{
    unsigned int attribute = cls_type_find_attribute( peek_object_type( ), p_attrib_name );
    pointer ptr_attribute_type = cls_type_attribute_type( peek_object_type( ), attribute );
    
    pointer ptr_attribute = cls_instance_find_attribute( peek_object_type( ), peek_object( ), p_attrib_name, ptr_attribute_type );
    
    rAssert( type_id( ptr_attribute_type ) == g_cls_id );
        
    push_object( ptr_attribute, ptr_attribute_type );
}

void end_attrib_object( void )
{
    pop_object( );
}

void begin_attrib_array( const char * p_attrib_name )
{
    unsigned int attribute = cls_type_find_attribute( peek_object_type( ), p_attrib_name );
    
    rAssert( attribute != 0xFFFFFFFF );
    
    pointer ptr_attribute_type = cls_type_attribute_type( peek_object_type( ), attribute );
    
    rAssert( type_id( ptr_attribute_type ) == g_array_id );
    
    pointer ptr_attribute = cls_instance_find_attribute( peek_object_type( ), peek_object( ), p_attrib_name, ptr_attribute_type );
    
    push_array( ptr_attribute, ptr_attribute_type );
    
}

void end_attrib_array( void )
{
    pop_array( );
}

void begin_array_item( void )
{
    rAssert( array_stack_empty( ) == false );
    
    pointer ptr_array           = peek_array( );
    pointer ptr_array_type      = peek_array_type( );
    pointer ptr_array_item_type = array_type_get_type( ptr_array_type );
    pointer ptr_new_item = array_instance_add_item( ptr_array, instance_size( ptr_array_type ) );
    instance_construct( ptr_array_item_type, ptr_new_item );
    
    push_object( ptr_new_item, ptr_array_item_type );
}

void end_array_item( )
{
    pop_object( );
}

void add_array_item( char c )
{
    rAssert( not_null( peek_array() ) );
    rAssert( type_id( array_type_get_type( peek_array_type( ) ) )== g_character_id );
    
    pointer ptr_char = array_instance_add_item( peek_array( ), instance_size( peek_array_type( ) )  );
    char_instance_write( ptr_char, c );
}

bool generate_code( stream_info * p_si)
{
    return project_generate_code( ptr_current_project, p_si );
}

}