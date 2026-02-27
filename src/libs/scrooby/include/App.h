//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This defines the Scrooby::App interface.
//
// Authors:     Ian Gipson
//              Darwin Chau
//
//===========================================================================

// Recompilation protection flag.
#ifndef SCROOBY_APP_H
#define SCROOBY_APP_H

#include <p3d/p3dtypes.hpp>
#include <radmemory.hpp>
#include "Enums.h"

namespace Scrooby {

//===========================================================================
// Forward References
//===========================================================================
class Project;
class LoadProjectCallback;
class ResourceManager;
class ResourceManagerCallback;
class TextBible;

//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: This is the client's starting point for accessing Scrooby.
//              Use this to load a project.
//
// Constraints: Note this class is a singleton.  The client is responsible
//              for calling DeleteInstance() to shutdown when it is finished
//              using Scrooby.
//
//===========================================================================
class App
{
public:
    
    //===================================================================
    // Description: App is a singleton so you can *not* instantiate it
    //              yourself. There is only one instance and this method
    //              is how you get access.
    //===================================================================
    static App* GetInstance();
    
    //===================================================================
    // Description: When you are finished using Scrooby, invoke this
    //              method to shutdown and release the resources.
    //===================================================================
    static void DeleteInstance();
    
    //===================================================================
    // Description: Loads a project from a specific path, and stores it.
    //              This is an ASYNCHRONOUS load.
    //
    // Constraints:    * Path must be valid, otherwise the RFC drive system
    //                will trigger an error.
    //
    //              * Currently, only one project can be loaded at a time,
    //                each subsequent call will unload current project
    //                before loading the new one.
    //
    // Parameters:    path - The path and name of the project file
    //                     e.g. "..\\Project\\Scrooby\\Test.fePrj"
    //
    //              pCallback - This callback will be invoked once the 
    //                          load is complete
    //===================================================================
    virtual void LoadProject( const char* path,
                              LoadProjectCallback* pCallback,
                              char* inventorySection = NULL,
                              radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT ) = 0;
    
    //===================================================================
    // Description: Renders a frame to the display.
    // 
    // Constraints: Do *NOT* call this before a project is loaded.
    //===================================================================
    virtual void DrawFrame( float deltaTime = 16.0f ) = 0;
    
    //===================================================================
    // Description: Renders all the pages on a screen up to and
    //              including the page named "FeDivider".
    // 
    // Constraints: Do *NOT* call this before a project is loaded.
    //===================================================================
    virtual void DrawBackground( float deltaTime = 16.0f ) = 0;
    
    //===================================================================
    // Description: Renders all the pages on a screen that come
    //              after "FeDivider".
    // 
    // Constraints: Do *NOT* call this before a project is loaded.
    //===================================================================
    virtual void DrawForeground( float deltaTime = 16.0f ) = 0;
    
    //===================================================================
    // Description: Renders a portion to the display.
    // 
    // Constraints: Do *NOT* call this before a project is loaded.
    //===================================================================
    virtual void Display( const char* startLayer, const char* endLayer ) = 0;
    
    //===================================================================
    // Description: Renders a portion to the display.
    // 
    // Constraints: Do *NOT* call this before a project is loaded.
    //===================================================================
    virtual void Display( tUID startLayer, tUID endLayer ) = 0;
    
    //===================================================================
    // Description: returns a pointer to a project that has been loaded.
    // 
    // Constraints: Do *NOT* call this before a project is loaded.
    //===================================================================
    virtual Project* GetProject() = 0;
    virtual Project* GetProject( unsigned int x ) = 0;
    
    virtual unsigned int GetProjectIndex() = 0;
    
    //===================================================================
    // Description: Accessor to the resource manager object
    // 
    // Constraints: Do *NOT* call this before a project is loaded.
    // 
    // Notes: IAN: I intend to make this public at some point
    //===================================================================
    virtual ResourceManager& GetResourceManager() = 0;

    // Accessor to the text bible
    //
    virtual TextBible* GetTextBible( const char* name ) const = 0;

    //===================================================================
    // Description: returns true if it's safe to render a screen - 
    //              ie resources have been loaded
    // 
    // Constraints: Do *NOT* call this before a project is loaded.
    //===================================================================
    virtual bool IsSafeToRender() = 0;
    
    //===================================================================
    // Description: this needs to be called by the GotoScreenCallback 
    //              function
    // 
    // Constraints: Do *NOT* call this before a project is loaded.
    //===================================================================
    virtual void MarkSafeToRender() = 0;
    
    
    //===================================================================
    // Description: registers a cement file to be checked before loading
    //              resources from disk.  This is for resources only
    // 
    // Constraints: NONE
    //===================================================================
    virtual void RegisterCementFile( const char* filename ) = 0;
    
    
    //===================================================================
    // Description: changes the localization language
    // 
    // Constraints: Do *NOT* call this before a project is loaded.
    //===================================================================
    virtual void SetLocalizationLanguage( const XLLanguage lang ) = 0;
    
    //===================================================================
    // Description: changes all the slashes in filenames to backslashes
    // 
    // Constraints: 
    //===================================================================
    static void SetBackslashes( bool backslashesOn ) ;
    
    //===================================================================
    // Description: sets the current project
    // 
    // Constraints: 
    //===================================================================
    virtual void SetProjectIndex( unsigned int x ) = 0;
    virtual void SetProject( Project* project ) = 0;

    //===================================================================
    // Description: unloads a project that has already been loaded.
    // 
    // Constraints: Do *NOT* call this before a project is loaded.
    //===================================================================
    virtual void UnloadProject( Project* pProject ) = 0;

    virtual void SetFullImageNames( bool full ) = 0;
    virtual bool UseFullImageNames() = 0;

    virtual void EnableScreenScaling( bool enable, float scaleX = 0.0f,
                                                   float scaleY = 0.0f,
                                                   float scaleZ = 0.0f );

    // Retrieve the screen resolution
    virtual float GetScreenHeight() = 0;
    virtual float GetScreenWidth() = 0;

    protected:
        App();
        virtual ~App();    
        
    private:
        static App* sInstance;
};


//===========================================================================
//
// Description: This class encapsulates the callback function that gets
//              invoked when the asynchronous call to:
//              Scrooby::App->LoadProject() finally completes.
//
//              The client must derive a concrete class from this abstract
//              base class and implement OnProjectLoadComplete().
//
// Constraints: None
//
//===========================================================================
class LoadProjectCallback
{
public:
    
    //===================================================================
    // Description: This member is invoked when the project has completed
    //              loading from disk.  The client can then use the
    //              pProject pointer to access it.  The client is
    //              responsible for implementing this member function.
    //===================================================================
    virtual void OnProjectLoadComplete( Project* pProject ) = 0;
};


} // End namespace

#endif // End conditional inclusion

