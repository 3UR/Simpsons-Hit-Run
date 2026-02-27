//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/scenegraph/scenegraph.hpp>
#include <p3d/anim/drawablepose.hpp>
#include <p3d/anim/pose.hpp>
#include <constants/chunks.h>
#include <constants/chunkids.hpp>
#include <p3d/chunkfile.hpp>
#include <p3d/utility.hpp>
#include <p3d/matrixstack.hpp>
#include <p3d/camera.hpp>
#include <p3d/light.hpp>

#include <string.h>

namespace Scenegraph 
{
static rmt::Matrix identity(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);
static rmt::Matrix tmp;

// Scenegraph  -----------------------------------------------
Scenegraph::Scenegraph() : root(NULL)
{
    boundingBox.high.Set( -FLT_MAX, -FLT_MAX, -FLT_MAX );
    boundingBox.low.Set( FLT_MAX,  FLT_MAX,  FLT_MAX );
}

Scenegraph::~Scenegraph() 
{
    tRefCounted::Release(root);
}

void Scenegraph::SetRoot(Node* n) 
{ 
    tRefCounted::Assign(root,n);
}

void Scenegraph::UpdateTransform(void)
{
    if(root->flags & Node::BRANCH_TRANSFORM_DIRTY)
    {
        root->UpdateTransform(identity);
    }
}

void Scenegraph::Display(void) 
{ 
    UpdateTransform();

    p3d::stack->Push();
    rmt::Matrix tmp = *p3d::stack->GetMatrix();
    root->Display(tmp, translucentDrawables);
    translucentDrawables.Display();  
    p3d::stack->Pop();
}

void Scenegraph::CustomDisplay(CustomDisplayCallback* callback) 
{ 
    UpdateTransform();

    p3d::stack->Push();
    rmt::Matrix tmp = *p3d::stack->GetMatrix();
    root->CustomDisplay(tmp, callback);
    p3d::stack->Pop();
}

Node* Scenegraph::Find(const char* name)
{
    return Search(root, tEntity::MakeUID(name));
}

Node* Scenegraph::Search(Node* node, tUID uid)
{
    if(!node)
        return NULL;

    if(node->GetUID() == uid)
        return node;

    unsigned count = node->GetNumChildren();
    for(unsigned i = 0; i < count; i++)
    {
        Node* node2 = Search(node->GetChild(i), uid);

        if(node2)
            return node2;
    }

    return NULL;
}

class ShaderVisitor : public SceneGraphVisitor
{
public:
    ShaderVisitor(tDrawable::ShaderCallback& c) : callback(c) {}
    virtual void Visit(Node* sceneGraphNode)
    {
        Drawable* drawable = dynamic_cast<Drawable*>(sceneGraphNode);
        if(drawable)
        {
            if(drawable->GetDrawable())
            {
                drawable->GetDrawable()->ProcessShaders(callback);
            }
        }
    }
protected:
    tDrawable::ShaderCallback& callback;
};

void Scenegraph::ProcessShaders(ShaderCallback& callback)
{

    ShaderVisitor v(callback);
    SceneGraphTraversal::Traverse(root, v);
}

void DrawableCounter::Visit(Node* sceneGraphNode)
{
    Drawable* drawable = dynamic_cast<Drawable*>(sceneGraphNode);
    if(drawable)
    {
        nDrawables++;
    }
}
void TranslucentDrawableCounter::Visit(Node* sceneGraphNode)
{
    Drawable* drawable = dynamic_cast<Drawable*>(sceneGraphNode);
    if(drawable && drawable->IsTranslucent())
    {
        nDrawables++;
    }
}

void SceneGraphTraversal::Traverse(Node* node, SceneGraphVisitor& vis)
{
    unsigned count = node->GetNumChildren();

    // Visit the node
    vis.Visit(node);
    for(unsigned i = 0; i < count; i++)
    {
        Traverse(node->GetChild(i), vis);
    }
}

Iterator::Iterator(Scenegraph* scenegraph)
:
m_scenegraph(NULL),
m_current(NULL),
m_isDone(false),
m_currentStackIndex(0),
m_searchStackSize(0),
m_searchStack(NULL)
{
    m_searchStackSize = 200; // this is temporary
    m_searchStack = new SearchState[m_searchStackSize];
    tRefCounted::Assign(m_scenegraph, scenegraph);
}

Iterator::~Iterator()
{
    delete [] m_searchStack;
    tRefCounted::Release(m_scenegraph);
}

void Iterator::First ()
{
    m_currentStackIndex = 0;
    m_current = m_scenegraph->GetRoot();
    m_searchStack[m_currentStackIndex].sceneNode = m_current;
    m_searchStack[m_currentStackIndex].currentChildIndex = 0;
    m_isDone = false;
}

void Iterator::Next ()
{
    P3DASSERT(m_currentStackIndex<m_searchStackSize);

    Node* currentParentNode = m_searchStack[m_currentStackIndex].sceneNode;
    unsigned currentChildCount  = m_searchStack[m_currentStackIndex].currentChildIndex;

    //
    // Are we Done?
    //
    if((m_currentStackIndex == 0) && (currentChildCount > currentParentNode->GetNumChildren()-1))
    {
        this->m_isDone = true;
        return;
    }
    else if(currentChildCount<currentParentNode->GetNumChildren())
    {
        //
        // increment the parents child search
        //
        m_searchStack[m_currentStackIndex].currentChildIndex++;

        //
        // Get the Next Child and push its context
        //        
        m_currentStackIndex++; 
        m_searchStack[m_currentStackIndex].sceneNode = currentParentNode->GetChild(currentChildCount);
        m_searchStack[m_currentStackIndex].currentChildIndex = 0;
        m_current = m_searchStack[m_currentStackIndex].sceneNode;
    }
    else if(m_currentStackIndex>0)
    {
        //
        // No more children to find so pop
        //
        while(currentChildCount>=currentParentNode->GetNumChildren() && m_currentStackIndex>0)
        {
            m_searchStack[m_currentStackIndex].sceneNode = NULL;
            m_searchStack[m_currentStackIndex].currentChildIndex = 0;
            --m_currentStackIndex;
            currentParentNode = m_searchStack[m_currentStackIndex].sceneNode;
            currentChildCount  = m_searchStack[m_currentStackIndex].currentChildIndex;
        }

        if((m_currentStackIndex>=0) && (currentChildCount < currentParentNode->GetNumChildren()))
        {
            m_current = currentParentNode->GetChild(currentChildCount);
            m_searchStack[m_currentStackIndex].currentChildIndex++;
        }
    }
}

void  LightGroupIterator::First(void)
{
    // assume that the first node is NEVER a lightGroup
    Iterator::First();// initialize
    Next();
}

void  LightGroupIterator::Next(void)
{
    LightGroup* lightNode = NULL;
    do
    {
        Iterator::Next();
        lightNode = dynamic_cast<LightGroup*>(m_current);

    }while(lightNode == NULL && !m_isDone);
}

LightGroup* LightGroupIterator::GetLightGroup(void)
{
    return(dynamic_cast<LightGroup*>(m_current));
}


// Node  -----------------------------------------------

Node::Node() : parent(NULL), flags(NODE_TRANSFORM_DIRTY | BRANCH_TRANSFORM_DIRTY)
{
}

Node::~Node() 
{
}

void Node::SetParent(Node* p) 
{
    parent = p; // weak reference, dont ref count it
}

Node*  Node::GetParent() 
{
    return parent; 
}

const rmt::Matrix& Node::GetTransform(void) 
{
    if(parent)
        return parent->GetTransform();
    else
        return identity;
}

const rmt::Matrix& Node::GetWorldTransform(void) 
{
    flags &= ~NODE_TRANSFORM_DIRTY;

    if(parent)
        return parent->GetWorldTransform();
    else
        return identity;
}

void Node::SetFlags(bool set, unsigned f, Propagation prop, bool stopIfSet)
{
    if((prop == PROPAGATE_UP) && parent)
    {
        if(set)
        {
            if(stopIfSet && ((flags & f) == f))
                return;
        }
        else
        {
            if(stopIfSet && ((flags & f) == 0))
                return;
        }
        parent->SetFlags(set,f,prop, stopIfSet);
    }

    if(set)
    {
        flags |= f;
    }
    else
    {
      flags &= ~f;
    }

}

// Branch  -----------------------------------------------

Branch::Branch(int initialChildren) : nChildren(0), allocatedChildren(initialChildren), children(NULL) 
{

    if(allocatedChildren)
    {
        children = new Node*[allocatedChildren];
    }
    else
    {
//        p3d::printf("Warning : Scenegraph branch '%s' has no children. It should be removed.\n", this->GetName());
    }

    for(int i = 0; i < allocatedChildren; i++)
        children[i] = NULL;
}

Branch::~Branch() 
{
    for(int i = 0; i < allocatedChildren; i++)
    {
        if(children[i])
            children[i]->SetParent(NULL);

        tRefCounted::Release(children[i]);
    }

    delete [] children;
}

unsigned Branch::GetNumChildren(void) 
{ 
    return nChildren; 
}

Node* Branch::GetChild(unsigned index) 
{ 
    unsigned count = 0;
    for(int i = 0; i < allocatedChildren; i++)
    {
        if(children[i])
        {
            if(count == index)
                return children[i];
            else
                count++;
        }
    }
    return NULL;
}

void Branch::AddChild(Node* child) 
{
    if(!child)
        return;

    if(nChildren == allocatedChildren)
        Resize(allocatedChildren*2);

    for(int i = 0; i < allocatedChildren; i++)
    {
        if(!children[i])
        {
            tRefCounted::Assign(children[i], child);
            child->SetParent(this);
            nChildren++;
            return;
        }
    }
}

void Branch::RemoveChild(Node* child) 
{
    for(int i = 0; i < allocatedChildren; i++)
    {
        if(children[i])
        {
            if(children[i] == child)
            {
                children[i]->SetParent(NULL);
                tRefCounted::Release(children[i]);
                nChildren--;
                return;
            }
        }
    }
}

void Branch::Display(const rmt::Matrix& view, DisplayList& list) 
{
    for(int i = 0; i < allocatedChildren; i++)
        if(children[i]) children[i]->Display(view, list);
}

void Branch::CustomDisplay(const rmt::Matrix& view, CustomDisplayCallback* callback) 
{
    for(int i = 0; i < allocatedChildren; i++)
        if(children[i]) children[i]->CustomDisplay(view, callback);
}

void Branch::UpdateTransform(const rmt::Matrix& parent) 
{
    for(int i = 0; i < allocatedChildren; i++)
    {
        if(children[i]) 
        {
            if(children[i]->GetFlags() & BRANCH_TRANSFORM_DIRTY)
                children[i]->UpdateTransform(parent);
        }
    }
    flags &= ~BRANCH_TRANSFORM_DIRTY;
}

void Branch::SetFlags(bool set, unsigned f, Propagation prop, bool stopIfSet)
{
    if(prop == PROPAGATE_DOWN)
    {
        if(set)
        {
            if(stopIfSet && ((flags & f) == f))
                return;
        }
        else
        {
            if(stopIfSet && ((flags & f) == 0))
                return;
        }

        for(int i = 0; i < allocatedChildren; i++)
            if(children[i]) children[i]->SetFlags(set,f,prop, stopIfSet);
    }

    Node::SetFlags(set,f,prop, stopIfSet);
}


void Branch::Resize(int n)
{
    allocatedChildren = n;
    Node** newChildren= new Node*[allocatedChildren];
    memset(newChildren,0, sizeof(int)*allocatedChildren);
    memcpy(newChildren,children, sizeof(Node*)*nChildren);
    delete [] children;
    children = newChildren;
}


// Leaf  -----------------------------------------------

Leaf::Leaf() 
{
}

Leaf::~Leaf() 
{
}

unsigned Leaf::GetNumChildren(void) 
{
    return 0; 
}

Node* Leaf::GetChild(unsigned i) 
{
    return NULL;
}

void Leaf::UpdateTransform(const rmt::Matrix& parent)
{
}

void Leaf::Display(const rmt::Matrix& view, DisplayList& list)
{
}

void Leaf::CustomDisplay(const rmt::Matrix& view, CustomDisplayCallback*)
{
}


// Transform  -----------------------------------------------
Transform::Transform(int n) : Branch(n)
{
    transform.Identity();
    world.Identity();
}

Transform::~Transform() 
{
}

const rmt::Matrix& Transform::GetTransform(void)
{
    return transform;
}

const rmt::Matrix& Transform::GetWorldTransform(void)
{
    if((flags & NODE_TRANSFORM_DIRTY) && !(flags & USER_WORLD_TRANSFORM))
    {
        world.Mult(transform, parent->GetWorldTransform());
        flags &= ~NODE_TRANSFORM_DIRTY;
    }

    return world;
}

rmt::Matrix* Transform::ModifyTransform(void)
{
    SetFlags(true, NODE_TRANSFORM_DIRTY | BRANCH_TRANSFORM_DIRTY, PROPAGATE_DOWN, true);
    if(parent)
        parent->SetFlags(true, BRANCH_TRANSFORM_DIRTY, PROPAGATE_UP, true);
    return &transform;
}

rmt::Matrix* Transform::ModifyWorldTransform(void)
{
    return &world;
}

void Transform::SetTransform(const rmt::Matrix& t)
{
    transform = t;
    SetFlags(true, NODE_TRANSFORM_DIRTY | BRANCH_TRANSFORM_DIRTY, PROPAGATE_DOWN, true);
    if(parent)
        parent->SetFlags(true, BRANCH_TRANSFORM_DIRTY, PROPAGATE_UP, true);
}

void Transform::SetWorldTransform(const rmt::Matrix& w)
{
    world = w;
}

void Transform::Display(const rmt::Matrix& view, DisplayList& list) 
{
    tmp.Mult(world, view);
    p3d::stack->PushLoad(tmp);
    Branch::Display(view, list);
    p3d::stack->Pop();
}

void Transform::CustomDisplay(const rmt::Matrix& view, CustomDisplayCallback* callback) 
{
    tmp.Mult(world, view);
    p3d::stack->PushLoad(tmp);
    Branch::CustomDisplay(view, callback);
    p3d::stack->Pop();
}

void Transform::UpdateTransform(const rmt::Matrix& parent)
{
    if(!(flags & USER_WORLD_TRANSFORM))
        world.Mult(transform, parent);

    flags &= ~NODE_TRANSFORM_DIRTY;
    Branch::UpdateTransform(world);
}


// Drawable  -----------------------------------------------
Drawable::Drawable() : draw(NULL), isTranslucent(false), sortOrder(0.5f)
{
    // ...
}

Drawable::Drawable(char* name) : draw(NULL) 
{
    tDrawable* d = p3d::find<tDrawable>(name);
    tRefCounted::Assign(draw, d);
}

Drawable::Drawable(tDrawable* d) : draw(NULL) 
{
    tRefCounted::Assign(draw, d);
}

void Drawable::SetDrawable(tDrawable* d) 
{
        tRefCounted::Assign(draw, d);
}

Drawable::~Drawable() 
{
    tRefCounted::Release(draw);
}

void Drawable::Display(const rmt::Matrix&, DisplayList& list) 
{
    if(draw)
    {
        if(isTranslucent)
        {
            // Register this object to be drawn last
            list.Add(draw, NULL, sortOrder);
        }
        else
        {
            draw->Display();
        }
    }
}

void Drawable::CustomDisplay(const rmt::Matrix& mat, CustomDisplayCallback* callback) 
{
    callback->Display(this);
}


// Transform  -----------------------------------------------

Attachment::Attachment()
{
}

Attachment::Attachment(tDrawablePose* p, int initialChildren) : Branch(initialChildren), pose(NULL)
{
    tRefCounted::Assign(pose, p);
    jointIndices = new int[allocatedChildren];
}

void Attachment::SetDrawablePose(tDrawablePose* p)
{
    tRefCounted::Assign(pose, p);
}

Attachment::~Attachment()
{
    tRefCounted::Release(pose);
    delete [] jointIndices;
}

tDrawablePose* Attachment::GetDrawablePose()
{
    return pose;
}

Node* Attachment::GetAttachment(int n)
{
    return Branch::GetChild(n);
}

int   Attachment::GetAttachmentJoint(int n)
{
    int count = 0;
    for(int i = 0; i < allocatedChildren; i++)
    {
        if(children[i])
        {
            if(count == n)
                return jointIndices[i];
            else
                count++;
        }
    }
    return 0;
}

void Attachment::AddChild(Node* child)
{
    SetAttachment(~0, child);
}

void Attachment::SetAttachment(int joint, Node* graft)
{
    if(!graft)
        return;

    if(nChildren == allocatedChildren)
        Resize(allocatedChildren*2);

    for(int i = 0; i < allocatedChildren; i++)
    {
        if(!children[i])
        {
            Transform* transform = new Transform(1);
            transform->AddChild(graft);
            Node* t = transform;

            tRefCounted::Assign(children[i], t);
            children[i]->SetParent(this);
            jointIndices[i] = joint;
            nChildren++;
            return;
        }
    }
}

void Attachment::RemoveAttachment(Node* graft)
{
    RemoveChild(graft);
}

void Attachment::Display(const rmt::Matrix& view, DisplayList& list)
{
    if(pose)
    {
        pose->Display();
    }
    Branch::Display(view, list);

    // HACK : can't tell if attachment is evaluated or not, so force a recalc
    SetFlags(true, NODE_TRANSFORM_DIRTY | BRANCH_TRANSFORM_DIRTY, PROPAGATE_DOWN, true);
    if(parent)
        parent->SetFlags(true, BRANCH_TRANSFORM_DIRTY, PROPAGATE_UP, true);

}

void Attachment::EvaluateAttachments(void)
{
    if(!pose)
        return;

    pose->GetPose()->Evaluate();
    for(int i = 0; i < allocatedChildren; i++)
    {
        if(children[i])
        {
            Transform* t = dynamic_cast<Transform*>(children[i]);
            P3DASSERT(t);
            *t->ModifyTransform() = pose->GetPose()->GetJoint(jointIndices[i])->worldMatrix;
             
        }
    }
}

void Attachment::UpdateTransform(const rmt::Matrix& parent)
{
    EvaluateAttachments();
    Branch::UpdateTransform(parent);
}

void Attachment::Resize(int n)
{
    Branch::Resize(n);
    int* newJoints = new int[allocatedChildren];
    memcpy(newJoints,jointIndices, sizeof(int)*nChildren);
    delete [] jointIndices;
    jointIndices = newJoints;
}

// Visibility  -----------------------------------------------
Visibility::Visibility(int initialChildren):isVisible(true)
{

}

void Visibility::Display(const rmt::Matrix& m, DisplayList& list)
{
    if(isVisible)
    {
        Branch::Display(m, list);
    }
}

void Visibility::CustomDisplay(const rmt::Matrix& m, CustomDisplayCallback* callback)
{
    if(isVisible)
    {
        Branch::CustomDisplay(m, callback);
    }
}

// Camera  -----------------------------------------------
Camera::Camera() : camera(NULL)
{
}

Camera::Camera(tCamera* c) : camera(NULL)
{
    tRefCounted::Assign(camera, c);
}

Camera::~Camera()
{
    tRefCounted::Release(camera);
}

tCamera* Camera::GetCamera()
{
    return camera;
}

void Camera::SetCamera(tCamera* c)
{
    tRefCounted::Assign(camera, c);
}

LightGroup::LightGroup() : lights(NULL)
{
}

LightGroup::LightGroup(tLightGroup* l) : lights(NULL)
{
    tRefCounted::Assign(lights, l);
}

LightGroup::~LightGroup()
{
    tRefCounted::Release(lights);

}

tLightGroup* LightGroup::GetLights()
{
    return lights;
}

void LightGroup::SetLights(tLightGroup* l)
{   
    tRefCounted::Assign(lights, l);
}

// Loaders  -----------------------------------------------
static char buf[256];
static char buf2[256];
Loader::Loader() : tSimpleChunkHandler(Pure3D::SceneGraph::SCENEGRAPH)
{
}

tEntity* Loader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    return(bothFormatsLoader.LoadObject(f, store));
}


//
// These are essentially static functions.
//
tEntity* SceneGraphGenericLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    f->GetPString(buf);
    unsigned version = f->GetLong();
    P3DASSERT(version == 0);

    Scenegraph* scene = new Scenegraph;
    scene->SetName(buf);

    //
    // Clear bounding box for loader
    //
    boundingBox.high.Set( -FLT_MAX, -FLT_MAX, -FLT_MAX );
    boundingBox.low.Set( FLT_MAX,  FLT_MAX,  FLT_MAX );

	rmt::Matrix root;
	root.Identity();

    while(f->ChunksRemaining())
    {
        switch(f->BeginChunk())
        {
            case Pure3D::SceneGraph::ROOT:
                {
                    f->BeginChunk();
                    scene->SetRoot(LoadNode(f,store, &root));
                    f->EndChunk();
                }
                break;

            default:
                break;
        }
        f->EndChunk();
    }
    
    //
    // Scene Graph drawList setup
    //
    TranslucentDrawableCounter translucentDrawableCount;
    SceneGraphTraversal::Traverse(scene->GetRoot(), translucentDrawableCount);
    scene->translucentDrawables.SetSize(translucentDrawableCount.GetCount());

    //
    // Copy calculated bounding box to scenegraph
    //
    scene->boundingBox = boundingBox;
    
    return scene;
}

Node* SceneGraphGenericLoader::LoadNode(tChunkFile* f, tEntityStore* store, rmt::Matrix* Root)
{
    tDrawablePose* drawablePose;
    tDrawable* loadedDrawable;
    rmt::Box3D drawableBox;

    switch(f->GetCurrentID())
    {
        case Pure3D::SceneGraph::BRANCH:
        {
            f->GetPString(buf);
            unsigned n = f->GetLong();
            Branch* branch = new Branch(n);
            branch->SetName(buf);

            for(unsigned i = 0; i < n; i++)
            {
                f->BeginChunk();
                branch->AddChild(LoadNode(f,store, Root));
                f->EndChunk();
            }

            return branch;
        }
        
        case Pure3D::SceneGraph::TRANSFORM:
        {
            f->GetPString(buf);
            unsigned n = f->GetLong();
            Transform* branch = new Transform(n);
            branch->SetName(buf);
            f->GetData(branch->ModifyTransform(), 16, tFile::DWORD);
			rmt::Matrix newRoot;
			if(Root)
			{
				newRoot.Mult(*Root, branch->GetTransform());
			}
			else
			{
				newRoot.Identity();
			}
            for(unsigned i = 0; i < n; i++)
            {
                f->BeginChunk();
                branch->AddChild(LoadNode(f,store, &newRoot));
                f->EndChunk();
            }

            return branch;
        }

        case Pure3D::SceneGraph::ATTACHMENT:
        {
            f->GetPString(buf);
            f->GetPString(buf2);

            unsigned n = f->GetLong();
            drawablePose = p3d::find<tDrawablePose>(store,buf2);
            Attachment* graft = new Attachment(drawablePose, n);
            graft->SetName(buf);

            //
            // Modify bounding box to enclose box for pose
            //
            if( drawablePose != NULL )
            {
                drawablePose->GetBoundingBox( &drawableBox );
				if(Root)
				{
					Root->Transform(drawableBox.high, &drawableBox.high);
					Root->Transform(drawableBox.low, &drawableBox.low);
				}
                boundingBox.high.x = rmt::Max( (drawableBox.high.x), (boundingBox.high.x) );
                boundingBox.high.y = rmt::Max( (drawableBox.high.y), (boundingBox.high.y) );
                boundingBox.high.z = rmt::Max( (drawableBox.high.z), (boundingBox.high.z) );

                boundingBox.low.x = rmt::Min( (drawableBox.low.x), (boundingBox.low.x) );
                boundingBox.low.y = rmt::Min( (drawableBox.low.y), (boundingBox.low.y) );
                boundingBox.low.z = rmt::Min( (drawableBox.low.z), (boundingBox.low.z) );
            }

            for(unsigned i = 0; i < n; i++)
            {
                f->BeginChunk();
                long CurrentID = f->GetCurrentID();
                P3DASSERT(CurrentID == Pure3D::SceneGraph::ATTACHMENTPOINT);
                int joint = f->GetLong();

                f->BeginChunk();
                graft->SetAttachment(joint,LoadNode(f,store, Root));
                f->EndChunk();
                f->EndChunk();
            }

            return graft;
        }

        case Pure3D::SceneGraph::VISIBILITY:
        {
            f->GetPString(buf);
            unsigned n = f->GetLong();
            Visibility* branch = new Visibility(n);
            branch->SetName(buf);
            branch->SetVisibility(f->GetLong()?true:false);

            for(unsigned i = 0; i < n; i++)
            {
                f->BeginChunk();
                branch->AddChild(LoadNode(f,store, Root));
                f->EndChunk();
            }

            return branch;
        }

        case Pure3D::SceneGraph::DRAWABLE:
        {
            f->GetPString(buf);
            f->GetPString(buf2);
            bool isTranslucent = (f->GetLong()==1)?true:false;
            
            loadedDrawable = p3d::find<tDrawable>(store, buf2);
            Drawable* drawable = new Drawable(loadedDrawable);
            drawable->SetName(buf);
            drawable->SetTranslucent(isTranslucent);

            //
            // Modify bounding box to enclose box for drawable
            //
            if( loadedDrawable != NULL )
            {
                loadedDrawable->GetBoundingBox( &drawableBox );
				if(Root)
				{
					Root->Transform(drawableBox.high, &drawableBox.high);
					Root->Transform(drawableBox.low, &drawableBox.low);
				}
                boundingBox.high.x = rmt::Max( (drawableBox.high.x), (boundingBox.high.x) );
                boundingBox.high.y = rmt::Max( (drawableBox.high.y), (boundingBox.high.y) );
                boundingBox.high.z = rmt::Max( (drawableBox.high.z), (boundingBox.high.z) );

                boundingBox.low.x = rmt::Min( (drawableBox.low.x), (boundingBox.low.x) );
                boundingBox.low.y = rmt::Min( (drawableBox.low.y), (boundingBox.low.y) );
                boundingBox.low.z = rmt::Min( (drawableBox.low.z), (boundingBox.low.z) );
            }

            while(f->ChunksRemaining())
            {
                switch(f->BeginChunk())
                {
                    case Pure3D::SceneGraph::SORTORDER:
                        {
                            drawable->SetSortOrder(f->GetFloat());
                        }
                        break;

                    default:
                        break;
                }
                f->EndChunk();
            }

            return drawable;
        }

        case Pure3D::SceneGraph::CAMERA:
        {
            f->GetPString(buf);
            f->GetPString(buf2);
            
            Camera* camera = new Camera(p3d::find<tCamera>(store,buf2));
            camera->SetName(buf);
            return camera;
        }
        
        case Pure3D::SceneGraph::LIGHTGROUP:
        {
            f->GetPString(buf);
            f->GetPString(buf2);
            
            LightGroup* lightGroup = new LightGroup(p3d::find<tLightGroup>(store,buf2));
            lightGroup->SetName(buf);
            return lightGroup;
        }
    }
    return NULL;
}

} // end namespace Scenegraph

