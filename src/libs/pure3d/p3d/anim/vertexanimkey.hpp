//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _TVERTEXANIMKEY_HPP
#define _TVERTEXANIMKEY_HPP

#include <p3d/entity.hpp>
#include <p3d/loadmanager.hpp>

class tColourOffset
{
public:
    tColourOffset( ) : red( 0 ), green( 0 ), blue( 0 ), alpha( 0 ) { };
    ~tColourOffset( ){ };    
    tColourOffset & operator=( const tColourOffset &co );
    void Clear( ){ red = green = blue = alpha = 0; }
    
    short red, green, blue, alpha;
};

inline tColourOffset & tColourOffset::operator=( const tColourOffset &co )
{
    red = co.red;
    green = co.green;
    blue = co.blue;
    alpha = co.alpha;

    return *this;
}


class tOffsetIndexList : public tRefCounted
{
public:
    tOffsetIndexList( );
    ~tOffsetIndexList( );
    void CreateIndex( int num );    

    int *mIndex;
    int mNumber;
};

class tColourOffsetList : public tOffsetIndexList
{
public:
    tColourOffsetList( );
    ~tColourOffsetList( );
    void CreateOffsets( int num );
    
    tColourOffset     *mColour;    
};

class tVectorOffsetList : public tOffsetIndexList
{
public:

    tVectorOffsetList( );
    ~tVectorOffsetList( );

    void CreateOffsets( int num );
    
    rmt::Vector     *mVector;    
};

class tVector2OffsetList : public tOffsetIndexList
{   
public:

    tVector2OffsetList( );
    ~tVector2OffsetList( );

    void CreateOffsets( int num );

    rmt::Vector2     *mVector2; 
};

class tVertexAnimKey:  public tEntity
{

public:
    tVertexAnimKey( );
    tVertexAnimKey( tVertexAnimKey *k, float delta = 1.f );
    tVertexAnimKey * Interpolate( float targetf, float myframe );

    tColourOffsetList *mColour;
    tVectorOffsetList *mPosition;
    tVectorOffsetList *mNormal;
    tVector2OffsetList *mUV[ 4 ];

protected:
    ~tVertexAnimKey( );
    

};


//Loads vertex animation keys from the 32-bit chunks
//-----------------------------------------------
class tVertexAnimKeyLoader : public tSimpleChunkHandler
{
public:
    tVertexAnimKeyLoader();
protected:
    ~tVertexAnimKeyLoader() {};
    tEntity* LoadObject(tChunkFile*, tEntityStore*);
};

//Load offset list for vertex animation keys
//-----------------------------------------------
class tOffsetListLoader 
{
public:
    tOffsetListLoader( ){};
    ~tOffsetListLoader( ){};

    tOffsetIndexList *LoadObject( tChunkFile *f, tEntityStore *store, unsigned id, int &param );
protected:
    tOffsetIndexList *LoadVectorOffsetList( tChunkFile *f, tEntityStore *store, int &param );
    tOffsetIndexList *LoadVector2OffsetList( tChunkFile *f, tEntityStore *store, int &param );
    tOffsetIndexList *LoadColourOffsetList( tChunkFile *f, tEntityStore *store );

};

#endif
