/*===========================================================================
    compositedrawable.hpp
    30-Oct-00 Created by Liberty 

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#include <p3d/billboardobject.hpp>
#include <p3d/anim/compositedrawable.hpp>
#include <p3d/anim/skeleton.hpp>
#include <p3d/anim/polyskin.hpp>
#include <p3d/anim/pose.hpp>
#include <p3d/anim/skeleton.hpp>
#include <p3d/utility.hpp>
#include <p3d/matrixstack.hpp>
#include <p3d/view.hpp>
#include <p3d/camera.hpp>
#include <p3d/chunkfile.hpp>
#include <p3d/file.hpp>
#include <constants/chunks.h>
#include <radmath/util.hpp>
#include <float.h>
#include <string.h>

const int resizeElementSize = 2;
const int defaultInitialElementSize = 5;

tCompositeDrawable::tCompositeDrawable() :
    billboard(false)
{
    elements.SetSize( defaultInitialElementSize );

    boundingBox.low.Set(0,0,0);
    boundingBox.high.Set(1,1,1);
    boundingSphere.centre.Set(0,0,0);
    boundingSphere.radius = 1;
}

tCompositeDrawable::tCompositeDrawable(int initialElementSize) :
    billboard(false)
{
    elements.SetSize( initialElementSize );

    boundingBox.low.Set(0,0,0);
    boundingBox.high.Set(1,1,1);
    boundingSphere.centre.Set(0,0,0);
    boundingSphere.radius = 1;

    translucentObjects.SetSize(initialElementSize);
}

tCompositeDrawable::~tCompositeDrawable()
{
    for( unsigned i=0; i< elements.Size( ); i++)
    {
        if( elements[ i ] )
            delete elements[i];
    }

    if(skeleton)
    {
        skeleton->Release();
    }
}

tCompositeDrawable* tCompositeDrawable::Clone(tPose* pose)
{
    tCompositeDrawable* compDraw = new tCompositeDrawable( elements.Size( ) );
    compDraw->SetNameObject(GetNameObject());
    if ( pose == NULL )
        compDraw->SetPose(new tPose(skeleton));
    else
        compDraw->SetPose(pose);

    compDraw->EnableBillboard(billboard);

    //
    // Attach the skeleton and release the old one
    //
    tRefCounted::Assign(compDraw->skeleton,skeleton);
            
    //
    // Add Drawable Elements
    //
    for( unsigned i=0; i< elements.Size( ); i++)
    {
        if( elements[ i ] ){
            switch (elements[i]->GetType())
            {
                case DrawableElement::PROP_ELEMENT:
                {
                    DrawablePropElement* prop = (DrawablePropElement*)(elements[i]);
                    DrawablePropElement* newProp = compDraw->AddProp(prop->GetDrawable(), prop->poseIndex);
                    newProp->SetIsTranslucent(prop->IsTranslucent());
                    newProp->SetSortOrder(prop->SortOrder());
                    break;
                }
                case DrawableElement::SKIN_ELEMENT:
                {
                    DrawablePoseElement* pose = (DrawablePoseElement*)(elements[i]);
                    DrawablePoseElement* newPose = compDraw->AddPose((tDrawablePose*)pose->GetDrawable());
                    newPose->SetIsTranslucent(pose->IsTranslucent());
                    newPose->SetSortOrder(pose->SortOrder());
                    break;
                }
                case DrawableElement::EFFECT_ELEMENT:
                {
                    DrawableEffectElement* effect = (DrawableEffectElement*)(elements[i]);
                    DrawableEffectElement* newEffect = compDraw->AddEffect((tEffect*)effect->GetDrawable(), effect->poseIndex);
                    newEffect->SetIsTranslucent(effect->IsTranslucent());
                    newEffect->SetSortOrder(effect->SortOrder());
                    break;
                }
                default:
                {
                    P3DASSERTMSG(0, "Invalid DrawableElement", "tCompositeDrawable::Copy()");
                    break;
                }
            }
        }
    }

    //
    // Copy the Bounding Boxes
    //
    compDraw->boundingBox.high = boundingBox.high;
    compDraw->boundingBox.low = boundingBox.low;
    compDraw->boundingSphere.centre = boundingSphere.centre;
    compDraw->boundingSphere.radius = boundingSphere.radius;

    //
    // Size the Display list
    //
    compDraw->translucentObjects.SetSize( elements.Size( ) );

    return compDraw;
}

//
// Effectively all we are doing here is 
// maintaining a collection of different drawables
// that can display themselves. The hierarchical 
// drawables used to take a "tpose" to draw... 
//
void tCompositeDrawable::Display(tPose* p)
{
    if (billboard)
    {
        rmt::Matrix worldMatrix = *(p3d::context->GetWorldMatrix());
        rmt::Matrix cameraMatrix = p3d::context->GetView()->GetCamera()->GetCameraToWorldMatrix();

        rmt::Vector normal;
        normal.Sub(cameraMatrix.Row(3),worldMatrix.Row(3));
        normal.NormalizeSafe();

        rmt::Matrix transform;
        transform.Identity();
        transform.FillHeadingXZ(normal);

        p3d::stack->PushMultiply(transform);
    }

    //
    // Transform the pose joints into world space
    //
    p->Evaluate();
    
    //
    // Display elements
    //
    for( unsigned i=0; i< elements.Size( ); i++)
    {
        if(elements[i])
        {
            elements[i]->SetPose(p);
            if (elements[i]->IsVisible())
            {
                if (elements[i]->IsTranslucent())
                {
                    //if( BillboardQuadManager::sEnabled )    
                    //    elements[i]->Display();
                    //else 
                        translucentObjects.Add(elements[i]->GetDrawable(), elements[i]->GetWorldMatrix(),elements[i]->SortOrder());            
                }
                else
                {
                    elements[i]->Display();
                }
            }
        }
    }

    //if( !BillboardQuadManager::sEnabled )
        translucentObjects.Display();

    if (billboard)
    {
        p3d::stack->Pop();
    }
}

void tCompositeDrawable::ProcessShaders(ShaderCallback& callback)
{
    for( unsigned i=0; i< elements.Size( ); i++)
    {
        if(elements[i])
        {
            elements[i]->GetDrawable()->ProcessShaders(callback);
        }
    }
}

void tCompositeDrawable::SetPose(tPose* p)
{
    tRefCounted::Assign(pose, p);
    for( unsigned i=0; i< elements.Size( ); i++)
    {
        if(elements[i])
        {
            elements[i]->SetPose(p);
        }
    }
}

tCompositeDrawable::DrawableElement* tCompositeDrawable::FindNode(tUID uid)
{
    for(unsigned i=0; i< elements.Size( ); i++)
    {
        if(elements[ i ] && elements[i]->GetUID() == uid)
        {
            return (elements[i]);
        }
    }
    //
    // Found nothing so return NULL
    //
    return (0);
}

int tCompositeDrawable::FindNodeIndex(tUID uid)
{
    for( unsigned i=0; i< elements.Size( ); i++)
    {
        if( elements[ i ] && elements[i]->GetUID() == uid)
        {
            return i;
        }
    }
    //
    // Found nothing so return NULL
    //
    return (-1);
}

//
// For props
//
tCompositeDrawable::DrawablePropElement* tCompositeDrawable::AddProp(tDrawable* prop, int poseIndex) 
{
    DrawablePropElement* dElement = 0;

    dElement = new DrawablePropElement;

    if(dElement)
    {
        dElement->SetVisibility(true);
        dElement->SetDrawable(prop);
        dElement->SetPoseIndex(poseIndex);
        elements.Add( dElement );
    }

    return(dElement);
}

//
// For effects
//
tCompositeDrawable::DrawableEffectElement* tCompositeDrawable::AddEffect(tEffect* effect, int poseIndex) 
{
    DrawableEffectElement* dElement = 0;

    dElement = new DrawableEffectElement;

    if(dElement)
    {
        dElement->SetDrawable(effect);
        dElement->SetVisibility(true);
        dElement->SetPoseIndex(poseIndex);
        elements.Add( dElement );
    }

    return(dElement);
}

//
// For poly skin props 
//
tCompositeDrawable::DrawablePoseElement* tCompositeDrawable::AddPose(tDrawablePose* pose) 
{
    DrawablePoseElement* dElement = 0;

    dElement = new DrawablePoseElement;

    if(dElement)
    {
        dElement->SetVisibility(true);
        dElement->SetDrawable(pose);
        elements.Add( dElement );
    }

    return(dElement);
}

void tCompositeDrawable::Remove(tUID uid)
{
    for(unsigned i=0; i< elements.Size( ); i++)
    {
        if( elements[ i ] && elements[i]->GetUID() == uid)
        {
            DrawableElement* elem = elements[i];
            elements[i] = NULL;  // No longer owned here
            delete elem;
        }
    }
}

void tCompositeDrawable::RemoveByIndex(int i)
{
    if (((unsigned)i < elements.Size( ) )&&(i>0)  && elements[ i ] )
    {
        delete elements[i];
        elements[i] = NULL;
    }
}

//-------------------------------------------------------
// Drawable Prop Element
//-------------------------------------------------------
void tCompositeDrawable::DrawablePropElement::SetDrawable(tDrawable* drawable)
{
    P3DASSERTMSG(drawable, "No drawable passed", "tCompositeDrawable::DrawablePropElement()");
    tRefCounted::Assign(prop,drawable);
}

void tCompositeDrawable::DrawablePropElement::Draw(tPose* pose)
{
    P3DASSERTMSG(prop, "No prop attached", "tCompositeDrawable::DrawPropElement()");

    p3d::stack->PushMultiply( (pose->GetJoint(poseIndex))->worldMatrix );
    prop->Display();
    p3d::stack->Pop();
}

//-------------------------------------------------------
// Drawable Pose Element
//-------------------------------------------------------
void tCompositeDrawable::DrawablePoseElement::SetDrawable(tDrawable* drawable)
{
    P3DASSERTMSG(drawable, "No drawable passed", "tCompositeDrawable::DrawablePoseElement()");
    
    tDrawablePose* newSkin = dynamic_cast<tDrawablePose*>(drawable);

    P3DASSERTMSG(newSkin, "Drawable not a Pose", "tCompositeDrawable::DrawablePoseElement()");

    tRefCounted::Assign(skin,newSkin);
}

void tCompositeDrawable::DrawablePoseElement::SetPose(tPose* p)
{ 
    DrawableElement::SetPose(p); 
    skin->SetPose(p); 
} 

void tCompositeDrawable::DrawablePoseElement::Draw(tPose* pose)
{
    P3DASSERTMSG(skin, "No skin attached", "tCompositeDrawable::DrawPoseElement()");

    skin->Display(pose);
}

//-------------------------------------------------------
// Drawable Effect Element
//-------------------------------------------------------
void tCompositeDrawable::DrawableEffectElement::SetDrawable(tDrawable* drawable)
{
    P3DASSERTMSG(drawable, "No drawable passed", "tCompositeDrawable::DrawableEffectElement()");
    
    tEffect* newEffect = dynamic_cast<tEffect*>(drawable);

    P3DASSERTMSG(newEffect, "Drawable not a Effect", "tCompositeDrawable::DrawableEffectElement()");

    tRefCounted::Assign(effect,newEffect);
    effect->SetVisibility(visible);
}

void tCompositeDrawable::DrawableEffectElement::Draw(tPose* pose)
{
    P3DASSERTMSG(effect, "No effect attached", "tCompositeDrawable::DrawEffectElement()");

    p3d::stack->PushMultiply( (pose->GetJoint(poseIndex))->worldMatrix );
    effect->Display();
    p3d::stack->Pop();
}

//-------------------------------------------------------------------
tCompositeDrawableLoader::tCompositeDrawableLoader() : tSimpleChunkHandler(P3D_COMPOSITE_DRAWABLE)
{
}

tEntity* tCompositeDrawableLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{  
    char name[128];
    f->GetPString(name);
    char skel[128];
    f->GetPString(skel);

    //
    // Have all that we need so make a new tCompositeDrawable
    //
    tCompositeDrawable* compositeDraw = new tCompositeDrawable(5);

    compositeDraw->SetName(name);
    
    //
    // Find Skeleton
    //
    tSkeleton* skeleton = p3d::find<tSkeleton>(store, skel);
    if(skeleton)
    {
        compositeDraw->skeleton = skeleton;
        compositeDraw->skeleton->AddRef();
        compositeDraw->SetPose(skeleton->NewPose());
    }

    long numTranslucentElements = 0;

    //
    // Clear Bounding Box
    //
    compositeDraw->boundingBox.high.Set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    compositeDraw->boundingBox.low.Set( FLT_MAX,  FLT_MAX,  FLT_MAX);   

    while(f->ChunksRemaining())
    {
        f->BeginChunk();
        if(f->GetCurrentID() == P3D_COMPOSITE_DRAWABLE_SKIN_LIST)
        {
            long NumElements = f->GetLong();
            for(int i=0; i < NumElements; i++)
            {
                f->BeginChunk();
                if(f->GetCurrentID() == P3D_COMPOSITE_DRAWABLE_SKIN)
                {
                    bool isTranslucent = false; 
                    tDrawablePose* skin;
                    f->GetPString(name);
                    isTranslucent = ((f->GetLong() == 0)?false:true);
                    skin = p3d::find<tDrawablePose>(store, name);
                    if(skin)
                    {
                        tCompositeDrawable::DrawablePoseElement* compDrawSkin = compositeDraw->AddPose(skin);
                        compDrawSkin->SetIsTranslucent(isTranslucent);
                        if(isTranslucent)
                        {
                            numTranslucentElements++;
                        }

                        //
                        // Compute the overall bounding box for the CompositeDrawable
                        //
                        rmt::Box3D skinBox;
                        skin->GetBoundingBox(&skinBox);
						tPose* p = compositeDraw->GetPose();
                        skin->SetPose(p);

                        // There should probably be something about translating the skin to it's joint
                        //position, but skins in a composite drawable aren't attached to a joint. Not joint ID
                        //to read from file.
						// So here's how we'll hack it. We'll ASSUME (so automatically you know this isn't going to work
						//for you) that the skeleton is mostly for the skined object. We'll walk through the skeleton
						//joints and build a box around it. Take the center of the box and translate the skinned mesh's
						//bounding box to that location.
						rmt::Box3D skelBox;
						skelBox.high.Set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
						skelBox.low.Set(FLT_MAX, FLT_MAX, FLT_MAX);
						if(!p->IsPoseReady())
						{
							p->Evaluate();
						}
						// Skip the first three joints.
						if(p)
						{
							bool haveNonOrginPoint = false;
							for(int j = 0; j < p->GetNumJoint(); ++j)
							{
								tPose::Joint* joint = p->GetJoint(j);
								if(joint)
								{
									rmt::Vector pt = joint->worldMatrix.Row(3);
									if(!haveNonOrginPoint && (pt.x == 0.0f && pt.y == 0.0f && pt.z == 0.0f))
									{
										//This is probably just a world transform or something. We can
										//skip it since the initialized skelBox will give us 0, 0, 0 point.
										continue;
									}
									haveNonOrginPoint = true;
									skelBox.Expand(joint->worldMatrix.Row(3));
								}
							}
						}
						rmt::Vector mid = skelBox.Mid();

                        compositeDraw->boundingBox.high.x = rmt::Max( (skinBox.high.x + mid.x), (compositeDraw->boundingBox.high.x) );
                        compositeDraw->boundingBox.high.y = rmt::Max( (skinBox.high.y + mid.y), (compositeDraw->boundingBox.high.y) );
                        compositeDraw->boundingBox.high.z = rmt::Max( (skinBox.high.z + mid.z), (compositeDraw->boundingBox.high.z) );

                        compositeDraw->boundingBox.low.x = rmt::Min( (skinBox.low.x + mid.x), (compositeDraw->boundingBox.low.x) );
                        compositeDraw->boundingBox.low.y = rmt::Min( (skinBox.low.y + mid.y), (compositeDraw->boundingBox.low.y) );
                        compositeDraw->boundingBox.low.z = rmt::Min( (skinBox.low.z + mid.z), (compositeDraw->boundingBox.low.z) );

                        while(f->ChunksRemaining())
                        {
                            switch(f->BeginChunk())
                            {
                                case P3D_COMPOSITE_DRAWABLE_SORTORDER:
                                    {
                                        compDrawSkin->SetSortOrder(f->GetFloat());
                                    }
                                    break;

                                default:
                                    break;
                            }
                            f->EndChunk();
                        }
                    }               
                }
                f->EndChunk();
            }
        }
        else  if(f->GetCurrentID() == P3D_COMPOSITE_DRAWABLE_EFFECT_LIST)
        {
            long NumElements = f->GetLong();
            for(int i=0; i < NumElements; i++)
            {
                f->BeginChunk();
                if(f->GetCurrentID() == P3D_COMPOSITE_DRAWABLE_EFFECT)
                {
                    bool isTranslucent = false;
                    long skeletonID = -1;
                    tEffect* effect;
                    f->GetPString(name);
                    isTranslucent = ((f->GetLong() == 0)?false:true);
                    skeletonID = f->GetLong();
                    effect = p3d::find<tEffect>(store, name);
                    if (effect)
                    {
                        tCompositeDrawable::DrawableEffectElement* compDrawEffect = compositeDraw->AddEffect(effect, skeletonID);
                        compDrawEffect->SetIsTranslucent(isTranslucent);
                        if(isTranslucent)
                        {
                            numTranslucentElements++;
                        }

                        //
                        // Compute the overall bounding box for the CompositeDrawable
                        //
                        rmt::Box3D effectBox;
                        effect->GetBoundingBox(&effectBox);
                        rmt::Matrix effectMatrix;
                        effectMatrix.Identity();
                        tSkeleton::Joint* effectJoint = skeleton->GetJoint(skeletonID);
                        effectMatrix.Mult(effectJoint->worldMatrix);

                        effectMatrix.Transform(effectBox.low, &effectBox.low);
                        effectMatrix.Transform(effectBox.high, &effectBox.high);
                        
                        // Because we can end up rotating the bounding box into who knows what angle
                        //we need to recalculate the min and the max values.
                        //
                        // X component
                        //
                        float v = rmt::Max(effectBox.low.x, effectBox.high.x);
                        compositeDraw->boundingBox.high.x = rmt::Max(v, compositeDraw->boundingBox.high.x);
                        v = rmt::Min(effectBox.low.x, effectBox.high.x);
                        compositeDraw->boundingBox.low.x = rmt::Min(v, compositeDraw->boundingBox.low.x);

                        //
                        // Y component
                        //
                        v = rmt::Max(effectBox.low.y, effectBox.high.y);
                        compositeDraw->boundingBox.high.y = rmt::Max(v, compositeDraw->boundingBox.high.y);
                        v = rmt::Min(effectBox.low.y, effectBox.high.y);
                        compositeDraw->boundingBox.low.y = rmt::Min(v, compositeDraw->boundingBox.low.y);

                        //
                        // Z component
                        //
                        v = rmt::Max(effectBox.low.z, effectBox.high.z);
                        compositeDraw->boundingBox.high.z = rmt::Max(v, compositeDraw->boundingBox.high.z);
                        v = rmt::Min(effectBox.low.z, effectBox.high.z);
                        compositeDraw->boundingBox.low.z = rmt::Min(v, compositeDraw->boundingBox.low.z);

                        while(f->ChunksRemaining())
                        {
                            switch(f->BeginChunk())
                            {
                                case P3D_COMPOSITE_DRAWABLE_SORTORDER:
                                    {
                                        compDrawEffect->SetSortOrder(f->GetFloat());
                                    }
                                    break;

                                default:
                                    break;
                            }
                            f->EndChunk();
                        }
                    }         
 
                }
                f->EndChunk();
            }
        }
        else if(f->GetCurrentID() == P3D_COMPOSITE_DRAWABLE_PROP_LIST)
        {
            long NumElements = f->GetLong();
            for(int i=0; i < NumElements; i++)
            {
                f->BeginChunk();
                if(f->GetCurrentID() == P3D_COMPOSITE_DRAWABLE_PROP)
                {
                    bool isTranslucent = false;
                    long skeletonID = -1;
                    tDrawable* prop;
                    f->GetPString(name);
                    isTranslucent = ((f->GetLong() == 0)?false:true);
                    skeletonID = f->GetLong();
                    prop = p3d::find<tDrawable>(store, name);
                    if(prop)
                    {
                        tCompositeDrawable::DrawablePropElement* compDrawProp = compositeDraw->AddProp(prop, skeletonID);
                        compDrawProp->SetIsTranslucent(isTranslucent);
                        if(isTranslucent)
                        {
                            numTranslucentElements++;
                        }

                        //
                        // Compute the overall bounding box for the CompositeDrawable
                        //
                        rmt::Box3D propBox;
                        prop->GetBoundingBox(&propBox);
                        rmt::Matrix propMatrix;
                        propMatrix.Identity();
                        tSkeleton::Joint* propJoint = skeleton->GetJoint(skeletonID);
                        propMatrix.Mult(propJoint->worldMatrix);

                        propMatrix.Transform(propBox.low, &propBox.low);
                        propMatrix.Transform(propBox.high, &propBox.high);

                        // Because we can end up rotating the bounding box into who knows what angle
                        //we need to recalculate the min and the max values.
                        //
                        // X component
                        //
                        float v = rmt::Max(propBox.low.x, propBox.high.x);
                        compositeDraw->boundingBox.high.x = rmt::Max(v, compositeDraw->boundingBox.high.x);
                        v = rmt::Min(propBox.low.x, propBox.high.x);
                        compositeDraw->boundingBox.low.x = rmt::Min(v, compositeDraw->boundingBox.low.x);

                        //
                        // Y component
                        //
                        v = rmt::Max(propBox.low.y, propBox.high.y);
                        compositeDraw->boundingBox.high.y = rmt::Max(v, compositeDraw->boundingBox.high.y);
                        v = rmt::Min(propBox.low.y, propBox.high.y);
                        compositeDraw->boundingBox.low.y = rmt::Min(v, compositeDraw->boundingBox.low.y);

                        //
                        // Z component
                        //
                        v = rmt::Max(propBox.low.z, propBox.high.z);
                        compositeDraw->boundingBox.high.z = rmt::Max(v, compositeDraw->boundingBox.high.z);
                        v = rmt::Min(propBox.low.z, propBox.high.z);
                        compositeDraw->boundingBox.low.z = rmt::Min(v, compositeDraw->boundingBox.low.z);

                        while(f->ChunksRemaining())
                        {
                            switch(f->BeginChunk())
                            {
                                case P3D_COMPOSITE_DRAWABLE_SORTORDER:
                                    {
                                        compDrawProp->SetSortOrder(f->GetFloat());
                                    }
                                    break;

                                default:
                                    break;
                            }
                            f->EndChunk();
                        }
                    }


                }
                f->EndChunk();
            }
        }
        f->EndChunk();
    }

    // really hacky sphere 
    compositeDraw->boundingSphere.centre.x = (compositeDraw->boundingBox.high.x + compositeDraw->boundingBox.low.x) / 2;
    compositeDraw->boundingSphere.centre.y = (compositeDraw->boundingBox.high.y + compositeDraw->boundingBox.low.y) / 2;
    compositeDraw->boundingSphere.centre.z = (compositeDraw->boundingBox.high.z + compositeDraw->boundingBox.low.z) / 2;
    rmt::Vector v;
    v.Sub(compositeDraw->boundingBox.high, compositeDraw->boundingSphere.centre);
    compositeDraw->boundingSphere.radius = v.Magnitude();

    compositeDraw->translucentObjects.SetSize(numTranslucentElements);

    return(compositeDraw);
}


