//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/geometry.hpp>
#include <p3d/anim/vertexanimkey.hpp>
#include <p3d/anim/vertexanimobject.hpp>
#include <p3d/primgroup.hpp>
#include <p3d/error.hpp>
#include <p3d/vertexlist.hpp>

////
///     Offset of a geometry for vertex animation
///
tGeometryOffset::tGeometryOffset( )
:mOffsetsNum( 0 )
{
}

tGeometryOffset::tGeometryOffset(  tGeometry *geo )
{
    mOffsetsNum = geo->GetNumPrimGroup( );
    mOffsets = new tVertexOffsetList*[ mOffsetsNum ];
    for( int i = 0; i < mOffsetsNum; ++i ){
        mOffsets[ i ] = new tVertexOffsetList( );       //create an offset list 
        mOffsets[ i ]->mVertexNum = geo->GetPrimGroup( i )->GetVertexCount( );  //only the vertex number is set, the offset list for each component 
                                                                                //is set by tVertexAnimObject::Update( )
        mOffsets[ i ]->AddRef( );
    }
}

tGeometryOffset::~tGeometryOffset( )
{
    for( int i = 0; i < mOffsetsNum; ++i )
        mOffsets[ i ]->Release( );

    delete[ ]  mOffsets;
}

///
///     offset list for vertexlist used by vertex animation
///
tVertexOffsetList::tVertexOffsetList( )
: mVertexNum( 0 ), mCoord( NULL ), mNormal( NULL ), mColour( NULL ), mMask( 0 )
{
    for( int i = 0; i < 4; ++i )
        mUV[ i ] = NULL;
}

void tVertexOffsetList::SetNormal( int i, rmt::Vector* n )
{
    P3DASSERT( i < mVertexNum );
    if( !mNormal )
        mNormal = new rmt::Vector[ mVertexNum ];

    P3DASSERT( mNormal );            //out of memory

    mNormal[ i ].x = n->x;
    mNormal[ i ].y = n->y;
    mNormal[ i ].z = n->z;
}


void tVertexOffsetList::AddNormalDiff( int i, rmt::Vector* n, float delta )
{
    P3DASSERT( i < mVertexNum );
    if( !mNormal )
        mNormal = new rmt::Vector[ mVertexNum ];

    P3DASSERT( mNormal );            //out of memory
    
    mNormal[ i ].x += ( n->x - mNormal[ i ].x )*delta;
    mNormal[ i ].y += ( n->y - mNormal[ i ].y )*delta;
    mNormal[ i ].z += ( n->z - mNormal[ i ].z )*delta;
}

void tVertexOffsetList::SetPosition( int i, rmt::Vector* n )
{
    P3DASSERT( i < mVertexNum );
    if( !mCoord )
        mCoord = new rmt::Vector[ mVertexNum ];

    P3DASSERT( mCoord );            //out of memory

    mCoord[ i ].x = n->x;
    mCoord[ i ].y = n->y;
    mCoord[ i ].z = n->z;
}

void tVertexOffsetList::AddPositionDiff( int i, rmt::Vector* n, float delta )
{
    P3DASSERT( i < mVertexNum );
    if( !mCoord )
        mCoord = new rmt::Vector[ mVertexNum ];

    P3DASSERT( mCoord );            //out of memory

    mCoord[ i ].x += ( n->x - mCoord[ i ].x )*delta;
    mCoord[ i ].y += ( n->y - mCoord[ i ].y )*delta;
    mCoord[ i ].z += ( n->z - mCoord[ i ].z )*delta;
}

void tVertexOffsetList::SetColour( int i, tColourOffset* n )
{
    P3DASSERT( i < mVertexNum );
    if( !mColour )
        mColour = new tColourOffset[ mVertexNum ];

    P3DASSERT( mColour );            //out of memory

    mColour[ i ] = *n;

}

void tVertexOffsetList::AddColourDiff( int i, tColourOffset* n, float delta )
{
    P3DASSERT( i < mVertexNum );
    if( !mColour )
        mColour = new tColourOffset[ mVertexNum ];

    P3DASSERT( mColour );            //out of memory


    mColour[ i ].red += (short) (( n->red - mColour[ i ].red )*delta);
    mColour[ i ].green += (short) (( n->green - mColour[ i ].green )*delta);
    mColour[ i ].blue += (short) (( n->blue - mColour[ i ].blue )*delta);
    mColour[ i ].alpha += (short) (( n->alpha - mColour[ i ].alpha )*delta);
   
}

void tVertexOffsetList::SetUV( int i, rmt::Vector2* n, int iu )
{
    P3DASSERT( i < mVertexNum );
    if( !mUV[ iu ] )
        mUV[ iu ] = new rmt::Vector2[ mVertexNum ];

    P3DASSERT( mUV[ iu ] );            //out of memory

    mUV[ iu ][ i ].u = n->u;
    mUV[ iu ][ i ].v = n->v;
}


void tVertexOffsetList::AddUVDiff( int i, rmt::Vector2* n, int iu, float delta )
{
    P3DASSERT( i < mVertexNum );
    if( !mUV[ iu ] )
        mUV[ iu ] = new rmt::Vector2[ mVertexNum ];

    P3DASSERT( mUV[ iu ] );            //out of memory

    mUV[ iu ][ i ].u += ( n->u - mUV[ iu ][ i ].u )*delta;
    mUV[ iu ][ i ].v += ( n->v - mUV[ iu ][ i ].v )*delta;
}

void tVertexOffsetList::Reset( )
{  
    int i;

    if( mMask & eOffsetCoord ){
        //clear coord offset channel
        for( i = 0; i < mVertexNum; ++i )
            mCoord[ i ].Clear( );
    }

    if( mMask & eOffsetNormal ){
        //clear normal offset channel
        for( i = 0; i < mVertexNum; ++i )
            mNormal[ i ].Clear( );
    }
    
    if( mMask & eOffsetColour ){
        //clear colour offset channel
        for( i = 0; i < mVertexNum; ++i )
            mColour[ i ].Clear( );
    }

    if( mMask & eOffsetUV0 ){
        //clear uv0 offset channel
        for( i = 0; i < mVertexNum; ++i )
            mUV[ 0 ][ i ].Clear( );
    }

    if( mMask & eOffsetUV1 ){
        //clear uv0 offset channel
        for( i = 0; i < mVertexNum; ++i )
            mUV[ 1 ][ i ].Clear( );
    }

    if( mMask & eOffsetUV2 ){
        //clear uv0 offset channel
        for( i = 0; i < mVertexNum; ++i )
            mUV[ 2 ][ i ].Clear( );
    }

    if( mMask & eOffsetUV3 ){
        //clear uv0 offset channel
        for( i = 0; i < mVertexNum; ++i )
            mUV[ 3 ][ i ].Clear( );
    }

    //clear mask
    mMask = 0;
   
}

tVertexOffsetList::~tVertexOffsetList( )
{
    if( mCoord )
        delete [ ] mCoord;

    if( mNormal )
        delete [ ] mNormal;

    if( mColour )
        delete [ ] mColour;

    for( int i = 0; i < 4; ++i ){
        if( mUV[ i ] )
            delete [] mUV[ i ];
    }
}

tVertexAnimObject::tVertexAnimObject( )
    :mAutoClear(true), mActiveGeo( NULL ), mGeoOffset( NULL ), mMask( 0x00 )
{

}

tVertexAnimObject::~tVertexAnimObject( )
{
    Reset( );
}

void tVertexAnimObject::Reset( )
{
    if( mActiveGeo )
        mActiveGeo->Release( );

    mActiveGeo = NULL;

    if( mGeoOffset )
        mGeoOffset->Release( );

    mGeoOffset = NULL;

    mMask = 0;
}

void tVertexAnimObject::SetActiveObject( tGeometry *p )
{
    int i;
    //detach the vertex offset list from the target geometry
    if( mActiveGeo ){
        for(  i = 0; i < mActiveGeo->GetNumPrimGroup( ); ++i ){
            tPrimGroup *pg = mActiveGeo->GetPrimGroup( i );
            tPrimGroupStreamed  *prim = dynamic_cast<tPrimGroupStreamed  *>( pg );

            //vertex animation only work on mesh, not on skin
            P3DASSERT( prim );

            tVertexList *vl = prim->GetVertexList( );
            vl->SetOffsetList( NULL );
        }
    }

    tRefCounted::Assign( mActiveGeo,p );
    if( mGeoOffset )
        mGeoOffset->Release( );

    mGeoOffset = new tGeometryOffset( p );
    mGeoOffset->AddRef( );

    //attach the offset to the target geometry
    for( i = 0; i < p->GetNumPrimGroup( ); ++i ){
//        tPrimGroupStreamed  *prim = (tPrimGroupStreamed *)p->GetPrimGroup( i );
        tPrimGroupStreamed  *prim = dynamic_cast<tPrimGroupStreamed *>( p->GetPrimGroup( i ) );
        P3DASSERT( prim );

        tVertexList *vl = prim->GetVertexList( );
        vl->SetOffsetList( mGeoOffset->mOffsets[ i ] );
    }
}

void tVertexAnimObject::Display( )
{
    if( mActiveGeo )
        mActiveGeo->Display( );
}



void tVertexAnimObject::ProcessShaders(ShaderCallback& callback)
{
    if( mActiveGeo )
        mActiveGeo->ProcessShaders(callback);
}

///
///         update the primgroup[ idxPrim ] of the targeted object
///         with the given animation key; 
void tVertexAnimObject::Update( tVertexAnimKey *key, int idxPrim )
{
    int i;
    //update the offset list's color channel
    tColourOffsetList *color;
    tVertexOffsetList *list;

    P3DASSERT( mGeoOffset );
    P3DASSERT( idxPrim < mGeoOffset->mOffsetsNum );

    color = key->mColour;
    list = mGeoOffset->mOffsets[ idxPrim ];

 
    P3DASSERT( list );

    //reset the offset list for primgroup with the index "idxPrim"
    if (mAutoClear)
    {
       list->Reset( );
    }

    if( color ){
        if( color->mIndex ){        //with mIndex channel, the color is a sub-set of the color channel of offsetlist
            for( i = 0; i < color->mNumber; ++i )
                list->SetColour( color->mIndex[ i ], &(color->mColour[ i ]) );
        }
        else{       //the whole color channel has to modified
            for( i = 0; i < list->mVertexNum; ++i )
                list->SetColour( i, &(color->mColour[ i ]) );
        }
        list->mMask |= tVertexOffsetList::eOffsetColour;
    }

    tVectorOffsetList *vector;
    vector = key->mPosition;
    if( vector ){
        if( vector->mIndex ){        //with mIndex channel, the vector is a sub-set of the vector channel of offsetlist
            for( i = 0; i < vector->mNumber; ++i )
                list->SetPosition( vector->mIndex[ i ], &(vector->mVector[ i ]) );
        }
        else{       //the whole vector channel has to modified
            for( i = 0; i < list->mVertexNum; ++i )
                list->SetPosition( i, &(vector->mVector[ i ]) );
        }
        list->mMask |= tVertexOffsetList::eOffsetCoord;
    }

    vector = key->mNormal;
    if( vector ){
        if( vector->mIndex ){        //with mIndex channel, the vector is a sub-set of the vector channel of offsetlist
            for( i = 0; i < vector->mNumber; ++i )
                list->SetNormal( vector->mIndex[ i ], &(vector->mVector[ i ]) );
        }
        else{       //the whole vector channel has to modified
            for( i = 0; i < list->mVertexNum; ++i )
                list->SetNormal( i, &(vector->mVector[ i ]) );
        }
        list->mMask |= tVertexOffsetList::eOffsetNormal;
    }
    

    int mask = tVertexOffsetList::eOffsetUV0;

    for( int j = 0; j < 4; ++j ){
        tVector2OffsetList *vector2;
        vector2 = key->mUV[ j ];
        if( vector2 ){
            if( vector2->mIndex ){        //with mIndex channel, the vector2 is a sub-set of the vector2 channel of offsetlist
                for( i = 0; i < vector2->mNumber; ++i )
                    list->SetUV( vector2->mIndex[ i ], &(vector2->mVector2[ i ]), j );
            }
            else{       //the whole vector2 channel has to modified
                for( i = 0; i < list->mVertexNum; ++i )
                    list->SetUV( i, &(vector2->mVector2[ i ]), j );
            }
            list->mMask |= ( mask << j );
        }
    }
}


///
///         update the primgroup[ idxPrim ] of the targeted object
///         with the given animation key; don't reset the 
///         offset list of mGeoOffset, instead check the difference
///         between the key and mGeoOffset, and add the difference
///         to current mGeoOffset with the delt multiplied
void tVertexAnimObject::AddDifference( tVertexAnimKey *key, int idxPrim, float delta )
{
    int i;
    //update the offset list's color channel
    tColourOffsetList *color;
    tVertexOffsetList *list;

    P3DASSERT( mGeoOffset );
    P3DASSERT( idxPrim < mGeoOffset->mOffsetsNum );

    color = key->mColour;
    list = mGeoOffset->mOffsets[ idxPrim ];

 
    P3DASSERT( list );

    if( color && ( mMask & eInterpolateColour ) ){
        if( color->mIndex ){        //with mIndex channel, the color is a sub-set of the color channel of offsetlist
            for( i = 0; i < color->mNumber; ++i )
                list->AddColourDiff( color->mIndex[ i ], &(color->mColour[ i ]), delta );
        }
        else{       //the whole color channel has to modified
            for( i = 0; i < list->mVertexNum; ++i )
                list->AddColourDiff( i, &(color->mColour[ i ]), delta );
        }
        list->mMask |= tVertexOffsetList::eOffsetColour;
    }

    tVectorOffsetList *vector;
    vector = key->mPosition;
    if( vector && ( mMask & eInterpolateCoord ) ){
        if( vector->mIndex ){        //with mIndex channel, the vector is a sub-set of the vector channel of offsetlist
            for( i = 0; i < vector->mNumber; ++i )
                list->AddPositionDiff( vector->mIndex[ i ], &(vector->mVector[ i ]), delta );
        }
        else{       //the whole vector channel has to modified
            for( i = 0; i < list->mVertexNum; ++i )
                list->AddPositionDiff( i, &(vector->mVector[ i ]), delta );
        }
        list->mMask |= tVertexOffsetList::eOffsetCoord;
    }

    vector = key->mNormal;
    if( vector && ( mMask & eInterpolateNormal ) ){
        if( vector->mIndex ){        //with mIndex channel, the vector is a sub-set of the vector channel of offsetlist
            for( i = 0; i < vector->mNumber; ++i )
                list->AddNormalDiff( vector->mIndex[ i ], &(vector->mVector[ i ]), delta );
        }
        else{       //the whole vector channel has to modified
            for( i = 0; i < list->mVertexNum; ++i )
                list->AddNormalDiff( i, &(vector->mVector[ i ]), delta );
        }
        list->mMask |= tVertexOffsetList::eOffsetNormal;
    }
    

    int mask = tVertexOffsetList::eOffsetUV0;
    int iMask = eInterpolateUV0;

    for( int j = 0; j < 4; ++j ){
        tVector2OffsetList *vector2;
        vector2 = key->mUV[ j ];
        if( vector2 && ( mMask & ( iMask << j ) ) ){
            if( vector2->mIndex ){        //with mIndex channel, the vector2 is a sub-set of the vector2 channel of offsetlist
                for( i = 0; i < vector2->mNumber; ++i )
                    list->AddUVDiff( vector2->mIndex[ i ], &(vector2->mVector2[ i ]), j, delta );
            }
            else{       //the whole vector2 channel has to modified
                for( i = 0; i < list->mVertexNum; ++i )
                    list->AddUVDiff( i, &(vector2->mVector2[ i ]), j, delta );
            }
            list->mMask |= ( mask << j );
        }
    }
}

void tVertexAnimObject::ClearOffsets()
{
    for (int i = 0; i < mGeoOffset->mOffsetsNum; i++)
    {
       mGeoOffset->mOffsets[i]->Reset();
    }
}
