//=============================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All Rights Reserved.
//
// Component   : Scrooby
//
// Description : P3D loading classes for Scrooby
//
// Authors     : Mike Perzel(10/31/01)
//
//=============================================================================

//=============================================================================
// Includes
//=============================================================================
#include <constants/chunkids.hpp>
#include <p3d/chunkfile.hpp>
#include <p3d/utility.hpp>
#include "FeLoaders.h"
#include <string.h>

#include "utility/debugmessages.h"
#include "FeApp.h"
#include "FeGroup.h"
#include "FeLanguage.h"
#include "FeLayer.h"
#include "FeLoadTags.h"
#include "FePage.h"
#include "FePolygon.h"
#include "FeProject.h"
#include "FePure3dObject.h"
#include "FeScreen.h"
#include "FeSprite.h"
#include "FeTextBible.h"
#include "FeTextStyle.h"
#include "FeText.h"
#include "strings/pcstring.h"
#include "resourcemanager/FeResourceManager.h"
#include "xml/xmltree.h"
#include "strings/stricmp.h"

//=============================================================================
// Local constants, typedefs, macros
//=============================================================================

//=============================================================================
// Global data, local data, local classes
//=============================================================================

bool FeTextBibleLoader::s_onlyLoadCurrentLanguage = false;
bool FeTextBibleLoader::s_useLastLoadedTextBible = false;

//=============================================================================
// Member functions
//=============================================================================

//=============================================================================
// FeProjectFileLoader::FeProjectFileLoader
//=============================================================================
// Description : Creates a new loader class and initializes it's applicable
//               file extensions
//
//=============================================================================
FeProjectFileHandler::FeProjectFileHandler()
{
    m_extensions = new const char*[FT_NO_TYPE];
    m_extensions[FT_XML] = ".xml"; // generic XML tree
    m_extensions[FT_PRJ] = ".prj"; // Scrooby Project File
    m_extensions[FT_SCR] = ".scr"; // Scrooby Screen File
    m_extensions[FT_PAG] = ".pag"; // Scrooby Page File
    m_extensions[FT_TS] = ".ts";  // Scrooby Text Style File

    m_tempString = new PascalCString();
}

//=============================================================================
// FeProjectFileLoader::FeProjectFileLoader
//=============================================================================
// Description : Basic Deconstructor
//
// Constraints : This is protected to force reference counting.  Use Release().
//
//=============================================================================
FeProjectFileHandler::~FeProjectFileHandler()
{
    delete m_tempString;
    delete[] m_extensions;
}

//=============================================================================
// FeProjectFileLoader::CheckExtension
//=============================================================================
// Description : Checks to see if a file of a certain extension is associated
//               with this loader
//
// Constraints : Called internally by the P3D loading system.
//
// Parameters  : filename - the name of the file to check the extension on. 
//                          This filename must have a period before the extension
//                          (like in the DOS 8.3 format)
//
// Returns     : True if the filename has one of the associated extensions,
//               False otherwise
//
//=============================================================================
bool FeProjectFileHandler::CheckExtension( char* filename )
{
    int fLength = strlen( filename );
    if( fLength > 3 )
    {
        int extensionPos = 0;
        while(( extensionPos < fLength ) && ( filename[extensionPos] != '.' ))
        {
            extensionPos++;
        }
        if( extensionPos < fLength )
        {
            int i = 0;
            for( ; i < FT_NO_TYPE; i++ )
            {
                if( rstricmp( filename + extensionPos, m_extensions[i] ) == 0 )
                {
                    return true;
                }
            }
        }
    }
    return false;
}

//=============================================================================
// FeProjectFileLoader::Load
//=============================================================================
// Description : Loads a file associated
//
// Constraints : Called internally by the P3D loading system.
//
// Parameters  : file - the p3d file object to load the data from
//               store - the inventory store to store the parsed object in
//
// Returns     : LOAD_OK on success, LOAD_ERROR otherwise
//
//=============================================================================
tLoadStatus FeProjectFileHandler::Load( tFile* file, tEntityStore* store )
{
    char* dataBuffer = new char[file->GetSize()];

    //Scrooby::Log::Message( Scrooby::LVL_DEBUG, "Loading XML File : %s", file->GetFullFilename() );

    // This call will pretty much kill any hope of asynchronous parsing
    file->GetData( dataBuffer, file->GetSize() );

    XMLTree* tree = new XMLTree();
    FeEntity* rValue = NULL;
    if( rstricmp( file->GetExtension(), m_extensions[FT_PRJ] ) == 0 )
    {
        if( tree->LoadTreeFromBuffer( dataBuffer, file->GetSize(), TAG_Project ) )
        {
            // Fake the filename, since we're loading from a buffer
            tree->SetFilename( file->GetFullFilename() );
            rValue = LoadProject( tree );
        }
    }
    else if ( rstricmp( file->GetExtension(), m_extensions[FT_SCR] ) == 0 )
    {
        if( tree->LoadTreeFromBuffer( dataBuffer, file->GetSize(), TAG_Screen ) )
        {
            // Fake the filename, since we're loading from a buffer
            tree->SetFilename( file->GetFullFilename() );
            rValue = LoadScreen( tree );
        }
    }
    else if ( rstricmp( file->GetExtension(), m_extensions[FT_PAG] ) == 0 )
    {
        if( tree->LoadTreeFromBuffer( dataBuffer, file->GetSize(), TAG_Page ) )
        {
            // Fake the filename, since we're loading from a buffer
            tree->SetFilename( file->GetFullFilename() );
            rValue = LoadPage( tree );
        }
    }
    else if ( rstricmp( file->GetExtension(), m_extensions[FT_TS] ) == 0 )
    {
        if( tree->LoadTreeFromBuffer( dataBuffer, file->GetSize(), TAG_TextStyle ) )
        {
            // Fake the filename, since we're loading from a buffer
            tree->SetFilename( file->GetFullFilename() );
            rValue = LoadTextStyle( tree );
        }
    }
    else
    {
    }
    
    // Clean up the objects before we decide whether to return or not.
    delete tree;
    delete[] dataBuffer;

    if( rValue != NULL )
    {
        store->Store( static_cast<tEntity*>( rValue ) );
        return LOAD_OK;
    }

    return LOAD_ERROR;
}

//=============================================================================
// FeProjectFileHandler::LoadProject
//=============================================================================
// Description : Loads a Scooby XML Project file
//
// Constraints : should only be called from the loader
//
// Parameters  : loadTree - the parsed XML tree containing the Project file
//
// Returns     : an FeProject object on successful loading,
//               NULL otherwise
//
//=============================================================================
FeProject* FeProjectFileHandler::LoadProject( XMLTree* loadTree )
{
    loadTree->GetFilename( *m_tempString );
    FeProject* rValue = new FeProject( (const char*)*m_tempString );

    // ******* Start Project File Parsing ******* //

    //While it isn't neccessary to parse the tags in order, it should be
    //faster to do so.

    // Ignore the version number

    int height = 0;
    int width = 0;
    // Get the Resolution
    if( loadTree->SetCurrentElementByName( TAG_Resolution ) )
    {
        int width, height;
        if( loadTree->GetAttribute( TAG_Width, &width ) &&
            loadTree->GetAttribute( TAG_Height, &height ) )
        {
            rValue->SetScreenRes( static_cast<float>( width ), static_cast<float>( height ) );
        }
    }

    // Ignore the Platform Name

    // Get the paths for the project resources
    if( ( loadTree->SetCurrentElementByName( TAG_ScreenPath ) ) &&
        ( loadTree->GetAttribute( TAG_Value, *m_tempString ) ) )
    {
        rValue->SetScreenPath( *m_tempString );
    }
    if( ( loadTree->SetCurrentElementByName( TAG_PagePath ) ) &&
        ( loadTree->GetAttribute( TAG_Value, *m_tempString ) ) )
    {
        rValue->SetPagePath( *m_tempString );
    }

    if( ( loadTree->SetCurrentElementByName( TAG_ResourcePath ) ) &&
        ( loadTree->GetAttribute( TAG_Value, *m_tempString ) ) )
    {
        rValue->SetResPath( *m_tempString );
    }
    // Add the pages to the ResourceManager for loading
    XMLTree* subTree = loadTree->GetSubTreeByName( TAG_Screens );
    rAssert( subTree );

    int i = 0;
    int lastScreen = 0;
    
    while( subTree->SetCurrentElementByIndex( i ) )
    {
        if( subTree->GetAttribute( TAG_File, *m_tempString) != 0 )
        {
            PascalCString temp = rValue->GetProjectPath();
            temp += rValue->GetScreenPath();
            temp += *m_tempString;
            lastScreen = FeApp::GetInstance()->GetFeResourceManager().AddResource( temp, rValue, RT_SCREEN, m_tempString->FullFilename() );
        }        
        ++i;
    }

    rValue->SetLastScreenToLoad( lastScreen );
    
    delete subTree;
    
    // ******* End Project File Parsing ******* //
    
    return rValue;
}

//=============================================================================
// FeProjectFileHandler::LoadScreen
//=============================================================================
// Description : Loads a Scooby XML Screen file
//
// Constraints : should only be called from the loader
//
// Parameters  : loadTree - the parsed XML tree containing the Screen file
//
// Returns     : an FeScreen object on successful loading,
//               NULL otherwise
//
//=============================================================================
FeScreen* FeProjectFileHandler::LoadScreen( XMLTree* loadTree )
{
    loadTree->GetFilename( *m_tempString );
    FeScreen* rValue = new FeScreen( (const char*)*m_tempString, NULL ); 
    
    XMLTree* subTree = loadTree->GetSubTreeByName( TAG_Pages );
    
    rAssert( subTree );

    int i = 0;
    
    bool isExport = false;
    while( subTree->SetCurrentElementByIndex( i ) )
    {
        if( subTree->GetAttribute( TAG_File, *m_tempString ) != 0)
        {            
            if( subTree->GetAttribute( TAG_Export, &isExport ) )
            {
                if( isExport )
                {
                    PascalCString temp = FeApp::GetInstance()->GetFeResourceManager().GetLoadingProject()->GetProjectPath();
                    temp += FeApp::GetInstance()->GetFeResourceManager().GetLoadingProject()->GetPagePath();
                    temp += *m_tempString;
                    FeApp::GetInstance()->GetFeResourceManager().AddResource( temp, rValue, RT_PAGE, m_tempString->FullFilename() );
                }
            }
        }        
        ++i;
    }
    
    delete subTree;
    return rValue;

}

//=============================================================================
// FeProjectFileHandler::LoadPage
//=============================================================================
// Description : Loads a Scooby XML Page file
//
// Constraints : should only be called from the loader
//
// Parameters  : loadTree - the parsed XML tree containing the page file
//
// Returns     : an FePage object on successful loading,
//               NULL otherwise
//
//=============================================================================
FePage* FeProjectFileHandler::LoadPage( XMLTree* loadTree )
{
    loadTree->GetFilename( *m_tempString );
    FePage* rValue = new FePage( (const char*)*m_tempString, NULL );


    // ******* Begin Page File Parsing ******* //
    
    // Ignore the version

    // Ignore the resolution

    // Parse the resources section
    ParsePageResources( loadTree, rValue );

    // Parse the layers section
    
    XMLTree* pLayersSubTree = loadTree->GetSubTreeByName( TAG_Layers );
    if( pLayersSubTree )
    {
        
        int i = 0;
        
        XMLTree* pCurrentLayerTree = pLayersSubTree->GetSubTreeByIndex( i );
        
        while( NULL != pCurrentLayerTree )
        {
            if( pCurrentLayerTree->GetAttribute( TAG_AttrName, *m_tempString ) > 0 )
            {            
                FeLayer* pLayer = new FeLayer( *m_tempString );

                bool visibility = false;
                if( pCurrentLayerTree->GetAttribute( TAG_AttrVisible, *m_tempString ) && (*m_tempString == "true") )
                {
                    visibility = true;
                }
                else
                {
                    visibility = false;
                }
                                                
                pLayer->SetVisible( visibility );
                
                ParseDrawables( pCurrentLayerTree, pLayer );
                rValue->AddChild( pLayer );
            }

            delete pCurrentLayerTree;
            
            pCurrentLayerTree = pLayersSubTree->GetSubTreeByIndex( ++i );
        }
        
        delete pLayersSubTree;
    }


    // ******* End Page File Parsing ******* //

    return rValue;
}

//=============================================================================
// FeProjectFileHandler::LoadTextStyle
//=============================================================================
// Description : Loads a Scooby XML TextStyle file
//
// Constraints : should only be called from the loader
//
// Parameters  : loadTree - the parsed XML tree containing the TextStyle file
//
// Returns     : an FeEntity object on successful loading, (no FeTextStyle object yet)
//               NULL otherwise
//
//=============================================================================
FeEntity* FeProjectFileHandler::LoadTextStyle( XMLTree* loadTree )
{
    FeTextStyle* rValue = new FeTextStyle();

    loadTree->GetFilename( *m_tempString );
    rValue->SetName( ( (const char*)(*m_tempString).FullFilename() ) );

    if( loadTree->SetCurrentElementByName( TAG_TextStyleFont ) )
    {
        if( loadTree->GetAttribute( TAG_AttrName, *m_tempString ) )
        {
            rValue->SetFontName( *m_tempString );
        }

        if( loadTree->GetAttribute( TAG_AttrData, *m_tempString ) )
        {
            rValue->SetFontFilename( *m_tempString );
        }
        else
        {
            // The font filename is absolutely required
            rAssert( false );
        }
    }

    if( loadTree->SetCurrentElementByName( TAG_Style ) )
    {
        if( loadTree->GetAttribute( TAG_Value, *m_tempString ) )
        {
            rValue->SetFontStyle( *m_tempString );
        }
    }

    int size = 20;
    if( loadTree->SetCurrentElementByName( TAG_Size ) )
    {
        if( !loadTree->GetAttribute( TAG_Value, &size ) )
        {
            // There should be a size specified
            rAssert( false );
        }
    }
    rValue->SetSize( size );

    int r, g, b, a;
    r = g = b = a = 0xff;
    if( loadTree->SetCurrentElementByName( TAG_Colour ) )
    {
        if( 
            loadTree->GetAttribute( TAG_ColorRed, &r ) &&
            loadTree->GetAttribute( TAG_ColorGreen, &g ) &&
            loadTree->GetAttribute( TAG_ColorBlue, &b ) &&
            loadTree->GetAttribute( TAG_ColorAlpha, &a )
          )
        {
            // Colour isn't required, so allow it to be flexible
        }
    }
    rValue->SetColour( new tColour( r, g, b, a ) );

    return rValue;
}

//===========================================================================
// void FeProjectFileHandler::ParsePageResources
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
void FeProjectFileHandler::ParsePageResources( XMLTree* pXmlTree, FePage* pOwningPage )
{
    rAssert( pXmlTree );
    rAssert( pOwningPage );

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
            if( pImageSubTree->GetAttribute( TAG_AttrName, alias ) )
            {
                if( pImageSubTree->GetAttribute( TAG_AttrData, filename ) )
                {
                    *m_tempString = FeApp::GetInstance()->GetFeResourceManager().GetLoadingProject()->GetProjectPath();
                    *m_tempString += FeApp::GetInstance()->GetFeResourceManager().GetLoadingProject()->GetResPath();
                    *m_tempString += filename;

                    if( !FeApp::GetInstance()->UseFullImageNames() || filename.JustExtension().EqualsInsensitive( "p3d" ) )
                    {
                        filename = filename.FullFilename();
                    }
                    else
                    {
                        filename = *m_tempString;
                    }
                    int index = FeApp::GetInstance()->GetFeResourceManager().AddResource( *m_tempString, NULL, RT_IMAGE, filename );
                    pOwningPage->AddResource( index );
                    FeApp::GetInstance()->GetFeResourceManager().AddAlias( static_cast< const char* >( alias ), index );                }
            }               
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
        PascalCString inventoryName;
        
        while( pTextBibleSubTree->SetCurrentElementByIndex( i ) )
        {
            if( pTextBibleSubTree->GetAttribute( TAG_AttrName, alias ) )
            {
                if( pTextBibleSubTree->GetAttribute( TAG_AttrData, filename ) )
                {
                    *m_tempString = FeApp::GetInstance()->GetFeResourceManager().GetLoadingProject()->GetProjectPath();
                    *m_tempString += FeApp::GetInstance()->GetFeResourceManager().GetLoadingProject()->GetResPath();
                    *m_tempString += filename;

                    if( !pTextBibleSubTree->GetAttribute( TAG_InventoryName, inventoryName ) )
                    {
                        inventoryName = filename.JustFilename();
                    }

                    int index = FeApp::GetInstance()->GetFeResourceManager().AddResource( *m_tempString, NULL, RT_TEXT_BIBLE, inventoryName );
                    pOwningPage->AddResource( index );
                    FeApp::GetInstance()->GetFeResourceManager().AddAlias( static_cast< const char* >( alias ), index );
                }
            }
            
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
        PascalCString inventoryName;
        
        while( pTextStyleSubTree->SetCurrentElementByIndex( i ) )
        {
            if( pTextStyleSubTree->GetAttribute( TAG_AttrName, alias ) )
            {
                if( pTextStyleSubTree->GetAttribute( TAG_AttrData, filename ) )
                {
                    *m_tempString = FeApp::GetInstance()->GetFeResourceManager().GetLoadingProject()->GetProjectPath();
                    *m_tempString += FeApp::GetInstance()->GetFeResourceManager().GetLoadingProject()->GetResPath();
                    *m_tempString += filename;

                    if( !pTextStyleSubTree->GetAttribute( TAG_InventoryName, inventoryName ) )
                    {
                        inventoryName = filename.FullFilename();
                    }

                    int index = FeApp::GetInstance()->GetFeResourceManager().AddResource( *m_tempString, NULL, RT_TEXT_STYLE, inventoryName );
                    pOwningPage->AddResource( index );
                    FeApp::GetInstance()->GetFeResourceManager().AddAlias( static_cast< const char* >( alias ), index );
                }
            }            
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
        //Movie clips aren't supported right now

        /*int i = 0;

        PascalCString alias;
        PascalCString filename;
        
        while( pMovieClipSubTree->SetCurrentElementByIndex( i ) )
        {
            if( pMovieClipSubTree->GetAttribute( TAG_AttrName, alias ) )
            {
                if( pMovieClipSubTree->GetAttribute( TAG_AttrData, filename ) )
                {
                    pOwningPage->AddMovieClipResourceEntry( alias, filename );
                }
            }            
            ++i;
        }*/
        
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
            if
            ( 
                p3dModelSubTree->GetAttribute( TAG_AttrName, alias ) &&
                p3dModelSubTree->GetAttribute( TAG_AttrData, filename ) &&
                p3dModelSubTree->GetAttribute( TAG_Pure3dInventoryName, inventoryName )
            )
            {
                // These two can be NULL, so we won't check them
                p3dModelSubTree->GetAttribute( TAG_Pure3dCameraName, cameraName );
                p3dModelSubTree->GetAttribute( TAG_Pure3dAnimationName, animationName );

                *m_tempString = FeApp::GetInstance()->GetFeResourceManager().GetLoadingProject()->GetProjectPath();
                *m_tempString += FeApp::GetInstance()->GetFeResourceManager().GetLoadingProject()->GetResPath();
                *m_tempString += filename;
                
                int index = FeApp::GetInstance()->GetFeResourceManager().AddResource( *m_tempString, NULL, RT_P3D_OBJECT, inventoryName );
                pOwningPage->AddResource( index );
                FeApp::GetInstance()->GetFeResourceManager().AddAlias( static_cast< const char* >( alias ), index );
                FeApp::GetInstance()->GetFeResourceManager().AddPure3dAttributes( cameraName, animationName, index );
            }
            
            ++i;
        }
        
        delete p3dModelSubTree;
        p3dModelSubTree = NULL;
    }


    delete pResourceSubTree;
}

void FeProjectFileHandler::ParseDrawables( XMLTree* pXmlTree, FeOwner* pOwner )
{
    rAssert( pXmlTree );
    rAssert( pOwner );

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

            if (pCurrentDElementSubTree->GetName( *m_tempString ) > 0 )
            {                
                //
                // Figure out what type of drawing element we've got and parse it.
                //
                if( ( ::rstricmp( *m_tempString, TAG_Sprite ) == 0 ) ||
                    ( ::rstricmp( *m_tempString, TAG_MultiSprite ) == 0 ) )
                {
                    ParseSprite( pCurrentDElementSubTree, pOwner );
                }
                else if( ( ::rstricmp( *m_tempString, TAG_Text ) == 0 ) ||
                    ( ::rstricmp( *m_tempString, TAG_MultiText ) == 0 ) )
                {
                    ParseText( pCurrentDElementSubTree, pOwner );
                }
                else if( ::rstricmp( *m_tempString, TAG_Polygon ) == 0 )
                {
                    ParsePolygon( pCurrentDElementSubTree, pOwner );
                }
                else if( ::rstricmp( *m_tempString, TAG_Group ) == 0 )
                {
                    ParseGroup( pCurrentDElementSubTree, pOwner );
                }
                else if( ::rstricmp( *m_tempString, TAG_Movie ) == 0 )
                {
                    ParseMovie( pCurrentDElementSubTree, pOwner );
                }
                else if( ::rstricmp( *m_tempString, TAG_Pure3dObject ) == 0 )
                {
                    ParsePure3dObject( pCurrentDElementSubTree, pOwner );
                }
                else
                {

                }
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
    }
    return;
}

void FeProjectFileHandler::ParseGroup( XMLTree* pXmlTree, FeOwner* pOwner )
{
    rAssert( pXmlTree );
    rAssert( pOwner );
    
    //
    // Read the group's name.
    //
    PascalCString name;
    if( pXmlTree->GetAttribute( TAG_AttrName, *m_tempString ) > 0 )
    {
        FeGroup* pGroup = new FeGroup( static_cast< const char* >( *m_tempString ) );

        int alpha = 255;
        if( !pXmlTree->GetAttribute( TAG_AttrAlpha, &alpha ) )
        {
            // Not required, so do nothing
        }
        pGroup->SetAlpha( static_cast<float>(alpha) / 255.0f );
        
        //
        // Recurse on the drawables contained in this group.
        //
        this->ParseDrawables( pXmlTree, pGroup );
        
        pOwner->AddChild( pGroup );
    }
    
    return;
    
}

void FeProjectFileHandler::ParseMovie( XMLTree* pXmlTree, FeOwner* pOwner )
{
}

void FeProjectFileHandler::ParsePolygon( XMLTree* pXmlTree, FeOwner* pOwner )
{
    rAssert( pXmlTree );
    rAssert( pOwner );
    
    // Read the name.
    if( pXmlTree->GetAttribute( TAG_AttrName, *m_tempString ) )
    {
        XMLTree* pVerticesTree = pXmlTree->GetSubTreeByName( TAG_Vertexes );
        rAssert( pVerticesTree );

        FePolygon* pPolygon = new FePolygon( (const char*)*m_tempString );
        
        // Iterate through all the vertices...
        int i = 0;        
        XMLTree* pCurrentVertexTree = pVerticesTree->GetSubTreeByIndex( i );
        
        while( NULL != pCurrentVertexTree )
        {
            // Read and set the vertex position.
            int x = 0;
            int y = 0;
            if( pCurrentVertexTree->SetCurrentElementByName( TAG_Position ) )
            {
                pCurrentVertexTree->GetAttribute( TAG_PositionX, &x );
                pCurrentVertexTree->GetAttribute( TAG_PositionY, &y );
            }
            pPolygon->SetVertexLocation( i, x, y );            
                        
            // Read and set the vertex colour and alpha.
            int red = 0xff;
            int green = 0xff;
            int blue = 0xff;
            int alpha = 0xff;
            if( pCurrentVertexTree->SetCurrentElementByName( TAG_Color ) )
            {
                pCurrentVertexTree->GetAttribute( TAG_ColorRed, &red );
                pCurrentVertexTree->GetAttribute( TAG_ColorGreen, &green );
                pCurrentVertexTree->GetAttribute( TAG_ColorBlue, &blue );
                pCurrentVertexTree->GetAttribute( TAG_ColorAlpha, &alpha );
            }
            pPolygon->SetVertexColour( i, red, green, blue );
            pPolygon->SetVertexAlpha( i, float( alpha / 255.0f ) );
            
            // Release the current vertex tree and try to get the next one.
            delete pCurrentVertexTree;
            
            pCurrentVertexTree = pVerticesTree->GetSubTreeByIndex( ++i );
        }
        
        pPolygon->SetNumVertex( i );   

        pOwner->AddChild( pPolygon );
        delete pVerticesTree;
    }
}

void FeProjectFileHandler::ParsePure3dObject( XMLTree* pXmlTree, FeOwner* pOwner )
{
    rAssert( pXmlTree );
    rAssert( pOwner );
    
    if( pXmlTree->GetAttribute( TAG_AttrName, *m_tempString ) )
    {
        int x = 0;
        int y = 0;
        if( pXmlTree->SetCurrentElementByName( TAG_Position ) )
        {
            if( !pXmlTree->GetAttribute( TAG_PositionX, &x ) || !pXmlTree->GetAttribute( TAG_PositionY, &y ) )
            {
                //Position is required
                rAssert( false );
            }
        }
        else        
        {
            // There must be a position
            rAssert( false );
        }
        
        FePure3dObject* pPure3dObject = new FePure3dObject( static_cast< const char* >( *m_tempString ), x, y );
        
        // Read and set the bounding box.
        int width = 0;
        int height = 0;
        if( pXmlTree->SetCurrentElementByName( TAG_Dimension ) )
        {
            if( !pXmlTree->GetAttribute( TAG_DimensionWidth, &width ) || !pXmlTree->GetAttribute( TAG_DimensionHeight, &height ) )
            {
                // there was a problem reading one of the dimensions
                rAssert( false );
            }
        }
        else
        {           
            // There must be a bounding box
            rAssert( false );
        }       
        
        pPure3dObject->SetBoundingBoxSize( width, height );
        
        // Read and set the justification.
        if( pXmlTree->SetCurrentElementByName( TAG_Justification ) )
        {
            if( pXmlTree->GetAttribute( TAG_JustificationVertical, *m_tempString ) )
            {
                pPure3dObject->SetVerticalJustification( StringToJustificationEnum( *m_tempString ) );
            }
            if( pXmlTree->GetAttribute( TAG_JustificationHorizontal, *m_tempString ) )
            {
                pPure3dObject->SetHorizontalJustification( StringToJustificationEnum( *m_tempString ) );
            }
        }
        // Ignore Colour
        // Ignore Translucency
        // Ignore Rotation

        // Read the Pure3d File alias
        if( pXmlTree->SetCurrentElementByName( TAG_3dModel ) )
        {
            if( pXmlTree->GetAttribute( TAG_AttrName, *m_tempString ) )
            {
                pPure3dObject->Add3dModel( *m_tempString );
            }
        }

        pOwner->AddChild( pPure3dObject );
        
    }
}

void FeProjectFileHandler::ParseSprite( XMLTree* pXmlTree, FeOwner* pOwner )
{
    rAssert( pXmlTree );
    rAssert( pOwner );
    
    // Is this a sprite or multisprite?
    PascalCString spriteType;
    
    int charCount = pXmlTree->GetName( spriteType );
    
    // Read the name of this drawable.
    if( pXmlTree->GetAttribute( TAG_AttrName, *m_tempString ) )
    {        
        // Read and set the position.
        int x = 0;
        int y = 0;
        if( pXmlTree->SetCurrentElementByName( TAG_Position ) )
        {
            pXmlTree->GetAttribute( TAG_PositionX, &x );
            pXmlTree->GetAttribute( TAG_PositionY, &y );
        }
        else
        {
            // sprites require a position tag
            rAssert( false );
        }
        
        FeSprite* pSprite = new FeSprite( static_cast< const char* >( *m_tempString ), x, y );
            
        //read and set the color
        int red = 0xff;
        int green = 0xff;
        int blue = 0xff;
        int alpha = 0xff;
        if( pXmlTree->SetCurrentElementByName( TAG_Color ) )
        {
            
            if( !pXmlTree->GetAttribute( TAG_ColorRed, &red ) ||
                !pXmlTree->GetAttribute( TAG_ColorGreen, &green ) ||
                !pXmlTree->GetAttribute( TAG_ColorBlue, &blue ) ||
                !pXmlTree->GetAttribute( TAG_ColorAlpha, &alpha )
              )
            {
                //Colour tags aren't enforced, so do nothing
            }
        }
        pSprite->SetColour( tColour( red, green, blue, alpha ) );
        
        
        // Read and set the bounding box.
        int width = 0;
        int height = 0;
        if( pXmlTree->SetCurrentElementByName( TAG_Dimension ) )
        {        
            if( !pXmlTree->GetAttribute( TAG_DimensionWidth, &width ) || !pXmlTree->GetAttribute( TAG_DimensionHeight, &height ) )
            {
                // Error reading one of the dimension tags
                rAssert( false );
            }
        }
        else
        {
            // Sprites require a bounding box
            rAssert( false );
        }
        pSprite->SetBoundingBoxSize( width, height );
        
        // Read and set the justification.
        if( pXmlTree->SetCurrentElementByName( TAG_Justification ) )
        {        
            if( pXmlTree->GetAttribute( TAG_JustificationVertical, *m_tempString ) > 0 )
            {            
                pSprite->SetVerticalJustification( StringToJustificationEnum( *m_tempString ) );
            }            
            if( pXmlTree->GetAttribute( TAG_JustificationHorizontal, *m_tempString ) > 0 )
            {
                pSprite->SetHorizontalJustification( StringToJustificationEnum( *m_tempString ) );
            }
            
        }
        
        
        // Is it a sprite or multisprite?
        if( ::rstricmp( spriteType, TAG_Sprite ) == 0 )
        {
            // There is only one image
            if( pXmlTree->SetCurrentElementByName( TAG_Image ) )
            {            
                if( pXmlTree->GetAttribute( TAG_AttrName, *m_tempString ) > 0 )
                {
                    pSprite->AddImage( *m_tempString );
                }            
            }
        }
        else
        {
            // Load multiple images.
            XMLTree* pImagesTree = pXmlTree->GetSubTreeByName( TAG_Images );
            rAssert( pImagesTree );
            
            int i = 0;
            
            while( pImagesTree->SetCurrentElementByIndex( i++ ) )
            {
                if( pImagesTree->GetAttribute( TAG_AttrName, *m_tempString ) > 0 )
                {
                    pSprite->AddImage( *m_tempString );
                }
            }
            
            delete pImagesTree;
        }

        pOwner->AddChild( pSprite );
    }
    
    return;
}

void FeProjectFileHandler::ParseText( XMLTree* pXmlTree, FeOwner* pOwner )
{
    FeText* pText = NULL;

    // is this a multi-text, or a single text
    PascalCString textType;
    pXmlTree->GetName( textType );

    pXmlTree->GetAttribute( TAG_AttrName, *m_tempString );

    int x = 0, y = 0;
    if( !pXmlTree->SetCurrentElementByName( TAG_Position ) || !pXmlTree->GetAttribute( TAG_PositionX, &x ) || !pXmlTree->GetAttribute( TAG_PositionY, &y ) )
    {
        //Scrooby::Log::Message( Scrooby::LVL_ERROR, "No position specified for Text object %s", (const char*)*m_tempString );
        rAssert( false );
    }

    pText = new FeText( (const char*)*m_tempString, x, y );

    int h = 0, w = 0;
    if( !pXmlTree->SetCurrentElementByName( TAG_Dimension ) || !pXmlTree->GetAttribute( TAG_Height, &h ) || !pXmlTree->GetAttribute( TAG_Width, &w ) )
    {
        //Scrooby::Log::Message( Scrooby::LVL_ERROR, "No dimension specified for Text object %s", (const char*)*m_tempString );
        rAssert( false );
    }
    pText->SetBoundingBoxSize( w, h );

    if( pXmlTree->SetCurrentElementByName( TAG_Justification ) )
    {        
        if( pXmlTree->GetAttribute( TAG_JustificationVertical, *m_tempString ) )
        {
            pText->SetVerticalJustification( StringToJustificationEnum( *m_tempString ) );
        }
        if( pXmlTree->GetAttribute( TAG_JustificationHorizontal, *m_tempString ) )
        {
            pText->SetHorizontalJustification( StringToJustificationEnum( *m_tempString ) );
        }
    }

    if( pXmlTree->SetCurrentElementByName( TAG_Colour ) )
    {
        int r, g, b, a;
        r = g = b = a = 0xff;
        if(
            pXmlTree->GetAttribute( TAG_ColorRed, &r ) &&
            pXmlTree->GetAttribute( TAG_ColorGreen, &g ) &&
            pXmlTree->GetAttribute( TAG_ColorBlue, &b ) &&
            pXmlTree->GetAttribute( TAG_ColorAlpha, &a )
          )
        {
            tColour * t = new tColour( r, g, b, a );
            pText->SetColour( *t ); 
            delete t;
        }
    }

    if( !pXmlTree->SetCurrentElementByName( TAG_TextStyle ) || !pXmlTree->GetAttribute( TAG_AttrName, *m_tempString ) )
    {
        //Scrooby::Log::Message( Scrooby::LVL_ERROR, "No text style specified for Text object %s", pText->GetName() );
    }
    else
    {
        pText->SetTextStyle( *m_tempString );
    }

    if( pXmlTree->SetCurrentElementByName( TAG_TextShadow ) )
    {
        if( pXmlTree->GetAttribute( TAG_AttrEnabled, *m_tempString ) )
        {
            pText->SetDisplayShadow( m_tempString->EqualsInsensitive( "true" ) );
        }
        int r, g, b, a;
        r = g = b = a = 0;
        if(
            pXmlTree->GetAttribute( TAG_ColorRed, &r ) &&
            pXmlTree->GetAttribute( TAG_ColorGreen, &g ) &&
            pXmlTree->GetAttribute( TAG_ColorBlue, &b ) &&
            pXmlTree->GetAttribute( TAG_ColorAlpha, &a )
          )
        {
            tColour * t = new tColour( r, g, b, a );
            pText->SetShadowColour( *t ); 
            delete t;
        }
        int x, y;
        x = y = 0;
        pXmlTree->GetAttribute( TAG_PositionX, &x );
        pXmlTree->GetAttribute( TAG_PositionY, &y );
        pText->SetShadowOffset( x, y );
    }

    int index = 0;
    if( pXmlTree->SetCurrentElementByName( TAG_CurrentIndex ) )
    {
        pXmlTree->GetAttribute( TAG_Value, &index );
    }

    if( ::rstricmp( textType, TAG_Text ) == 0 )
    {
        // Single Text Item        
        if( pXmlTree->SetCurrentElementByName( TAG_String ) )
        {
            if( pXmlTree->GetAttribute( TAG_HardCodedString, *m_tempString ) )
            {
                pText->AddHardCodedString( *m_tempString );
            }
            else if( pXmlTree->GetAttribute( TAG_AttrStringID, *m_tempString ) )
            {
                PascalCString textBible;
                if( pXmlTree->GetAttribute( TAG_AttrBible, textBible ) )
                {
                    pText->AddTextBibleString( textBible, *m_tempString );
                }
            }
        }
    }
    else
    {
        // Multi-text Item
        if( pXmlTree->SetCurrentElementByName( TAG_Strings ) )
        {
            XMLTree* pStringsTree = pXmlTree->GetSubTreeByName( TAG_Strings );
            rAssert( pStringsTree );
            
            int i = 0;
            
            while( pStringsTree->SetCurrentElementByIndex( i++ ) )
            {
                if( pStringsTree->GetAttribute( TAG_HardCodedString, *m_tempString ) )
                {
                    pText->AddHardCodedString( *m_tempString );
                }
                else if( pStringsTree->GetAttribute( TAG_AttrStringID, *m_tempString ) )
                {
                    PascalCString textBible;
                    if( pStringsTree->GetAttribute( TAG_AttrBible, textBible ) )
                    {
                        pText->AddTextBibleString( textBible, *m_tempString );
                    }
                }
            }
            
            delete pStringsTree;


        }
    }

    // We have to set this after the strings have been added
    pText->SetIndex( index );

    pOwner->AddChild( pText );
}



FeTextBibleLoader::FeTextBibleLoader()
:   tSimpleChunkHandler( Pure3D::Scrooby::TEXTBIBLE ),
    m_lastLoadedTextBible( NULL )
{
}

tEntity* FeTextBibleLoader::LoadObject( tChunkFile* file, tEntityStore* store )
{
    if( s_useLastLoadedTextBible && m_lastLoadedTextBible != NULL )
    {
        return m_lastLoadedTextBible;
    }

    char name[256];
    unsigned int numLanguages = 0;
    char languages[256];
    FeTextBible* rValue = new FeTextBible();
    m_lastLoadedTextBible = rValue;

    file->GetString( name );
    numLanguages = file->GetUInt();
    file->GetString( languages );

    rValue->SetName( name );
//    rValue->m_languageIDs = new char[strlen(languages)+1];
    strcpy( rValue->m_languageIDs, languages );

    while( file->ChunksRemaining() )
    {
        unsigned int returnMeId = file->BeginChunk();
        FeLanguage* entity = NULL;
        switch( returnMeId )
        {
        case Pure3D::Scrooby::LANGUAGE :
            entity = dynamic_cast<FeLanguage*>(LoadLanguage( file, store ));
            break;
        default:
            break;
        }
        file->EndChunk();
        if( entity )
        {
            entity->AddRef();
        }
        rValue->m_languages.Insert( entity );
    }

    if( s_onlyLoadCurrentLanguage )
    {
        rAssert( rValue->m_languages[ FeTextBible::GetLanguageIndex() ] != NULL );
        for( unsigned int i = 0; i < static_cast<unsigned int>( rValue->m_languages.Size() ); i++ )
        {
            if( i != FeTextBible::GetLanguageIndex() )
            {
                rAssert( rValue->m_languages[ i ] == NULL );
                rValue->m_languages[ i ] = rValue->m_languages[ FeTextBible::GetLanguageIndex() ];
                rValue->m_languages[ i ]->AddRef();
            }
        }
    }

    return rValue;
}

tEntity* FeTextBibleLoader::LoadLanguage( tChunkFile* file, tEntityStore* store )
{
    char name[256];
    file->GetString( name );

    char languageID = file->GetChar();

    if( s_onlyLoadCurrentLanguage )
    {
        if( languageID != FeTextBible::GetCurrentLanguageID() )
        {
            return NULL;
        }
    }

    FeLanguage* rValue = new FeLanguage( name );

    rValue->m_languageID = languageID;
    rValue->m_numStrings = file->GetUInt();
    rValue->m_modulo = file->GetUInt();
    rValue->m_bufferSize = file->GetUInt();

    rValue->m_hashes = new unsigned int[rValue->m_numStrings];
    rValue->m_offsets = new unsigned int[rValue->m_numStrings];
    rValue->m_buffer = new unsigned short[rValue->m_bufferSize/2];

    file->GetData((void*)rValue->m_hashes, rValue->m_numStrings, tFile::DWORD );
    file->GetData((void*)rValue->m_offsets, rValue->m_numStrings, tFile::DWORD );
    file->GetData((void*)rValue->m_buffer, rValue->m_bufferSize/2, tFile::WORD );

    return rValue;
}

