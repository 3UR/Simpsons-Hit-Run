//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _LIGHT_HPP
#define _LIGHT_HPP

#include <p3d/entity.hpp>
#include <p3d/loadmanager.hpp>
#include <p3d/array.hpp>

class tView;

class tLight : public tEntity
{
public:
    tLight();
    tLight(tLight *light);

    virtual void Activate(unsigned slot);
    virtual void Deactivate(void);

    void SetColour(tColour colour);
    tColour GetColour();

    virtual void SetPosition(const rmt::Vector& d) { SetPosition(d.x, d.y, d.z); }
    virtual void SetPosition(float x, float y, float z);
    void GetPosition(float* x, float* y, float* z);
    const rmt::Vector& GetPosition(void) { return position; }

    void Enable(bool e);
    bool IsEnabled()    { return enabled; }
    void SetShadowCaster(bool IsShadowCaster) { isShadowCaster = IsShadowCaster; }
    bool IsShadowCaster(void) { return isShadowCaster; }
    
    void Animated(bool a);
    bool IsAnimated() { return animated; }

    struct DecayRange{
        enum Type{ NO_DECAY_RANGE, SPHERE_DECAY_RANGE,
                   CUBOID_DECAY_RANGE, ELLIPSOID_DECAY_RANGE };
        
        DecayRange() : type(NO_DECAY_RANGE),
            inner(0.0f, 0.0f, 0.0f), outer(0.0f, 0.0f, 0.0f), sinRotationY(0.0f),
            cosRotationY(1.0f) {/**/};
        DecayRange(const DecayRange &r)
            : type(r.type), inner(r.inner), outer(r.outer), 
            sinRotationY(r.sinRotationY), cosRotationY(r.cosRotationY) {/**/};
        
        Type   type;
        rmt::Vector inner; // Inner range.
        rmt::Vector outer; // Outer range.
        float sinRotationY; // Store sine and cosine rather than angle of rotation
        float cosRotationY; // to avoid repeated trig calls.
    };

    void SetDecayType(DecayRange::Type type);
    tLight::DecayRange::Type GetDecayType();
    void SetDecayRange(const rmt::Vector &inner, const rmt::Vector &outer);
    void SetDecayRotationY(float rotationY);
    rmt::Vector GetInnerDecayRange();
    rmt::Vector GetOuterDecayRange();
    float GetDecayRotationY() const;
    float Decay(const rmt::Vector &pos);  // Compute the decay at pos.

    // Lights illumination can be positive (standard light), zero (shadow only),
    // or negative (remove illumination in area of influence).  Currently only
    // supported through the tLightsChooser class.
    enum IlluminationType{ POSITIVE_ILLUMINANT, ZERO_ILLUMINANT, NEGATIVE_ILLUMINANT };

    void SetIlluminationType(IlluminationType type);
    IlluminationType GetIlluminationType() { return illuminationType; }

protected:
    virtual ~tLight();

    virtual void Update(void) = 0;

    tColour colour;
    rmt::Vector position;
    unsigned slot;
    bool active;
    bool enabled;
    bool isShadowCaster;
    bool animated;
    DecayRange decayRange;
    IlluminationType illuminationType;
};

//Loads lights from the 32-bit chunks
//-----------------------------------------------
class tLightLoader : public tSimpleChunkHandler
{
public:
    tLightLoader();
protected:
    ~tLightLoader() {};
    tEntity* LoadObject(tChunkFile*, tEntityStore*);
};

//-----------------------------------------------
class tLightGroup : public tEntity
{
public:
    tLightGroup();
    tLightGroup(int n);

    void    SetViewLights(tView *view);
    void    SetLight(int n, tLight* light);
    void    SetNumLights(int n);

    int     GetNumLights(void) { return mCurNumLights;}
    tLight* GetLight(int n) { return (n < mNumLights) ? mLights[n] : NULL;}

private:
    virtual ~tLightGroup();

    int mNumLights;
    int mCurNumLights;
    tPtrArray<tLight *> mLights;

    friend class tLightGroupLoader;
};

//-----------------------------------------------
class tLightGroupLoader : public tSimpleChunkHandler
{
public:
    tLightGroupLoader();

protected:
    ~tLightGroupLoader() {};
    tEntity* LoadObject(tChunkFile*, tEntityStore*);
};


#endif //_LIGHT_HPP

