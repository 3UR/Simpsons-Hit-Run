//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//============================================================================
// Include Files
//============================================================================
#include "pch.hpp"
#include <string.h>
#include <ctype.h>
#include <typeinfo>

#include <radobject.hpp>
#include <radobjectlist.hpp>
#include <radstring.hpp>
#include <radobjectbtree.hpp>
#include <radscript.hpp>
#include <radtypeinfo.hpp>


//============================================================================
// Global variables
//============================================================================
const unsigned int ParserConst::TOK_CREATE        = ParserConst::GetTokenID( "create" );
const unsigned int ParserConst::TOK_WITH          = ParserConst::GetTokenID( "with" );
const unsigned int ParserConst::TOK_NAMED         = ParserConst::GetTokenID( "named" );
const unsigned int ParserConst::TOK_OPTION        = ParserConst::GetTokenID( "option" );
const unsigned int ParserConst::TOK_PC            = ParserConst::GetTokenID( "PC" );
const unsigned int ParserConst::TOK_XBOX          = ParserConst::GetTokenID( "XBOX" );

const unsigned int ParserConst::TOK_LCURLY        = ParserConst::GetTokenID( "{" );
const unsigned int ParserConst::TOK_RCURLY        = ParserConst::GetTokenID( "}" );
const unsigned int ParserConst::TOK_LPAREN        = ParserConst::GetTokenID( "(" );
const unsigned int ParserConst::TOK_RPAREN        = ParserConst::GetTokenID( ")" );

const unsigned int ParserConst::TOK_BOOL          = ParserConst::GetTokenID( "bool" );
const unsigned int ParserConst::TOK_CHAR          = ParserConst::GetTokenID( "char" );
const unsigned int ParserConst::TOK_UCHAR         = ParserConst::GetTokenID( "unsigned char" );
const unsigned int ParserConst::TOK_SHORT         = ParserConst::GetTokenID( "short" );
const unsigned int ParserConst::TOK_USHORT        = ParserConst::GetTokenID( "unsigned short" );
const unsigned int ParserConst::TOK_INT           = ParserConst::GetTokenID( "int" );
const unsigned int ParserConst::TOK_UINT          = ParserConst::GetTokenID( "unsigned int" );
const unsigned int ParserConst::TOK_LONG          = ParserConst::GetTokenID( "long" );
const unsigned int ParserConst::TOK_ULONG         = ParserConst::GetTokenID( "unsigned long" );
const unsigned int ParserConst::TOK_FLOAT         = ParserConst::GetTokenID( "float" );
const unsigned int ParserConst::TOK_DOUBLE        = ParserConst::GetTokenID( "double" );

const unsigned int ParserConst::TOK_IREFCOUNT     = ParserConst::GetTokenID( "IRefCount" );


//============================================================================
// Local structure
//============================================================================
//========================================================================
// Component: IRadTypeInfoSystem
//========================================================================

radRef< IRadString > IRadTypeInfoSystem::ms_pEmptyString = NULL;

radRef< IRadTypeInfoSystem > IRadTypeInfoSystem::s_pTheTypeInfoSystem = NULL;

//============================================================================
// ::radTypeInfoSystemGet
//============================================================================

IRadTypeInfoSystem * radTypeInfoSystemGet( void )
{
    rAssert( IRadTypeInfoSystem::s_pTheTypeInfoSystem != NULL );

    return IRadTypeInfoSystem::s_pTheTypeInfoSystem;
}

//============================================================================
// ::IsRadTypeInfoSystemInitialize
//============================================================================

bool IsRadTypeInfoSystemInitialize()
{
    if (IRadTypeInfoSystem::s_pTheTypeInfoSystem == NULL)
    {
        return(false);
    }
    else
    {
        return(true);
    }
}

//============================================================================
// ::radTypeInfoInitialize
//============================================================================

void radTypeInfoSystemInitialize( radMemoryAllocator allocator )
{
    if ( IRadTypeInfoSystem::s_pTheTypeInfoSystem == NULL )
    {
        new ( allocator ) IRadTypeInfoSystem( allocator );
        ::radStringCreate( & IRadTypeInfoSystem::ms_pEmptyString, allocator );
        IRadTypeInfoSystem::ms_pEmptyString->Copy( "String Not Found" );
    }
    else
    {
        rAssertMsg( 0, "Type info system already initialized" );
    }
}

//============================================================================
// ::radTypeInfoTermaine
//============================================================================

void radTypeInfoSystemTerminate( void )
{
    rAssert( IRadTypeInfoSystem::s_pTheTypeInfoSystem != NULL );
    IRadTypeInfoSystem::ms_pEmptyString = NULL;
    IRadTypeInfoSystem::s_pTheTypeInfoSystem = NULL;
}

void radScriptUnLoadAllTypeInfo( void )
{
    IRadTypeInfoSystem * pSys = (IRadTypeInfoSystem*) IRadTypeInfoSystem::s_pTheTypeInfoSystem.m_pInterface;
    
    pSys->m_xIOl_TypeInfo->RemoveAll( ); 
    pSys->m_xIOl_Enum->RemoveAll( );
    pSys->m_xIOt_StringTable->RemoveAll( );
}



