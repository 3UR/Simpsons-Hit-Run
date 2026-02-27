/*===========================================================================
    displaylist.hpp
    20-Jan-01 Created by Liberty 
    Mini Display List... used by the SceneGraph and the tCompositeDrawable
    for rendering translucent object last

    Copyright (c)2001 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#ifndef _DISPLAYLIST_HPP_
#define _DISPLAYLIST_HPP_

#include <p3d/drawable.hpp>

class DisplayList
{
public:
    DisplayList();
    ~DisplayList();
    void Display(void);

    void Add(tDrawable* object, const rmt::Matrix* worldMatrix = NULL, float sortOrder = 0.5f);
    unsigned GetCount(void) { return nDrawablesEndPos; }
    tDrawable* GetSorted(unsigned i, rmt::Matrix* matrix);

    void Purge();
    void SetSize(int size);

private:
    struct Drawable
    {
        float sortPosition;
        float zPosition;
        rmt::Matrix objectToView;
        tDrawable* draw;
    };

    static int ZSortCompare(const void* drawable1, const void* drawable2);
    void Sort(void);
    unsigned nDrawables;
    unsigned nDrawablesEndPos;
    Drawable* drawables;
    Drawable** toSort;

    float sortRange;

    bool sorted;
};


#endif

