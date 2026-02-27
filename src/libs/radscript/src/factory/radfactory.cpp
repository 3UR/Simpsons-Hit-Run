//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include <radobject.hpp>
#include <radstring.hpp>
#include <radfactory.hpp>

//============================================================================
// Globals
//============================================================================

static int g_RadFactoryInitializeCount = 0;
static radMemoryAllocator g_RadFactoryAllocator = RADMEMORY_ALLOC_DEFAULT;

//============================================================================
// Globals
//============================================================================

struct radFactoryNode
    :
    public radObject
{
    IMPLEMENT_BASEOBJECT( "radFactoryNode" )

    radFactoryNode
    (
        radKey32 key,
        radFactoryProc * pFactoryProc,
        radFactoryNode * pNext
    )
        :
        m_RadKey( key ),
        m_pFactoryProc( pFactoryProc ),
        m_pNext( pNext )
    {
        rAssert( pFactoryProc != NULL );
    }

    radKey32                 m_RadKey;
    radFactoryProc *         m_pFactoryProc;
    
    radFactoryNode * m_pNext;
};

static radFactoryNode * g_pScriptableHead;

//============================================================================
// Function: ::radFactoryInitialize
//============================================================================

void radFactoryInitialize( radMemoryAllocator allocator)
{
    g_RadFactoryAllocator = allocator;

    if ( g_RadFactoryInitializeCount == 0 )
    {
        rAssert( g_pScriptableHead == NULL );
    }

    g_RadFactoryInitializeCount++;
}

//============================================================================
// Function: ::radFactoryTerminate
//============================================================================

void radFactoryTerminate( void )
{
    rAssert( g_RadFactoryInitializeCount > 0 );

    g_RadFactoryInitializeCount--;

    if ( g_RadFactoryInitializeCount == 0 )
    {
        radFactoryNode * pSearch = g_pScriptableHead;
        radFactoryNode * pTemp = NULL;

        while( pSearch != NULL )
        {
            pTemp = pSearch;
            pSearch = pSearch->m_pNext;
            delete pTemp;
        }

        g_pScriptableHead = NULL;
    }
        
}

//============================================================================
// Function: ::radFactoryRegister
//============================================================================

void radFactoryRegister( const char * pStringName, radFactoryProc * pFactory )
{
    rAssert( g_RadFactoryInitializeCount > 0 );

    radFactoryRegister( radMakeKey32( pStringName ), pFactory );
}

//============================================================================
// Function: ::radFactoryRegister
//============================================================================

void radFactoryRegister( radKey32 key, radFactoryProc * pFactory )
{
    rAssert( g_RadFactoryInitializeCount > 0 );

    radFactoryNode * pNew = new ( g_RadFactoryAllocator ) radFactoryNode( key, pFactory, g_pScriptableHead );
    g_pScriptableHead = pNew;    
}

//============================================================================
// Function: ::radFactoryCreateInstance
//============================================================================

void radFactoryCreateInstance(  const char * pStringName, IRefCount ** ppIRefCount, radMemoryAllocator allocator )
{
    rAssert( g_RadFactoryInitializeCount > 0 );

    radFactoryCreateInstance( radMakeKey32( pStringName ), ppIRefCount, allocator );

    if ( *ppIRefCount == NULL )
    {
        rDebugString( "radFactory: Failed to create instance: [" );
        rDebugString( pStringName );
        rDebugString( "]\n" );
    }
}

//============================================================================
// Function: ::radFactoryCreateInstance
//============================================================================

void radFactoryCreateInstance( radKey32 key,IRefCount ** ppIRefCount, radMemoryAllocator allocator )
{
    rAssert( g_RadFactoryInitializeCount > 0 );
    rAssert( ppIRefCount != NULL );   

    radFactoryNode * pSearch = g_pScriptableHead;

    while( pSearch != NULL )
    {
        if ( pSearch->m_RadKey == key )
        {
            if ( pSearch->m_pFactoryProc != NULL )
            {
                *ppIRefCount = pSearch->m_pFactoryProc( allocator );                
               
                if ( *ppIRefCount != NULL )
                {
                    (*ppIRefCount)->AddRef( );
                }
                
                return;              
                                
            }
        }

        pSearch = pSearch->m_pNext;
    }

    *ppIRefCount = NULL;
    
}