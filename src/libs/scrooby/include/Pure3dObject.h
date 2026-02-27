//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This contains the Scrooby::Pure3dObject class.
//
// Authors:     Darwin Chau
//
//===========================================================================

// Recompilation protection flag.
#ifndef SCROOBY_PURE3DOBJECT_H
#define SCROOBY_PURE3DOBJECT_H

//===========================================================================
// Nested Includes
//===========================================================================
#include "BoundedDrawable.h"
#include <p3d/drawable.hpp>

//===========================================================================
// Forward References
//===========================================================================

class tView;
class tCamera;
class tMultiController;
class tLight;

namespace Scrooby
{
//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: This class is used to represent a 3d drawing element.
//
// Constraints: None
//
//===========================================================================

class Pure3dObject 
: 
    virtual public BoundedDrawable
{
    public:
        
        //advances the animation by this many frames
        virtual void AdvanceAnimation( float deltaTime ) = 0;

        //gets a pointer to the view in which the P3D object is rendered
        virtual tView* GetView() const = 0;

        //gets a pointer to the camera contained in the P3D object
        virtual tCamera* GetCamera() = 0;

        //gets a pointer to the underlying P3D object that's being rendered
        virtual tDrawable* GetDrawable() = 0;

        //sets the pure3d file
        virtual void Add3dModel( const char* alias ) = 0;

        //sets the camera for the P3D object to use when rendering
        virtual void SetCamera( tCamera* camera ) = 0;

        //sets the camera near and far planes
        virtual void SetCameraSettings( float nearPlane, float farPlane ) = 0;

        //overrides the underlying P3D object being rendered
        virtual void SetDrawable( tDrawable* drawable ) = 0;

        //adds a default light to P3D object's view
        virtual void SetDefaultLight( tLight* light ) = 0;

        //set multicontroller for P3D object
        virtual void SetMultiController( tMultiController* controller ) = 0;

        //get multicontroller from P3D object
        virtual tMultiController* GetMultiController() = 0;

        virtual void SetDrawableTranslation( float x, float y, float z ) = 0;
        virtual void SetDrawableScale( float scale ) = 0;

        virtual void SetColourWrite( bool enable ) = 0;
        virtual void SetClearDepthBuffer( bool enable ) = 0;
        virtual void SetZBufferEnabled( bool enable ) = 0;

        virtual void SetWideScreenCorrectionEnabled( bool isEnabled ) = 0;

    protected:
    private:
};


} // End namespace

#endif // End conditional inclusion
