//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//============================================================================
// Include Files
//============================================================================

#include "pch.hpp"

#include <radtypeinfo.hpp>

#if defined( RAD_WIN32 ) || defined( RAD_UWP )
#include "win32\win32typeinfovfcall.h"
#endif

//============================================================================
// Component: IRadTypeInfoMethod
//============================================================================

//========================================================================
// IRadTypeInfoMethod::Invoke
//========================================================================
unsigned int IRadTypeInfoMethod::Invoke
(
    void * pI,
    void * pParams,
    unsigned int numParams
) 
{
    if ( pI == NULL )
    {
       rDebugString( "IRadTypeInfoMethod: Invoke() called on NULL object!\n" ); 
    }
    else
    {
        if ( numParams != m_lstParamsPtr->GetSize( ) )
        {
            rDebugString( "IRadTypeInfoMethod: [" );
            rDebugString( GetName( ) );
            rDebugString( "] was called incorrectly!\n" );
        }
        else
        {
            //
            //  ---- RAD_WIN32 or RAD_UWP ----------------------------------------------------
            // 
            #if defined( RAD_WIN32 ) || defined( RAD_UWP )

                return InvokeVf( pI, m_VTableOffset, pParams, numParams );

            #endif 
       }
   }
   
   return 0; 
}


