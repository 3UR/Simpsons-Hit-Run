//===========================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Radical performance statistics collection service.
//
// Description: This file contains classes and definitions relevant to the
//              statistics collection facilities.
//
// Authors:     Soleil Lapierre
//
//===========================================================================

// Recompilation protection flag.
#ifndef __STATMGR_H
#define __STATMGR_H

#ifdef PROFILE    // Stats stuff only exists in debug builds.


//===========================================================================
// Nested Includes
//===========================================================================

#include <stdio.h>
#include <radmemory.hpp>
#include <raddebug.hpp>
#include <radstats.hpp>
#include "simplestat.hpp"
#include "statmanager.hpp"

#define MODULE_NAME_MAX_LENGTH 256


//===========================================================================
// Private Class Definitions
//===========================================================================

//===========================================================================
//
// Description: This private class implements the global statistica manager.
//              It is a singleton, but manages stats accumulators for
//              multiple modules.
//
//              This object is not reference counted because it is a 
//              singleton and nobody is ever given a pointer to it. All
//              public access is done via global functions.
//
//===========================================================================

class radStatManager
{
    public:

    void Initialize( radMemoryAllocator alloc );
    void Terminate ( void );

    //
    // Implementations of the public access methods.
    //
    void SetAccumulatorName( const char* moduleName, unsigned int index, const char* accumName );
    void GetAccumulatorName( const char* moduleName, unsigned int index, char* pAccumName );
    void InitModule( const char* moduleName, unsigned int numEntries, unsigned int numSamplesPerEntry );
    void TerminateModule( const char* moduleName );
    void ResetAccumulator( const char* moduleName, unsigned int index, unsigned int numSamplesToStore );
    void AddSample( const char* moduleName, unsigned int index, float sample );
    void Query( const char* moduleName, unsigned int index, unsigned int* pNumSamples, 
                float* pTotal, float* pMean, float* pMax, float* pMin, float* pStandardDeviation );
    void QueryPercentile( const char* moduleName, unsigned int index, 
                          unsigned int percentile, float* pNumBelow );
    void EnumerateAccumulators( const char* modulePattern, const char* accumPattern,
                       void (*enumerationCallback)( const char* moduleName, const char* accumName ) );

    //
    // This member determines if there are any active stats modules.
    //
    bool ModulesExist( void );

    //
    // This member converts a user-defined accumulator name to an index within a module.
    //
    unsigned int GetIndex( const char* moduleName, const char* accumName );

    static bool        s_Initialized;

    private:

    //
    // This structure holds the stats accumulators for one code module.
    // It also implements part of a linked list.
    //
    struct radStatModuleContainer
    {
        IRadSimpleStatistic** m_Accumulators;
        unsigned int m_NumAccum;
        char m_ModuleName[ MODULE_NAME_MAX_LENGTH ];
        radStatModuleContainer* m_Next;
    };

    //
    // This is the head of our linked list of module stats containers.
    //
    radStatModuleContainer* m_Modules;

    //
    // This method locates a module container in the list given a name.
    //
    radStatModuleContainer* FindModule( const char* moduleName );

    //
    // Allocator for internal use.
    //
    static radMemoryAllocator s_Alloc;
};

#endif                                  // PROFILE

#endif                                  // End conditional inclusion

