//===========================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Component
//
// Description: FePage.h - this class controls the individual pages
//
// Authors:     Ian Gipson
//
//===========================================================================
#ifndef FEPAGE_H
#define FEPAGE_H

//===========================================================================
// Nested Includes
//===========================================================================
#include "Page.h"
#include "FeOwner.h"
#include "FeProject.h"          // so FindPageResource work properly

//===========================================================================
// Forward References
//===========================================================================
class FeLayer;
class FePageResourceHolder;
class FeText;
class FeSprite;
class FeProject;
class tEntityTable;

#if defined( RAD_WIN32 ) || defined( RAD_UWP )
#pragma warning( disable : 4250 )
#endif

//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
// FePage
//
//===========================================================================
class FePage : 
    public FeOwner,
    virtual public Scrooby::Page
{
public: 
    //constructor
    FePage( const char* fullFileName, FeProject* project );

    //adds a layer to the page
    FeLayer* AddLayer( const char* name );

    //gets the width and the height of the page
    virtual void GetBoundingBoxSize( int& width, int& height ) const;

    //gets the bounding box of the page
    virtual void GetBoundingBox( int& xMin, int& yMin, int& xMax, int& yMax ) const;

    //returns the original filename of this page
    const char* GetFileName();

    //gets a group object from within the page by hash
    virtual Scrooby::Group* GetGroup( const tUID hashValue );

    //gets a text object from within the page by name
    virtual Scrooby::Group* GetGroup( const char* name );

    //gets a layer object from within the page by hash
    virtual Scrooby::Layer* GetLayer( const tUID hashValue );

    //gets a text object from within the page by name
    virtual Scrooby::Layer* GetLayer( const char* name );

    //gets a text object from within the page by name
    virtual Scrooby::Layer* GetLayerByIndex( const int index );

       //gets a text object from within the page by name
    virtual int GetNumberOfLayers() const;

    //gets a polygon object from within the page by hash
    virtual Scrooby::Polygon* GetPolygon( const tUID hashValue );

    //gets a polygon object from within the page by name
    virtual Scrooby::Polygon* GetPolygon( const char* name );

    //gets a movie object from within the page by hash
    virtual Scrooby::Movie* GetMovie( const tUID hashValue );

    //gets a movie object from within the page by name
    virtual Scrooby::Movie* GetMovie( const char* name );

    //gets a pure3d object from within the page by hash
    virtual Scrooby::Pure3dObject* GetPure3dObject( const tUID hashValue );

    //gets a pure3d object from within the page by name
    virtual Scrooby::Pure3dObject* GetPure3dObject( const char* name );

    //determines whether or not this object is a page
    virtual bool IsPage() const;

    //gets the parent project for this page
    FeProject* Project();

    //sets the alpha value
    void SetAlpha( float a );

    void AddResource( int resourceID ) { mResources.Insert( resourceID ); };


private:    
    //destructor
    virtual ~FePage();
    
    // No copying. Declare but don't define.
    //
    FePage( const FePage& );

    // No assignment. Declare but don't define.
    //
    FePage& operator= ( const FePage& );

    
    void Display();

    FeProject* mProject;
    PascalCString mFileName;

    rVector<int> mResources;

};

#endif
