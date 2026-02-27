#ifndef _RESTDETECTOR_HPP_
#define _RESTDETECTOR_HPP_

#include "simcommon/kalmann.hpp"

namespace sim
{

class PhysicsObject;

class PhysicsRestingDetector
{
public:
    void TestRestingCondition(float in_dt);      // Test if resting criteral are satisfied.
    bool UpdateRestingCondition( float in_dt );  // Advance in time the filter.
    bool IsAtRest() const 
    { 
        return mAtRest; 
    }
    void SetNeedReset() { mNeedToComputeTypicalValues = true; mAtRest = false;}
    void SetRestingSensitivity ( float in_Sensitivity ) { mPeriodSensitivity   = in_Sensitivity;}
    void SetLinSpeedSensitivity( float in_Sensitivity ) { mLinSpeedSensitivity = in_Sensitivity;}
    void SetAngSpeedSensitivity( float in_Sensitivity ) { mAngSpeedSensitivity = in_Sensitivity;}
    
protected:
    friend class PhysicsObject;
    friend class ArticulatedPhysicsObject;
    
    PhysicsRestingDetector();
    ~PhysicsRestingDetector();
    
    bool ResetRestingCondition( );   // Restart the filter.

    inline float GetTypicalPeriod()   const { return mTypicalPeriod; }
    inline float GetTypicalLinSpeed() const { return mTypicalLinSpeed; }
    inline float GetTypicalLength()   const { return mTypicalLength; }
    inline float GetTypicalAngSpeed() const { return mTypicalAngSpeed; }
    
    void SetObj( const PhysicsObject* inObj);
    
    bool ComputeTypicalValues( float in_dt );
    void AffectTypicalValuesByScaleFactor(float inScaleFactor)
    {
        mTypicalLength*=inScaleFactor;
    }
    
private:
    
    SimpleFilterAbs3D mLinRestCond;        // Linear absolute value speed filtering.
    SimpleFilter1D    mAngRestCond1D_mag;  // Magnetude of angular speed filtering,
    
    float mTypicalLinSpeed;
    float mTypicalAngSpeed;
    float mTypicalLength;
    float mTypicalPeriod;
    float mPeriodSensitivity;    //Sensitivity factor to control time need to fall in resting (linear domain)..
    float mLinSpeedSensitivity;  //Sensitivity factor to control time need to fall in resting (linear domain)..
    float mAngSpeedSensitivity;  //Sensitivity factor to control time need to fall in resting (angular domain)..
    
    bool  mNeedToComputeTypicalValues;
    bool  mAtRest;
    
    const PhysicsObject* mPhysicsObject;
    
};

} // sim

#endif // _RESTDETECTOR_HPP_
