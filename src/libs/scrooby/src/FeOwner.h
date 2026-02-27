//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   FeOwner
//
// Description: Interface to FeOwner class.  This class both is an FeDrawable
//              and can "own" other FeDrawables.
//
//
// Authors:     Wilkin Ng
//
// Revisions        Date            Author        Revision
//                  2000/05/11      Wng         Created
//                  2000/08/03        DChau        Added 3dObject and Movie
//
//===========================================================================

#ifndef FEOWNER_H
#define FEOWNER_H

//===========================================================================
// Nested Includes
//===========================================================================
#include "FeParent.h"
#include "FeBoundedDrawable.h"

//===========================================================================
// Forward References
//===========================================================================
class FeGroup;
class FeText;
class FeSprite;
class FePolygon;
class FeMovie;
class FePure3dObject;

enum FeLayeringEnum
{
    FEL_BACKGROUND,
    FEL_FOREGROUND,
    FEL_ALL,
    FEL_INVALID
};

//===========================================================================
// Interface Definitions
//  this class is the parent of both layer and group
//  it implement adding the single object within it
//===========================================================================
class FeOwner : public FeDrawable,
                virtual public FeParent
{

public:
    FeOwner( const tName& name ); 
    FeText*         AddText        ( const tName& name, int x, int y );
    FeSprite*       AddSprite      ( const tName& name, int x, int y );
    FeGroup*        AddGroup       ( const tName& name );
    FePolygon*      AddPolygon     ( const tName& name );
    FeMovie*        AddMovie       ( const tName& name, int x, int y );
    FePure3dObject* AddPure3dObject( const tName& name, int x, int y );
    void AddPure3dObject( FePure3dObject* object );

    virtual bool IsOwner() const;
    virtual void Update( float elapsedTime );
    virtual void Display(); //Override

    // Override these to allow them to work on the objects beneath them
    //void SetAlpha( float a );
    //virtual void SetColour( tColour c );


    void Show();    // called on screen update
    virtual void GetBoundingBoxSize( int& width, int& height ) const = 0;
    virtual void GetBoundingBox( int& xMin, int& yMin, int& xMax, int& yMax ) const = 0;

protected:    
    virtual ~FeOwner();
    virtual void DisplayChildren();

    virtual void ShowChildren();
    void HaveNewObject(FeDrawable *s);

};

#endif
