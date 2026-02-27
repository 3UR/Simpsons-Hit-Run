//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   FeApp
//
// Description: Implementation of the FeApp class.
//
// Authors:     wng on Mon, Mar 20, 2000 @ 4:57 PM.
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================
#include "FeApp.h"

#include "utility/debugmessages.h"
#include "FeChunkHandler.h"
#include "FeProject.h"
#include "FeScreen.h"
#include "ResourceManager/FeResourceManager.h"
#include <raddebug.hpp>
#include <p3d/utility.hpp>
#include <p3d/loadmanager.hpp>
#include "feloaders.h"
#include <fetextbible.h>
#include <string.h>

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================
FeApp* FeApp::s_Instance = NULL;


//===========================================================================
// Member Functions
//===========================================================================

//===========================================================================
// FeApp::AddProject
//===========================================================================
// Description: adds a project to the app
//
// Constraints:    None
//
// Parameters:    project - the project to be added to the app
//
// Return:      None
//
//===========================================================================
void FeApp::AddProject( Scrooby::Project* project )
{
    FeProject* feProject = dynamic_cast<FeProject*>( project );
    if( feProject )
    {
        m_Projects.Insert( feProject );
    }
    else
    {
        Scrooby::ScroobyError( "Failed to add project" );
    }
}


//===========================================================================
// FeApp::GetInstance
//===========================================================================
// Description: This method is the only was for clients to access this
//              singleton.
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      Pointer to the single instance of this class.
//
//===========================================================================
FeApp* FeApp::GetInstance()
{
    if( s_Instance == NULL )
    {
        s_Instance = new FeApp;
    }
    
    return( s_Instance );
}

//===========================================================================
// GetProject()
//===========================================================================
// Description: allows access to the single project that's loaded
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      Pointer to the single project that could be loaded
//
//===========================================================================
Scrooby::Project* FeApp::GetProject()
{
    return GetProject( m_projectIndex );
}

Scrooby::Project* FeApp::GetProject( unsigned int x )
{
    if( static_cast<int>(x) >= m_Projects.Size() )
    {
        return NULL;
    }
    return dynamic_cast<Scrooby::Project*>( m_Projects[x] );
}

//===========================================================================
// GetProject() const
//===========================================================================
// Description: returns a pointer to a project that has been loaded.
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      Pointer to the single project that could be loaded
//
//===========================================================================
/*const Scrooby::Project* FeApp::GetProject() const
{
return this->m_Children[ m_projectIndex ];
}*/


//===========================================================================
// ResourceManager& FeApp::GetResourceManager()
//===========================================================================
// Description: accessor function to the resource manager object
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      reference to the resource manager
//
//===========================================================================
Scrooby::ResourceManager& FeApp::GetResourceManager()
{
    rAssert( this->m_ResourceManager != NULL );
    Scrooby::ResourceManager* returnMe;
    returnMe = dynamic_cast< Scrooby::ResourceManager* >( this->m_ResourceManager );
    rAssert( returnMe != NULL );
    return *returnMe;
}

Scrooby::TextBible*
FeApp::GetTextBible( const char* name ) const
{
    Scrooby::TextBible* textBible = NULL;
    
    tEntity* entity = m_ResourceManager->GetResource( name );
    if( entity != NULL )
    {
        textBible = dynamic_cast<Scrooby::TextBible*>( entity );
    }

    rWarningMsg( textBible != NULL, "Can't find text bible!" );

    return textBible;
}

//===========================================================================
// int FeApp::GetScreenHeight()
//===========================================================================
// Description: Returns the Y Resolution of the current screen
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      the Y Resolution (height) of the screen in pixels
//
//===========================================================================
float FeApp::GetScreenHeight()
{
    FeProject* project = GetProjectFast();
    if( !project )
    {
        return static_cast<float>( p3d::display->GetHeight() );
    }
    return project->GetScreenHeight();
}

//===========================================================================
// int FeApp::GetScreenWidth()
//===========================================================================
// Description: Returns the X Resolution of the current screen
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      the X Resolution (width) of the screen in pixels
//
//===========================================================================
float FeApp::GetScreenWidth()
{
    FeProject* project = GetProjectFast();
    if( !project )
    {
        return static_cast<float>( p3d::display->GetWidth() );
    }
    return project->GetScreenWidth();
}

//===========================================================================
// FeApp::AddScroobyHandlers
//===========================================================================
// Description: Adds the chunk handlers that scrooby cares about to P3D
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
void FeApp::AddScroobyHandlers()
{
    tP3DFileHandler* filehandler = p3d::loadManager->GetP3DHandler();
    filehandler->AddHandler( new Scrooby::FeProjectChunkHandler );
    filehandler->AddHandler( new FeTextBibleLoader() );
    p3d::context->GetLoadManager()->AddHandler( new FeProjectFileHandler(), "p3d" );
}


//===========================================================================
// FeApp::DeleteInstance
//===========================================================================
// Description: Frees the single allocated instance of this class.  The
//              client must call this during shutdown.
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
void FeApp::DeleteInstance()
{
    if( s_Instance != NULL )
    {
        delete s_Instance;
        s_Instance = NULL;
    }
}


//===========================================================================
// FeApp::FeApp
//===========================================================================
// Description: Constructor.  This is private so clients can not invoke this
//              directly.
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
FeApp::FeApp() 
:
m_Callback( NULL ),
#ifdef RAD_DEBUG
m_ShowBoundingBoxes( false ),
#endif
m_ProjectLoadPending( false ),
m_DrawingFrame( false ),
m_SafeToRender( false ),
m_FullImageNames( true ),
m_projectIndex( 0 ),
m_deltaTime( 0.0f ),
m_textBuffer( NULL )
{
    m_ResourceManager = new FeResourceManager();
    //m_Fe2D =  Fe2DCore::GetInstance();// lives as a singleton
    //m_FeScreenResolution = new( ScroobyPermPool )FeScreenResolution;
    //m_FeProjectLoader = new( ScroobyPermPool )FeProjectLoader;

    // create other Scrooby singletons
    //
#ifndef RAD_RELEASE
    Scrooby::Log::GetInstance();
#endif

    AddScroobyHandlers();
    m_Projects.Reserve( 10 );
}


//===========================================================================
// FeApp::~FeApp
//===========================================================================
// Description: Destructor.  This is private so clients can not invoke this
//              directly.
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
FeApp::~FeApp()
{
    int x = m_Projects.Size() - 1;
    while( x >= 0 )
    {
        UnloadProject( GetProject( x ) );
        x = m_Projects.Size() - 1;
    }
    m_ResourceManager->Release();
}

//===========================================================================
// Display
//===========================================================================
// Description: renders a portion of a frame to the screen
//
// Constraints:    NONE
//
// Parameters:    startLayer  - this is the first layer to be drawn
//              endLayer    - this is the first layer not to be drawn
//
// Return:      None
//
//===========================================================================
void FeApp::Display( tUID startLayer, tUID endLayer )
{
    //
    // Nothing to do if no project is loaded.
    //

    // MIKE IMPROVE : re-implement this function
    /*Scrooby::Project* project = GetProject();
    if( project != NULL )
    {
        rAssert( project );
        // object on layer 0.0f is not drawn
        Fe2DCore::GetInstance()->Display( startLayer, endLayer );
    }*/
}

//===========================================================================
// Display
//===========================================================================
// Description: renders a portion of a frame to the screen
//
// Constraints:    NONE
//
// Parameters:    startLayer  - this is the first layer to be drawn
//              endLayer    - this is the first layer not to be drawn
//
// Return:      None
//
//===========================================================================
void FeApp::Display( const char* startLayer, const char* endLayer )
{
    //
    // Nothing to do if no project is loaded.
    //

    // MIKE IMPROVE : re-implement this function
    /*
    Scrooby::Project* project = this->GetProject();
    if( project != NULL )
    {
        rAssert( project );
        tUID startUID   = tEntity::MakeUID( startLayer );
        tUID endUID     = tEntity::MakeUID( endLayer );
        
        // object on layer 0.0f is not drawn
        Fe2DCore::GetInstance()->Display( startUID, endUID );
    }*/
}

//===========================================================================
// FeApp::DrawFrame
//===========================================================================
// Description: Renders a single frame from the current screen of the project
//              to the display.
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
void FeApp::DrawFrame( float deltaTime )
{
    m_deltaTime = deltaTime;
    // Nothing to do if no project is loaded.
    Scrooby::Project* project = this->GetProject();
    if( project != NULL ) 
    {        
        rAssert( project );
        FeProject* feProject = dynamic_cast<FeProject*>(project);
        if( feProject )
        {
            if( feProject->IsLoaded() )
            {
                m_DrawingFrame = true;        
                project->GetCurrentScreen()->Display();
                m_DrawingFrame = false;
            }
            else
            {
                if( !m_ResourceManager->IsAsyncLoading() )
                {
                    m_ResourceManager->ContinueLoading();
                }
            }
        }
    }
    else
    {
        if( !m_ResourceManager->IsAsyncLoading() )
        {
            m_ResourceManager->ContinueLoading();
        }
    }
}

//===========================================================================
// FeApp::DrawBackground
//===========================================================================
// Description: 
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
void FeApp::DrawBackground( float deltaTime )
{
    m_deltaTime = deltaTime;
    Scrooby::Project* project = this->GetProject();
    if( project != NULL ) 
    {
        rAssert( project );
        FeProject* feProject = dynamic_cast<FeProject*>(project);
        if( feProject )
        {
            if( feProject->IsLoaded() )
            {
                m_DrawingFrame = true;        
                FeScreen* screen = dynamic_cast<FeScreen*>(project->GetCurrentScreen());
                if( screen )
                {
                    screen->DisplayBackground();
                }
                m_DrawingFrame = false;
            }
            else
            {
                if( !m_ResourceManager->IsAsyncLoading() )
                {
                    m_ResourceManager->ContinueLoading();
                }

            }
        }
    }
    else
    {
        if( !m_ResourceManager->IsAsyncLoading() )
        {
            m_ResourceManager->ContinueLoading();
        }
    }
}


//===========================================================================
// FeApp::DrawForeground
//===========================================================================
// Description: 
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
void FeApp::DrawForeground( float deltaTime )
{
    m_deltaTime = deltaTime;
    Scrooby::Project* project = this->GetProject();
    if( project != NULL ) 
    {
        rAssert( project );
        FeProject* feProject = dynamic_cast<FeProject*>(project);
        if( feProject )
        {
            if( feProject->IsLoaded() )
            {
                m_DrawingFrame = true;        
                FeScreen* screen = dynamic_cast<FeScreen*>(project->GetCurrentScreen());
                if( screen )
                {
                    screen->DisplayForeground();
                }
                m_DrawingFrame = false;
            }
        }
    }
}

//===========================================================================
// FeApp::GetFeResourceManager()
//===========================================================================
// Description: accessor function to the resource manager object
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      pointer to the resource manager
//
//===========================================================================
FeResourceManager& FeApp::GetFeResourceManager()
{
    rAssert( m_ResourceManager != NULL );
    return *m_ResourceManager;
}



//===========================================================================
// IsSafeToRender
//===========================================================================
// Description: determines if all the resources have been loaded
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      true or false have all the resoruces been loaded
//
//===========================================================================
bool FeApp::IsSafeToRender()
{
    Scrooby::Project* project = this->GetProject();
    if( project == NULL )
    {
        return false;
    }
    if( project->IsLoadingScreens() )
    {
        return false;
    }
    return true;
}

//===========================================================================
// FeApp::LoadProject( char* path, LoadProjectCallback* pCallback )
//===========================================================================
// Description: Loads a project from a specific path, and stores it.
//              This is an ASYNCRONOUS load.
//
// Constraints:    Path must be valid, otherwise no project will be loaded
//
// Parameters:    path - The path to the location of the project file
//
//              pCallback - This client implemented callback will be invoked
//                          once the load is complete
//
// Return:      None.
//
//===========================================================================
void FeApp::LoadProject( const char* path, Scrooby::LoadProjectCallback* pCallback, char* inventorySection, radMemoryAllocator allocator )
{
    rAssert( path );
    radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
        PascalCString filePath( path );
    ::radMemorySetCurrentAllocator( old );

    // Ignore subsequent load calls until the first request is complete.
    if( m_ProjectLoadPending )
    {
        rAssertMsg( 0, "Still busy loading a project!  Go away!" );
        return;
    }

    bool allocated = false;

    if( inventorySection == NULL )
    {
        inventorySection = new char[strlen( "ScroobySection_" ) + 2];
        sprintf( inventorySection, "ScroobySection_%d", static_cast<int>( m_Projects.Size() ) );
        allocated = true;
    }
    old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_DEFAULT );
        p3d::inventory->AddSection( inventorySection );
    ::radMemorySetCurrentAllocator( old );
    
    m_ResourceManager->SetInventorySection( inventorySection );

    m_ResourceManager->SetAllocator (allocator);

    if( allocated )
    {
        delete[] inventorySection;
        inventorySection = NULL;
    }
    
    m_ProjectLoadPending = true;
    
    // Store this for now and we'll invoke it once we're done.
    m_Callback = pCallback;
    
    // Register the cement file that packs the project
    //mFeProjectLoader->RegisterCementFile();
    old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
        PascalCString fullFileName = filePath.FullFilename();
        m_ResourceManager->AddResource( filePath, NULL, RT_PROJECT, fullFileName );
    ::radMemorySetCurrentAllocator( old );
    m_ResourceManager->ContinueLoading();
}

//===========================================================================
// LoadResourcesAll();
//===========================================================================
// Description: triggers a load of all the resources
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      None.
//
//===========================================================================
void FeApp::LoadResourcesAll( Scrooby::ResourceManagerCallback* callback )
{
    this->m_ResourceManager->LoadAll( callback );
}


//===========================================================================
// MarkSafeToRender();
//===========================================================================
// Description: flags the front end as safe to render - unless this gets 
//              called, no rendering will occur
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      None.
//
//===========================================================================
void FeApp::MarkSafeToRender()
{
    this->m_SafeToRender = true;
}

//===========================================================================
// MarkUnsafeToRender();
//===========================================================================
// Description: flags the front end as unsafe to render 
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      None.
//
//===========================================================================
void FeApp::MarkUnsafeToRender()
{
    this->m_SafeToRender = false;
}


//===========================================================================
// void FeApp::OnProjectLoadComplete()
//===========================================================================
// Description: FeProjectLoader will invoke this when the project is
//              finished all the asynchronous loading.
//
// Parameters:    pProject - The new project that has been loaded.
//
// Constraints:    We are responsible for deleting the project when it is
//              unloaded (or we shutdown).
//
// Return:      None.
//
//===========================================================================
void FeApp::OnProjectLoadComplete( Scrooby::Project* pProject )
{
    Scrooby::LoadProjectCallback* tempCallback;

    rAssert( pProject );
    //mProject = dynamic_cast< FeProject* >( pProject );
    //rAssert( mProject );
    
    //this->AddProject( pProject );
    
    //
    // Clear the flag so the client can now request another load.
    //
    m_ProjectLoadPending = false;
    
    //
    // Invoke the client supplied callback.
    //

    //
    // NOTE: this is necessary in the case that two Scrooby projects are loaded
    //       back-to-back, since m_Callback gets set for the second project in 
    //       the call to OnProjectLoadComplete()
    //
    tempCallback = m_Callback;
    m_Callback = NULL;

    if( tempCallback != NULL )
    {
        tempCallback->OnProjectLoadComplete( pProject );
    }
}

//===========================================================================
// RegisterCementFile
//===========================================================================
// Description: registers a cement file for the resource manager to use 
//              when opening files
//
// Parameters:    filename - string representing the name of the cement file
//
// Return:      None
//
//===========================================================================
void FeApp::RegisterCementFile( const char* filename )
{
    this->m_ResourceManager->RegisterCementFile( filename );
}

//===========================================================================
// RemoveProject
//===========================================================================
// Description: removes a project from the App
//
// Parameters:    project - the project to be removed
//
// Return:      None
//
//===========================================================================
void FeApp::RemoveProject( Scrooby::Project* project )
{
    char temp[80];
    FeProject* feProject = dynamic_cast<FeProject*>( project );
    strcpy( temp, feProject->GetInventorySection() );
    m_ResourceManager->SetInventorySection( temp );
    m_ResourceManager->RemoveResource( feProject );
    m_Projects.Erase( feProject );
    feProject->Release();

    p3d::inventory->RemoveSectionElements( temp );
    p3d::inventory->DeleteSection( temp );
    
    //couldn't find this particular project
    //Scrooby::Log::Message( Scrooby::LVL_ERROR, "FeApp::RemoveProject : tried to remove a project which was not in it's inventory" );
    //rAssert( false );
}


//===========================================================================
// void FeApp::SetLocalizationLanguage( const XLLanguage lang )
//===========================================================================
// Description: changes the localization language
//
// Parameters:    lang - the language we're changing to
//
// Return:      None
//
//===========================================================================
void FeApp::SetLocalizationLanguage( const Scrooby::XLLanguage lang )
{
    this->m_ResourceManager->SetLocalizationLanguage( lang );
    this->m_ResourceManager->ContinueLoading();
}

//===========================================================================
// void FeApp::SetProjectIndex
//===========================================================================
// Description : Changes the index to the current project to be drawn when
//               multiple projects are loaded
//
// Parameters  : x - the index of the new project to be the current project
//
// Return      : None
//
//===========================================================================
void FeApp::SetProjectIndex( unsigned int x )
{
    m_projectIndex = x;
}

void FeApp::SetProject( Scrooby::Project* project )
{
    rAssert( project != NULL );

    FeProject* feProject = dynamic_cast<FeProject*>( project );
    for( unsigned int i = 0; i < static_cast<unsigned int>( m_Projects.Size() ); i++ )
    {
        if( feProject == m_Projects[ i ] )
        {
            m_projectIndex = i;

            break;
        }
    }
}

//===========================================================================
// void FeApp::SetProjectIndex
//===========================================================================
// Description : For retrieving the index number of the currently loaded project
//
// Return      : the index to the current project
//
//===========================================================================
unsigned int FeApp::GetProjectIndex()
{
    return m_projectIndex;
}
//===========================================================================
// void FeApp::UnloadProject( Scrooby::Project* pProject )
//===========================================================================
// Description: Unloads the specified project.
//
// Parameters:    pProject - The project to unload.
//
// Return:      None.
//
//===========================================================================
void FeApp::UnloadProject( Scrooby::Project* pProject )
{
    if( pProject == NULL )
    {
        return;
    }
    rAssert( pProject );
    
    FeProject* feProject = dynamic_cast<FeProject*>(pProject);
    unsigned int i = 0;
    for( i; i < static_cast<unsigned int>(m_Projects.Size()); i++ )
    {
        if( feProject == m_Projects[i] )
        {
            break;
        }
    }
    if( i <= m_projectIndex )
    {
        m_projectIndex = m_projectIndex > 0 ? m_projectIndex - 1 : 0;
    }

    if( feProject )
    {
        if( feProject->IsLoaded() )
        {
            RemoveProject( pProject );
        }
        else
        {
            //Scrooby::Log::Message( Scrooby::LVL_ERROR, "Project %s is still loading.  Unload ignored", feProject->GetName() );
        }
    }
    return;
}

float FeApp::GetDeltaTime()
{
    return m_deltaTime;
}
