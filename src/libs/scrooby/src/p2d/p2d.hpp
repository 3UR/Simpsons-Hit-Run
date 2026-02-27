#ifndef _P2D_HPP_
#define _P2D_HPP_

#include <p3dtypes.hpp>
#include <entity.hpp>

class tChunkHandler;
class tDrawable;
class tFile;
class tFileHandler;
class tFont;
class tSprite;

class FeDrawable;
class FeText;
class FeSprite;
class FePolygon;

typedef unsigned short UnicodeChar;

namespace Pure2D
{

#ifdef P2D_UNICODE
//typedef wchar_t* String;
typedef unsigned long* String;
#else
typedef char* String;
#endif

const unsigned INVALID_HANDLE = 0xffffffff;

const unsigned MAX_FONTS    = 64;
const unsigned MAX_OBJECTS  = 2048;
const unsigned MAX_EFFECTS   = 64;

//class FileSystem;

// the client (front end) view of the Pure2D system
class Interface
{
protected:
/*   virtual void     LoadData(char* screenName) = 0;
   virtual void     FreeData(char* screenName) = 0;
*/

   virtual tFont*   LoadFontFromBuffer( char* name, char* buffer, const int bufferSize, float fontSize ) = 0;
   virtual void FreeFont( tFont* theFont ) = 0;

   // object managment
   virtual unsigned AddDummy( FeDrawable* ) = 0;
   virtual unsigned AddObject( FeDrawable* , char* name ) = 0;
   
   virtual unsigned AddSprite( FeSprite* pFeSprite, tSprite* pSprite ) = 0;
   virtual unsigned AddText( FeText* pFeText, tFont* pFont, UnicodeChar* string ) = 0;
   virtual unsigned AddPoly( FePolygon* pFePolygon, int nVer, rmt::Vector* ver, tColour* c ) = 0;

   virtual void     DeleteObject(unsigned handle) = 0;
   virtual void     ClearObjects(void) = 0;

   virtual void     Link(unsigned parent, unsigned child) = 0;

   // effect managment
   virtual unsigned AddEffect(char* name) = 0;
   virtual void     ApplyEffect(unsigned effectHandle, unsigned objectHandle) = 0;  // can apply effect to multiple objects
   virtual void     DeleteEffect(unsigned effectHandle) = 0;
   virtual void     ClearEffects(void) = 0;

   virtual void     SetTime(unsigned effectHandle, float time) = 0;
   virtual void     Play(unsigned effectHandle, bool playing) = 0;
};

/*class FileSystem
{
public:
   virtual tFile* Open(char* filename) = 0;

   virtual tFile* FindFirst(char* path) = 0;
   virtual tFile* FindNext(void) = 0;
};
*/
}

#endif
