/*===========================================================================
    visibilityanimation.hpp
    3-May-2000 Created by Stephen Lambie

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _VISIBILITYANIMATION_HPP_
#define _VISIBILITYANIMATION_HPP_

#include <p3d/anim/animate.hpp>
#include <p3d/anim/compositedrawable.hpp>
#include <p3d/scenegraph/scenegraph.hpp>

//**********************************************************
// Class tSimpleVisibilityAnimationController
//**********************************************************
class tSimpleVisibilityAnimationController : public tSimpleFrameController
{
public:
    tSimpleVisibilityAnimationController() : tSimpleFrameController() {}
    tSimpleVisibilityAnimationController(tSimpleVisibilityAnimationController* c) : tSimpleFrameController(c) {}

    bool ValidateAnimation(tAnimation* anim) = 0;

protected:
    ~tSimpleVisibilityAnimationController(){}
    void Update();

    virtual void RebindAnimationAndDrawable() = 0;
    virtual bool HasHirarchy(void) = 0;
    virtual void SetVisibilityOnNode(int nodeIndex, bool isVisible) = 0;
};

//**********************************************************
// Class tSceneGraphVisibilityAnimationController
//**********************************************************
class tSceneGraphVisibilityAnimationController : public tSimpleVisibilityAnimationController
{
public:
    tSceneGraphVisibilityAnimationController();
    tSceneGraphVisibilityAnimationController(tSceneGraphVisibilityAnimationController* c);
    
    virtual tFrameController* Clone(void);

    bool ValidateAnimation(tAnimation* anim);

    virtual void SetAnimation(tAnimation* anim);
    virtual void SetAnimation(tAnimation* anim, float startFrame, float blendFrames);

    void SetDrawable(Scenegraph::Scenegraph* scene);
    Scenegraph::Scenegraph* GetDrawable(void) { return pScene; }

protected:
    ~tSceneGraphVisibilityAnimationController();

    virtual void RebindAnimationAndDrawable();
    virtual bool HasHirarchy(void){ return( (pScene)?true:false ); }
    virtual void SetVisibilityOnNode(int nodeIndex, bool isVisible);

    Scenegraph::Scenegraph*    pScene;     // pointer to the simple scene that this controller animates
    Scenegraph::Visibility**   visNodes;   // array of pointers to Visibility nodes in the pScene that is animated
};

//**********************************************************
// Class tCompDrawVisibilityAnimationController
//**********************************************************
class tCompDrawVisibilityAnimationController : public tSimpleVisibilityAnimationController
{
public:
    tCompDrawVisibilityAnimationController();
    tCompDrawVisibilityAnimationController(tCompDrawVisibilityAnimationController* c);
    
    virtual tFrameController* Clone(void);

    bool ValidateAnimation(tAnimation* anim);

    virtual void SetAnimation(tAnimation* anim);
    virtual void SetAnimation(tAnimation* anim, float startFrame, float blendFrames);

    void SetDrawable(tCompositeDrawable* compDraw);
    tCompositeDrawable* GetDrawable(void) { return pCompDraw; }

protected:
    ~tCompDrawVisibilityAnimationController();

    virtual void RebindAnimationAndDrawable();
    virtual bool HasHirarchy(void){ return( (pCompDraw)?true:false ); }   
    virtual void SetVisibilityOnNode(int nodeIndex, bool isVisible);

    tCompositeDrawable*                       pCompDraw;   // pointer to the composite drawable that this controller animates
    tCompositeDrawable::DrawableElement**     visNodes;    // array of pointers to Visibility nodes in the composite drawable that is animated
};

#endif   // _VISIBILITYANIMATION_HPP_

