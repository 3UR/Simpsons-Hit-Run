//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This contains the Drawable Base class.
//
// Authors:     Wilkin Ng, Ian Gipson
//
//===========================================================================


// Recompilation protection flag.
#ifndef FEDRAWABLE_H
#define FEDRAWABLE_H

//===========================================================================
// Nested Includes
//===========================================================================
#include "Drawable.h"
#include "FeEntity.h"
#include <raddebugwatch.hpp>

//===========================================================================
// Forward References
//===========================================================================
class tDrawable;
class FeOwner;

//===========================================================================
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//===========================================================================

//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: This class is used to represent any drawable front end element
//
// Constraints: None
//
//===========================================================================
class FeDrawable
: 
    public FeEntity,
    virtual public Scrooby::Drawable
    
{
public:

    FeDrawable( const tName& name );

    virtual void Update( float elapsedTime );
    virtual void Display();

    //allows access to the alpha value
    virtual float GetAlpha() const;
    virtual void SetAlpha( float a );

    //allows access to the color values
    virtual const tColour& GetColour() const;
    virtual void SetColour( tColour c );

    //allows access to the original position of the object
    virtual void GetOriginPosition( int& x, int& y) const;

    virtual void GetBoundingBoxSize( int& width, int& height ) const = 0;
    virtual void GetBoundingBox( int& xMin, int& yMin, int& xMax, int& yMax ) const = 0;

    virtual rmt::Matrix* GetTransform();
    virtual const rmt::Matrix* GetTransform() const;
    virtual void SetTransform( rmt::Matrix* matrix );
    virtual void SetTransform( const rmt::Matrix& matrix );

    bool IsVisible() const;

    static void ModulateColour( tColour& colour, const tColour& modulator );

    virtual void ResetTransformation();

    virtual void RotateArbitrary( float axisX, float axisY, float axisZ, float angle ) ;
    virtual void RotateX( float angleX );
    virtual void RotateY( float angleY );
    virtual void RotateZ( float angleZ );

    virtual void Scale( float factor );
    virtual void Scale( float factorX, float factorY, float factorZ );

    virtual void SetTranslation( int x, int y );
    virtual void Translate( int x, int y );

    virtual void SetVisible( bool v );


    void Transform( rmt::Matrix* matrix );
    void TranslateRealWorld( float x, float y );
    void SetTranslationRealWorld( float x, float y );
    void SetPosition(int x, int y);

    //this alters the origin of the FeDrawable on the screen
    virtual void TranslatePosition( int x, int y );
    void GetNormalizedPosition(float &x, float &y) const;
    virtual rmt::Matrix* GetMatrix();
    virtual const rmt::Matrix* GetMatrix() const;

    void LinkPrimitive( unsigned handle );
    
    virtual void Reset();
    virtual void Show();    // called on screen update    
    
    FeDrawable* Parent();
    void SetParent( FeDrawable* p );

    float GetLayer();
    
    bool IsClip();
    pddiRect* GetClipRect();
    virtual bool IsPage() const;          //use instead of dynamic_cast to Page*
    virtual bool IsDrawable() { return true; };

    #ifdef DEBUGWATCH
        virtual void WatchAll       ( const char* nameSpace );
        virtual void WatchColor     ( const char* nameSpace );
        virtual void WatchPosition  ( const char* nameSpace );
        virtual void WatchVisibility( const char* nameSpace );
    #endif

protected:

    virtual ~FeDrawable();
    float DegreesToRadian( float degree );
    void NormalizeCoord( int ix, int iy, float& x, float& y ) const;
    void InverseNormalizeCoord( float x, float y, int& ix, int& iy ) const;
    void SetAlphaNoRedraw( float a );
    void SetLayer( float layer );
    void SetClip( float l, float b, float r, float t );

    FeDrawable* mParent;
    bool        mVisible;

    rmt::Matrix m_matrix;
    bool        m_clip;
    mutable     pddiRect    m_rect;
    float       m_layer;  // visibility

    tColour mColour;      //color and alpha settings
    float mAlpha;

    float        mPosX;    // position of object in the screen
    float        mPosY;  

    #ifdef DEBUGWATCH
        int          mPosXInt;      //used for output via watcher only
        int          mPosYInt;      //used for output via watcher only
    #endif // RAD_DEBUG_WATCH
};

#endif
