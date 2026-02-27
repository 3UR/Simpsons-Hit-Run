//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This contains the chunk handler classes.
//
// Authors:     Ian Gipson, Mike Perzel (1/11/02)
//
//===========================================================================
#ifndef _FECHUNKHANDLER_H_
#define _FECHUNKHANDLER_H_

//===========================================================================
// Includes
//===========================================================================

#include <p3d/loadmanager.hpp>

class FeGroup;
class FeLayer;
class FeMovie;
class FePage;
class FePolygon;
class FeProject;
class FePure3dObject;
class FeScreen;
class FeSprite;
class FeText;

namespace Scrooby
{
class FeProjectChunkHandler
:
    public tSimpleChunkHandler
{
public:
    //constructor
    FeProjectChunkHandler();

    //loads the object out of a chunk
    virtual tEntity* LoadObject( tChunkFile* file, tEntityStore* store );

protected:

    //loads a group object
    FeGroup* LoadGroupChunk( tChunkFile* file );

    //loads a layer object
    FeLayer* LoadLayerChunk( tChunkFile* file );

    //loads a movie object
    FeMovie* LoadMovieChunk( tChunkFile* file );

    //loads a page object
    FePage* LoadPageChunk( tChunkFile* file );

    //loads a polygon object
    FePolygon* LoadPolygonChunk( tChunkFile* file );

    //loads a pure3dObject object
    FePure3dObject* LoadPure3dObjectChunk( tChunkFile* file );

    //loads a screen object
    FeScreen* LoadScreenChunk( tChunkFile* file );
    
    //loads a sprite object
    FeSprite* LoadSpriteChunk( tChunkFile* file );

    //loads a text object
    FeText* LoadTextChunk( tChunkFile* file );

    FeProject* m_pProject;

};

}//end namespace Scrooby
#endif
