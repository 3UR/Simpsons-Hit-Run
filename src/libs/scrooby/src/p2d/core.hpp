#ifndef _CORE_HPP_
#define _CORE_HPP_

#include <p3dtypes.hpp>
#include <entity.hpp>
#include <utility/memory.h>

#include "p2d.hpp"

class FeSprite;
class FeText;
class FePolygon;

class tChunkHandler;
class tDrawable;
class tFile;
class tFileHandler;
class tFont;
class tSprite;

namespace Pure2D
{
   class Effect;
   class SceneElement;
   class SceneElementRenderProperties;

//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: This class is used to represent the 2D rendering core
//
// Constraints: None
//
//===========================================================================
class Core : public Interface
{
public: 
   Core();
   virtual ~Core();

public: // FE interface
//   void LoadData( char* screenName );
//   void FreeData( char* screenName );

   virtual tFont* LoadFontFromBuffer( char* name, char* buffer, const int bufferSize, float fontSize );
   virtual void FreeFont( tFont* theFont );

   // object managment
   unsigned AddDummy( FeDrawable* );
   void ReplaceDummy( FeDrawable* pFeSprite, unsigned coreHandle );
   unsigned AddObject( FeDrawable*, char* name );

   unsigned AddSprite( FeSprite* pFeSprite, tSprite* ptSprite );
   void ReplaceSprite( FeSprite* pFeSprite, unsigned coreHandle, tSprite* ptSprite );
   
   unsigned AddText( FeText* pFeText, tFont* pFont, UnicodeChar* string );
   void ReplaceTextObject( FeText* pFeText, tFont* pFont, UnicodeChar* string, unsigned oldTextObjectHandle );

   unsigned AddPoly( FePolygon*, int nVer, rmt::Vector* ver, tColour* c );
   void ReplacePoly( FePolygon*, unsigned i,int nVer, rmt::Vector* ver, tColour* c );
 
   unsigned AddPure3dObject( FeDrawable* pFeDrawable, tDrawable* ptDrawable );
   void ReplacePure3dObject( FeDrawable* pFeDrawable, unsigned coreHandle, tDrawable* ptDrawable );

   void DeleteObject(unsigned handle);
   void ClearObjects(void);

   void Link(unsigned parent, unsigned child);
   void Unlink(unsigned parent, unsigned child);

   int GetDisplayWidth();
   int GetDisplayHeight();

   // effect managment
   unsigned AddEffect(char* name);
   void     ApplyEffect(unsigned effectHandle, unsigned objectHandle);  // can apply effect to multiple objects

   void DeleteEffect(unsigned);
   void ClearEffects(void);

   void SetTime(unsigned effectHandle, float time);
   void Play(unsigned effectHandle, bool playing);

public: // rendering interface

    void SetTime(float time);

    void Display(void);
    void Display(float startLayer, float endLayer);
    void Display( tUID startLayer, tUID endLayer );    //NB - end layer not drawn
    void DrawBackground(void);
    void DrawForeground(void);

    void DisplaySetup();
    void DisplayTeardown();
private:

   // plug-ins
   void InstallFileHandler(tFileHandler*);
   void InstallChunkHandler(tChunkHandler*);
   void InstallDrawable(tDrawable* drawable);
   void InstallEffect(Effect* effect);

public:
   unsigned Insert( tDrawable*, FeDrawable* );
   void Render( SceneElement*, const SceneElementRenderProperties& properties );
   void Sort(void);

//   FileSystem* fileSystem;

   unsigned sceneUsed;
   SceneElement* scene;
   float mStartRenderLayer;
   float mEndRenderLayer;

   bool isSorted;
   SceneElement* sorted[MAX_OBJECTS];

   Effect*       effects[MAX_EFFECTS];
   bool          effectPlaying[MAX_EFFECTS];
};

//===========================================================================
//
// Description: This class is used to represent the special effects class
//
// Constraints: None
//
//===========================================================================
class Effect : public tEntity
{
public:
   virtual Effect* Clone(void) = 0;
   virtual void ApplyTo(tDrawable* object) = 0;

   virtual void SetTime(float time) = 0;
   virtual void Update(void) = 0;
};


//===========================================================================
//
// Description: This class is used to represent the rendering properties of
//              any single scene element
//
// Constraints: None
//
//===========================================================================
class SceneElementRenderProperties
{
public:
    tColour colour;
public:
    SceneElementRenderProperties();
};

//===========================================================================
//
// Description: This class is used to represent the individual scene elements
//
// Constraints: None
//
//===========================================================================
class SceneElement 
:
    public FeAllocatesMemory
{
private:
    bool isUsed;
    tDrawable* m_tDrawable;
    FeDrawable* m_feDrawable;
    int mType;

public:
    SceneElement();
    ~SceneElement();
    void Init();
    void SetTDrawable(tDrawable *d);
    tDrawable* GetTDrawable() { return m_tDrawable; }
    void SetFeDrawable( FeDrawable* d );
    FeDrawable* GetFeDrawable() { return m_feDrawable; }
    SceneElementRenderProperties UpdateProperties( const SceneElementRenderProperties& properties );

    //renders this scene element (no children) to the screen
    void Display( const SceneElementRenderProperties& properties );
    bool &Used() {return isUsed;}
    void Release();
    void SetType(int t);

public:
   bool isRoot;
   SceneElement* child;
   SceneElement* sibling;

protected:
};



}
#endif
