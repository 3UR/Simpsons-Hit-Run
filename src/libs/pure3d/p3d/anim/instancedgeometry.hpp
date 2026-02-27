//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _INSTANCEDGEOMETRY_HPP_
#define _INSTANCEDGEOMETRY_HPP_

#include <p3d/drawable.hpp>

class tPose;
class tPolySkin;

class tInstancedGeometry : public tDrawable
{
public:
    tInstancedGeometry(tPolySkin*);
    ~tInstancedGeometry();

    virtual void Display(void);
    virtual void Flush(void);
    static void FlushAll(void);

    virtual void ProcessShaders(ShaderCallback&);

protected:
    tPolySkin* skin;
    tPose* pose;
    
    int curCount;
    int maxCount;

    tInstancedGeometry* prev;
    tInstancedGeometry* next;
};

#endif _INSTANCEDGEOMETRY_HPP_

