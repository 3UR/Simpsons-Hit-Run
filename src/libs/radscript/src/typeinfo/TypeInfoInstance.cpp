//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include <radtypeinfoutil.hpp>

#include <radobject.hpp>


//============================================================================
// Component: RadTypeInfoInstanceMethod
//============================================================================

struct radTypeInfoInstanceMethod
    :
    public IRadTypeInfoInstanceMethod,
    public radRefCount
{

    //========================================================================
    // Data Members
    //========================================================================

    radRef< IRefCount > m_xIRefCount;
    radRef< IRadTypeInfoMethod > m_xIRadTypeInfoMethod;

    IMPLEMENT_REFCOUNTED( "RadTypeInfoInstanceMethod" )

    //========================================================================
    // RadTypeInfoInstanceMethod::radTypeInfoInstanceMethod
    //========================================================================

    radTypeInfoInstanceMethod( IRefCount * pIRefCount, const char * pMethodName )
        :
        radRefCount( 0 ),
        m_xIRefCount( pIRefCount )
    {
        m_xIRadTypeInfoMethod = ::radTypeInfoSystemGet( )->GetTypeInfoMethod( pIRefCount, pMethodName );
    }

    //========================================================================
    // RadTypeInfoInstanceMethod::Invoke
    //========================================================================

    virtual int Invoke( void * pParams, unsigned int numParams )
    {
        return m_xIRadTypeInfoMethod->Invoke( m_xIRefCount.m_pInterface, pParams, numParams );
    }

    //========================================================================
    // RadTypeInfoInstanceMethod::GetObject
    //========================================================================

    virtual IRefCount * GetObject( void )
    {
        return m_xIRefCount;
    }

	//========================================================================
    // RadTypeInfoInstanceMethod::GetMethod
    //========================================================================
	
	virtual const char * GetMethodName( void )
	{
		return m_xIRadTypeInfoMethod->GetName( );
	}

    virtual unsigned int GetMethodHashedName( void )
    {
        return m_xIRadTypeInfoMethod->GetHashedName( );
    }
};

//============================================================================
// ::rfcCreateTypeInfoInstanceMethod
//============================================================================

IRadTypeInfoInstanceMethod * radTypeInfoCreateInstanceMethod( IRefCount * pIRefCount, const char * pMethodName, unsigned int allocator )
{
    return new( allocator ) radTypeInfoInstanceMethod( pIRefCount, pMethodName );

}
