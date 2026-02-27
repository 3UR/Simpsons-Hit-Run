//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   FeProjectLoader
//
// Description: Implementation of the FeProjectLoader class.
//
// Authors:     Darwin Chau
//
// Revisions        Date            Author        Revision
//                  2000/07/06        DChau        Created
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================
#include "assert.h"
#include "FeApp.h"
#include "FeChunkHandler.h"
#include "FeGroup.h"
#include "FeLayer.h"
#include "FeLoadTags.h"
#include "FeMovie.h"
#include "FePage.h"
#include "FePolygon.h"
#include "FeProject.h"
#include "FeProjectLoader.h"
#include "FePure3dObject.h"
#include "FeScreen.h"
#include "FeSprite.h"
#include "FeText.h"
#include <file.hpp>
#include "lPath.h"
#include <string.h>
#include "strings/stricmp.h"
#include "tLinearTable.h"
#include <utility.hpp>
#include "utility/debugMessages.h"
#include "xml/XMLTree.h"

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================

//===========================================================================
// Public Member Functions
//===========================================================================

//===========================================================================
// FeProjectLoader::FeProjectLoader
//===========================================================================
// Description: Constructor.
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      N/A.
//
//===========================================================================
FeProjectLoader::FeProjectLoader()
:
    mpProject( NULL ),
    mpScreenIterator( NULL ),
    mpPageIterator( NULL ),
    m_ProjectType( SCROOBY_INVALID )
{
    mFileLoader = new AsyncFileLoader;
    mFileLoader->AddRef();

    //
    // Nothing
    //
}



//===========================================================================
// FeProjectLoader::~FeProjectLoader()
//===========================================================================
// Description: Destructor.
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      N/A.
//
//===========================================================================
FeProjectLoader::~FeProjectLoader()
{
    mFileLoader->Release();
    mFileLoader->Release();

    //if this assert fails, then that means the ResourceLoadComplete function 
    //never got called.
    rAssert( NULL == mpProject );
    rAssert( NULL == mpScreenIterator );
    rAssert( NULL == mpPageIterator );
}


//===========================================================================
// void FeProjectLoader::LoadProjectFile( const char* fullFilePath )
//===========================================================================
// Description: Starts loading the specified XML project file.
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::LoadProjectFile
(
    const char* fullFilePath
)
{
    //
    // Create a new FeProject, the start of our data heirarchy.
    //
    Scrooby::ScroobyInfo( "FeProjectLoader::LoadProjectFile" );
    mpProject = new ( ScroobyPermPool )FeProject( fullFilePath );
   
    //
    // This is now our state.
    //
    mCurrentState = eProjectFilePending;
    
    //
    // Is this an XML or a P3D file?
    //

    //IAN IMPROVE: what if filename is < 3 characters
    int length = ::strlen( fullFilePath );
    PascalCString last3 = fullFilePath + length - 3;
    last3.ToUpper();
    if( last3 == "P3D" )
    {
        this->m_ProjectType = SCROOBY_P3D;
    }
    else
    {
        this->m_ProjectType = SCROOBY_XML;      //IAN IMPROVE: make this test the fileformat
    }

    //
    // Request the file load.
    //
    mFileLoader->LoadFile( fullFilePath, this );
}



//===========================================================================
// FeProjectLoader::OnFileLoadComplete
//===========================================================================
// Description: This is the heart of our state machine.
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::OnFileLoadComplete
(
    const unsigned int fileSize,
    char* fileDataBuffer
)
{
    rAssert( fileSize > 0 );
    rValid( fileDataBuffer );

    switch( mCurrentState )
    {
        case eProjectFilePending:
        {
            this->HandleProjectFilePending( fileSize, fileDataBuffer );
        }
        break;

        case eScreenFilePending:
        {
            this->HandleScreenFilePending( fileSize, fileDataBuffer );
        }
        break;

        case ePageFilePending:
        {
            this->HandlePageFilePending( fileSize, fileDataBuffer );
        }
        break;

        default:
        {
            rAssert( 0 );
        }
        break;
    }

    //
    // We're responsible for freeing the memory allocated for the buffer
    // the file is read into.  It's allocated inside AsyncFileLoader.
    //
#ifdef P3D_PS2
    free( fileDataBuffer );
#else
    delete [] fileDataBuffer;
#endif

    return;
}



//===========================================================================
// FeProjectLoader::OnResourceLoadComplete
//===========================================================================
// Description: All the resources have finished loading.
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::OnResourceLoadComplete()
{
    //
    // DARWIN TODO: Is this static function how we really want to implement
    // this callback?
    //
    FeApp::GetInstance()->OnProjectLoadComplete( mpProject );

    //
    // Deleting the project is now the responsibility of the FeApp.
    //
    mpProject = NULL;
    
    return;
}



//===========================================================================
// Private Member Functions
//===========================================================================

//===========================================================================
// void FeProjectLoader::HandlePageFilePending
//===========================================================================
// Description: 
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::HandlePageFilePending
(
    const unsigned int fileSize,
    char* fileDataBuffer
)
{
    rValid( fileDataBuffer );

    //
    // Build the XML tree.
    //
    XMLTree xmlTree;

    bool ok = xmlTree.LoadTreeFromBuffer( fileDataBuffer,
                                          fileSize,
                                          TAG_Page );
    rAssert( true == ok );

    //
    // Parse and popluate the resources and layers (i.e. all the drawables ).
    //
    FeEntity* current = mpPageIterator->Current();
    FePage* pCurrentPage = dynamic_cast< FePage* >( current );
       
    this->ParsePageResources( &xmlTree, pCurrentPage );

    this->ParsePageLayers( &xmlTree, pCurrentPage );


    //
    // Are we done with loading the pages?
    //
    FeEntity* next = mpPageIterator->Next();
    FePage* pNextPage = dynamic_cast< FePage* >( next );
    
    //
    // WARNING: At one time there was a PS2 bug that cause the above
    //          to continue calling Next() until it caused a crash.
    //          The workaround for this was to split up the calls like this:
    //
    //          FeEntity* pEntity = mpPageIterator->Next();
    //          FePage* pNextPage = dynamic_cast<FePage*>( pEntity );

    if( NULL == pNextPage )
    {
        delete mpPageIterator;
        mpPageIterator = NULL;

        //we're done with loading the project/screen/page files
        //this->

        //
        // Now load the resources!
        //
        PascalCString fullResourcePath( mpProject->GetProjectPath() );
        fullResourcePath += mpProject->GetResPath();
        FeApp::GetInstance()->GetFeResourceManager().SetResourcePath( fullResourcePath );

        FeApp::GetInstance()->OnProjectLoadComplete( this->mpProject );

        //
        // Ownership of the project has been passed off to FeApp.
        //
        mpProject = NULL;
    }
    else
    {
        this->LoadPage( pNextPage );
    }

    return;
}


//===========================================================================
// void FeProjectLoader::ParsePageResources
//===========================================================================
// Description: 
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::ParsePageResources
(
    XMLTree* pXmlTree,
    FePage* pOwningPage
)
{
    rValid( pXmlTree );
    rValid( pOwningPage );

    XMLTree* pResourceSubTree = pXmlTree->GetSubTreeByName( TAG_Resources );
    
    if( NULL == pResourceSubTree)
    {
        //
        // No resources.
        //
        return;
    }

    //------------------------------------------------------------------------
    // Parse the Image Resources.
    //------------------------------------------------------------------------
    XMLTree* pImageSubTree = pResourceSubTree->GetSubTreeByName( TAG_Images );
    
    if( NULL != pImageSubTree )
    {
        // There are images.
        int i = 0;
        
        PascalCString alias;
        PascalCString filename;
                
        while( pImageSubTree->SetCurrentElementByIndex( i ) )
        {
            int charCount;
            
            charCount = pImageSubTree->GetAttribute( TAG_AttrName, alias );
            rAssert( 0 != charCount );
                
            
            charCount = pImageSubTree->GetAttribute( TAG_AttrData, filename );
            rAssert( 0 != charCount );
        
            pOwningPage->AddImageResourceEntry( alias, filename );
            
            ++i;
        }

        delete pImageSubTree;
        pImageSubTree = NULL;
    }

    //------------------------------------------------------------------------
    // Parse the Text Bible Resources.
    //------------------------------------------------------------------------
    XMLTree* pTextBibleSubTree = pResourceSubTree->GetSubTreeByName( TAG_Bibles );
    
    if( NULL != pTextBibleSubTree )
    {
        int i = 0;

        PascalCString alias;
        PascalCString filename;
        
        while( pTextBibleSubTree->SetCurrentElementByIndex( i ) )
        {
            int charCount;
            
            charCount = pTextBibleSubTree->GetAttribute( TAG_AttrName, alias );
            rAssert( 0 != charCount );
                
            
            charCount = pTextBibleSubTree->GetAttribute( TAG_AttrData, filename );
            rAssert( 0 != charCount );

            pOwningPage->AddTextBibleResourceEntry( alias, filename );
            
            ++i;
        }
        
        delete pTextBibleSubTree;
        pTextBibleSubTree = NULL;
    }

    //------------------------------------------------------------------------
    // Parse the Text Style Resources.
    //------------------------------------------------------------------------
    XMLTree* pTextStyleSubTree = pResourceSubTree->GetSubTreeByName( TAG_TextStyles );
    
    if( NULL != pTextStyleSubTree )
    {
        int i = 0;

        PascalCString alias;
        PascalCString filename;
        
        while( pTextStyleSubTree->SetCurrentElementByIndex( i ) )
        {
            int charCount;
            
            charCount = pTextStyleSubTree->GetAttribute( TAG_AttrName, alias );
            rAssert( 0 != charCount );
                
            
            charCount = pTextStyleSubTree->GetAttribute( TAG_AttrData, filename );
            rAssert( 0 != charCount );

            pOwningPage->AddTextStyleResourceEntry( alias, filename );
            
            ++i;
        }
        
        delete pTextStyleSubTree;
        pTextStyleSubTree = NULL;
    }

    //------------------------------------------------------------------------
    // Parse the Movie Clip Resources.
    //------------------------------------------------------------------------
    XMLTree* pMovieClipSubTree = pResourceSubTree->GetSubTreeByName( TAG_MovieClips );
    
    if( NULL != pMovieClipSubTree )
    {
        int i = 0;

        PascalCString alias;
        PascalCString filename;
        
        while( pMovieClipSubTree->SetCurrentElementByIndex( i ) )
        {
            int charCount;
            
            charCount = pMovieClipSubTree->GetAttribute( TAG_AttrName, alias );
            rAssert( 0 != charCount );
                
            
            charCount = pMovieClipSubTree->GetAttribute( TAG_AttrData, filename );
            rAssert( 0 != charCount );

            pOwningPage->AddMovieClipResourceEntry( alias, filename );
            
            ++i;
        }
        
        delete pMovieClipSubTree;
        pMovieClipSubTree = NULL;
    }

    //------------------------------------------------------------------------
    // Parse the 3D Model Resources.
    //------------------------------------------------------------------------
    XMLTree* p3dModelSubTree = pResourceSubTree->GetSubTreeByName( TAG_3dModels );
    
    if( NULL != p3dModelSubTree )
    {
        int i = 0;

        PascalCString alias;
        PascalCString filename;
        PascalCString inventoryName;
        PascalCString animationName;
        PascalCString cameraName;
        
        while( p3dModelSubTree->SetCurrentElementByIndex( i ) )
        {
            int charCount;
            
            charCount = p3dModelSubTree->GetAttribute( TAG_AttrName, alias );
            rAssert( 0 != charCount );//this assertion fires if there was no alias in the XML file                
            
            charCount = p3dModelSubTree->GetAttribute( TAG_AttrData, filename );
            rAssert( 0 != charCount );//this assertion fires if there was no filename in the XML file

            charCount = p3dModelSubTree->GetAttribute( TAG_Pure3dInventoryName, inventoryName );
            rAssert( 0 != charCount );//this assertion fires if there was no p3d inventory name  in the XML file

            charCount = p3dModelSubTree->GetAttribute( TAG_Pure3dAnimationName, animationName );
            //rAssert( 0 != charCount );//this assertion fires if there was no multicontroller in the XML file

            charCount = p3dModelSubTree->GetAttribute( TAG_Pure3dCameraName, cameraName );
            //rAssert( 0 != charCount );  //this assertion fires if there was no camera in the XML file


            pOwningPage->Add3dModelResourceEntry
            ( 
                alias, 
                filename, 
                inventoryName,
                animationName,
                cameraName
            );
            
            ++i;
        }
        
        delete p3dModelSubTree;
        p3dModelSubTree = NULL;
    }


    delete pResourceSubTree;
}



//===========================================================================
// void FeProjectLoader::ParsePageLayers
//===========================================================================
// Description: Iterates through the layers in a page, parsing the XML tree
//              and populating the data heirarchy. 
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::ParsePageLayers
(
    XMLTree* pXmlTree,
    FePage* pOwningPage 
)
{
    rValid( pXmlTree );
    rValid( pOwningPage );

    //-------------------------------------------------------------------------
    // Ripped off from FeLoadPage::LoadLayers()
    //-------------------------------------------------------------------------

    PascalCString layerName;
    
    XMLTree* pLayersSubTree = pXmlTree->GetSubTreeByName( TAG_Layers );
    rAssert( NULL != pLayersSubTree );  // There must be layers in a page.

    int i = 0;
    
    XMLTree* pCurrentLayerTree = pLayersSubTree->GetSubTreeByIndex( i );
    
    while( NULL != pCurrentLayerTree )
    {
        int charCount = pCurrentLayerTree->GetAttribute( TAG_AttrName, layerName );
        rAssert( 0 != charCount );  // There must be a name.

        PascalCString visibilityString;
        bool visibility = false;
        charCount = pCurrentLayerTree->GetAttribute( TAG_AttrVisible, visibilityString );
        if( visibilityString == "true" )
        {
            visibility = true;
        }
        else
        {
            visibility = false;
        }
        
        //
        // Did layer exist?  if so, just reload it instead of adding it.
        //
        Scrooby::Layer* sLayer = pOwningPage->GetLayer( layerName );
        FeLayer* pLayer = dynamic_cast< FeLayer* >( sLayer );
        
        if( NULL == pLayer )
        {
            pLayer = pOwningPage->AddLayer( layerName );
        }
        else
        {
            pLayer->Reset();               
        }

        pLayer->SetVisible( visibility );
        this->ParseDrawables( pCurrentLayerTree, pLayer );

        delete pCurrentLayerTree;
        
        pCurrentLayerTree = pLayersSubTree->GetSubTreeByIndex( ++i );
    }

    delete pLayersSubTree;

    return;
}


//===========================================================================
// void FeProjectLoader::ParseDrawables
//===========================================================================
// Description: 
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::ParseDrawables
(
    XMLTree* pXmlTree,
    FeOwner* pOwner
)
{
    rValid( pXmlTree );
    rValid( pOwner );

    XMLTree* pDrawingElementsSubTree = pXmlTree->GetSubTreeByName( TAG_DrawingElements );
    
    int i = 0;
    
    if( NULL != pDrawingElementsSubTree )
    {
        XMLTree* pCurrentDElementSubTree = pDrawingElementsSubTree->GetSubTreeByIndex( i );
    
        //
        // Work through all the drawing elements on this page.
        //
        while( NULL != pCurrentDElementSubTree )
        {
            PascalCString drawingElementType;
            int charCount;

            charCount = pCurrentDElementSubTree->GetName( drawingElementType );
            rAssert( 0 != charCount ); // Must have a name for this type of drawing element.

            //
            // Figure out what type of drawing element we've got and parse it.
            //
            if( ( ::rstricmp( drawingElementType, TAG_Sprite ) == 0 ) ||
                ( ::rstricmp( drawingElementType, TAG_MultiSprite ) == 0 ) )
            {
                this->ParseSprite( pCurrentDElementSubTree, pOwner );
            }
            else if( ( ::rstricmp( drawingElementType, TAG_Text ) == 0 ) ||
                     ( ::rstricmp( drawingElementType, TAG_MultiText ) == 0 ) )
            {
                this->ParseText( pCurrentDElementSubTree, pOwner );
            }
            else if( ::rstricmp( drawingElementType, TAG_Polygon ) == 0 )
            {
                this->ParsePolygon( pCurrentDElementSubTree, pOwner );
            }
            else if( ::rstricmp( drawingElementType, TAG_Group ) == 0 )
            {
                this->ParseGroup( pCurrentDElementSubTree, pOwner );
            }
            else if( ::rstricmp( drawingElementType, TAG_Movie ) == 0 )
            {
                this->ParseMovie( pCurrentDElementSubTree, pOwner );
            }
            else if( ::rstricmp( drawingElementType, TAG_Pure3dObject ) == 0 )
            {
                this->ParsePure3dObject( pCurrentDElementSubTree, pOwner );
            }

            //
            // Free up this sub-tree.
            //
            delete pCurrentDElementSubTree;
            
            //
            // Get the next one.
            //
            pCurrentDElementSubTree = pDrawingElementsSubTree->GetSubTreeByIndex( ++i );
        }
        
        //
        // Finished with all the drawing elements on this page. Clean up.
        //
        delete pDrawingElementsSubTree;
        
        return;
    }
}



//===========================================================================
// void FeProjectLoader::ParseGroup
//===========================================================================
// Description: 
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::ParseGroup
(
    XMLTree* pXmlTree,
    FeOwner* pOwner
)
{
    rValid( pXmlTree );
    rValid( pOwner );
    
    //
    // Read the group's name.
    //
    PascalCString name;
    int charCount = pXmlTree->GetAttribute( TAG_AttrName, name );
    rAssert( 0 != charCount ); // Must have a name.

    // Did group exist?  if so, just reload it instead of adding it
    //
    Scrooby::Group* sGroup = pOwner->GetGroup( name );
    FeGroup* pGroup;
    pGroup = dynamic_cast< FeGroup* >( sGroup );

    if( NULL == pGroup )
    {
        pGroup = pOwner->AddGroup( name );
    }
    else
    {
        pGroup->Reset();
    }

    //
    //Read the group's alpha
    //
    int alpha;
    bool success = pXmlTree->GetAttribute( TAG_AttrAlpha, &alpha );
    pGroup->SetAlpha( alpha / 256.0f );

    //
    // Recurse on the drawables contained in this group.
    //
    this->ParseDrawables( pXmlTree, pGroup );

    return;
}


//===========================================================================
// void FeProjectLoader::ParsePolygon
//===========================================================================
// Description: 
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::ParsePolygon
(
    XMLTree* pXmlTree,
    FeOwner* pOwner
)
{
    rValid( pXmlTree );
    rValid( pOwner );

    //
    // Read the name.
    //
    PascalCString polygonName;
    
    int charCount = pXmlTree->GetAttribute( TAG_AttrName, polygonName );
    rAssert( 0 != charCount ); // Must have a name.
    
    XMLTree* pVerticesTree = pXmlTree->GetSubTreeByName( TAG_Vertexes );
    rValid( pVerticesTree );
   
    //
    // Did polygon already exist?  if so, just reload it.
    //
    bool isReload = false;
    
    Scrooby::Polygon* sPolygon = pOwner->GetPolygon( polygonName );
    FePolygon* pPolygon = dynamic_cast<FePolygon*>( sPolygon );

    if( NULL == pPolygon )
    {
        pPolygon = pOwner->AddPolygon( polygonName );
    }
    else
    {
        pPolygon->Reset();
        isReload = true;
    }

    //
    // Iterate through all the vertices...
    //
    int i = 0;

    XMLTree* pCurrentVertexTree = pVerticesTree->GetSubTreeByIndex( i );
    
    while( NULL != pCurrentVertexTree )
    {
        bool ok;

        //
        // Read and set the vertex position.
        //
        ok = pCurrentVertexTree->SetCurrentElementByName( TAG_Position );
        rAssert( true == ok );

        int x;
        ok = pCurrentVertexTree->GetAttribute( TAG_PositionX, &x );
        rAssert( true == ok );

        int y;
        ok = pCurrentVertexTree->GetAttribute( TAG_PositionY, &y );
        rAssert( true == ok );
        
        pPolygon->SetVertexLocationNoRedraw( i, x, y );

        
        //
        // Read and set the vertex colour and alpha.
        //
        ok = pCurrentVertexTree->SetCurrentElementByName( TAG_Color );
        rAssert( true == ok );

        int red;
        ok = pCurrentVertexTree->GetAttribute( TAG_ColorRed, &red );
        rAssert( true == ok );

        int green;
        ok = pCurrentVertexTree->GetAttribute( TAG_ColorGreen, &green );
        rAssert( true == ok );
        
        int blue;
        ok = pCurrentVertexTree->GetAttribute( TAG_ColorBlue, &blue );
        rAssert( true == ok );
        
        pPolygon->SetVertexColourNoRedraw( i, red, green, blue );

        int alpha;
        ok = pCurrentVertexTree->GetAttribute( TAG_ColorAlpha, &alpha );
        rAssert( true == ok );

        pPolygon->SetVertexAlphaNoRedraw( i, float( alpha / 255.0f ) );
        
        //
        // Release the current vertex tree and try to get the next one.
        //
        delete pCurrentVertexTree;
        
        pCurrentVertexTree = pVerticesTree->GetSubTreeByIndex( ++i );
    }
    
    pPolygon->SetNumVertex( i );

    //
    // If we are reloading the poly, then there should already be a handle, so it is safe
    // to call display.
    //
    if( isReload )
    {
        pPolygon->Display();
    }

    delete pVerticesTree;
    
    return;
}    



//===========================================================================
// void FeProjectLoader::ParseSprite
//===========================================================================
// Description: 
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::ParseSprite
(
    XMLTree* pXmlTree,
    FeOwner* pOwner
)
{
    rValid( pXmlTree );
    rValid( pOwner );

    //
    // Is this a sprite or multisprite?
    //
    PascalCString spriteType;

    int charCount = pXmlTree->GetName( spriteType );
    rAssert( 0 != charCount ); // Must have a type!

    //
    // Read the name of this drawable.
    //
    PascalCString spriteName;
    
    charCount = pXmlTree->GetAttribute( TAG_AttrName, spriteName );
    rAssert( 0 != charCount ); // Must have a name!

    
    //
    // Read and set the position.
    //
    bool ok;

    ok = pXmlTree->SetCurrentElementByName( TAG_Position );
    rAssert( true == ok );

    int x;
    ok = pXmlTree->GetAttribute( TAG_PositionX, &x );
    rAssert( true == ok );

    int y;
    ok = pXmlTree->GetAttribute( TAG_PositionY, &y );
    rAssert( true == ok );

    
    // Did sprite already exist?  if so, just reload it instead of adding it
    //
    Scrooby::Sprite* sSprite = pOwner->GetSprite( spriteName );
    FeSprite* pSprite = dynamic_cast< FeSprite* >( sSprite );
    
    bool isReload = false;

    if( NULL == pSprite )
    {
        pSprite = pOwner->AddSprite( spriteName, x, y );
    }
    else
    {
        pSprite->Reset();
        isReload = true;
    }

    //
    //read and set the color
    //
    ok = pXmlTree->SetCurrentElementByName( TAG_Color );
    rAssert( true == ok );

    int red;
    ok = pXmlTree->GetAttribute( TAG_ColorRed, &red );
    rAssert( true == ok );

    int green;
    ok = pXmlTree->GetAttribute( TAG_ColorGreen, &green );
    rAssert( true == ok );

    int blue;
    ok = pXmlTree->GetAttribute( TAG_ColorBlue, &blue );
    rAssert( true == ok );

    int alpha;
    ok = pXmlTree->GetAttribute( TAG_ColorAlpha, &alpha );
    rAssert( true == ok );

    pSprite->SetColourNoRedraw( tColour( red, green, blue, alpha ) );


    //
    // Read and set the bounding box.
    //
    ok = pXmlTree->SetCurrentElementByName( TAG_Dimension );
    rAssert( true == ok );

    int width;
    ok = pXmlTree->GetAttribute( TAG_DimensionWidth, &width );
    rAssert( true == ok );

    int height;
    ok = pXmlTree->GetAttribute( TAG_DimensionHeight, &height );
    rAssert( true == ok );

    pSprite->SetBoundingBoxSize( width, height );

    //
    // Read and set the justification.
    //
    ok = pXmlTree->SetCurrentElementByName( TAG_Justification );
    rAssert( true == ok );
        
    PascalCString justification;

    charCount = pXmlTree->GetAttribute( TAG_JustificationVertical, justification );
    rAssert( 0 != charCount );

    pSprite->SetVerticalJustification( StringToJustificationEnum( justification ) );

    charCount = pXmlTree->GetAttribute( TAG_JustificationHorizontal, justification );
    rAssert( 0 != charCount );

    pSprite->SetHorizontalJustification( StringToJustificationEnum( justification ) );


    //
    // If we are not reloading the page, then we need the images to be added
    //
    if( !isReload )
    {
        //
        // Is it a sprite or multisprite?
        //
        if( ::rstricmp( spriteType, TAG_Sprite ) == 0 )
        {
            //
            // There is only one image
            //
            ok = pXmlTree->SetCurrentElementByName( TAG_Image );
            rAssert( true == ok );
                
            PascalCString imageName;
            
            int charCount = pXmlTree->GetAttribute( TAG_AttrName, imageName );
            rAssert( 0 != charCount );

            FeEntity* entity = mpPageIterator->Current();
            FePage* pCurrentPage = dynamic_cast< FePage* >( entity );

            unsigned int imageResourceID = pCurrentPage->FindImageResourceEntry( imageName );

            pSprite->AddImage( imageResourceID );
        }
        else
        {
            //
            // Load multiple images.
            //
            XMLTree* pImagesTree = pXmlTree->GetSubTreeByName( TAG_Images );
            rValid( pImagesTree );

            int i = 0;

            FeEntity* entity = mpPageIterator->Current();
            FePage* pCurrentPage = dynamic_cast< FePage* >( entity );

            while( pImagesTree->SetCurrentElementByIndex( i ) )
            {
                PascalCString imageName;

                int charCount = pImagesTree->GetAttribute( TAG_AttrName, imageName );
                rAssert( 0 != charCount );
                
                unsigned int imageResourceID = pCurrentPage->FindImageResourceEntry( imageName );

                pSprite->AddImage( imageResourceID );

                ++i;
            }
            
            delete pImagesTree;
        }
    }

    //
    // Redisplay the object
    //
    if( isReload )
    {
        pSprite->Display();
    }

    return;
}


//===========================================================================
// void FeProjectLoader::ParseText
//===========================================================================
// Description: 
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::ParseText
(
    XMLTree* pXmlTree,
    FeOwner* pOwner
)
{
    rValid( pXmlTree );
    rValid( pOwner );

    //
    // Is this a single text or multi-text?
    //
    PascalCString textType;
    int charCount;

    charCount = pXmlTree->GetName( textType );
    rAssert( 0 != charCount );

    //
    // Read the name of this drawable.
    //
    PascalCString textName;

    charCount = pXmlTree->GetAttribute( TAG_AttrName, textName );
    rAssert( 0 != charCount ); // Must have a name.

    
    //
    // Read and set the position.
    //
    bool ok;
    ok = pXmlTree->SetCurrentElementByName( TAG_Position );
    rAssert( true == ok );
    
    int x;
    ok = pXmlTree->GetAttribute( TAG_PositionX, &x );
    rAssert( true == ok );

    int y;
    ok = pXmlTree->GetAttribute( TAG_PositionY, &y );
    rAssert( true == ok );

    // Did text exist?  if so, just reload it instead of adding it
    //
    Scrooby::Text* sText = pOwner->GetText( textName );
    FeText* pText = dynamic_cast< FeText* >( sText );
    
    bool isReload = false;

    if( NULL == pText )
    {
        pText = pOwner->AddText( textName, x, y );
    }
    else
    {
        pText->Reset();
        isReload = true;
    }

    
    //
    // Read and set the bounding box.
    //
    ok = pXmlTree->SetCurrentElementByName( TAG_Dimension );
    rAssert( true == ok );
    
    int width;
    ok = pXmlTree->GetAttribute( TAG_DimensionWidth, &width );
    rAssert( true == ok );        
    
    int height;
    ok = pXmlTree->GetAttribute( TAG_DimensionHeight, &height );
    rAssert( true == ok );    

    pText->SetBoundingBoxSize( width, height );


    //
    // Read and set the justification.
    //
    ok = pXmlTree->SetCurrentElementByName( TAG_Justification );
    rAssert( true == ok );
        
    PascalCString justification;

    charCount = pXmlTree->GetAttribute( TAG_JustificationVertical, justification );
    rAssert( 0 != charCount );

    pText->SetVerticalJustification( StringToJustificationEnum( justification ) );

    charCount = pXmlTree->GetAttribute( TAG_JustificationHorizontal, justification );
    rAssert( 0 != charCount );

    pText->SetHorizontalJustification( StringToJustificationEnum( justification ) );


    //
    // Read and set the text style.
    //
    ok = pXmlTree->SetCurrentElementByName( TAG_TextStyle );
    rAssert( true == ok );
    
    PascalCString textStyleName;
    charCount = pXmlTree->GetAttribute( TAG_AttrName, textStyleName );
    rAssert( 0 != charCount );

    FeEntity* entity = mpPageIterator->Current();
    FePage* pCurrentPage = dynamic_cast< FePage* >( entity );
    rValid( pCurrentPage );

    unsigned int textStyleResourceID;
    textStyleResourceID = pCurrentPage->FindTextStyleResourceEntry( textStyleName );

    pText->SetTextStyle( textStyleResourceID );


    //
    // If we are reloading the page, remake all the text objects
    //
    if( isReload )
    {
        pText->DeleteChildren();
    }


    //
    // Is it a text or multitext?
    //
    if( ::rstricmp( textType, TAG_Text ) == 0 )
    {
        // There is only one text
        ok = pXmlTree->SetCurrentElementByName( TAG_String );
        rAssert( true == ok );

        PascalCString textBibleName;

        if( pXmlTree->GetAttribute( TAG_AttrBible, textBibleName ) == 0 )
        {
            PascalCString hardCodedString;

            charCount = pXmlTree->GetAttribute( TAG_HardCodedString, hardCodedString );
            rAssert( 0 != charCount );
            
            pText->AddHardCodedString( hardCodedString );
        }
        else 
        {
            PascalCString stringID;
            
            charCount = pXmlTree->GetAttribute( TAG_AttrStringID, stringID );
            rAssert( 0 != charCount );
            
            unsigned int textBibleResourceID;
            textBibleResourceID = pCurrentPage->FindTextBibleResourceEntry( textBibleName );

            pText->AddTextBibleString( textBibleResourceID, stringID );
        }

    }
    else
    {
        // Load multiple text
        XMLTree* pStringsTree = pXmlTree->GetSubTreeByName( TAG_Strings );
        rValid( pStringsTree );
        
        int i = 0;

        while( pStringsTree->SetCurrentElementByIndex( i ) )
        {
            PascalCString textBibleName;

            //
            // Text bible string or hardcoded string?
            //
            if( pStringsTree->GetAttribute( TAG_AttrBible, textBibleName ) == 0 )
            {
                PascalCString hardCodedString;

                charCount = pStringsTree->GetAttribute( TAG_HardCodedString, hardCodedString );
                rAssert( 0 != charCount );
                
                pText->AddHardCodedString( hardCodedString );
            }
            else 
            {
                PascalCString stringID;
            
                charCount = pStringsTree->GetAttribute( TAG_AttrStringID, stringID );
                rAssert( 0 != charCount );
                
                unsigned int textBibleResourceID;
                textBibleResourceID = pCurrentPage->FindTextBibleResourceEntry( textBibleName );

                pText->AddTextBibleString( textBibleResourceID, stringID );
            }

            ++i;
        }
        
        delete pStringsTree;
    }

    //
    // Redisplay the object
    //
    if( isReload )
    {
        pText->Display();
    }

    return;
}


//===========================================================================
// void FeProjectLoader::ParseMovie
//===========================================================================
// Description: 
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::ParseMovie
(
    XMLTree* pXmlTree,
    FeOwner* pOwner
)
{
    rValid( pXmlTree );
    rValid( pOwner );

    //
    // Read the name of this drawable.
    //
    PascalCString movieName;
    
    int charCount = pXmlTree->GetAttribute( TAG_AttrName, movieName );
    rAssert( 0 != charCount ); // Must have a name!

    
    //
    // Read and set the position.
    //
    bool ok;

    ok = pXmlTree->SetCurrentElementByName( TAG_Position );
    rAssert( true == ok );

    int x;
    ok = pXmlTree->GetAttribute( TAG_PositionX, &x );
    rAssert( true == ok );

    int y;
    ok = pXmlTree->GetAttribute( TAG_PositionY, &y );
    rAssert( true == ok );
    
    // Did sprite already exist?  if so, just reload it instead of adding it
    //
    Scrooby::Movie* sMovie = pOwner->GetMovie( movieName );
    FeMovie* pMovie = dynamic_cast< FeMovie* >( sMovie  );
    
    bool isReload = false;

    if( NULL == pMovie )
    {
        pMovie = pOwner->AddMovie( movieName, x, y );
    }
    else
    {
        pMovie->Reset();
        isReload = true;
    }

    //
    // Read and set the bounding box.
    //
    ok = pXmlTree->SetCurrentElementByName( TAG_Dimension );
    rAssert( true == ok );

    int width;
    ok = pXmlTree->GetAttribute( TAG_DimensionWidth, &width );
    rAssert( true == ok );

    int height;
    ok = pXmlTree->GetAttribute( TAG_DimensionHeight, &height );
    rAssert( true == ok );

    pMovie->SetBoundingBoxSize( width, height );

    //
    // Read and set the justification.
    //
    ok = pXmlTree->SetCurrentElementByName( TAG_Justification );
    rAssert( true == ok );
        
    PascalCString justification;

    charCount = pXmlTree->GetAttribute( TAG_JustificationVertical, justification );
    rAssert( 0 != charCount );

    pMovie->SetVerticalJustification( StringToJustificationEnum( justification ) );

    charCount = pXmlTree->GetAttribute( TAG_JustificationHorizontal, justification );
    rAssert( 0 != charCount );

    pMovie->SetHorizontalJustification( StringToJustificationEnum( justification ) );


    //
    // If we are not reloading the page, then we need the movie clips to be added
    //
    if( !isReload )
    {
        ok = pXmlTree->SetCurrentElementByName( TAG_MovieClip );
        rAssert( true == ok );
            
        PascalCString clipName;
        
        int charCount = pXmlTree->GetAttribute( TAG_AttrName, clipName );
        rAssert( 0 != charCount );

        FeEntity* entity = mpPageIterator->Current();
        FePage* pCurrentPage = dynamic_cast< FePage* >( entity );

        unsigned int imageResourceID = pCurrentPage->FindMovieClipResourceEntry( clipName );

        pMovie->AddMovieClip( imageResourceID );
    }

    //
    // Redisplay the object
    //
    if( isReload )
    {
        pMovie->Display();
    }

    return;
}



//===========================================================================
// void FeProjectLoader::ParsePure3dObject
//===========================================================================
// Description: 
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::ParsePure3dObject
(
    XMLTree* pXmlTree,
    FeOwner* pOwner
)
{
    rValid( pXmlTree );
    rValid( pOwner );

    //
    // Read the name of this drawable.
    //
    PascalCString p3dObjectName;
    
    int charCount = pXmlTree->GetAttribute( TAG_AttrName, p3dObjectName );
    rAssert( 0 != charCount ); // Must have a name!

    
    //
    // Read and set the position.
    //
    bool ok;

    ok = pXmlTree->SetCurrentElementByName( TAG_Position );
    rAssert( true == ok );

    int x;
    ok = pXmlTree->GetAttribute( TAG_PositionX, &x );
    rAssert( true == ok );

    int y;
    ok = pXmlTree->GetAttribute( TAG_PositionY, &y );
    rAssert( true == ok );
    
    // Did sprite already exist?  if so, just reload it instead of adding it
    //
    Scrooby::Pure3dObject* entity = pOwner->GetPure3dObject( p3dObjectName );
    FePure3dObject* pPure3dObject = dynamic_cast< FePure3dObject* >( entity );
    
    bool isReload = false;

    if( NULL == pPure3dObject )
    {
        pPure3dObject = pOwner->AddPure3dObject( p3dObjectName, x, y );
    }
    else
    {
        pPure3dObject->Reset();
        isReload = true;
    }

    //
    // Read and set the bounding box.
    //
    ok = pXmlTree->SetCurrentElementByName( TAG_Dimension );
    rAssert( true == ok );

    int width;
    ok = pXmlTree->GetAttribute( TAG_DimensionWidth, &width );
    rAssert( true == ok );

    int height;
    ok = pXmlTree->GetAttribute( TAG_DimensionHeight, &height );
    rAssert( true == ok );

    pPure3dObject->SetBoundingBoxSize( width, height );

    //
    // Read and set the justification.
    //
    ok = pXmlTree->SetCurrentElementByName( TAG_Justification );
    rAssert( true == ok );
        
    PascalCString justification;

    charCount = pXmlTree->GetAttribute( TAG_JustificationVertical, justification );
    rAssert( 0 != charCount );

    pPure3dObject->SetVerticalJustification( StringToJustificationEnum( justification ) );

    charCount = pXmlTree->GetAttribute( TAG_JustificationHorizontal, justification );
    rAssert( 0 != charCount );

    pPure3dObject->SetHorizontalJustification( StringToJustificationEnum( justification ) );

    //
    // If we are not reloading the page, then we need the 3d model needs to be added
    //
    if( !isReload )
    {
        ok = pXmlTree->SetCurrentElementByName( TAG_3dModel );
        rAssert( true == ok );
            
        PascalCString modelName;
        
        int charCount = pXmlTree->GetAttribute( TAG_AttrName, modelName );
        rAssert( 0 != charCount );

        FeEntity* entity = mpPageIterator->Current();
        FePage* pCurrentPage = dynamic_cast< FePage* >( entity );

        unsigned int imageResourceID = pCurrentPage->Find3dModelResourceEntry( modelName );

        pPure3dObject->Add3dModel( imageResourceID );
    }

    //
    // Redisplay the object
    //
    if( isReload )
    {
        pPure3dObject->Display();
    }

    return;
}


//===========================================================================
// void FeProjectLoader::LoadPage
//===========================================================================
// Description: 
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::LoadPage
(
    FePage* pPage
)
{
    rValid( pPage );

    //
    // Construct the full path and filename for the page file.
    //
    PascalCString fullFilePath( mpProject->GetProjectPath() );

    fullFilePath += mpProject->GetPagePath();
    fullFilePath += pPage->GetFileName();

    //
    // This is now our state.
    //
    mCurrentState = ePageFilePending;
    
    //
    // Request the file load.
    //
    mFileLoader->LoadFile( fullFilePath, this );

    return;
}


//===========================================================================
// void FeProjectLoader::StartPageLoading
//===========================================================================
// Description: 
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::StartPageLoading
(
    void
)
{
    mpPageIterator = new( ScroobyPermPool ) tLinearTable::RawIterator( mpProject->GetPageList() );
    
    FeEntity* entity = mpPageIterator->First();
    FePage* pPage = dynamic_cast< FePage* >( entity );
    
    if( pPage != NULL )
    {
        this->LoadPage( pPage );
    }
}


//===========================================================================
// void FeProjectLoader::HandleScreenFilePending
//===========================================================================
// Description: 
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::HandleScreenFilePending
(
    const unsigned int fileSize,
    char* fileDataBuffer
)
{
    rValid( fileDataBuffer );

    //
    // Build the XML tree.
    //
    XMLTree xmlTree;

    bool ok = xmlTree.LoadTreeFromBuffer( fileDataBuffer,
                                          fileSize,
                                          TAG_Screen );
    rAssert( true == ok );

    //
    // Parse the XML tree.
    //
    PascalCString pageFileName;
    
    XMLTree* subTree = xmlTree.GetSubTreeByName( TAG_Pages );
    
    rValid( subTree );

    int i = 0;
    
    while( subTree->SetCurrentElementByIndex( i ) )
    {
        int charCount = subTree->GetAttribute( TAG_File, pageFileName );
        rAssert( charCount != 0 );
        
        bool isExport;
        
        if( subTree->GetAttribute( TAG_Export, &isExport ) )
        {
            if( isExport )
            {
                FeEntity* entity = mpScreenIterator->Current();
                FeScreen* screen = dynamic_cast< FeScreen* >( entity );
                if( screen != NULL )
                {
                    screen->AddPage( pageFileName );
                }
                else
                {
                    //IAN IMPROVE:
                    //why would the screen be NULL
                }
            }
        }
        
        ++i;
    }
    
    delete subTree;
    
    //-------------------------------------------------------------------------
   
    //
    // Are we done with loading the screens?
    //
    FeEntity* entity = mpScreenIterator->Next();
    FeScreen* pNextScreen = dynamic_cast< FeScreen* >( entity );
    //
    // WARNING: At one time there was a PS2 bug that cause the above
    //          to continue calling Next() until it caused a crash.
    //          The workaround for this was to split up the calls like this:
    //
    //          FeEntity* pEntity = mpScreenIterator->Next();
    //          FeScreen* pNextScreen = dynamic_cast<FeScreen*>( pEntity );
    
    if( NULL == pNextScreen )
    {
        delete mpScreenIterator;
        mpScreenIterator = NULL;

        this->StartPageLoading();
    }
    else
    {
        this->LoadScreen( pNextScreen );
    }

    return;
}

//===========================================================================
// void FeProjectLoader::LoadScreen
//===========================================================================
// Description: 
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::LoadScreen
(
    FeScreen* pScreen
)
{
    rValid( pScreen );

    //
    // Construct the full path and filename for the screen file.
    //
    PascalCString fullFilePath( mpProject->GetProjectPath() );
    
    fullFilePath += mpProject->GetScreenPath();
    fullFilePath += pScreen->GetFileName();

    //
    // This is now our state.
    //
    mCurrentState = eScreenFilePending;
    
    //
    // Request the file load.
    //
    mFileLoader->LoadFile( fullFilePath, this );

    return;
}


//===========================================================================
// void FeProjectLoader::StartScreenLoading
//===========================================================================
// Description: 
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::StartScreenLoading
(
    void
)
{
    //
    // Create and iterator so we can keep track of which screen we're 
    // currently loading.  We'll use this again to keep iterating until all
    // the screens are loaded.
    //
    mpScreenIterator = new( ScroobyPermPool ) tLinearTable::RawIterator( mpProject->GetScreenList() );
    
    FeEntity* entity = mpScreenIterator->First();
    FeScreen* pScreen = dynamic_cast< FeScreen* >( entity );
    
    this->LoadScreen( pScreen );

    return;
}

//===========================================================================
// HandleProjectParsingXML
//===========================================================================
// Description: Parses the XML version of the file
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::HandleProjectParsingXML( char* fileDataBuffer, size_t fileSize )
{
        //
    // Build the XML tree.
    //
    XMLTree xmlTree;

    bool ok = xmlTree.LoadTreeFromBuffer( fileDataBuffer,
                                          fileSize,
                                          TAG_Project );
    rAssert( true == ok );

    //-------------------------------------------------------------------------
    // Parse the XML tree for the Project settings.
    //-------------------------------------------------------------------------

    PascalCString stringBuffer;

    if( ( xmlTree.SetCurrentElementByName( TAG_ResourcePath ) ) &&
        ( xmlTree.GetAttribute( TAG_Value, stringBuffer ) ) )
    {
        rAssert( mpProject != NULL );
        mpProject->SetResPath( stringBuffer );
    }

    if( ( xmlTree.SetCurrentElementByName( TAG_ScreenPath ) ) &&
        ( xmlTree.GetAttribute( TAG_Value, stringBuffer ) ) )
    {
        mpProject->SetScreenPath( stringBuffer );
    }

    if( ( xmlTree.SetCurrentElementByName( TAG_PagePath ) ) &&
        ( xmlTree.GetAttribute( TAG_Value, stringBuffer ) ) )
    {
        mpProject->SetPagePath( stringBuffer );
    }

    if( xmlTree.SetCurrentElementByName( TAG_Resolution ) )
    {
        int width, height;
        if( xmlTree.GetAttribute( TAG_Width, &width ) &&
            xmlTree.GetAttribute( TAG_Height, &height ) )
        {
            FeScreenResolution::Instance()->SetResolution( width, height );
        }
    }

    //-------------------------------------------------------------------------
    // Continue to parse the XML tree for the screen list.
    //-------------------------------------------------------------------------

    XMLTree* subTree = xmlTree.GetSubTreeByName( TAG_Screens );
    rValid( subTree );

    int i = 0;
    PascalCString screenFileName;
    
    while( subTree->SetCurrentElementByIndex( i ) )
    {
        int charCount = subTree->GetAttribute( TAG_File, screenFileName );
        rAssert( charCount != 0 );
        
        mpProject->AddScreen( screenFileName );
        
        ++i;
    }
    
    delete subTree;
}

//===========================================================================
// HandleProjectParsingP3D
//===========================================================================
// Description: Parses the P3D version of the file
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::HandleProjectParsingP3D( char* fileDataBuffer, size_t fileSize )
{
    tFileMem* mem = new tFileMem
                        ( 
                            reinterpret_cast<unsigned char*>( fileDataBuffer ), 
                            fileSize 
                        ); //P3D NEW
    mem->AddRef();
    mem->SetFilename( mpProject->GetFileName() );
    p3d::context->GetLoadManager()->Load( mem );
    mem->Release();

    //go find the actual project that we just loaded
    // We're going to do the callback in the FeProjectChunkHandler itself,
    // so just clean up and leave
    delete mpProject;
    mpProject = NULL;
}


//===========================================================================
// void FeProjectLoader::HandleProjectFilePending
//===========================================================================
// Description: 
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::HandleProjectFilePending
(
    const unsigned int fileSize,
    char* fileDataBuffer
)
{
    rValid( fileDataBuffer );

    //was this a P3D file or an XML file?
    if( m_ProjectType == SCROOBY_XML )
    {
        this->HandleProjectParsingXML( fileDataBuffer, fileSize );
        this->StartScreenLoading();
    }
    else
    {
        this->HandleProjectParsingP3D( fileDataBuffer, fileSize );
    }
    return;
}

//===========================================================================
// FeProjectLoader::OnGotoScreenComplete
//===========================================================================
// Description: 
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      None.
//
//===========================================================================
void FeProjectLoader::OnGotoScreenComplete()
{
    this->OnResourceLoadComplete();
}
