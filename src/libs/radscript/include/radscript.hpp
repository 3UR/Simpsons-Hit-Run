//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File: radscript.hpp
//
// Subsystem:	
//
// Description:	
//
//=============================================================================

#ifndef RADSCRIPT_HPP
#define RADSCRIPT_HPP

//============================================================================
// Include Files
//============================================================================

#include <radobject.hpp>
#include <radfactory.hpp>
#include <radnamespace.hpp>
#include <radtypeinfo.hpp>
#include <radfile.hpp>

//============================================================================
// Forward Declarations
//============================================================================

struct IRadScript;

//============================================================================
// Types
//============================================================================

typedef void (ScriptLoadCallback)( void * );
// FOR SIMPSONS 2
typedef void (ScriptObjCreateCallback)( const char*, IRefCount* );

enum radScriptWarningLevel {
	radScriptWarningLevelLow, 
	radScriptWarningLevelMedium,
	radScriptWarningLevelHigh };

//============================================================================
// Functions
//============================================================================

void radScriptInitialize( radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );
void radScriptTerminate( void );

void radRemoteScriptInitialize( radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );
void radRemoteScriptTerminate( void );

inline IRadScript * radScriptCreateScript( radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );
IRefCount  * radScriptGetRemoteScript( radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );

void radScriptLoadScriptSync(
	const char * pScriptFile, IRadScript ** pIRadScript,
	radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );
	
void radScriptUnLoadAllTypeInfo( );

void radScriptLoadAndRunScriptSync(
	const char * pScriptFile, IRadNameSpace * pPog2,
	radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT,
	radScriptWarningLevel warningLevel = radScriptWarningLevelHigh );


//============================================================================
// Interface: ITypeInfoScript
//============================================================================

// Maxium number of parameter in the function
const int MAX_PARAM_SIZE = 32;
// Maxium deep of platform option ( 2 is really enough )
const int MAX_OPTIONSTACK_SIZE = 8;    // 8 is a really deep stack

struct IRadScript
    :
    public IRadFileCompletionCallback,
    public radRefCount
{

    IRadScript( radMemoryAllocator allocator );
    virtual ~IRadScript( void );
    void Load( const char * pMemory, unsigned int size );
    void UnLoad( void );
    void SetWarningLevel( radScriptWarningLevel warningLevel );
    inline bool AttemptMatch( const unsigned int nTokenID );    
    inline bool MustMatch( const unsigned int nTokenID, const char * szToken = "" );  
    inline void NextToken( void );
    inline char * ReadToken( void );
    inline bool IsFloat( const char * p );
    inline bool IsInt( const char * p );
    inline int SplitOption( char * pToken, int * pnOptionField );
    inline bool IsCurrentPlatForm( ) const;
    inline bool OnBeginObject( const char * pObjectName, const char * pClassName, const bool bCreate );
    inline bool OnEndObject( void );
    inline bool ParamTypeCheck( const char * szParam, IRadTypeInfoParam * pTypeInfoParam );
    inline bool OnBeginMethod( const char * pMethodName, char *pParamList[], const int nParamSize );
    inline bool OnEndMethod( );
    inline bool ParseObject( char * & pObjectName, char * & pClassName, bool & bCreate );
    inline bool ParseMethod( char * & pMethodName, char **  pszParamList, int & nParamSize );
    inline void Run( ScriptObjCreateCallback* pCallback = NULL );
    void SetContext( IRadNameSpace * pIPersistObjectGroup2 );
    void SetAllocator( radMemoryAllocator allocator );
    void Load( const char * pFileName, ScriptLoadCallback * pCallback, void * pCallbackUserData, radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );    
    void LoadSync( const char * pFileName, radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );
    IRadNameSpace * GetContext( void );
    void TokenizeScript( void );
    void OnFileOperationsComplete( void* pUserData );
    
    IMPLEMENT_REFCOUNTED( "radScript" )

    //========================================================================
    // Memory allocation and file reading stuff
    //========================================================================

    radMemoryAllocator      m_ScriptAllocator;
    radScriptWarningLevel   m_WarningLevel;

    unsigned int            m_FileSize;
    char *                  m_pFileMemory;
    radRef< IRadNameSpace >    m_xIRadNameSpace_Context;

    radRef< IRadFile >         m_xIRadFile;

    ScriptLoadCallback *    m_pScriptLoadCallback;
    void *                  m_pScriptLoadCallbackUserData;

    // ADDED FOR SIMPSONS 2
    ScriptObjCreateCallback* m_pObjCreateCallback;

    enum ScriptLoadState
    {
        GetFileSize,
        ReadFile
    };

    ScriptLoadState m_ScriptLoadState;

    const char *            m_pCurrObjectName;
    const char *            m_pCurrClassName;
    const char *            m_pCurrMethodName;

    char *                  m_pszCurrMethodParams[ MAX_PARAM_SIZE ];
    int                     m_nCurrMethodParamSize;

    ParserConst::PlatformType   m_PlatformOptionStack[ MAX_OPTIONSTACK_SIZE ];
    int                     m_nCurrPlatformOptionStackSize;

    char *                  m_szCurrToken;


    radRef< IRefCount >        m_xCurrObjectInstance;
    radRef< IRadTypeInfo >     m_xCurrObjectTypeInfo;
};

inline bool IRadScript::AttemptMatch( const unsigned int nTokenID )
{
    if ( ParserConst::GetTokenID( m_szCurrToken ) == nTokenID )
    {
        return true;
    }
    else
    {
        return false;
    }
}

inline void IRadScript::NextToken( )
{
    m_szCurrToken += ( strlen( m_szCurrToken ) + 1 );

    while( ( m_szCurrToken < m_pFileMemory + m_FileSize ) && ( strlen( m_szCurrToken ) == 0 ) )
    {
        m_szCurrToken++;
    }    
}

inline bool IRadScript::MustMatch( const unsigned int nTokenID, const char * szToken )
{
    if ( ParserConst::GetTokenID( m_szCurrToken ) == nTokenID )
    {
        NextToken( );
        return true;
    }
    else
    {
        rDebugString( "IRadScript : (Parser error) Cannot match token [" );
        rDebugString( szToken );
        rDebugString( "].\n" );
        rAssert( 0 );
        NextToken( );
        return false;
    }
}

inline char * IRadScript::ReadToken( )
{
    char * pToken = m_szCurrToken;
    NextToken( );
    return pToken;
}


inline bool IRadScript::IsFloat( const char * p )
{
    bool foundDot = false;

    for( unsigned int i = 0; i < strlen( p ); i ++ )
    {
        if( !( p[i] >= '0' && p [i] <= '9' ) && p[i] != '-' && p[i] != '.')
        {                
            return false;
        }

        if ( p[i] == '.' )
        {
            foundDot = true;
        }
    }

    return foundDot;
}

inline bool IRadScript::IsInt( const char * p )
{
    for( unsigned int i = 0; i < strlen( p ); i ++ )
    {
        if( !( p[i] >= '0' && p [i] <= '9' ) && p[i] != '-' )
        {
            return false;
        }
    }

    return true;
}

inline int IRadScript::SplitOption( char * pToken, int * pnOptionField )
{
    // option looks like this
    // PC|XBOX
    // DEFAULT
    // PC
    //
    // DEFAULT is mutually exclusive with everything else.
    if ( strcmp ( pToken, "DEFAULT" ) == 0 )
    {
        *pnOptionField = ParserConst::PT_DEFAULT;

        return 1;
    }
    else
    {
        int nTokenLength = strlen( pToken );
        *pnOptionField = 0;

        // rule check : first or last character cannot be '|'
        if ( pToken [ 0 ] == '|' || pToken [ nTokenLength ] == '|' )
        {
            rDebugString( "IRadScript: Invalid syntax after \'option\' keyword, \'|\' cannot be leading or trailing character.\n" );
            return -1;
        }

        // split the options to individual option
        int nOptionCount = 0;
        char *pszOption [ 8 ] = { 0 };

        nOptionCount ++;
        pszOption [ 0 ] = pToken;
        for ( int i = 0; i < nTokenLength; i++ )
        {
            if ( pToken [ i ] == '|' )
            {
                pszOption [ nOptionCount ] = &pToken[ i + 1 ]; nOptionCount ++;
            }
        }

        for ( int j = 0; j < nOptionCount; j++ )
        {
            if ( strncmp ( pszOption [ j ], "PC", 2 ) == 0 )
            {
                *pnOptionField |= ParserConst::PT_PC;
            }
            else if ( strncmp ( pszOption [ j ], "XBOX", 4 ) == 0 )
            {
                *pnOptionField |= ParserConst::PT_XBOX;
            }
            else
            {
                rDebugString( "IRadScript: Invalid option \'" );
                rDebugString( pszOption [ j ] );
                rDebugString( "\', symbol cannot be used.\n" );
                return -1;
            }
        }

        return nOptionCount;
    }
}

inline bool IRadScript::IsCurrentPlatForm( ) const
{
    ParserConst::PlatformType nReturnOption = (ParserConst::PlatformType) 0;

    for ( int i = 0; i < m_nCurrPlatformOptionStackSize; i++ )
    {
		if ( m_PlatformOptionStack [ i ] != ParserConst::PT_DEFAULT )
		{
			nReturnOption = ((ParserConst::PlatformType) ((int) nReturnOption | (int) m_PlatformOptionStack [ i ]) );
		}
    }

	if ( nReturnOption == 0 )
	{
		nReturnOption = ParserConst::PT_DEFAULT;
	}

    #if defined RAD_WIN32
    if ( nReturnOption & ParserConst::PT_PC )
    {
        return true;
    }
    #endif

    #if defined RAD_UWP
    if ( nReturnOption & ParserConst::PT_XBOX )
    {
        return true;
    }
    #endif
    
    return false;
}

inline bool IRadScript::OnBeginObject( const char * pObjectName, const char * pClassName, const bool bCreate )
{
    m_pCurrClassName    = NULL;
    m_pCurrObjectName   = NULL;
    m_pCurrMethodName   = NULL;
    m_nCurrMethodParamSize = 0;

    if ( bCreate )
    {
        radFactoryCreateInstance( pClassName, & m_xCurrObjectInstance, m_ScriptAllocator );

        if ( m_xCurrObjectInstance != NULL )
		{
  			if( false == m_xIRadNameSpace_Context->AddInstance( pObjectName, m_xCurrObjectInstance ) )
  			{
  				rTunePrintf( "AUDIO: WARNING: Inventory Collision!: %s %seg\n", pClassName, pObjectName );
  			}
		}
    }
    else
    {
        m_xCurrObjectInstance = m_xIRadNameSpace_Context->GetInstance( pObjectName );
    }

	if ( m_xCurrObjectInstance != NULL)
	{
		m_xCurrObjectTypeInfo = ::radTypeInfoSystemGet( )->GetTypeInfo( m_xCurrObjectInstance );
	}
	else
	{
		return false;
	}

    m_pCurrClassName    = pClassName;
    m_pCurrObjectName   = pObjectName;

    return true;
}

inline bool IRadScript::OnEndObject( )
{
    // pop stack after object is finished
    m_nCurrPlatformOptionStackSize --;

    m_pCurrObjectName = NULL;
    m_pCurrClassName = NULL;
    m_xCurrObjectInstance = NULL;
    m_xCurrObjectTypeInfo = NULL;
    return true;
}

inline bool IRadScript::ParamTypeCheck( const char * szParam, IRadTypeInfoParam * pTypeInfoParam )
#ifdef RADSCRIPT_DEBUG
{
    char szParamTypeGot[ ParserConst::MAX_ID_STRING_SIZE ];

    if ( szParam[ 0 ] == '\"' )
    {
        if (
            ( ( pTypeInfoParam->GetHashedType( ) == ParserConst::TOK_CHAR ) ||
                ( pTypeInfoParam->GetHashedType( ) == ParserConst::TOK_UCHAR ) ) &&
            pTypeInfoParam->GetIndLvl( ) == 1
            )
        {
            return true;
        }
        else
        {
            strcpy( szParamTypeGot, "string" );
        }
    }
    else if ( IsFloat( szParam ) )
    {
        // float type
        if ( pTypeInfoParam->GetHashedType( ) == ParserConst::TOK_FLOAT )
        {
            return true;
        }
        else
        {
            strcpy( szParamTypeGot, "float" );
        }
    }
    else if ( strcmp( szParam, "true" ) == 0 || strcmp( szParam, "false" ) == 0 )
    {
        if ( pTypeInfoParam->GetHashedType( ) == ParserConst::TOK_BOOL )
        {
            return true;
        }
        else
        {
            strcpy( szParamTypeGot, "bool" );
        }
    }
    else if ( IsInt( szParam ) )
    {
        // Integer type is
        // unsigned int/int
        // unsigned long/long
        // unsigned short/short
        // unsigned char/char
        // or enums

        // cannot be float
        if ( pTypeInfoParam->GetHashedType( ) == ParserConst::TOK_FLOAT )
        {
            strcpy( szParamTypeGot, "int" );
        }
        else
        {
            return true;
        }
    }
    else if ( strncmp( szParam, "ref:", 4 ) == 0 )
    {
        IRefCount * pIRefCount = m_xIRadNameSpace_Context->GetInstance( szParam + 4 );
        if ( pIRefCount == NULL )
        {
            rDebugString( "IRadScript : Couldn't find referenced instance: [" );
            rDebugString( szParam + 4 );
            rDebugString( "]\n" );

            return false;
        }
        //
        // Check that param is the right type.
        // 
        IRadTypeInfo * pIRadTypeInfo = ::radTypeInfoSystemGet( )->GetTypeInfo( pIRefCount );

        if ( pIRadTypeInfo == NULL )
        {
			if ( m_WarningLevel > radScriptWarningLevelMedium )
			{
				rDebugString( "IRadScript : No Type Info For referenced object [" );
				rDebugString( szParam + 4 );
				rDebugString( "], cannot verify script integrity\n" );
			}
        }
        else if (
                    pIRadTypeInfo->GetHashedName( ) != pTypeInfoParam->GetHashedType( ) &&
                    pTypeInfoParam->GetHashedType( ) != ParserConst::TOK_IREFCOUNT
                )
        {
			if ( m_WarningLevel > radScriptWarningLevelMedium )
			{
				rDebugString( "IRadScript: Possible type mis-match between [" );
				rDebugString( pIRadTypeInfo->GetName( ) );
				rDebugString( "] and [" );
				rDebugString( pTypeInfoParam->GetType( ) );
				rDebugString( "].\n" );
			}
        }
        return true;
    }
    else
    {
        rDebugString( "IRadScript: Unrecognized parameter:[" );
        rDebugString( szParam );
        rDebugString( "]\n" );
        return false;
    }

    rDebugString( "IRadScript : method [(" );
    rDebugString( m_pCurrClassName );
    rDebugString( ")" );
    rDebugString( m_pCurrObjectName );
    rDebugString( "." );
    rDebugString( m_pCurrMethodName );
    rDebugString( "( )] is expecting type : [" );
    rDebugString( pTypeInfoParam->GetType( ) );
    for ( int nIndCount = 0; nIndCount < pTypeInfoParam->GetIndLvl( ); nIndCount++ )
    {
        rDebugString( "*" );
    }
    rDebugString( "] got : [" );
    rDebugString( szParamTypeGot );
    rDebugString( "].\n");

    return false;
}
#else
{
    return true;
}
#endif

inline bool IRadScript::OnBeginMethod(
                            const char * pMethodName,
                            char *pParamList[],
                            const int nParamSize
                            )
{
	radRef< IRadTypeInfoMethod > xIRadTypeInfoMethod;

	if ( m_xCurrObjectTypeInfo != NULL )
	{
		xIRadTypeInfoMethod = m_xCurrObjectTypeInfo->GetMethodByName( pMethodName );

		if ( xIRadTypeInfoMethod == NULL )
		{
			rDebugString( "IRadScript: Couldn't find type info method: [" );
			rDebugString( m_pCurrClassName );
			rDebugString( ")" );
			rDebugString( m_pCurrObjectName );
			rDebugString( "." );
			rDebugString( pMethodName );
			rDebugString( "( )] .\n" );

			rAssert( 0 );
			return false;
		}
	}

    m_pCurrMethodName = pMethodName;

    radRef< IRadTypeInfoParam > pIRadTypeInfoParam;

    // stack of parameters pointer
    unsigned int nInvokeParams[ MAX_PARAM_SIZE ];

    //
    // Check parameter size
    //

	if ( xIRadTypeInfoMethod != NULL )
	{
		if ( xIRadTypeInfoMethod->GetNumberOfParams( ) != static_cast< unsigned int >( nParamSize ) )
		{
			rDebugString( "IRadScript: Parameter size mismatch: [" );
			rDebugString( m_pCurrClassName );
			rDebugString( ")" );
			rDebugString( m_pCurrObjectName );
			rDebugString( "." );
			rDebugString( pMethodName );
			rDebugString( "( )] .\n" );

			rAssert( 0 );
			return false;
		}
	}
    //
    // Populate parameter pointers
    //
    for ( int nParamIndex = 0; nParamIndex < nParamSize; nParamIndex ++ )
    {
        char * pParamString = pParamList[ nParamIndex ];

		if ( xIRadTypeInfoMethod != NULL )
		{
			pIRadTypeInfoParam = xIRadTypeInfoMethod->GetParamInfoAt( nParamIndex );

			//
			// type checking
			//
			if ( !ParamTypeCheck( pParamString, pIRadTypeInfoParam ) )
			{
				rAssert( 0 );
				return false;
			}
		}

        if ( pParamString[0] == '\"' )
        {
            pParamString[ strlen( pParamString ) -1 ] = '\0';

            *((char**)&(nInvokeParams[ nParamIndex ])) = pParamString + 1;
        }
        else if ( IsFloat( pParamString ) )
        {
            *((float*)&(nInvokeParams[ nParamIndex ])) = (float) atof( pParamString );
        }
        else if ( IsInt( pParamString ) )
        {
            *((int*)&(nInvokeParams[ nParamIndex ])) = atoi( pParamString );
        }
        else if ( strcmp( pParamString, "true" ) == 0 || strcmp( pParamString, "false" ) == 0 )
        {
            *((int*)&(nInvokeParams[ nParamIndex ])) = strcmp( pParamString, "true" ) == 0 ? 1 : 0;
        }
#if defined(WIN32) && !defined(_WIN64)
        else if ( strncmp( pParamString, "ref:", 4 ) == 0 )
        {
            IRefCount * pIRefCount = m_xIRadNameSpace_Context->GetInstance( pParamString + 4 );
            *((int*) &(nInvokeParams[ nParamIndex ]) ) = (int) pIRefCount;
        }
#else
        // TODO(3ur): how hard would this be?
        #pragma message("NOTE: InvokeVf is unsupported on x64")
#endif
    }

    //================================================
    // Invoke function call
    //================================================

	if ( xIRadTypeInfoMethod != NULL )
	{
		xIRadTypeInfoMethod->Invoke( m_xCurrObjectInstance, nInvokeParams, nParamSize );

		xIRadTypeInfoMethod = NULL;
	}
    return true;
}

inline bool IRadScript::OnEndMethod( )
{
    // pop stack after method is finished
    m_nCurrPlatformOptionStackSize --;

    m_pCurrMethodName = NULL;
    return true;
}

inline bool IRadScript::ParseObject(
                            char * & pObjectName,
                            char * & pClassName,
                            bool & bCreate
                        )
{
    // assign default value
    ParserConst::PlatformType eCurrOptionField = ParserConst::PT_DEFAULT;
    pObjectName = NULL;
    pClassName = NULL;
    bCreate = true;

    // create/with (class name) named (object name) [option (option list)]
    if ( AttemptMatch( ParserConst::TOK_CREATE ) )
    {
        bCreate = true;
        NextToken( );
    }
    else if ( AttemptMatch( ParserConst::TOK_WITH ) )
    {
        bCreate = false;
        NextToken( );
    }
    else
    {
        rDebugString( "IRadScript: (Parser error) \"create\" or \"with\" keyword expected.\n" );
        rAssert( 0 );
    }

    pClassName = ReadToken( );

    MustMatch( ParserConst::TOK_NAMED, "named" );

    pObjectName = ReadToken( );

    if ( AttemptMatch( ParserConst::TOK_OPTION ) )
    {
        MustMatch( ParserConst::TOK_OPTION, "option" );
        
        SplitOption ( ReadToken( ), ( int * ) & eCurrOptionField );
    }

    m_PlatformOptionStack[m_nCurrPlatformOptionStackSize] = eCurrOptionField;
    m_nCurrPlatformOptionStackSize ++;

    return true;
}

//========================================================================
// IRadScript::ParseMethod
//========================================================================
inline bool IRadScript::ParseMethod(
                            char * & pMethodName,
                            char **  pszParamList,
                            int & nParamSize
                        )
{
    pMethodName = NULL;
    nParamSize = 0;
    ParserConst::PlatformType eCurrOptionField = ParserConst::PT_DEFAULT;

    pMethodName = ReadToken( );

    MustMatch( ParserConst::TOK_LPAREN, "(" );

    while ( !AttemptMatch( ParserConst::TOK_RPAREN ) )
    {
        pszParamList[ nParamSize ] = ReadToken( );
        nParamSize ++;
        if ( nParamSize >= MAX_PARAM_SIZE )
        {
            rDebugString( "IRadScript : (Parser error) parameters exceeds 31, possible corrupted script file.\n" );
            rAssert( 0 );
        }
    }

    MustMatch( ParserConst::TOK_RPAREN, ")" );

    if ( AttemptMatch( ParserConst::TOK_OPTION ) )
    {
        MustMatch( ParserConst::TOK_OPTION );

        SplitOption ( ReadToken( ), ( int * ) & eCurrOptionField );
    }

    m_PlatformOptionStack[ m_nCurrPlatformOptionStackSize ] = eCurrOptionField;
    m_nCurrPlatformOptionStackSize ++;

    return true;
}

//========================================================================
// IRadScript::Run
//========================================================================
inline void IRadScript::Run( ScriptObjCreateCallback* pCallback )
{
    m_szCurrToken = m_pFileMemory;

    // SIMPSONS 2
    m_pObjCreateCallback = pCallback;

    // position to the first valid token
    if ( strlen( m_szCurrToken ) == 0 )
    {
        NextToken( );
    }

    //====================================================================
    // Parse the script file
    //====================================================================
    while ( ( m_szCurrToken + 1 ) < ( m_pFileMemory + m_FileSize ) )
    {
        bool    bObjectCreate   = true;
        char *  pClassName      = NULL;
        char *  pObjectName     = NULL;
        ParserConst::PlatformType    eOptionField = ParserConst::PT_DEFAULT;

        if (
            ParseObject( pObjectName, pClassName, bObjectCreate ) &&
            IsCurrentPlatForm( )
            )
        {
            
            OnBeginObject( pObjectName, pClassName, bObjectCreate );

            if ( AttemptMatch( ParserConst::TOK_LCURLY ) )
            {
                MustMatch( ParserConst::TOK_LCURLY, "{" );
                while ( ! AttemptMatch( ParserConst::TOK_RCURLY ) )
                {
                    char * pMethodName = NULL;
                    char * pszParamTextList[ MAX_PARAM_SIZE ];
                    int nParamSize = 0;

                    if (
                        ParseMethod( pMethodName, pszParamTextList, nParamSize ) &&
                        IsCurrentPlatForm( )
                        )
                    {
                        OnBeginMethod( pMethodName, pszParamTextList, nParamSize );
                    }

                    OnEndMethod( );
                }
                MustMatch( ParserConst::TOK_RCURLY, "}" );
            }

        }
        else
        {
            // skip the entire object if not needed.
            if ( AttemptMatch( ParserConst::TOK_LCURLY ) )
            {
                MustMatch( ParserConst::TOK_LCURLY, "{" );
                while ( ! AttemptMatch( ParserConst::TOK_RCURLY ) )
                {
                    NextToken( );
                }
                MustMatch( ParserConst::TOK_RCURLY, "}" );
            }
        }

        // SIMPSONS 2
        if( bObjectCreate &&
            ( m_pObjCreateCallback != NULL ) )
        {
            m_pObjCreateCallback( pObjectName, m_xCurrObjectInstance );
        }

        OnEndObject( );
    }
}

inline IRadScript * radScriptCreateScript( radMemoryAllocator allocator )
{
    return new ( allocator ) IRadScript( allocator );
}

#endif // RADSCRIPT_HPP