//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include <radobject.hpp>
#include <radobjectlist.hpp>

#include <radtypeinfo.hpp>
#include <radtypeinfoutil.hpp>
#include <radkey.hpp>
//============================================================================
// Component: radTypeInfoDistributor
//============================================================================

struct radTypeInfoDistributor
    :
    public IRadTypeInfoDistributor,
    public radRefCount
{

    IMPLEMENT_REFCOUNTED( "radTypeInfoDistributor" )

    //========================================================================
    // Data Members
    //========================================================================

    radMemoryAllocator m_ThisAllocator;

    radRef< IRadObjectList > m_xIOl_InstanceMethods;

    //========================================================================
    // radTypeInfoDistributor::radTypeInfoDistributor
    //========================================================================

    radTypeInfoDistributor( radMemoryAllocator allocator )
        :
        radRefCount( 0 ),
        m_ThisAllocator( allocator )
    {
        ::radObjectListCreate( & m_xIOl_InstanceMethods, m_ThisAllocator );        
    }

    //========================================================================
    // radTypeInfoDistributor::radTypeInfoDistributor
    //========================================================================

    ~radTypeInfoDistributor( void )
    {
        // rAssert( m_xIOl_InstanceMethods->GetSize( ) == 0 );
    }

    //========================================================================
    // radTypeInfoDistributor::AddObject
    //========================================================================

    virtual void AddObject( IRefCount * pIRefCount, const char * pMethodName )
    {
        rAssert( pIRefCount != NULL );
        rAssert( pMethodName != NULL );

        radRef< IRadTypeInfoInstanceMethod > xITiim;

        xITiim = ::radTypeInfoCreateInstanceMethod( pIRefCount, pMethodName, m_ThisAllocator );

        if ( xITiim != NULL )
        {
            m_xIOl_InstanceMethods->AddObject( xITiim ); 
        }
        else
        {
            rDebugString( "radTypeInfoDistributor - Interface has no such method: [" );
            rDebugString( pMethodName );
            rDebugString( "]\n" );
        }       
    }

    //========================================================================
    // radTypeInfoDistributor::RemoveObject
    //========================================================================

    virtual void RemoveObject( IRefCount * pIObject, const char * pPropery )
    {
        radRef< IRefCount >                 xIRefCount;

        IRadTypeInfoInstanceMethod * pIRadTypeInfoInstanceMethod;
        for( unsigned int i = 0; i < m_xIOl_InstanceMethods->GetSize( ); i ++ )
        {
            pIRadTypeInfoInstanceMethod = static_cast< IRadTypeInfoInstanceMethod * >( m_xIOl_InstanceMethods->GetAt( i ) );

            if
			(
				( pIRadTypeInfoInstanceMethod->GetObject( ) == pIObject ) &&
                ( pIRadTypeInfoInstanceMethod->GetMethodHashedName( ) == ::radMakeKey32( pPropery ) )
			)
            {
                m_xIOl_InstanceMethods->RemoveObject( pIRadTypeInfoInstanceMethod );
                return;
            }
        }

        rDebugString( "radTypeInfoDistributor - Can't find object to remove it\n" );
    }

    //========================================================================
    // radTypeInfoDistributor::Invoke
    //========================================================================

    virtual void Invoke( void * pParams, unsigned int numParams )
    {
        IRadTypeInfoInstanceMethod * pIRadTypeInfoInstanceMethod;

        for( unsigned int i = 0; i < m_xIOl_InstanceMethods->GetSize( ); i ++ )
        {
            pIRadTypeInfoInstanceMethod = static_cast< IRadTypeInfoInstanceMethod * >( m_xIOl_InstanceMethods->GetAt( i ) );
            
            pIRadTypeInfoInstanceMethod->Invoke( pParams, numParams );
        }
    }
};

//============================================================================
// ::radTypeInfoCreateDistributor
//============================================================================

IRadTypeInfoDistributor * radTypeInfoCreateDistributor( radMemoryAllocator allocator )
{
    return new( allocator ) radTypeInfoDistributor( allocator );
}
