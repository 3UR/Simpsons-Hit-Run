//===========================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Statistics collection service.
//
// Description: This file contains the implementation of the basic 
//              performance statistics collection utilities.
//
// Author:      Soleil Lapierre
//
//===========================================================================

//---------------------------------------------------------------------------
// Includes
//===========================================================================

#include "pch.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <radstring.hpp>
#include <radstats.hpp>     // interface specification
#include "statmanager.hpp"  // class specification


#ifdef PROFILE    // we only profile in debug mode


//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================


//===========================================================================
// Static Data Definitions
//===========================================================================

//
// The singleton file server instance.
//
static radStatManager  s_theManager;

bool               radStatManager::s_Initialized = false;
radMemoryAllocator radStatManager::s_Alloc       = RADMEMORY_ALLOC_DEFAULT;


//===========================================================================
// Global Functions
//===========================================================================

//===========================================================================
// radStatInitialize
//===========================================================================
// Description: Initialize statistics gathering for a module. This also
//              initializes the statistics manager if needed. The manager
//              adds a section for the named module to store its stats in.
//
// Constraints:	The statistics manager is a singleton that internally
//              manages multiple statistics structures identified by
//              name strings. 
//
// Parameters:	moduleName - the name of amoduel to add or reset.
//              numEntries - the number of accumulators for this module.
//              numSamplesPerEntry - the history length to store for each 
//                  accumulator.
//
//===========================================================================

void radStatInitialize_Implementation
( 
    const char* moduleName, 
    unsigned int numEntries,                                        
    unsigned int numSamplesPerEntry,
    radMemoryAllocator alloc
)
{
    //
    // First instantiate the singleton if it does not exist.
    //
    if( s_theManager.s_Initialized )
    {
        s_theManager.Terminate( );
    }
    s_theManager.Initialize( alloc );
    s_theManager.InitModule( moduleName, numEntries, numSamplesPerEntry );
    rAssert( s_theManager.ModulesExist( ) );
}


//===========================================================================
// radStatTerminate
//===========================================================================
// Description: Terminate statistics gathering for a module and free up
//              associated memory. 
//
// Constraints:	In order to avoid memory leaks, this function should
//              be called to clean up all active modules before the program
//              exits.
//
// Parameters:	moduleName - the stats module to delete.
//
//===========================================================================

void radStatTerminate_Implementation( const char* moduleName )
{
    if( !s_theManager.s_Initialized )
    {
        return;
    }

    s_theManager.TerminateModule( moduleName );

    //
    // If there are no more modules, delete the singleton.
    //
    if( !s_theManager.ModulesExist( ) )
    {
        s_theManager.Terminate( );
    }
}


//===========================================================================
// radStatSetAccumulatorName
//===========================================================================
// Description: Assign a user-defined name string to a stats accumulator.
//
// Parameters:	moduleName - the name of the module owning the accumulator.
//              index - the number of the accumulator to clear.
//              accumName - the string to use as a label for the register.
//
//===========================================================================

void radStatSetAccumulatorName_Implementation
( 
    const char* moduleName, 
    unsigned int index,                       
    const char* accumName
)
{
    if( !s_theManager.s_Initialized )
    {
        return;
    }

    s_theManager.SetAccumulatorName( moduleName, index, accumName );
}


//===========================================================================
// radStatGetAccumulatorName
//===========================================================================
// Description: Retrieve the user-defined name of a stats accumulator
//
// Parameters:	moduleName - the name of the module owning the accumulator.
//              index - the number of the accumulator to clear.
//
// Returns:     accumName - buffer to copy the name to. Will be set to the
//                      empty string if the accumulator has no name.
//
//===========================================================================

void radStatGetAccumulatorName_Implementation
( 
    const char* moduleName, 
    unsigned int index,                       
    char* accumName 
)
{
    if( !s_theManager.s_Initialized )
    {
        return;
    }

    s_theManager.GetAccumulatorName( moduleName, index, accumName );
}


//===========================================================================
// radStatReset
//===========================================================================
// Description: Reset a single accumulator in a statistics module. This
//              zeroes out the gathered data of the accumulator. The
//              optional parameter can be used to change the size of the
//              next sample history the accumulator will store.
//
// Parameters:	moduleName - the name of the module owning the accumulator.
//              index - the number of the accumulator to clear.
//              numSamplesToStore - (optional) the size of the history buffer
//                  to save samples in for the standard deviation and
//                  percentile calculations.
//
//===========================================================================

void radStatReset_Implementation
( 
    const char* moduleName, 
    unsigned int index,    
    unsigned int numSamplesToStore
)
{
    if( !s_theManager.s_Initialized )
    {
        return;
    }

    s_theManager.ResetAccumulator( moduleName, index, numSamplesToStore );
}


//===========================================================================
// radStatReset
//===========================================================================
// Description: Reset a single accumulator in a statistics module. This
//              zeroes out the gathered data of the accumulator. The
//              optional parameter can be used to change the size of the
//              next sample history the accumulator will store.
//
// Parameters:	moduleName - the name of the module owning the accumulator.
//              accumName - the user-assigned name of the accumulator.
//              numSamplesToStore - (optional) the size of the history buffer
//                  to save samples in for the standard deviation and
//                  percentile calculations.
//
//===========================================================================

void radStatReset_Implementation
( 
    const char* moduleName, 
    const char* accumName,    
    unsigned int numSamplesToStore
)
{
    if( !s_theManager.s_Initialized )
    {
        return;
    }

    unsigned int index = s_theManager.GetIndex( moduleName, accumName );
    s_theManager.ResetAccumulator( moduleName, index, numSamplesToStore );
}


//===========================================================================
// radStatAddSample
//===========================================================================
// Description: Record a sample for later statistical analysis. This simple
//              adds an integer to one of the accumulators.
//
// Parameters:	moduleName - the name of the module owning the accumulator.
//              index - the number of the accumulator to update.
//              sample - the number to add to the accumulator.
//
//===========================================================================

void radStatAddSample_Implementation
( 
    const char* moduleName, 
    unsigned int index, 
    float sample 
)
{
    if( !s_theManager.s_Initialized )
    {
        return;
    }

    s_theManager.AddSample( moduleName, index, sample );
}


void radStatAddSample_Implementation
( 
    const char* moduleName, 
    unsigned int index, 
    int sample 
)
{
    if( !s_theManager.s_Initialized )
    {
        return;
    }

    s_theManager.AddSample( moduleName, index, ( float ) sample );
}


//===========================================================================
// radStatAddSample
//===========================================================================
// Description: Record a sample for later statistical analysis. This simple
//              adds an integer to one of the accumulators.
//
// Parameters:	moduleName - the name of the module owning the accumulator.
//              accumName - the user-assigned name of the accumulator.
//              sample - the number to add to the accumulator.
//
//===========================================================================

void radStatAddSample_Implementation
( 
    const char* moduleName, 
    const char* accumName,
    float sample 
)
{
    if( !s_theManager.s_Initialized )
    {
        return;
    }

    unsigned int index = s_theManager.GetIndex( moduleName, accumName );
    s_theManager.AddSample( moduleName, index, sample );
}


void radStatAddSample_Implementation
( 
    const char* moduleName, 
    const char* accumName,
    int sample 
)
{
    if( !s_theManager.s_Initialized )
    {
        return;
    }

    unsigned int index = s_theManager.GetIndex( moduleName, accumName );
    s_theManager.AddSample( moduleName, index, ( float ) sample );
}


//===========================================================================
// radStatQuery
//===========================================================================
// Description: Query an accumulator about the statistics it has gathered.
//              This function returns multiple statistics about the
//              samples that have been added to an accumulator since the
//              last time it was reset.
//
// Parameters:	moduleName - the name of the module owning the accumulator.
//              index - the number of the accumulator to query.
//
// Returns:     pNumSamples - optional place to return the number of samples
//                            that have been entered.
//              pTotal - optional place to store the total value of the samples.
//              pMean - optional place to store the average of the samples.
//              pMax - optional place to store the maximum sample value.
//              pMin - optional place to store the minimum sample value.
//              pStandardDeviation - optional place to store the standard
//                      deviation of the recorded samples.
//
//===========================================================================

void radStatQuery_Implementation
( 
    const char* moduleName, 
    unsigned int index,
    unsigned int* pNumSamples, 
    float* pTotal,                  
    float* pMean, 
    float* pMax, 
    float* pMin,
    float* pStandardDeviation 
)
{
    if( !s_theManager.s_Initialized )
    {
        return;
    }

    s_theManager.Query( moduleName, index, pNumSamples, pTotal, 
                        pMean, pMax, pMin, pStandardDeviation );
}


//===========================================================================
// radStatQuery
//===========================================================================
// Description: Query an accumulator about the statistics it has gathered.
//              This function returns multiple statistics about the
//              samples that have been added to an accumulator since the
//              last time it was reset.
//
// Parameters:	moduleName - the name of the module owning the accumulator.
//              accumName - the user-assigned name of the accumulator to query.
//
// Returns:     pNumSamples - optional place to return the number of samples
//                            that have been entered.
//              pTotal - optional place to store the total value of the samples.
//              pMean - optional place to store the average of the samples.
//              pMax - optional place to store the maximum sample value.
//              pMin - optional place to store the minimum sample value.
//              pStandardDeviation - optional place to store the standard
//                      deviation of the recorded samples.
//
//===========================================================================

void radStatQuery_Implementation
( 
    const char* moduleName, 
    const char* accumName,
    unsigned int* pNumSamples, 
    float* pTotal,                  
    float* pMean, 
    float* pMax, 
    float* pMin,
    float* pStandardDeviation 
)
{
    if( !s_theManager.s_Initialized )
    {
        return;
    }

    unsigned int index = s_theManager.GetIndex( moduleName, accumName );
    s_theManager.Query( moduleName, index, pNumSamples, pTotal, 
                        pMean, pMax, pMin, pStandardDeviation );
}


//===========================================================================
// radStatQueryPercentile
//===========================================================================
// Description: This function queries the recorded statistical samples
//              in one accumulator and answers the question: "What value
//              are N% of the samples at or below?" That is, the input
//              is a number from 1 to 99 representing a percentage,
//              and the output is a sample value that is that percentage
//              of the way through a sorted list of the recorded samples.
//              The 0th percentile is the minimum. The 50th percentile
//              is the median. The 100th percentile is the maximum sample value.
//
// Parameters:	moduleName - the name of the module owning the accumulator.
//              index - the number of the accumulator to query.
//              percentile - the percentile to get (1 to 99 are valid).
//
// Returns:     pNumBelow - place to store the answer.
//
//===========================================================================
void radStatQueryPercentile_Implementation
( 
    const char* moduleName, 
    unsigned int index,                       
    unsigned int percentile, 
    float* pNumBelow
)
{
    if( !s_theManager.s_Initialized )
    {
        return;
    }

    s_theManager.QueryPercentile( moduleName, index, percentile, pNumBelow );
}


//===========================================================================
// radStatQueryPercentile
//===========================================================================
// Description: This function queries the recorded statistical samples
//              in one accumulator and answers the question: "What value
//              are N% of the samples at or below?" That is, the input
//              is a number from 1 to 99 representing a percentage,
//              and the output is a sample value that is that percentage
//              of the way through a sorted list of the recorded samples.
//              The 0th percentile is the minimum. The 50th percentile
//              is the median. The 100th percentile is the maximum sample value.
//
// Parameters:	moduleName - the name of the module owning the accumulator.
//              accumName - the user-assigned name of the accumulator to query.
//              percentile - the percentile to get (1 to 99 are valid).
//
// Returns:     pNumBelow - place to store the answer.
//
//===========================================================================
void radStatQueryPercentile_Implementation
( 
    const char* moduleName, 
    const char* accumName,
    unsigned int percentile, 
    float* pNumBelow
)
{
    if( !s_theManager.s_Initialized )
    {
        return;
    }

    unsigned int index = s_theManager.GetIndex( moduleName, accumName );
    s_theManager.QueryPercentile( moduleName, index, percentile, pNumBelow );
}


//===========================================================================
// radStatEnumerateAccumulators
//===========================================================================
// Description: This function enumerates modules and accumulators. For each
//              accumulator in each module matching given wildcard patterns,
//              it invokes given callback function.
//
// Parameters:	modulePattern - wildcard pattern to match module names against.
//              accumPattern - wildcard pattern to match accumulator names against.
//              enumerationCallback - function to call with each match.
//
// Returns:     None.
//
// Notes:       Enumeration is implemented via a direct callback function 
//              instead of via an interface to avoid polluting class
//              definitions with ifdefs. This way a class doesn't need to
//              ifdef the list of interfaces it implements.
//
//===========================================================================

void radStatEnumerateAccumulators_Implementation
( 
    const char* modulePattern, 
    const char* accumPattern,    
    void (*enumerationCallback)( const char* moduleName, const char* accumName )
)
{
    if( !s_theManager.s_Initialized )
    {
        return;
    }

    s_theManager.EnumerateAccumulators( modulePattern, accumPattern, enumerationCallback );
}



//===========================================================================
// Member Functions
//===========================================================================

//===========================================================================
// radStatManager::Initialize
//===========================================================================
// Description: Initializes the manager and records the allocator to use.
//
// Parameters:	None.
//
// Returns:     None.
//
//===========================================================================

void radStatManager::Initialize( radMemoryAllocator alloc )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radStatManager" );
    m_Modules = NULL;
    s_Alloc = alloc;
    s_Initialized = true;
}


//===========================================================================
// radStatManager::Terminate
//===========================================================================
// Description: Frees all memory allocated by the manager.
//
// Parameters:	None.
//
// Returns:     None.
//
//===========================================================================

void radStatManager::Terminate( void )
{
    radStatModuleContainer* module;
    radStatModuleContainer* temp;
    module = m_Modules;
    while( module != NULL )
    {
        temp = module;
        module = module->m_Next;
        if( temp->m_Accumulators != NULL )
        {
            for( unsigned int i = 0; i < temp->m_NumAccum; i++ )
            {
                radRelease( temp->m_Accumulators[ i ], this );
            }
            radMemoryFree( s_Alloc, temp->m_Accumulators );
        }
        radMemoryFree( s_Alloc, temp );
    }
}


//===========================================================================
// radStatManager::ModulesExist
//===========================================================================
// Description: Determine if there are any active statistics collection modules.
//
// Parameters:	None.
//
// Returns:     True if there are any active modules. If false is returned,
//              it means the stats manager itself can be deleted.
//
//===========================================================================

bool radStatManager::ModulesExist( void )
{
    return( m_Modules != NULL );
}


//===========================================================================
// radStatManager::FindModule
//===========================================================================
// Description: Private function for finding a stats module by name. 
//
// Parameters:	moduleName - the name of the module to search for.
//
// Returns:     A pointer to the module container or NULL if not found.
//
//===========================================================================

radStatManager::radStatModuleContainer* radStatManager::FindModule( const char* moduleName )
{
    radStatModuleContainer* module = m_Modules;
    bool found = false;
    while( !found && module != NULL )
    {
        if( strcmp( moduleName, module->m_ModuleName ) == 0 )
        {
            found = true;
        }
        else
        {
            module = module->m_Next;
        }
    }

    return( module );
}


//===========================================================================
// radStatManager::GetIndex
//===========================================================================
// Description: Searches for a stats accumulator with a given user-defined
//              name, and returns its numeric index within the module.
//
// Parameters:	moduleName - the name of the module to search for.
//              accumName - the accumulator name to search for.
//
// Returns:     The index that can be used to address the accumulator.
//
//===========================================================================

unsigned int radStatManager::GetIndex( const char* moduleName, const char* accumName )
{
    radStatModuleContainer* module = FindModule( moduleName );
    unsigned int index = 0xffffffff;  // invalid index

    if( module != NULL )
    {
        bool found = false;
        for( unsigned int i = 0; !found && i < module->m_NumAccum; i++ )
        {
            char name[ 256 ];
            module->m_Accumulators[ i ]->GetName( name );
            if( strcmp( accumName, name ) == 0 )
            {
                found = true;
                index = i;
            }
        }
    }
    else
    {
        char buf[ 512 ];
        sprintf( buf, "Statistics module %s not found.\n", moduleName );
        rAssertMsg( false, buf );
    }

    if( index == 0xffffffff )
    {
        rDebugPrintf( "Warning: No accumulator called '%s' found in module %s\n",
                      accumName, moduleName );
    }

    return( index );
}


//===========================================================================
// radStatManager::InitModule
//===========================================================================
// Description: Create a new statistics collection module or reset an 
//              existing one.
//
// Parameters:	moduleName - the name of the module to create or clear.
//              numEntries - the number of accumulators we want in it.
//              numSamplesPerEntry - the history size for all acucmulators.
//
// Returns:     None.
//
//===========================================================================

void radStatManager::InitModule
( 
    const char* moduleName, 
    unsigned int numEntries, 
    unsigned int numSamplesPerEntry 
)
{
    //
    // Locate the module. If it doesn't exist, create one and store it.
    //
    radStatModuleContainer* module = FindModule( moduleName );
    if( module == NULL )
    {
        module = ( radStatModuleContainer* )radMemoryAlloc( s_Alloc, sizeof( radStatModuleContainer ) );
        rAssert( module != NULL );
        module->m_Next = m_Modules;
        m_Modules = module;
        module->m_Accumulators = NULL;
        module->m_NumAccum = 0;
        strcpy( module->m_ModuleName, moduleName );
    }

    //
    // Resize the accumulator array if necessary.
    //
    if( module->m_Accumulators == NULL || module->m_NumAccum != numEntries )
    {
        if( module->m_Accumulators != NULL )
        {
            for( unsigned int i = 0; i < module->m_NumAccum; i++ )
            {
                radRelease( module->m_Accumulators[ i ], this );
            }
            radMemoryFree( s_Alloc, module->m_Accumulators );
        }
        module->m_Accumulators = ( IRadSimpleStatistic** )radMemoryAlloc( s_Alloc, numEntries * sizeof( IRadSimpleStatistic* ) );
        rAssert( module->m_Accumulators != NULL );
        for( unsigned int i = 0; i < numEntries; i++ )
        {
            IRadSimpleStatistic *stat;
            radStatCreateSimpleStatistic( &stat, 0, s_Alloc );
            module->m_Accumulators[ i ] = stat;;
            rAssert( module->m_Accumulators[ i ] != NULL );
        }
        module->m_NumAccum = numEntries;
    }

    //
    // Reset the accumulators and resize their history buffers.
    //
    for( unsigned int i = 0; i < module->m_NumAccum; i++ )
    {
        module->m_Accumulators[ i ]->Reset( numSamplesPerEntry );
        module->m_Accumulators[ i ]->SetName( "" );
    }
}


//===========================================================================
// radStatManager::TerminateModule
//===========================================================================
// Description: Shut down and delete one statistics module. This must be done
//              when a code module is finished profiling itself.
//
// Parameters:	moduleName - the name of the module to delete.
//
// Returns:     None.
//
//===========================================================================

void radStatManager::TerminateModule
(
    const char* moduleName
)
{
    //
    // Locate the module and its predecessor in the list.
    //
    bool found = false;
    radStatModuleContainer* module = m_Modules;
    radStatModuleContainer* prev = NULL;
    while( !found && module != NULL )
    {
        if( strcmp( moduleName, module->m_ModuleName ) == 0 )
        {
            found = true;
        }
        else
        {
            prev = module;
            module = module->m_Next;
        }
    }

    //
    // If we found it, unlink it from the list and
    // free its memory.
    //
    if( found )
    {
        if( prev == NULL )
        {
            m_Modules = module->m_Next;
        }
        else
        {
            prev->m_Next = module->m_Next;
        }

        if( module->m_Accumulators != NULL )
        {
            for( unsigned int i = 0; i < module->m_NumAccum; i++ )
            {
                radRelease( module->m_Accumulators[ i ], this );
            }
            radMemoryFree( s_Alloc, module->m_Accumulators );
        }
        radMemoryFree( s_Alloc, module );
    }
}


//===========================================================================
// radStatManager::ResetAccumulator
//===========================================================================
// Description: Reset and optionally resize the history buffer of a single
//              stats accumulator in a stats module.
//
// Parameters:	moduleName - the name of the module to reference.
//              index - the index of the accumulator to reset.
//              numSamplesToStore - the new history size for this accumulator.
//
// Returns:     None.
//
//===========================================================================

void radStatManager::ResetAccumulator
( 
    const char* moduleName, 
    unsigned int index, 
    unsigned int numSamplesToStore
)
{
    //
    // Locate the module. If it doesn't exist, issue a warning.
    //
    radStatModuleContainer* module = FindModule( moduleName );
    if( module == NULL )
    {
        char buf[ 512 ];
        sprintf( buf, "Statistics module %s not found.\n", moduleName );
        rAssertMsg( false, buf );
    }
    else
    {
        //
        // Found it - update the appropriate accumulator.
        //
        rAssert( module->m_Accumulators != NULL );
        if( index < module->m_NumAccum )
        {
            module->m_Accumulators[ index ]->Reset( numSamplesToStore );
        }
    }
}


//===========================================================================
// radStatManager::AddSample
//===========================================================================
// Description: Record a new sample in an accumulator.
//
// Parameters:	moduleName - the name of the module recording the data.
//              index - the number of the statistic to update.
//              sample - the value to add to the statistic.
//
// Returns:     None.
//
//===========================================================================

void radStatManager::AddSample
( 
    const char* moduleName, 
    unsigned int index, 
    float sample
)
{
    //
    // Locate the module. If it doesn't exist, issue a warning.
    //
    radStatModuleContainer* module = FindModule( moduleName );
    if( module == NULL )
    {
        char buf[ 512 ];
        sprintf( buf, "Statistics buffer for module %s not found.\n", moduleName );
        rAssertMsg( false, buf );
    }
    else
    {
        //
        // Found it - update the appropriate accumulator.
        //
        rAssert( module->m_Accumulators != NULL );
        rAssert( module->m_NumAccum > index );
        if( index < module->m_NumAccum )
        {
            module->m_Accumulators[ index ]->AddSample( sample );
        }
    }
}


//===========================================================================
// radStatManager::Query
//===========================================================================
// Description: Query a statistics accumulator about the samples it has
//              recorded so far.
//
// Constraints:	The accuracy of the standard deviation result depends on the
//              accumulator's history buffer being set large enough to 
//              record all the samples given.
//
// Parameters:	moduleName - the name of the profiling module.
//              index - the number of the accumulator to query in this module.
//
// Returns:     pNumSamples - the number of samples recorded.
//              pTotal - the sum of all the samples.
//              pMean - the average of all the samples.
//              pMax - the largest sample value.
//              pMin - the smallest sample value.
//              pStandardDeviation - the standard deviation of the samples.
//
// Notes:       All the return parameters are optional, and NULL can be given
//              for those that are not needed.
//
//===========================================================================

void radStatManager::Query
( 
    const char* moduleName, 
    unsigned int index, 
    unsigned int* pNumSamples,     
    float* pTotal, 
    float* pMean, 
    float* pMax, 
    float* pMin, 
    float* pStandardDeviation
)
{
    //
    // Locate the module. If it doesn't exist, issue a warning.
    //
    radStatModuleContainer* module = FindModule( moduleName );
    if( module == NULL )
    {
        char buf[ 512 ];
        sprintf( buf, "Statistics buffer for module %s not found.\n", moduleName );
        rAssertMsg( false, buf );
    }
    else
    {
        //
        // Found it - get the appropriate accumulator.
        //
        rAssert( module->m_Accumulators != NULL );
        if( index < module->m_NumAccum )
        {
            IRadSimpleStatistic* stat = module->m_Accumulators[ index ];

            //
            // Answer requested queries.
            //
            if( pNumSamples != NULL )
            {
                *pNumSamples = stat->GetNumSamples( );
            }

            if( pTotal != NULL )
            {
                *pTotal = stat->GetTotal( );
            }

            if( pMean != NULL )
            {
                *pMean = stat->GetMean( );
            }

            if( pMax != NULL )
            {
                *pMax = stat->GetMax( );
            }

            if( pMin != NULL )
            {
                *pMin = stat->GetMin( );
            }

            if( pStandardDeviation != NULL )
            {
                *pStandardDeviation = stat->GetStandardDeviation( );
            }
        }
    }
}


//===========================================================================
// radStatManager::QueryPercentile
//===========================================================================
// Description: Perform a percentile query of an accumulator.
//              See radSimpleStatistic for the definition of percentile.
//
// Parameters:	moduleName - the name of the module making the request.
//              index - the number of the accumulator to query.
//              percentile - a number from 1 to 99.
//
// Returns:     pNumBelow - the sample value that "percentile" percent
//                  of the samples are equal or less than.
//
//===========================================================================

void radStatManager::QueryPercentile
( 
    const char* moduleName, 
    unsigned int index,     
    unsigned int percentile, 
    float* pNumBelow
)
{
    //
    // Locate the module. If it doesn't exist, issue a warning.
    //
    radStatModuleContainer* module = FindModule( moduleName );
    if( module == NULL )
    {
        char buf[ 512 ];
        sprintf( buf, "Statistics buffer for module %s not found.\n", moduleName );
        rAssertMsg( false, buf );
    }
    else
    {
        //
        // Found it - query the appropriate accumulator.
        //
        rAssert( module->m_Accumulators != NULL );
        rAssert( pNumBelow != NULL );
        if( index < module->m_NumAccum )
        {
            *pNumBelow = module->m_Accumulators[ index ]->GetPercentile( percentile );
        }
    }
}


//===========================================================================
// radStatManager::SetAccumulatorName
//===========================================================================
// Description: Set the user-defined name of a stats accumulator.
//
// Parameters:	moduleName - the name of the module making the request.
//              index - the number of the accumulator to query.
//              accumName - the new name for the register.
//
// Returns:     None.
//
//===========================================================================

void radStatManager::SetAccumulatorName
( 
    const char* moduleName, 
    unsigned int index, 
    const char* accumName
)
{
    //
    // Locate the module. If it doesn't exist, issue a warning.
    //
    radStatModuleContainer* module = FindModule( moduleName );
    if( module == NULL )
    {
        char buf[ 512 ];
        sprintf( buf, "Statistics buffer for module %s not found.\n", moduleName );
        rAssertMsg( false, buf );
    }
    else
    {
        //
        // Found it - Update the appropriate accumulator.
        //
        rAssert( module->m_Accumulators != NULL );
        if( index < module->m_NumAccum )
        {
            module->m_Accumulators[ index ]->SetName( accumName );
        }
    }
}


//===========================================================================
// radStatManager::GetAccumulatorName
//===========================================================================
// Description: Get the user-defined name of a stats accumulator.
//
// Parameters:	moduleName - the name of the module making the request.
//              index - the number of the accumulator to query.
//
// Returns:     pAccumName - buffer to copy the name into. Will be set to
//                  the empty string if there is no matching accumulator
//                  or if it does not have a name.
//
//===========================================================================

void radStatManager::GetAccumulatorName
( 
    const char* moduleName, 
    unsigned int index, 
    char* pAccumName
)
{
    //
    // Locate the module. If it doesn't exist, issue a warning.
    //
    radStatModuleContainer* module = FindModule( moduleName );
    if( module == NULL )
    {
        char buf[ 512 ];
        sprintf( buf, "Statistics buffer for module %s not found.\n", moduleName );
        rAssertMsg( false, buf );
    }
    else
    {
        //
        // Found it - Update the appropriate accumulator.
        //
        rAssert( module->m_Accumulators != NULL );
        if( index < module->m_NumAccum )
        {
            module->m_Accumulators[ index ]->GetName( pAccumName );
        }
    }
}


//===========================================================================
// radStatManager::EnumerateAccumulators
//===========================================================================
// Description: This function enumerates modules and accumulators. For each
//              accumulator in each module matching given wildcard patterns,
//              it invokes given callback function.
//
// Parameters:	modulePattern - wildcard pattern to match module names against.
//              accumPattern - wildcard pattern to match accumulator names against.
//              enumerationCallback - function to call with each match.
//
// Returns:     None.
//
// Notes:       Enumeration is implemented via a direct callback function 
//              instead of via an interface to avoid polluting class
//              definitions with ifdefs. This way a class doesn't need to
//              ifdef the list of interfaces it implements.
//
//===========================================================================
void radStatManager::EnumerateAccumulators
( 
    const char* modulePattern, 
    const char* accumPattern,                       
    void (*enumerationCallback)( const char* moduleName, const char* accumName )
)
{
    rAssert( enumerationCallback != NULL );
    rAssert( modulePattern != NULL );
    rAssert( accumPattern != NULL );

    //
    // Loop over all known modules.
    //
    radStatModuleContainer* module = m_Modules;
    while( module != NULL )
    {
        if( radStringMatchesWildCardPattern( module->m_ModuleName, modulePattern ) )
        {
            //
            // If the module name matches, loop over all its accumulators.
            //
            for( unsigned int i = 0; i < module->m_NumAccum; i++ )
            {
                char accumName[ 256 ];
                module->m_Accumulators[ i ]->GetName( accumName );

                //
                // If the accumulator name matches, invoke the callback.
                //
                if( radStringMatchesWildCardPattern( accumName, accumPattern ) )
                {
                    (*enumerationCallback)( module->m_ModuleName, accumName );
                }
            }
        }
        module = module->m_Next;
    }    
}


#endif                                  // PROFILE
