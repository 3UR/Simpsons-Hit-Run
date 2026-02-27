#ifndef _FONT_HPP
#define _FONT_HPP
/*===========================================================================
  font.hpp
  28-Feb-98 Created by Neall
  06-Nov-99 Ported to v12
 
  Copyright (c)1998, 1999 Radical Entertainment Inc.
  All rights reserved.
===========================================================================*/

#include <pddi/pdditype.hpp>

class pddiDevice;
class pddiRenderContext;
class pddiShader;

class pddiBaseFont
{
public:
    pddiBaseFont(pddiRenderContext* ctx);
    ~pddiBaseFont();

    void Print(const char* s, int x, int y, float z, pddiColour colour);

    static void SetScale(float xscale = 1.0f, float yscale = 1.0f);

private:
    pddiRenderContext* context;
    static float xscale;
    static float yscale;
};

#endif /* _FONT_HPP */