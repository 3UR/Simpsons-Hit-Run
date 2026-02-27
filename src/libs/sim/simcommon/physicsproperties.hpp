
#ifndef _PHYSICSOBJECTPROPERTIES_HPP_
#define _PHYSICSOBJECTPROPERTIES_HPP_


#include <simcommon/simenvironment.hpp>

class tEntityStore;

namespace sim
{

class PhysicsProperties
    : public tEntity,
    protected SimUnits
{
public:
    PhysicsProperties();
    
    // see comments in the source for understanding of the parameters
    void SetFrictCoeffCGS(float f);
    void SetRestCoeffCGS(float f);
    void SetTangRestCoeffCGS(float f);
    void SetDensityCGS(float d);
    
    float FrictCoeff()         const { return mFrictCoeff; }
    float Density()            const { return mDensity; }
    float RestCoeff()          const { return mRestCoeff; }
    float TangRestCoeff()      const { return mTangRestCoeff; }
    
    float FrictCoeffCGS()      const { return mFrictCoeffCGS; }
    float DensityCGS()         const { return mDensityCGS; }
    float RestCoeffCGS()       const { return mRestCoeff; }
    float TangRestCoeffCGS()   const { return mTangRestCoeff; }
    
    PhysicsProperties &operator= ( const PhysicsProperties &in_param )
    {
        SetRestCoeffCGS(in_param.mRestCoeff);
        SetTangRestCoeffCGS(in_param.mTangRestCoeff);
        SetFrictCoeffCGS(in_param.mFrictCoeffCGS);
        SetDensityCGS(mDensityCGS);
        Update();
        return *this;
    }
    
    static PhysicsProperties* FindPhysicsProperty(const char* stringData, tEntityStore* store = NULL);
    static PhysicsProperties* FindPhysicsProperty(tUID uid, tEntityStore* store = NULL);
    
    static PhysicsProperties* DefaultPhysicsProperties(tEntityStore* store=NULL);
    static PhysicsProperties* DefaultArtPhysicsProperties(tEntityStore* store=NULL);

    static PhysicsProperties* HardWoodProperties(tEntityStore* store=NULL);
    static PhysicsProperties* SoftWoodProperties(tEntityStore* store=NULL);
    static PhysicsProperties* HardRubberProperties(tEntityStore* store=NULL);
    static PhysicsProperties* SoftRubberProperties(tEntityStore* store=NULL);
    static PhysicsProperties* HardMetalProperties(tEntityStore* store=NULL);
    static PhysicsProperties* SoftMetalProperties(tEntityStore* store=NULL);
    static PhysicsProperties* ConcreteProperties(tEntityStore* store=NULL);
    static PhysicsProperties* StickyProperties(tEntityStore* store=NULL);
    static PhysicsProperties* IcyProperties(tEntityStore* store=NULL);

    static float CombinedRestCoeff(const PhysicsProperties* inProperties1, const PhysicsProperties* inProperties2);
    static float CombinedTangRestCoeff(const PhysicsProperties* inProperties1, const PhysicsProperties* inProperties2);
    static float CombinedFrictCoeff(const PhysicsProperties* inProperties1, const PhysicsProperties* inProperties2);
    
protected:
    static PhysicsProperties* GetPhysicsProperties(tEntityStore* store, const char* inName, float inRest, float inFrict, float inTangRest, float density = 1.0f);

    virtual void Update();
    
    float mFrictCoeff;
    float mDensity;
    float mRestCoeff;             // no units
    float mTangRestCoeff;         // no units
    
    float mFrictCoeffCGS;         // 1 / sec
    float mDensityCGS;            // gram / cm^3
    
    virtual ~PhysicsProperties();
};

} // sim

#endif // _PHYSICSOBJECTPROPERTIES_HPP_
