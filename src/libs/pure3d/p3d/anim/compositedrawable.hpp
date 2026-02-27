/*===========================================================================
    compositedrawable.hpp
    30-Oct-00 Created by Liberty 

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _COMPOSITEDRAWABLE_HPP
#define _COMPOSITEDRAWABLE_HPP

#include <p3d/loadmanager.hpp>
#include <p3d/anim/drawablepose.hpp>
#include <p3d/anim/pose.hpp>
#include <p3d/displaylist.hpp>
#include <p3d/effects/effect.hpp>
#include <p3d/array.hpp>
 
class tSkeleton;
class tCompositeDrawable;
class DrawableElement;


class tCompositeDrawable : public tDrawablePose
{
public:
    tCompositeDrawable();
    tCompositeDrawable(int initialElementSize);
    virtual ~tCompositeDrawable();

    // Added an optional parameter to Clone - used to save memory as it isn't 
    // often necessary to save unique tPoses for each object - MikeR
    tCompositeDrawable* Clone(tPose* pose = NULL);
   
    inline virtual void Display()                   { tDrawablePose::Display();}
    virtual void Display(tPose* p);

    virtual void SetPose(tPose* p);

    virtual void GetBoundingBox(rmt::Box3D* b)          { *b = boundingBox; }
    virtual void GetBoundingSphere(rmt::Sphere* s)      { *s = boundingSphere; }

    void EnableBillboard(bool on) { billboard = on; }

    virtual void ProcessShaders(ShaderCallback&);

    //
    // Drawable Element Wrappers
    //
    class DrawableElement
    {
    public:
        enum { BASE_ELEMENT, PROP_ELEMENT, SKIN_ELEMENT, EFFECT_ELEMENT };

        DrawableElement() :
            visible(true), lockVisibility(false), isTranslucent(false), sortOrder(0.5f), pose(NULL) { type = BASE_ELEMENT; }
        virtual ~DrawableElement()                                          { tRefCounted::Release(pose); }
        virtual tUID GetUID() = 0;
        virtual void Display(void)                                          { if ((pose)&&(visible)) Draw(pose); }
        virtual void SetPose(tPose* p)                                      { tRefCounted::Assign(pose,p); } // refcounting
        virtual void SetVisibility(bool vis)                                { if (!lockVisibility) visible = vis; }
        virtual void SetLockVisibility(bool lock)                           { lockVisibility = lock; }
        bool IsVisible(void)                                                { return visible; }
        bool IsVisibilityLocked(void)                                       { return lockVisibility; }
        void SetIsTranslucent(bool trans)                                   { isTranslucent = trans; }
        bool IsTranslucent(void)                                            { return isTranslucent; }
        void SetSortOrder(float sort)                                       { sortOrder = sort; }
        float SortOrder(void)                                               { return sortOrder; }
        virtual void SetDrawable(tDrawable* drawable) = 0;
        virtual tDrawable* GetDrawable(void) = 0;
        const virtual rmt::Matrix* GetWorldMatrix() = 0;

        int GetType()                                                       { return type; }

    protected:
        int type;
        bool visible;
        bool lockVisibility;
        bool isTranslucent;
        float sortOrder;
        tPose* pose;

    private:
        virtual void Draw(tPose* p) = 0;
        friend class tCompositeDrawable;
    };

    class DrawablePropElement : public DrawableElement
    {
    public:
        DrawablePropElement() : DrawableElement(), prop(NULL), poseIndex(0) { type = PROP_ELEMENT; }
        virtual ~DrawablePropElement()                                      { tRefCounted::Release(prop); }
        virtual tUID GetUID()                                               { return prop->GetUID(); }
        virtual void SetDrawable(tDrawable* drawable);
        virtual tDrawable* GetDrawable()                                    { return prop; }
        virtual void SetPoseIndex(int index)                                { poseIndex = index; }
        int GetPoseIndex() const                                            { return poseIndex; }
        const virtual rmt::Matrix* GetWorldMatrix()                         { return &(pose->GetJoint(poseIndex))->worldMatrix; }

    protected:
        tDrawable* prop;
        int poseIndex;    // where to draw in the hierarchy

    private:
        virtual void Draw(tPose* p);      
        friend class tCompositeDrawable;
    };

    class DrawablePoseElement : public DrawableElement
    {
    public:
        DrawablePoseElement() : DrawableElement(), skin(NULL)   { type = SKIN_ELEMENT; }
        virtual ~DrawablePoseElement()                          { tRefCounted::Release(skin); }
        virtual tUID GetUID()                                   { return skin->GetUID(); }
        virtual void SetDrawable(tDrawable* drawable);
        virtual void SetPose(tPose* p);
        virtual tDrawable* GetDrawable()                        { return skin; }
        const virtual rmt::Matrix* GetWorldMatrix()             { return NULL; }

    protected:
        tDrawablePose* skin;

    private:
        virtual void Draw(tPose* p);      
        friend class tCompositeDrawable;
    };

    class DrawableEffectElement : public DrawableElement
    {
    public:
        DrawableEffectElement() : DrawableElement(), effect(NULL), poseIndex(0) { type = EFFECT_ELEMENT; }
        virtual ~DrawableEffectElement()                                        { tRefCounted::Release(effect); }
        virtual tUID GetUID()                                                   { return effect->GetUID(); }
        virtual void SetVisibility(bool vis)                                    { DrawableElement::SetVisibility(vis); if (effect) effect->SetVisibility(vis); }
        virtual void SetDrawable(tDrawable* drawable);
        virtual tDrawable* GetDrawable()                                        { return effect; }
        virtual void SetPoseIndex(int index)                                    { poseIndex = index; }
        int GetPoseIndex() const                                                { return poseIndex; }
        const virtual rmt::Matrix* GetWorldMatrix()                             { return &(pose->GetJoint(poseIndex))->worldMatrix; }

    protected:
        tEffect* effect;
        int poseIndex;    // where to draw in the hierarchy

    private:
        virtual void Draw(tPose* p);      
        friend class tCompositeDrawable;
    };

    //
    // Element searching... this was taken from the compoundMesh
    // This is used by the visibility animation controller to 
    // bind sceneNodes in a compound mesh for animating.
    //
    DrawableElement* FindNode(const char* name)                 { return FindNode(tEntity::MakeUID(name)); }
    DrawableElement* FindNode(tUID uid);
    int FindNodeIndex(const char* name)                         { return FindNodeIndex(tEntity::MakeUID(name)); }
    int FindNodeIndex(tUID uid);

    DrawablePropElement* AddProp(tDrawable*, int poseIndex);    // for props
    DrawablePoseElement* AddPose(tDrawablePose*);               // for skins
    DrawableEffectElement* AddEffect(tEffect*, int poseIndex);  // for effects
    void Remove(const char* name)                               { Remove(tEntity::MakeUID(name)); }
    void Remove(tUID uid);
    void RemoveByIndex(int i);

    int GetNumDrawableElement() const                           { return elements.RealSize( ); }
    DrawableElement* GetDrawableElement(int i)                  { return elements[i]; }

private:

    rmt::Box3D boundingBox;
    rmt::Sphere boundingSphere;
    //
    // These are the drawables. Since we support adding and removing 
    // this should this be a list.
    //
    tPtrDynamicArray<DrawableElement*> elements;
        
    //
    // Display list for translucent object
    //
    DisplayList translucentObjects;

    bool billboard;
       
    friend class tCompositeDrawableLoader;
};

//-------------------------------------------------------------------
class tCompositeDrawableLoader : public tSimpleChunkHandler
{
public:
    tCompositeDrawableLoader();
    tEntity* LoadObject(tChunkFile*, tEntityStore* store);
protected:
    ~tCompositeDrawableLoader() {};
};

#endif

