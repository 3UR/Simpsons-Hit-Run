//===========================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   profiler
//
// Description: This file contains the implementation of the 
//              profiler.
//
// Authors:     James Tan       July 29th
//
//===========================================================================

//---------------------------------------------------------------------------
// Includes
//===========================================================================
#include "pch.hpp"
#include "profiler.hpp"
#include "profilesample.hpp"
#include <string.h>

#ifdef RADPROFILER

//===========================================================================
// radProfiler::GetLastUnClosedProfile
//===========================================================================
// Description: return the last unclosed profile sample
//
// Constraints: Not yet
//
// Parameters:  None
//
// Return:      radProfileSample - last unclosed profile sample
//
//===========================================================================
radProfileSample * radProfiler::GetLastUnClosedProfile( )
{
    return m_pLastUnClosedProfile;
}

//===========================================================================
// radProfiler::FindProfileSample
//===========================================================================
// Description: find a sample node by name
//
// Constraints: Not yet
//
// Parameters:  None
//
// Return:      radProfileSample - found profile sample, else NULL
//
//===========================================================================
radProfileSample * radProfiler::FindProfileSample( const char * pProfileName )
{
    int nIndex = BinarySearchSampleProfileArrayByHashedName( radMakeKey( pProfileName ) );

    if ( nIndex == -1 )
    {
        //
        // cannot find the entry
        //
        return NULL;
    }
    else
    {
        //
        // must be in the range of the size
        //
        rAssert( nIndex < (int)m_uProfileSampleArraySize );
        return m_pProfileSampleArrayByHashedName[ nIndex ].pSample;
    }
}

//===========================================================================
// radProfiler::InsertChildProfileSample
//===========================================================================
// Description: insert a child profile sample node into the tree
//
// Constraints: Not yet
//
// Parameters:  pProfileName - name of the sample
//              pParentProfileSample - parent profile sample
//
// Return:      radProfileSample - newly inserted profile sample
//
//===========================================================================
radProfileSample * radProfiler::InsertChildProfileSample( const char * pProfileName, radProfileSample * pParentProfileSample )
{
    //------------------------------------------------------------------------
    // Step 1, create a sample node, and insert into array and sort the array.
    //========================================================================

    //
    // allocate additional memory for profiler
    //
    AllocateMemoryForProfileSampleArrayAndCopyOldData( m_uProfileSampleArraySize + 1 );

    //
    // add new entry into the profile sample pointer array
    //
    m_pProfileSampleArrayByHashedName[ m_uProfileSampleArraySize - 1 ].HashedName = radMakeKey( pProfileName );

    radRef< radProfileSample > pProfileSample = radProfileSampleCreate( m_alloc );
    rAssertMsg( pProfileSample, "radProfiler: Error: Cannot create profile sample instance. ???Outof memory???" );

    m_pProfileSampleArrayByHashedName[ m_uProfileSampleArraySize - 1 ].pSample = pProfileSample;

    //
    // prev node is the m_pLastClosedProfile if (m_pLastClosedProfile and pProfileSample)
    // has the same parent.
    //
    radProfileSample * pPrevProfileSample = NULL;

    if ( m_pLastUnClosedProfile != NULL && m_pLastClosedProfile != NULL )
    {
        if ( m_pLastClosedProfile->GetParentNode( ) == m_pLastUnClosedProfile )
        {
            pPrevProfileSample = m_pLastClosedProfile;
        }
    }

    pProfileSample->Initialize( pProfileName, pParentProfileSample, pPrevProfileSample );

    //
    // sort array by hashed name, and check for hashed name duplicates
    //
    SortSampleProfileArrayByHashedName( );

    //------------------------------------------------------------------------
    // Step 2, see if the insertion is a special case, if is, do things we need.
    //========================================================================

    //
    // if there's no root profile sample, then this is the root profile sample.
    //
    if ( m_pRootProfileSample == NULL )
    {
        rAssert( pParentProfileSample == NULL );

        m_pRootProfileSample = pProfileSample;
    }

    return pProfileSample;
}

//===========================================================================
// radProfiler::EndFrameForAllProfileNode
//===========================================================================
// Description: end frame for all profile node in the tree
//
// Constraints: Not yet
//
// Parameters:
//
// Return:     None
//
//===========================================================================
void radProfiler::EndFrameForAllProfileNode( radTime64 uTotalFrameTimeInMicroSec, radTime64 uAveFrameTimeInMicroSec, radTime64 uThisFrameTimeInMicroSec, unsigned int uFrameCount )
{
    for ( unsigned int i = 0; i < m_uProfileSampleArraySize; i++ )
    {
        m_pProfileSampleArrayByHashedName[ i ].pSample->EndFrame( uTotalFrameTimeInMicroSec, uAveFrameTimeInMicroSec, uThisFrameTimeInMicroSec, uFrameCount );
    }
}


//===========================================================================
// radProfiler::AllocateMemoryForProfileSampleArrayAndCopyOldData
//===========================================================================
// Description: help function to allocate memory for profile sample array
//
// Constraints: It may not allocate memory, allocated memory is rounded to
//              nearest 16 entry, so we don't have to do allocation everytime.
//
// Parameters:  uSize - size of the new array
//
// Return:      None
//
//===========================================================================
void radProfiler::AllocateMemoryForProfileSampleArrayAndCopyOldData( unsigned int uSize )
{
    //
    // we always allocate extra memory to fit unit of 16 entry
    //
    unsigned int uRoundedSize = ( uSize / 16 + 1 ) * 16;

    //
    // if memory we have is sufficient, don't allocate extra memory
    //
    if ( m_uProfileSampleMemorySize >= uRoundedSize )
    {
        m_uProfileSampleArraySize = uSize;
        return;
    }
    else    // allocate memory with rounded size
    {
        ProfileSamplePair * pNewSampleArrayMemory = static_cast< ProfileSamplePair * >( radMemoryAlloc( m_alloc, uRoundedSize * sizeof( ProfileSamplePair ) ) );
        memset( pNewSampleArrayMemory, 0, sizeof( ProfileSamplePair ) * uRoundedSize );

        if ( m_pProfileSampleArrayByHashedName != NULL )
        {
            memcpy( pNewSampleArrayMemory, m_pProfileSampleArrayByHashedName, sizeof( ProfileSamplePair ) * m_uProfileSampleMemorySize );
            radMemoryFree( m_pProfileSampleArrayByHashedName );
        }

        m_pProfileSampleArrayByHashedName = pNewSampleArrayMemory;
        m_uProfileSampleArraySize = uSize;
        m_uProfileSampleMemorySize = uRoundedSize;
    }
}

//===========================================================================
// radProfiler::SortSampleProfileArrayByHashedName
//===========================================================================
// Description: help function to sort array of profile sample by hashed name.
//
// Constraints: Not yet.
//
// Parameters:  None
//
// Return:      None
//
//===========================================================================
void radProfiler::SortSampleProfileArrayByHashedName( )
{
    //
    // little bubble-bubble sort
    //
    ProfileSamplePair tmp;
    
    for ( unsigned int i=0; i < m_uProfileSampleArraySize - 1; i++ )
    {
        for ( unsigned int j=0; j < m_uProfileSampleArraySize - 1 - i; j++ )
        {
            if ( m_pProfileSampleArrayByHashedName[ j + 1 ].HashedName < m_pProfileSampleArrayByHashedName[ j ].HashedName )
            {
                tmp = m_pProfileSampleArrayByHashedName[ j ];
                m_pProfileSampleArrayByHashedName[ j ] = m_pProfileSampleArrayByHashedName[ j + 1 ];
                m_pProfileSampleArrayByHashedName[ j + 1 ] = tmp;
            }
        }
    }
}

//===========================================================================
// radProfiler::BinarySearchSampleProfileArrayByHashedName
//===========================================================================
// Description: help function to find a sample node by hash name.
//
// Constraints: Not yet.
//
// Parameters:  None
//
// Return:      the index of the sample node, if not found return -1
//
//===========================================================================
int radProfiler::BinarySearchSampleProfileArrayByHashedName( radKey HashedName )
{
    if ( m_pProfileSampleArrayByHashedName == NULL || m_uProfileSampleArraySize == 0 )
    {
        return -1;
    }

    //
    // lovely binary search
    //
    unsigned int uBottom = 0;
    unsigned int uTop = m_uProfileSampleArraySize - 1;
    bool bFound = false;
    unsigned int uMid = 0;
    while( ( uTop >= uBottom ) && ( ! bFound ) )
    {
        uMid = ( uTop + uBottom ) / 2;
        if ( m_pProfileSampleArrayByHashedName[ uMid ].HashedName == HashedName )
        {
            bFound = true;
        }
        else if ( HashedName > m_pProfileSampleArrayByHashedName[ uMid ].HashedName )
        {
            if ( uBottom >= m_uProfileSampleArraySize )
            {
                return -1;
            }
            uBottom = uMid + 1;
        }
        else
        {
            if ( uTop == 0 )
            {
                return -1;
            }
            uTop = uMid - 1;
        }
    }

    if ( bFound )
    {
        return uMid;
    }
    else
    {
        return -1;
    }
}
#endif // RAD_PROFILE
