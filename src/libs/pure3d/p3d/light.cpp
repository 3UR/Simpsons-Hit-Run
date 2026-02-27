//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/light.hpp>
#include <p3d/ambientlight.hpp>
#include <p3d/view.hpp>
#include <p3d/error.hpp>
#include <p3d/utility.hpp>

static const float EPSILON = 1.0e-4f;

//----------------------------------------------------------------
tLight::tLight() :
    colour(0xffffffff),
    position(0.0f, 0.0f, 1.0f),
    slot(0xffffffff),
    active(false),
    enabled(true),
    isShadowCaster(false),
    animated(false),
    illuminationType(POSITIVE_ILLUMINANT)
{
}


tLight::tLight(tLight *light) :
    tEntity(light),
    colour(light->colour),
    position(light->position),
    slot(0xffffffff),
    active(false),
    enabled(light->enabled),
    isShadowCaster(light->isShadowCaster),
    animated(false),
    decayRange(light->decayRange),
    illuminationType(POSITIVE_ILLUMINANT)
{
}

tLight::~tLight()
{
    /* */   
}

void tLight::Activate(unsigned s)
{
    active = true;
    slot = s;
    Update();
}

void tLight::Deactivate(void)
{
    if(slot == 0xffffffff)
        return;

    p3d::pddi->EnableLight(slot, false);
    active = false;
    slot = 0xffffffff;
}

void tLight::SetColour(tColour col)
{
    colour = col;
    Update();
}

tColour tLight::GetColour()
{
    return colour;
}

void tLight::SetPosition(float x, float y, float z)
{
    position.x = x;
    position.y = y;
    position.z = z;
}

void tLight::GetPosition(float* x, float* y, float* z)
{
    *x = position.x;
    *y = position.y;
    *z = position.z;
}

void tLight::Enable(bool e) 
{ 
    enabled = e; 
    Update();
}

void tLight::Animated(bool a)
{
    animated = a;
}

void tLight::SetDecayType(DecayRange::Type type)
{
    decayRange.type = type;
}

tLight::DecayRange::Type tLight::GetDecayType()
{
    return decayRange.type;
}

void tLight::SetDecayRange(const rmt::Vector &inner, const rmt::Vector &outer)
{
    decayRange.outer.x = rmt::Max(0.0f, outer.x);
    decayRange.outer.y = rmt::Max(0.0f, outer.y);
    decayRange.outer.z = rmt::Max(0.0f, outer.z);
    decayRange.inner.x = rmt::Clamp(inner.x, 0.0f, decayRange.outer.x);
    decayRange.inner.y = rmt::Clamp(inner.y, 0.0f, decayRange.outer.y);
    decayRange.inner.z = rmt::Clamp(inner.z, 0.0f, decayRange.outer.z);
}

void tLight::SetDecayRotationY(float rotationY)
{
    decayRange.sinRotationY = rmt::Sin(rotationY);
    decayRange.cosRotationY = rmt::Cos(rotationY);
}

rmt::Vector tLight::GetInnerDecayRange()
{
    return decayRange.inner;
}

rmt::Vector tLight::GetOuterDecayRange()
{
    return decayRange.outer;
}

float tLight::GetDecayRotationY() const
{
    return rmt::ATan2(-decayRange.sinRotationY, decayRange.cosRotationY);
}

// Return a scale to be applied to the light intensity which
// represents the light's decay at some given position.
float tLight::Decay(const rmt::Vector &samplePosition)
{
    float decay = 0.0f;
    if(decayRange.type == DecayRange::NO_DECAY_RANGE)
    {
        decay = 1.0f;
    }
    else if(decayRange.type == DecayRange::SPHERE_DECAY_RANGE)
    {
        // Computation is simplified since only x-component is used.
        float in = decayRange.inner.x;
        float out = decayRange.outer.x;
        float distance = (samplePosition - position).Magnitude(); // Distance from sample to light.

        if(distance <= in)
        {
            decay = 1.0f;
        }
        else if(distance < out)
        {
            decay = rmt::SmoothClamp( (out - distance) / (out - in) );
        }
    }
    // HBW TODO: For now just treat ELLIPSOID_DECAY_RANGE as CUBOID_DECAY_RANGE.  Update
    // this later with proper range code.
    else if(decayRange.type == DecayRange::CUBOID_DECAY_RANGE ||
            decayRange.type == DecayRange::ELLIPSOID_DECAY_RANGE)
    {
        const rmt::Vector &in  = decayRange.inner;
        const rmt::Vector &out = decayRange.outer;
        rmt::Vector offset(samplePosition.x - position.x,
                           samplePosition.y - position.y,
                           samplePosition.z - position.z); // Vector from sample to light
        
        if(rmt::Fabs(offset.y) >= out.y)
        {
            decay = 0.0f;
        }
        else
        {
            if(!rmt::Epsilon(decayRange.sinRotationY, 0.0f, EPSILON))
            {
                float tmpX = decayRange.cosRotationY * offset.x +
                             decayRange.sinRotationY * offset.z;
                offset.z =  decayRange.cosRotationY * offset.z -
                            decayRange.sinRotationY * offset.x;
                offset.x = tmpX;
            }

            offset.x = rmt::Fabs(offset.x);
            offset.y = rmt::Fabs(offset.y);
            offset.z = rmt::Fabs(offset.z);
            
            if( offset.x >= out.x || offset.z >= out.z )
            {
                decay = 0.0f;
            }
            else
            {
                decay = 1.0f;
                
                if( offset.x > in.x )
                {
                    decay *= rmt::SmoothClamp((out.x - offset.x) / (out.x - in.x));
                }
                if( offset.y > in.y )
                {
                    decay *= rmt::SmoothClamp((out.y - offset.y) / (out.y - in.y));
                }
                if( offset.z > in.z )
                {
                    decay *= rmt::SmoothClamp((out.z - offset.z) / (out.z - in.z));
                }
            }
        }
    }

    return decay;
}

void tLight::SetIlluminationType(IlluminationType type)
{
    illuminationType = type;
}

//*************************************************
//
// Class tLightGroup
//
//
tLightGroup::tLightGroup() :
    mNumLights(0),
    mCurNumLights(0),
    mLights(0)
{
}

tLightGroup::tLightGroup(int n) :
    mNumLights(n),
    mCurNumLights(n)
{
    mLights.SetSize( n );
}

void tLightGroup::SetNumLights(int n)
{
    if(mNumLights == 0)
    {
        mNumLights = mCurNumLights = n;
        mLights.SetSize( n );
    }
    else
    {
        P3DASSERT(n <= mNumLights);
        mCurNumLights = n;
    }
}


//*************************************************
//
// CLass tLightGroup
//
//
tLightGroup::~tLightGroup()
{
    int a;
    for (a = 0; a < mNumLights; a++)
    {
        if(mLights[a])
            mLights[a]->Release();
    }

    mNumLights = 0;
    mCurNumLights = 0;
}


//*************************************************
//
// CLass tLightGroup
//
//
void tLightGroup::SetViewLights(tView *view)
{
    P3DASSERT(view != NULL);

    view->RemoveAllLights();

    int a;
    for (a = 0; a < mCurNumLights; a++) 
    {
        if(dynamic_cast<tAmbientLight*>(mLights[a]) != 0)
        {
            view->SetAmbientLight(mLights[a]->GetColour());
        }
        else if(mLights[a])
        {
            view->AddLight(mLights[a]);
        }
    }
}

void tLightGroup::SetLight(int n, tLight* light)
{
    P3DASSERT(n < mCurNumLights);
    tRefCounted::Assign(mLights[n], light);
}

