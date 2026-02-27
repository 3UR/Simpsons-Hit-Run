//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/anim/lightanimation.hpp>
#include <p3d/anim/channel.hpp>
#include <p3d/light.hpp>
#include <p3d/directionallight.hpp>
#include <p3d/pointlight.hpp>
#include <p3d/spotlight.hpp>
#include <p3d/utility.hpp>
#include <p3d/view.hpp>

#include <assert.h>

using namespace RadicalMathLibrary;

//*********************************************************
// Class tLightAnimationController
//*********************************************************
tLightAnimationController::tLightAnimationController() : 
    tBlendFrameController(),
    mLight(NULL), 
    mLightType(Invalid)
{
}

//---------------------------------------------------------
tLightAnimationController::tLightAnimationController(tLightAnimationController* c) : 
    tBlendFrameController(c),
    mLight(NULL)
{
    SetLight(c->mLight);
    mLightType = c->mLightType;
}

//---------------------------------------------------------
tLightAnimationController::~tLightAnimationController()
{
    tRefCounted::Release(mLight);
}
 
//---------------------------------------------------------
tFrameController* tLightAnimationController::Clone(void)
{
    return new tLightAnimationController(this);
}

//---------------------------------------------------------
void tLightAnimationController::SetLight(tLight* light)
{
    tRefCounted::Assign(mLight, light);

    mLightType = Invalid;

    if (mLight)
    {
        if (dynamic_cast<tDirectionalLight *>(mLight)) 
        {
            mLightType = Directional;
        }
        else if (dynamic_cast<tPointLight *>(mLight))       
        {
            mLightType = Point;
        }
        // Added by Bryan Ewert on 19 Dec 2001
        else if (dynamic_cast<tSpotLight *>(mLight))
        {
            mLightType = Spot;
        }
        // Ambient light doesn't exist yet
        else 
        {
            mLightType = Ambient;
        }
    }
    // Assume that if no light was set, it must be an ambient light animation
    else
    {
        mLightType = Ambient;
    }
}

//---------------------------------------------------------
bool tLightAnimationController::ValidateAnimation(tAnimation* anim)
{
    return (anim->GetAnimationType()==Pure3DAnimationTypes::LIGHT_LITE);
}

//---------------------------------------------------------
void tLightAnimationController::UpdateNoBlending()
{
    tAnimation* anim = playInfo[0].GetAnimation();
    float frame = playInfo[0].GetCurrentFrame();

    const tAnimationGroup* animGroup = anim->GetGroupByIndex(0);
    const tColourChannel *colour = NULL;
    const tVectorChannel *pos = NULL;
    const tBoolChannel *enable = NULL;

    // DIRECTION_DIR, ATTENUATION_ATTN and CONEANGLE_CONE Added by Bryan Ewert on 19 Dec 2001
    const tVectorChannel *dir = NULL;
    const tVectorChannel *cone = NULL;
    const tVectorChannel *atten = NULL;

    if(animGroup)
    {
        colour  = animGroup->GetColourChannel(Pure3DAnimationChannels::Light::COLOUR_CLR);
        pos     = animGroup->GetVectorChannel(Pure3DAnimationChannels::Light::PARAMETER_PARM);
        enable  = animGroup->GetBoolChannel(Pure3DAnimationChannels::Light::ENABLE_EABL);
        dir     = animGroup->GetVectorChannel(Pure3DAnimationChannels::Light::DIRECTION_DIR);
        atten   = animGroup->GetVectorChannel(Pure3DAnimationChannels::Light::ATTENUATION_ATTN);
        cone    = animGroup->GetVectorChannel(Pure3DAnimationChannels::Light::CONEANGLE_CONE);

        if (mLight)
        {
            if (colour)
            {
                tColour c;
                colour->GetValue(frame, &c);
                mLight->SetColour(c);
            }

            // Position: Point, Spot
            if (pos)
            {
                Vector v;
                pos->GetValue(frame,  &v);

                // Option: Declare (virtual tLight::SetPosition) and skip the switch{}
                switch(mLightType)
                {
                    case Point:
                    {
                        ((tPointLight *)mLight)->SetPosition(v);
                        break;
                    }
                    case Spot:
                    {
                        ((tSpotLight *)mLight)->SetPosition(v);
                        break;
                    }
                    default:
                        break;
                }
            }

            // Direction: Directional, Spot
            if ( dir )
            {
                Vector  v;
                dir->GetValue( frame, &v );

                // Is there a way to do this without the switch()?
                switch(mLightType)
                {
                    case Directional:
                    {
                        ((tDirectionalLight*)mLight)->SetDirection(v);
                        break;
                    }
                    case Spot:
                    {
                        ((tDirectionalLight*)mLight)->SetDirection(v);
                        break;
                    }
                    default:
                    {
                        // ** ERROR ** Unexpected Directional Channel on non-supporting Light type.
                        assert( 0 );
                    }
                }

            }

            // Attenuation: Point, Spot
            if ( atten )
            {
                Vector  v;
                atten->GetValue( frame, &v );

                // Option: Declare (virtual tLight::SetAttenuation) and skip the switch{}
                switch(mLightType)
                {
                    // Note: The X, Y and Z components of the Attenuation Vector Channel are
                    //       used to drive Constant, Linear and Squared, respectively.
                    case Point:
                    {
                        ((tPointLight*)mLight)->SetAttenuation(v.x, v.y, v.z);
                        break;
                    }
                    case Spot:
                    {
                        ((tSpotLight*)mLight)->SetAttenuation(v.x, v.y, v.z);
                        break;
                    }
                    default:
                    {
                        // ** ERROR ** Unexpected Attenuation Channel on non-supporting Light type.
                        assert( 0 );
                    }
                }
            }

            // ConeAngle: Spot
            if ( cone && ( mLightType == Spot ) )
            {
                Vector  v;
                cone->GetValue( frame, &v );
                
                // Note: The X, Y and Z components of the ConeAngle Vector Channel are
                //       used to drive Phi, Theta and Falloff, respectively.
                ((tSpotLight*)mLight)->SetCone( v.x, v.y, v.z );
            }

            // Enable: ALL
            if (enable)
            {
                bool b;
                enable->GetValue(frame, &b);
                mLight->Enable(b);
            }
        }
        else if (mLightType == Ambient)
        {
            if (colour)
            {
                tColour c;
                colour->GetValue(frame, &c);
                tView *view = p3d::context->GetView();

                if(view)
                {
                    view->SetAmbientLight(c);
                }
            }
        }
    }
}

//---------------------------------------------------------
void tLightAnimationController::UpdateWithBlending()
{
    // If there is no destination, why do the work?
    if (mLight == NULL) 
        return;

    float blendColour[4] = {0.0f,0.0f,0.0f,0.0f};
    Vector blendPos(0.0f, 0.0f, 0.0f);          // Position
    Vector blendDir(0.0f, 0.0f, 0.0f);          // Direction
    Vector blendAtten(0.0f, 0.0f, 0.0f);        // Attenuation
    Vector blendCone(0.0f, 0.0f, 0.0f);         // Cone Angle
    float blendEnable = 0.0f;

    for (int i = 0; i < nAnim; i++)
    {
        tAnimation* anim = playInfo[i].GetAnimation();
        float frame = playInfo[i].GetCurrentFrame();
        float weight = playInfo[i].GetWeight();
        if (i == 0) 
        {
            weight = 1.0F;
        }

        const tAnimationGroup* animGroup = anim->GetGroupByIndex(0);
        const tColourChannel *colour = NULL;
        const tVectorChannel *pos  = NULL;
        const tBoolChannel *enable = NULL;

        // DIRECTION_DIR, ATTENUATION_ATTN and CONEANGLE_CONE Added by Bryan Ewert on 19 Dec 2001
        const tVectorChannel *dir = NULL;
        const tVectorChannel *cone = NULL;
        const tVectorChannel *atten = NULL;

        if(animGroup)
        {
            colour  = animGroup->GetColourChannel(Pure3DAnimationChannels::Light::COLOUR_CLR);
            pos     = animGroup->GetVectorChannel(Pure3DAnimationChannels::Light::PARAMETER_PARM);
            enable  = animGroup->GetBoolChannel(Pure3DAnimationChannels::Light::ENABLE_EABL);
            dir     = animGroup->GetVectorChannel(Pure3DAnimationChannels::Light::DIRECTION_DIR);
            atten   = animGroup->GetVectorChannel(Pure3DAnimationChannels::Light::ATTENUATION_ATTN);
            cone    = animGroup->GetVectorChannel(Pure3DAnimationChannels::Light::CONEANGLE_CONE);

            if (colour != NULL)
            {
                tColour c;
                colour->GetValue(frame, &c);

                blendColour[0] += (float)c.Red() * weight;
                blendColour[1] += (float)c.Green() * weight;
                blendColour[2] += (float)c.Blue() * weight;
                blendColour[3] += (float)c.Alpha() * weight;
            }

            if (pos != NULL)
            {
                Vector v;
                pos->GetValue(frame, &v);
                blendPos.ScaleAdd(weight, v);
            }

            if ( dir != NULL )
            {
                Vector v;
                dir->GetValue( frame, &v );
                blendDir.ScaleAdd( weight, v );
            }

            if ( atten != NULL )
            {
                Vector v;
                atten->GetValue( frame, &v );
                blendAtten.ScaleAdd( weight, v );
            }

            if ( cone != NULL )
            {
                Vector v;
                cone->GetValue( frame, &v );
                blendCone.ScaleAdd( weight, v );
            }

            if (enable != NULL)
            {
                bool b;
                enable->GetValue(frame, &b);
                blendEnable += (b ? 1.0f : 0.0f) * weight;
            }
        }
    }

    switch (mLightType)
    {
        case Point:       
        {
            ((tPointLight*)mLight)->SetPosition(blendPos);
            ((tPointLight*)mLight)->SetAttenuation(blendAtten.x, blendAtten.y, blendAtten.z);
            break;
        }
        case Directional:
        {
            ((tDirectionalLight*)mLight)->SetDirection(blendDir);
            break;
        }
        case Spot:
        {
            ((tSpotLight*)mLight)->SetPosition(blendPos);
            ((tSpotLight*)mLight)->SetDirection(blendDir);

            // Note: The X, Y and Z components of the Attenuation Vector Channel are
            //       used to drive Constant, Linear and Squared, respectively.
            ((tSpotLight*)mLight)->SetAttenuation(blendAtten.x, blendAtten.y, blendAtten.z);

            // Note: The X, Y and Z components of the ConeAngle Vector Channel are
            //       used to drive Phi, Theta and Falloff, respectively.
            ((tSpotLight*)mLight)->SetCone(blendCone.x, blendCone.y, blendCone.z);
            break;
        }
        default:
            break;
    }

    if (blendEnable > 0.5F) 
    {
        mLight->Enable(true);
    }
    else
    {
        mLight->Enable(false);
    }

    // Compute colour and clamp
    int r = FtoL(blendColour[0]);
    int g = FtoL(blendColour[1]);
    int b = FtoL(blendColour[2]);
    int a = FtoL(blendColour[3]);

    r = Clamp(r,0,255);
    g = Clamp(g,0,255);
    b = Clamp(b,0,255);
    a = Clamp(a,0,255);

    mLight->SetColour(tColour(r, g, b, a));
}

