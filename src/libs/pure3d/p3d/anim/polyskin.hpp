/*===========================================================================
    polyskin.hpp
    30-Nov-99 Created by Neall

    Copyright (c)1999 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _POLYSKIN_HPP
#define _POLYSKIN_HPP

#include <p3d/anim/drawablepose.hpp>
#include <p3d/loadmanager.hpp>
#include <p3d/anim/expressionoffsets.hpp>
#include <p3d/array.hpp>

class tPose;
class tSkeleton;
class tPrimGroup;
class tPrimGroupSkinnedStreamed;
class tVertexList;
class tPrimGroupLoader;

class tPolySkin : public tDrawablePose
{
public:
    tPolySkin(int nPrimGroups=0);

    virtual void Display()  { tDrawablePose::Display(); }
    virtual void Display(tPose* p);
    virtual void DisplayInstanced(tPose* p, unsigned count);

    virtual void GetBoundingBox(rmt::Box3D* b)      { *b = boundingBox; }
    virtual void GetBoundingSphere(rmt::Sphere* s)  { *s = boundingSphere; }

    virtual void ProcessShaders(ShaderCallback&);

    int GetNumPrimGroups()                          { return primGroup.Size(); }
    tPrimGroup* GetPrimGroupSkinned(int i)   { return primGroup[i]; }
    tExpressionOffsets* GetExpressionOffsets()      { return exprAnimOffsets; }
    
    bool IsDeformable()    const { return exprAnimOffsets != NULL; }

protected:
    virtual ~tPolySkin();

    rmt::Matrix* boneMatrix; // restpose -> joint -> world space matrices

    tPtrArray<tPrimGroup*> primGroup;
    tExpressionOffsets* exprAnimOffsets;  // stores all tOffsetLists for this skin

    rmt::Sphere boundingSphere;
    rmt::Box3D  boundingBox;

    friend class tPolySkinLoader;
};

//-------------------------------------------------------------------
class tPolySkinLoader : public tSimpleChunkHandler
{
public:
    tPolySkinLoader();

    virtual tEntity* LoadObject(tChunkFile* chunk, tEntityStore* store);
    virtual void SetOptimise(bool b) { optimise = b;}

protected:
    tExpressionOffsetsLoader offsetLoader;

    bool optimise;

};

#endif /* _POLYSKIN_HPP */
