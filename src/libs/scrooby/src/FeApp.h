//===========================================================================
// Copyright (C) 1999 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Component
//
// Description: this is a container object that holds all the front end data
//
// Authors:     Wilkin Ng
//
// Revisions        Date            Author    Revision
//                    2000/03/20        WNG        Created
//
//===========================================================================

#ifndef FEAPP_H
#define FEAPP_H

//===========================================================================
// Nested Includes
//===========================================================================
#include "App.h"
#include "utility/rVector.hpp"
#include <radmemory.hpp>

//===========================================================================
// Forward References
//===========================================================================
class FeProject;
class FeProjectLoader;
class FeScreen;
class FeScreenResolution;
class FeResourceManager;

namespace Scrooby
{
    class ResourceManagerCallback;
    class Screen;
    class TextBible;
}

//===========================================================================
// Interface Definitions
//===========================================================================
class FeApp 
:  
//public FeAllocatesMemory,
public Scrooby::App
{
    
public:
    // Instance handler functions
    static FeApp* GetInstance();    
    static void DeleteInstance();
    
    //Adds the chunk handlers that scrooby cares about to P3D
    void AddScroobyHandlers();
    
    // Project Loading / Handling functions
    virtual void LoadProject( const char* name,
                              Scrooby::LoadProjectCallback* pCallback,
                              char* inventorySection = NULL,
                              radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );
    void UnloadProject( Scrooby::Project* pProject );
    //triggers a load of all the resources
    virtual void LoadResourcesAll( Scrooby::ResourceManagerCallback* callback );
    //returns a pointer to a project that has been loaded.
    virtual Scrooby::Project* GetProject();
    Scrooby::Project* GetProject( unsigned int x );
    //virtual const Scrooby::Project* GetProject() const;
    // For multiple project support
    void SetProjectIndex( unsigned int x );
    void SetProject( Scrooby::Project* project );
    unsigned int GetProjectIndex();

    // Rendering functions
    // Renders a frame of the current screen to the display.
    virtual void DrawFrame( float deltaTime = 16.0f );    
    // Renders all the pages on a screen up to and including the page named "FeDivider".
    virtual void DrawBackground( float deltaTime = 16.0f );   
    // Renders all the pages on a screen that come after "FeDivider".
    virtual void DrawForeground( float deltaTime = 16.0f );
    //renders a portion of a frame to the screen
    virtual void Display( const char* startLayer, const char* endLayer );
    virtual void Display( tUID startLayer, tUID endLayer );
    
    // Accessors to the resource Manager
    FeResourceManager& GetFeResourceManager();
    Scrooby::ResourceManager& GetResourceManager();

    // Accessor to the text bible
    //
    virtual Scrooby::TextBible* GetTextBible( const char* name ) const;

    // Global state functions
    virtual bool IsSafeToRender();    
    void MarkSafeToRender();
    void MarkUnsafeToRender();
    bool IsDrawingFrame() { return( m_DrawingFrame ); };    
    
    //
    void OnProjectLoadComplete( Scrooby::Project* pProject );
    
    // Register cement file for the resource manager to use when opening files
    void RegisterCementFile( const char* filename );    
    
    //changes the localization language
    virtual void SetLocalizationLanguage( const Scrooby::XLLanguage lang );

    // Retrieve the screen resolution
    float GetScreenHeight();
    float GetScreenWidth();    
    
    //adds a project to the app
    void AddProject( Scrooby::Project* project );
    //removes a project from the App
    void RemoveProject( Scrooby::Project* project );

    float GetDeltaTime();

#ifdef RAD_DEBUG
    bool IsShowingBoundingBoxes() { return m_ShowBoundingBoxes; };
    void SetShowBoundingBoxes( bool show ) { m_ShowBoundingBoxes = show; };
#else
    bool IsShowingBoundingBoxes() { return false; };
    void SetShowBoundingBoxes( bool show ) {};
#endif

    void SetFullImageNames( bool full ) { m_FullImageNames = full; }
    bool UseFullImageNames() { return m_FullImageNames; }
    Scrooby::LoadProjectCallback* GetCallback() { return m_Callback; }
    void SetCallback( Scrooby::LoadProjectCallback* callback ) { m_Callback = callback; }

    P3D_UNICODE* GetTextBuffer() { return m_textBuffer; }
    void SetTextBuffer( P3D_UNICODE* buffer ) { m_textBuffer = buffer; }


protected:
   
    inline FeProject* GetProjectFast( unsigned int x ) 
    {
        if( static_cast<int>(x) >= m_Projects.Size() )
        {
            return NULL;
        }
        return m_Projects[x];
    }
    inline FeProject* GetProjectFast() { return GetProjectFast( m_projectIndex ); }
    
    // This is a singleton so no public constructor or destructor.
    FeApp();
    virtual ~FeApp();    
    
    
    // Resource manager object
    FeResourceManager* m_ResourceManager;
    
    static FeApp* s_Instance;    
    Scrooby::LoadProjectCallback* m_Callback;
    

#ifdef RAD_DEBUG
    bool m_ShowBoundingBoxes;
#endif

    // App states
    bool m_ProjectLoadPending;
    bool m_DrawingFrame;    
    bool m_SafeToRender;
    bool m_FullImageNames;
    
    unsigned int m_projectIndex;

    float m_deltaTime;

    rVector<FeProject*> m_Projects;

    P3D_UNICODE* m_textBuffer;

};

#endif
