#ifndef _COLLISIONANALYSERINFO_HPP_
#define _COLLISIONANALYSERINFO_HPP_

#include <radmath/radmath.hpp>

namespace sim
{

enum CollisionAnalyserStateEnum {
    cCollisionAnalyserStateUnknown=0,
    cCollisionAnalyserStateBouncing,
    cCollisionAnalyserStateInteraction};
    
//class CollisionAnalyserInfo : public tRefCounted {
class CollisionAnalyserInfo {
public:
    CollisionAnalyserInfo(void);
    CollisionAnalyserInfo(const CollisionAnalyserInfo& info);
    
    //these calls affect the result of the event calculations
    void SetBounceSpeedThreshold(float threshold);
    void SetBounceTimeThreshold(float threshold);
    void SetRollingThreshold(float threshold);
    void SetLowerThresholdFactor(float factor);
    void SetUpperThresholdFactor(float factor);
    
    CollisionAnalyserStateEnum mState;  //what this pair is doing
    rmt::Vector mCollisionPosition;
    rmt::Vector mCollisionNormal;
    float mStartTime;     //when this pair started doing the current interaction
    float mSlidingFactor; //how much this object is sliding (0.0 - 1.0)
    float mRollingFactor; //how much this object is rolling (0.0 - 1.0)
    float mSpinSpeedA;    //how much the first object in this pair is spinning
    float mSpinSpeedB;    //how much the second object in this pair is spinning
    
    //these next values are averaged results of the most recent frames.
    //they are not the EXACT values for the single most recent frame.
    //this is done to smooth out abnormalities in the collision results.
    rmt::Vector mSpeedDiff;  //the difference in speed between the two objects (only calculated if some sort of interaction is being calculated)
    float mDvN;       //the speed of the collision, parallel to the collision normal
    float mDvT;       //the speed of the collision, perpendicular to the collision normal
    float mSpeedA;    //the approximate speed of the first object (only calculated if spinning is being calculated)
    float mSpeedB;    //the approximate speed of the second object (only calculated if spinning is being calculated)
    
    //these are the default values that will be used for new instances
    //of this class. feel free to change them as necessary.
    static float sDefaultBounceSpeedThreshold;
    static float sDefaultBounceTimeThreshold;
    static float sDefaultRollingThreshold;
    static float sDefaultLowerThresholdFactor;
    static float sDefaultUpperThresholdFactor;
private:
    friend class CollisionAnalyserData;
    
    //these values affect the result of the event calculations.
    //they have corresponding Set methods in the public interface.
    float mBounceSpeedThreshold;
    float mBounceTimeThreshold;
    float mRollingThreshold;
    float mLowerThreshold;
    float mUpperThreshold;
};

}

#endif