//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This contains the Srooby::ResourceManager class.
//
// Authors:     Ian Gipson
//
//===========================================================================

// Recompilation protection flag.
#ifndef SCROOBY_RESOURCE_MANAGER_H
#define SCROOBY_RESOURCE_MANAGER_H

//===========================================================================
// Nested Includes
//===========================================================================
namespace Scrooby
{
//===========================================================================
// Early Definitions
//===========================================================================
class Project;
class ResourceManagerCallback;
//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: This class is used to represent a Scrooby Text element
//
// Constraints: None
//
//===========================================================================
class ResourceManager
{
public:
    //triggers loading of all the resources
    virtual void LoadAll( ResourceManagerCallback* callback ) = 0;

    //triggers loading of all the resources associated with a project
    virtual void LoadProject(  Scrooby::Project* project, ResourceManagerCallback* callback ) = 0;

    //triggers unloading of all the resources associated with a project
    virtual void UnloadProject(  Scrooby::Project* project ) = 0;

    virtual void SetSecondaryInventorySection( const char* name ) = 0;

    // TC: This is a dirty hack for removing a particular Scrooby resource!
    //     I promise I won't do it again!!!
    //
    virtual void RemoveResource( tEntity* resource, const char* inventorySection = NULL ) = 0;

protected:
private:
};


//===========================================================================
// ResourceManagerCallback
//
// callback functor for when all the loading is done
//===========================================================================
class ResourceManagerCallback
{
public:
    virtual ~ResourceManagerCallback(){};
    virtual void OnResourceLoadComplete() = 0;
};



} // End namespace

#endif // End conditional inclusion
