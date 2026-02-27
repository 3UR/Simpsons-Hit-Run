//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File: radtypeinfo.hpp
//
// Subsystem:	
//
// Description:	
//
//=============================================================================

#ifndef	TYPEINFO_HPP
#define TYPEINFO_HPP

//=============================================================================
// Build Configuration Check
//=============================================================================

#if !defined(RAD_DEBUG) && !defined(RAD_TUNE) && !defined(RAD_RELEASE) 
    #error 'FTech requires definition of RAD_DEBUG, RAD_TUNE, or RAD_RELEASE'
#endif

//============================================================================
// Include Files
//============================================================================
#include <string.h>
#include <radobject.hpp>
#include <radmemory.hpp>
#include <radkey.hpp>
#include <radstring.hpp>
#include <radobjectlist.hpp>
#include <radobjectbtree.hpp>
#include <string.h>
#include <ctype.h>
#include <typeinfo>

//============================================================================
// Types Used by the .hpp parser
//============================================================================
const int TYP_FILE_VERSION = 0x04;

namespace ParserConst
{
    //========================================================================
    // Inline
    //========================================================================
    inline unsigned int GetTokenID( const char * pToken )
    {
        return radMakeKey32( pToken );
    }

    //
    // list of token used in the type info file
    //
    enum TokenType
    {
        TOK_VERSION     = 0x43,     // typeinfo file version
        TOK_CLASS       = 0x40,     // class
        TOK_METHOD      = 0x41,     // method
        TOK_ENUM        = 0x44      // enum
    };

    enum PlatformType
    {
        PT_PC           = 0x0001,
        PT_XBOX         = 0x0002,

        PT_ALL          = PT_PC | PT_XBOX,
        PT_DEFAULT      = -1
    };

    const unsigned int MAX_ID_STRING_SIZE = 64;

    extern const unsigned int TOK_CREATE;
    extern const unsigned int TOK_WITH;
    extern const unsigned int TOK_NAMED;
    extern const unsigned int TOK_OPTION;
    extern const unsigned int TOK_PC;
    extern const unsigned int TOK_XBOX;

    extern const unsigned int TOK_LCURLY;
    extern const unsigned int TOK_RCURLY;
    extern const unsigned int TOK_LPAREN;
    extern const unsigned int TOK_RPAREN;

    extern const unsigned int TOK_BOOL;
    extern const unsigned int TOK_CHAR;
    extern const unsigned int TOK_UCHAR;
    extern const unsigned int TOK_SHORT;
    extern const unsigned int TOK_USHORT;
    extern const unsigned int TOK_INT;
    extern const unsigned int TOK_UINT;
    extern const unsigned int TOK_LONG;
    extern const unsigned int TOK_ULONG;
    extern const unsigned int TOK_FLOAT;
    extern const unsigned int TOK_DOUBLE;

    extern const unsigned int TOK_IREFCOUNT;
}

//============================================================================
// Forward Declarations
//============================================================================

struct IRadTypeInfoSystem;
struct IRadTypeInfo;
struct IRadTypeInfoMethod;
struct IRadTypeInfoEnum;
struct IRadTypeInfoParam;
struct IRadTypeInfoIntLiteral;
struct IRadTypeInfoLoader;
//============================================================================
// Functions
//============================================================================

bool IsRadTypeInfoSystemInitialize();
void radTypeInfoSystemInitialize( radMemoryAllocator allocator );
void radTypeInfoSystemTerminate( void );

IRadTypeInfoSystem * radTypeInfoSystemGet( void );

IRadTypeInfo *       radTypeInfoCreate( radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );
inline IRadTypeInfoMethod * radTypeInfoMethodCreate( radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );
inline IRadTypeInfoParam *  radTypeInfoParamCreate( radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );
inline IRadTypeInfoIntLiteral *  radTypeInfoIntLiteralCreate( radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );
IRadTypeInfoLoader * radTypeInfoLoaderCreate( radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );

//============================================================================
// Interface: IRadTypeInfoSystem
//============================================================================

typedef void (TypeInfoLoadCallback)( void * pUserData );

struct IRadTypeInfoSystem
    :
    public radRefCount
{
public:
    IMPLEMENT_REFCOUNTED( "IRadTypeInfoSystem" )

    inline IRadTypeInfoSystem( radMemoryAllocator allocator );
    inline virtual ~IRadTypeInfoSystem( void );

    inline void LoadTypeInfo( radMemoryAllocator alloc, const char * pFileName, TypeInfoLoadCallback *, void * pUserData );

    inline IRadTypeInfo * GetTypeInfo( IRefCount * pIRefCount );
    inline IRadTypeInfoMethod * GetTypeInfoMethod( IRefCount * pIRefCount, const char * pMethodName );
                 
    inline void AddTypeInfo( IRadTypeInfo * pIRadTypeInfo );

    inline void AddEnum( IRadTypeInfoEnum * pIRadTypeInfoEnum );

    inline int GetTypeInfoCount( void );
    inline IRadTypeInfo * GetTypeInfoAt( unsigned int index );

    inline int GetEnumCount( void );
    inline IRadTypeInfoEnum * GetEnumAt( unsigned int index );

    inline bool GetLoadString( void );
    inline void SetLoadString( bool bLoadString );

    inline void AddString( unsigned int uHashValue, const char * szString );

    inline const char * LookupString( unsigned int uHashValue );

    inline void EmptyStrTbl( );

#ifdef RADSCRIPT_DEBUG
    inline void DebugDump( void );
#endif

    static radRef< IRadTypeInfoSystem > s_pTheTypeInfoSystem;
    static radRef< IRadString > ms_pEmptyString;

    //========================================================================
    // DataMembers
    //========================================================================
    radMemoryAllocator      m_ThisAllocator;

    bool                    m_bLoadString;

    radRef< IRadObjectList >   m_xIOl_TypeInfo;
    radRef< IRadObjectList >   m_xIOl_Enum;

    // string table used for all typeinfo string in typeinfo system
    radRef< radObjectBTree >  m_xIOt_StringTable;

};


//============================================================================
// Interface: IRadTypeInfoLoader
//============================================================================

struct IRadTypeInfoLoader : public IRefCount
{
    virtual void Initialize( IRadTypeInfoSystem *, radMemoryAllocator allocator, const char * pFileName, TypeInfoLoadCallback * , void * pUserData , bool bLoadEnum = false ) = 0;
};

//============================================================================
// Interface: IRadTypeInfo
//============================================================================

struct IRadTypeInfo :
    public IRefCount,
    public radRefCount
{
public:
    IMPLEMENT_REFCOUNTED( "IRadTypeInfo" )

    IRadTypeInfo( radMemoryAllocator allocator );

    inline virtual ~IRadTypeInfo( void );

    inline void Initialize( const char * pInterfaceName  );
    inline const char * GetName( void );
    inline unsigned int GetHashedName( void );

    inline unsigned int  GetNumberOfMethods( void );
    inline IRadTypeInfoMethod * GetMethodByIndex( unsigned int index );
    inline IRadTypeInfoMethod * GetMethodByName( const char * pName );

    inline unsigned int  GetNumberOfEnums( void );
    inline IRadTypeInfoEnum * GetEnumByIndex( unsigned int index );
    inline IRadTypeInfoEnum * GetEnumByName( const char * pName );

    inline void AddMethod( IRadTypeInfoMethod * pIRadTypeInfoMethod );
    inline void AddEnum( IRadTypeInfoEnum * pIRadTypeInfoEnum );

#ifdef RADSCRIPT_DEBUG
    inline void DebugDump( void );
#endif

private:

    //========================================================================
    // Data Members
    //========================================================================
    radMemoryAllocator      m_ThisAllocator;

    unsigned int            m_uHashedName;
    radRef< IRadObjectList >   m_xIOl_Methods;
    radRef< IRadObjectList >   m_xIOl_Enums;
};

//============================================================================
// Interface: IRadTypeInfoLiteral
//============================================================================

struct IRadTypeInfoIntLiteral :
    public IRefCount,
    public radRefCount
{
public:
    IMPLEMENT_REFCOUNTED( "IRadTypeInfoIntLiteral" )

    inline IRadTypeInfoIntLiteral( );

    inline virtual ~IRadTypeInfoIntLiteral( void );

    inline void Initialize( const char * szName, int nValue );

    inline const char * GetName( void );

    inline unsigned int GetHashedName( void );

    inline int GetValue( void );

#ifdef RADSCRIPT_DEBUG
    inline void DebugDump( void );
#endif

private:

    //========================================================================
    // Data Members
    //========================================================================
    unsigned int            m_uHashedName;
    int                     m_nValue;
};


//============================================================================
// Interface: IRadTypeInfoEnum
//============================================================================

struct IRadTypeInfoEnum :
    public IRefCount,
    public radRefCount
{
public:
    IMPLEMENT_REFCOUNTED( "IRadTypeInfoEnum" )

    inline IRadTypeInfoEnum( );

    inline virtual ~IRadTypeInfoEnum( void );

    inline void Initialize( const char * pName );

    inline void AddIntLiteral( IRadTypeInfoIntLiteral * pIntLiteral );

    inline const char *  GetName( void );

    inline unsigned int GetHashedName( void );

    inline unsigned int  GetNumberOfIntLiteral( void );
    inline IRadTypeInfoIntLiteral * GetIntLiteralPtrAt( unsigned int nIndex );
    inline int           GetValueByName( const char * pName );

#ifdef RADSCRIPT_DEBUG
    inline void DebugDump( void );
#endif

private:

    unsigned int                m_uHashedName;

    radRef< IRadObjectList >       m_lstIntLiteralsPtr;
};

struct IRadTypeInfoParam
    :
    public IRefCount,
    public radRefCount
{
public:

    IMPLEMENT_REFCOUNTED( "IRadTypeInfoParam" )

    inline IRadTypeInfoParam( );

    inline virtual void Initialize( const char * szName, const char * szType, int nIndLvl );

    inline const char * GetName( void );
    inline unsigned int GetHashedName( void );

    inline const char * GetType( void );
    inline unsigned int GetHashedType( void );

    inline int GetIndLvl( void );

    inline bool CompareType( IRadTypeInfoParam * pParam );

#ifdef RADSCRIPT_DEBUG
    inline void DebugDump( void );
#endif

    unsigned int        m_uHashedName;
    unsigned int        m_uHashedType;
    int                 m_nIndLvl;
};



//============================================================================
// Interface: IRadTypeInfoMethod
//============================================================================

struct IRadTypeInfoMethod :
    public IRefCount,
    public radRefCount
{
public:
    
    IMPLEMENT_REFCOUNTED( "IRadTypeInfoMethod" )

    inline IRadTypeInfoMethod( );

    inline virtual ~IRadTypeInfoMethod( void );

    inline void Initialize( const char * pName,
                             unsigned int vTableOffset,
                             IRadTypeInfoParam * pResult );

    inline void AddParamPtr( IRadTypeInfoParam * pParam );

    inline const char * GetName( void );

    inline unsigned int GetHashedName( void );
    
    inline unsigned int GetNumberOfParams( void );
    
    inline IRadTypeInfoParam * GetParamInfoAt( unsigned int nParamIndex );
    
    inline IRadTypeInfoParam * GetReturnInfo( void );
    
    inline unsigned int GetVTableOffset( void );

    unsigned int Invoke( void * pInterface, void * pParams, unsigned int numParams );

#ifdef RADSCRIPT_DEBUG
    inline void DebugDump( void );
#endif

private:

    unsigned int                m_VTableOffset;

    radRef< IRadTypeInfoParam >    m_pReturnParam;

    radRef< IRadObjectList >       m_lstParamsPtr;

    unsigned int                m_uHashedName;

};


//========================================================================
// IRadTypeInfoSystem::AddString
//========================================================================

inline void IRadTypeInfoSystem::AddString( unsigned int uHashValue, const char * szString )
{
    if ( m_bLoadString )
    {
        rAssert( szString );

#ifdef RADSCRIPT_DEBUG
        // hash collision detection
        // see if string already present in the tree
        radRef< IRadString > pFindString = static_cast< IRadString * >( m_xIOt_StringTable->FindObject( uHashValue ) );

        if ( pFindString != NULL && strcmp( pFindString->GetChars( ), szString ) != 0 )
        {
            char szBuffer[256];
            sprintf( szBuffer, "radScript: string collision detected [%s] and [%s].\n",
                     pFindString->GetChars( ), szString );
            rDebugString( szBuffer );
        }
#endif 

		if ( m_xIOt_StringTable->FindObject( uHashValue ) == NULL )
		{
			radRef< IRadString > pString;
			::radStringCreate( & pString, m_ThisAllocator );
			pString->Copy( szString );
							
			m_xIOt_StringTable->AddObject( uHashValue, pString );
		}
    }
}

//========================================================================
// IRadTypeInfoSystem::LookupString
//========================================================================

inline const char * IRadTypeInfoSystem::LookupString( unsigned int uHashValue )
{
    if ( m_bLoadString )
    {
        radRef< IRadString > pFindString = static_cast< IRadString * >( m_xIOt_StringTable->FindObject( uHashValue ) );

        if ( pFindString == NULL )
        {
            return ms_pEmptyString->GetChars( );
        }

        return pFindString->GetChars( );
    }
    return ms_pEmptyString->GetChars( );
}

//========================================================================
// IRadTypeInfoIntLiteral::GetHashedName
//========================================================================

inline unsigned int IRadTypeInfoIntLiteral::GetHashedName( void )
{
    return m_uHashedName;
}

//========================================================================
// IRadTypeInfoIntLiteral::GetValue
//========================================================================

inline int IRadTypeInfoIntLiteral::GetValue( void )
{
    return m_uHashedName;
}

//
//
// Enum
//
//
//

inline IRadTypeInfoEnum::IRadTypeInfoEnum( ) :
    radRefCount( 0 )
{
    radObjectListCreate( & m_lstIntLiteralsPtr, GetThisAllocator( ) );
}

inline IRadTypeInfoEnum * radTypeInfoEnumCreate( radMemoryAllocator allocator )
{
    return new ( allocator ) IRadTypeInfoEnum( );
}

inline IRadTypeInfoEnum::~IRadTypeInfoEnum( void )
{
}

inline void IRadTypeInfoEnum::Initialize( const char * pName )
{
    // copy name
    m_uHashedName = ::radMakeKey32( pName );
    ::radTypeInfoSystemGet( )->AddString( m_uHashedName, pName );
}

inline void IRadTypeInfoEnum::AddIntLiteral( IRadTypeInfoIntLiteral * pIntLiteral )
{
    m_lstIntLiteralsPtr->AddObject( pIntLiteral );
}

inline const char *  IRadTypeInfoEnum::GetName( void )
{
    return ::radTypeInfoSystemGet( )->LookupString( m_uHashedName );
}

inline unsigned int IRadTypeInfoEnum::GetHashedName( void )
{
    return m_uHashedName;
}

inline unsigned int  IRadTypeInfoEnum::GetNumberOfIntLiteral( void )
{
    return m_lstIntLiteralsPtr->GetSize( );
}

inline IRadTypeInfoIntLiteral * IRadTypeInfoEnum::GetIntLiteralPtrAt( unsigned int nIndex )
{
    return static_cast< IRadTypeInfoIntLiteral * >( m_lstIntLiteralsPtr->GetAt( nIndex ) );
}

inline int IRadTypeInfoEnum::GetValueByName( const char * pName )
{
    unsigned int uHashedName = ::radMakeKey32( pName );
    radRef< IRadTypeInfoIntLiteral > pIntLiteral;
    for ( unsigned int i = 0; i < m_lstIntLiteralsPtr->GetSize( ); i ++ )
    {
        pIntLiteral = static_cast< IRadTypeInfoIntLiteral * >( m_lstIntLiteralsPtr->GetAt( i ) );
        if ( pIntLiteral->GetHashedName( ) == uHashedName )
        {
            return pIntLiteral->GetValue( );
        }
    }

#ifdef RADSCRIPT_DEBUG

    char szBuffer[ 256 ];

    sprintf( szBuffer, "radScript: Cannot find Integer Literal named [%s] in Enumeration [%s].\n",
             pName, GetName( ) );
    rDebugString( szBuffer );

    rAssert( 0 );
#endif 
    return 0;
}

#ifdef RADSCRIPT_DEBUG
inline void IRadTypeInfoEnum::DebugDump( void )
{
    char szBuffer[ 256 ];

    sprintf( szBuffer, "Enum: Name:[%s], Number of Integer Literal: [%i]\n",
        GetName( ), GetNumberOfIntLiteral( ) );
    rDebugString( szBuffer );

    for ( unsigned int i = 0; i < m_lstIntLiteralsPtr->GetSize( ); i ++ )
    {
        static_cast< IRadTypeInfoIntLiteral * >( m_lstIntLiteralsPtr->GetAt( i ) )->DebugDump( );
    }
}
#endif

//
//
// Int Literal
//
//

//========================================================================
// IRadTypeInfoIntLiteral::IRadTypeInfoIntLiteral
//========================================================================

inline IRadTypeInfoIntLiteral::IRadTypeInfoIntLiteral( ) :
    radRefCount( 0 ),
    m_uHashedName( 0 ),
    m_nValue( 0 )
{
}

//========================================================================
// IRadTypeInfoIntLiteral::~IRadTypeInfoIntLiteral
//========================================================================

inline IRadTypeInfoIntLiteral::~IRadTypeInfoIntLiteral( void )
{
}

//========================================================================
// IRadTypeInfoIntLiteral::Initialize
//========================================================================

inline void IRadTypeInfoIntLiteral::Initialize( const char * szName, int nValue )
{
    // copy name
    m_uHashedName = ::radMakeKey32( szName );
    ::radTypeInfoSystemGet( )->AddString( m_uHashedName, szName );

    m_nValue = nValue;
}

//========================================================================
// IRadTypeInfoIntLiteral::GetName
//========================================================================

inline const char * IRadTypeInfoIntLiteral::GetName( void )
{
    return ::radTypeInfoSystemGet( )->LookupString( m_uHashedName );
}

#ifdef RADSCRIPT_DEBUG
//========================================================================
// IRadTypeInfoIntLiteral::DebugDump
//========================================================================

inline void IRadTypeInfoIntLiteral::DebugDump( void )
{
    char buf[ 255 ];

    sprintf
    (
        buf,
        "       Integer Iteral:  Name:[%s] Value: [%i]\n",
        GetName( ),
        GetValue( )
    );

    rDebugString( buf );
}
#endif

//============================================================================
// ::radTypeInfoIntLiteralCreate
//============================================================================

inline IRadTypeInfoIntLiteral *  radTypeInfoIntLiteralCreate( radMemoryAllocator allocator )
{
    return new ( allocator ) IRadTypeInfoIntLiteral( );
}

//
//
//
// Param
//
//
//

//========================================================================
// IRadTypeInfoParam::IRadTypeInfoParam
//========================================================================
inline IRadTypeInfoParam::IRadTypeInfoParam( ) :
    radRefCount( 0 ),
    m_uHashedType( 0 ),
    m_nIndLvl( 0 )
{
}

//========================================================================
// IRadTypeInfoParam::Initialize
//========================================================================
inline void IRadTypeInfoParam::Initialize( const char * szName, const char * szType, int nIndLvl )
{
    m_uHashedName = ::radMakeKey32( szName );
    ::radTypeInfoSystemGet( )->AddString( m_uHashedName, szName );

    m_uHashedType = ::radMakeKey32( szType );
    ::radTypeInfoSystemGet( )->AddString( m_uHashedType, szType );

    m_nIndLvl = nIndLvl;
}

//========================================================================
// IRadTypeInfoParam::GetName
//========================================================================
inline const char * IRadTypeInfoParam::GetName( void )
{
    return ::radTypeInfoSystemGet( )->LookupString( m_uHashedName );
}

//========================================================================
// IRadTypeInfoParam::GetHashedName
//========================================================================
inline unsigned int IRadTypeInfoParam::GetHashedName( void )
{
    return m_uHashedName;
}

//========================================================================
// IRadTypeInfoParam::GetType
//========================================================================
inline const char * IRadTypeInfoParam::GetType( void )
{
    return ::radTypeInfoSystemGet( )->LookupString( m_uHashedType );
}

//========================================================================
// IRadTypeInfoParam::GetHashedType
//========================================================================
inline unsigned int IRadTypeInfoParam::GetHashedType( void )
{
    return m_uHashedType;
}

//========================================================================
// IRadTypeInfoParam::GetIndLvl
//========================================================================
inline int IRadTypeInfoParam::GetIndLvl( void )
{
    return m_nIndLvl;
}

//========================================================================
// IRadTypeInfoParam::CompareType
//========================================================================
inline bool IRadTypeInfoParam::CompareType( IRadTypeInfoParam * pParam )
{
    if ( ( GetHashedType( ) == pParam->GetHashedType( ) ) &&
         ( GetIndLvl( ) == pParam->GetIndLvl( ) ) )
    {
        return true;
    }

    return false;
}

#ifdef RADSCRIPT_DEBUG
//========================================================================
// IRadTypeInfoParam::DebugDump
//========================================================================
inline void IRadTypeInfoParam::DebugDump( void )
{
    char buf[ 256 ];
    sprintf( buf, "    Param: Type [%s]; IndLvl [%i].\n",
             GetType( ),
             GetIndLvl( ) );

    rDebugString( buf );
}
#endif

//========================================================================
// radTypeInfoParamCreate
//========================================================================
inline IRadTypeInfoParam *  radTypeInfoParamCreate( radMemoryAllocator allocator )
{
    return new ( allocator ) IRadTypeInfoParam( );
}


//
//
//
// Method
//
//
//

//========================================================================
// IRadTypeInfoMethod::IRadTypeInfoMethod
//========================================================================

inline IRadTypeInfoMethod::IRadTypeInfoMethod( ) :
    radRefCount( 0 ),
    m_VTableOffset( 0xDEADBEEF )
{
    ::radObjectListCreate( & m_lstParamsPtr, GetThisAllocator( ) );
}

//========================================================================
// IRadTypeInfoMethod::~IRadTypeInfoMethod
//========================================================================
inline IRadTypeInfoMethod::~IRadTypeInfoMethod( void )
{
}

//========================================================================
// IRadTypeInfoMethod::Initialize
//========================================================================
inline void IRadTypeInfoMethod::Initialize
(
    const char * pName,
    unsigned int vTableOffset,
    IRadTypeInfoParam * pResult
)
{
    // copy name
    m_uHashedName = ::radMakeKey32( pName );
    ::radTypeInfoSystemGet( )->AddString( m_uHashedName, pName );

    m_VTableOffset = vTableOffset;

    // copy return parameter
    m_pReturnParam = pResult;
}

//========================================================================
// IRadTypeInfoMethod::AddParamPtr
//========================================================================
inline void IRadTypeInfoMethod::AddParamPtr( IRadTypeInfoParam * pParam )
{
    rAssert( pParam );
    m_lstParamsPtr->AddObject( pParam );
}

//========================================================================
// IRadTypeInfoMethod::GetName
//========================================================================
inline const char * IRadTypeInfoMethod::GetName( void )
{
    return ::radTypeInfoSystemGet( )->LookupString( m_uHashedName );
}

//========================================================================
// IRadTypeInfoMethod::GetName
//========================================================================
inline unsigned int IRadTypeInfoMethod::GetHashedName( void )
{
    return m_uHashedName;
}

//========================================================================
// IRadTypeInfoMethod::GetNumberOfParams
//========================================================================
inline unsigned int IRadTypeInfoMethod::GetNumberOfParams( void )
{
    return m_lstParamsPtr->GetSize( );
}

//========================================================================
// IRadTypeInfoMethod::GetParamInfo
//========================================================================
inline IRadTypeInfoParam * IRadTypeInfoMethod::GetParamInfoAt( unsigned int nParamIndex )
{
    return static_cast< IRadTypeInfoParam * >( m_lstParamsPtr->GetAt( nParamIndex ) );
}

//========================================================================
// IRadTypeInfoMethod::GetReturnInfo
//========================================================================
inline IRadTypeInfoParam * IRadTypeInfoMethod::GetReturnInfo( void )
{
    rAssert( m_pReturnParam );
    return m_pReturnParam;
}

//========================================================================
// IRadTypeInfoMethod::GetVTableOffset
//========================================================================
inline unsigned int IRadTypeInfoMethod::GetVTableOffset( void )
{
    return m_VTableOffset;
}

//========================================================================
// IRadTypeInfoMethod::DebugDump
//========================================================================
#ifdef RADSCRIPT_DEBUG
inline void IRadTypeInfoMethod::DebugDump( void )
{
    radRef< IRadTypeInfoParam > pParam;

    char buf[ 256 ];
    sprintf( buf, "    Method: Name: [%s] VTable Offset: [0x%x]\n", GetName( ), GetVTableOffset( ) );
    rDebugString( buf );

    for( unsigned int i = 0; i < GetNumberOfParams( ); i ++ )
    {
        pParam = GetParamInfoAt( i );
        pParam->DebugDump( );
    }
}
#endif


//============================================================================
// ::radTypeInfoCreateMethod
//============================================================================
inline IRadTypeInfoMethod * radTypeInfoMethodCreate( radMemoryAllocator allocator )
{
    return new ( allocator ) IRadTypeInfoMethod( );
}


//
//
// type Info
//
//
//

inline IRadTypeInfo::IRadTypeInfo( radMemoryAllocator allocator )
    :
    radRefCount( 0 ),
    m_ThisAllocator( allocator )
{
    radObjectListCreate( & m_xIOl_Methods, allocator );
    radObjectListCreate( & m_xIOl_Enums, allocator );
}

//========================================================================
// IRadTypeInfo::IRadTypeInfo
//========================================================================

inline IRadTypeInfo::~IRadTypeInfo( void )
{
}

//========================================================================
// IRadTypeInfo::Initialize
//========================================================================

inline void IRadTypeInfo::Initialize( const char * pName )
{
    // copy name
    m_uHashedName = ::radMakeKey32( pName );
    ::radTypeInfoSystemGet( )->AddString( m_uHashedName, pName );
}

//========================================================================
// IRadTypeInfo::GetName
//========================================================================

inline const char * IRadTypeInfo::GetName( void )
{
    return ::radTypeInfoSystemGet( )->LookupString( m_uHashedName );
}

//========================================================================
// IRadTypeInfo::GetHashedName
//========================================================================

inline unsigned int IRadTypeInfo::GetHashedName( void )
{
    return m_uHashedName;
}

//========================================================================
// IRadTypeInfo::GetNumberOfMethods
//========================================================================

inline unsigned int IRadTypeInfo::GetNumberOfMethods( void )
{
    return m_xIOl_Methods->GetSize( );
} 
//========================================================================
// IRadTypeInfo::AddMethod
//========================================================================
   
inline void IRadTypeInfo::AddMethod( IRadTypeInfoMethod * pIRadTypeInfoMethod )
{
    m_xIOl_Methods->AddObject( (IRefCount*) pIRadTypeInfoMethod );
} 

//========================================================================
// IRadTypeInfo::GetMethodByIndex
//========================================================================

inline IRadTypeInfoMethod * IRadTypeInfo::GetMethodByIndex( unsigned int i )
{
    return static_cast< IRadTypeInfoMethod * >( m_xIOl_Methods->GetAt( i ) );
}

//========================================================================
// IRadTypeInfo::GetMethodByName
//========================================================================

inline IRadTypeInfoMethod * IRadTypeInfo::GetMethodByName( const char * pName )
{
    IRadTypeInfoMethod * pITim = NULL;
    unsigned int uHashedName = ::radMakeKey32( pName );

    for( unsigned int i = 0; i < m_xIOl_Methods->GetSize( ); i ++ )
    {
        pITim = static_cast< IRadTypeInfoMethod * >( m_xIOl_Methods->GetAt( i ) );

        if ( pITim->GetHashedName( ) == uHashedName )
        {
            return pITim;
        }
    }

    return NULL;
}

//========================================================================
// IRadTypeInfo::GetNumberOfEnums
//========================================================================

inline unsigned int IRadTypeInfo::GetNumberOfEnums( void )
{
    return m_xIOl_Enums->GetSize( );
}

//========================================================================
// IRadTypeInfo::GetNumberOfEnums
//========================================================================

inline IRadTypeInfoEnum * IRadTypeInfo::GetEnumByIndex( unsigned int index )
{
    return static_cast< IRadTypeInfoEnum * >( m_xIOl_Enums->GetAt( index ) );
}

//========================================================================
// IRadTypeInfo::GetEnumByName  Linear search dohhhhhh!!!
//========================================================================

inline IRadTypeInfoEnum * IRadTypeInfo::GetEnumByName( const char * pName )
{
    radRef< IRadTypeInfoEnum > pTypeInfoEnum = NULL;
    unsigned int uEnumName = ::radMakeKey32( pName );

    for (unsigned int i = 0; i < m_xIOl_Enums->GetSize( ); i++)
    {
        pTypeInfoEnum = static_cast< IRadTypeInfoEnum * >( m_xIOl_Enums->GetAt( i ) );

        if ( pTypeInfoEnum->GetHashedName( ) == uEnumName )
        {
            return pTypeInfoEnum;
        }
    }
    return NULL;
}

//========================================================================
// IRadTypeInfo::AddEnum
//========================================================================

inline void IRadTypeInfo::AddEnum( IRadTypeInfoEnum * pIRadTypeInfoEnum )
{
    m_xIOl_Enums->AddObject( pIRadTypeInfoEnum );
}

#ifdef RADSCRIPT_DEBUG
inline void IRadTypeInfo::DebugDump( void )
{
    char buf[ 255 ];

    sprintf
    (
        buf,
        "TypeInfo:  Name:[%s] NumMethods: [0x%x]\n",
        GetName( ),
        GetNumberOfMethods( )
    );

    rDebugString( buf );

    for( unsigned int i = 0; i < m_xIOl_Methods->GetSize( ); i ++ )
    {
        static_cast< IRadTypeInfoMethod * >( m_xIOl_Methods->GetAt( i ) )->DebugDump( );
    }

    for( unsigned int j = 0; j < m_xIOl_Enums->GetSize( ); j ++ )
    {
        static_cast< IRadTypeInfoEnum * >( m_xIOl_Enums->GetAt( j ) )->DebugDump( );
    }
}
#endif

//============================================================================
// ::radTypeInfoCreate
//============================================================================

inline IRadTypeInfo * radTypeInfoCreate( radMemoryAllocator allocator )
{
    return new ( allocator ) IRadTypeInfo( allocator );
}


//
//
// Type info system
//
//

inline IRadTypeInfoSystem::IRadTypeInfoSystem( radMemoryAllocator allocator ) :
    radRefCount( 0 ),
    m_ThisAllocator( allocator ),
#ifdef RADSCRIPT_DEBUG
    m_bLoadString( true )
#else
    m_bLoadString( false )
#endif
{
    // singalton object check
    rAssert( s_pTheTypeInfoSystem == NULL );

    ::radObjectListCreate( & m_xIOl_TypeInfo, m_ThisAllocator );
    ::radObjectListCreate( & m_xIOl_Enum, m_ThisAllocator );
    m_xIOt_StringTable = new( m_ThisAllocator ) radObjectBTree( );

    s_pTheTypeInfoSystem = this;
}

//========================================================================
// IRadTypeInfoSystem::~IRadTypeInfoSystem
//========================================================================

inline IRadTypeInfoSystem::~IRadTypeInfoSystem( void )
{

}

//========================================================================
// IRadTypeInfoSystem::LoadTypeInfo
//========================================================================

inline void IRadTypeInfoSystem::LoadTypeInfo( radMemoryAllocator alloc, const char * pFileName, TypeInfoLoadCallback *pCb, void * pUserData )
{
    radRef< IRadTypeInfoLoader > xIRadTypeInfoLoader( ::radTypeInfoLoaderCreate( RADMEMORY_ALLOC_TEMP ) );

    xIRadTypeInfoLoader->Initialize( this, alloc, pFileName, pCb, pUserData );
}

//========================================================================
// IRadTypeInfoSystem::GetTypeInfo
//========================================================================

inline IRadTypeInfo * IRadTypeInfoSystem::GetTypeInfo( IRefCount * pIRefCount )
{
    rAssert( pIRefCount != NULL );

    if ( pIRefCount != NULL )
    {
        //
        // Figure out what class this thing is
        //

        const char * pName = typeid( *pIRefCount).name();
    
        rAssert( pName != NULL );

        char interfaceName[ 128 ]; 
        const char * pClassName = NULL;
    
        #if defined( RAD_WIN32 ) || defined( RAD_UWP )
        {
            //
            // The pName takes the form "class Scope::ClassName" or
            // "struct Scope::StructName".
            // We will strip out the scope and the class string so
            // that we only have the class name.
            //
            unsigned int uStrIdx = 0;
            pClassName = pName;
            while( pName[ uStrIdx ] != '\0' )
            {
                if
                (
                    ( pName[ uStrIdx ] == ':' ) ||
                    ( pName[ uStrIdx ] == ' ' )
                )
                {
                    pClassName = pName + uStrIdx + 1;
                    rAssert( *pClassName != '\0' );
                }
                uStrIdx++;
            }
            interfaceName[ 0 ] = 'I';
            strcpy( interfaceName + 1, pClassName );
        }
        #endif

        interfaceName[ 1 ] = toupper( interfaceName[ 1 ] );                                             

        //
        // find the typeinfo with the same name
        //
        unsigned int nHashedName = ::radMakeKey32( interfaceName );
        radRef< IRadTypeInfo > pIRadTypeInfo = NULL;

        for( unsigned int i = 0; i < m_xIOl_TypeInfo->GetSize( ); i++ )
        {
            pIRadTypeInfo = static_cast< IRadTypeInfo * >( m_xIOl_TypeInfo->GetAt( i ) );

            if ( pIRadTypeInfo->GetHashedName( ) == nHashedName )
            {
                return pIRadTypeInfo;
            }      
        }
    }

    return NULL;
}

//========================================================================
// IRadTypeInfoSystem::GetTypeInfoMethod
//========================================================================

inline IRadTypeInfoMethod * IRadTypeInfoSystem::GetTypeInfoMethod( IRefCount * pIRefCount, const char * pMethodName )
{
    radRef< IRadTypeInfo > xITi = GetTypeInfo( pIRefCount );

    if ( xITi != NULL )
    {
        return xITi->GetMethodByName( pMethodName );
    }

    return NULL;
}
             
//========================================================================
// IRadTypeInfoSystem::AddTypeInfo
//========================================================================

inline void IRadTypeInfoSystem::AddTypeInfo( IRadTypeInfo * pIRadTypeInfo )
{
    m_xIOl_TypeInfo->AddObject( static_cast< IRefCount * >( pIRadTypeInfo ) );
}

//========================================================================
// IRadTypeInfoSystem::AddEnum
//========================================================================

inline void IRadTypeInfoSystem::AddEnum( IRadTypeInfoEnum * pIRadTypeInfoEnum )
{
    m_xIOl_Enum->AddObject( static_cast< IRefCount * >( pIRadTypeInfoEnum ) );
}

//========================================================================
// IRadTypeInfoSystem::IRadTypeInfoSystem
//========================================================================

int IRadTypeInfoSystem::GetTypeInfoCount( void )
{
    return m_xIOl_TypeInfo->GetSize( );
}

//========================================================================
// IRadTypeInfoSystem::GetTypeInfoAt
//========================================================================

inline IRadTypeInfo * IRadTypeInfoSystem::GetTypeInfoAt( unsigned int index )
{
    return static_cast< IRadTypeInfo * >( m_xIOl_TypeInfo->GetAt( index ) );
}

//========================================================================
// IRadTypeInfoSystem::IRadTypeInfoSystem
//========================================================================

inline int IRadTypeInfoSystem::GetEnumCount( void )
{
    return m_xIOl_Enum->GetSize( );
}

//========================================================================
// IRadTypeInfoSystem::GetEnumAt
//========================================================================

inline IRadTypeInfoEnum * IRadTypeInfoSystem::GetEnumAt( unsigned int index )
{
    return static_cast< IRadTypeInfoEnum * >( m_xIOl_Enum->GetAt( index ) );
}

//========================================================================
// IRadTypeInfoSystem::IRadTypeInfoSystem
//========================================================================

inline bool IRadTypeInfoSystem::GetLoadString( void )
{
    return m_bLoadString;
}

//========================================================================
// IRadTypeInfoSystem::SetLoadString
//========================================================================

inline void IRadTypeInfoSystem::SetLoadString( bool bLoadString )
{
    m_bLoadString = bLoadString;
}

//========================================================================
// IRadTypeInfoSystem::EmptyStrTbl
//========================================================================

inline void IRadTypeInfoSystem::EmptyStrTbl( )
{
    m_xIOt_StringTable->RemoveAll( );
}

#ifdef RADSCRIPT_DEBUG
//========================================================================
// IRadTypeInfoSystem::DebugDump
//========================================================================

inline void IRadTypeInfoSystem::DebugDump( void )
{
    rDebugString( "IRadTypeInfoSystem Dump:\n" );
    rDebugString( "--------------------\n" );

    IRadTypeInfo * pIRadTypeInfo;

    for ( unsigned int i = 0; i < m_xIOl_TypeInfo->GetSize( ); i ++ )
    {
        pIRadTypeInfo = static_cast< IRadTypeInfo * >( m_xIOl_TypeInfo->GetAt( i ) );
        pIRadTypeInfo->DebugDump( );
    }
}
#endif

#endif // TYPEINFO_HPP


