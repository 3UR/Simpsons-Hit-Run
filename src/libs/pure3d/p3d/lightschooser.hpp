//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _LIGHTSCHOOSER_HPP
#define _LIGHTSCHOOSER_HPP

/*
  class tLightsChooser

  Purpose:
  Reduce the number of lights in a scene while maintaining similar appearance.

  Detail:
  Given a set of tLights and a position, this class will create a new set of
  tLights to mimic the original lights at that position.  Typically you will
  want to have it create 4 tDirecitonalLight's - this combination of lights works
  well on all platforms and does a good job at representing the original lights.  
  Key class attributes:
  (1) make it look like the real lights
  (2) no popping
  (3) reduce to the original lights if possible
  (4) do it fast

  To set up, add your lights with AddLight() and/or AddLights() (be sure the ones
  you want to use are enabled).  Get the simplified set of lights with
  GetBestLights().  You can improve on the performance by using
  tLightsChooserCache to cache the results.  tLightschooser does not keep record
  of the cache object so you will need to do this yourself.

  You can also find the best 1 or 2 lights for casting shadows with
  GetShadowCasters().  The returned intensity should be used to set the shadow
  wash colour.  As with GetBestLights(), you can use a (separate) cache to improve
  performance.
*/

#include <p3d/entity.hpp>
#include <p3d/error.hpp>
#include <p3d/light.hpp>

class tAmbientLight;
class tDirectionalLight;
class tLightGroup;
class tPointLight;
class tSpotLight;

class tLightsChooserCache : public tEntity
{
public:
    tLightsChooserCache(int maxNumLights );

    // Number of reduced lights in this cache.
    int GetNumLights() const { return mNumLights; }
    // Number of USEFULL world lights used to build cache.
    // This includes those not too dim nor too far.
    int GetNumWorldLights() const { return mNumWorldLights; }
    // Don't update till this far away from previous results.
    void SetDistance( float distance );
    
    tColour GetAmbientColour() const { return mAmbientColour; }
    bool UsesAnimatedLight() const { return mUsesAnimatedLight; }

protected:
    virtual ~tLightsChooserCache();

    void Set(tLightGroup *lightGroup, const rmt::Vector& target, int synchronizeID); 
    void SetLightGroup(tLightGroup *lightGroup) const;

    int mMaxNumLights;   // Allocated space for cache lights.
    int mNumLights;      // Current number cache lights.
    int mNumWorldLights; // Number lights used to build cache.
    int mSynchronizeID;  // ID used to verfy up to date lighting.
    float mDistance;     // Distance till next update.
    
    class pddiLightDesc *mCache; // Description of lights in cache.
    rmt::Vector mTarget; // Previous target position.
    tColour mAmbientColour;
    bool mUsesAnimatedLight;

    friend class tLightsChooser;
};

class tLightsChooser : public tEntity
{
public:

    static const unsigned DEFAULT_MAX_NUM_LIGHTS;
    static const unsigned DEFAULT_MAX_NUM_SHADOW_CASTERS;

    tLightsChooser(int maxNumLights = DEFAULT_MAX_NUM_LIGHTS,
                   int maxNumShadowCasters = DEFAULT_MAX_NUM_SHADOW_CASTERS);
    
    // Control which world lights to use in determining the best
    // lights.
    void AddLight(tLight* light);
    void AddLights(tLightGroup* lights);
    void RemoveLight(tLight* oldLight);
    void RemoveAllLights();

    // Create a set of lights to be be returned in bestLights.  This
    // will create at most bestLights->GetNumLights() lights so be
    // sure to set this before calling the function.  
    void GetBestLights(tLightGroup* bestLights,    
                       const rmt::Vector& target,
                       tLightsChooserCache *cache = NULL);

    // Update bestShadowCasters with lights whose 'colour' corresponde
    // to the intensity of the shadow.  This can then be passed into the
    // ShadowGenerator as the wash colour.  Note that the brighter the colour
    // returned, the dimmer the shadow.  A colour of white will
    // produce no shadow.  The darkness of the shadow can be modified locally by
    // setting shadowScale and/or globally by using SetShadowContrast()
    void GetShadowCasters(tLightGroup* bestShadowCasters,
                          const rmt::Vector& target,
                          tLightsChooserCache *cache = NULL,
                          float shadowScale = 1.0f);
        
    // Set the global darkness of shadows. 1.0=dark - 0.0=light.
    void SetShadowContrast(float shadowContrast);

    tColour GetAmbientColour() const; // Last computed ambient.
    int GetNumLights() const; // Total number world lights.
    int GetMaxNumLights() const;
    int GetMaxNumShadowCasters() const;

    //==========================================================================
  protected:
    virtual ~tLightsChooser(void);

    // Helper structures & classes
    struct LightInfo;
    class LightList;        
    class LightPriorityQueue;

    friend class LightList;

    void ComputeAmbientColour(float ambientRGB[3], const rmt::Vector &target);
    float TargetDirLights(const rmt::Vector &target, unsigned mark);
    float TargetPntLights(const rmt::Vector &target, unsigned mark);
    float TargetSptLights(const rmt::Vector &target, unsigned mark);
    float Target(const rmt::Vector &target, unsigned mark = 0);
    float PickBrightestLights(LightList *brightestLights);
    void  SelectLights(tLightGroup *bestLights, float ambientRGB[3],
                       const rmt::Vector &target);
    void  SelectShadowCasters(tLightGroup *bestLights, float ambient[3], const rmt::Vector &target);
    bool GetCacheLights(tLightGroup *bestLights, const rmt::Vector& target,
                        tLightsChooserCache *cache); 
    bool GetCacheShadowCasters(tLightGroup *bestShadowCasters, const rmt::Vector& target,
                               tLightsChooserCache *cache) const; 
    void ScaleShadowCasters(tLightGroup *bestShadowCasters, float shadowScale);
    void UpdateLightGroupCount(tLightGroup *lightGroup) const;
    void EnableLights(tLightGroup *bestLights) const;

    tColour mAmbientColour;
    float   mShadowContrast;     // Darkness on shadows.
    int     mNumImportantLights;
    int     mMaxNumLights;
    int     mMaxNumShadowCasters;
    long    mCacheSynchronizeID; // Incremented on each light update.

    LightList* GetLightList(const tLight *light); // Get the appropriate list based on light type.

    LightList *mAmbLightList;
    LightList *mDirLightList;
    LightList *mPntLightList;
    LightList *mSptLightList;
    LightList *mTmpLightList;

    LightPriorityQueue *mPQueue;
}; // class tLightsChooser

struct tLightsChooser::LightInfo
{
    float intensity;
    tColour colour;
    float radiusSqr;
    rmt::Vector direction;
    unsigned mask;
    struct LightInfo *next;
};

class tLightsChooser::LightList
{
  public:
    LightList(int n = DEFAULT_MAX_NUM_LIGHTS);
    ~LightList();
    
    void Clear();
    
    void SetNumLights(int n) { mLightGroup->SetNumLights(n); }
    void SetLight(int n, tLight* light);
    
    void SetIntensity(int n, float intensity);
    void SetColour(int n, tColour colour);
    void SetRadiusSqr(int n, float radiusSqr);
    void SetDirection(int n, const rmt::Vector &direction);
    void Animated(int n, bool animated);
    void ShadowCaster(int n, bool shadowCaster);
    void IlluminationType(int n, tLight::IlluminationType type);
    
    tLight* GetLight(int n) { return mLightGroup->GetLight(n); }
    int GetNumLights() { return mLightGroup->GetNumLights(); }
    
    float GetIntensity(int n);
    tColour GetColour(int n);
    float GetRadiusSqr(int n);
    rmt::Vector GetDirection(int n);
    bool  IsAnimated(int n);
    bool  IsNegative(int n);
    unsigned GetMask(int n);
    
    struct LightInfo* GetLightInfo(int n);
    
  private:
    tLightGroup *mLightGroup;
    LightInfo *mLightInfo;
    int mNumAllocated;
};

class tLightsChooser::LightPriorityQueue
{
  public:
    LightPriorityQueue();
    
    void Add(LightInfo *light);
    void Reset();
    LightInfo* Head() {return mHead;}
    
    void SetDepth(int depth);
    int  GetDepth() const;
    
  private:
    LightInfo *mHead;
    LightInfo mBlank;
    int       mDepth;
};

#endif //_LIGHTSCHOOSER_HPP

