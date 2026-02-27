//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Component
//
// Description: this object coordinates the loading of all resources
//
// Authors:     Ian Gipson
//
// Revisions        Date            Author    Revision
//                    2000/07/10        IJG        Created
//
//===========================================================================

#ifndef _FERESOURCEMANAGER_H_
#define _FERESOURCEMANAGER_H_

//===========================================================================
// Nested Includes
//===========================================================================
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
#pragma warning ( disable : 4530 )
#endif

#include "strings/pcstring.h"
#include "ResourceManager.h"
#include "utility/rVector.hpp"
#include "Enums.h"
#include <p3d/loadmanager.hpp>
#include <p3d/imagefactory.hpp>
#include <string.h>
#include <feentity.h>
//===========================================================================
// Defines
//===========================================================================

//===========================================================================
// Forward References
//===========================================================================

class FeProject;
class tCamera;
class tMultiController;

namespace Scrooby
{
    class LoadProjectCallback;
    class Project;
}

enum ResourceManagerLoadMode
{
    RM_LOAD_ON_DEMAND,
    RM_LOAD_ON_PAGELOAD,
    RM_LOAD_ON_STARTUP
};

enum ResourceManagerUnloadMode
{
    RM_UNLOAD_NEVER,
    RM_UNLOAD_WHEN_NOT_USED
};

enum ResourceManagerStatus
{
    RM_FILENAME_UNKNOWN,    //the filename has not yet been passed to the resourceManager
    RM_NOT_LOADED,            //nobody has yet asked for a load
    RM_FTT_LOAD_IN_PROGRESS,    //the load is still in progress
    RM_P3D_LOAD_IN_PROGRESS,    //the load is still in progress
    RM_LOAD_COMPLETE,        //the load is complete, and the data is ready to use
    RM_REMOVED,             //a deleted item that can be used by another load attempt now
    RM_END                    //end of the list
};

enum ResourceType
{
    RT_PROJECT,
    RT_SCREEN,
    RT_PAGE,
    RT_IMAGE,
    RT_TEXT_BIBLE,
    RT_TEXT_STYLE,
    RT_P3D_OBJECT,
    RT_PROJECT_LOAD_CALLBACK
};

//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
// ResourceManager
//
//    manages loading and unloading of all the resources
//
//===========================================================================
class FeResourceManager  :
    public FeEntity,
    virtual public Scrooby::ResourceManager
{
public:
    //constructor
    FeResourceManager();

    //general loading/unloading
    virtual void LoadAll( Scrooby::ResourceManagerCallback* callback );
    void UnloadAll();
    //project based loading/unloading
    virtual void LoadProject(  Scrooby::Project* project, Scrooby::ResourceManagerCallback* callback );
    virtual void UnloadProject(  Scrooby::Project* project );
    void ProjectLoadComplete();
    //accessors for different loading parameters
    bool IsAsyncLoading();
    void SetAsyncLoading( bool async );
    //main loading loop
    void ContinueLoading();

    //callback Access functions
    Scrooby::ResourceManagerCallback* GetCallback() const;
    void SetCallback( Scrooby::ResourceManagerCallback* callback );
    void RegisterProjectCallback( Scrooby::LoadProjectCallback* callback );

    //localization access functions
    void SetLocalizationLanguage( const Scrooby::XLLanguage lang );
    Scrooby::XLLanguage GetLocalizationLanguage() const;

    //registers a cement file for resource loading
    void RegisterCementFile( const char* filename );

    //stores the full path to the project's resource subdirectory
    //void SetResourcePath( const char* path );

    //add items into the resource manager
    int AddResource( const char* filename, FeEntity* parent, ResourceType type, const char* inventoryName = NULL );
    int AddAlias( const tName& alias, int index );
    int AddPure3dAttributes( const char* camera, const char* animation, int index );
    //retrieve from the resource manager
    int GetIndex( const tName& alias, bool searchBackwards = false );
    tEntity* GetResource( const char* alias );
    tEntity* GetResource( int index );
    tCamera* GetCamera( const char* alias );
    tCamera* GetCamera( int index );
    tMultiController* GetMultiController( const char* alias );
    tMultiController* GetMultiController( int index );
    void RemoveResource( tEntity* resource, const char* inventorySection = NULL );
    void RemoveResource( int resourceID, const char* inventorySection = NULL );
    void RemoveAlias( int resourceID );
    void RemoveP3DAttributes( int resourceID );

    void SetInventorySection( const char* name );
    const char* GetInventorySection() {return m_pInventorySection;}
    void SetAllocator (radMemoryAllocator allocator) { m_Allocator = allocator; }
    radMemoryAllocator GetAllocator () const { return m_Allocator; }

    void SetSecondaryInventorySection( const char* name );

    FeProject* GetLoadingProject() {return m_pLoadingProject;}

    //debugging methods - Requires the LVL_DEBUG message level to use
    void DumpResources();
    void DumpAliases();
    void DumpPure3dAttributes();

    void SetCallbackResourceID( unsigned int id ) { mCallbackID = id; }

    virtual bool IsDrawable() { return false; }
    class ResourceManagerEntryBase;

    //===========================================================================
    // nested classes/structs
    //===========================================================================
    class ResourceManagerEntryBase : public FeEntity
    {
    public:
        bool isThisCurrentlyBeingLoaded;
        ResourceManagerStatus m_Status;
        char* m_ScroobyName;

        ResourceManagerEntryBase(); 

        virtual ~ResourceManagerEntryBase()
        {
            delete m_ScroobyName;
        }

        void SetName(const char *n)
        { 
            delete m_ScroobyName;
            m_ScroobyName = NULL;
            radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
            m_ScroobyName = new char[strlen( n ) + 1];
            ::radMemorySetCurrentAllocator( old );
            strcpy( m_ScroobyName, n );
        }
        
        const char* GetName(void) const 
        { 
            return m_ScroobyName;
        }

        ResourceManagerEntryBase& operator= ( const ResourceManagerEntryBase& rhs )
        {
            if( this == &rhs )
            {
                return( *this );
            }
            SetName( rhs.GetName() );
            isThisCurrentlyBeingLoaded = rhs.isThisCurrentlyBeingLoaded;
            m_Status = rhs.m_Status;
            
            return( *this );
        }

        ResourceManagerEntryBase( const ResourceManagerEntryBase& rhs ) { *this = rhs; };

        virtual bool IsDrawable() { return false; }

    };
protected:

    //===========================================================================
    // member functions
    //===========================================================================

    //destructor
    virtual ~FeResourceManager();

    class ResourceEntry : public ResourceManagerEntryBase
    {
    public:
        FeEntity* m_parent;
        ResourceType m_type;
        PascalCString m_inventoryName;
        tEntity* theData;

        // Constructor.
        ResourceEntry():
            ResourceManagerEntryBase(),
            m_parent( NULL ),
            theData( NULL )
        {
        };

        // Destructor.
        virtual ~ResourceEntry()
        {
            if( theData )
            {
                theData->Release();                
            }
        };

        // Assignment operator.
        ResourceEntry& operator= ( const ResourceEntry& rhs )
        {
            if( this == &rhs )
            {
                return( *this );
            }

            ResourceManagerEntryBase::operator=( rhs );

            // Shallow copy.
            m_parent = rhs.m_parent;
            m_type = rhs.m_type;
            m_inventoryName = rhs.m_inventoryName;
            theData = rhs.theData;
            return( *this );
        }
        
        // Copy constructor.
        ResourceEntry( const ResourceEntry& rhs ):ResourceManagerEntryBase( rhs ) { *this = rhs; };
    };

    class AliasEntry
    {
    public:
        int m_resourceIndex;

        AliasEntry(){};
        virtual ~AliasEntry(){};
        AliasEntry( const AliasEntry& rhs ){m_resourceIndex = rhs.m_resourceIndex;};
        AliasEntry& operator= ( const AliasEntry& rhs )
        {
            if( this == &rhs )
            {
                return( *this );
            }

            // Shallow copy.
            m_resourceIndex = rhs.m_resourceIndex;
            return( *this );
        }
        tName& GetName()
        {
            return mName;
        }
        void SetName( const tName& name )
        {
            mName = name;
        }
        void SetName( const char* name )
        {
            mName = name;
        }
    protected:
        tName mName;
    };

      struct Pure3dAttributesEntry : public ResourceManagerEntryBase
    {
    public:
        int m_resourceIndex;
        PascalCString m_CameraName;
        PascalCString m_AnimName;

        // Constructor.
        Pure3dAttributesEntry():ResourceManagerEntryBase(){};
        // Destructor.
        virtual ~Pure3dAttributesEntry(){};

        // Assignment operator.
        Pure3dAttributesEntry& operator= ( const Pure3dAttributesEntry& rhs )
        {
            if( this == &rhs )
            {
                return( *this );
            }

            ResourceManagerEntryBase::operator=( rhs );

            // Shallow copy.
            m_resourceIndex = rhs.m_resourceIndex;
            m_CameraName = rhs.m_CameraName;
            m_AnimName = rhs.m_AnimName;
            return( *this );
        }

        // Copy constructor.
        Pure3dAttributesEntry( const Pure3dAttributesEntry& rhs ):ResourceManagerEntryBase( rhs ){ *this = rhs; };
    };

    class P3DCallback : public tLoadRequest::Callback
    {
    public:

        void AddRef() { radLoadObject::AddRef(); }
        void Release() { radLoadObject::Release(); }
        void Done( tLoadStatus status, tLoadRequest *load )
        {
            bool isDummyLoad = load != NULL ? load->IsDummy() : false;

            // continue loading, or are we all done??
            //
            rAssert( m_pResourceManager );
            if( isDummyLoad )
            {
                m_pResourceManager->GetCallback()->OnResourceLoadComplete();
                m_pResourceManager->ProjectLoadComplete();
            }
            else
            {
                if( m_pResourceManager->IsAsyncLoading() )
                {
                    m_pResourceManager->ContinueLoading(); 
                }
            }
        }
        void SetResourceManager( FeResourceManager* resourcemanager ){ m_pResourceManager = resourcemanager; }
        FeResourceManager* GetResourceManager(){ return m_pResourceManager; }
    protected:
        FeResourceManager* m_pResourceManager;
    };

    //===========================================================================
    // data members
    //===========================================================================

    //resource lists
    rVector< ResourceEntry* > mResources;
    //rVector< AliasEntry* > mAliases;
    rVector< AliasEntry* > mAliases;
    rVector< Pure3dAttributesEntry* > mP3DAttributes;
    int mDeletedResources;
    int mDeletedAliases;
    int mDeletedAttributes;

    //callbacks
    Scrooby::LoadProjectCallback* m_pLoadProjectCallback;
    Scrooby::ResourceManagerCallback* m_pResourceManagerCallback;
    P3DCallback *m_pP3DCallback;
    
    //loading parameters
    bool m_bAsyncLoading;
    FeProject* m_pLoadingProject;
    char m_pInventorySection[ 64 ];
    char m_pSecondaryInventorySection[ 64 ];
    radMemoryAllocator m_Allocator;

    char* m_pResourcePath;
    Scrooby::XLLanguage m_eLang;

    tImageHandler* m_imageHandler;
    tImageHandler::LoadType m_loadType;
    
private:
    int mCallbackID;
};
#endif
