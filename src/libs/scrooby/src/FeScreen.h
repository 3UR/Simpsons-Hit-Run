//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This defines the FeScreen interface
//
// Authors:     Ian Gipson
//              Darwin Chau
//              Wilkin Ng
//
//===========================================================================

// Recompilation protection flag.
#ifndef __FESCREEN_H
#define __FESCREEN_H

#include "FeDrawable.h"
#include "FeEntity.h"
#include "FeOwner.h"
#include "Screen.h"
#include "strings/pcstring.h"

//===========================================================================
// Forward References
//===========================================================================
class FePage;
class FeProject;

namespace Scrooby
{
    class Page;
} 

// TC: Screen scaling hack to accomodate for different frame buffer sizes on
//     different platforms.
//
#define SCREEN_SCALING_HACK

#ifdef SCREEN_SCALING_HACK
    extern bool g_screenScalingEnabled;

    extern float g_screenScaleX;
    extern float g_screenScaleY;
    extern float g_screenScaleZ;
#endif

//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: this class represents a screen functional object
//
// Constraints: NONE
//
//===========================================================================
class FeScreen
: 
    public FeOwner,
    virtual public Scrooby::Screen
{
public:
    FeScreen( const char* name, FeProject* project);

	void AddChild( FeEntity* s );
    FePage* AddPage( const char* name);

    //overload the display function
    virtual void Display();
    void DisplayForeground();
    void DisplayBackground();

    //gets the bounding box for this screen
    virtual void GetBoundingBox( int& xMin, int& yMin, int& xMax, int& yMax ) const;

    //gets the size of the bounding box for this screen
    virtual void GetBoundingBoxSize( int& width, int& height ) const;

    //gets the filename for the screen //IAN IMPROVE: is this ever called - and should it be public?
    const char* GetFileName();

    //get a page by name
    virtual Scrooby::Page* GetPage( const char* name );

    //get a page by hashValue
    virtual Scrooby::Page* GetPage( const tUID hashValue );

    //get a page by index
    virtual Scrooby::Page* GetPageByIndex( const int index );

    //get the number of pages used by this screen
    virtual int GetNumberOfPages() const;

    //screen scaling (for zooming in/out effects)
    virtual void SetScale( float screenScale ) { m_screenScale = screenScale; }
    virtual float GetScale() const { return m_screenScale; }

    //gets the parent project
    FeProject* GetProject();
    void Reload() {};

private:
    // No copying. Declare but don't define.
    FeScreen( const FeScreen& );

    // No assignment. Declare but don't define.
    FeScreen& operator=( const FeScreen& );

    virtual ~FeScreen();

    FeProject* mProject;
    PascalCString mFileName;
    PascalCString mDivide;

    float m_screenScale;

    friend class FeProject;

};

#endif
