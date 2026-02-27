//===========================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime
//
// Description: This contains the Chunk handler objects
//
// Authors:     Ian Gipson
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================

#include <SDL2/SDL.h>

#include <p3d/chunkfile.hpp>
#include <p3d/inventory.hpp>
#include <constants/chunks.h>
#include <constants/chunkids.hpp>
#include "FeApp.h"
#include "FeChunkHandler.h"
#include "FeGroup.h"
#include "FeLayer.h"
#include "FeMovie.h"
#include "FePage.h"
#include "FePolygon.h"
#include "FeProject.h"
#include "FePure3dObject.h"
#include "FeScreen.h"
#include "FeSprite.h"
#include "FeText.h"
#include "resourcemanager/FeResourceManager.h"

namespace Scrooby
{
//===========================================================================
// Local Constants, Typedefs, and Macros
//===========================================================================

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================

//===========================================================================
// Member Functions
//===========================================================================

//===========================================================================
// FeProjectChunkHandler
//===========================================================================
// Description: constructor
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
FeProjectChunkHandler::FeProjectChunkHandler()
:
    tSimpleChunkHandler( Pure3D::Scrooby::PROJECT ),
    m_pProject( NULL )
{
    //Nothing
}

//===========================================================================
// LoadGroupChunk
//===========================================================================
// Description: loads a Group object
//
// Constraints:    None
//
// Parameters:    file - the file to load the chunk from
//
// Return:      Group* - the Group we just loaded
//
//===========================================================================
FeGroup* FeProjectChunkHandler::LoadGroupChunk( tChunkFile* file )
{
    char name[ 256 ];
    file->GetString( name );
    FeGroup* returnMe = new FeGroup( name );
    returnMe->Resize( 128 );

    unsigned int version = file->GetUInt();
    unsigned int alpha = file->GetUInt();
    //returnMe->SetAlpha( alpha / 256.0f );     //IAN IMPROVE: don't set the alpha?

    while( file->ChunksRemaining() )
    {
        FeDrawable* subEntity = NULL;
        unsigned int returnMeId = file->BeginChunk();
        switch( returnMeId )
        {
        case Pure3D::Scrooby::GROUP :
            {
                subEntity = this->LoadGroupChunk( file );
                break;
            }
        case Pure3D::Scrooby::MULTISPRITE :
            {
                subEntity = this->LoadSpriteChunk( file );
                break;
            }
        case Pure3D::Scrooby::MULTITEXT :
            {
                subEntity = this->LoadTextChunk( file );
                break;
            }
        case Pure3D::Scrooby::POLYGON :
            {
                subEntity = this->LoadPolygonChunk( file );
                break;
            }
        case Pure3D::Scrooby::P3DOBJECT :
            {
                subEntity = this->LoadPure3dObjectChunk( file );
                break;
            }
        case Pure3D::Scrooby::MOVIE :
            {
                subEntity = this->LoadMovieChunk( file );
                break;
            }
        default :
            {
                rAssert( false );        //what the heck did we try to load?
                break;
            }
        }
        file->EndChunk();
        subEntity->SetParent( returnMe );
        returnMe->AddChild( subEntity );
    }
    return returnMe;
}

//===========================================================================
// LoadObject
//===========================================================================
// Description: Loads the chunk from the file
//
// Constraints:    None
//
// Parameters:    file - the file to load the chunk from
//              store - the place to put the chunk
//
// Return:      loading successful?
//
//===========================================================================
tEntity* FeProjectChunkHandler::LoadObject( tChunkFile* file, tEntityStore* store )
{
    char tempString[256];
    //create storage
    FeProject* pProject = new FeProject( file->GetFullFilename() );
    m_pProject = pProject;
    m_pProject->Resize( 128 );

    FeApp::GetInstance()->AddProject(m_pProject);

    // Grab the name, then ignore it
    file->GetString( tempString );

    // Ignore version number
    int version = file->GetUInt();

    int width = file->GetUInt();
    int height= file->GetUInt();
    //FeScreenResolution::Instance()->SetResolution( width, height );
    pProject->SetScreenRes( static_cast<float>(width), static_cast<float>(height) );

    // Get the platform, then ignore it
    file->GetString( tempString );

    // Get the page path
    file->GetString( tempString );
    pProject->SetPagePath( tempString );

    // Get the resource path
    file->GetString( tempString );
    pProject->SetResPath( tempString );

    // Get the Screen path
    file->GetString( tempString );
    pProject->SetScreenPath( tempString );
    
    while( file->ChunksRemaining() )
    {
        unsigned int chunkid = file->BeginChunk();
        if ( chunkid == Pure3D::Scrooby::SCREEN )
        {
            FeScreen* screen = LoadScreenChunk( file );
            rAssert( screen != NULL );
            pProject->AddChild( screen );
        }
        else if ( chunkid == Pure3D::Scrooby::PAGE )
        {
            FePage* page = LoadPageChunk( file );
            rAssert( page != NULL );
            // We're just going to store this, then we'll wait for the screens to claim them
            store->Store( page );
        }
        else
        {
            // There shouldn't be anything else on the top level
            rAssert( false );
        }
        file->EndChunk();
    }

    radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
        FeApp::GetInstance()->GetFeResourceManager().SetCallbackResourceID( FeApp::GetInstance()->GetFeResourceManager().AddResource( "dummy.p3d", NULL, RT_PROJECT_LOAD_CALLBACK ) );
    ::radMemorySetCurrentAllocator( old );

    m_pProject = NULL;
    return pProject;
}

//===========================================================================
// LoadLayerChunk
//===========================================================================
// Description: loads a layer object
//
// Constraints:    None
//
// Parameters:    file - the file to load the chunk from
//
// Return:      layer* - the layer we just loaded
//
//===========================================================================
FeLayer* FeProjectChunkHandler::LoadLayerChunk( tChunkFile* file )
{
    char name[ 256 ] = "";
    file->GetString( name );

    FeLayer* returnMe = new FeLayer( name );
    returnMe->Resize( 128 );

    unsigned int version = file->GetUInt();
    bool visible = ( file->GetUInt() != 0 );
    returnMe->SetVisible( visible );
    bool editable = ( file->GetUInt() != 0 );
    char alpha = file->GetUInt();

    while( file->ChunksRemaining() )
    {
        FeDrawable* subEntity = NULL;
        unsigned int chunkId = file->BeginChunk();
        switch( chunkId )
        {
        case Pure3D::Scrooby::GROUP :
            {
                subEntity = this->LoadGroupChunk( file );
                break;
            }
        case Pure3D::Scrooby::MULTISPRITE :
            {
                subEntity = this->LoadSpriteChunk( file );
                break;
            }
        case Pure3D::Scrooby::MULTITEXT :
            {
                subEntity = this->LoadTextChunk( file );
                break;
            }
        case Pure3D::Scrooby::POLYGON :
            {
                subEntity = this->LoadPolygonChunk( file );
                break;
            }
        case Pure3D::Scrooby::P3DOBJECT :
            {
                subEntity = this->LoadPure3dObjectChunk( file );
                break;
            }
        case Pure3D::Scrooby::MOVIE :
            {
                subEntity = this->LoadMovieChunk( file );
                break;
            }
        default :
            {
                rAssert( false );        //what the heck did we try to load?
                break;
            }
        }
        file->EndChunk();
        subEntity->SetParent( returnMe );
        returnMe->AddChild( subEntity );
    }
    return returnMe;
}


//===========================================================================
// LoadMovieChunk
//===========================================================================
// Description: loads a movie object
//
// Constraints:    None
//
// Parameters:    file - the file to load the chunk from
//
// Return:      movie* - the movie we just loaded
//
//===========================================================================
FeMovie* FeProjectChunkHandler::LoadMovieChunk( tChunkFile* file )
{
    rAssert( false );
    return NULL;
}

//===========================================================================
// LoadPageChunk
//===========================================================================
// Description: loads a page object
//
// Constraints:    None
//
// Parameters:    file - the file to load the chunk from
//
// Return:      page* - the page we just loaded
//
//===========================================================================
FePage* FeProjectChunkHandler::LoadPageChunk( tChunkFile* file )
{
    char name[ 256 ] = "";
    file->GetString( name );

    FePage* returnMe = new FePage( name, NULL );
    returnMe->Resize( 128 );

    // ignore the version and the resolution
    unsigned int version = file->GetUInt();
    unsigned int resx = file->GetUInt();
    unsigned int resy = file->GetUInt();

    while( file->ChunksRemaining() )
    {
        unsigned int chunkId = file->BeginChunk();
        switch( chunkId )
        {
        case Pure3D::Scrooby::LAYER :
            {
                FeLayer* subLayer = this->LoadLayerChunk( file );
                subLayer->SetParent( returnMe );
                returnMe->AddChild( subLayer );
                break;
            }
        case Pure3D::Scrooby::RESOURCEIMAGE :
            {
                char name[ 256 ] = "";
                unsigned int version;
                char filename[ 256 ] = "";
                file->GetString( name );
                version = file->GetUInt();
                file->GetString( filename );

                radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
                    PascalCString tempString = m_pProject->GetProjectPath();
                    tempString += m_pProject->GetResPath();
                    tempString += filename;
                
                    PascalCString pfilename;
                    pfilename.Grow( 128 );
                    pfilename = filename;

                    PascalCString fullFileName = pfilename.FullFilename();
                ::radMemorySetCurrentAllocator( old );
                
                if( !FeApp::GetInstance()->UseFullImageNames() || pfilename.JustExtension().EqualsInsensitive( "p3d" ) )
                {
                    pfilename = fullFileName;
                }
                else
                {
                    pfilename = tempString;
                }
                int index = FeApp::GetInstance()->GetFeResourceManager().AddResource( tempString, NULL, RT_IMAGE, pfilename );
                returnMe->AddResource( index );
                FeApp::GetInstance()->GetFeResourceManager().AddAlias( name, index );
                break;
            }
        case Pure3D::Scrooby::RESOURCETEXTSTYLE  :
            {
                char name[ 256 ] = "";
                unsigned int version;
                char filename[ 256 ] = "";
                char inventoryName[256] = "";
                file->GetString( name );
                version = file->GetUInt();
                file->GetString( filename );
                file->GetString( inventoryName );
                radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
                    PascalCString tempString = m_pProject->GetProjectPath();
                    tempString += m_pProject->GetResPath();
                    tempString += filename;
                    
                    PascalCString invName( inventoryName );
                ::radMemorySetCurrentAllocator( old );
                
                int index = FeApp::GetInstance()->GetFeResourceManager().AddResource( tempString, NULL, RT_TEXT_STYLE, invName );
                returnMe->AddResource( index );
                FeApp::GetInstance()->GetFeResourceManager().AddAlias( name, index );
                break;
            }
        case Pure3D::Scrooby::RESOURCETEXTBIBLE  :
            {
                char name[ 256 ] = "";
                unsigned int version;
                char filename[ 256 ] = "";
                char inventoryName[ 256 ] = "";
                file->GetString( name );
                version = file->GetUInt();
                file->GetString( filename ); 
                file->GetString( inventoryName );
                radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
                    PascalCString tempString = m_pProject->GetProjectPath();
                    tempString += m_pProject->GetResPath();
                    tempString += filename;
                    PascalCString invName( inventoryName );
                ::radMemorySetCurrentAllocator( old );
                
                int index = FeApp::GetInstance()->GetFeResourceManager().AddResource( tempString, NULL, RT_TEXT_BIBLE, invName );
                returnMe->AddResource( index );
                FeApp::GetInstance()->GetFeResourceManager().AddAlias( name, index );
                break;
            }
        case Pure3D::Scrooby::RESOURCEPURE3D  :
            {
                char name[ 256 ] = "";
                unsigned int version;
                char filename[ 256 ] = "";
                char inventoryName[ 256 ] = "";
                char cameraName[ 256 ] = "";
                char animationName[ 256 ] = "";
                file->GetString( name );
                version = file->GetUInt();
                file->GetString( filename );
                file->GetString( inventoryName );
                file->GetString( cameraName );
                file->GetString( animationName );
                radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
                    PascalCString tempString = m_pProject->GetProjectPath();
                    tempString += m_pProject->GetResPath();
                    tempString += filename;
                ::radMemorySetCurrentAllocator( old );
             
                int index = FeApp::GetInstance()->GetFeResourceManager().AddResource( tempString, NULL, RT_P3D_OBJECT, inventoryName );
                returnMe->AddResource( index );
                FeApp::GetInstance()->GetFeResourceManager().AddAlias( name, index );
                FeApp::GetInstance()->GetFeResourceManager().AddPure3dAttributes( cameraName, animationName, index );

                break;
            }
        default:
            {
                rAssert( false );
            }
        }
        file->EndChunk();
    }
    return returnMe;
}

//===========================================================================
// LoadPolygonChunk
//===========================================================================
// Description: loads a Polygon object
//
// Constraints:    None
//
// Parameters:    file - the file to load the chunk from
//
// Return:      Polygon* - the Polygon we just loaded
//
//===========================================================================
FePolygon* FeProjectChunkHandler::LoadPolygonChunk( tChunkFile* file )
{
    char name[ 256 ] = "";
    file->GetString( name );
    FePolygon* returnMe = new FePolygon( name );

    unsigned int version = file->GetUInt();
    unsigned int translucency = file->GetUInt();    //IAN IMPROVE: ignored?
    unsigned int numPoints = file->GetUInt();
    returnMe->SetNumVertex( numPoints );
    unsigned int i;
    for( i = 0; i < numPoints; i++ )
    {
        float x = file->GetFloat();
        float y = file->GetFloat();
        float z = file->GetFloat();
        returnMe->SetVertexLocation( i, (int)x, (int)y );      //IMPROVE: what about Z?
                                                      //IMPROVE: casting?
    }

    for( i = 0; i < numPoints; i++ )
    {
        long int color = file->GetLong();
        tColour c = tColour( color );
        returnMe->SetVertexColour( i, c.Red(), c.Green(), c.Blue() );
        returnMe->SetVertexAlpha( i, c.Alpha() / 255.0f );
                                                    //IMPROVE: casting?
                                                    //IMPROVE: use a tColour
    }
    return returnMe;
}

//===========================================================================
// LoadPure3dObjectChunk
//===========================================================================
// Description: loads a Pure3dObject object
//
// Constraints:    None
//
// Parameters:    file - the file to load the chunk from
//
// Return:      Pure3dObject* - the Pure3dObject we just loaded
//
//===========================================================================
FePure3dObject* FeProjectChunkHandler::LoadPure3dObjectChunk( tChunkFile* file )
{
    char name[ 256 ] = "";
    file->GetString( name );
    unsigned int version = file->GetUInt();
    unsigned int x = file->GetUInt();
    unsigned int y = file->GetUInt();

    FePure3dObject* returnMe = new FePure3dObject( name, x, y );

    unsigned int dimensionX = file->GetUInt();
    unsigned int dimensionY = file->GetUInt();
    returnMe->SetBoundingBoxSize( dimensionX, dimensionY );

    unsigned int justificationX = file->GetInt();
    unsigned int justificationY = file->GetInt();
    returnMe->SetVerticalJustification( Scrooby::JustificationEnum( justificationY ) );     //IAN IMPROVE: new style casts please
    returnMe->SetHorizontalJustification( Scrooby::JustificationEnum( justificationX ) );

    tColour colour;
    colour.c = file->GetUInt();
    returnMe->SetColour( colour );

    //ignore translucency and rotation
    unsigned int translucency = file->GetInt();
    float rotationValue = file->GetFloat();

    char p3dFilename[ 256 ] = "";
    file->GetString( p3dFilename );
    returnMe->Add3dModel( p3dFilename );
    return returnMe;
}


//===========================================================================
// LoadScreenChunk
//===========================================================================
// Description: loads a screen object
//
// Constraints:    None
//
// Parameters:    file - the file to load the chunk from
//
// Return:      Screen* - the screen we just loaded
//
//===========================================================================
FeScreen* FeProjectChunkHandler::LoadScreenChunk( tChunkFile* file )
{
    char name[ 256 ] = "";
    file->GetString( name );

    FeScreen* returnMe = new FeScreen( name, NULL );

    unsigned int version = file->GetUInt();
    unsigned int numPages = file->GetUInt();
    returnMe->Resize( 128 );

    // Get the name of each of the subPages, then add them
    while ( numPages > 0 )
    {
        file->GetString( name );
        radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
            PascalCString temp = m_pProject->GetProjectPath();
            temp += m_pProject->GetPagePath();
            temp += name;
            PascalCString fullFileName = temp.FullFilename();
        ::radMemorySetCurrentAllocator( old );
        FeApp::GetInstance()->GetFeResourceManager().AddResource( temp, returnMe, RT_PAGE, fullFileName );
        numPages--;
    }

    while( file->ChunksRemaining() )
    {
        unsigned int chunkId = file->BeginChunk();
        switch( chunkId )
        {
        case Pure3D::Scrooby::PAGE :
            {
                FePage* subPage = this->LoadPageChunk( file );
                subPage->SetParent( returnMe );
                returnMe->AddChild( subPage );
                break;
            }
        default:
            {
                rAssert( false );//what the hell chunk was this?
                break;
            }
        }
        file->EndChunk();
    }
    
    rAssert( returnMe != NULL );
    return returnMe;
}

//===========================================================================
// LoadSpriteChunk
//===========================================================================
// Description: loads a Sprite object
//
// Constraints:    None
//
// Parameters:    file - the file to load the chunk from
//
// Return:      Sprite* - the Sprite we just loaded
//
//===========================================================================
FeSprite* FeProjectChunkHandler::LoadSpriteChunk( tChunkFile* file )
{
    char name[ 256 ] = "";
    file->GetString( name );
    
    unsigned int version = file->GetUInt();
    unsigned int x = file->GetUInt();
    unsigned int y = file->GetUInt();

    FeSprite* returnMe = new FeSprite( name, x, y );

    unsigned int dimensionX = file->GetUInt();
    unsigned int dimensionY = file->GetUInt();
    returnMe->SetBoundingBoxSize( dimensionX, dimensionY );

    unsigned int justificationX = file->GetInt();
    unsigned int justificationY = file->GetInt();
    returnMe->SetVerticalJustification( Scrooby::JustificationEnum( justificationY ) );     //IAN IMPROVE: new style casts please
    returnMe->SetHorizontalJustification( Scrooby::JustificationEnum( justificationX ) );

    tColour colour;
    colour.c = file->GetUInt();
    returnMe->SetColour( colour );

    // Ignore translucency and rotation for now
    unsigned int translucency = file->GetInt();
    float rotationValue = file->GetFloat();

    unsigned int numberOfImages = file->GetUInt();
    unsigned int i;
    for( i = 0; i < numberOfImages; i++ )
    {
        char imageName[ 256 ] = "";
        file->GetString( imageName );
        returnMe->AddImage( imageName );
    }
    return returnMe;
}

//===========================================================================
// LoadTextChunk
//===========================================================================
// Description: loads a Text object
//
// Constraints:    None
//
// Parameters:    file - the file to load the chunk from
//
// Return:      Text* - the Text we just loaded
//
//===========================================================================
FeText* FeProjectChunkHandler::LoadTextChunk( tChunkFile* file )
{
    char name[ 256 ] = "";
    file->GetString( name );

    unsigned int version = file->GetUInt();
    unsigned int x = file->GetUInt();
    unsigned int y = file->GetUInt();

    FeText* returnMe = new FeText( name, x, y );

    //
    // some text objects require different sized arrays - ugly hack
    //

    if( 
        ( SDL_strcasecmp( name , "Message" )             == 0 ) ||
        ( SDL_strcasecmp( name , "MissionObjective" )    == 0 )
      )
    {
        returnMe->Resize( 512 );
    }
    else if( 
        ( SDL_strcasecmp( name , "Car_Value" )           == 0 ) ||
        ( SDL_strcasecmp( name , "MissionFailureHint" )  == 0 ) ||
        ( SDL_strcasecmp( name , "MissionInfo" )         == 0 ) ||
        ( SDL_strcasecmp( name , "MissionTitle" )        == 0 )
        )
    {
        returnMe->Resize( 128 );
    }
    else
    {
        returnMe->Resize( 64 );
    }

    unsigned int dimensionX = file->GetUInt();
    unsigned int dimensionY = file->GetUInt();
    returnMe->SetBoundingBoxSize( dimensionX, dimensionY );

    unsigned int justificationX = file->GetInt();
    unsigned int justificationY = file->GetInt();
    returnMe->SetVerticalJustification( Scrooby::JustificationEnum( justificationY ) );     //IAN IMPROVE: new style casts please
    returnMe->SetHorizontalJustification( Scrooby::JustificationEnum( justificationX ) );

    tColour colour;
    colour.c = file->GetUInt();
    returnMe->SetColour( colour );

    unsigned int translucency = file->GetInt();     //IAN IMPROVE: this is ignored
    float rotationValue = file->GetFloat();         //IAN IMPROVE: this is ignored

    char textStyleName[ 256 ] = "";
    file->GetString( textStyleName );
    returnMe->SetTextStyle( textStyleName );

    int index = 0;
    // Added drop shadows to text in version 17.  Ignore in order to make old art work
    if( version > 16 )
    {
        returnMe->SetDisplayShadow( (file->GetUChar() != 0) );
        tColour shadowColour;
        shadowColour.c = file->GetUInt();
        returnMe->SetShadowColour( shadowColour );
        int xOffset = file->GetInt();
        int yOffset = file->GetInt();
        returnMe->SetShadowOffset( xOffset, yOffset );
        index =  file->GetUInt();
    }

    while( file->ChunksRemaining() )
    {
        unsigned int chunkId = file->BeginChunk();
        switch( chunkId )
        {
        case Pure3D::Scrooby::STRINGTEXTBIBLE :
            {
                char bibleName[ 256 ];
                char stringId[ 256 ];
                file->GetString( bibleName );
                file->GetString( stringId );
                returnMe->AddTextBibleString( bibleName, stringId );
                break;
            }
        case Pure3D::Scrooby::STRINGHARDCODED :
            {
                char string[ 256 ] = "";
                file->GetString( string );
                returnMe->AddHardCodedString( string );
                break;
            }
        default:
            {
                rAssert( false );//what the hell chunk was this?
                break;
            }
        }
        file->EndChunk();
    }

    // From before the strings.  We need to set this after the strings have been added though.
    returnMe->SetIndex( index );

    return returnMe;
}

}   //end namespace Scrooby
