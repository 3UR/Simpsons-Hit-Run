//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radprofiler.hpp
//
// Subsystem:   Foundation Technologies - radProfiler
//
// Description: This file contain all interfaces used for rad profiler.
//              It is based on Steve Rabin's Real-Time In-Game Profiling
//              in "Game Programming Gems" - Mark Deloura
//
// Notes:       
//
// Revisions:   July 10, 2002        James Tan
//
//=============================================================================

#ifndef RADPROFILER_HPP
#define RADPROFILER_HPP

//=============================================================================
// Include Files
//=============================================================================
#include <radoptions.hpp>

#include <radobject.hpp>
#include <radmemory.hpp>

//=============================================================================
// External Defintions
//=============================================================================

//=============================================================================
// Forward Class/Struct Declarations
//=============================================================================
struct IRadProfileSample;

struct IRadProfiler;

//=============================================================================
// Functions
//=============================================================================

//
// InitReserveProfileSamplesMemory - reserve memory for profile samples, this
// requires no memory allocation for profiler during execution, and much less
// memory fragmentation.
//
// MaxProfileSamples - max number of profile sample we can have, useful if
// something goes wrong, so we will get asserted instead of program goes into
// some bad state.
//
// UseWatcher - if user wants to use debug watcher as default output of
// profiler data.
//
// alloc - allocator for memory used by profiler.
//
void radProfilerInitialize( unsigned int InitReserveProfileSamplesMemory = 50, unsigned int MaxProfileSamples = 200, bool UseWatcher = true, radMemoryAllocator alloc = RADMEMORY_ALLOC_DEFAULT );

//
// Terminate the profiler.
//
void radProfilerTerminate( void );

//
// Return the pointer of the profiler sub-system.
//
IRadProfiler * radProfilerGet( );

//=============================================================================
// Interfaces
//=============================================================================

//
// Profile sample, and allow navigating through the entire profiler sample lists.
// Profile data is organized into a hierarchy tree.
// For Example:
//     Game loop
//          - Render
//          - AI
//          - Physics
//
// Notes: If user want to implement their own display for profile sample, code
//        to display profile sample must be vanished if RADPROFILER is not defined.
//        Because profiler is gone if RADPROFILER is not defined.
//
struct IRadProfileSample : public IRefCount
{
    //
    // Get parent sample node.
    //
    virtual IRadProfileSample * GetParentNode( ) = 0;

    //
    // Get the name of the profile sample
    //
    virtual const char * GetName( ) = 0;

    //
    // Get first child node.
    //
    virtual IRadProfileSample * GetFirstChildNode( ) = 0;

    //
    // Get next sibling node
    //
    virtual IRadProfileSample * GetNextSilbingNode( ) = 0;

    //
    // Get prev sibling node
    //
    virtual IRadProfileSample * GetPrevSiblingNode( ) = 0;

    //
    // Get sample result data (msec based)
    //
    virtual void GetSampleResult( float * pAveTimeInMSec, float * pMinTimeInMSec, float * pMaxTimeInMSec, unsigned int * pExecutionCount ) = 0;

    //
    // Get sample result data (frame based)
    //
    virtual void GetSampleResultPerFrame( float * pAveTimePerFrameInPercent, float * pMinTimePerFrameInPercent, float * pMaxTimePerFrameInPercent, unsigned int * pExecutionCountPerFrame ) = 0;

};

//
// Profiler
//
struct IRadProfiler : public IRefCount
{
    //
    // Begin a profiling a block of code, name must be unique
    //
    virtual void BeginProfile( const char * pProfileName ) = 0;

    //
    // End a profiling a block of code, name must be unique
    //
    virtual void EndProfile( const char * pProfileName ) = 0;

    //
    // Mark beginning of a frame,
    //
    virtual void BeginFrame( ) = 0;

    //
    // Mark ending of a frame,
    //
    virtual void EndFrame( ) = 0;

    //
    // Get the root profile sample 
    //
    virtual IRadProfileSample * GetRootProfileSample( ) = 0;

    //
    // Get a profile sample by name, name is always unique
    //
    virtual IRadProfileSample * GetProfileSample( const char * pName ) = 0;
};


//
// Macro and inlines for profiling, they will make sure all functions are
// vanished if RADPROFILER is not defined.
//
#ifndef RADPROFILER

    #define radProfilerBeginProfile( string )
    #define radProfilerEndProfile( string )

    #define radProfilerBeginFrame( )
    #define radProfilerEndFrame( )

    inline void radProfilerInitialize( unsigned int InitReserveProfileSamplesMemory, unsigned int MaxProfileSamples, bool UseWatcher, radMemoryAllocator alloc )
    {
        (void)InitReserveProfileSamplesMemory;
        (void)MaxProfileSamples;
        (void)UseWatcher;
        (void)alloc;
    }

    inline void radProfilerTerminate( )
    {
    }

    inline IRadProfiler * radProfilerGet( )
    {
        return NULL;
    }

#else

    //
    // wrap around a piece of code using those two functions, it
    // it measure the time used by a piece of code.
    //
    #define radProfilerBeginProfile( string )     radProfilerGet( )->BeginProfile( string );
    #define radProfilerEndProfile( string )       radProfilerGet( )->EndProfile( string );

    //
    // wrap those two functions around beginning/ending rendering of a
    // frame. Stats are based on frame.
    //
    #define radProfilerBeginFrame( )     radProfilerGet( )->BeginFrame( );
    #define radProfilerEndFrame( )       radProfilerGet( )->EndFrame( );

#endif

#endif // RADPROFILER_HPP

