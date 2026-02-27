//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _SHADOW_HPP_
#define _SHADOW_HPP_
//---------------------------------------------------------------------
// Stencil buffer shadow support
//---------------------------------------------------------------------
/*

#include <radmath/radmath.hpp>
#include <p3d/entity.hpp>
#include <p3d/loadmanager.hpp>

#include <p3d/shadow/implementation.hpp>


//---------------------------------------------------------------------
// Shadow generator
//
// Controls shadow rendering
//---------------------------------------------------------------------
class tShadowGenerator : public tRefCounted
{
public:
    void Begin() { impl.Begin(); }
    void End()   { impl.End(); }

    void SetWashColour(tColour c) { impl.SetWashColour(c); }

    void PreRender() { impl.PreRender(); }

protected:
    tShadowGeneratorImpl impl;
};

//---------------------------------------------------------------------
// Shadow
//
// Abstract base class of all shadows
//
//---------------------------------------------------------------------
class tShadow : public tEntity
{
public:
    tShadow() {};

    // draw the shadow, must be called between tShadowGenerator::Begin and 
    // tShadowGenerator::Begin 
    virtual void Display(void) = 0;

    // set the current light for this shadow
    virtual void SetLight(bool IsPoint, const rmt::Vector& PosOrDir) = 0;
    
    // draw various bits of the shadow (for debugging)
    virtual void DrawNormals(void) = 0;
    virtual void DrawShell(void) = 0;
    
    // set the length of the extruded shadow volume, if zero the system will 
    // try to guess based on extruded bounding volume
    virtual void SetVolumeLength(float len) = 0;

protected:
    virtual ~tShadow() {};
};


//---------------------------------------------------------------------
// Shadow Mesh
//
// Shadow volume without vertex weighting (equivalent of tGeometry)
//---------------------------------------------------------------------
class tShadowMesh : public tShadow
{
public:
    tShadowMesh() {}

    virtual void Display() {impl.Display();}

    virtual void GetBoundingBox(rmt::Box3D* b)          { impl.GetBoundingBox(b);}
    virtual void GetBoundingSphere(rmt::Sphere* s)      { impl.GetBoundingSphere(s);}

    virtual void SetLight(bool isPoint, const rmt::Vector& posOrDir) { impl.SetLight(isPoint, posOrDir);}
    virtual void SetVolumeLength(float len) { impl.SetVolumeLength(len); }

    virtual void DrawNormals(void) { impl.DrawNormals();}
    virtual void DrawShell(void) { impl.DrawShell();}

protected:
    virtual ~tShadowMesh() {}
    tShadowMeshImpl impl;
    friend class tShadowMeshLoader;
};

//---------------------------------------------------------------------
// Shadow Mesh
//
// Shadow volume with vertex weighting (equivalent of tPolySkin)
//---------------------------------------------------------------------
class tShadowSkin : public tShadow
{
public:
    tShadowSkin() {}

    virtual void Display() { impl.Display(); }
    virtual void Display(tPose* p) {impl.Display(p); }

    virtual void GetBoundingBox(rmt::Box3D* b)          { impl.GetBoundingBox(b);}
    virtual void GetBoundingSphere(rmt::Sphere* s)      { impl.GetBoundingSphere(s);}

    virtual void SetLight(bool isPoint, const rmt::Vector& posOrDir) { impl.SetLight(isPoint, posOrDir);}
    virtual void SetVolumeLength(float len) { impl.SetVolumeLength(len); }

    virtual void DrawNormals(void) { impl.DrawNormals();}
    virtual void DrawShell(void) { impl.DrawShell();}

    virtual void SetPose(tPose* p) {impl.SetPose(p); }
    virtual tPose* GetPose(void) { return impl.GetPose(); }

    virtual tSkeleton* GetSkeleton() { return impl.GetSkeleton(); }

protected:
    virtual ~tShadowSkin() {}
    tShadowSkinImpl impl;
    friend class tShadowSkinLoader;
};

//---------------------------------------------------------------------
// Shadow Skin Loader
//---------------------------------------------------------------------
class tShadowSkinLoader : public tSimpleChunkHandler
{
public:
    tShadowSkinLoader();
    virtual tEntity* LoadObject(tChunkFile* chunk, tEntityStore* store);

protected:
    virtual tShadowSkin* Allocate();
};

//---------------------------------------------------------------------
// Shadow Skin Loader
//---------------------------------------------------------------------
class tShadowMeshLoader : public tSimpleChunkHandler
{
public:
    tShadowMeshLoader();
    virtual tEntity* LoadObject(tChunkFile* chunk, tEntityStore* store);

protected:
    virtual tShadowMesh* Allocate();
};
*/

#endif //_SHADOW_HPP_