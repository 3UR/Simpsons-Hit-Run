#ifndef _POLY_HPP_
#define _POLY_HPP_

#include <p3dtypes.hpp>
#include <drawable.hpp>
#include <pddi/pdditype.hpp>
#include "../utility/memory.h"

class tShader;

namespace Pure2D
{

class Poly 
: 
    public tDrawable,
    public FeAllocatesMemory
{
public:
   Poly( const int nVer, const rmt::Vector* verts, const tColour* colours);
   ~Poly();

   void Display(void);
   void SetColour( tColour c );

private:
   int nVer;

   //pddiVertex_CT* verts;
   rmt::Vector* m_Vectors;
   tColour*     m_Colours;
   tShader*     m_Shader;
   tColour      m_Color;        //this is a base color for the polygon
};

}

#endif
