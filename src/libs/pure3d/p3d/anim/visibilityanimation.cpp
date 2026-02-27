/*===========================================================================
    visibilityanimation.cpp
    3-May-2000 Created by Stephen Lambie

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#include <p3d/anim/visibilityanimation.hpp>
#include <p3d/anim/channel.hpp>
#include <p3d/utility.hpp>
#include <p3d/error.hpp>

// Macro that returns false if x is even and
// true if x is odd.
#define ODD(x) ((x)&1)

//*****************************************************************************
// Class tSimpleVisibilityAnimationController
//*****************************************************************************
void tSimpleVisibilityAnimationController::Update()
{
    float                   fFrame = 0.0f;
    int                     i = 0;
    int                     iFrameIndex = 0;
    int                     iChannels = 0;
    int                     iChanFrames = 0;
    const tBoolChannel*     pVisChan = NULL;
    bool                    bSet = false;
    int                     iAnimIndex = 0;
    
    if (animation == NULL) return;
    
    // get current frame of animation
    fFrame = animation->MakeValidFrame(GetFrame(),minFrame,maxFrame,cycleMode);   

    // get number of nodes /channels to animate
    iChannels = animation->GetNumGroups();
    
    if(HasHirarchy())
    {
        for (i = 0; i < iChannels; i++)
        {
            // get pointer to channel
            pVisChan = animation->GetGroupByIndex(i)->GetBoolChannel(Pure3DAnimationChannels::Visibility::VISIBILITY_VIS);
            
            if(pVisChan)
            {
                bool visible;
                pVisChan->GetValue(fFrame, &visible);
                SetVisibilityOnNode(i, visible);
            }
        }
    }
}

//*****************************************************************************
// Class tSceneGraphVisibilityAnimationController
//*****************************************************************************
tSceneGraphVisibilityAnimationController::tSceneGraphVisibilityAnimationController() : 
    tSimpleVisibilityAnimationController(),
    pScene(NULL), 
    visNodes(NULL)
{
}

//-----------------------------------------------------------------------------
tSceneGraphVisibilityAnimationController::tSceneGraphVisibilityAnimationController(tSceneGraphVisibilityAnimationController* c) :
    tSimpleVisibilityAnimationController(c),
    pScene(NULL),
    visNodes(NULL)
{
    SetDrawable(c->GetDrawable());
}

//-----------------------------------------------------------------------------
tSceneGraphVisibilityAnimationController::~tSceneGraphVisibilityAnimationController()
{
    tRefCounted::Release(pScene);
    if(visNodes)
    {
        delete [] visNodes;
    }
    visNodes = NULL;
}

//-----------------------------------------------------------------------------
tFrameController* tSceneGraphVisibilityAnimationController::Clone(void)
{
    return new tSceneGraphVisibilityAnimationController(this);
}

//-----------------------------------------------------------------------------
bool tSceneGraphVisibilityAnimationController::ValidateAnimation(tAnimation* anim)
{
    return (anim->GetAnimationType() == Pure3DAnimationTypes::SCENEGRAPH_VISIBILITY_SVIS);
}

//-----------------------------------------------------------------------------
void tSceneGraphVisibilityAnimationController::SetAnimation(tAnimation* pVAnim)
{
    // Set the animation using base class method
    tSimpleFrameController::SetAnimation(pVAnim);

    RebindAnimationAndDrawable();
}

//-----------------------------------------------------------------------------
void tSceneGraphVisibilityAnimationController::SetAnimation(tAnimation* anim, float startFrame, float blendFrames)
{
    SetAnimation(anim);
    P3DASSERTMSG(startFrame==0.0f, "VisibilityAnimation wont work if you set the frame", "visibilityanimation.cpp");
}

//-----------------------------------------------------------------------------
void  tSceneGraphVisibilityAnimationController::SetDrawable(Scenegraph::Scenegraph* scene)
{
    tRefCounted::Assign(pScene, scene);

    RebindAnimationAndDrawable();
}

//-----------------------------------------------------------------------------
void tSceneGraphVisibilityAnimationController::RebindAnimationAndDrawable()
{  
    // delete old node data
    if(visNodes)
    {
        delete [] visNodes;
    }
    visNodes = NULL;

    if ((animation != NULL) && (pScene != NULL))
    {
        int i = 0;
        int iNumNodes = 0;

        // get number of nodes to find UIDs for
        iNumNodes = animation->GetNumGroups();
      
        // allocate array of pointers to scene nodes
        visNodes = new Scenegraph::Visibility*[iNumNodes];

        // store pointer to scene node that matches the stored UID
        for (i = 0; i < iNumNodes; i++)
        {
            Scenegraph::Visibility* visibilityNode = dynamic_cast<Scenegraph::Visibility*>(pScene->Find( animation->GetGroupByIndex(i)->GetUID() ) );
            P3DASSERT(visibilityNode);
            visNodes[i] = visibilityNode;
        }
    }
}

//-----------------------------------------------------------------------------
void tSceneGraphVisibilityAnimationController::SetVisibilityOnNode(int nodeIndex, bool isVisible)
{
    P3DASSERT(visNodes[nodeIndex]);
    visNodes[nodeIndex]->SetVisibility(isVisible);
}

//*****************************************************************************
// tCompDrawVisibilityAnimationController
//*****************************************************************************
tCompDrawVisibilityAnimationController::tCompDrawVisibilityAnimationController() : 
    tSimpleVisibilityAnimationController(),
    pCompDraw(NULL), 
    visNodes(NULL)
{
}

//-----------------------------------------------------------------------------   
tCompDrawVisibilityAnimationController::tCompDrawVisibilityAnimationController(tCompDrawVisibilityAnimationController* c) :
    tSimpleVisibilityAnimationController(c),
    pCompDraw(NULL), 
    visNodes(NULL)
{
    SetDrawable(c->GetDrawable());
}

//-----------------------------------------------------------------------------   
tCompDrawVisibilityAnimationController::~tCompDrawVisibilityAnimationController()
{
    tRefCounted::Release(pCompDraw);
    if(visNodes)
    {
        delete [] visNodes;
    }
    visNodes = NULL;
}

//-----------------------------------------------------------------------------   
tFrameController *tCompDrawVisibilityAnimationController::Clone(void)
{
    return new tCompDrawVisibilityAnimationController(this);
}

//-----------------------------------------------------------------------------
bool tCompDrawVisibilityAnimationController::ValidateAnimation(tAnimation* anim)
{
    return (anim->GetAnimationType() == Pure3DAnimationTypes::POSE_VISIBILITY_PVIS);
}

//-----------------------------------------------------------------------------   
void tCompDrawVisibilityAnimationController::SetAnimation(tAnimation* pVAnim)
{
    // Set the animation using base class method
    tSimpleFrameController::SetAnimation(pVAnim);

    RebindAnimationAndDrawable();
}

//-----------------------------------------------------------------------------   
void tCompDrawVisibilityAnimationController::SetAnimation(tAnimation* anim, float startFrame, float blendFrames)
{
    SetAnimation(anim);
    P3DASSERTMSG(startFrame==0.0f, "VisibilityAnimation wont work if you set the frame", "visibilityanimation.cpp");
}

//-----------------------------------------------------------------------------   
void tCompDrawVisibilityAnimationController::SetDrawable(tCompositeDrawable* compDraw)
{
    tRefCounted::Assign(pCompDraw, compDraw);

    RebindAnimationAndDrawable();
}

//-----------------------------------------------------------------------------   
void tCompDrawVisibilityAnimationController::RebindAnimationAndDrawable()
{
    // delete old node data
    if(visNodes)
    {
        delete [] visNodes;
    }
    visNodes = NULL;

    if ((animation != NULL) && (pCompDraw != NULL))
    {
        int i = 0;
        int iNumNodes = 0;

        // get number of nodes to find UIDs for
        iNumNodes = animation->GetNumGroups();
      
        // allocate array of pointers to scene nodes
        visNodes = new tCompositeDrawable::DrawableElement*[iNumNodes];

        // store pointer to scene node that matches the stored UID
        for (i = 0; i < iNumNodes; i++)
        {
            tCompositeDrawable::DrawableElement* drawable = pCompDraw->FindNode( animation->GetGroupByIndex(i)->GetUID() );
            P3DASSERT(drawable);
            visNodes[i] = drawable;
        }
    }
}

//-----------------------------------------------------------------------------   
void tCompDrawVisibilityAnimationController::SetVisibilityOnNode(int nodeIndex, bool isVisible)
{
    P3DASSERT(visNodes[nodeIndex]);
    visNodes[nodeIndex]->SetVisibility(isVisible);
}

