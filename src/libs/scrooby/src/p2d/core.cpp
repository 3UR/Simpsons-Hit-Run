// Pure 3d stuff.
#include <sprite.hpp>
#include <textstring.hpp>
#include <file.hpp>
#include <typeface.hpp>
#include <font.hpp>
#include <utility.hpp>
#include <drawable.hpp>
#include <shader.hpp>
#include <matrixstack.hpp>
#include <assert.h>
#include "core.hpp"
#include "FeDrawable.h"
#include "FePolygon.h"
#include "FePage.h"
#include "FePure3dObject.h"
#include "FeSprite.h"
#include "FeText.h"
#include "FeApp.h"
#include "ResourceManager/ResourceTextStyle.h"
#include <error.hpp>
#include <imagefactory.hpp>
#include "poly.hpp"
#include <raddebug.hpp>
#include <stdio.h>
#include <string.h>


namespace Pure2D
{
enum {
    SE_GENERAL,
    SE_SPRITE,
    SE_TEXT,
    SE_PURE3DOBJECT,
    SE_POLYGON
};

SceneElement::SceneElement()
{
   Init();
}

void SceneElement::Init()
{
  isUsed = false;
  isRoot = true;
  m_tDrawable = NULL;
  m_feDrawable = NULL;
  child = NULL;
  sibling = NULL;
  mType = SE_GENERAL;
}

SceneElement::~SceneElement()
{
    Release();
}

//===========================================================================
// SceneElement::Display
//===========================================================================
// Description: this particular Scene Element actually gets rendered
//
// Constraints:    NONE
//
// Parameters:    properties - the properties ie alpha, etc that modulate the 
//                           display of the SceneElements
//
// Return:      None
//
//===========================================================================
void SceneElement::Display( const SceneElementRenderProperties& properties )
{
    if( !m_tDrawable )
    {
        return;
    }
    
    P3DASSERT( GetFeDrawable() );
    SceneElementRenderProperties newProp = this->UpdateProperties( properties );//IAN IMPROVE: don't call this twice all the time
    const tColour& c = newProp.colour;

    if( mType == SE_SPRITE )
    {
        tSprite* sprite = static_cast<tSprite*>( m_tDrawable );
        rAssert( NULL != sprite );
        sprite->SetColour( c );    
        sprite->Display();
    }
    else if( mType == SE_TEXT )
    {
        rValid(m_tDrawable);
        tTextString* text = static_cast<tTextString*>( m_tDrawable );
        rAssert( NULL != text );

#ifdef _BASE_FONT_SIZE
        FeText* feText = static_cast<FeText*>( m_feDrawable );
        rAssert( NULL != feText );

        FeResourceManager* pResourceManager = &FeApp::GetInstance()->GetFeResourceManager();
        rAssert( NULL != pResourceManager );
        FeResourceTextStyle* pTextStyle = pResourceManager->GetResourceTextStyle( feText->GetTextStyle() );
        rAssert( NULL != pTextStyle );

        int fontResourceHandle = pTextStyle->GetFontResourceID();    
        tTextureFont* pFont = static_cast<tTextureFont*>( pResourceManager->GetResourceFont( fontResourceHandle ) );        
        rAssert( NULL != pFont );

        pFont->SetSize( static_cast<float>( pTextStyle->GetFontSize() ) );
#endif

        text->SetColour( c );
        text->Display();
    }
    else if( mType == SE_PURE3DOBJECT )
    {
        FePure3dObject* pure3d_object = static_cast< FePure3dObject* >( m_feDrawable );
        rAssert(pure3d_object);
        pure3d_object->Render();
    }
    else if( mType == SE_POLYGON )
    {
//        FePolygon* fePolygon = dynamic_cast< FePolygon* >( this->m_feDrawable );
//        rAssert( fePolygon != NULL );
//        fePolygon->SetColour( c );
//        pddiColour cNoConst = c;    //IAN IMPROVE: why does PDDIcolour not respect const?
//        fePolygon->SetAlpha( cNoConst.Alpha() / 255.0f );   //IAN IMPROVE: why is alpha disconnected from color?
        Poly* thePoly = static_cast< Poly* >( m_tDrawable );
        thePoly->SetColour( c );
        m_tDrawable->Display();
    }
    else
    {
        m_tDrawable->Display();
    }
}

//===========================================================================
// SceneElement::Release
//===========================================================================
// Description: releases a scene element so that it no longer honlds pointers
//              to FeDrawables and tDrawables
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
void SceneElement::Release()
{
    if( m_tDrawable != NULL )
    {
        m_tDrawable->Release();   //the resource manager will release the 
                                    //drawable
        m_tDrawable = NULL;
    }

    if( m_feDrawable != NULL )
    {
        // Fixes recursive dependency
        FeDrawable* drawable = m_feDrawable;
        m_feDrawable = NULL;
        drawable->Release();
        //m_feDrawable->Release();
    }
}

//===========================================================================
// SceneElement::SetFeDrawable
//===========================================================================
// Description: Sets the FeDrawable associated with this scene element
//
// Constraints:    NONE
//
// Parameters:    d - pointer to the new feDrawable
//
// Return:      NONE
//
//===========================================================================
void SceneElement::SetFeDrawable( FeDrawable* d )
{
    if( d != NULL )
    {
        d->AddRef();
    }
    if( m_feDrawable != NULL )
    {
        m_feDrawable->Release();
    }
    m_feDrawable = d;
}

//===========================================================================
// SceneElement::SetTDrawable
//===========================================================================
// Description: Sets the tDrawable associated with this scene element
//
// Constraints:    NONE
//
// Parameters:    d - pointer to the new tDrawable
//
// Return:      NONE
//
//===========================================================================
void SceneElement::SetTDrawable( tDrawable* d )
{
//    if( d != NULL )
//    {
//        d->AddRef();
//    }
    if( m_tDrawable != NULL )
    {
        m_tDrawable->Release();
    }
    m_tDrawable = d;
    if( m_tDrawable != NULL )
    {                               //IAN IMPROVE: you've got it backwards, are you sure?
        m_tDrawable->AddRef();    //DON'T add a reference here - the resource 
                                    //manager still owns the one and only copy
    }
}

void SceneElement::SetType( int t )
{
    mType = t;
    if( t == SE_SPRITE )
    {
           P3DASSERT( m_tDrawable && dynamic_cast< tSprite* >( m_tDrawable ) );
    }
    if( t == SE_TEXT )
    {
           P3DASSERT( m_tDrawable && dynamic_cast< tTextString* >( m_tDrawable ) );
    }
    if( t == SE_POLYGON )
    {
        P3DASSERT( m_tDrawable && dynamic_cast< Poly* >( m_tDrawable ) );
    }
}

//===========================================================================
// SceneElementRenderProperties
//===========================================================================
// Description: constructor : sets the color to white, and the alpha to opaque
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
SceneElementRenderProperties SceneElement::UpdateProperties( const SceneElementRenderProperties& properties )
{
    SceneElementRenderProperties returnMe = properties;
    const tColour& drawableColour = this->m_feDrawable->GetColour();
    const tColour& propertyColour = properties.colour;
    int newR = static_cast< int >( drawableColour.Red()   * propertyColour.Red()   / 255 );
    int newG = static_cast< int >( drawableColour.Green() * propertyColour.Green() / 255 );
    int newB = static_cast< int >( drawableColour.Blue()  * propertyColour.Blue()  / 255 );
    int newA = static_cast< int >( drawableColour.Alpha() * propertyColour.Alpha() / 255 );
    returnMe.colour = tColour( newR, newG, newB, newA );
    return returnMe;
}

//===========================================================================
// Core::Core
//===========================================================================
// Description: constructor - initializes an array of SceneElements
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
Core::Core()
{
   scene = new ( ScroobyPermPool ) SceneElement[ MAX_OBJECTS ];
   scene[0].Used() = true;
   sceneUsed = 0;

   unsigned int i;
   for( i = 0; i < MAX_EFFECTS; i++ )
   {
      effects[ i ] = NULL;
      effectPlaying[ i ] = false;
   }

   isSorted = false;
}

Core::~Core()
{
   ClearObjects();
   ClearEffects();

   delete[] scene;
}

/*void Core::LoadData(char* screenName)
{
    p3d::inventory->AddSection(screenName);
    p3d::inventory->SelectSection(screenName);

    tFile* file;

    char p3dfile[512];
    sprintf(p3dfile,"%s.p3d",screenName);
//    file = fileSystem->Open( p3dfile );
    if( file )
    {
        p3d::loadManager->Load(file);
    }
    else
    {
        tImageHandler::LoadType t_tga = tImageHandler::SPRITE;  //initialization is umimportant - it removes warnings though
        tImageHandler::LoadType t_png = tImageHandler::SPRITE;
        tImageHandler::LoadType t_bmp = tImageHandler::SPRITE;

        tImageHandler* tga = dynamic_cast<tImageHandler*>(p3d::loadManager->GetHandler("tga"));
        tImageHandler* png = dynamic_cast<tImageHandler*>(p3d::loadManager->GetHandler("png"));
        tImageHandler* bmp = dynamic_cast<tImageHandler*>(p3d::loadManager->GetHandler("bmp"));

        if(tga)
        {
            t_tga = tga->GetLoadType();
            tga->SetLoadType(tImageHandler::SPRITE);
        }

        if(png)
        {
            t_png = png->GetLoadType();
            png->SetLoadType(tImageHandler::SPRITE);
        }

        if(bmp)
        {
            t_bmp = bmp->GetLoadType();
            bmp->SetLoadType(tImageHandler::SPRITE);
        }


//        file = fileSystem->FindFirst(screenName);

        while(file)
        {
            p3d::loadManager->Load(file);
//            file = fileSystem->FindNext();
        }

        if(tga)
        {
            tga->SetLoadType(t_tga);
        }

        if(png)
        {
            png->SetLoadType(t_png);
        }

        if(bmp)
        {
            bmp->SetLoadType(t_bmp);
        }
    }

    p3d::inventory->SelectSection("default");
}
*/
  
/*
void Core::FreeData(char* screenName)
{
   p3d::inventory->DeleteSection(screenName);
}
*/


P3D_WCHAR defaultCharacters[] = 
{
    '$',    //24
    ' ',    //32
    '!',    //33
    '"',    //34
    '#',    //35
    '\'',   //39
    '(',    //40
    ')',    //41
    '+',    //43
    ',',    //44
    '-',    //45    //hyphen
    '.',    //46
    '/',    //47
    '0','1','2','3','4','5','6','7','8','9',    //48-57
    ':',    //58
    '?',    //63
    '_',    //underscore
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',    //65-90
    'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',    //97-122
     133,   //133 - this is the '...' character   
    176,    //°
    194,    //'Â',     //194
    201,    //'É',     //201
    220,    //'Ü',     //220
    223,    //'ß',     //223
    224,    //'à',     //224
    226,    //'â',     //226
    228,    //'ä',     //228
    231,    //'ç',     //231
    232,    //'è',     //232
    233,    //'é',     //233
    234,    //'ê',     //234
    238,    //'î',     //238
    244,    //'ô',     //244
    246,    //'ö',     //246
    252,    //'ü',     //252
//    0x30a6,
//    0x30aa,
//    0x30b5,
//    0x30b7,
//    0x30c9,
//    0x30d7,
//    0x30e7,
//    0x30f3,
    '\0',
    '\0' // Extra null can't hurt.
};

//IAN IMPROVE: these desired text characters must be loaded from disk at some point

//char defaultCharacters[] = "ABCDEFGHIJKLMNOPQRSTYVWXYZabcdefghijklmnopqrstuvwxyz";

//===========================================================================
// LoadFontFromBuffer
//===========================================================================
// Description: Builds a font with pure3d - TTF file already loaded
//
// Constraints:    None
//
// Parameters:    name - the name that we're calling this font
//              buffer - the data buffer of the TTF from the disk
//              bufferSize - the size of the data buffer in bytes
//              fsize - the size of the font in pixels
//
// Return:      NONE
//
//===========================================================================
tFont* Core::LoadFontFromBuffer
(
    char* name,
    char* buffer,
    const int bufferSize,
    float fontSize
)
{
    tTextureFont* thisFont = NULL;


    tFileMem* mem = new tFileMem( reinterpret_cast<unsigned char*>( buffer ), bufferSize ); //P3D NEW
    mem->AddRef();
    mem->SetFilename( name );

    tTypeFace* typeFace = new tTypeFace( mem, static_cast< int >( fontSize ) );             //P3D NEW
    typeFace->AddRef();
    typeFace->SetCharacters( defaultCharacters );
    thisFont = typeFace->MakeTextureFont( name, P3D_SCREEN_FONT, static_cast< int >( fontSize ) );      //IAN IMPROVE: casting int to float is bad
    thisFont->SetMissingLetter( (P3D_WCHAR)'#' );
    typeFace->Release();        //IAN IMPROVE: 
    rValid( thisFont );
//    thisFont->AddRef();
//    mem->refCount = 1;
    mem->Release();

    thisFont->SetName( name );
    //thisFont->GetShader()->SetBlendMode( PDDI_BLEND_ALPHA );
    return( thisFont );
}

void Core::FreeFont( tFont* theFont )
{
   rValid( theFont );
   theFont->Release();
}


unsigned Core::AddDummy( FeDrawable* feDrawable )
{
    unsigned returnMe = Insert( NULL, feDrawable );
    return returnMe;
}

void Core::ReplaceDummy( FeDrawable* pFeDrawable, unsigned coreHandle )
{
    rValid( pFeDrawable );

    scene[ coreHandle ].SetFeDrawable( pFeDrawable );
    scene[ coreHandle ].SetTDrawable( NULL );
    return;   
}

unsigned Core::AddObject( FeDrawable* feDrawable, char *name )
{
   return Insert( p3d::find<tDrawable>(name), feDrawable );
}


unsigned Core::AddSprite( FeSprite* pFeSprite, tSprite* ptSprite )
{
    rValid( pFeSprite );
    rValid( ptSprite );

    //
    // Set the material properties.
    //
    tShader* pShader = NULL;
    pShader = ptSprite->GetShader();
    if( pShader == NULL )
    {
        //IAN IMPROVE: this whole if statement is meant to trap a bug in release mode
        pShader++;
    }
    else
    {
        rValid( pShader );
        pShader->SetInt( PDDI_SP_BLENDMODE, PDDI_BLEND_ALPHA );
        pShader->SetInt( PDDI_SP_FILTER,  PDDI_FILTER_NONE );
        pShader->SetInt( PDDI_SP_UVMODE, PDDI_UV_CLAMP );
#ifdef P3D_PS2
        pShader->SetInt( PDDI_SP_ALPHATEST, 1 );//turn on alpha testing     (only required for PS2 16 bit framebuffers )
        pShader->SetInt( PDDI_SP_ALPHACOMPARE, PDDI_COMPARE_GREATER );//turn alpha testing to "GREATER THAN"
    //IAN IMPROVE: check the framebuffer depth also
#endif

    }

    //
    // Insert the sprite into the scene element list.
    //
    unsigned handle = this->Insert( ptSprite, pFeSprite );
    scene[ handle ].SetType( SE_SPRITE );
    
    return( handle );
}


unsigned Core::AddPure3dObject( FeDrawable* pFeDrawable, tDrawable* ptDrawable )
{
    rValid( pFeDrawable );
    rValid( ptDrawable );

    //
    // Insert the pure3d object into the scene element list.
    //
    unsigned handle = this->Insert( ptDrawable, pFeDrawable );
    scene[ handle ].SetType( SE_PURE3DOBJECT );
    
    return( handle );
}

//===========================================================================
// ReplacePure3dObject
//===========================================================================
// Description: Replaces a Pure3D object that's been added to the core
//
// Constraints:    None
//
// Parameters:    pFeDrawable - pointer to replace for the drawable
//              coreHandle - the handle of the core object to replace
//              ptDrawable
//
// Return:      NONE
//
//===========================================================================
void Core::ReplacePure3dObject
( 
    FeDrawable* pFeDrawable, 
    unsigned coreHandle, 
    tDrawable* ptDrawable 
)
{
    rValid( pFeDrawable );
    rValid( ptDrawable );
    scene[ coreHandle ].SetFeDrawable( pFeDrawable );
    scene[ coreHandle ].SetTDrawable( ptDrawable );
}

//===========================================================================
// ReplacePure3dObject
//===========================================================================
// Description: Replaces a Pure3D object that's been added to the core
//
// Constraints:    None
//
// Parameters:    pFeDrawable - pointer to replace for the drawable
//              coreHandle - the handle of the core object to replace
//              ptDrawable
//
// Return:      NONE
//
//===========================================================================

void Core::ReplaceSprite
(
    FeSprite* pFeSprite,
    unsigned spriteHandle, 
    tSprite* pNewImage
)
{
    rValid( pFeSprite );
    rValid( pNewImage );

    //
    // Set the material properties.
    //
    //IAN IMPROVE: this doesn't belong here
    tShader* pShader = pNewImage->GetShader();
    pShader->SetInt( PDDI_SP_BLENDMODE, PDDI_BLEND_ALPHA );
    pShader->SetInt( PDDI_SP_FILTER,  PDDI_FILTER_NONE );
    pShader->SetInt( PDDI_SP_UVMODE, PDDI_UV_CLAMP );

    scene[spriteHandle].SetFeDrawable( pFeSprite );
    scene[spriteHandle].SetTDrawable( pNewImage );
   
    return;   
}


unsigned Core::AddText( FeText* pFeText, tFont* pFont, UnicodeChar* string )
{
    //
    // Note: UnicodeChar == P3D_WCHAR == unsigned short
    //
    tTextString* pTextString = new tTextString( pFont, reinterpret_cast<P3D_WCHAR*>( string ), 1.0f ); //P3D NEW
   
    unsigned handle = this->Insert( pTextString, pFeText );
    
    scene[handle].SetType( SE_TEXT );
    
    return( handle );
    return 0;
}


void Core::ReplaceTextObject( FeText* pFeText, tFont* pFont, UnicodeChar* string, unsigned textHandle )
{ 
    //
    // Note: UnicodeChar == P3D_WCHAR == unsigned short
    //
    FeDrawable* drawable = scene[ textHandle ].GetFeDrawable();
    tTextString* pTextString = dynamic_cast< tTextString* >( scene[textHandle].GetTDrawable() );
    if( (drawable == NULL) || (pTextString == NULL) )
    //if( true )
    {
        pTextString = new tTextString( pFont, reinterpret_cast<P3D_WCHAR*>( string ), 1.0f );
        scene[textHandle].SetFeDrawable( pFeText );
        scene[textHandle].SetTDrawable( pTextString );
    }
    else
    {
        pTextString->SetFont( pFont );
        pTextString->SetString( reinterpret_cast<P3D_WCHAR*>( string ) );
    }
    return;
}


unsigned Core::AddPoly( FePolygon* pFePolygon, int nVer, rmt::Vector* ver, tColour* c )
{
    Poly* pPoly = new ( ScroobyPermPool ) Poly( nVer, ver, c );
    unsigned handle = this->Insert( pPoly, pFePolygon );
    scene[ handle ].SetType( SE_POLYGON );        
    return( handle );
}

//===========================================================================
// DeleteObject
//===========================================================================
// Description: Deletes a FeDrawable/tDrawable pair from the scene element 
//              array
//
// Constraints:    NONE
//
// Parameters:    unsigned int i - the index in the array to delete
//
// Return:      NONE
//
//===========================================================================
void Core::DeleteObject( unsigned i )
{
   P3DASSERT(i != 0);

   //if the element is already dead, don't do anything
   if( !scene[ i ].Used() )
   {
      return;
   }

   //mark the element unused and release it
   scene[i].Used() = false;
   scene[i].Release();

   //update the number of used slots if the deletion was performed at the end 
   //of the list
   if( i + 1 == sceneUsed )
   {
      while( scene[ sceneUsed ].Used() == false )
      {
          sceneUsed--;
      }
   }
}

void Core::ReplacePoly( FePolygon* pFePolygon, unsigned polyHandle, int nVer, rmt::Vector* ver, tColour* c )
{
   //IAN IMPROVE: instead of newing a new polygon, why not just replace the data?
   Poly* pPoly = new( ScroobyPermPool ) Poly( nVer, ver, c );
   
   scene[polyHandle].SetTDrawable( pPoly );
   scene[polyHandle].SetFeDrawable( pFePolygon );
}


//===========================================================================
// ClearObjects
//===========================================================================
// Description: clears all the objects from the list of scene elements
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
void Core::ClearObjects()
{
    for( unsigned i = 1; i < sceneUsed; i++ )
    {
        if( scene[ i ].Used() )
        {
            scene[ i ].Used() = false;
            scene[ i ].Release();
        }
    }
    sceneUsed = 0;
}


void Core::Link(unsigned parent, unsigned child)
{
   P3DASSERT( scene != 0 );
   if( scene == 0 )
   {
       return;
   }
   
   if((!scene[parent].Used()) || (!scene[child].Used()))
   {
      return;
   }
   
   scene[child].isRoot = false;
   
   if (scene[parent].child==NULL)
   {
       scene[parent].child = &scene[child];
   }
   else
   {// append in the end to preserve bottom to top order
        SceneElement *ptr = scene[parent].child;
        while(ptr->sibling != NULL)
        {
            ptr = ptr->sibling;
        }
        ptr->sibling = &scene[child];
   }
}

void Core::Unlink(unsigned parent, unsigned child)
{
   if((!scene[parent].Used()) || (!scene[child].Used()))
      return;

   scene[child].isRoot = true;
   
   SceneElement *prevPtr,*tPtr2;
   for (tPtr2 = scene[parent].child, prevPtr = NULL; tPtr2 != NULL;
     prevPtr = tPtr2, tPtr2 = tPtr2->sibling) {
        if (tPtr2 != &scene[child]){
            continue;
        }
        if (prevPtr == NULL) {
            scene[parent].child = tPtr2->sibling;
        } else {
            prevPtr->sibling = tPtr2->sibling;
        }
        break;         
     }
}

int Core::GetDisplayWidth()
{
    return p3d::display->GetHeight();
}

int Core::GetDisplayHeight()
{
    return p3d::display->GetWidth();
}

unsigned Core::AddEffect(char* name)
{
   Effect* effect = p3d::find< Effect >( name );
   if( effect )
   {
      unsigned int handle;
      for( handle = 0; handle < MAX_EFFECTS; handle++ )
      {
         if(effects[handle] == NULL)
            break;
      }
      P3DASSERT(handle < MAX_EFFECTS);
      effects[handle] = effect->Clone();
      effects[handle]->AddRef();

      return handle;
   }
   return 0;
}

void Core::ApplyEffect(unsigned effectHandle, unsigned objectHandle)
{
   if(effects[effectHandle] && scene[objectHandle].Used() && scene[objectHandle].GetTDrawable())
   {
      effects[effectHandle]->ApplyTo(scene[objectHandle].GetTDrawable());
   }
}

void Core::DeleteEffect(unsigned effectHandle)
{
   if(effects[effectHandle])
      effects[effectHandle]->Release();

   effects[effectHandle] = NULL;
   effectPlaying[effectHandle] = true;
}

void Core::ClearEffects(void)
{
   unsigned int i;
   for(i = 0; i < MAX_EFFECTS; i++)
   {
      if( effects[i] )
      {
         effects[i]->Release();
      }

      effects[i] = NULL;
      effectPlaying[i] = false;
   }
}

void Core::SetTime(unsigned effectHandle, float time)
{
   if(effects[effectHandle])
      effects[effectHandle]->SetTime(time);
}

void Core::Play(unsigned effectHandle, bool playing)
{
   if( effects[ effectHandle ] )
   {
      effectPlaying[ effectHandle ] = playing;
   }
}

void Core::SetTime( float time )
{
    unsigned int i;
    for( i = 0; i < MAX_EFFECTS; i++ )
    {
        if( effects[i] && effectPlaying[i] )
        {
            effects[i]->SetTime(time);
        }
    }
}

void Core::Display(void)
{
   Display(0.0f, 1.0f);
}

void Core::Display(float startLayer, float endLayer)
{
    this->DisplaySetup();

    mStartRenderLayer = startLayer;     //IAN IMPROVE: are these significant
    mEndRenderLayer = endLayer;

    SceneElement** walk = sorted;
    SceneElementRenderProperties properties;
    
    while( *walk )
    {
        float layer = 1.0f;

        //if there's a drawable associated with this scene element, ocnsider drawing it
        FeDrawable* feDrawable = ( *walk )->GetFeDrawable();
        if( feDrawable )
        {
            layer = feDrawable->GetLayer();

            //only draw if the layer is within the prescribed range
            if( ( layer >= startLayer ) && ( layer <= endLayer ) )
            {
                this->Render( *walk,  properties );
            }
        }
        ++walk;
    }

    this->DisplayTeardown();
}

//===========================================================================
// Display
//===========================================================================
// Description: Displays a range of layers based on their names.  
//              NB - end layer not drawn
//
// Constraints:    NONE
//
// Parameters:    startLayer - handle to the layer that is the frist one drawn
//              endLayer - this is the layer after the last one drawn
//
// Return:      NONE
//
//===========================================================================
void Core::Display( tUID startLayer, tUID endLayer )
{
    this->DisplaySetup();
    SceneElement** walk = sorted;
    SceneElementRenderProperties properties;
    
    mStartRenderLayer = 0.1f;     //IAN IMPROVE: are these significant
    mEndRenderLayer = 1.0f;


    bool seenStartFlag = false;

    while( *walk )
    {
        float layer = 1.0f;

        //if there's a drawable associated with this scene element, ocnsider drawing it
        if( (*walk)->GetFeDrawable() )
        {
            tUID uid = ( *walk )->GetFeDrawable()->GetUID();
            if( uid == startLayer )
            {
                seenStartFlag = true;
            }
            if( uid == endLayer )
            {
                break;
            }

            //only draw if the layer is within the prescribed range
            if( seenStartFlag == true )
            {
                Render( *walk,  properties );
            }
        }
                
        ++walk;
    }
    this->DisplayTeardown();
}


//===========================================================================
// Core::DrawBackground
//===========================================================================
// Description: Displays a range of layers based on their names.  
//              NB - end layer not drawn
//
// Constraints:    NONE
//
// Parameters:    startLayer - handle to the layer that is the frist one drawn
//              endLayer - this is the layer after the last one drawn
//
// Return:      NONE
//
//===========================================================================
void Core::DrawBackground()
{
    this->DisplaySetup();
    SceneElement** walk = sorted;
    SceneElementRenderProperties properties;
    
    mStartRenderLayer = 0.1f;     //IAN IMPROVE: are these significant
    mEndRenderLayer = 1.0f;


    bool seenStartFlag = false;

    while( *walk )
    {
        //if there's a drawable associated with this scene element, ocnsider drawing it
        if( (*walk)->GetFeDrawable() )
        {
            tUID uid = ( *walk )->GetFeDrawable()->GetUID();

            Render( *walk,  properties );

            if( uid == tEntity::MakeUID( "FeDivide" ) )
            {
                break;
            }
        }
                
        ++walk;
    }
    this->DisplayTeardown();
}


//===========================================================================
// Core::DrawForeground
//===========================================================================
// Description: Displays a range of layers based on their names.  
//              NB - end layer not drawn
//
// Constraints:    NONE
//
// Parameters:    startLayer - handle to the layer that is the frist one drawn
//              endLayer - this is the layer after the last one drawn
//
// Return:      NONE
//
//===========================================================================
void Core::DrawForeground()
{

    this->DisplaySetup();
    SceneElement** walk = sorted;
    SceneElementRenderProperties properties;
    
    mStartRenderLayer = 0.1f;     //IAN IMPROVE: are these significant
    mEndRenderLayer = 1.0f;


    bool seenStartFlag = false;

    while( *walk )
    {
        if( (*walk)->GetFeDrawable() )
        {
            tUID uid = ( *walk )->GetFeDrawable()->GetUID();

            if( uid == tEntity::MakeUID( "FeDivide" ) )
            {
                ++walk;
                break;
            }
        }
                
        ++walk;
    }

    
    while( *walk )
    {
        //if there's a drawable associated with this scene element, ocnsider drawing it
        if( (*walk)->GetFeDrawable() )
        {
            Render( *walk,  properties );
        }
                
        ++walk;
    }
    DisplayTeardown();
}


//===========================================================================
// DisplaySetup
//===========================================================================
// Description: does all the setup stuff for displaying a frame - clearing Z 
//              buffer, etc
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
void Core::DisplaySetup()
{
    this->Sort();

    p3d::pddi->PushState( PDDI_STATE_RENDER );
    p3d::pddi->PushState( PDDI_STATE_VIEW );
    p3d::pddi->EnableZBuffer( false );

    //float n;
    //float f;
    //float fov;
    float aspect;

    //p3d::pddi->GetCamera( &n, &f, &fov, &aspect );

    aspect = static_cast< float >( FeApp::GetInstance()->GetScreenWidth() ) / static_cast< float >( FeApp::GetInstance()->GetScreenHeight() );
    //aspect = static_cast< float >( p3d::display->GetWidth() ) / static_cast< float >( p3d::display->GetHeight() );

    //p3d::pddi->SetCamera( 0.1f, 10.0f, fov, aspect );
    //fov = 1.5707f;
    //fov *= ((640.0f/480.0f) / aspect);
    //aspect *= ((640.0f/480.0f) / aspect);
    p3d::pddi->SetCamera( 0.1f, 10.0f, 1.5707f, aspect ); // 1.5707 = PI /2
    //p3d::pddi->SetCamera( 0.1f, 10.0f, 1.50f, aspect * 0.9333f);    
    //p3d::pddi->SetProjectionMode( PDDI_PROJECTION_ORTHOGRAPHIC );

    p3d::pddi->SetProjectionMode( PDDI_PROJECTION_PERSPECTIVE );
    p3d::pddi->SetCullMode( PDDI_CULL_NONE );
    p3d::pddi->SetAlphaRef( 1 / 127.0f );

/*    unsigned i;
    for( i = 0; i < MAX_EFFECTS; i++)
    {
        if( effects[ i ] )
        {
            effects[i]->Update();
        }
    }
*/

    p3d::stack->Push();
    p3d::stack->LoadIdentity();

    p3d::stack->Translate( -0.5f, -0.5f / aspect, 0.5f );
    //p3d::stack->Translate( -0.0f, -0.0f / aspect, -1.5f );
}

//===========================================================================
// DisplayTeardown
//===========================================================================
// Description: does all the setup stuff for displaying a frame - clearing Z 
//              buffer, etc
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
void Core::DisplayTeardown()
{
    p3d::stack->Pop();
    p3d::pddi->PopState( PDDI_STATE_RENDER );
    p3d::pddi->PopState( PDDI_STATE_VIEW );
}

//===========================================================================
// Insert
//===========================================================================
// Description: Inserts a FeDrawable/tDrawable pair into the scene element 
//              list
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
unsigned Core::Insert( tDrawable* tDrawable, FeDrawable* feDrawable )
{
    for( unsigned i = 0; i < MAX_OBJECTS; i++ )
    {
        if( !scene[ i ].Used() )
        {
            scene[ i ].Init();
            scene[ i ].Used() = true;
            scene[ i ].SetTDrawable( tDrawable );
            scene[ i ].SetFeDrawable( feDrawable );

            //scene used is set to i + 1 unless it was already larger
            sceneUsed = ( i + 1 > sceneUsed ) ? i + 1 : sceneUsed;
            return i;
        }
    }
    rAssertMsg( false, "Core::Insert - filled up the scene element list" );
    return 0;
}

void Core::InstallFileHandler(tFileHandler* f)
{
   p3d::loadManager->AddHandler(f);
}

void Core::InstallChunkHandler(tChunkHandler* c)
{
   p3d::loadManager->GetP3DHandler()->AddHandler(c);
}

void Core::InstallDrawable(tDrawable* tDrawable)
{
   p3d::inventory->Store(tDrawable);
}

void Core::InstallEffect(Effect* effect)
{
   p3d::inventory->Store(effect);
}

//===========================================================================
// Render
//===========================================================================
// Description: renders an individual scene element depending on what type it 
//              is
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
void Core::Render( SceneElement* se, const SceneElementRenderProperties& properties )
{
    FeDrawable* feDrawable = se->GetFeDrawable();

    if( feDrawable->IsVisible() == true )
    {
        //don't draw the children
        if( feDrawable->IsClip() )
        {
            pddiRect rect;
            p3d::pddi->GetScissor( &rect );
            p3d::pddi->SetScissor( feDrawable->GetClipRect() );
        }

        p3d::stack->Push();

        float pos_x, pos_y;
        feDrawable->GetNormalizedPosition(pos_x,pos_y);

        //user transform is the transform the the user has set 
        rmt::Matrix* userTransform = feDrawable->GetMatrix();
        userTransform->m[3][0] += pos_x;
        userTransform->m[3][1] += pos_y;
        //d::stack->Translate(pos_x,pos_y,0);
        p3d::stack->Multiply( *userTransform );
        userTransform->m[3][0] -= pos_x;
        userTransform->m[3][1] -= pos_y;

        se->Display( properties );

        if( se->child ) 
        {
            //get the properties needed to draw the children
            SceneElementRenderProperties newProperties;
            newProperties = se->UpdateProperties( properties );
            this->Render( se->child, newProperties );
        }
        p3d::stack->Pop();
    }

    //IAN IMPROVE: HACK - do not render the siblings if the fedrawable is a page
    bool isPage = feDrawable->IsPage();
    if( isPage )
    {
        return;
    }

    if( se->sibling )   //IAN IMPROVE: I don't understand why we have to render the siblings if we render all the children
    {
        Render( se->sibling, properties );
    }
}

//===========================================================================
// Sort
//===========================================================================
// Description: sorts the list of scene elements for quick display
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
void Core::Sort()
{
    //erase the current contents of the sorted list
    for( unsigned int a = 0; a < MAX_OBJECTS; a++)
    {
        sorted[a] = NULL;
    }

    unsigned int i;
    SceneElement** insertPos = sorted;
    for( i = 0; i < sceneUsed; i++ )
    {
        SceneElement& se = scene[ i ];
        if( se.Used() )
        {
            FeDrawable* feDrawable = se.GetFeDrawable();
            if( ( feDrawable != NULL ) && ( feDrawable->IsPage() ) )
            //if( dynamic_cast< FePage* >( feDrawable ) != NULL )
            {
                *insertPos = &scene[i];
                insertPos++;
            }
        }
    }
}


//===========================================================================
// SceneElementRenderProperties
//===========================================================================
// Description: constructor : sets the color to white, and the alpha to opaque
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
SceneElementRenderProperties::SceneElementRenderProperties():
    colour( 255, 255, 255, 255 )
{
}

}


