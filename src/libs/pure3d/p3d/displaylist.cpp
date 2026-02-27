/*===========================================================================
    displaylist.cpp
    20-Jan-01 Created by Liberty 
    Mini Display List... used by the SceneGraph and the tCompositeDrawable
    for rendering translucent object last

    Copyright (c)2001 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#include <p3d/displaylist.hpp>
#include <p3d/matrixstack.hpp>

#include <p3d/utility.hpp>
#include <p3d/matrixstack.hpp>

#include <string.h>
#include <stdlib.h> 

DisplayList::DisplayList() 
: 
nDrawables(0), 
nDrawablesEndPos(0),
drawables(NULL),
toSort(NULL),
sortRange(1.0f),
sorted(true)
{
}

DisplayList::~DisplayList()
{
    delete [] drawables;
    delete [] toSort;
}

void DisplayList::Display(void)
{   
    p3d::stack->Push();

    Sort();

    for(unsigned i = 0; i < nDrawablesEndPos; i++)
    {
        P3DASSERT(toSort[i]);
        p3d::stack->Load(toSort[i]->objectToView);
        toSort[i]->draw->Display();
    }

    Purge(); 
    p3d::stack->Pop();
}

void DisplayList::Add(tDrawable* object, const rmt::Matrix* worldMatrix, float sortPosition)
{
    sorted = false;

    if(nDrawablesEndPos<nDrawables)
    {
        // grap a pointer to the next drawable for sortin
        toSort[nDrawablesEndPos] = &drawables[nDrawablesEndPos];

        // store away the current modelview matrix
        if(worldMatrix)
        {
            rmt::Matrix tmp;
            tmp.Mult(*worldMatrix, *p3d::stack->GetMatrix());
            drawables[nDrawablesEndPos].objectToView = tmp;
        }
        else
        {
            drawables[nDrawablesEndPos].objectToView = *p3d::stack->GetMatrix();
        }

        
        drawables[nDrawablesEndPos].draw = object;
        drawables[nDrawablesEndPos].draw->AddRef();
    
        // calculate the z position in camera soace for sorting
        rmt::Sphere sphere;
        rmt::Vector cameraPosition;
        object->GetBoundingSphere(&sphere);
        drawables[nDrawablesEndPos].objectToView.Transform(sphere.centre, &cameraPosition);
        drawables[nDrawablesEndPos].zPosition = cameraPosition.z;
        drawables[nDrawablesEndPos].sortPosition = sortPosition;

        nDrawablesEndPos++;
    }
    else
    {
        p3d::print("warning : DisplayList::Add, too many drawables submitted\n");
    }
}

tDrawable* DisplayList::GetSorted(unsigned i, rmt::Matrix* matrix)
{
    P3DASSERT(i < nDrawablesEndPos);
    
    if(!sorted)
        Sort();

    P3DASSERT(toSort[i]);

    *matrix = toSort[i]->objectToView;
    return toSort[i]->draw;
}

void DisplayList::Purge()
{
    for(unsigned i = 0; i < nDrawablesEndPos; i++)
    {
        drawables[i].draw->Release();
        toSort[i] = NULL;
    }
    nDrawablesEndPos = 0;
    sorted = true;
}

void DisplayList::SetSize(int size)
{
    Purge();

    if(drawables)
    {
        delete [] drawables;
        delete [] toSort;
    }

    nDrawables = size;

    if(nDrawables )
    {
        drawables = new Drawable[nDrawables];
        toSort = new Drawable*[nDrawables];
    }
    else
    {
        drawables = NULL;
        toSort = NULL;
    }
}

void DisplayList::Sort(void)
{
   if(sorted)
        return;

    qsort(toSort, nDrawablesEndPos, sizeof(Drawable*), DisplayList::ZSortCompare);
    sorted = true;
}

// < 0 elem1 less than elem2 
// 0 elem1 equivalent to elem2 
// > 0 elem1 greater than elem2 
// 
// we want to sort the list from far -> new, so we actually invert the comparison
int DisplayList::ZSortCompare(const void* drawable1, const void* drawable2)
{
    Drawable** pdraw1 = (Drawable**)(drawable1);
    Drawable** pdraw2 = (Drawable**)(drawable2);
    Drawable* draw1 = (Drawable*)(*pdraw1);
    Drawable* draw2 = (Drawable*)(*pdraw2);

    if( draw1->sortPosition != draw2->sortPosition )
    {
        return (draw1->sortPosition < draw2->sortPosition) ? 1 : -1;
    }

    if( draw1->zPosition == draw2->zPosition )
    {
        return 0;
    }

    return (draw1->zPosition < draw2->zPosition ) ? 1 : -1;
}

