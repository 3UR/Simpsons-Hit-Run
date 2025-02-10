//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radobject.cpp
//
// Subsystem:   Fountation Technologies Base Class Implementations
//
// Description: This file contains the implementation of the radical object
//              base class. Provides object tracking in a debug build.
//
// Revisions:   V1.00 Mar 9, 2001       Creation
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <radobject.hpp>
#include <radmemory.hpp>
#include <radmemorymonitor.hpp>
#include <typeinfo>
#include <string.h>
#include <radthread.hpp>

//=============================================================================
// Implementation
//=============================================================================
const char * const g_nameFTech = "FTech";

radMemoryAllocator radObject::s_Allocator = -1;

#ifdef RAD_DEBUG

radBaseObject * radBaseObject::s_pRadBaseObjectHead = NULL;

void radBaseObject::DumpObjects( void )
{
	rDebugString( "\nradObject Dump:\n" );
	rDebugString(   "---------------\n" );

    char className[ 256 ];
    strcpy( className, " " );
	unsigned int totalMemoryCountDebug = 0;
	unsigned int totalMemoryCountRelease = 0;
	unsigned int totalObjectCount = 0;
    bool leaksFound = s_pRadBaseObjectHead != NULL;        

    while( 1 )
    {   
        // Find the class which is next alphabetically.

        radBaseObject * pSearch = s_pRadBaseObjectHead;
        radBaseObject * pLower = NULL;
        
        while ( pSearch != NULL )
        {
            // if this class is greater that the previous one we printed

            if ( strcmp( typeid( *pSearch ).name( ), className ) > 0 )
            {
                // AND lower than the last search item, it is our new candiate for
                // summarizing.

                if ( pLower == NULL || strcmp( typeid( *pLower ).name( ), typeid( *pSearch ).name( ) ) > 0 )
                {
                    pLower = pSearch;
                }
            }
            
            // Keeping going until we've found the next lexigraphical class name
               
            pSearch = pSearch->m_pRadBaseObjectNext;
        }

        // Now search for all classes of this type, and tally up the info.

        if ( pLower != NULL )
        {
            strcpy( className, typeid( *pLower ).name( ) );

            unsigned int classObjectCount = 0;
            unsigned int classObjectSizeDebug = 0;
            unsigned int classObjectSizeRelease = 0;
            unsigned int classSizeDebug = pLower->GetObjectSize( );
            unsigned int classSizeRelease = classSizeDebug - 12;

            radBaseObject * pTallySearch = s_pRadBaseObjectHead;

            while ( pTallySearch != NULL )
            {
		        const char * pClassName = typeid( *pTallySearch ).name( );

                if ( strcmp( pClassName, className ) == 0 )
                {
                    classObjectCount ++;
                    classObjectSizeDebug += classSizeDebug;
                    classObjectSizeRelease += classSizeRelease;
                    totalObjectCount ++;
                    totalMemoryCountDebug += classSizeDebug;
                    totalMemoryCountRelease += classSizeRelease;
                }

                pTallySearch = pTallySearch->m_pRadBaseObjectNext;
            }

            //
            // Print Results for this class type
            //

		    if ( classSizeDebug == 0xFFFFFFFF )
		    {
			    rDebugPrintf( "    Class: [%48s] Count: [%5d] No Size Information!\n",
			             className,
					     classObjectCount );
		    }
		    else
		    {
			    rDebugPrintf( "    Class: [%48s] Count: [%5d] Bytes: [%5d(Debug),%5d(Release)] TotalBytes: [%5d(Debug),%5d(Release)]\n",
				    className,
				    classObjectCount,
				    classSizeDebug,
                    classSizeRelease,
				    classObjectSizeDebug,
                    classObjectSizeRelease );
	        }
        }
        else
        {
            if ( leaksFound == false )
            {
                rDebugPrintf( "    No leaks detected.\n" );
            }

            // print total results and end.

	        rDebugString( "\n" );
	        rDebugString( "radObject Dump Summary:\n" );
	        rDebugString( "---------------------\n" );
	        
	        rDebugPrintf( "    Total number of objects: [%d].\n", totalObjectCount );
	        rDebugPrintf( "    Total number of byte allocated for debug: [%d].\n", totalMemoryCountDebug );
	        rDebugPrintf( "    Total number of byte allocated for release: [%d].\n\n", totalMemoryCountRelease );

            break;
        }
    }
}

void radBaseObject::AddToList( void )
{
    m_pRadBaseObjectNext = s_pRadBaseObjectHead;
    if ( m_pRadBaseObjectNext != NULL )
    {
        m_pRadBaseObjectNext->m_pRadBaseObjectPrev = this;
    }
    m_pRadBaseObjectPrev = NULL;
    s_pRadBaseObjectHead = this;

}

void radBaseObject::RemoveFromList( void )
{
    if ( m_pRadBaseObjectPrev != NULL )
    {
        m_pRadBaseObjectPrev->m_pRadBaseObjectNext = m_pRadBaseObjectNext;
    }
    else
    {
        s_pRadBaseObjectHead = m_pRadBaseObjectNext;
    }

    if ( m_pRadBaseObjectNext != NULL )
    {
        m_pRadBaseObjectNext->m_pRadBaseObjectPrev = m_pRadBaseObjectPrev;
    }
}

size_t radBaseObject::GetObjectSize( void )
{
	return 0xFFFFFFFF;	
}

void radBaseObject::Dump( char* pStringBuffer, unsigned int bufferSize )
{
	strcpy( pStringBuffer, "Unknown!" );
}

#endif

#if defined RADMEMORYMONITOR
void radMemoryMonitorIdentifyAllocationAdaptor( void * address, const char * group, const char * name, unsigned int* pReferenceCount )
{
    radMemoryMonitorIdentifyAllocation( address, group, name, pReferenceCount );
}

void radMemoryMonitorReportAddRefAdaptor( void* pReference, void* pObject )
{
    radMemoryMonitorReportAddRef( pReference, pObject );
}

void radMemoryMonitorReportReleaseAdaptor( void* pReference, void* pObject )
{
    radMemoryMonitorReportRelease( pReference, pObject );
}
#endif