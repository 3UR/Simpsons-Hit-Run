//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//============================================================================
// Include Files
//============================================================================

#include "pch.hpp"
#include <radtypeinfo.hpp>
#include <radobject.hpp>
#include <radfile.hpp>

#ifdef RADSCRIPT_DEBUG
static const char dc[] = "radTypeInfoLoader";
#endif 

static inline unsigned int radEndianLittleToPlat( unsigned int in )
{
	return in;	
}

//========================================================================
// Compoenent::radTypeInfoLoader
//========================================================================

struct radTypeInfoLoader
    :
    public IRadTypeInfoLoader,
	public IRadFileCompletionCallback,
	public radRefCount
{

    IMPLEMENT_REFCOUNTED( "radTypeInfoLoader" )

    //========================================================================
    // Data Members
    //========================================================================
    
    radMemoryAllocator     m_TypeInfoAllocator;
    
    TypeInfoLoadCallback * m_pTypeInfoLoadCallback;
    void                 * m_pTypeInfoLoadCallbackUserData;

    char                 * m_pTypeInfoMemory;
    unsigned int           m_TypeInfoMemorySize;
    
    radRef< IRadTypeInfoSystem > m_xIRadTypeInfoSystem;

	radRef< IRadFile >        m_pIRadFile;

    bool                   m_bLoadEnum;

	enum FileLoadState
	{
		GetSize,
		ReadFile,
		None
	};

    //========================================================================
    // radTypeInfoLoader::radTypeInfoLoader
    //========================================================================

    radTypeInfoLoader( void )
        :
        radRefCount( 0 ),
        m_TypeInfoAllocator( RADMEMORY_ALLOC_DEFAULT ),
        m_pTypeInfoLoadCallback( NULL ),
        m_pTypeInfoLoadCallbackUserData( NULL ),
        m_pTypeInfoMemory( NULL ),
        m_TypeInfoMemorySize( 0 ),
		m_pIRadFile( NULL ),
        m_bLoadEnum( false )
    {

    }

    //========================================================================
    // radTypeInfoLoader::Initialize
    //========================================================================

    virtual void Initialize
    (
        IRadTypeInfoSystem * pITis,
        radMemoryAllocator allocator,
        const char * pFileName,
        TypeInfoLoadCallback * pCb,
        void * pUserData,
        bool bLoadEnum = false 
    )
    {
        rAssert( pCb != NULL );

        m_TypeInfoAllocator = allocator;
        m_pTypeInfoLoadCallback = pCb;
        m_pTypeInfoLoadCallbackUserData = pUserData;
        m_xIRadTypeInfoSystem = pITis;

        m_bLoadEnum = bLoadEnum;

        radFileOpen( &m_pIRadFile, pFileName, false, OpenExisting, NormalPriority, 0, RADMEMORY_ALLOC_TEMP );		
        m_pIRadFile->GetSizeAsync( & m_TypeInfoMemorySize );
		m_pIRadFile->AddCompletionCallback( this, (void*) GetSize );
    }

    //========================================================================
    // radTypeInfoLoader::~radTypeInfoLoader
    //========================================================================

    ~radTypeInfoLoader( void )
    {
        radMemoryFreeAligned( RADMEMORY_ALLOC_TEMP, m_pTypeInfoMemory );
    }
    

    //
    // Help function, read string, and advance the file pointer
    //
    char * ReadString( unsigned int ** p )
    {
        // size of string in byte, always multiple of 4
        unsigned int uStringLength = **p; (*p)++; uStringLength = radEndianLittleToPlat( uStringLength );
        char * pName = reinterpret_cast< char * >( *p );
        (*p) += uStringLength * sizeof( char ) / sizeof( *p );
        return pName;
    }

    //========================================================================
    // radTypeInfoLoader::OnReadAsyncComplete
    //========================================================================

    virtual void OnFileOperationsComplete( void* pUserData )
    {
        ParserConst::TokenType token;
        unsigned int * p = (unsigned int *)m_pTypeInfoMemory;
        
        // we only assume all class is only derived from IRefCount.
        const int nBaseClassMethodCount = 1;

        unsigned int nTotalMethodCount = 0;
        unsigned int methodNumber = 0;

        unsigned int nTotalEnumCount = 0;
        unsigned int enumNumber = 0;

        radRef< IRadTypeInfo > xIRadTypeInfo;

		switch( (FileLoadState) (intptr_t) pUserData )
		{
			case GetSize:
			{
				// GetSizeAsync is complete. Read into aligned memory to improve performance.
                unsigned int readSize = ::radMemoryRoundUp( m_TypeInfoMemorySize, m_pIRadFile->GetOptimalSize( ) );

		        m_pTypeInfoMemory = (char*) radMemoryAllocAligned( RADMEMORY_ALLOC_TEMP, readSize, 128 );

				m_pIRadFile->ReadAsync( m_pTypeInfoMemory, readSize );

				m_pIRadFile->AddCompletionCallback( this, (void*) ReadFile );

				break;
			}
			case ReadFile:
			{
				// ReadAsync is complete

				while ( (char *)p < m_pTypeInfoMemory + m_TypeInfoMemorySize )
				{
					token = static_cast< ParserConst::TokenType >( *p ); p++; token = static_cast< ParserConst::TokenType >( radEndianLittleToPlat( token ) );

					switch (token)
					{
						case ParserConst::TOK_CLASS:
						{
							if ( xIRadTypeInfo != NULL )
							{
								m_xIRadTypeInfoSystem->AddTypeInfo( xIRadTypeInfo );
								xIRadTypeInfo = NULL;
								methodNumber = nBaseClassMethodCount;
							}

							xIRadTypeInfo = ::radTypeInfoCreate( m_TypeInfoAllocator );

                            char * pName = ReadString( &p );

							xIRadTypeInfo->Initialize( pName ); // iid / piid legacy

                            nTotalMethodCount = (*p); p++; nTotalMethodCount = radEndianLittleToPlat( nTotalMethodCount );
                            nTotalMethodCount += nBaseClassMethodCount;
                            nTotalEnumCount   = *p; p ++; nTotalEnumCount = radEndianLittleToPlat( nTotalEnumCount );

                            methodNumber = nBaseClassMethodCount;
                            enumNumber = 0;

                            break;
						}

						case ParserConst::TOK_METHOD:
						{
                            //
                            // Method format layout
                            //
                            // TOK_METHOD           4 bytes
                            // 
                            // Name                 ParserConst::MAX_ID_STRING_SIZE
                            // Argument Count       4 bytes
                            // Return Arg 
                            // Arguments list
                            //
                            radRef< IRadTypeInfoMethod > xIRadTypeInfoMethod ( ::radTypeInfoMethodCreate( m_TypeInfoAllocator ) );

                            char * pName = ReadString( &p );

                            unsigned int numParams = *p; p++; numParams = radEndianLittleToPlat( numParams );

                            char * pParamName;
                            char * pParamType;
                            int nIndLvl;

                            //
                            // Return type
                            //
                            pParamName = ReadString( &p );
                            pParamType = ReadString( &p );
                            nIndLvl = radEndianLittleToPlat( *p ); p ++;

                            radRef< IRadTypeInfoParam > pRTIReturnParam( ::radTypeInfoParamCreate( m_TypeInfoAllocator ) );
                            pRTIReturnParam->Initialize( pParamName, pParamType, nIndLvl );

                            //
                            // Initalize the method
                            //
                            xIRadTypeInfoMethod->Initialize( pName, methodNumber - 1, pRTIReturnParam );

                            //
                            // Parameter type
                            //
                            for ( unsigned int i = 0; i < numParams; i++ )
                            {
                                pParamName = ReadString( &p );
                                pParamType = ReadString( &p );
                                nIndLvl = radEndianLittleToPlat( *p ); p ++;

                                radRef< IRadTypeInfoParam > pRTIParam( ::radTypeInfoParamCreate( m_TypeInfoAllocator ) );
                                pRTIParam->Initialize( pParamName, pParamType, nIndLvl );
                                xIRadTypeInfoMethod->AddParamPtr( pRTIParam );
                            }

							xIRadTypeInfo->AddMethod( xIRadTypeInfoMethod );

							methodNumber++;

                            // reset Total method count to 0 when we got all method for this class
                            if (methodNumber == nTotalMethodCount)
                            {
                                nTotalMethodCount = 0;
                            }
							break;
						}

                        case ParserConst::TOK_ENUM:
						{
                            //
                            // TOK_ENUM
                            // Name
                            // enum count
                            // enum list
                            //
                            char * pName = ReadString( &p );

                            unsigned int numEnum = *p; p += 1; numEnum = radEndianLittleToPlat( numEnum );

                            radRef< IRadTypeInfoEnum > xIRadTypeInfoEnum;

                            if (m_bLoadEnum)
                            {
                                xIRadTypeInfoEnum = ::radTypeInfoEnumCreate( m_TypeInfoAllocator );
    							xIRadTypeInfoEnum->Initialize( pName );
                            }

                            for (unsigned int i = 0; i < numEnum; i++)
                            {
                                char * pIntLiteralName = ReadString( &p );
                                int nIntLiteralValue = radEndianLittleToPlat( *p ); p++;

                                if (m_bLoadEnum)
                                {
                                    radRef< IRadTypeInfoIntLiteral > pRTIIntLiteral( ::radTypeInfoIntLiteralCreate( m_TypeInfoAllocator ) );
                                    pRTIIntLiteral->Initialize( pIntLiteralName, nIntLiteralValue );
                                    xIRadTypeInfoEnum->AddIntLiteral( pRTIIntLiteral );
                                }
                            }

                            if (m_bLoadEnum)
                            {
                                // if current class does not have any more enumeration, then
                                // the enumeration must be belong to global enumeration list.
                                if (nTotalEnumCount > 0)
                                {
                                    // adding enum into current class
							        xIRadTypeInfo->AddEnum( xIRadTypeInfoEnum );
                                }
                                else
                                {
                                    // adding enum into global list
                                    radTypeInfoSystemGet()->AddEnum( xIRadTypeInfoEnum );
                                }
                            }

                            enumNumber++;
                            // reset Total enum count to 0 when we got all enum for this class
                            if (methodNumber == nTotalEnumCount)
                            {
                                nTotalMethodCount = 0;
                            }
                            break;
						}

                        case ParserConst::TOK_VERSION:
						{
							token = static_cast<ParserConst::TokenType>(*p); p++; token = static_cast<ParserConst::TokenType>(radEndianLittleToPlat( token ));

							rAssert( token == TYP_FILE_VERSION );
							break;
						}

                        default:
						{
							rAssert(!"unknown token");
						}
					}
				}

				if ( xIRadTypeInfo != NULL )
				{
					m_xIRadTypeInfoSystem->AddTypeInfo( xIRadTypeInfo );
					xIRadTypeInfo = NULL;
				}

				if ( m_pTypeInfoLoadCallback != NULL )
				{
					m_pTypeInfoLoadCallback( m_pTypeInfoLoadCallbackUserData );
				}        

				break;
			}
			default:
			{
				rAssert( 0 ); // somethings REALLY wrong.
			}
		}
	}
};

//============================================================================
// ::radTypeInfoCreateLoader
//============================================================================

IRadTypeInfoLoader * radTypeInfoLoaderCreate( radMemoryAllocator allocator )
{
    return new ( allocator ) radTypeInfoLoader( );
}
