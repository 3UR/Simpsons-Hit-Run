//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/file.hpp>
#include <p3d/chunkfile.hpp>
#include <constants/chunks.h>
#include <constants/chunkids.hpp>
#include <constants/atenum.hpp>
#include <p3d/anim/vertexanimkey.hpp>
#include <p3d/error.hpp>

#define MAX_NAME_LEN    128

static const unsigned int VERTEXANIM_VERSION = 0x00;

tOffsetIndexList::tOffsetIndexList( )
: mIndex( NULL ), mNumber( 0 )
{
}

tOffsetIndexList::~tOffsetIndexList( )
{
    if( mIndex )
        delete [ ] mIndex;
}

void tOffsetIndexList::CreateIndex( int num )
{
    if( mIndex )
        delete [ ] mIndex;

    mIndex = new int[ num ];
    mNumber = num;

    P3DASSERT( mIndex );
}

tColourOffsetList::tColourOffsetList( )
: mColour( NULL )
{
}


tColourOffsetList::~tColourOffsetList( )
{
    if( mColour )
        delete [ ] mColour;
}

void tColourOffsetList::CreateOffsets( int num )
{
    if( mColour )
        delete [ ] mColour;

    mColour = new tColourOffset[ num ];
    mNumber = num;

    P3DASSERT( mColour );
}

tVectorOffsetList::tVectorOffsetList( )
: mVector( NULL )
{
}

tVectorOffsetList::~tVectorOffsetList( )
{
    if( mVector )
        delete [ ] mVector;
}

void tVectorOffsetList::CreateOffsets( int num )
{
    if( mVector )
        delete [ ] mVector;

    mVector = new rmt::Vector[ num ];
    mNumber = num;

    P3DASSERT( mVector );
}


tVector2OffsetList::tVector2OffsetList( )
: mVector2( NULL )
{
}


tVector2OffsetList::~tVector2OffsetList( )
{
    if( mVector2 )
        delete [ ] mVector2;
}

void tVector2OffsetList::CreateOffsets( int num )
{
    if( mVector2 )
        delete [ ] mVector2;

    mVector2 = new rmt::Vector2[ num ];
    mNumber = num;

    P3DASSERT( mVector2 );
}

tVertexAnimKey::tVertexAnimKey( tVertexAnimKey *k, float delta )
{
    if( k->mColour ){
        mColour = new tColourOffsetList( );
        mColour->CreateOffsets( k->mColour->mNumber );
        if( k->mColour->mIndex )
            mColour->CreateIndex( k->mColour->mNumber );
        for( int i = 0; i < mColour->mNumber; ++i ){
            if( k->mColour->mIndex )
                mColour->mIndex[ i ] = k->mColour->mIndex[ i ];
            mColour->mColour[ i ].red = (short)(delta*k->mColour->mColour[ i ].red);
            mColour->mColour[ i ].green = (short)(delta*k->mColour->mColour[ i ].green);
            mColour->mColour[ i ].blue = (short)(delta*k->mColour->mColour[ i ].blue);
            mColour->mColour[ i ].alpha = (short)(delta*k->mColour->mColour[ i ].alpha);
        }
    }

    if( k->mNormal ){
        mNormal = new tVectorOffsetList( );
        mNormal->CreateOffsets( k->mNormal->mNumber );
        if( k->mNormal->mIndex )
            mNormal->CreateIndex( k->mNormal->mNumber );
        for( int i = 0; i < mNormal->mNumber; ++i ){
            if( k->mNormal->mIndex )
                mNormal->mIndex[ i ] = k->mNormal->mIndex[ i ];
            mNormal->mVector[ i ] = k->mNormal->mVector[ i ];
            mNormal->mVector[ i ].Scale( delta );
        }
    }

    if( k->mPosition ){
        mPosition = new tVectorOffsetList( );
        mPosition->CreateOffsets( k->mPosition->mNumber );
        if( k->mPosition->mIndex )
            mPosition->CreateIndex( k->mPosition->mNumber );
        for( int i = 0; i < mPosition->mNumber; ++i ){
            if( k->mPosition->mIndex )
                mPosition->mIndex[ i ] = k->mPosition->mIndex[ i ];
            mPosition->mVector[ i ] = k->mPosition->mVector[ i ];
            mPosition->mVector[ i ].Scale( delta );
        }
    }

    for( int j = 0; j < 4; ++j ){
        if( k->mUV[ j ] ){
            mUV[ j ] = new tVector2OffsetList( );
            mUV[ j ]->CreateOffsets( k->mUV[ j ]->mNumber );
            if( k->mUV[ j ]->mIndex )
                mUV[ j ]->CreateIndex( k->mUV[ j ]->mNumber );
            for( int i = 0; i < mUV[ j ]->mNumber; ++i ){
                if( k->mUV[ j ]->mIndex )
                    mUV[ j ]->mIndex[ i ] = k->mUV[ j ]->mIndex[ i ];
                mUV[ j ]->mVector2[ i ].u = k->mUV[ j ]->mVector2[ i ].u * delta;                
                mUV[ j ]->mVector2[ i ].v = k->mUV[ j ]->mVector2[ i ].v * delta;                
            }
        }
    }
}

tVertexAnimKey::tVertexAnimKey( )
: mColour(NULL), mPosition(NULL), mNormal(NULL)
{

    for( int i = 0; i < 4; ++i )
        mUV[ i ] = NULL;
}

tVertexAnimKey::~tVertexAnimKey( )
{
    if( mColour )
        mColour->Release( );

    if( mPosition )
        mPosition->Release( );

    if( mNormal )
        mNormal->Release( );

    for( int i = 0; i < 4; ++i )
        if( mUV[ i ] )
            mUV[ i ]->Release( );
}

///
///     given the frame number(myframe), create the key
///     whose frame number is targetf
///
tVertexAnimKey * tVertexAnimKey::Interpolate( float targetf, float myframe )
{
    float delta = targetf/myframe;

    tVertexAnimKey *key = new tVertexAnimKey( this, delta );
    key->AddRef( );

    return key;
}

//------------------------------------Vertex Animation Key Loader-----------------------------
tVertexAnimKeyLoader::tVertexAnimKeyLoader( )
:tSimpleChunkHandler( Pure3D::Animation::VertexAnim::KEY )
{
}

tEntity* tVertexAnimKeyLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    tVertexAnimKey *key = new tVertexAnimKey( );

    P3DASSERT( key );

    unsigned version = f->GetUInt( );
    P3DASSERT( version == VERTEXANIM_VERSION );

    char name[ MAX_NAME_LEN ];
    
    f->GetPString( name );
    key->SetName( name );

    while( f->ChunksRemaining( ) ){
        f->BeginChunk( );

        int id = f->GetCurrentID( ); 
        int param;
        tOffsetListLoader loader;
        switch( id ){
            case Pure3D::Animation::VertexAnim::OffsetList::COLOUR:
            {                
                tColourOffsetList *cl = (tColourOffsetList *)loader.LoadObject( f, store, id, param );
                P3DASSERT( cl );
                cl->AddRef( );
                key->mColour = cl;
                break;
            }

            case Pure3D::Animation::VertexAnim::OffsetList::VECTOR:
            {
                tVectorOffsetList *vl = (tVectorOffsetList *)loader.LoadObject( f, store, id, param );
                P3DASSERT( vl );
                vl->AddRef( );
                if( param == (int)Pure3DVertexAnim::Offset::Coord )
                    key->mPosition = vl;            
                else if( param == (int)Pure3DVertexAnim::Offset::Normal )
                    key->mNormal = vl;
                break;
            }
    
            case Pure3D::Animation::VertexAnim::OffsetList::VECTOR2:
            {
                tVector2OffsetList *vl = (tVector2OffsetList *)loader.LoadObject( f, store, id, param );
                P3DASSERT( vl );
                vl->AddRef( );

                switch( param ){
                    case Pure3DVertexAnim::Offset::UV0:
                        key->mUV[ 0 ] = vl;            
                    break;

                    case Pure3DVertexAnim::Offset::UV1:
                        key->mUV[ 1 ] = vl;            
                    break;

                    case Pure3DVertexAnim::Offset::UV2:
                        key->mUV[ 2 ] = vl;            
                    break;

                    case Pure3DVertexAnim::Offset::UV3:
                        key->mUV[ 3 ] = vl;            
                    break;

                    default:
                        P3DASSERT( 0 );
                }
                break;

            }

            default:
                P3DASSERT( 0 );
        }
        
        f->EndChunk( );     //finish reading a sub-chunk
    }   
    
    return key;
}


//-----------------------------tOffsetListLoader--------------------------------
tOffsetIndexList * tOffsetListLoader::LoadObject( tChunkFile *f, tEntityStore *store, unsigned id, int &param )
{
    tOffsetIndexList *list = NULL;
    switch( id ){
        case Pure3D::Animation::VertexAnim::OffsetList::COLOUR:
        {                               
            list = LoadColourOffsetList( f, store );
            break;
        }
            
        case Pure3D::Animation::VertexAnim::OffsetList::VECTOR:
        {
            list = LoadVectorOffsetList( f, store, param );;
            break;
        }

        case Pure3D::Animation::VertexAnim::OffsetList::VECTOR2:
        {
            list = LoadVector2OffsetList( f, store, param );;
            break;
        }

        default:
            P3DASSERT( 0 );
    }
    
    //load in index list in case current chunk has one
    while( f->ChunksRemaining( ) ){
        f->BeginChunk( );

        unsigned id = f->GetCurrentID( );

        P3DASSERT( id == Pure3D::Animation::VertexAnim::OffsetList::INDEX );

        unsigned version = f->GetUInt( );
        P3DASSERT( version == VERTEXANIM_VERSION );

        int num = f->GetLong( );
        list->CreateIndex( num );

        for( int i = 0; i < num; ++i ){
            f->GetData( &list->mIndex[ i ], 1, tFile::DWORD );          
        }
        f->EndChunk( );
    }

    return list;
}

tOffsetIndexList *tOffsetListLoader::LoadVectorOffsetList( tChunkFile *f, tEntityStore *store, int &param )
{    
    unsigned version = f->GetUInt( );
    P3DASSERT( version == VERTEXANIM_VERSION );

    int num = f->GetLong( );

    tVectorOffsetList *cl = new tVectorOffsetList;

    P3DASSERT( cl );

    cl->CreateOffsets( num );

    param = f->GetLong( );

    for( int i = 0; i < num; ++i ){
        f->GetData( &cl->mVector[ i ], 3, tFile::DWORD );              
    }

    return cl;
}

tOffsetIndexList *tOffsetListLoader::LoadVector2OffsetList( tChunkFile *f, tEntityStore *store, int &param )
{
    unsigned version = f->GetUInt( );
    P3DASSERT( version == VERTEXANIM_VERSION );

    int num = f->GetLong( );

    tVector2OffsetList *cl = new tVector2OffsetList;
    P3DASSERT( cl );

    cl->CreateOffsets( num );

    param = f->GetLong( );

    for( int i = 0; i < num; ++i ){
        f->GetData( &cl->mVector2[ i ], 2, tFile::DWORD );            
    }

    return cl;
}

tOffsetIndexList *tOffsetListLoader::LoadColourOffsetList( tChunkFile *f, tEntityStore *store )
{
    unsigned version = f->GetUInt( );
    P3DASSERT( version == VERTEXANIM_VERSION );

    int num = f->GetLong( );
    tColourOffsetList *cl = new tColourOffsetList;

    cl->CreateOffsets( num );
    
    for( int i = 0; i < num; ++i )
        f->GetData( &cl->mColour[ i ], 4, tFile::WORD );   
    
    return cl;
}
