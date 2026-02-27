//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        raddebugwatch.hpp
//
// Subsystem:   Radical Fountation Tech - Debug Watch Interface
//
// Description: This file contains the interface used to monitor and alter 
//              an applications member variables. This subsystem works
//              in conjuction with the host tool, the Watcher. 
//
//              It also contains the macro defintions for easy use
//
//
// Revisions:   Jan 9, 2001   Creation
//
//=============================================================================

#ifndef RADDEBUGWATCH_HPP
#define RADDEBUGWATCH_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <limits.h>
#include <float.h>
#include <radoptions.hpp>
#include <radobject.hpp>
#include <radmemory.hpp>

//=============================================================================
// Defintions
//=============================================================================

//
// This function prototype is used when the client wants to be notified when
// the watch point variable is altered by the host.
//
typedef void (*RADDEBUGWATCH_CALLBACK)( void* userData );

//=============================================================================
// Forward Class Declarations
//=============================================================================

struct IRadDebugWatch;

//=============================================================================
// Functions
//=============================================================================

//
// Use these functions to intialize and terminate the watch system. The debug
// communication system must have been previously initialized. Note that cleints
// should use the inline definitions of these functions as they compile to nothing
// in a release build.  Pass the amount of memory allocated for internal buffer
// manaagement. As well, the client specifies the default name space.
// 
void radDebugWatchInitialize( const char* pNameSpace, unsigned int internalBufferSize, 
                             radMemoryAllocator alloc );
void radDebugWatchTerminate( void );

//
// Use this member to obtain the iterface to the watch functions. The marco defintions
// should normally be used. AddRef interface if you hold onto it.
//
IRadDebugWatch* radDebugWatchGet( void );
bool radDebugWatchEnabled();

//=============================================================================
// Interface Definitions
//=============================================================================

//
// The following interface is used to interact with the watch server. The 
// idenifification of process variables is done through names and namespaces.
// The combination of these two must be unique. The name is will be used to indentify
// the process variable in the user interface. 
//
struct IRadDebugWatch : public IRefCount
{
    //
    // These members are used to add various types of member variables to the watch
    // system. The first set are for adding the various sizes and types of integers. The
    // min and max values are used to control the allowable range of values that can be entered 
    // in the user interface. The callback function and assocaiated user data is optional. Pass null
    // for callback if you don't want change notification.
    //
    virtual void AddUnsignedInt( unsigned int* pValue, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, unsigned int min,  unsigned int max, bool readonly ) = 0;
    virtual void AddInt( int* pValue, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, int min, int max, bool readonly ) = 0;

    virtual void AddUnsignedShort( unsigned short* pValue, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, unsigned short min, unsigned short max, bool readonly ) = 0;
    virtual void AddShort( short* pValue, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, short min, short max, bool readonly ) = 0;

    virtual void AddUnsignedChar( unsigned char* pValue, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, unsigned char min, unsigned char max, bool readonly ) = 0;
    virtual void AddChar( char* pValue, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, char min, char max, bool readonly ) = 0;

    //
    // This member is used to add a boolean.
    //
    virtual void AddBoolean( bool* pValue, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, bool readonly ) = 0;
    
    //
    // This one is used to monitor the value of a float.
    //
    virtual void AddFloat( float* pValue, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, float min, float max, bool readonly ) = 0;

    //
    // Use this to monitor and alter the value of a string. The length must be provided and must include space for the
    // null termination.
    //
    virtual void AddString( char* pValue, unsigned int stringLength, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, bool readonly ) = 0;

    //
    // This member allows an enumerated type to be monitored and controlled. Caller must build up the table of 
    // strings identiting the enumberate values and the their associated value. The format of the table is 
    // a series of null terminated string - integer value pairs. 
    //
    struct EnumerationElement
    {
        const char*     m_Name;
        int             m_Value;
    };

    virtual void AddEnumeration( int* pValue, const EnumerationElement* pEnumerationInfo, unsigned int numberOfElements, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, bool readonly ) = 0;
        
    //
    // Use this to add a vector. Currently only support floating point vectors. Assumes vector is stored as 
    // three consectutive floats, x, y , z 
    //
    virtual void AddVector( float* pValue, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, float min, float max, bool readonly ) = 0;

    //
    // Use this member to remove a previously added watch variable
    //
    virtual void Delete( void* pValue ) = 0;

    //
    // This member is used to add a function. The host server displays a control which allows the use to invoke
    // this function. Delete it used the watch point delete function.
    //
    virtual void AddFunction( const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData ) = 0;

    //
    // This member allows the caller to associate a file with a name space. The caller can specify the format of the
    // file ( XML, #define, or const declarions. ). The file name can be specified as absolute (c:\dir\file.xml) or as
    // relative ( dir\file.xml ). 
    //
    enum FileType
    {
        FileXml,                            // Values save in file is saved using XML
        FileDefine,                         // Values saved in file usign #define 
        FileConst                           // Values saved in file using const
    };
        
    virtual void AssociateFile( const char* pNameSpace, const char* pFilename, FileType fileType ) = 0;

    //
    // This member function is provided for applications that want to implement proprietary commands on the host.
    // All this member does is send a binary blob of data to the host. This does not do anything on the Game Tuner
    // application, however can be used by other game specific implementations,
    //
    virtual void IssueHostRequest( unsigned int Id, void* pData, unsigned int length ) = 0;

};

//=============================================================================
// Macro
//=============================================================================

//
// Use these macro to add the various simple data types.
//

//
// To avoid string being compiled into release build, here are marco used for
// that purpose.
// Basically, instead of calling radDbgWatchAddUnsignedInt( ... )
// use radDbgWatchAddUnsignedInt_Macro( ( ... ) );
//
#ifdef DEBUGWATCH

#define radDbgWatchInitialize_Macro( args ) radDbgWatchInitialize args
#define radDbgWatchTerminate_Macro( args ) radDbgWatchTerminate args
#define radDbgWatchDelete_Macro( args ) radDbgWatchDelete args
#define radDbgWatchAddUnsignedInt_Macro( args ) radDbgWatchAddUnsignedInt args
#define radDbgWatchAddInt_Macro( args ) radDbgWatchAddInt args
#define radDbgWatchAddUnsignedShort_Macro( args ) radDbgWatchAddUnsignedShort args
#define radDbgWatchAddShort_Macro( args ) radDbgWatchAddShort args
#define radDbgWatchAddUnsignedChar_Macro( args ) radDbgWatchAddUnsignedChar args
#define radDbgWatchAddChar_Macro( args ) radDbgWatchAddChar args
#define radDbgWatchAddBoolean_Macro( args ) radDbgWatchAddBoolean args
#define radDbgWatchAddFloat_Macro( args ) radDbgWatchAddFloat args
#define radDbgWatchAddString_Macro( args ) radDbgWatchAddString args
#define radDbgWatchAddEnumeration_Macro( args )  radDbgWatchAddEnumeration args
#define radDbgWatchAddVector_Macro( args ) radDbgWatchAddVector args
#define radDbgWatchAddFunction_Macro( args ) radDbgWatchAddFunction args
#define radDbgWatchIssueHostRequest_Macro( args ) radDbgWatchIssueHostRequest args
#define radDbgWatchAssociateFile_Macro( args ) radDbgWatchAssociateFile args

#else 

#define radDbgWatchInitialize_Macro( args )
#define radDbgWatchTerminate_Macro( args )
#define radDbgWatchDelete_Macro( args )
#define radDbgWatchAddUnsignedInt_Macro( args )
#define radDbgWatchAddInt_Macro( args )
#define radDbgWatchAddUnsignedShort_Macro( args )
#define radDbgWatchAddShort_Macro( args )
#define radDbgWatchAddUnsignedChar_Macro( args )
#define radDbgWatchAddChar_Macro( args )
#define radDbgWatchAddBoolean_Macro( args )
#define radDbgWatchAddFloat_Macro( args )
#define radDbgWatchAddString_Macro( args )
#define radDbgWatchAddEnumeration_Macro( args )
#define radDbgWatchAddVector_Macro( args )
#define radDbgWatchAddFunction_Macro( args )
#define radDbgWatchIssueHostRequest_Macro( args )
#define radDbgWatchAssociateFile_Macro( args )

#endif // DEBUGWATCH

//=============================================================================
// Inline Function Definitions
//=============================================================================

//
// These two calls should frame all other calls to the debug watch system. Pass the
// default namespace. The other parameter controls how much space
// allocated to internal message buffers.
//
inline void radDbgWatchInitialize( const char* pNameSpace, unsigned int internalBufferSize = 16384,
                                  radMemoryAllocator alloc = RADMEMORY_ALLOC_DEFAULT )
{
#ifdef DEBUGWATCH
    radDebugWatchInitialize( pNameSpace, internalBufferSize, alloc );
#else
	(void) pNameSpace;
	(void) internalBufferSize;
    (void) alloc;
#endif
}

inline void radDbgWatchTerminate( void )
{
#ifdef DEBUGWATCH
    radDebugWatchTerminate( );
#endif
}

//
// Use this function to remove a previosuly added watch point.
//
inline void radDbgWatchDelete( void* pValue )
{
#ifdef DEBUGWATCH
	radDebugWatchGet( )->Delete( pValue );
#else
	(void) pValue;
#endif
}

inline void radDbgWatchDelete(void(*pValue)(void*))
{
    radDbgWatchDelete((void*)pValue);
}

//
// Use these functions to add the various simple data types.
//
inline void radDbgWatchAddUnsignedInt( unsigned int* pValue, const char* pName, const char* pNameSpace = NULL, RADDEBUGWATCH_CALLBACK callback = NULL, void* userData = NULL, unsigned int min = 0, unsigned int max = UINT_MAX / 2, bool readonly = false )
{
#ifdef DEBUGWATCH
	radDebugWatchGet( )->AddUnsignedInt( pValue, pName, pNameSpace, callback, userData, min, max, readonly );
#else
	(void) pValue; (void) pName; (void) pNameSpace; (void) min; (void) max; (void) callback; (void) userData; (void) readonly;
#endif
}

inline void radDbgWatchAddInt( int* pValue, const char* pName, const char* pNameSpace = NULL, RADDEBUGWATCH_CALLBACK callback = NULL, void* userData = NULL, int min = INT_MIN / 2, int max = INT_MAX / 2, bool readonly = false )
{
#ifdef DEBUGWATCH
	radDebugWatchGet( )->AddInt( pValue, pName, pNameSpace, callback, userData, min, max, readonly );
#else
	(void) pValue; (void) pName; (void) pNameSpace; (void) min; (void) max; (void) callback; (void) userData; (void) readonly;
#endif
}

inline void radDbgWatchAddUnsignedShort( unsigned short* pValue, const char* pName, const char* pNameSpace = NULL, RADDEBUGWATCH_CALLBACK callback = NULL, void* userData = NULL, unsigned short min = 0, unsigned short max = USHRT_MAX, bool readonly = false )
{
#ifdef DEBUGWATCH
	radDebugWatchGet( )->AddUnsignedShort( pValue, pName, pNameSpace, callback, userData, min, max, readonly );
#else
	(void) pValue; (void) pName; (void) pNameSpace; (void) min; (void) max; (void) callback; (void) userData; (void) readonly;
#endif
}

inline void radDbgWatchAddShort( short* pValue, const char* pName, const char* pNameSpace = NULL, RADDEBUGWATCH_CALLBACK callback = NULL, void* userData = NULL, short min = SHRT_MIN, short max = SHRT_MAX, bool readonly = false )
{
#ifdef DEBUGWATCH
	radDebugWatchGet( )->AddShort( pValue, pName, pNameSpace, callback, userData, min, max, readonly );
#else
	(void) pValue; (void) pName; (void) pNameSpace; (void) min; (void) max; (void) callback; (void) userData; (void) readonly;
#endif
}

inline void radDbgWatchAddUnsignedChar( unsigned char* pValue, const char* pName, const char* pNameSpace = NULL, RADDEBUGWATCH_CALLBACK callback = NULL, void* userData = NULL, unsigned char min = 0, unsigned char max = UCHAR_MAX, bool readonly = false )
{
#ifdef DEBUGWATCH
	radDebugWatchGet( )->AddUnsignedChar( pValue, pName, pNameSpace, callback, userData, min, max, readonly );
#else
	(void) pValue; (void) pName; (void) pNameSpace; (void) min; (void) max; (void) callback; (void) userData; (void) readonly;
#endif
}

inline void radDbgWatchAddChar( char* pValue, const char* pName, const char* pNameSpace = NULL, RADDEBUGWATCH_CALLBACK callback = NULL, void* userData = NULL, char min = CHAR_MIN, char max = CHAR_MAX, bool readonly = false )
{
#ifdef DEBUGWATCH
	radDebugWatchGet( )->AddChar( pValue, pName, pNameSpace, callback, userData, min, max, readonly );
#else
	(void) pValue; (void) pName; (void) pNameSpace; (void) min; (void) max; (void) callback; (void) userData; (void) readonly;
#endif
}

inline void radDbgWatchAddBoolean( bool* pValue, const char* pName, const char* pNameSpace = NULL, RADDEBUGWATCH_CALLBACK callback = NULL, void* userData = NULL, bool readonly = false )
{
#ifdef DEBUGWATCH
	radDebugWatchGet( )->AddBoolean( pValue, pName, pNameSpace, callback, userData, readonly );
#else
	(void) pValue; (void) pName; (void) pNameSpace; (void) callback; (void) userData; (void) readonly;
#endif
}

inline void radDbgWatchAddFloat( float* pValue, const char* pName, const char* pNameSpace = NULL, RADDEBUGWATCH_CALLBACK callback = NULL, void* userData = NULL, float min = 0.0f, float max = 1.0f, bool readonly = false )
{
#ifdef DEBUGWATCH
	radDebugWatchGet( )->AddFloat( pValue, pName, pNameSpace, callback, userData, min, max, readonly );
#else
	(void) pValue; (void) pName; (void) pNameSpace; (void) min; (void) max; (void) callback; (void) userData; (void) readonly;
#endif
}

//
// Use this to add string. Lenght must include space for null termination.
//
inline void radDbgWatchAddString( char* pValue, unsigned int stringLength, const char* pName, const char* pNameSpace = NULL, RADDEBUGWATCH_CALLBACK callback = NULL, void* userData = NULL, bool readonly = false )
{
#ifdef DEBUGWATCH
	radDebugWatchGet( )->AddString( pValue, stringLength, pName, pNameSpace, callback, userData, readonly );
#else
	(void) pValue; (void) stringLength; (void) pName; (void) pNameSpace; (void) callback; (void) userData; (void) readonly;
#endif
}

//
// This member allows an enumerated type to be monitored and controlled. Caller must build up the table of 
// strings identiting the enumberate values and the their associated value. The format of the table is 
// a series of null terminated string - integer value pairs, terminated by a null.
//
inline void radDbgWatchAddEnumeration( int* pValue, const IRadDebugWatch::EnumerationElement* pEnumerationInfo, unsigned int numberOfElements, const char* pName, const char* pNameSpace = NULL, RADDEBUGWATCH_CALLBACK callback = NULL, void* userData = NULL, bool readonly = false )
{
#ifdef DEBUGWATCH
	radDebugWatchGet( )->AddEnumeration( pValue, pEnumerationInfo, numberOfElements, pName, pNameSpace, callback, userData, readonly );
#else
	(void) pValue; (void) pEnumerationInfo; (void) numberOfElements; (void) pName; (void) pNameSpace; (void) callback; (void) userData; (void) readonly;
#endif
}

//
// Use this to add a vector. Currently only support floating point vectors. Assumes vector is stored as 
// three consectutive floats, x, y , z 
//
inline void radDbgWatchAddVector( float* pValue, const char* pName, const char* pNameSpace = NULL, RADDEBUGWATCH_CALLBACK callback = NULL, void* userData = NULL, float min = 0.0f, float max = 1.0f, bool readonly = false )
{
#ifdef DEBUGWATCH
	radDebugWatchGet( )->AddVector( pValue, pName, pNameSpace, callback, userData, min, max, readonly );
#else
	(void) pValue; (void) pName; (void) pNameSpace; (void) min; (void) max; (void) callback; (void) userData; (void) readonly;
#endif
}

//
// This member allows a function to be added. Host tool will display a control which will allow user to invoked
// function.
//
inline void radDbgWatchAddFunction( const char* pName, RADDEBUGWATCH_CALLBACK callback, void* userData = NULL, const char* pNameSpace = NULL )
{
#ifdef DEBUGWATCH
	radDebugWatchGet( )->AddFunction( pName, pNameSpace, callback, userData );
#else
	(void) pName; (void) pNameSpace; (void) callback; (void) userData;
#endif
}

//
// This member simply sends a blob of data to the host. Immplementation specific as to what it does.
//
inline void radDbgWatchIssueHostRequest( unsigned int Id, void* pData, unsigned int length )
{
#ifdef DEBUGWATCH
	radDebugWatchGet( )->IssueHostRequest( Id, pData, length );
#else
	(void) Id; (void) pData; (void) length;
#endif
}

//
// Use this macro to assocaiate file with a name space.
//
inline void radDbgWatchAssociateFile( const char* pNameSpace, const char* pFilename, IRadDebugWatch::FileType fileType = IRadDebugWatch::FileXml )
{
#ifdef DEBUGWATCH
	radDebugWatchGet( )->AssociateFile( pNameSpace, pFilename, fileType );
#else
	(void) pNameSpace; (void) pFilename; (void) fileType;
#endif
}

#endif

