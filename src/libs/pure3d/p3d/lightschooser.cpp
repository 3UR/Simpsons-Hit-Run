//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


// Maximum world lights.

#include <p3d/lightschooser.hpp>
#include <p3d/error.hpp>
#include <p3d/light.hpp>
#include <p3d/ambientlight.hpp>
#include <p3d/directionallight.hpp>
#include <p3d/pointlight.hpp>
#include <p3d/spotlight.hpp>
#include <pddi/pddi.hpp>
#include <radmath/trig.hpp>

static const int MAX_DIRECTIONAL_LIGHTS = 128;
static const int MAX_POINT_LIGHTS = 128;
static const int MAX_SPOT_LIGHTS = 128;
static const int MAX_AMBIENT_LIGHTS = 128;

static const tColour TCOLOUR_BLACK( 0xff000000 );
static const tColour TCOLOUR_WHITE( 0xffffffff );

static const float BIG_NUM    = 1.0e12f;
static const float EPSILON    = 1.0e-4f;
static const float DEFAULT_SHADOW_CONTRAST = 1.0f;
static const float DEFAULT_CACHE_DISTANCE  = 1.0e-3f;

static const unsigned POSITIVE_ILLUMINANT = 0x01;
static const unsigned NEGATIVE_ILLUMINANT = 0x02;
static const unsigned SHADOW_CASTER       = 0x04;
static const unsigned ANIMATED_LIGHT      = 0x08;

const unsigned tLightsChooser::DEFAULT_MAX_NUM_LIGHTS = 4;
const unsigned tLightsChooser::DEFAULT_MAX_NUM_SHADOW_CASTERS = 2;

tLightsChooser::tLightsChooser(int maxNumLights,
                               int maxNumShadowCasters) :
    mAmbientColour(TCOLOUR_BLACK),
    mShadowContrast(DEFAULT_SHADOW_CONTRAST), 
    mNumImportantLights(0),
    mCacheSynchronizeID(0)
{
    mMaxNumLights = rmt::Max( 0, maxNumLights );
    mMaxNumShadowCasters = rmt::Max( 0, maxNumShadowCasters );

    mAmbLightList = new LightList(MAX_AMBIENT_LIGHTS);
    mAmbLightList->SetNumLights(0);
    mDirLightList = new LightList(MAX_DIRECTIONAL_LIGHTS);
    mDirLightList->SetNumLights(0);
    mPntLightList = new LightList(MAX_POINT_LIGHTS);
    mPntLightList->SetNumLights(0);
    mSptLightList = new LightList(MAX_SPOT_LIGHTS);
    mSptLightList->SetNumLights(0);
    mTmpLightList = new LightList(mMaxNumLights + 1);
    mTmpLightList->SetNumLights(0);
    mPQueue = new LightPriorityQueue;
}
    
tLightsChooser::~tLightsChooser(void)
{
    RemoveAllLights();
    delete mAmbLightList;
    delete mDirLightList;
    delete mPntLightList;
    delete mSptLightList;
    delete mTmpLightList;
    delete mPQueue;
}
    
void tLightsChooser::AddLight(tLight* light)
{
    if(light == NULL)
    {
        return;
    }
    
    LightList *lightList = GetLightList(light);

    int numLights = lightList->GetNumLights();
#ifdef RAD_DEBUG
    int i;
    for(i = lightList->GetNumLights() - 1; i >= 0; i--)
    {
        if(lightList->GetLight(i) == light)
        {
            P3DASSERT(0 && "Duplicate light in tLightsChooser::AddLight()");
        }
    }
#endif        
    lightList->SetNumLights(numLights + 1);
    lightList->SetLight(numLights, light);
    mCacheSynchronizeID++; // Ivalidate Cache.
}
    
void tLightsChooser::AddLights(tLightGroup* lights)
{
    if(lights == NULL)
    {
        return;
    }
        
    int i;
    for(i = 0; i < lights->GetNumLights(); i++)
    {
        AddLight(lights->GetLight(i));
    }
}
    
// If you are going to be removing quite a few lights
// it's probably best just to call RemoveAllLights() and
// add again the ones you want.
void tLightsChooser::RemoveLight(tLight* oldLight)
{
    if(oldLight == NULL)
    {
        return;
    }

    LightList *lightList = GetLightList(oldLight);

    int i;
    for(i = lightList->GetNumLights() - 1; i >= 0; i--)
    {
        if(lightList->GetLight(i) == oldLight)
        {
            int lastIndex = lightList->GetNumLights() - 1;
            if(i == lastIndex)
            {
                // Replace the last light with NULL.
                lightList->SetLight(i, NULL); 
            }
            else
            {
                // Replace any other with the last light.
                lightList->SetLight(i, lightList->GetLight(lastIndex));
                lightList->Animated(i, lightList->IsAnimated(lastIndex));
                lightList->SetLight(lastIndex, NULL);
            }
            lightList->SetNumLights(lastIndex);
            mCacheSynchronizeID++; // Ivalidate Cache.
        }
    }
}        
    
void tLightsChooser::RemoveAllLights()
{
    // Remove all the world lights.
    LightList *lists[4] = {mAmbLightList, mDirLightList,
                           mPntLightList, mSptLightList};
    int i, j;
    for(i = 0; i < 4; i++)
    {
        for(j = lists[i]->GetNumLights() - 1; j >= 0; j--)
        {
            lists[i]->SetLight(j, NULL);
        }
        lists[i]->SetNumLights(0);
    }
    mCacheSynchronizeID++; // Ivalidate Cache.
}
    
// Find the best N lights to approximate the world lights.  The number of lights
// returned, N, is the number set in the bestLights.  This function uses lights already
// available in bestLights if they are of the correct type, otherwise it creates
// new lights.  If there are fewer lights generated than the number set in bestLights,
// additional lights are set NULL.
void tLightsChooser::GetBestLights(tLightGroup* bestLights, const rmt::Vector& target,
                                   tLightsChooserCache *cache)
{
    P3DASSERT(bestLights != NULL);
    if(bestLights->GetNumLights() == 0)
    {
        return;
    }

    // Try and use cached results.
    if(GetCacheLights( bestLights, target, cache))  
    {
        UpdateLightGroupCount(bestLights);
        EnableLights(bestLights);
        return;
    }

    // Compute the ambient contribution from the tAmbientLights.
    float ambientRGB[3];
    ComputeAmbientColour(ambientRGB, target);
    
    // Determine the best directional representation of the other world lights at the target
    // position.  Use additional ambient colour to transition between lights.
    SelectLights(bestLights, ambientRGB, target);

    mAmbientColour.Set(rmt::Clamp(rmt::FtoL(ambientRGB[0]), 0, 255),
                       rmt::Clamp(rmt::FtoL(ambientRGB[1]), 0, 255),
                       rmt::Clamp(rmt::FtoL(ambientRGB[2]), 0, 255));

    if(cache != NULL)
    {
        cache->mNumWorldLights = mNumImportantLights;
        cache->Set(bestLights, target, mCacheSynchronizeID);
        cache->mAmbientColour = mAmbientColour;
    }
    
    UpdateLightGroupCount(bestLights); // bestLights->GetNumLights() should return correct number.
    EnableLights(bestLights);
}


// Update bestShadowCasters with the best lights for generating shadows.  Return lights
// in the same manner as GetBestLights().
void tLightsChooser::GetShadowCasters( tLightGroup* bestShadowCasters,
                                       const rmt::Vector& target, 
                                       tLightsChooserCache *cache, float shadowScale )
{
    P3DASSERT( bestShadowCasters != NULL );
    if( bestShadowCasters->GetNumLights() == 0 )
    {
        return;
    }

    // Try and use cached results.
    if( GetCacheShadowCasters( bestShadowCasters, target, cache) )  
    {
        UpdateLightGroupCount( bestShadowCasters );
        EnableLights( bestShadowCasters );
        return;
    }

    // Compute the ambient contribution from the tAmbientLights.
    float ambientRGB[3];
    ComputeAmbientColour(ambientRGB, target);

    SelectShadowCasters(bestShadowCasters, ambientRGB, target);

    ScaleShadowCasters( bestShadowCasters, shadowScale );

    if( cache != NULL )
    {
        cache->mNumWorldLights = mNumImportantLights;
        cache->Set(bestShadowCasters, target, mCacheSynchronizeID);
    }

    UpdateLightGroupCount( bestShadowCasters );
    EnableLights( bestShadowCasters );
}

// Scale the colour of the shadow casters to share the requested
// shadow contrast.
void tLightsChooser::ScaleShadowCasters( tLightGroup *shadowCasters, float shadowScale )
{
    int sumIntensity = 0;
    int i;
    int lightCount = 0;

    for( i = 0; i < shadowCasters->GetNumLights(); i++ )
    {
        if( shadowCasters->GetLight( i ) != NULL )
        {
            const tColour &c = shadowCasters->GetLight( i )->GetColour();
            sumIntensity += rmt::Max( c.Red(), rmt::Max( c.Green(), c.Blue() ) );
        }
    }
    
    for( i = 0; i < shadowCasters->GetNumLights(); i++ )
    {
        tLight *light = shadowCasters->GetLight( i );
        if( light != NULL )
        {
            if( sumIntensity != 0 )
            {
                const tColour &c = light->GetColour();
                int intensity = rmt::Max( c.Red(), rmt::Max( c.Green(), c.Blue() ) );
                    
                float scale   = mShadowContrast * rmt::LtoF(intensity) / rmt::LtoF(sumIntensity);
                // HBW: Not enough shadow in dim areas.  
                // Try ignoring ambient in shadow calculation.
                // tColour a = GetAmbientColour();
                // scale *= rmt::LtoF(intensity) / ( rmt::LtoF(intensity) + rmt::Max( a.Red(), rmt::Max( a.Green(), a.Blue() ) ) );
                scale *= shadowScale;
                int channel   = 255 - rmt::Clamp( rmt::FtoL( scale * rmt::LtoF(intensity) ), 0, 255 );
                // int channel   = rmt::Clamp( intensity, 0, 255 );
                light->SetColour( tColour ( channel, channel, channel ) );
            }
            else
            {
                light->SetColour( TCOLOUR_WHITE );
            }
        }
    }
}
   
// Set the darkness of shadows. 1.0=dark - 0.0=light.
void tLightsChooser::SetShadowContrast( float shadowContrast )
{
    mShadowContrast = rmt::Clamp( shadowContrast, 0.0f, 1.0f );
}

tColour tLightsChooser::GetAmbientColour() const
{
    return mAmbientColour;
}

// Total number world lights currently added.
int tLightsChooser::GetNumLights() const
{
    int numLights = mAmbLightList->GetNumLights();
    numLights += mDirLightList->GetNumLights();
    numLights += mPntLightList->GetNumLights();
    numLights += mSptLightList->GetNumLights();

    return numLights;
}

int tLightsChooser::GetMaxNumLights() const
{
    return mMaxNumLights;
}

int tLightsChooser::GetMaxNumShadowCasters() const
{
    return mMaxNumShadowCasters;
}

// Compute the ambient contribution from tAmbientLights at some point.  If there are negative
// ambient lights in the scene, the result may have negative colours.
void tLightsChooser::ComputeAmbientColour(float ambientRGB[3], const rmt::Vector &target)
{
    if(ambientRGB == NULL) return;
    
    ambientRGB[0] = 0.0f;
    ambientRGB[1] = 0.0f;
    ambientRGB[2] = 0.0f;
    tLight *light;
    
    int i;
    for(i = mAmbLightList->GetNumLights() - 1; i >= 0; i--)
    {
        light = mAmbLightList->GetLight(i);
        if(light == NULL)
        {
            continue;
        }
        tColour c = light->GetColour();
        float decay = light->Decay(target);
        if(mAmbLightList->IsNegative(i))
        {
            decay *= -1.0f;
        }
        ambientRGB[0] += decay * rmt::LtoF(c.Red());
        ambientRGB[1] += decay * rmt::LtoF(c.Green());
        ambientRGB[2] += decay * rmt::LtoF(c.Blue());
    }
}

float tLightsChooser::TargetDirLights(const rmt::Vector &target, unsigned mark)
{
    float totalIntensity = 0.0f;
    int i;
    // Process directional lights.
    for(i = mDirLightList->GetNumLights() - 1; i >= 0; i--)
    {
        tLight *light = mDirLightList->GetLight(i);
        if(light == NULL || !light->IsEnabled() ||
           (mark & mDirLightList->GetMask(i)) != mark)
        {
            mDirLightList->SetIntensity(i, 0.0f);
            continue;
        }

        rmt::Vector dir = target;
        dir.Sub(light->GetPosition());
        if(dir.MagnitudeSqr() >= mDirLightList->GetRadiusSqr(i))
        {
            mDirLightList->SetIntensity(i, 0.0f);
            continue;
        }
        
        tDirectionalLight *dLight = static_cast<tDirectionalLight*>(light);
        float decay     = dLight->Decay(target);
        tColour col     = dLight->GetColour();
        float intensity = decay * rmt::LtoF(rmt::Max(col.Red(), rmt::Max(col.Green(), col.Blue())));
        col.Set(rmt::Clamp(rmt::FtoL(decay * rmt::LtoF(col.Red())), 0, 255),
                rmt::Clamp(rmt::FtoL(decay * rmt::LtoF(col.Green())), 0, 255),
                rmt::Clamp(rmt::FtoL(decay * rmt::LtoF(col.Blue())), 0, 255));

        mDirLightList->SetColour(i, col);
        mDirLightList->SetIntensity(i, intensity);
        mDirLightList->SetDirection(i, dLight->GetDirection());

        if(intensity >= 0.0f)
        {
            mNumImportantLights++;
        }
        totalIntensity += intensity;
    }

    return totalIntensity;
}

float tLightsChooser::TargetPntLights(const rmt::Vector &target, unsigned mark)
{
    float totalIntensity = 0.0f;
    int i;
    // Process point lights.
    for(i = mPntLightList->GetNumLights() - 1; i >= 0; i--)
    {
        tLight *light = mPntLightList->GetLight(i);
        if(light == NULL || !light->IsEnabled() ||
           (mark & mPntLightList->GetMask(i)) != mark)
        {
            mPntLightList->SetIntensity(i, 0.0f);
            continue;
        }

        rmt::Vector dir = target;
        dir.Sub(light->GetPosition());
        if(dir.MagnitudeSqr() >= mPntLightList->GetRadiusSqr(i))
        {
            mPntLightList->SetIntensity(i, 0.0f);
            continue;
        }
        
        float decay     = light->Decay(target);
        if(decay == 0.0f)
        {
            mPntLightList->SetIntensity(i, 0.0f);
            continue;
        }
        
        tPointLight *pLight = static_cast<tPointLight*>(light);
        float a, b, c;
        pLight->GetAttenuation(&a, &b, &c);
        float d = dir.NormalizeSafe();

        float attenuation = a;
        if(b > 0.0f)
        {
            attenuation = b * d;
        }
        else if(c > 0.0f)
        {
            attenuation = d * d * c;
        }

        tColour col(TCOLOUR_BLACK);
        float intensity = 0.0f;

        // Peform detailed attenuation calculation only if useable.
        if(attenuation > 0.0f)
        {
            attenuation = (attenuation <= 0.0f) ? 0.0f : decay / attenuation;
            
            col     = pLight->GetColour();
            intensity = attenuation * rmt::LtoF(rmt::Max(col.Red(), rmt::Max(col.Green(), col.Blue())));
            col.Set(rmt::Clamp(rmt::FtoL(attenuation * rmt::LtoF(col.Red()  )), 0, 255),
                    rmt::Clamp(rmt::FtoL(attenuation * rmt::LtoF(col.Green())), 0, 255),
                    rmt::Clamp(rmt::FtoL(attenuation * rmt::LtoF(col.Blue() )), 0, 255));
        }

        mPntLightList->SetColour(i, col);
        mPntLightList->SetIntensity(i, intensity);
        mPntLightList->SetDirection(i, dir);

        if(intensity > 0.0f)
        {
            mNumImportantLights++;
        }
        totalIntensity += intensity;
    }

    return totalIntensity;
}

float tLightsChooser::TargetSptLights(const rmt::Vector &target, unsigned mark)
{
    float totalIntensity = 0.0f;
    int i;
    // Process spot lights.
    for(i = mSptLightList->GetNumLights() - 1; i >= 0; i--)
    {
        tLight *light = mSptLightList->GetLight(i);
        if(light == NULL || !light->IsEnabled() ||
           (mark & mSptLightList->GetMask(i)) != mark)
        {
            mSptLightList->SetIntensity(i, 0.0f);
            continue;
        }

        rmt::Vector dir = target;
        dir.Sub(light->GetPosition());
        if(dir.MagnitudeSqr() >= mSptLightList->GetRadiusSqr(i))
        {
            mSptLightList->SetIntensity(i, 0.0f);
            continue;
        }
        
        float decay = light->Decay(target);
        if(decay == 0.0f)
        {
            mSptLightList->SetIntensity(i, 0.0f);
            continue;
        }
        
        tSpotLight *sLight = static_cast<tSpotLight*>(light);
        float a, b, c;
        sLight->GetAttenuation(&a, &b, &c);
        float d = dir.NormalizeSafe();

        float attenuation = a;
        if(b > 0.0f)
        {
            attenuation = b * d;
        }
        else if(c > 0.0f)
        {
            attenuation = d * d * c;
        }
        attenuation = (attenuation <= 0.0f) ? 0.0f : decay / attenuation;

        float alpha = sLight->GetDirection().Dot(dir);
        float phi;     // Cone angle.
        float theta;   // Penumbra angle.
        float falloff; // Falloff scale?
        sLight->GetCone(&phi, &theta, &falloff);
        phi = rmt::Cos(0.5f * phi);
        if(alpha < phi)
        {
            attenuation = 0.0f;
        }

        tColour col(TCOLOUR_BLACK);
        float intensity = 0.0f;

        // Peform detailed attenuation calculation only if useable.
        if(attenuation > 0.0f)
        {
            theta = rmt::Cos(0.5f * theta);
            float diff = theta - phi;
            attenuation *= (diff == 0.0f) ? 0.0f : rmt::Clamp((alpha - phi) / diff , 0.0f, 1.0f);
            float fakePowf;
            if(rmt::Epsilon(falloff, 0.0f))
            {
                fakePowf = 1.0f;
            }
            else
            {
                fakePowf = powf(alpha, falloff);
            }
            attenuation *= rmt::Clamp(fakePowf, 0.0f, 1.0f);
            
            col     = sLight->GetColour();
            intensity = attenuation * rmt::LtoF(rmt::Max(col.Red(), rmt::Max(col.Green(), col.Blue())));
            col.Set(rmt::Clamp(rmt::FtoL(attenuation * rmt::LtoF(col.Red())),   0, 255),
                    rmt::Clamp(rmt::FtoL(attenuation * rmt::LtoF(col.Green())), 0, 255),
                    rmt::Clamp(rmt::FtoL(attenuation * rmt::LtoF(col.Blue())),  0, 255));
        }
        
        mSptLightList->SetColour(i, col);
        mSptLightList->SetIntensity(i, intensity);
        mSptLightList->SetDirection(i, dir);

        if(intensity > 0.0f)
        {
            mNumImportantLights++;
        }
        totalIntensity += intensity;
    }

    return totalIntensity;
}

float tLightsChooser::Target(const rmt::Vector &target, unsigned mark)
{
    float totalIntensity = 0.0f;

    mNumImportantLights = 0;

    totalIntensity += TargetDirLights(target, mark);
    totalIntensity += TargetPntLights(target, mark);
    totalIntensity += TargetSptLights(target, mark);

    return totalIntensity;
}

// Select the brightest lights.  Fill in the LightList passed in to this
// function.  The number of LightInfos filled in is equal to the current
// LightList count.  Return the combined intensities of the returned lights.
float tLightsChooser::PickBrightestLights(LightList *brightestLights)
{
    P3DASSERT(brightestLights != NULL);

    float selectedIntensity = 0.0f;  // The returned intensity of the selected lights.

    int numLights = brightestLights->GetNumLights();

    mPQueue->Reset();
    mPQueue->SetDepth(numLights);
    LightList *lightlist[3] = {mDirLightList, mPntLightList, mSptLightList};
    LightList *llist;

    // Add the directional, point, and spotlights to the priority queue.
    int i, j;
    for(i = 0; i < 3; i++)
    {
        llist = lightlist[i];
        for(j = llist->GetNumLights() - 1; j >= 0; j--)
        {
            mPQueue->Add(llist->GetLightInfo(j));
        }
    }

    // Use the brightest lights of the priority queue to set the returned LightInfos
    LightInfo *brightLightInfo = mPQueue->Head();
    for(i = 0; i < numLights; i++)
    {
        // Update this returned LightInfo with the next bright light.
        brightestLights->SetColour(i, brightLightInfo->colour);
        brightestLights->SetDirection(i, brightLightInfo->direction);
        brightestLights->SetIntensity(i, brightLightInfo->intensity);

        selectedIntensity += brightLightInfo->intensity;  

        brightLightInfo = brightLightInfo->next;
    }

    return selectedIntensity;
}

// Determine the best directional representation of the other world lights at the target
// position.  Use additional ambient colour to transition between lights.
void tLightsChooser::SelectLights(tLightGroup *bestLights, float ambientRGB[3],
                                  const rmt::Vector &target)
{
    P3DASSERT(ambientRGB != NULL);

    int numLights = bestLights->GetNumLights();
    mTmpLightList->Clear();
    mTmpLightList->SetNumLights(numLights + 1);
    tColour transitionColour = TCOLOUR_BLACK;

    // Go through each light and see if it is active and within range.  If so compute
    // the intensity for the given target.
    float totalIntensity = Target(target);

    float subtractiveAmbient = 0.333f * (rmt::Min(ambientRGB[0], 0.0f) +
                                         rmt::Min(ambientRGB[1], 0.0f) +
                                         rmt::Min(ambientRGB[2], 0.0f));
    float allLightScale = 1.0f; // Scale all lights instead of subtracting negative lights.
                                // This makes for a smoother transition.
    bool hasSubtractiveAmbient = false;
    if(totalIntensity > 0.0f && subtractiveAmbient < -EPSILON)
    {
        hasSubtractiveAmbient = true;
        allLightScale = (totalIntensity + subtractiveAmbient) / totalIntensity;
        allLightScale = rmt::Max(allLightScale, 0.0f);
    }

    // Based on the intensites computed in Target(), determine the brightest N
    // lights, where N is the current number of lights set in mTmpLightList
    // (numLights + 1). 
    float brightestIntensities = PickBrightestLights(mTmpLightList);
    
    // Intensity of the next brightest light.
    float nextBrightestIntensity = mTmpLightList->GetIntensity(numLights);

    // Intensity of the next brightest light and all dimmer lights.
    float allOtherIntensities = totalIntensity - brightestIntensities +
                                nextBrightestIntensity;

    // Set the transional ambient with either the next brightest light or fraction
    // of all extra lights.
    int maxAmbient = 0;
    if(mTmpLightList->GetIntensity(numLights) > 0.0f)
    {
        const float AMB_SCALE = 0.33f;
        if(nextBrightestIntensity > AMB_SCALE * allOtherIntensities)
        {
            maxAmbient = rmt::FtoL(nextBrightestIntensity * allLightScale);
        }
        else
        {
            maxAmbient = rmt::FtoL(AMB_SCALE * allOtherIntensities * allLightScale);
        }

        ambientRGB[0] = ambientRGB[0] + rmt::LtoF(maxAmbient);
        ambientRGB[1] = ambientRGB[1] + rmt::LtoF(maxAmbient);
        ambientRGB[2] = ambientRGB[2] + rmt::LtoF(maxAmbient);
    }
    
    int i;
    // Reduce and copy the N brightest lights to the returned light group.
    for(i = 0; i < numLights; i++)
    {
        if(rmt::FtoL(mTmpLightList->GetIntensity(i)) - maxAmbient <= 0)
        {
            bestLights->SetLight(i, NULL); // zero intensity -> no light.
            continue;
        }

        // Get the next directional light to update.
        tDirectionalLight *dLight = dynamic_cast<tDirectionalLight*>(bestLights->GetLight(i));
        if(dLight == NULL)
        {
            dLight = new tDirectionalLight; // Create a new directional light if needed.
            bestLights->SetLight(i, dLight);
        }

        // Colour is colour of one of the bright lights less the transitional ambient.
        tColour col = mTmpLightList->GetColour(i);
        if(hasSubtractiveAmbient)
        {
            col.Set(rmt::FtoL(rmt::LtoF(col.Red())   * allLightScale),
                    rmt::FtoL(rmt::LtoF(col.Green()) * allLightScale),
                    rmt::FtoL(rmt::LtoF(col.Blue())  * allLightScale));
        }
                 
        dLight->SetColour(tColour(rmt::Clamp(col.Red()   - maxAmbient, 0, 255 ),
                                  rmt::Clamp(col.Green() - maxAmbient, 0, 255 ),
                                  rmt::Clamp(col.Blue()  - maxAmbient, 0, 255 )));

        dLight->SetDirection(mTmpLightList->GetDirection(i));
        dLight->Animated(mTmpLightList->IsAnimated(i));
    }
}

void tLightsChooser::SelectShadowCasters(tLightGroup *bestShadowCasters, 
                                         float ambientRGB[3], const rmt::Vector &target)
{
    P3DASSERT(bestShadowCasters != NULL);

    int numLights = bestShadowCasters->GetNumLights();

    if(numLights == 0)
    {
        return;
    }

    mTmpLightList->Clear();
    mTmpLightList->SetNumLights(numLights + 1);

    float totalIntensity = Target(target, SHADOW_CASTER);  

    float subtractiveAmbient = 0.333f * (rmt::Min(ambientRGB[0], 0.0f) +
                                         rmt::Min(ambientRGB[1], 0.0f) +
                                         rmt::Min(ambientRGB[2], 0.0f));
    float allLightScale = 1.0f; // Scale all lights instead of subtracting negative lights.
                                // This makes for a smoother transition.
    bool hasSubtractiveAmbient = false;
    if(totalIntensity > 0.0f && subtractiveAmbient < -EPSILON)
    {
        hasSubtractiveAmbient = true;
        allLightScale = (totalIntensity + subtractiveAmbient) / totalIntensity;
        allLightScale = rmt::Max(allLightScale, 0.0f);
    }
 
    // Based on the intensites computed in Target(), determine the brightest N
    // lights, where N is the current number of lights set in mTmpLightList
    // (numLights + 1). 
    PickBrightestLights(mTmpLightList);
    
    // Use the colour of the extra light to set the shadow transition.  This is
    // to make smooth transitions between shadow casters.  Shadows should be
    // dimmed to allow for a smooth introduction of the next shadow.
    tColour col = mTmpLightList->GetColour(numLights);
    int transition = rmt::Max(col.Red(), rmt::Max(col.Green(), col.Blue()));

    // Reduce and copy the N brightest lights to the returned light group.
    int i;
    for(i = 0; i < numLights; i++)
    {
        // Colour is colour of one of the bright shadow casters less the shadow transition.
        col = mTmpLightList->GetColour(i);
        int intensity = rmt::Max(col.Red(), rmt::Max(col.Green(), col.Blue()));
        intensity = rmt::Clamp(intensity - transition, 0, 255);
        if(hasSubtractiveAmbient)
        {
            intensity = rmt::FtoL( rmt::LtoF(intensity) * allLightScale);
        }

        if(intensity <= 0)
        {
            bestShadowCasters->SetLight(i, NULL); // zero intensity -> no shadow.
            continue;
        }

        // Get the next directional light to update.
        tDirectionalLight *dLight = dynamic_cast<tDirectionalLight*>(bestShadowCasters->GetLight(i));
        if(dLight == NULL)
        {
            dLight = new tDirectionalLight; // Create a new directional light if needed.
            bestShadowCasters->SetLight(i, dLight);
        }

        dLight->SetColour(tColour(intensity, intensity, intensity));

        dLight->SetDirection(mTmpLightList->GetDirection(i));
    }
}

// Use last computed set of lights to set bestLights.
// Return success of this operation.
bool tLightsChooser::GetCacheLights( tLightGroup *bestLights, const rmt::Vector& target,
                                     tLightsChooserCache *cache )
{
    bool result = false;

    if( cache != NULL )
    {
        if( target.Equals( cache->mTarget, cache->mDistance ) &&
            mCacheSynchronizeID == cache->mSynchronizeID )
        {
            // Also must not have any usefull animated lights this frame or last frame.
            if(!cache->UsesAnimatedLight() && Target(target, ANIMATED_LIGHT) <= 0.0f)
            {
                cache->SetLightGroup( bestLights );
                result = true;
            }
        }
    }

    return result;
}

bool tLightsChooser::GetCacheShadowCasters( tLightGroup *bestShadowCasters, const rmt::Vector& target,
                                            tLightsChooserCache *cache ) const
{
    bool result = false;
    
    if( cache != NULL )
    {
        if( target.Equals( cache->mTarget, cache->mDistance ) &&
            mCacheSynchronizeID == cache->mSynchronizeID )
        {
            cache->SetLightGroup( bestShadowCasters );
            result = true;
        }
    }

    return result;
}

// Set the count of a tLightGroup to be the number of non NULL
// light contained in the lightGroup.
void tLightsChooser::UpdateLightGroupCount(tLightGroup *lightGroup) const
{
    if(lightGroup == NULL)
    {
        return;
    }

    int count = 0;
    int i;
    for(i = 0; i < lightGroup->GetNumLights(); i++)
    {
        if(lightGroup->GetLight(i) == NULL)
        {
            break;
        }
        count++;
    }

    lightGroup->SetNumLights(count);
}

void tLightsChooser::EnableLights(tLightGroup *bestLights) const
{
    if(bestLights == NULL) 
    {
        return;
    }

    int i;
    for(i = 0; i < bestLights->GetNumLights(); i++)
    {
        bestLights->GetLight(i)->Enable(true);
    }
}

tLightsChooser::LightList* tLightsChooser::GetLightList(const tLight *light)
{
    LightList *lightList = NULL;
    if(light == NULL)
    {
        return lightList;
    }

    if(dynamic_cast<const tAmbientLight*>(light) != NULL)
    {
        lightList = mAmbLightList;
    }
    else if(dynamic_cast<const tDirectionalLight*>(light) != NULL)
    {
        lightList = mDirLightList;
    }
    else if(dynamic_cast<const tPointLight*>(light) != NULL)
    {
        lightList = mPntLightList;
    }
    else if(dynamic_cast<const tSpotLight*>(light) != NULL)
    {
        lightList = mSptLightList;
    }

    return lightList;
}

tLightsChooserCache::tLightsChooserCache( int maxNumLights ) :
    mMaxNumLights( maxNumLights ),
    mNumLights( 0 ),
    mNumWorldLights( 0 ),
    mSynchronizeID( -1 ),
    mDistance( DEFAULT_CACHE_DISTANCE ),
    mCache( NULL ),
    mTarget( 0.0f, 0.0f, 0.0f ),
    mAmbientColour( TCOLOUR_BLACK ),
    mUsesAnimatedLight( false )
{
    mMaxNumLights = rmt::Max( 0, mMaxNumLights );
    mCache = new pddiLightDesc[mMaxNumLights];
}

tLightsChooserCache::~tLightsChooserCache()
{
    delete[] mCache;
}

// Set this cache with the light of a tLightGroup.
void tLightsChooserCache::Set( tLightGroup *lightGroup,
                               const rmt::Vector& target,
                               int synchronizeID )
{
    int cacheCount = 0;
    mUsesAnimatedLight = false;
    
    int i;
    for( i = 0; i < lightGroup->GetNumLights() && cacheCount < mMaxNumLights; i++ )
    {
        tLight *light = lightGroup->GetLight( i );
        if( light == NULL )
        {
            continue; // No light.  Don't update cache.
        }

        // If any lights are animated, make the proper notation in cache
        if(light->IsAnimated())
        {
            mUsesAnimatedLight = true;
        }

        // Update cache with only directional, point and spotlights.
        tDirectionalLight *dirLight = NULL;
        tPointLight *pntLight = NULL;
        tSpotLight *sptLight = NULL; 

        if( ( dirLight = dynamic_cast<tDirectionalLight*>(light) ) != NULL )
        {
            rmt::Vector dir;
            dirLight->GetDirection( &dir.x, &dir.y, &dir.z );
            mCache[cacheCount].SetDirectionalLight( dirLight->GetColour(), &dir );
            cacheCount++;
        }
        else if( ( pntLight = dynamic_cast<tPointLight*>(light) ) != NULL )
        {
            float a, b, c;
            pntLight->GetAttenuation( &a, &b, &c );
            rmt::Vector pos;
            pntLight->GetPosition( &pos.x, &pos.y, &pos.z );
            mCache[cacheCount].SetPointLight( pntLight->GetColour(), &pos, a, b, c );
            cacheCount++;
        }
        else if( ( sptLight = dynamic_cast<tSpotLight*>(light) ) != NULL )
        {
            float a, b, c;
            sptLight->GetAttenuation( &a, &b, &c );
            float phi, theta, falloff;
            sptLight->GetCone( &phi, &theta, &falloff );
            rmt::Vector pos, dir;
            sptLight->GetPosition( &pos.x, &pos.y, &pos.z );
            sptLight->GetDirection( &dir.x, &dir.y, &dir.z );
            mCache[cacheCount].SetSpotLight( sptLight->GetColour(), &pos,
                                            &dir, phi, theta, falloff, a, b, c);
            cacheCount++;
        }
    }

    mNumLights = cacheCount;
    mTarget    = target;
    mSynchronizeID = synchronizeID;
}

// Update a lightGroup using the cached data.
void tLightsChooserCache::SetLightGroup(tLightGroup *lightGroup) const
{
    P3DASSERT(lightGroup != NULL);
    
    int i;
    for(i = 0; i < lightGroup->GetNumLights(); i++)
    {
        tLight *light = lightGroup->GetLight(i);
        
        if(GetNumLights() <= i)
        {
            // Out of cached lights, fill in the rest with NULL.
            if(light != NULL)
            {
                light->Enable( false );
            }
            lightGroup->SetLight(i, NULL);
            continue;  
        }

        switch(mCache[i].GetType())
        {
            case PDDI_LIGHT_DIRECTIONAL:
            {
                tDirectionalLight *dirLight = dynamic_cast<tDirectionalLight*>(light);
                if( dirLight == NULL )
                {
                    dirLight = new tDirectionalLight;
                    P3DASSERT( dirLight != NULL );
                    light = dirLight;
                }
                const pddiVector &dir = mCache[i].GetDirection();
                dirLight->SetDirection( dir.x, dir.y, dir.z );

                break;
            }
            case PDDI_LIGHT_POINT:
            {
                tPointLight *pntLight = dynamic_cast<tPointLight*>(light);
                if( pntLight == NULL )
                {
                    pntLight = new tPointLight;
                    P3DASSERT( pntLight != NULL );
                    light = pntLight;
                }
                const pddiVector &pos = mCache[i].GetPosition();
                pntLight->SetPosition( pos.x, pos.y, pos.z );
                float a, b, c;
                mCache[i].GetAttenuation( &a, &b, &c );
                pntLight->SetAttenuation( a, b, c );

                break;
            }
            case PDDI_LIGHT_SPOT:
            {
                tSpotLight *sptLight = dynamic_cast<tSpotLight*>(light);
                if( sptLight == NULL )
                {
                    sptLight = new tSpotLight;
                    P3DASSERT( sptLight != NULL );
                    light = sptLight;
                }
                const pddiVector &pos = mCache[i].GetPosition();
                sptLight->SetPosition( pos.x, pos.y, pos.z );
                const pddiVector &dir = mCache[i].GetDirection();
                sptLight->SetDirection( dir.x, dir.y, dir.z );
                float a, b, c;
                mCache[i].GetAttenuation( &a, &b, &c );
                sptLight->SetAttenuation( a, b, c );
                float phi, theta, falloff;
                mCache[i].GetCone( &phi, &theta, &falloff );
                mCache[i].SetCone( phi, theta, falloff );

                break;
            }
            default:
            {
                if(light != NULL)
                {
                    light->Enable( false );
                    light = NULL;
                }
            }
        } // end switch

        // Common attributes.
        if( light != NULL )
        {
            light->Enable( true );
            light->SetColour( mCache[i].GetColour() );
        }

        lightGroup->SetLight( i, light ); // Update the lightgroup.
    }

    lightGroup->SetNumLights(GetNumLights());
}

void tLightsChooserCache::SetDistance( float distance )
{
    mDistance = rmt::Max( 0.0f, distance );
}

tLightsChooser::LightList::LightList(int n) : mNumAllocated(n)
{
    mLightGroup = new tLightGroup(n);
    mLightGroup->AddRef();
    mLightInfo  = new LightInfo[n];
    int i;
    for(i = 0; i < n; i++)
    {
        mLightInfo[i].intensity  = 0.0f;
        mLightInfo[i].radiusSqr  = 0.0f;
        mLightInfo[i].mask       = 0;
    }
}

tLightsChooser::LightList::~LightList()
{
    mLightGroup->Release();
    delete[] mLightInfo;
}

void tLightsChooser::LightList::Clear()
{
    int i;
    for(i = mNumAllocated - 1; i >= 0; i--)
    {
        mLightInfo[i].intensity  = 0.0f;
        mLightInfo[i].radiusSqr  = 0.0f;
        mLightInfo[i].mask      = 0;
    }
    for(i = mLightGroup->GetNumLights() - 1; i >= 0; i--)
    {
        mLightGroup->SetLight(i, NULL);
    }
}

void tLightsChooser::LightList::SetLight(int n, tLight* light)
{
    mLightGroup->SetLight(n, light);

    mLightInfo[n].intensity = 0.0f;
    mLightInfo[n].colour = TCOLOUR_BLACK;
    mLightInfo[n].next = NULL;
    mLightInfo[n].mask = 0;

    if(light == NULL)
    {
        return;
    }

    // Find the square of the bounding spherical radius.
    if(light->GetDecayType() == tLight::DecayRange::SPHERE_DECAY_RANGE)
    {
        mLightInfo[n].radiusSqr = light->GetOuterDecayRange().x;
        mLightInfo[n].radiusSqr *= mLightInfo[n].radiusSqr;
    }
    else if(light->GetDecayType() == tLight::DecayRange::CUBOID_DECAY_RANGE)
    {
        mLightInfo[n].radiusSqr = light->GetOuterDecayRange().MagnitudeSqr();
    }
    else if(light->GetDecayType() == tLight::DecayRange::ELLIPSOID_DECAY_RANGE)
    {
        mLightInfo[n].radiusSqr = light->GetOuterDecayRange().MagnitudeSqr();
    }
    else // Any other range type treat as NO_DECAY_RANGE
    {
        mLightInfo[n].radiusSqr = BIG_NUM;
    }
    
    bool isShadowCaster = (light->GetIlluminationType() != NEGATIVE_ILLUMINANT) && light->IsShadowCaster();
    ShadowCaster(n, isShadowCaster);
    Animated(n, light->IsAnimated());
    IlluminationType(n, light->GetIlluminationType());
}

void tLightsChooser::LightList::SetColour(int n, tColour colour)
{
    P3DASSERT(n >= 0 && n < mNumAllocated);
        
    mLightInfo[n].colour = colour;
}

void tLightsChooser::LightList::SetRadiusSqr(int n, float radiusSqr)
{
    P3DASSERT(n >= 0 && n < mNumAllocated);
        
    mLightInfo[n].radiusSqr = radiusSqr;
}

void tLightsChooser::LightList::SetDirection(int n, const rmt::Vector &direction)
{
    P3DASSERT(n >= 0 && n < mNumAllocated);
        
    mLightInfo[n].direction = direction;
}

void tLightsChooser::LightList::SetIntensity(int n, float intensity)
{
    P3DASSERT(n >= 0 && n < mNumAllocated);
        
    mLightInfo[n].intensity = intensity;
}

void tLightsChooser::LightList::Animated(int n, bool animated)
{
    P3DASSERT(n >= 0 && n < mNumAllocated);

    if(animated)
    {
        mLightInfo[n].mask |= ANIMATED_LIGHT;
    }
    else
    {
        mLightInfo[n].mask &= ~ANIMATED_LIGHT;
    }
}

void tLightsChooser::LightList::ShadowCaster(int n, bool shadowCaster)
{
    P3DASSERT(n >= 0 && n < mNumAllocated);

    if(shadowCaster)
    {
        mLightInfo[n].mask |= SHADOW_CASTER;
    }
    else
    {
        mLightInfo[n].mask &= ~SHADOW_CASTER;
    }
}

void tLightsChooser::LightList::IlluminationType(int n, tLight::IlluminationType type)
{
    P3DASSERT(n >= 0 && n < mNumAllocated);

    mLightInfo[n].mask &= (~POSITIVE_ILLUMINANT & ~NEGATIVE_ILLUMINANT);

    if(type == tLight::POSITIVE_ILLUMINANT)
    {
        mLightInfo[n].mask |= POSITIVE_ILLUMINANT;
    }
    else if(type == tLight::NEGATIVE_ILLUMINANT)
    {
        mLightInfo[n].mask |= NEGATIVE_ILLUMINANT;
    }
}

float tLightsChooser::LightList::GetIntensity(int n)
{
    P3DASSERT(n >= 0 && n < mNumAllocated);
        
    return mLightInfo[n].intensity;
}    

tColour tLightsChooser::LightList::GetColour(int n)
{
    P3DASSERT(n >= 0 && n < mNumAllocated);
        
    return mLightInfo[n].colour;
}

float tLightsChooser::LightList::GetRadiusSqr(int n)
{
    P3DASSERT(n >= 0 && n < mNumAllocated);
        
    return mLightInfo[n].radiusSqr;
}

rmt::Vector tLightsChooser::LightList::GetDirection(int n)
{
    P3DASSERT(n >= 0 && n < mNumAllocated);
        
    return mLightInfo[n].direction;
}

bool  tLightsChooser::LightList::IsAnimated(int n)
{
    P3DASSERT(n >= 0 && n < mNumAllocated);
        
    return ((mLightInfo[n].mask & ANIMATED_LIGHT) == ANIMATED_LIGHT);
}

bool  tLightsChooser::LightList::IsNegative(int n)
{
    P3DASSERT(n >= 0 && n < mNumAllocated);
        
    return ((mLightInfo[n].mask & NEGATIVE_ILLUMINANT) == NEGATIVE_ILLUMINANT);
}

unsigned tLightsChooser::LightList::GetMask(int n)
{
    P3DASSERT(n >= 0 && n < mNumAllocated);
        
    return mLightInfo[n].mask;
}

// (tLightsChooser::LightInfo)* tLightsChooser::LightList::GetLightInfo(int n)
tLightsChooser::LightInfo *tLightsChooser::LightList::GetLightInfo(int n)
{
    P3DASSERT(n >= 0 && n < mNumAllocated);

    return &mLightInfo[n];
}

tLightsChooser::LightPriorityQueue::LightPriorityQueue() : mDepth(1)
{
    mBlank.intensity = 0.0f;
    mBlank.colour    = TCOLOUR_BLACK;
    mBlank.next      = &mBlank;
    mHead = &mBlank;
}
    
void tLightsChooser::LightPriorityQueue::Reset()
{
    mHead = &mBlank;
}

void tLightsChooser::LightPriorityQueue::Add(LightInfo *light)
{
    int i;
    LightInfo *current = mHead;
    LightInfo *prev    = NULL;
    for(i = 0; i < mDepth; i++)
    {
        if(light->intensity > current->intensity)
        {
            light->next = current;
            if(prev==NULL)
            {
                mHead = light;
            }
            else
            {
                prev->next = light;
            }
            break;
        }
        prev    = current;
        current = current->next;
    }
}

void tLightsChooser::LightPriorityQueue::SetDepth(int depth)
{
    mDepth = depth;
}

int tLightsChooser::LightPriorityQueue::GetDepth() const
{
    return mDepth;
}    


