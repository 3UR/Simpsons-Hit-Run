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
#include <radtime.hpp>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//-----------------------------------------------------------------------------
// Local Constants, Typedefs, and Macros
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Global Data, Local Data, Local Classes [Optional commment can be deleted]
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Member Functions
//-----------------------------------------------------------------------------

namespace ods
{

void _horiz_rule( stream_info * p_si )
{
    stream_printf( p_si, "//-----------------------------------------------------------------------------\n" );
}

void _header( stream_info * p_si, const char * p_text )
{
    stream_printf( p_si, "\n\n" );
    _horiz_rule( p_si );
    stream_printf( p_si, "// " );
    stream_printf( p_si,p_text );
    stream_printf( p_si, "\n" );    
    _horiz_rule( p_si );
}

void _get_type_name( pointer ptr_type, char * p_name, unsigned int len )
{
    unsigned char id = type_id( ptr_type );
    
    switch( id )
    {
        case g_uint32_id:    strncpy( p_name, "uint32", len ); break;  
        case g_bool_id:      strncpy( p_name, "bool", len ); break;
        case g_float_id:     strncpy( p_name, "float", len ); break;
        case g_string_id:    strncpy( p_name, "string", len ); break;
        case g_character_id: strncpy( p_name, "char", len ); break;
        case g_array_id:
        {
            pointer ptr_array_item_type = array_type_get_type( ptr_type );
            char name[ 256 ];
            _get_type_name( ptr_array_item_type, name, 256 );
            sprintf( p_name, "%s_array", name );
            break;
        }
        
        case g_ref_id:
        {
            pointer ptr_ref_type = ref_type_get_type( ptr_type );
            char name[ 256 ];
            _get_type_name( ptr_ref_type, name, 256 );
            sprintf( p_name, "%s_ref", name );
            break;
        }  
        case g_cls_id:
        {
            cls_type_name( ptr_type, p_name, len );
            break;
        }
        default:
        {
            rAssert( false );
        }      
    }
}

bool _generate_cls_types( pointer ptr_project, stream_info * p_si )
{
    stream_printf( p_si, "\n//\n// class types\n//\n" );
    
    unsigned int num_types = project_num_cls_types( ptr_project );
           
    for( unsigned int t = 0; t < num_types; t ++ )
    {
        pointer ptr_cls_type = project_cls_type_at( ptr_project, t );    
        char cls_name[ 256 ];
        cls_type_name( ptr_cls_type, cls_name, 256 );        
        stream_printf(
            p_si,
            "struct _%s; typedef pointer_templ<_%s> %s;\n",
            cls_name,
            cls_name,
            cls_name );
  
    }
    
    return true;
}

bool _generate_ref_types(
    pointer ptr_project,
    stream_info * p_si )
{
    stream_printf( p_si, "\n//\n// ref types\n//\n" );
        
    unsigned int num_ref_types = project_num_ref_types( ptr_project );
    
    for( unsigned int r = 0; r < num_ref_types; r ++ )
    {
        pointer ptr_ref_type = project_ref_type_at( ptr_project, r );
        
        char type_name[256];
        
        _get_type_name( ptr_ref_type, type_name, 256 );
        
        stream_printf( p_si, "struct _%s; typedef pointer_templ<_%s> %s;\n", type_name, type_name, type_name );
    }
    
    return true;
}

bool _generate_array_types(
    pointer ptr_project,
    stream_info * p_si )
{
    stream_printf( p_si, "\n//\n// array types\n//\n" );
    
    unsigned int num_array_types = project_num_array_types( ptr_project );
    
    for( unsigned int r = 0; r < num_array_types; r ++ )
    {
        pointer ptr_array_type = project_array_type_at( ptr_project, r );
        
        char type_name[256];
        
        _get_type_name( ptr_array_type, type_name, 256 );
        
        stream_printf( p_si, "struct _%s; typedef pointer_templ<_%s> %s;\n", type_name, type_name, type_name );
        
    }
    
    return true;
}

bool _generate_cls_declarations(
    pointer ptr_project,
    stream_info * p_si )
{

    stream_printf( p_si, "\n//\n// class attribute accessor functions\n//\n" );

    unsigned int num_types = project_num_cls_types( ptr_project );
    
    for( unsigned int t = 0; t < num_types; t ++ )
    {
        pointer ptr_cls_type = project_cls_type_at( ptr_project, t );

        char name[ 256 ];
        cls_type_name( ptr_cls_type, name, 256 );
        
        stream_printf( p_si, "\n// cls type %s\n\n", name );
       
        stream_printf( p_si, "inline %s %s_new( block * p_memory );\n",     name,   name ); // create
        stream_printf( p_si, "inline %s %s_construct( const %s & ptr );\n", "void", name, name ); // construct
        stream_printf( p_si, "inline %s %s_destroy( const %s & ptr );\n",   "void", name, name );       
        stream_printf( p_si, "inline %s %s_delete( const %s & ptr );\n",    "void", name, name ); // delete
        
        unsigned int num_attributes = cls_type_num_attributes( ptr_cls_type );
        
        unsigned int pos = 0;
        
        for( unsigned int a = 0; a < num_attributes; a ++ )
        {
            char attribute_name[ 256 ];
            char attribute_type_name[ 256 ];
            
            pointer ptr_attribute_type = cls_type_attribute_type( ptr_cls_type, a );
            cls_type_attribute_name( ptr_cls_type, a, attribute_name, 256 );

            _get_type_name( ptr_attribute_type, attribute_type_name, 256 );
           
            unsigned char id = type_id( ptr_attribute_type );
            
            switch( id )
            {
                case g_uint32_id:
                case g_bool_id:
                case g_float_id:
                case g_character_id:
                {
                    stream_printf( p_si, "inline %s %s_%s( const %s & );\n", attribute_type_name, name, attribute_name, name );
                    stream_printf( p_si, "inline %s %s_%s( const %s &, %s );\n", "void", name, attribute_name, name, attribute_type_name );
                                
                    break;                  
                }                                        
                case g_string_id: 
                {
                    stream_printf( p_si, "inline %s %s_%s( const %s & , char * p_string, unsigned int len);\n", "void", name, attribute_name, name );
                    stream_printf( p_si, "inline %s %s_%s( const %s &, const char * );\n", "void", name, attribute_name, name );
                    break;
                }
                case g_ref_id:
                {
                    _get_type_name( ref_type_get_type( ptr_attribute_type ), attribute_type_name, 256 );
                    stream_printf( p_si, "inline const %s %s_%s( const %s & );\n", attribute_type_name, name, attribute_name, name );
                    stream_printf( p_si, "inline %s %s_%s( const %s &, const %s & );\n", "void", name, attribute_name, name, attribute_type_name );
                    break;                
                }
               
                case g_array_id:
                case g_cls_id:                
                {
                    stream_printf( p_si, "inline const %s %s_%s( const %s & );\n", attribute_type_name, name, attribute_name, name );
                    stream_printf( p_si, "inline %s %s_%s( const %s &, const %s );\n", "void", name, attribute_name, name, attribute_type_name );
                    break;    
                }
                default:
                {
                    rAssert( false );
                }
            }
              
            pos += instance_size( ptr_attribute_type );
           
        }
    }
    
    return true;}


bool _generate_ref_declarations(
    pointer ptr_project,
    stream_info * p_si )
{
    stream_printf( p_si, "\n//\n// type safe dereferencing functions\n//\n" );
    
    unsigned int num_ref_types = project_num_ref_types( ptr_project );
    
    for( unsigned int r = 0; r < num_ref_types; r ++ )
    {
        pointer ptr_ref_type = project_ref_type_at( ptr_project, r );
        pointer ptr_ref_type_type = ref_type_get_type( ptr_ref_type );
        
        char ref_type_name[256];
        char type_name[256];
        
        _get_type_name( ptr_ref_type, ref_type_name, 256 );
        _get_type_name( ptr_ref_type_type, type_name, 256 );
        
        unsigned char id = type_id( ptr_ref_type_type );
        
        stream_printf( p_si, "inline %s %s_construct( const %s & ptr );\n", "void", ref_type_name, ref_type_name );
        stream_printf( p_si, "inline %s %s_destroy( const %s & ptr );\n", "void", ref_type_name, ref_type_name );
                
        switch( id )
        {
            case g_uint32_id:
            case g_bool_id:  
            case g_float_id:      
            case g_character_id:
            {
                stream_printf( p_si, "inline %s deref( const %s & ptr );\n", type_name, ref_type_name );
                break;       
            }
            case g_string_id:
            {
                stream_printf( p_si, "inline %s deref( const %s & ptr, char * p_string, unsigned int len );\n", "void", ref_type_name );
                break;
            }
            case g_array_id:    
            case g_ref_id:      
            case g_cls_id:      
            {
                stream_printf( p_si, "inline %s deref( const %s & ptr );\n", type_name, ref_type_name );
                break;
            }            
            default:
                rAssert( false );   
        }
    }
    
    return true;
    
}

bool _generate_array_declarations(
    pointer ptr_project,
    stream_info * p_si )
{
    stream_printf( p_si, "\n//\n// array manipulation functions\n//\n" );

    unsigned int num_array_types = project_num_array_types( ptr_project );
    
    for( unsigned int a = 0; a < num_array_types; a ++ )
    {
        pointer ptr_array_type = project_array_type_at( ptr_project, a );
        pointer ptr_array_type_type = array_type_get_type( ptr_array_type );
        
        char array_type_name[256];
        char type_name[256];
        
        _get_type_name( ptr_array_type, array_type_name, 256 );
        _get_type_name( ptr_array_type_type, type_name, 256 );
        
        unsigned char id = type_id( ptr_array_type_type );

        stream_printf( p_si, "inline %s %s_new( block * );\n", array_type_name, array_type_name );
        stream_printf( p_si, "inline %s %s_delete( const %s & );\n", array_type_name, array_type_name, array_type_name );
        stream_printf( p_si, "inline %s %s_construct( const %s & );\n", "void", array_type_name, array_type_name );
        stream_printf( p_si, "inline %s %s_destroy( const %s & );\n", "void", array_type_name, array_type_name );
        stream_printf( p_si, "inline %s %s_num_items( const %s &);\n", "unsigned int", array_type_name, array_type_name );
        stream_printf( p_si, "inline void %s_remove_item( const %s &, unsigned int item);\n", array_type_name, array_type_name );
                
        switch( id )
        {
            case g_uint32_id:
            case g_bool_id:  
            case g_float_id:      
            case g_character_id:
            {
                stream_printf( p_si, "inline const %s %s_item_at( const %s & ptr, unsigned int index );\n", type_name, array_type_name, array_type_name );
                stream_printf( p_si, "inline %s %s_add_item( const %s &, %s item);\n", "void", array_type_name, array_type_name, type_name );
                break;       
            }
            case g_string_id:
            {
                stream_printf( p_si, "inline %s %s_item_at( const %s & ptr, unsigned int index, char * p_string, unsigned int len );\n", "void", array_type_name, array_type_name );
                stream_printf( p_si, "inline void string_array_item_at( const string_array & ptr, unsigned int index, const char * p_new_val );\n" );
                stream_printf( p_si, "inline %s %s_add_item( const %s &, const char * p_string );\n", "void", array_type_name, array_type_name );
                break;
            }
            case g_ref_id:
            {
                _get_type_name( ref_type_get_type( ptr_array_type_type ), type_name, 256 );
                
                stream_printf( p_si, "inline const %s %s_item_at( const %s & ptr, unsigned int index );\n", type_name, array_type_name, array_type_name );
                stream_printf( p_si, "inline %s %s_add_item( const %s &, const %s & ptr_item);\n", "void", array_type_name, array_type_name, type_name );
                break;
            }
            case g_array_id: 
            case g_cls_id:      
            {
                stream_printf( p_si, "inline const %s %s_item_at( const %s & ptr, unsigned int index );\n", type_name, array_type_name, array_type_name );
                stream_printf( p_si, "inline const %s %s_add_item( const %s &);\n", type_name, array_type_name, array_type_name );
                break;
            }            
            default:
                rAssert( false );   
        }
        
        stream_printf( p_si, "\n" );
    }
    
    return true;
}

bool _generate_cls_definitions(
    pointer ptr_project,
    stream_info * p_si )
{
    stream_printf( p_si, "\n//\n// class attribute accessor definitions\n//\n" );

    unsigned int num_types = project_num_cls_types( ptr_project );
    
    for( unsigned int t = 0; t < num_types; t ++ )
    {
        pointer ptr_cls_type = project_cls_type_at( ptr_project, t );

        char name[ 256 ];
        cls_type_name( ptr_cls_type, name, 256 );
        
        stream_printf( p_si, "\n// cls type %s\n\n", name );

        unsigned int num_attributes = cls_type_num_attributes( ptr_cls_type );
                
        //
        // create function
        //
        
        stream_printf( p_si, "inline %s %s_new( block * p_memory )\n{\n", name, name );
        stream_printf( p_si, "    %s ptr = (%s) block_alloc( p_memory, %d, \"%s\" ) ;\n", name, name, instance_size( ptr_cls_type ), name );
        stream_printf( p_si, "    %s_construct( ptr );\n", name );
        stream_printf( p_si, "    return ptr;\n}\n" );
        
        //
        // construct function
        //
                        
        stream_printf( p_si, "inline %s %s_construct( const %s & ptr )\n{\n", "void", name, name );

        unsigned int pos = 0;
                
        for( unsigned int a = 0; a < num_attributes; a ++ )
        {
            char attribute_name[ 256 ];
            char attribute_type_name[ 256 ];
            
            pointer ptr_attribute_type = cls_type_attribute_type( ptr_cls_type, a );
            cls_type_attribute_name( ptr_cls_type, a, attribute_name, 256 );

            _get_type_name( ptr_attribute_type, attribute_type_name, 256 );
           
            unsigned char id = type_id( ptr_attribute_type );
            
            switch( id )
            {
                case g_uint32_id:
                case g_bool_id:
                case g_float_id:               
                case g_character_id:
                case g_string_id:                 
                {             
                    stream_printf( p_si, "    %s_instance_construct( pointer( ptr.memory( ), ptr.offset( ) + %d ) );\n", attribute_type_name, pos );
                    break;                  
                }                 
                case g_array_id:
                case g_ref_id:      
                case g_cls_id:                
                {                  
                    stream_printf( p_si, "    %s_construct( pointer_templ< _%s>( ptr.memory( ), ptr.offset( ) + %d ) );\n", attribute_type_name, attribute_type_name, pos );
                    break;    
                }  
                default:
                {
                    rAssert( false );
                }
            }
            
            pos += instance_size( ptr_attribute_type );                    
            
        }
        
        stream_printf( p_si, "}\n" );
        
        //
        // destroy function
        //
                          
        stream_printf( p_si, "inline %s %s_destroy( const %s & ptr )\n{\n", "void", name, name );       

        pos = 0;
                
        for( unsigned int a = 0; a < num_attributes; a ++ )
        {
            char attribute_name[ 256 ];
            char attribute_type_name[ 256 ];
            
            pointer ptr_attribute_type = cls_type_attribute_type( ptr_cls_type, a );
            cls_type_attribute_name( ptr_cls_type, a, attribute_name, 256 );

            _get_type_name( ptr_attribute_type, attribute_type_name, 256 );
           
            unsigned char id = type_id( ptr_attribute_type );
            
            switch( id )
            {
                case g_uint32_id:
                case g_bool_id:
                case g_float_id:               
                case g_character_id:
                case g_string_id:                 
                {             
                    stream_printf( p_si, "    %s_instance_destroy( pointer( ptr.memory( ), ptr.offset( ) + %d ) );\n", attribute_type_name, pos );
                    break;                  
                }                 
                case g_array_id:
                case g_ref_id:      
                case g_cls_id:                
                {                  
                    stream_printf( p_si, "    %s_destroy( pointer_templ< _%s>( ptr.memory( ), ptr.offset( ) + %d ) );\n", attribute_type_name, attribute_type_name, pos );
                    break;    
                }  
                default:
                {
                    rAssert( false );
                }
            }
            
            pos += instance_size( ptr_attribute_type );                    
            
        }
        
        stream_printf( p_si, "}\n" );

        //
        // delete function
        // 
        
        stream_printf( p_si, "inline void %s_delete( const %s & ptr )\n{\n", name, name );
        stream_printf( p_si, "    %s_destroy( ptr );\n    block_free( ptr, %d );\n}\n", name, instance_size( ptr_cls_type ) );
        
        // accessors
                
        pos = 0;
        
        for( unsigned int a = 0; a < num_attributes; a ++ )
        {
            char attribute_name[ 256 ];
            char attribute_type_name[ 256 ];
            
            pointer ptr_attribute_type = cls_type_attribute_type( ptr_cls_type, a );
            cls_type_attribute_name( ptr_cls_type, a, attribute_name, 256 );

            _get_type_name( ptr_attribute_type, attribute_type_name, 256 );
           
            unsigned char id = type_id( ptr_attribute_type );
            
            switch( id )
            {
                case g_uint32_id:
                case g_bool_id:
                case g_float_id:               
                case g_character_id:
                {
                    stream_printf( p_si, "inline %s %s_%s( const %s & ptr)\n{\n", attribute_type_name, name, attribute_name, name );
                    stream_printf( p_si, "    return %s_instance_read( pointer( ptr.memory( ), ptr.offset( ) + %d ) );\n}\n", attribute_type_name, pos );
                    stream_printf( p_si, "inline void %s_%s( const %s & ptr, %s value )\n{\n", name, attribute_name, name, attribute_type_name );
                    stream_printf( p_si, "    %s_instance_write( pointer( ptr.memory( ), ptr.offset( ) + %d ), value );\n", attribute_type_name, pos );                                               
                    break;                  
                }                 
                case g_string_id: 
                {
                    stream_printf( p_si, "inline void %s_%s( const %s & ptr, char * p_string, unsigned int len)\n{\n", name, attribute_name, name );
                    stream_printf( p_si, "    string_instance_read( pointer( ptr.memory( ), ptr.offset( ) + %d ), p_string, len );\n}\n", pos );
                    stream_printf( p_si, "inline void %s_%s( const %s & ptr, const char * p_string )\n{\n", name, attribute_name, name );
                    stream_printf( p_si, "    string_instance_write( pointer( ptr.memory( ), ptr.offset( ) + %d ), p_string );\n", pos );                                               
                    break;
                }
                case g_ref_id:
                {
                    _get_type_name( ref_type_get_type( ptr_attribute_type ), attribute_type_name, 256 );
                    stream_printf( p_si, "inline const %s %s_%s( const %s & ptr)\n{\n", attribute_type_name, name, attribute_name, name );
                    stream_printf( p_si, "    return (%s) ref_instance_read( pointer( ptr.memory( ), ptr.offset( ) + %d ) );\n}\n", attribute_type_name, pos );
                    stream_printf( p_si, "inline void %s_%s( const %s & ptr, const %s & ptr_item )\n{\n", name, attribute_name, name, attribute_type_name );
                    stream_printf( p_si, "    ref_instance_write( pointer( ptr.memory( ), ptr.offset( ) + %d ), ptr_item );\n", pos );                                               
                    break;                
                }                  
                case g_array_id:
                case g_cls_id:                
                {
                    stream_printf( p_si, "inline const %s %s_%s( const %s & ptr)\n{\n", attribute_type_name, name, attribute_name, name );
                    stream_printf( p_si, "    return pointer_templ< _%s>( ptr.memory( ), ptr.offset( ) + %d );\n", attribute_type_name, pos );
                    break;    
                }  
                default:
                {
                    rAssert( false );
                }
            }
                    
            stream_printf( p_si, "}\n" );
             
            pos += instance_size( ptr_attribute_type );
           
        }
    }
    
    return true;
}

bool _generate_ref_definitions(
    pointer ptr_project,
    stream_info * p_si )
{
    stream_printf( p_si, "\n//\n// type safe dereferencing functions\n//\n" );
   
    unsigned int num_ref_types = project_num_ref_types( ptr_project );
    
    for( unsigned int r = 0; r < num_ref_types; r ++ )
    {
        pointer ptr_ref_type = project_ref_type_at( ptr_project, r );
        pointer ptr_ref_type_type = ref_type_get_type( ptr_ref_type );
        
        char ref_type_name[256];
        char type_name[256];
        
        _get_type_name( ptr_ref_type, ref_type_name, 256 );
        _get_type_name( ptr_ref_type_type, type_name, 256 );

        _header( p_si, ref_type_name );
        
        stream_printf( p_si, "\n#ifndef %s_DEFINED\n", ref_type_name );
        stream_printf( p_si,"#define %s_DEFINED\n\n", ref_type_name );
        
        stream_printf( p_si, "inline %s %s_construct( const %s & ptr )\n{\n", "void", ref_type_name, ref_type_name );
        stream_printf( p_si, "    ref_instance_construct( ptr );\n}\n" );
        stream_printf( p_si, "inline %s %s_destroy( const %s & ptr )\n{\n", "void", ref_type_name, ref_type_name );
        stream_printf( p_si, "    ref_instance_destroy( ptr );\n}\n" );        
                         
        unsigned char id = type_id( ptr_ref_type_type );
        
        switch( id )
        {
            case g_uint32_id:
            case g_bool_id:  
            case g_float_id:      
            case g_character_id:
            {
                stream_printf( p_si, "inline %s deref( const %s & ptr )\n{\n", type_name, ref_type_name );
                stream_printf( p_si, "    return %s_instance_read( block_read_reference( ptr ) ) );\n}\n", type_name );          
                break;       
            }
            case g_string_id:
            {
                stream_printf( p_si, "inline %s deref( const %s & ptr, char * p_string, unsigned int len )\n{\n", "void", ref_type_name );
                stream_printf( p_si, "    return string_instance_read( block_read_reference( ptr ), p_string, len );\n}\n"  );          
                break;
            }
            case g_array_id:    
            case g_ref_id:      
            case g_cls_id:      
            {
                stream_printf( p_si, "inline %s deref( const %s & ptr )\n{\n", type_name, ref_type_name );
                stream_printf( p_si, "    return pointer_templ< _%s>( block_read_reference( ptr ) );\n}\n", type_name );          
                break;
            }            
            default:
                rAssert( false );   
        }
        
        stream_printf( p_si,"#endif // %s_DEFINDED\n", ref_type_name );
       
    }
    
    return true;
}

bool _generate_array_definitions(
    pointer ptr_project,
    stream_info * p_si )
{
    stream_printf( p_si, "\n//\n// type safe array functions\n//\n" );
    
    unsigned int num_array_types = project_num_array_types( ptr_project );
    
    for( unsigned int a = 0; a < num_array_types; a ++ )
    {
        pointer ptr_array_type = project_array_type_at( ptr_project, a );
        pointer ptr_array_type_type = array_type_get_type( ptr_array_type );
        
        char array_type_name[256];
        char type_name[256];
        
        _get_type_name( ptr_array_type, array_type_name, 256 );
        _get_type_name( ptr_array_type_type, type_name, 256 );
        
        unsigned char id = type_id( ptr_array_type_type );
        
        _header( p_si, array_type_name );
                
        stream_printf( p_si, "\n#ifndef %s_DEFINED\n", array_type_name );
        stream_printf( p_si,"#define %s_DEFINED\n\n", array_type_name );
                
        stream_printf( p_si, "inline unsigned int %s_num_items( const %s & ptr )\n{\n    return array_instance_num_items( ptr );\n}\n", array_type_name, array_type_name );
        stream_printf( p_si, "inline void %s_construct( const %s & ptr )\n{\n", array_type_name, array_type_name );
        stream_printf( p_si, "    array_instance_construct( ptr, %d );\n}\n", instance_size( ptr_array_type_type ) );
                        
        switch( id )
        {
            case g_uint32_id:
            case g_bool_id:  
            case g_float_id:      
            case g_character_id:
            {
                stream_printf( p_si, "inline const %s %s_item_at( const %s & ptr, unsigned int index )\n{\n", type_name, array_type_name, array_type_name );
                stream_printf( p_si, "    return %s_instance_read( array_instance_item_at( ptr, %d, index ) );\n}\n", type_name, instance_size( ptr_array_type_type ) );          
                stream_printf( p_si, "inline void %s_add_item( const %s & ptr, %s item )\n{\n", array_type_name, array_type_name, type_name );
                stream_printf( p_si, "    %s_instance_write( array_instance_add_item( ptr, %d ), item );\n}\n", type_name, instance_size( ptr_array_type_type ) );
                stream_printf( p_si, "inline void %s_destroy( const %s & ptr )\n{\n", array_type_name, array_type_name );
                stream_printf( p_si, "    array_instance_destroy( ptr, %d );\n}\n", instance_size( ptr_array_type_type ) );        
                break;       
            }
            case g_string_id:
            {
                stream_printf( p_si, "inline void string_array_item_at( const %s & ptr, unsigned int index, char * p_string, unsigned int len )\n{\n", array_type_name );
                stream_printf( p_si, "    return string_instance_read( array_instance_item_at( ptr, %d, index ), p_string, len );\n}\n", instance_size( ptr_array_type_type ) );          

                stream_printf( p_si, "inline void string_array_item_at( const string_array & ptr, unsigned int index, const char * p_new_val )\n{\n" );
                stream_printf( p_si, "    pointer ptr_item = array_instance_item_at( ptr, %d, index );\n", instance_size( ptr_array_type_type ) );
                stream_printf( p_si, "    string_instance_destroy( ptr_item );\n" );
                stream_printf( p_si, "    string_instance_construct( ptr_item );\n" );
                stream_printf( p_si, "    string_instance_write( ptr_item, p_new_val );\n}\n" );
                                
                stream_printf( p_si, "inline void string_array_add_item( const string_array & ptr, const char * p_string )\n{\n" );
                stream_printf( p_si, "    pointer ptr_new_item = array_instance_add_item( ptr, %d );\n", instance_size( ptr_array_type_type ) );
                stream_printf( p_si, "    string_instance_construct( ptr_new_item );\n" );
                stream_printf( p_si, "    string_instance_write( ptr_new_item, p_string );\n}\n"  );                            
                
                stream_printf( p_si, "inline void string_array_remove_item( const string_array & ptr, unsigned int index )\n{\n" );
                stream_printf( p_si, "    pointer ptr_item = array_instance_item_at( ptr, %d, index );\n", instance_size( ptr_array_type_type ) );
                stream_printf( p_si, "    string_instance_destroy( ptr_item );\n" );
                stream_printf( p_si, "    ptr_item = pointer_null;\n" );
                stream_printf( p_si, "    array_instance_remove_item( ptr, %d, index );\n}\n", instance_size( ptr_array_type_type ) );
                
                stream_printf( p_si, "inline void %s_destroy( const %s & ptr )\n{\n", array_type_name, array_type_name );
                stream_printf( p_si, "    unsigned int items = array_instance_num_items( ptr );\n" );
                stream_printf( p_si, "    for(unsigned int i = 0; i < items; i ++ )\n    {\n" );
                stream_printf( p_si, "        pointer ptr_item = array_instance_item_at( ptr, %d, i );\n", instance_size( ptr_array_type_type ) );
                stream_printf( p_si, "        string_instance_destroy( ptr_item );\n    }\n" );
                stream_printf( p_si, "    array_instance_destroy( ptr, %d );\n}\n", instance_size( ptr_array_type_type ) );        
                break;
            }
            case g_ref_id:
            {
                char ref_type_name[ 256 ];
                _get_type_name( ref_type_get_type( ptr_array_type_type ), ref_type_name, 256 );
                
                stream_printf( p_si, "inline const %s %s_item_at( const %s & ptr, unsigned int index )\n{\n", ref_type_name, array_type_name, array_type_name );
                stream_printf( p_si, "    pointer ptr_item = array_instance_item_at( ptr, %d, index );\n", instance_size( ptr_array_type_type ) );          
                stream_printf( p_si, "    return (%s) block_read_reference( ptr_item );\n}\n", ref_type_name );
                                
                stream_printf( p_si, "inline void %s_add_item( const %s & ptr, const %s & ptr_item )\n{\n", array_type_name, array_type_name, ref_type_name );
                stream_printf( p_si, "     %s ptr_new_item = (%s) array_instance_add_item( ptr, %d );\n", type_name, type_name, instance_size( ptr_array_type_type ) );
                stream_printf( p_si, "     block_write_reference( ptr_new_item, ptr_item );\n}\n" );
                
                stream_printf( p_si, "inline void %s_destroy( const %s & ptr )\n{\n", array_type_name, array_type_name );
                stream_printf( p_si, "    unsigned int items = array_instance_num_items( ptr );\n" );
                stream_printf( p_si, "    for(unsigned int i = 0; i < items; i ++ )\n    {\n" );
                stream_printf( p_si, "        %s ptr_item = (%s) array_instance_item_at( ptr, %d, i );\n", type_name, type_name, instance_size( ptr_array_type_type ) );
                stream_printf( p_si, "        %s_destroy( ptr_item );\n    }\n", type_name );
                stream_printf( p_si, "    array_instance_destroy( ptr, %d );\n}\n", instance_size( ptr_array_type_type ) );        
                break;              
            }      
            case g_array_id:   
            case g_cls_id:      
            {
                stream_printf( p_si, "inline const %s %s_item_at( const %s & ptr, unsigned int index )\n{\n", type_name, array_type_name, array_type_name );
                stream_printf( p_si, "    return pointer_templ< _%s>( array_instance_item_at( ptr, %d, index ) );\n}\n", type_name, instance_size( ptr_array_type_type ) );          
                
                stream_printf( p_si, "inline const %s %s_add_item( const %s & ptr )\n{\n", type_name, array_type_name, array_type_name );
                stream_printf( p_si, "     %s ptr_new_item = (%s) array_instance_add_item( ptr, %d );\n", type_name, type_name, instance_size( ptr_array_type_type ) );
                stream_printf( p_si, "     %s_construct( ptr_new_item );\n", type_name );
                stream_printf( p_si, "    return ptr_new_item;\n}\n" );
                
                stream_printf( p_si, "inline void %s_destroy( const %s & ptr )\n{\n", array_type_name, array_type_name );
                stream_printf( p_si, "    unsigned int items = array_instance_num_items( ptr );\n" );
                stream_printf( p_si, "    for(unsigned int i = 0; i < items; i ++ )\n    {\n" );
                stream_printf( p_si, "        %s ptr_item = (%s) array_instance_item_at( ptr, %d, i );\n", type_name, type_name, instance_size( ptr_array_type_type ) );
                stream_printf( p_si, "        %s_destroy( ptr_item );\n    }\n", type_name );
                stream_printf( p_si, "    array_instance_destroy( ptr, %d );\n}\n", instance_size( ptr_array_type_type ) );        
                
                stream_printf( p_si, "inline void %s_remove_item( const %s & ptr, unsigned int index)\n{\n", array_type_name, array_type_name );
                stream_printf( p_si, "    %s ptr_item = (%s) array_instance_item_at( ptr, %d, index );\n", type_name, type_name, instance_size( ptr_array_type_type ) );
                stream_printf( p_si, "    %s_destroy( ptr_item );\n", type_name );
                stream_printf( p_si, "    ptr_item = pointer_null;\n" );
                stream_printf( p_si, "    array_instance_remove_item( ptr, %d, index );\n}\n", instance_size( ptr_array_type_type ) );
                break;
            }            
            default:
                rAssert( false );   
        }
        
        stream_printf( p_si,"#endif // %s_DEFINED\n", array_type_name );        
    }
    
    return true;
}

bool project_generate_code( const pointer & ptr_project, stream_info * p_si )
{
    stream_printf( p_si, "\n// Generated by Schema+ V1.0 (C) Radical Games\n\n" );
    
    srand( (unsigned int) radTimeGetMilliseconds() );
    
    unsigned int inc = rand();
    
    stream_printf( p_si, "#ifndef __%d\n#define __%d\n", inc, inc );
    stream_printf( p_si, "\n#include <ods/ods.hpp>\n" );
    stream_printf( p_si, "\nnamespace ods {\n" );
        
    bool ok = (
        ( _generate_cls_types( ptr_project, p_si ) )            &&
        ( _generate_ref_types( ptr_project, p_si ) )            &&    
        ( _generate_array_types( ptr_project, p_si ) )          &&
        ( _generate_cls_declarations( ptr_project, p_si ) )     &&
        ( _generate_ref_declarations( ptr_project, p_si ) )     &&
        ( _generate_array_declarations( ptr_project, p_si ) )   &&
        ( _generate_cls_definitions( ptr_project, p_si ) )      &&
        ( _generate_ref_definitions( ptr_project, p_si ) )      &&
        ( _generate_array_definitions( ptr_project, p_si ) ) );

    stream_printf( p_si, "}\n" );
            
    if ( ok )
    {
        stream_printf( p_si, "#endif\n" );
    }
        
    return ok;        
}

}