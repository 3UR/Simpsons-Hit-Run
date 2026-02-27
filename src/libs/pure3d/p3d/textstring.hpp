//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _TEXTSTRING_HPP_
#define _TEXTSTRING_HPP_

#include <p3d/drawable.hpp>

class tFont;

class tTextString : public tDrawable
{
public:
    tTextString(tFont* font, P3D_UNICODE* string,  float s = 1.0f, bool del = false);
  
    void Display(void);

    void SetScale(float s) {scale = s;}
    float GetScale(void)   {return scale;}

    void SetFont(tFont*);
    tFont* GetFont(void);

    void SetString(P3D_UNICODE*, bool del = false);
    P3D_UNICODE* GetString(void);

    void SetColour(tColour colour);
    tColour GetColour(void);

    void GetBoundingBox(rmt::Box3D* box);

protected:
    ~tTextString();

    tFont* font;
    tColour colour;
    char* string;
    float scale;
    bool  del;
};

#endif

