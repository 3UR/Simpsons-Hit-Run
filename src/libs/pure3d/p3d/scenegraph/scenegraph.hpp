//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _SCENEGRAPH_HPP_
#define _SCENEGRAPH_HPP_

#include <p3d/drawable.hpp>
#include <p3d/loadmanager.hpp>
#include <p3d/displaylist.hpp>

class tDrawablePose;
class tCamera;
class tLightGroup;

namespace Scenegraph
{


// forward declarations
class Node;
    class Branch;
        class Transform;
        class Attachment;
        class Visibility;
    class Leaf;
        class Drawable;
        class Camera;
        class Light;

class Scenegraph;

class CustomDisplayCallback
{
public:
    virtual void Display(Drawable* node) = 0;
};

class Node : public tEntity
{
public:
    Node();

    enum Propagation
    {
        PROPAGATE_NONE,
        PROPAGATE_UP,
        PROPAGATE_DOWN
    };

    enum NodeFlags
    {
        BRANCH_TRANSFORM_DIRTY = 0x01,
        NODE_TRANSFORM_DIRTY   = 0x02,
        USER_WORLD_TRANSFORM   = 0x04
    };

    // abstract traversal functions
    virtual unsigned GetNumChildren(void) = 0;
    virtual Node*    GetChild(unsigned i) = 0;

    // parent control
    virtual void     SetParent(Node* p);
    virtual Node*    GetParent();

    // transforms
    virtual const rmt::Matrix& GetTransform(void);
    virtual const rmt::Matrix& GetWorldTransform(void);

    // standard traversals
    virtual void Display(const rmt::Matrix& view, DisplayList& list) = 0;
    virtual void CustomDisplay(const rmt::Matrix& view, CustomDisplayCallback*) = 0;
    virtual void UpdateTransform(const rmt::Matrix& parent) = 0;

    unsigned     GetFlags(void) {return flags;}
    virtual void SetFlags(bool set, unsigned flag, Propagation prop, bool stopIfSet);


protected:
    friend class Scenegraph;
    Node* parent;
    unsigned flags;

    ~Node();
};

class Branch : public Node
{
public:
    Branch(int initialChildren = 2);

    // traversal/child managment
    unsigned GetNumChildren(void);
    Node*    GetChild(unsigned i);

    virtual void     AddChild(Node* child);
    virtual void     RemoveChild(Node* child);

    // standard traversals
    void Display(const rmt::Matrix& view, DisplayList& list);
    void CustomDisplay(const rmt::Matrix& view, CustomDisplayCallback*);
    void UpdateTransform(const rmt::Matrix& parent);

    void SetFlags(bool set, unsigned flag, Propagation prop, bool stopIfSet);

protected:
    ~Branch();

    virtual void Resize(int n);

    int nChildren;
    int allocatedChildren;
    Node** children;
};

class Leaf : public Node
{
public:
    Leaf();

    // stubbed traversal functions
    unsigned GetNumChildren(void);
    Node*    GetChild(unsigned i);

    void Display(const rmt::Matrix&, DisplayList&);
    void CustomDisplay(const rmt::Matrix& view, CustomDisplayCallback*);
    void UpdateTransform(const rmt::Matrix& parent);

protected:
    ~Leaf();
    
};

class Transform : public Branch
{
public:
    Transform(int n);

    const rmt::Matrix& GetTransform(void);
    const rmt::Matrix& GetWorldTransform(void);

    rmt::Matrix* ModifyTransform(void);
    rmt::Matrix* ModifyWorldTransform(void);

    void SetTransform(const rmt::Matrix& t);
    void SetWorldTransform(const rmt::Matrix& w);

    // standard traversals
    void Display(const rmt::Matrix& view, DisplayList& list);
    void CustomDisplay(const rmt::Matrix& view, CustomDisplayCallback*);
    void UpdateTransform(const rmt::Matrix& parent);

protected:
    ~Transform();

    rmt::Matrix transform;
    rmt::Matrix world;

    void BuildWorld(void);
};

class Drawable : public Leaf
{
public:
    Drawable();
    Drawable(char* name);
    Drawable(tDrawable*);
    
    void       SetDrawable(tDrawable* d);
    // DisplayListDrawable interfaces
    tDrawable* GetDrawable(void) { return draw;}
    const rmt::Matrix* GetWorldMatrix(){return(&(GetWorldTransform()));}

    void  SetTranslucent(bool isTrans){isTranslucent = isTrans;}
    bool  IsTranslucent(void){return(isTranslucent);}

    void  SetSortOrder(float s){sortOrder = s;}
    float SortOrder(void){return(sortOrder);}

    // standard traversals
    void Display(const rmt::Matrix&, DisplayList& list );
    void CustomDisplay(const rmt::Matrix& view, CustomDisplayCallback*);

protected:
    ~Drawable();

    tDrawable* draw;
    bool isTranslucent;
    float sortOrder;
};

class Attachment : public Branch
{
public:
    Attachment();
    Attachment(tDrawablePose* pose, int initialChildren);

    void SetDrawablePose(tDrawablePose*);
    tDrawablePose* GetDrawablePose();

    void     AddChild(Node* child);

    Node* GetAttachment(int n);
    int   GetAttachmentJoint(int n);
    void  SetAttachment(int joint, Node* graft);
    void  RemoveAttachment(Node* graft);

    void EvaluateAttachments(void);

    // standard traversals
    void Display(const rmt::Matrix& view, DisplayList& list);
//    void CustomDisplay(const rmt::Matrix& view, CustomDisplayCallback*);
    void UpdateTransform(const rmt::Matrix& parent);

protected:
    ~Attachment();
    void Resize(int n);

    tDrawablePose* pose;
    int* jointIndices;
};

class Camera : public Leaf
{
public:
    Camera();
    Camera(tCamera*);

    tCamera* GetCamera();
    void SetCamera(tCamera*);
    
protected:
    ~Camera();

    tCamera* camera;

};

class LightGroup : public Leaf
{
public:
    LightGroup();
    LightGroup(tLightGroup*);

    tLightGroup* GetLights();
    void SetLights(tLightGroup*);

protected:
    ~LightGroup();

    tLightGroup* lights;

};

// the basic scenegraph
class Scenegraph : public tDrawable
{
public:
    Scenegraph();

    Node* GetRoot(void) { return root;}
    void  SetRoot(Node* n);

    Node* Find(const char* name);
    Node* Find(tUID uid){return Search(root, uid);}

//   tCamera*     GetCamera();
//   tLightGroup* GetLights();

    void Display(void);
    void CustomDisplay(CustomDisplayCallback*);
    void UpdateTransform(void);
    void ProcessShaders(ShaderCallback&);

    virtual void GetBoundingBox(rmt::Box3D* b)          { *b = boundingBox; }
    virtual void GetBoundingSphere(rmt::Sphere* s)          { boundingBox.GetBoundingSphere(s); }

protected:
    ~Scenegraph();

    Node* Search(Node* node, tUID name);

    Node* root;

    DisplayList translucentDrawables;

    //Hack
    friend class SceneGraphGenericLoader;

private:
    rmt::Box3D boundingBox;
};

class Visibility : public Branch
{
public:
    Visibility(int initialChildren = 1);

    // standard traversals
    void Display(const rmt::Matrix&, DisplayList& list);
    void CustomDisplay(const rmt::Matrix& view, CustomDisplayCallback*);

    void SetVisibility(bool vis){isVisible = vis;}

protected:
    ~Visibility(){}

    bool isVisible;
};

class SceneGraphGenericLoader
{
 public:
    tEntity* LoadObject(tChunkFile*, tEntityStore* store);
	Node*    LoadNode(tChunkFile*, tEntityStore* store, rmt::Matrix* Root = 0);

private:
    rmt::Box3D boundingBox;
};

class Loader : public tSimpleChunkHandler
{
 public:
    Loader();

 protected:
     ~Loader() {};
    tEntity* LoadObject(tChunkFile*, tEntityStore* store);
    Node*    LoadNode(tChunkFile*, tEntityStore* store);

    SceneGraphGenericLoader bothFormatsLoader;
};

//----------------------------------------------------------------
// SceneGraph Traversal objects
//----------------------------------------------------------------
class SceneGraphVisitor
{
public:
    virtual void Visit(Node* sceneGraphNode) = 0;
};

class DrawableCounter : public SceneGraphVisitor
{
public:
    DrawableCounter():nDrawables(0){}
    virtual void Visit(Node* sceneGraphNode);
    long GetCount(){return(nDrawables);}
protected:
    long nDrawables;
};

class TranslucentDrawableCounter : public DrawableCounter
{
public:
    virtual void Visit(Node* sceneGraphNode);
};

class SceneGraphTraversal
{
public:
    static void Traverse(Node* node, SceneGraphVisitor& vis);
};

class Iterator
{
public:
    Iterator(Scenegraph* scenegraph);
    virtual ~Iterator();

    virtual void  First(void);
    virtual void  Next(void);
    virtual bool  IsDone(void){return(m_isDone);}
    virtual Node* GetNode(void){return(m_current);}

protected:
    Scenegraph* m_scenegraph;
    Node*       m_current;
    bool        m_isDone;

    class SearchState
    {
    public:
        SearchState():sceneNode(NULL),currentChildIndex(0){}
        Node*    sceneNode;
        unsigned currentChildIndex;
    };

private:
    
    Iterator();
    
    long m_currentStackIndex;
    long m_searchStackSize;
    SearchState* m_searchStack; 
};

class LightGroupIterator : public Iterator
{
public:
    LightGroupIterator(Scenegraph* scenegraph):Iterator(scenegraph){}
    virtual void  First(void);
    virtual void  Next(void);
    LightGroup* GetLightGroup(void);
private:

};


} // end namespace

namespace sg = Scenegraph;

#endif

