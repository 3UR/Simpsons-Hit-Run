//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   FePure3dObject
//
// Description: 
//
// Authors:     Darwin Chau
//
// Revisions        Date            Author        Revision
//                  2000/08/03        DChau        Created
//
//===========================================================================

#ifndef FEPURE3DOBJECT_H
#define FEPURE3DOBJECT_H

//===========================================================================
// Nested Includes
//===========================================================================
#include "Pure3dObject.h"
#include "FeBoundedDrawable.h"
#include "FeParent.h"

//===========================================================================
// Forward References
//===========================================================================
class tView;
class tMultiController;
class tLight;
class PascalCString;

const unsigned int MAX_NUM_LIGHTS = 8;

//===========================================================================
// Interface Definitions
//===========================================================================
class FePure3dObject : 
                public FeBoundedDrawable,
                public FeParent,
                virtual public Scrooby::Pure3dObject
{
    public:

        FePure3dObject( const tName& name, int x, int y );

        void Add3dModel( unsigned int imageResourceID );
        void Add3dModel( const char* alias );

        tView* GetView() const { return m_View; }
        tCamera* GetCamera();
        tMultiController* GetMultiController();

        virtual void SetCamera( tCamera* camera );
        virtual void SetMultiController( tMultiController* controller );
        virtual void SetDefaultLight( tLight* light );
        void SetupCamera();
        void SetupMultiController();

        virtual void SetCameraSettings( float nearPlane, float farPlane )
        {
            m_CameraNearPlane = nearPlane;
            m_CameraFarPlane = farPlane;
        }

        virtual void Update( float elapsedTime );
        virtual void Display();
        virtual void Reset();
        
        //
        // Implementation of Scrooby interface.
        //
        virtual void AdvanceAnimation( float deltaTime );
        rmt::Matrix* GetMatrix();
        const rmt::Matrix* GetMatrix() const;
        void Render();
        virtual void ResetTransformation(); 
        void SetTranslation( int x, int y );
        void Translate( int x, int y );
        void SetDrawable( tDrawable* drawable );
        tDrawable* GetDrawable();

        void SetDrawableTranslation( float x, float y, float z )
        {
            m_drawableTranslateX = x;
            m_drawableTranslateY = y;
            m_drawableTranslateZ = z;
        }

        void SetDrawableScale( float scale ) { m_drawableScale = scale; }

        void SetColourWrite( bool enable ) { m_colourWriteEnabled = enable; }
        void SetClearDepthBuffer( bool enable ) { m_clearDepthBufferEnabled = enable; }
        void SetZBufferEnabled( bool enable ) { m_zbufferEnabled = enable; }

        virtual void SetWideScreenCorrectionEnabled( bool isEnabled ) { m_isWideScreenCorrectionEnabled = isEnabled; }

    protected:
         virtual ~FePure3dObject();

    private:
        //---------------------------------------------------------------------
        // Private Functions
        //---------------------------------------------------------------------

        // No copying or assignment. Declare but don't define.
        //
        FePure3dObject( const FePure3dObject& );
        FePure3dObject& operator= ( const FePure3dObject& );

        void RecalcView();

        //---------------------------------------------------------------------
        // Private Data
        //---------------------------------------------------------------------
        float mAlpha;
        tColour mColour;
        
        float mViewTranslateX;
        float mViewTranslateY;

        tView*              m_View;     // this is the view in which lights are added
        tCamera*            m_Camera;
        tMultiController*   m_MultiController;
        tDrawable*          m_RuntimeDrawable;
        tLight*             m_DefaultLight;

        unsigned int        m_lightsAdded[ MAX_NUM_LIGHTS ];
        unsigned int        m_numLightsAdded;

        PascalCString*      m_alias;

        float m_CameraNearPlane;
        float m_CameraFarPlane;

        float m_drawableTranslateX;
        float m_drawableTranslateY;
        float m_drawableTranslateZ;

        float m_drawableScale;

        bool m_alreadyRendered : 1;
        bool m_colourWriteEnabled : 1;
        bool m_clearDepthBufferEnabled : 1;
        bool m_zbufferEnabled : 1;

        bool m_isWideScreenCorrectionEnabled : 1;

};

#endif
