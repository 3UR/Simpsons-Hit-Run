// FeParent.h
// Created by wng on Fri, May 05, 2000 @ 5:29 PM.

#ifndef __FeParent__
#define __FeParent__

#include "owner.h"
#include <p3d/p3dtypes.hpp>
#include <tLinearTable.h>

class FeEntity;
//class tLinearTable;


namespace Scrooby
{
    class Drawable;
    class Group;
    class Polygon;
    class Layer;
    class Sprite;
    class Text;
    class Movie;
    class Pure3dObject;
}

class FeParent :
    virtual public Scrooby::Owner
{
    public:

    FeParent();
    
    virtual void AddChild( FeEntity* s );
    void Resize( const size_t size );
    void RemoveChild( FeEntity* s );
    void ReplaceChild( FeEntity* s, FeEntity* newS );


    FeEntity*       GetChildIndex( int i );
    const FeEntity* GetChildIndex( int i ) const;
    virtual Scrooby::Drawable* GetChildDrawable( const unsigned int index );

    int GetChildrenCount() const;
    virtual unsigned int Size() const;
    FeEntity* GetChild( const char* name );

    //gets a child entity by the hash value of the name
    FeEntity* GetChild( const tUID hashvalue );
    void DeleteChildren();

    //gets a sprite from within the page by hash 
    Scrooby::Sprite* GetSprite( const tUID hashValue );

    //gets a sprite from within the page by name
    Scrooby::Sprite* GetSprite( const char* name );

    //gets a text object from within the page by hash
    Scrooby::Text* GetText( const tUID hashValue );
    
    //gets a text object from within the page by name
    Scrooby::Text* GetText( const char* name );

    //gets a group object from within the page by hash
    Scrooby::Group* GetGroup( const tUID hashValue );

    //gets a text object from within the page by name
    Scrooby::Group* GetGroup( const char* name );

    //gets a polygon object from within the page by hash
    Scrooby::Polygon* GetPolygon( const tUID hashValue );

    //gets a text object from within the page by name
    Scrooby::Polygon* GetPolygon( const char* name );

    //gets a movie object from within the page by hash
    Scrooby::Movie* GetMovie( const tUID hashValue );

    //gets a movie object from within the page by name
    Scrooby::Movie* GetMovie( const char* name );

    //gets a pure3d object from within the page by hash
    Scrooby::Pure3dObject* GetPure3dObject( const tUID hashValue );

    //gets a pure3d object from within the page by name
    Scrooby::Pure3dObject* GetPure3dObject( const char* name );

    //gets a layer object from within the page by hash
    Scrooby::Layer* GetLayer( const tUID hashValue );

    //gets a text object from within the page by name
    Scrooby::Layer* GetLayer( const char* name );

    
    protected:
    
    void ResetIter() const;

    tLinearTable::RawIterator* mpChildIter;
    mutable int mCurChildIndex;
    tLinearTable* mChildren;
    virtual ~FeParent();


    private:
    
    // No copying. Declare but don't define.
    //
    FeParent( const FeParent& );

    // No assignment. Declare but don't define.
    //
    FeParent& operator=( const FeParent& );

};

#endif
