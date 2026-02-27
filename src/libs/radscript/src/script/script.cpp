//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include <radscript.hpp>
#include <radtypeinfo.hpp>
#include <raddispatch.hpp>
#include <raddebug.hpp>
#include <radstring.hpp>
#include <radmemory.hpp>

#include <stdlib.h>

#include <radobject.hpp>
#include <radfile.hpp>

#ifdef RADSCRIPT_DEBUG
static char dc[] = "IRadScript";
#endif

//============================================================================
// Constants
//============================================================================


//========================================================================
// IRadScript::IRadScript
//========================================================================

IRadScript::IRadScript( radMemoryAllocator allocator )
    :
    radRefCount( 0 ),
    m_ScriptAllocator( RADMEMORY_ALLOC_DEFAULT ),
	m_WarningLevel( radScriptWarningLevelHigh ),
    m_FileSize( 0 ),
    m_pFileMemory( NULL ),
    // SIMPSONS2
    m_pObjCreateCallback( NULL ),

    m_pCurrObjectName( NULL ),
    m_pCurrClassName( NULL ),
    m_pCurrMethodName( NULL ),
    m_nCurrMethodParamSize( 0 ),
    m_nCurrPlatformOptionStackSize( 0 ),
    m_szCurrToken( NULL )
{
}

//========================================================================
// IRadScript::~Scripts
//========================================================================

IRadScript::~IRadScript( void )
{
    radMemoryFreeAligned( m_pFileMemory );
    m_pFileMemory = NULL;
    m_FileSize = 0;
}

//========================================================================
// IRadScript::Load
//========================================================================

void IRadScript::Load( const char * pMemory, unsigned int size )
{
    rAssert( m_pFileMemory == NULL );

    m_FileSize = size + 1;

    //total Hack 6 is the FE heap
    rReleaseAssert( m_pFileMemory == NULL );
    m_pFileMemory = (char*) radMemoryAllocAligned( 6, m_FileSize, 128 );

    ::memcpy( m_pFileMemory, pMemory, m_FileSize - 1 );

    m_pFileMemory[ m_FileSize - 1 ] = '\0';
}

//========================================================================
// IRadScript::UnLoad
//========================================================================

void IRadScript::UnLoad( void )
{
    rWarning( m_pFileMemory != NULL );

    if ( m_pFileMemory != NULL )
    {
        radMemoryFreeAligned( m_pFileMemory );

        m_pFileMemory = NULL;
        m_FileSize = NULL;
    }
}

void IRadScript::SetWarningLevel( radScriptWarningLevel warningLevel )
{
	m_WarningLevel = warningLevel;
}

//========================================================================
// IRadScript::AttemptMatch
//========================================================================


//========================================================================
// IRadScript::MustMatch
//========================================================================


//========================================================================
// IRadScript::NextToken
//========================================================================


//========================================================================
// IRadScript::ReadToken
//========================================================================

//========================================================================
// IRadScript::IsFloat
//========================================================================



//========================================================================
// IRadScript::IsInt
//========================================================================



//========================================================================
// IRadScript::SplitOption
//      split PC|XBOX into separated option PC XBOX enumeration
//========================================================================


//========================================================================
// IRadScript::IsCurrentPlatForm
//========================================================================



//========================================================================
// IRadScript::OnBeginObject
//========================================================================



//========================================================================
// IRadScript::OnEndObject
//========================================================================



//========================================================================
// IRadScript::ParamTypeCheck
//      Debug version only, release version always return true.
//========================================================================



//========================================================================
// IRadScript::OnBeginMethod
//========================================================================


//========================================================================
// IRadScript::OnEndMethod
//========================================================================


//========================================================================
// IRadScript::ParseObject
//========================================================================


//========================================================================
// IRadScript::SetContext
//========================================================================
void IRadScript::SetContext( IRadNameSpace * pIPersistObjectGroup2 )
{
    m_xIRadNameSpace_Context = pIPersistObjectGroup2;
}

IRadNameSpace * IRadScript::GetContext( void )
{
	return m_xIRadNameSpace_Context;
}

//========================================================================
// IRadScript::SetAllocator
//========================================================================

void IRadScript::SetAllocator( radMemoryAllocator allocator )
{
    m_ScriptAllocator = allocator;
}

//========================================================================
// IRadScript::Load
//========================================================================

void IRadScript::Load
(
    const char * pFileName,
    ScriptLoadCallback * pCallback,
    void * pCallbackUserData,
    radMemoryAllocator allocator
)
{
    //rDebugChannel( dc, "Loading IRadScript: [" );
    //rDebugChannel( dc, pFileName );
    //rDebugChannel( dc, "]\n" );

    rAssert( pCallback != NULL );

    m_pScriptLoadCallback = pCallback;
    m_pScriptLoadCallbackUserData = pCallbackUserData;

    //
    // Get the script and read it
    //

    radFileOpen( &m_xIRadFile, pFileName, false, OpenExisting, NormalPriority, 0, allocator );
    
    //
    // Get the size of the file
    //

    m_xIRadFile->GetSizeAsync( &m_FileSize );
    m_ScriptLoadState = GetFileSize;

    m_xIRadFile->AddCompletionCallback( this, (void*) & m_ScriptLoadState );
}

//========================================================================
// IRadScript::LoadSync
//========================================================================
void IRadScript::LoadSync
(
    const char * pFileName,
    radMemoryAllocator allocator
)
{

    //rDebugChannel( dc, "Loading IRadScript: [" );
    //rDebugChannel( dc, pFileName );
    //rDebugChannel( dc, "]\n" );

    //
    // Get the script and read it
    //

    radFileOpen( &m_xIRadFile, pFileName, false, OpenExisting, NormalPriority, 0, RADMEMORY_ALLOC_TEMP );
    
    //
    // Ask and wait for the size
    //

    m_xIRadFile->GetSizeAsync( &m_FileSize );
    m_xIRadFile->WaitForCompletion( );

    //
    // Prepare the buffer and read the script. Align it to 128 to improve performance
    //
    //total Hack 6 is the fe heap
    rReleaseAssert( m_pFileMemory == NULL );
    m_pFileMemory = (char*) radMemoryAllocAligned( 6, m_FileSize + 1, 128 ); // + 1 for adding last '\0'
    m_pFileMemory[ m_FileSize ] = '\0';

    if ( m_FileSize > 0 )
    {
        m_xIRadFile->ReadAsync( m_pFileMemory, m_FileSize );
    }

    m_xIRadFile->WaitForCompletion( );
    m_xIRadFile = NULL; // close the file

    TokenizeScript( );
}

void IRadScript::TokenizeScript( void )
{
    // Process the data now that it's loaded

    m_FileSize++;
    bool nStateString = false;


    for( unsigned int i = 0; i < m_FileSize; i ++ )
    {
        if
        (
            m_pFileMemory[ i ] == ' ' || 
            m_pFileMemory[ i ] == '\t'
        )
        {
            // if we are not inside a middle of a string, tokenize it.
            if (!nStateString)
            {
                m_pFileMemory[ i ] = '\0';
            }
        }
        else if
        (
            m_pFileMemory[ i ] == 13 || 
            m_pFileMemory[ i ] == 10
        )
        {
            m_pFileMemory[ i ] = '\0';
        }
        else if ( m_pFileMemory[ i ] == '\"' )
        {
            // toggle string state
            if (nStateString)
            {
                nStateString = false;
            }
            else
            {
                nStateString = true;
            }
        }
    }
}

//========================================================================
// IRadScript::OnFileOperationsComplete
//========================================================================

void IRadScript::OnFileOperationsComplete( void* pUserData )
{
    //
    // Determine what we just did using pUserData
    //
    
    switch( m_ScriptLoadState )
    {
        case GetFileSize:
        {
            // Read the data now that we know the file size . Read to aligned 128 to improve performance 

            radMemoryAllocator current = radMemoryGetCurrentAllocator();
            rReleaseAssert( m_pFileMemory == NULL );
            
            unsigned int overSizedRead =
                radMemoryRoundUp( m_FileSize + 1, m_xIRadFile->GetOptimalSize( ) );
            
            //total Hack 6 is the fe heap
            m_pFileMemory = (char*) radMemoryAllocAligned( 6, overSizedRead, 128 ); // + 1 for adding last '\0'
            m_pFileMemory[ m_FileSize ] = '\0';

            if  ( m_FileSize > 0 )
            {
                m_xIRadFile->ReadAsync( m_pFileMemory, overSizedRead );
            }

            m_ScriptLoadState = ReadFile;
            m_xIRadFile->AddCompletionCallback( this, &m_ScriptLoadState );

            break;
        }
        case ReadFile:
        {
            m_xIRadFile = NULL; // close the file

            TokenizeScript( );

            AddRef( );
            m_pScriptLoadCallback( m_pScriptLoadCallbackUserData );
            Release( );

            break;
        }
        default:
        {
            rAssertMsg( NULL, "Script: Unrecognized load state" );
        }
    }
}


//========================================================================
// ::radScriptLoadAndRunScriptSync
//========================================================================

void radScriptLoadAndRunScriptSync
(
    const char * pScript,
    IRadNameSpace * pPog2,
    radMemoryAllocator allocator,
	radScriptWarningLevel warningLevel


)
{
    rAssert( pScript != NULL );
    rAssert( pPog2 != NULL );

    radRef< IRadScript > xIRadScript( ::radScriptCreateScript( RADMEMORY_ALLOC_TEMP ) );

	xIRadScript->SetWarningLevel( warningLevel ),
    xIRadScript->SetContext( pPog2 );
    xIRadScript->SetAllocator( allocator );
    xIRadScript->LoadSync( pScript );
    
    xIRadScript->Run( );
}

//========================================================================
// ::radScriptLoadAndRunScriptSync
//========================================================================

void radScriptLoadScriptSync
(
    const char * pScriptFile,
    IRadScript ** ppIRsdScript,
    radMemoryAllocator allocator
)
{
    rAssert( pScriptFile != NULL );
    rAssert( ppIRsdScript != NULL );

    *ppIRsdScript = ::radScriptCreateScript( allocator );
    (*ppIRsdScript)->AddRef( );
    (*ppIRsdScript)->LoadSync( pScriptFile );
}

//========================================================================
// ::radScriptInitialize
//========================================================================

static int g_RadScriptInitializeCount = 0;

void radScriptInitialize( radMemoryAllocator allocator )
{
    if ( g_RadScriptInitializeCount == 0 )
    {
        rDebugString( "::RadScript Version 3.4\n" );
        radFactoryInitialize( allocator );
        radTypeInfoSystemInitialize( allocator );
    }

    g_RadScriptInitializeCount++;
}

//========================================================================
// ::radScriptTerminate
//========================================================================

void radScriptTerminate( void )
{
    rAssert( g_RadScriptInitializeCount > 0 );

    g_RadScriptInitializeCount--;

    if ( g_RadScriptInitializeCount == 0 )
    {
        radTypeInfoSystemTerminate( );
        radFactoryTerminate( );
    }
}
