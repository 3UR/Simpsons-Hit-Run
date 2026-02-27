//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include <radscript.hpp>

#include <string.h>
#include <radremotecommand.hpp>
#include <radobject.hpp>

struct radRemoteScript
	:
	public IRefCount,
	public radRefCount
{
	IMPLEMENT_REFCOUNTED( "radRemoteScript" )
       
	radRemoteScript( radMemoryAllocator allocator )
        :
        m_ThisAllocator( allocator )
	{
        s_pTheRemoteScript = this;

        m_xIRadScript = ::radScriptCreateScript( m_ThisAllocator );
        
		radRemoteCommandInitialize( allocator );
		m_xIRemoteCommand = ::radRemoteCommandGet( );

		m_xIRemoteCommand->RegisterRemoteFunction( "rs", RemoteFunction, (void*) this );
	}
    
    HrcsResultCode Execute( const char * pObjectName, const char * pMemBlock, unsigned int size )
    {
           
        radRef< IRadNameSpace > m_xIRadNameSpace = ::radNameSpaceFindObjectNameSpace( pObjectName );

        if ( m_xIRadNameSpace != NULL )
        {
            m_xIRadScript->SetContext( m_xIRadNameSpace );
            m_xIRadScript->Load( pMemBlock, size );
            m_xIRadScript->Run( );
            m_xIRadScript->UnLoad( );

            return HrcsSuccess;
        }

        rDebugString ( "\nradScript: Abort remote call, unable to find object [" );
        rDebugString ( pObjectName );
        rDebugString ( "] in NameSpace.\n" );
        return HrcsFail;
    }

	static HrcsResultCode RemoteFunction( int argc, char * argv[], void* pUserData )
	{
        HrcsResultCode hrc = HrcsFail;

        radRemoteScript * pThis = (radRemoteScript*) pUserData;

        if ( argc >= 2 )
        {
            char pObjectName[ 256 ];
            strcpy( pObjectName, argv[ 3 ] );
            //
            // Turn the script into one long block.
            //
            unsigned int memlength = 0;

            for( int i = 0; i < argc; i ++ )
            {
                memlength += strlen( argv[ i ] ) + 1;
            }

            char * pMemBlock = (char*) radMemoryAlloc( pThis->m_ThisAllocator, memlength );

            char  * pIp = pMemBlock;

            for( int j = 0; j < argc; j ++ )
            {
                ::memcpy( pIp, argv[ j ], strlen( argv[ j ] ) + 1 ) ;

                pIp += strlen( argv[ j ] ) + 1;
            }

            hrc =pThis->Execute( pObjectName, pMemBlock, memlength );

            radMemoryFree( pThis->m_ThisAllocator, pMemBlock );
        }
        else
        {
            rAssertMsg( false, "Bad args to remote script." );
        }
 

        return hrc;        
	}

	~radRemoteScript( void )
	{
        s_pTheRemoteScript = NULL;

        m_xIRemoteCommand->UnRegisterRemoteFunction( "rs" );
        m_xIRemoteCommand = NULL;
		radRemoteCommandTerminate( );
	}

    radMemoryAllocator m_ThisAllocator;

	radRef< IRadRemoteCommand > m_xIRemoteCommand;
    radRef< IRadScript >            m_xIRadScript;

    static radRemoteScript * s_pTheRemoteScript;
};

radRemoteScript * radRemoteScript::s_pTheRemoteScript = NULL;

void radRemoteScriptInitialize( radMemoryAllocator allocator )
{
    rAssert( radRemoteScript::s_pTheRemoteScript == NULL );

    new ( allocator ) radRemoteScript( allocator );
}

void radRemoteScriptTerminate( void )
{
    rAssert( radRemoteScript::s_pTheRemoteScript != NULL );

    radRemoteScript::s_pTheRemoteScript->Release( );  
}
