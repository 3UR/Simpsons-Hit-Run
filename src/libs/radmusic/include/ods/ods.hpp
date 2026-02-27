//-----------------------------------------------------------------------------
///
//  Copyright (C) 2003 Radical Entertainment Ltd.  All Rights Reserved.
/// 
//-----------------------------------------------------------------------------

#ifndef ODS_HPP
#define ODS_HPP

//-----------------------------------------------------------------------------
// Nested Includes
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <ods/parser.hpp>

//-----------------------------------------------------------------------------
// Forward References
//-----------------------------------------------------------------------------

namespace ods
{
    struct block;
    class  pointer;
    struct stream_info;    
    struct stream_functions;
}
//-----------------------------------------------------------------------------
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//-----------------------------------------------------------------------------

#define MEMORY_DYNAMIC

//-----------------------------------------------------------------------------
// Interface Definitions
//-----------------------------------------------------------------------------

namespace ods
{

//----------------------------------------------------------------------------
// file system plug-in system
//----------------------------------------------------------------------------

bool stream_write( stream_info * p_si, const void * buffer, unsigned int chars );
bool stream_read( stream_info * p_si, void * buffer, unsigned int chars );
bool stream_printf( stream_info * p_si, const char *, ... );

// for writing your own stream types

typedef bool   (stream_write_fx)( void * stream, const void *buffer, unsigned int chars );
typedef bool   (stream_read_fx) ( void * stream, void * buffer, unsigned int chars  );

struct stream_functions
{
    stream_write_fx * p_write;
    stream_read_fx  * p_read;
};

struct stream_info
{
   ods::stream_functions* p_sf;
   void*                  p_stream;
};

// pre-defined stream types

extern stream_functions stdio_stream_functions;
extern stream_functions radfile_stream_functions;
extern stream_functions radload_stream_functions;
extern stream_functions debug_printf_stream_functions;
extern stream_functions memory_stream_functions;

void * stdio_stream_open( const char * p_file_name, bool write );
bool stdio_stream_close( void * p_file_handle );

void * radfile_stream_open( const char * p_file_name, bool write );
bool radfile_stream_close( void * p_file_handle );

void * radload_stream_open( const char * p_file_name, bool write );
bool radload_stream_close( void * p_file_handle);

void * debug_printf_stream_open( const char * p_channel );
bool debug_printf_stream_close( void * p_file_handle );

struct memory_stream
{
    bool write;
    const char * p_memory;
    unsigned int bytes;    
    
    char * p_current;
};

void memory_stream_open ( memory_stream *, char * p_memory, unsigned int bytes, bool write );
void memory_stream_close( memory_stream * );


//----------------------------------------------------------------------------
// memory plug in system
//----------------------------------------------------------------------------

typedef void * (realloc_fx)(void *, size_t old_size, size_t new_size , const char * p_id);
typedef void   (free_fx)( void * p_free );

struct memory_hooks
{
    realloc_fx * p_realloc;
    free_fx    * p_free;
};

//----------------------------------------------------------------------------
// memory system
//----------------------------------------------------------------------------

typedef unsigned int offset_t;

bool          block_save_to_stream_synch( const block *, stream_info * );

void          block_construct( block *, memory_hooks *, bool reverse_endian );
bool          block_construct_from_stream_synch( block *, stream_info *, pointer * p_first_item, memory_hooks *, bool reverse_endian );

void          block_destroy( block * p_m );

bool          block_reverse_endian( block * );
offset_t      block_size( block * );

void          block_memory_destroy( block * );
pointer       block_alloc( block *, unsigned int bits, const char * );
void          block_free( const pointer &, unsigned int bits );
void          block_insert( const pointer &, const void *, offset_t );
void          block_insert( const pointer &, offset_t bit_offset, offset_t bits );
void          block_insert( bool reverse, const pointer &, offset_t bit_offset, const void *, offset_t );
void          block_delete( pointer p, unsigned int bits );

void          block_write( bool reverse_endian, const pointer &, const void *, offset_t );
void          block_write( bool reverse_endian, pointer, offset_t bit_offset, const void *, offset_t );
void          block_read(  bool reverse_endian, const pointer &, offset_t bit_offset, void *, offset_t );
void          block_read(  bool reverse_endian, const pointer &, void *, offset_t );

// built-in types

void          block_write_reference( const pointer & , offset_t bit_offset, offset_t );
void          block_write_reference( const pointer &, const pointer & );
pointer       block_read_reference( const pointer & , offset_t bit_offset );
pointer       block_read_reference( const pointer & );
void          block_revoke_reference( block *, offset_t bit_offset );
void          block_fill( const pointer &, unsigned char, unsigned int bits );

pointer       array_construct( block *, offset_t size );
void          array_destroy( pointer );
pointer       array_add_tail( const pointer & );
void          array_remove_at( const pointer &, unsigned int index );
unsigned int  array_size( pointer );
pointer       array_get_pointer_at( pointer, unsigned int );

pointer       reference_array_construct( block * );
void          reference_array_destroy( pointer );
void          reference_array_add_tail( pointer, pointer );
pointer       reference_array_item_at( pointer, unsigned int index );
void          reference_array_remove_at( pointer, unsigned int );
unsigned int  reference_array_num_items( pointer );

pointer       string_construct( block *, const char * p_string );
void          string_destroy(  pointer );
void          string_get(   pointer, char * p_string, unsigned int buffer_size );
unsigned int  string_len(   pointer );

inline void         reverse_endian( bool, void *, unsigned int );
inline unsigned int reverse_endian( bool, unsigned int );
 
//----------------------------------------------------------------------------
// pointer
//----------------------------------------------------------------------------


struct null_ptr_type { int dummy; };
extern const null_ptr_type pointer_null;

class pointer
{
    public:
    
        pointer( );
        pointer( const null_ptr_type & n );       
        pointer( const pointer & ); 
        pointer( const block *, unsigned int offset );
        ~pointer( );

        const offset_t * p_offset( ) const;
        offset_t         offset( ) const;
        void                  offset( offset_t ) const;
        block *             memory( ) const;

#ifdef MEMORY_DYNAMIC    
        mutable const pointer * prev;
        mutable const pointer * next;
#endif
        
        pointer & operator=( const pointer & );        
        pointer & operator=( const null_ptr_type & );
                      
        bool       operator==( const pointer & ) const;
        bool       operator!=( const pointer & ) const;
        
        static void construct( pointer * p_pointer );
        static void destroy( pointer * p_pointer );        
        
    private:

#ifdef MEMORY_DYNAMIC    
        static void _add_to_list( const pointer *  );
        static void _remove_from_list( const pointer * );
#endif
        
        mutable block * p_mem;
        mutable offset_t o;
};

inline bool is_null( const pointer & pointer );
inline bool not_null( const pointer & pointer );

void pointer_construct( pointer * p_pointer );
void pointer_destroy( pointer * p_pointer );

struct block
{
    char * p_mem;
    unsigned int size;

    offset_t * p_references;
    offset_t num_references;
        
    const pointer * p_pointer_head;
    
    memory_hooks mem_hooks;
    
    bool reverse_endian;
};

//----------------------------------------------------------------------------
// inlines
//----------------------------------------------------------------------------

inline bool is_null( const pointer & pointer )
{
    return pointer.offset( ) == 0xFFFFFFFF;
}

inline bool not_null( const pointer & pointer )
{
    return pointer.offset( ) != 0xFFFFFFFF;
}

inline void reverse_endian( bool reverse, void * p1, unsigned int bytes)
{    
    if ( reverse )
    {
        char * p = (char*) p1;
        
        unsigned int i = 0;
        unsigned int j = bytes -1;
        
        while ( i <= j )
        {
            char c = p[ i ];
            p[ i ] = p[ j ];
            p[ j ] = c;
            
            i++;
            j--;
        }
    }
}

inline unsigned int reverse_endian( bool reverse, unsigned int i )
{
    reverse_endian( reverse, & i, sizeof( i ) );
    
    return i;
}

typedef unsigned int uint32;

const unsigned char g_uint32_id       = 0;
const unsigned char g_bool_id        = 1;
const unsigned char g_float_id       = 2;
const unsigned char g_string_id      = 3;
const unsigned char g_character_id   = 4;
const unsigned char g_array_id       = 5;
const unsigned char g_ref_id         = 6;
const unsigned char g_cls_id         = 7;
const unsigned char g_num_types      = 8;
    
//----------------------------------------------------------------------------
// types
//----------------------------------------------------------------------------

unsigned char type_id( const ods::pointer & ptr_type );

void uint32_type_format_name( const ods::pointer & ptr_type, char * p_string, unsigned int size );
void bool_type_format_name( const ods::pointer & ptr_type, char * p_string, unsigned int size );
void float_type_format_name( const ods::pointer & ptr_type, char * p_string, unsigned int size ); 
void string_type_format_name( const ods::pointer & ptr_type, char * p_string, unsigned int size );
void char_type_format_name( const ods::pointer & ptr_type, char * p_string, unsigned int size );
void array_type_format_name( const ods::pointer & ptr_type, char * p_string, unsigned int size );
void ref_type_format_name( const ods::pointer & ptr_type, char * p_string, unsigned int size );
void cls_type_format_name( const ods::pointer & ptr_type, char * p_string, unsigned int size );

void type_format_name( const ods::pointer & ptr_type, char * p_string, unsigned int size );

// class



ods::pointer        cls_type_construct( ods::block * p_m, const char * p_string );
void                cls_type_destroy( const ods::pointer & ptr_cls_type );
void                cls_type_add_attribute( const ods::pointer & ptr_cls_type, const char * p_name, const ods::pointer & ptr_type );
unsigned int        cls_type_num_attributes( const ods::pointer & ptr_cls_type );
void                cls_type_attribute_name( const ods::pointer & ptr_cls_type, unsigned int attrib, char *, unsigned int );
ods::pointer        cls_type_attribute_type( const ods::pointer & ptr_cls_type, unsigned int attrib );
ods::offset_t       cls_type_attribute_offset( const ods::pointer & ptr_cls_type, unsigned int attrib );
void                cls_type_name( const ods::pointer & ptr_cls_type, char * p_string, unsigned int buf_size );
unsigned int        cls_type_find_attribute( const ods::pointer & ptr_cls_type, const char * p_name );
unsigned int        cls_size( const ods::pointer & ptr_cls_type );

// reference

ods::pointer        ref_type_construct( ods::block *, const ods::pointer & ptr_type );
ods::pointer        ref_type_get_type( const ods::pointer & ptr_ref );
void                ref_type_destroy( const ods::pointer & ptr_ref_type );

// array

ods::pointer        array_type_construct( ods::block * p_m, const ods::pointer & ptr_type );
void                array_type_destroy( const ods::pointer & ptr_array_type ); 
ods::pointer        array_type_get_type( const ods::pointer & ptr_ref );

//----------------------------------------------------------------------------
// instances
//----------------------------------------------------------------------------

void                instance_construct( const ods::pointer & ptr_type, const ods::pointer & ptr_inplace );
unsigned int        instance_size( const ods::pointer & ptr_type );
void                instance_destroy( const ods::pointer & ptr_instance_type, const ods::pointer & ptr_instance );
bool                instance_write_text( const ods::pointer & ptr_project, const ods::pointer & ptr_type, const ods::pointer & ptr_instance, stream_info *, unsigned int indent );
bool                instance_read_text( const ods::pointer & ptr_type, const ods::pointer & ptr_instance, Parser & );
bool                instance_compare_text( const ods::pointer & ptr_type, const ods::pointer & ptr_instance, Parser &, bool * p_same );
ods::pointer        instance_create( const ods::pointer & ptr_type, ods::block * p_m );

// uint32

void                uint32_instance_construct( const ods::pointer & ptr_type, const ods::pointer & ptr_inplace );
void                uint32_instance_destroy( const ods::pointer & ptr_type, const ods::pointer & ptr_instance );
unsigned int        uint32_instance_size( const ods::pointer & ptr_type );
bool                uint32_instance_write_text( const ods::pointer & ptr_project, const ods::pointer & ptr_type, const ods::pointer & ptr_instance, stream_info *, unsigned int indent );
bool                uint32_instance_read_text( const ods::pointer & ptr_type, const ods::pointer & ptr_instance, Parser & );
bool                uint32_instance_compare_text( const ods::pointer & ptr_type, const ods::pointer & ptr_instance, Parser &, bool * p_same );

void                uint32_instance_construct( const ods::pointer & ptr_inplace );
void                uint32_instance_destroy( const ods::pointer & ptr_instance );
void                uint32_instance_write( const ods::pointer ptr_uint32, unsigned int v );
unsigned int        uint32_instance_read( const ods::pointer ptr_uint32 );


// float

void                float_instance_construct( const ods::pointer & ptr_type, const ods::pointer & ptr_inplace );
void                float_instance_destroy( const ods::pointer & ptr_type, const ods::pointer & ptr_instance );
unsigned int        float_instance_size( const ods::pointer & ptr_type );
bool                float_instance_write_text( const ods::pointer & ptr_project, const ods::pointer & ptr_type, const ods::pointer & ptr_instance, stream_info *, unsigned int indent );
bool                float_instance_read_text( const ods::pointer & ptr_type, const ods::pointer & ptr_instance, Parser & );
bool                float_instance_compare_text( const ods::pointer & ptr_type, const ods::pointer & ptr_instance, Parser &, bool * p_same );

void                float_instance_construct( const ods::pointer & ptr_inplace );
void                float_instance_destroy( const ods::pointer & ptr_instance );
void                float_instance_write( const ods::pointer & ptr_instance, float value );
float               float_instance_read( const ods::pointer & ptr_instance );

// character

void                char_instance_construct( const ods::pointer & ptr_type, const ods::pointer & ptr_inplace );
void                char_instance_destroy( const ods::pointer & ptr_type, const ods::pointer & ptr_instance );
unsigned int        char_instance_size( const ods::pointer & ptr_type );
bool                char_instance_write_text( const ods::pointer & ptr_project, const ods::pointer & ptr_type, const ods::pointer & ptr_instance, stream_info *, unsigned int indent );
bool                char_instance_read_text( const ods::pointer & ptr_type, const ods::pointer & ptr_instance, Parser & );
bool                char_instance_compare_text( const ods::pointer & ptr_type, const ods::pointer & ptr_instance, Parser &, bool * p_same );

void                char_instance_construct( const ods::pointer & ptr_inplace );
void                char_instance_destroy( const ods::pointer & ptr_instance );
void                char_instance_write( const ods::pointer & ptr_instance, char c );
char                char_instance_read( const ods::pointer & ptr_instance );

// bool

void                bool_instance_construct( const ods::pointer & ptr_type, const ods::pointer & ptr_inplace );
void                bool_instance_destroy( const ods::pointer & ptr_type, const ods::pointer & ptr_instance);
unsigned int        bool_instance_size( const ods::pointer & ptr_type );
bool                bool_instance_write_text( const ods::pointer & ptr_project, const ods::pointer & ptr_type, const ods::pointer & ptr_instance, stream_info *, unsigned int indent  );
bool                bool_instance_read_text( const ods::pointer & ptr_type, const ods::pointer & ptr_instance, Parser & );
bool                bool_instance_compare_text( const ods::pointer & ptr_type, const ods::pointer & ptr_instance, Parser &, bool * p_same );

void                bool_instance_construct( const ods::pointer & ptr_inplace );
void                bool_instance_destroy( const ods::pointer & ptr_instance );
void                bool_instance_write( const ods::pointer & ptr_instance, bool value );
bool                bool_instance_read( const ods::pointer & ptr_instance );

// string

void                string_instance_construct( const ods::pointer & ptr_type, const ods::pointer & ptr_inplace );
void                string_instance_destroy( const ods::pointer & ptr_type, const ods::pointer & ptr_instance );
unsigned int        string_instance_size( const ods::pointer & ptr_type );
bool                string_instance_write_text( const ods::pointer & ptr_project, const ods::pointer & ptr_type, const ods::pointer & ptr_instance, stream_info *, unsigned int indent  );
bool                string_instance_read_text( const ods::pointer & ptr_type, const ods::pointer & ptr_instance, Parser & );
bool                string_instance_compare_text( const ods::pointer & ptr_type, const ods::pointer & ptr_instance, Parser &, bool * p_same );

void                string_instance_construct( const ods::pointer & ptr_inplace );
void                string_instance_destroy( const ods::pointer & ptr_instance );
void                string_instance_write( const ods::pointer & ptr_string, const char * p_string );
void                string_instance_read( const ods::pointer & ptr_string, char * p_string, unsigned int max );

// array

void                array_instance_construct( const ods::pointer & ptr_type, const ods::pointer & ptr_inplace );
void                array_instance_destroy( const ods::pointer & ptr_type, const ods::pointer & ptr_instance);
unsigned int        array_instance_size( const ods::pointer & ptr_type );
bool                array_instance_write_text( const ods::pointer & ptr_project, const ods::pointer & ptr_type, const ods::pointer & ptr_instance, stream_info *, unsigned int indent  );
bool                array_instance_read_text( const ods::pointer & ptr_type, const ods::pointer & ptr_instance, Parser & );
bool                array_instance_compare_text( const ods::pointer & ptr_type, const ods::pointer & ptr_instance, Parser &, bool * p_same );

bool                array_instance_assemble_name ( const ods::pointer & ptr_project, const ods::pointer & ptr_type, const ods::pointer & ptr_object, const ods::pointer & ptr_instance, char * p_string, unsigned int max_chars );

void                array_instance_construct( const ods::pointer & ptr_inplace, unsigned int item_size_in_bits );
void                array_instance_destroy( const ods::pointer & ptr_array, unsigned int item_size_in_bits );
unsigned int        array_instance_num_items( const ods::pointer & ptr_instance );
ods::pointer        array_instance_add_item( const ods::pointer & ptr_array, unsigned int item_size );
ods::pointer        array_instance_item_at( const ods::pointer & ptr_array, unsigned int item_size, unsigned int index );
void                array_instance_remove_item( const ods::pointer & ptr_array, unsigned int item_size, unsigned int index );


// ref
void                ref_instance_construct( const ods::pointer & ptr_type, const ods::pointer & ptr_inplace );
void                ref_instance_destroy( const ods::pointer & ptr_type, const ods::pointer & ptr_instance );
unsigned int        ref_instance_size( const ods::pointer & ptr_type );    
bool                ref_instance_write_text( const ods::pointer & ptr_project, const ods::pointer & ptr_type, const ods::pointer & ptr_instance, stream_info *, unsigned int indent  );
bool                ref_instance_read_text( const ods::pointer & ptr_type, const ods::pointer & ptr_instance, Parser & );
bool                ref_instance_compare_text( const ods::pointer & ptr_type, const ods::pointer & ptr_instance, Parser &, bool * p_same );

void                ref_instance_construct( const ods::pointer & ptr_inplace );
void                ref_instance_destroy( const ods::pointer & ptr_instance ); 
void                ref_instance_write( const ods::pointer & ptr_ref, const ods::pointer & ptr_value );
ods::pointer        ref_instance_read( const ods::pointer & ptr_ref );

// cls

void                cls_instance_construct( const ods::pointer & ptr_type, const ods::pointer & ptr_inplace );
void                cls_instance_destroy( const ods::pointer & ptr_type, const ods::pointer & ptr_instance );
unsigned int        cls_instance_size( const ods::pointer & ptr_type );
bool                cls_instance_write_text( const ods::pointer & ptr_project, const ods::pointer & ptr_type, const ods::pointer & ptr_instance, stream_info *, unsigned int indent  );
bool                cls_instance_read_text( const ods::pointer & ptr_type, const ods::pointer & ptr_instance, Parser & );
bool                cls_instance_compare_text( const ods::pointer & ptr_type, const ods::pointer & ptr_instance, Parser &, bool * same );

ods::pointer        cls_instance_find_attribute( const ods::pointer & ptr_type, const ods::pointer & ptr_cls_instance, const char * p_name, const pointer & ptr_expected_type );

bool                cls_instance_assemble_name ( const ods::pointer & ptr_project, const ods::pointer & ptr_type, const ods::pointer & ptr_object, const ods::pointer & ptr_instance, char * p_string, unsigned int max_chars );

//----------------------------------------------------------------------------
// project management
//----------------------------------------------------------------------------

ods::pointer   project_construct( ods::block *, const char * p_name );
void           project_destroy( ods::pointer & ptr_project );

ods::pointer   project_get_built_in_type( const ods::pointer &, unsigned char );

void           project_name( const ods::pointer & ptr_project, char *, unsigned int );

unsigned int   project_num_cls_types( const ods::pointer & ptr_project );
ods::pointer   project_cls_type_find( const ods::pointer & ptr_project, const char * p_name );
ods::pointer   project_cls_type_at( const ods::pointer & ptr_project, unsigned int i );
void           project_add_cls_type( const ods::pointer & ptr_project, ods::pointer & ptr_cls_type);
ods::pointer   project_cls_type_find_or_alloc( const ods::pointer & ptr_project, const char * p_name );

unsigned int   project_num_ref_types( const ods::pointer & ptr_project );
ods::pointer   project_ref_type_find( const ods::pointer & ptr_project, const ods::pointer & ptr_type );
ods::pointer   project_ref_type_at( const ods::pointer & ptr_project, unsigned int i );
void           project_add_ref_type( const ods::pointer & ptr_project, const ods::pointer & ptr_ref_type );
ods::pointer   project_ref_type_find_or_alloc( const ods::pointer & ptr_project, const ods::pointer & ptr_ref_type );

unsigned int   project_num_array_types( const ods::pointer & ptr_project );
ods::pointer   project_array_type_find( const ods::pointer & ptr_project, const ods::pointer & ptr_type );
ods::pointer   project_array_type_at( const ods::pointer & ptr_project, unsigned int i );
void           project_add_array_type( const ods::pointer & ptr_project, const ods::pointer & ptr_array_type );
ods::pointer   project_array_type_find_or_alloc( const ods::pointer & ptr_project, const ods::pointer & ptr_array_type );

void           project_add_instance( const ods::pointer & ptr_project, const char * p_name, const ods::pointer & ptr_object, const ods::pointer & ptr_type );
unsigned int   project_num_instances( const ods::pointer & ptr_project );
ods::pointer   project_instance_at( const ods::pointer & ptr_project, unsigned int i, char *, unsigned int, ods::pointer * ptr_type );
ods::pointer   project_find_instance( const ods::pointer & ptr_project, const char *, ods::pointer * ptr_type );
void           project_assemble_instance_name( const ods::pointer & ptr_project, const ods::pointer & ptr_instance, char *, unsigned int );

bool           project_generate_code( const ods::pointer & ptr_project, ods::stream_info * p_stream_info );
bool           project_write_objects_text( ods::pointer & ptr_project, ods::stream_info * );
bool           project_read_objects_text( ods::pointer & ptr_project, ods::stream_info * );

//----------------------------------------------------------------------------
//
// schema generation utility interface
//
//----------------------------------------------------------------------------

ods::pointer create_schema( const char *, bool reverse_endian );
void         destroy_schema( void );

void         begin_schema( const pointer & ptr_project );
void         end_schema( void );

void end_schema( void );
void dump_schema( void );
bool save_schema( ods::stream_info * p_si );

bool generate_code( ods::stream_info * p_si  );

void kill_schema( void );

void          begin_type( const char * p_name );
void          end_type( void );

void          attribute(  const char * p_name, ods::pointer ptr_type );

ods::pointer  uint();
ods::pointer  booln();
ods::pointer  string();
ods::pointer  array( ods::pointer & ptr_type );
ods::pointer  rf( ods::pointer & ptr_type );
ods::pointer  cls( const char * p_name );
ods::pointer  flt( void );
ods::pointer  character( void );

void begin_object( const char * p_name, ods::pointer ptr_type );
void set_attrib( const char * p_name, unsigned int i );
void set_attrib( const char * p_name, bool         b );
void set_attrib( const char * p_name, const char * s );
void set_attrib( const char * p_name, float        f );
void set_attrib( const char * p_name, ods::pointer & ptr_class );

ods::pointer object( const char * p_object );

void begin_attrib_object( const char * p_attrib_name );
void end_attrib_object( void );

void begin_attrib_array( const char * p_attrib_name );
void end_attrib_array( void );

void begin_array_item( void );
void end_array_item( );

void add_array_item( char c );

void end_object( );

//----------------------------------------------------------------------------
//
// pointer_templ
//
//----------------------------------------------------------------------------

template <class T > class pointer_templ : public ods::pointer
{
    public:
        pointer_templ( ) { }
        ~pointer_templ( ) { }
        pointer_templ( const null_ptr_type & n ) : ods::pointer( n ) { }
        explicit pointer_templ( const ods::pointer & ptr ) : ods::pointer( ptr ) { }
        pointer_templ( const ods::block * p_mem, ods::offset_t o ) : ods::pointer( p_mem, o ) { }

        //const pointer_templ<T> operator=( const null_ptr_type & n ) const { return pointer_templ<T>( ods::pointer::operator=(n) ); }
        
        bool operator==( const pointer_templ<T> & o) const { return ods::pointer::operator==( o ); }
        bool operator!=( const pointer_templ<T> & o) const{ return ods::pointer::operator!=( o ); }        
};

}

#endif