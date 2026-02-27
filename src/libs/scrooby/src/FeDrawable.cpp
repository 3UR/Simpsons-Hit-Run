// FeDrawable.cpp
// Created by wng on Wed, May 03, 2000 @ 12:55 PM.

#ifndef __FeDrawable__
#include "FeDrawable.h"
#endif

#include <p3d/utility.hpp>
#include <raddebug.hpp>
#include <raddebugwatch.hpp>
#include "FeApp.h"
#include "FeOwner.h"
#include "FeScreen.h"
#include "utility/debugmessages.h"

FeDrawable::FeDrawable( const tName& name )       
:
    mParent( NULL ),
    mColour( 255, 255, 255, 255 ),
    mAlpha( 1.0f ),
    mPosX( 0.0f ),
    mPosY( 0.0f )
{   
    FeEntity::SetNameObject( name );
    
    //
    // Always use explicit qualification when calling a virtual function from
    // a constructor.
    //
    FeDrawable::Reset();
}

FeDrawable::~FeDrawable()
{
}

float FeDrawable::DegreesToRadian( float degree )
{
    return degree / 360 * rmt::PI_2;
}

pddiRect* FeDrawable::GetClipRect()
{
    rAssert( this->m_clip == true );
    return &m_rect;
}

void FeDrawable::Update( float elapsedTime )
{
    #ifdef DEBUGWATCH
        //
        // Update output values for the watcher
        //
        InverseNormalizeCoord( mPosX, mPosY, mPosXInt, mPosYInt );
    #endif //DEBUGWATCH
}

void FeDrawable::Display()
{
    rAssert( false );
}

//===========================================================================
// GetAlpha()
//===========================================================================
// Description: get the alpha value for this object
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      the alpha value for this object
//
//===========================================================================
float FeDrawable::GetAlpha() const
{
    return this->mAlpha;
}

//===========================================================================
// GetColour()
//===========================================================================
// Description: get the colour value for this object
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      the colour value for this object
//
//===========================================================================
const tColour& FeDrawable::GetColour() const
{
    return mColour;
}

float FeDrawable::GetLayer()
{
    return m_layer;
}

//===========================================================================
// int GetMatrix() const
//===========================================================================
// Description: allows access to the transformation matrix
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      pointer to the matrix
//
//===========================================================================
rmt::Matrix* FeDrawable::GetMatrix()
{
    return &m_matrix;
}

//===========================================================================
// int GetMatrix() const
//===========================================================================
// Description: allows access to the transformation matrix
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      const pointer to the matrix
//
//===========================================================================
const rmt::Matrix* FeDrawable::GetMatrix() const
{
    return &m_matrix;
}
  
//===========================================================================
// int GetOriginPosition( int& x, int& y ) const
//===========================================================================
// Description: allows access to the original position of the object
//
// Constraints:    None
//
// Parameters:    x - the x value gets placed here
//              y - the y value gets placed here
//
// Return:      None
//
//===========================================================================
void FeDrawable::GetOriginPosition( int& x, int& y ) const
{
    InverseNormalizeCoord( mPosX, mPosY, x, y );
}

//===========================================================================
// void InverseNormalizeCoord( float x, float y, int& ix, int& iy ) const
//===========================================================================
// Description: allows conversion from P3D coords to pixel coords
//
// Constraints:    None
//
// Parameters:    x - the P3D x value 
//              y - the P3D y value 
//              ix - integer x value gets put here
//              iy - integer y value gets put here
//
// Return:      None
//
//===========================================================================
void FeDrawable::InverseNormalizeCoord( float x, float y, int& ix, int& iy ) const
{    
    float yRes = FeApp::GetInstance()->GetScreenHeight();
    float xRes = FeApp::GetInstance()->GetScreenWidth();
    
    float loc = (float )( y * yRes ) - 0.0f;
    iy = static_cast< int >( loc * xRes / yRes );       //fix for the aspect ratio
    ix = static_cast< int >( x * xRes - 0.0f );
}

bool FeDrawable::IsClip()
{
    return m_clip;
}

//===========================================================================
// IsPage
//===========================================================================
// Description: returns whether or not this object is a page - used instead
//              of dynamic cast
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      bool - true or false is the object was a page
//
//===========================================================================
bool FeDrawable::IsPage() const
{
    return false;
}

//===========================================================================
// IsVisible
//===========================================================================
// Description: returns whether or not this object should be drawn at all
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      bool - true or false is the object visible
//
//===========================================================================
bool FeDrawable::IsVisible() const
{
    return mVisible;
}

//===========================================================================
// LinkPrimitive
//===========================================================================
// Description: links this FeDrawable with it's parent at the scene element 
//              level
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
void FeDrawable::LinkPrimitive
(
    unsigned handle 
)
{
}

//===========================================================================
// ModulateColour
//===========================================================================
// Description: determines how colors modify one another
//
// Constraints:    IF YOU CHANGE THIS FUNCTION, IT SHOULD STILL BE SYMMETRIC - 
//              IE ModulateColour( a, b ) == ModulateColour( b, a ).  It's
//              used both ways 
//
// Parameters:    colour - the colour that's getting changed
//              modulator - the colour to change it by
//
// Return:      None
//
//===========================================================================
void FeDrawable::ModulateColour( tColour& colour, const tColour& modulator )
{
    float a = modulator.Alpha() / 255.0f;
    float r = modulator.Red()   / 255.0f;
    float g = modulator.Green() / 255.0f;
    float b = modulator.Blue()  / 255.0f;

    int ia = static_cast< int >( a * colour.Alpha() );
    int ir = static_cast< int >( r * colour.Red() );
    int ig = static_cast< int >( g * colour.Green() );
    int ib = static_cast< int >( b * colour.Blue() );

    colour.SetAlpha( ia );
    colour.SetRed(   ir );
    colour.SetGreen( ig );
    colour.SetBlue(  ib );
}

void FeDrawable::NormalizeCoord(int ix, int iy, float &x, float &y) const
{
    float height = FeApp::GetInstance()->GetScreenHeight();
    float width = FeApp::GetInstance()->GetScreenWidth();
    float loc = static_cast< float >( iy - 0.0f ) / height;
    x = static_cast< float >( ix - 0.0f ) / width;
    y = loc * height / width;
}

//===========================================================================
// void FeDrawable::Reset()
//===========================================================================
// Description: Reset to default values (used for reloading purposes)
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
void FeDrawable::Reset()
{
    mVisible = true;
    m_clip = false;
    m_layer = 1.0f;

    //
    // Compiler bug?  Just calling ResetTransformation() without the FeDrawable
    // qualifier causes corruption.
    //
    FeDrawable::ResetTransformation();
}

void FeDrawable::ResetTransformation()
{
    m_matrix.Identity();
}

//===========================================================================
// void FeDrawable::RotateArbitrary( float axisX, float axisY, float axisZ, float angle )
//===========================================================================
// Description: allows an FeDrawable to rotate an arbitrary angle about an
//                arbitrary axis of rotation
//
// Constraints:    None
//
// Parameters:    axis(X,Y,Z) - floating point numbers representing the axis 
//                              of rotation
//                angle - the angle to rotate in degrees
//
// Return:      NONE
//
//===========================================================================
void FeDrawable::RotateArbitrary( float axisX, float axisY, float axisZ, float angle )
{
    rmt::Matrix rotation;
    rotation.Identity();

    rmt::Vector axis( axisX, axisY, axisZ );
    rotation.FillRotation( axis, DegreesToRadian( angle ) );

    m_matrix.Mult( rotation );

}

void FeDrawable::RotateX( float angle )
{
    RotateArbitrary( 1.0f, 0, 0, angle );
}

void FeDrawable::RotateY( float angle )
{
    RotateArbitrary( 0, 1.0f, 0, angle );
}

void FeDrawable::RotateZ( float angle )
{
    RotateArbitrary( 0, 0, 1.0f, angle );
}

void FeDrawable::Scale( float factor )
{
    Scale( factor, factor, factor );
}

void FeDrawable::Scale( float factorX, float factorY, float factorZ )
{
    rmt::Matrix scale;
    scale.Identity();
    
    scale.FillScale( factorX, factorY, factorZ );
    
    m_matrix.Mult( scale );
}

//===========================================================================
// SetAlpha
//===========================================================================
// Description: allows the user to set the alpha value for this drawable object
//
// Constraints:    None
//
// Parameters:    float a - the alpha value to be set
//
// Return:      NONE
//
//===========================================================================
void FeDrawable::SetAlpha( float a )    //IAN IMPROVE: this should be a byte, not a float
{
    rAssertMsg( a >= 0 && a <= 1.0f, "FeDrawable::SetAlphaNoRedraw Bad Alpha Value received" );
    mAlpha = a;
    char intAlpha = static_cast< char >( a * 255 );
    mColour.Set( mColour.Red(),mColour.Green(),mColour.Blue(), intAlpha );
    //this->Display();
}

//===========================================================================
// SetClip
//===========================================================================
// Description: alters the clipping rectangle used for drawing this object
//
// Constraints:    None
//
// Parameters:    l - the left clip plane
//              r - the right clip plane
//              t - the top clip plane
//              b - the bottom clip plane
//
// Return:      NONE
//
//===========================================================================
void FeDrawable::SetClip( float l, float b, float r, float t )
{
    int screenResX = p3d::display->GetWidth();
    int screenResY = p3d::display->GetHeight();

    m_rect.top = static_cast<int>( t * screenResY );
    m_rect.bottom = static_cast<int>( b * screenResY );
    m_rect.left = static_cast<int>( l * screenResX );
    m_rect.right = static_cast<int>( r * screenResX );

    m_clip = true;
}     


//===========================================================================
// SetColour
//===========================================================================
// Description: allows the user to set the color value for this drawable object
//
// Constraints:    None
//
// Parameters:    tColour c - the color value to be set
//
// Return:      NONE
//
//===========================================================================
void FeDrawable::SetColour( tColour c )
{
    mColour.Set( c.Red(), c.Green(), c.Blue(), c.Alpha() );
}

void FeDrawable::SetLayer( float layer )
{
    m_layer = layer;
}

void FeDrawable::SetParent( FeDrawable* p )
{
    //NEVER EVER EVER add a reference to your parent - you won't be able to 
    //release things recursively, because the child won't release the parent
    //if( p != NULL )
    //{
        //p->AddRef();
    //}

    //if( mParent != NULL )
    //{
    //    mParent->Release();
    //}

    mParent = p;
}

void FeDrawable::SetTransform( rmt::Matrix* matrix )
{
   m_matrix = *matrix;
}

void FeDrawable::SetTransform( const rmt::Matrix& matrix )
{
    m_matrix = matrix;
}

void FeDrawable::SetTranslation( int x, int y )
{
    float tx,ty;
    NormalizeCoord( x, y, tx, ty);
    SetTranslationRealWorld( tx, ty );
}

void FeDrawable::SetTranslationRealWorld( float x, float y )
{
   m_matrix.Row(3).x = x;
   m_matrix.Row(3).y = y;
}

void FeDrawable::SetVisible(bool v)
{
    if ( v == true )
    {
        //make visible
        SetLayer( 1.0f );
    }
    else
    {
        //make invisible
        SetLayer( 0.0f );     //IAN IMPROVE - this is a major hack - everything gets drawn
    }
    mVisible = v;
}

void FeDrawable::Show()
// called after the 2d list is cleared
{
    Display();
}

void FeDrawable::Transform(rmt::Matrix* matrix)
{
   m_matrix.Mult( *matrix );
}

void FeDrawable::Translate( int x, int y )
{
    float tx,ty;
    float zx,zy;
    NormalizeCoord( 0, 0, zx, zy );
    NormalizeCoord( x, y, tx, ty );

    rmt::Matrix translation;
    translation.Identity();

    rmt::Vector direction( tx - zx, ty - zy , 0 );
    translation.FillTranslate( direction );
    m_matrix.Mult( translation );
}

void FeDrawable::TranslateRealWorld(float x, float y)
{
   m_matrix.Row(3).x += x;
   m_matrix.Row(3).y += y;
}

rmt::Matrix* FeDrawable::GetTransform()
{
    return &this->m_matrix;
}

const rmt::Matrix* FeDrawable::GetTransform() const
{
    return &this->m_matrix;
}

void FeDrawable::GetNormalizedPosition(float &x, float &y) const
{
    x = mPosX;
    y = mPosY;
}


void FeDrawable::SetPosition(int x, int y)
{
    NormalizeCoord( x, y, mPosX, mPosY );
}

//=============================================================================
// TranslatePosition
//=============================================================================
// Description: alters the position of this object
//
// Constraints:    NONE
//
// Parameters:    X - number of pixels to translate in X
//              Y - number of pixels to translate in Y
//
// Return:      NONE
//
//=============================================================================
void FeDrawable::TranslatePosition( int x, int y )
{
    float fx,fy;
    NormalizeCoord( x, y, fx, fy );
    mPosX += fx;
    mPosY += fy;
}


FeDrawable* FeDrawable::Parent()
{ 
    return mParent; 
}

//=============================================================================
// FeDrawable::WatchPosition
//=============================================================================
// Description: adds all attributes to the watcher
//
// Constraints: NONE
//
// Parameters:  NONE
//
// Return:      NONE
//
//=============================================================================
#ifdef DEBUGWATCH
void FeDrawable::WatchAll( const char* nameSpace )
{
    WatchVisibility( nameSpace );
    WatchPosition  ( nameSpace );
    WatchColor     ( nameSpace );
}
#endif

//=============================================================================
// FeDrawable::WatchPosition
//=============================================================================
// Description: add the position values to the watcher
//
// Constraints: NONE
//
// Parameters:  NONE
//
// Return:      NONE
//
//=============================================================================
#ifdef DEBUGWATCH
void FeDrawable::WatchColor( const char* nameSpace )
{
    const char* drawableName = GetName();
    char finishedNamespace[ 1024 ];
    sprintf( finishedNamespace, "%s\\%s\\", nameSpace, drawableName );
    unsigned char* color = reinterpret_cast< unsigned char* >( &mColour );
    ::radDbgWatchDelete( color + 0 );
    ::radDbgWatchDelete( color + 1 );
    ::radDbgWatchDelete( color + 2 );
    ::radDbgWatchDelete( color + 3 );
    ::radDbgWatchAddUnsignedChar( color + 2, "R",     finishedNamespace, NULL, NULL, 0, 255, false );
    ::radDbgWatchAddUnsignedChar( color + 1, "G",     finishedNamespace, NULL, NULL, 0, 255, false );
    ::radDbgWatchAddUnsignedChar( color + 0, "B",     finishedNamespace, NULL, NULL, 0, 255, false );
    ::radDbgWatchAddUnsignedChar( color + 3, "A",     finishedNamespace, NULL, NULL, 0, 255, false );
}
#endif

//=============================================================================
// FeDrawable::WatchPosition
//=============================================================================
// Description: add the position values to the watcher
//
// Constraints: NONE
//
// Parameters:  NONE
//
// Return:      NONE
//
//=============================================================================
#ifdef DEBUGWATCH
void FeDrawable::WatchPosition( const char* nameSpace )
{
    const char* drawableName = GetName();
    char finishedNamespace[ 1024 ];
    sprintf( finishedNamespace, "%s\\%s\\", nameSpace, drawableName );
    ::radDbgWatchDelete( &mPosX );
    ::radDbgWatchDelete( &mPosY );
    ::radDbgWatchAddFloat( &mPosX, "X",     finishedNamespace, NULL, NULL, -1.0f, 1.0f, false );
    ::radDbgWatchAddFloat( &mPosY, "Y",     finishedNamespace, NULL, NULL, -1.0f, 1.0f, false );
    #ifdef DEBUGWATCH
        ::radDbgWatchDelete( &mPosXInt );
        ::radDbgWatchDelete( &mPosYInt );
        ::radDbgWatchAddInt( &mPosXInt, "Xi", finishedNamespace, NULL, NULL, -640, 640  , true  );
        ::radDbgWatchAddInt( &mPosYInt, "Yi", finishedNamespace, NULL, NULL, -640, 640  , true  );
    #endif //DEBUGWATCH
}
#endif
//=============================================================================
// FeDrawable::WatchVisibility
//=============================================================================
// Description: add the position values to the watcher
//
// Constraints: NONE
//
// Parameters:  NONE
//
// Return:      NONE
//
//=============================================================================
#ifdef DEBUGWATCH
void FeDrawable::WatchVisibility( const char* nameSpace )
{
    const char* drawableName = GetName();
    char finishedNamespace[ 1024 ];
    sprintf( finishedNamespace, "%s\\%s\\", nameSpace, drawableName );
    ::radDbgWatchDelete( &mVisible );
    ::radDbgWatchAddBoolean( &mVisible, "Visible", finishedNamespace, NULL, NULL, false );
}
#endif