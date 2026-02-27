//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        raddebug.hpp
//
// Subsystem:   Foundation Technolgies Debug Services
//
// Description: This file contains all definitions and functions prototypes
//              required to use  debug services. These functions include simple 
//              assertion and printing functions.
//
// Note:        These functions do not conform to the naming conventions used
//              by FTECH for historical reasons.

// Revisions:   V1.00 Mar 4, 2001           Creation
//
//=============================================================================

#ifndef  RADDEBUG_HPP
#define  RADDEBUG_HPP

//=============================================================================
// Build Configuration Check
//=============================================================================

#if !defined(RAD_UWP) && !defined(RAD_WIN32)
    #error 'FTech requires definition of RAD_UWP, or RAD_WIN32'
#endif
#if !defined(RAD_DEBUG) && !defined(RAD_TUNE) && !defined(RAD_RELEASE) 
    #error 'FTech requires definition of RAD_DEBUG, RAD_TUNE, or RAD_RELEASE'
#endif

//=============================================================================
// Include Files
//=============================================================================

#include <stdio.h>
#include <stdarg.h>

//=============================================================================
// Definitions
//=============================================================================

typedef void (radDebugOutputHandler)(const char * pString );

//=============================================================================
// Macros and defines
//=============================================================================

//
// all mesages will be truncated to be this size
//
#define RAD_DEBUG_PRINT_MAX_LENGTH 1024

//=============================================================================
// Function Prototypes
//=============================================================================

//
// DON'T USE THESE FUNCTIONS, USE THE MACROS BELOW.
//

#if defined (RAD_WIN32) || defined (RAD_UWP)
#ifdef __cplusplus
    bool rDebugAssertFail_Implementation( const char* condition, const char* filename, unsigned int linenum);
#else
    int rDebugAssertFail_Implementation( const char* condition, const char* filename, unsigned int linenum);
#endif
#endif

void rDebugHaltOnAsserts_Implementation( bool halt );
void rDebugWarningFail_Implementation( const char* condition, const char* filename, unsigned int linenum);
void rDebugValidFail_Implementation( const char *condition, const char *filename, unsigned int linenum);

void rDebuggerString_Implementation( const char* string );
void rDebugString_Implementation( const char * pString );

void rDebugChannelInitialize_Implementation( int allocator );
void rDebugChannel_Implementation( const char * pChannel, const char * pMessage );
void rDebugChannelEnable_Implementation( const char * pChannel );
void rDebugChannelDisable_Implementation( const char * pChannelName );
void rDebugChannelTerminate_Implementaion( void );

//
// USE THESE MACROS AND FUNCTIONS
//

#define rNullStringWrap( x ) ( ( x ) == NULL ? "<NULL>" : x )
void rDebugSetOutputHandler     ( radDebugOutputHandler * pOutputProc );

//
// These functions are always available, that is, they get compiled in
// in release.
//

#if defined( RAD_WIN32 ) //|| defined( RAD_UWP )
	#define rReleaseBreak() { __debugbreak(); }
#else
    #define rReleaseBreak() { }
#endif

//#define rReleaseString( x )( rDebugString_Implementation( x ) )
#define rReleaseString( x )( (void)0 )
//
// Always compile in release asserts.
//


#define rReleaseAssert( x ) if (!(x)) if ( rDebugAssertFail_Implementation(#x,__FILE__,__LINE__) ) { rReleaseBreak(); }
#define rReleaseAssertMsg( x, msg )  if (!(x)) if ( rDebugAssertFail_Implementation(msg,__FILE__,__LINE__) ) { rReleaseBreak(); }
#define rReleaseWarning( x ) if (!(x)) rDebugWarningFail_Implementation(#x,__FILE__,__LINE__)
#define rReleaseWarningMsg( x, msg ) if (!(x)) rDebugWarningFail_Implementation(msg,__FILE__,__LINE__)

void rReleasePrintf( const char *fmt, ... );

//
// Tune asserts, compiled only in RAD_TUNE and RAD_DEBUG
//

#if defined RAD_DEBUG || defined RAD_TUNE

    #define rTuneString( x )(rDebugString_Implementation( x ) )
    #define rTuneAssert( x ) if (!(x)) if ( rDebugAssertFail_Implementation(#x,__FILE__,__LINE__) ) { rReleaseBreak(); }
    #define rTuneAssertMsg( x, msg )  if (!(x)) if ( rDebugAssertFail_Implementation(msg,__FILE__,__LINE__) ) { rReleaseBreak(); }
    #define rTuneWarning( x ) if (!(x)) rDebugWarningFail_Implementation(#x,__FILE__,__LINE__)
    #define rTuneWarningMsg( x, msg ) if (!(x)) rDebugWarningFail_Implementation(msg,__FILE__,__LINE__)

    void rTunePrintf( const char *fmt, ... );

#else

    //
    // These functions get compiled out in release
    //
    #define rTuneString( x )((void )0)       // Output string

    #define rTuneAssert( x ) ((void)0)            // Assert condition X TRUE
    #define rTuneAssertMsg( x, msg ) ((void)0)    // Assert x true, display msg if not
    #define rTuneWarning( x ) ((void)0)           // If x false, display warning
    #define rTuneWarningMsg( x, msg ) ((void)0)   // If x false, display msg

    #if defined (RAD_WIN32) || defined (RAD_UWP)
        inline void rTunePrintf( const char *fmt, ... ) { }
    #endif

#endif

//
// Compiled in in RAD_DEBUG only
//

#if defined RAD_DEBUG 

    #define rBreak() rReleaseBreak()
    #define rWarning( x ) if (!(x)) rDebugWarningFail_Implementation(#x,__FILE__,__LINE__)
    #define rWarningMsg( x, msg ) if (!(x)) rDebugWarningFail_Implementation(msg,__FILE__,__LINE__)
    #define rDebugString( x )(rDebugString_Implementation( x ) )
    #define rDebugChannelInitialize( x ) rDebugChannelInitialize_Implementation( x )
    #define rDebugChannel( y, x ) rDebugChannel_Implementation( y, x )
    #define rDebugChannelEnable( y ) rDebugChannelEnable_Implementation( y )
    #define rDebugChannelDisable( y ) rDebugChannelDisable_Implementation( y )
    #define rDebugChannelTerminate() rDebugChannelTerminate_Implementaion( )

    #define rDebugHaltOnAsserts( x ) rDebugHaltOnAsserts_Implementation( x )

    #define rAssert( x ) if (!(x)) if ( rDebugAssertFail_Implementation(#x,__FILE__,__LINE__) ) { rReleaseBreak(); }
	#define rAssertMsg( x, msg )  if (!(x)) if ( rDebugAssertFail_Implementation(msg,__FILE__,__LINE__) ) { rReleaseBreak(); }
    
    void rDebugPrintf( const char *fmt, ... );

    void rDebugChannelPrintf( const char * pChannel, const char *fmt, ... );

#else

    #define rDebugHaltOnAsserts( x ) ((void)0)
    //
    // These functions get compiled out in release
    //
    #define rBreak() ((void)0)
    #define rAssert( x ) ((void)0)            // Assert condition X TRUE
    #define rAssertMsg( x, msg ) ((void)0)    // Assert x true, display msg if not
    #define rWarning( x ) ((void)0)           // If x false, display warning
    #define rWarningMsg( x, msg ) ((void)0)   // If x false, display msg
    
    #define rDebugString( x )((void )0)       // Output string
    #define rDebugChannelInitialize( x ) ((void)0)
    #define rDebugChannel( y, x ) ((void)0)
    #define rDebugChannelEnable( y ) ((void)0)
    #define rDebugChannelDisable( y ) ((void)0)
    #define rDebugChannelTerminate()((void)0)

    #if defined (RAD_WIN32) || defined (RAD_UWP)
        inline void rDebugPrintf( const char *fmt, ... ) { }
        inline void rDebugChannelPrintf( const char *fmt, ... ) { }
    #endif

#endif

#endif // RADDEBUG_HPP
