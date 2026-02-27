//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   FeResourceManager
//
// Description: FeOwner class - this class contains other objects in the FE
//
// Authors:     IJG        ???
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================


#include "FeGroup.h"
#include "FeMovie.h"
#include "FeOwner.h"
#include "FePolygon.h"
#include "FePure3dObject.h"
#include "FeSprite.h"
#include "FeText.h"
#include <p3d/matrixstack.hpp>
#include "tLinearTable.h"
#include <raddebug.hpp>
#include <p3d/utility.hpp>
#include "utility/debugmessages.h"
#include "string.h"


FeOwner::FeOwner( const tName& name ) : 
    FeDrawable( name )
{
}


FeOwner::~FeOwner()
{
}

FeGroup* FeOwner::AddGroup( const tName& name )
{
    FeGroup* s = new FeGroup( name );
    HaveNewObject( s );
    return s;    
}



void FeOwner::HaveNewObject(FeDrawable *s)
{
    s->SetParent(this);
    AddChild(s);
}

void FeOwner::Show()
// called after the 2d list is cleared
{
    ShowChildren();
    SetVisible(IsVisible());// restore visibility setting
}

void FeOwner::ShowChildren()
{
    tLinearTable::RawIterator iter(mChildren);
    FeEntity* entity = iter.First();
    while ( entity != NULL )
    {
        rAssert( entity );
        FeDrawable* drawable = dynamic_cast< FeDrawable* >( entity );
        drawable->Show();
        entity = iter.Next();
    }
}


//this never seems to get called
void FeOwner::DisplayChildren()
{
    tLinearTable::RawIterator iter(mChildren);
    FeDrawable *drawable = dynamic_cast<FeDrawable*>( iter.First() );
    while (drawable)
    {
        drawable->Display();
        drawable = dynamic_cast<FeDrawable*>( iter.Next() );
    }
}

void FeOwner::Update( float elapsedTime )
{
    int numChildren = this->GetChildrenCount();

    for( int i = 0; i < numChildren; i++ )
    {
        FeEntity* feEntity = this->GetChildIndex( i );
        rAssert( feEntity );

        if( feEntity->IsDrawable() )
        {
            FeDrawable* drawable = static_cast< FeDrawable* >( feEntity );
            if( drawable && drawable->IsVisible() )
            {
                drawable->Update( elapsedTime );
            }
        }
    }
}

void FeOwner::Display()
{
    int numChildren = this->GetChildrenCount();

    for( int i = 0; i < numChildren; i++ )
    {
        FeEntity* feEntity = this->GetChildIndex( i );
        rAssert( feEntity );

        if( feEntity->IsDrawable() )
        {
            FeDrawable* drawable = static_cast< FeDrawable* >( feEntity );
            rAssert( drawable );

            if( drawable->IsVisible() )
            {
                float x = 0.0f;
                float y = 0.0f;

                p3d::stack->Push();
            
                // Apply the user transforms
                rmt::Matrix* userMatrix = drawable->GetMatrix();
                p3d::stack->Multiply( *userMatrix );

                // Translate by the original position of the object
                drawable->GetNormalizedPosition( x, y );
                p3d::stack->Translate( x, y, 0.0f );

                tColour originalColour = drawable->GetColour();
                tColour newColour = drawable->GetColour();
                drawable->ModulateColour( newColour, GetColour() );
                drawable->SetColour( newColour );
                drawable->Display();
                drawable->SetColour( originalColour );

                p3d::stack->Pop();
            }
        }
    }
}

FeText* FeOwner::AddText( const tName& name, int x, int y )
{
    FeText* s = new FeText( name, x, y );
    HaveNewObject( s );
    return s;
}


FeSprite* FeOwner::AddSprite( const tName& name, int x, int y)
{
    FeSprite* s= new FeSprite( name, x, y );
    HaveNewObject( s );
    return s;    
}

FePolygon* FeOwner::AddPolygon( const tName& name )
{
    FePolygon* s= new FePolygon( name );
    HaveNewObject( s );
    return s;    
}


FeMovie* FeOwner::AddMovie( const tName& name, int x, int y )
{
    FeMovie* pMovie = new FeMovie( name, x, y );
    
    this->HaveNewObject( pMovie );
    
    return( pMovie );    
}

FePure3dObject* FeOwner::AddPure3dObject( const tName& name, int x, int y )
{
    FePure3dObject* pObject = new FePure3dObject( name, x, y );
    
    this->HaveNewObject( pObject );
    
    return( pObject );    
}

//===========================================================================
// AddPure3dObject
//===========================================================================
// Description: adds a pure3d object that has already been created to the 
//              hierarchy
//
// Constraints:    the pointer is now owned by this owner, and will be deleted 
//              when the destructor is called
//
// Parameters:    object - pointer to the FePure3dObject to the added
//
// Return:      NONE
//
//===========================================================================
void FeOwner::AddPure3dObject( FePure3dObject* object )
{
    this->HaveNewObject( object );
}

/*void FeOwner::SetColour( tColour c )
{
    int i = 0;
    for( i; i < GetChildrenCount(); i++ )
    {
        FeDrawable* drawable = dynamic_cast<FeDrawable*>(GetChildIndex( i ));
        if( drawable )
        {
            drawable->SetColour( c );
        }
    }
    FeDrawable::SetColour( c );
}

void FeOwner::SetAlpha( float a )
{
    int i = 0;
    for( i; i < GetChildrenCount(); i++ )
    {
        FeDrawable* drawable = dynamic_cast<FeDrawable*>(GetChildIndex( i ));
        if( drawable )
        {
            drawable->SetAlpha( a );
        }
    }
    FeDrawable::SetAlpha( a );
}*/

//===========================================================================
// FeOwner::IsOwner()
//===========================================================================
// Description: checks to see if this object derives from a certain class
//
// Constraints: NONE
//
// Parameters:  NONE
//
// Return:      is the object an "owner"
//
//===========================================================================
bool FeOwner::IsOwner() const
{
    return true;
}