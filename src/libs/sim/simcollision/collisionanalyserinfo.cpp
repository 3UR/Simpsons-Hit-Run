#include "simcollision\collisionanalyserinfo.hpp"

namespace sim
{

float CollisionAnalyserInfo::sDefaultBounceSpeedThreshold = 2.0f;
float CollisionAnalyserInfo::sDefaultBounceTimeThreshold = 0.2f;
float CollisionAnalyserInfo::sDefaultRollingThreshold = 3.2f;
float CollisionAnalyserInfo::sDefaultLowerThresholdFactor = 0.8f;
float CollisionAnalyserInfo::sDefaultUpperThresholdFactor = 2.0f;

CollisionAnalyserInfo::CollisionAnalyserInfo(void) :
mState(cCollisionAnalyserStateUnknown),
    mCollisionPosition(0, 0, 0),
    mCollisionNormal(0, 0, 0),
    mStartTime(0),
    mSlidingFactor(0),
    mRollingFactor(0),
    mSpinSpeedA(0),
    mSpinSpeedB(0),
    mSpeedDiff(0, 0, 0),
    mDvN(0),
    mDvT(1),
    mSpeedA(0),
    mSpeedB(0),
    mBounceSpeedThreshold(sDefaultBounceSpeedThreshold),
    mBounceTimeThreshold(sDefaultBounceTimeThreshold),
    mRollingThreshold(sDefaultRollingThreshold),
    mLowerThreshold(sDefaultRollingThreshold * sDefaultLowerThresholdFactor),
    mUpperThreshold(sDefaultRollingThreshold * sDefaultUpperThresholdFactor)
{
}

CollisionAnalyserInfo::CollisionAnalyserInfo(const CollisionAnalyserInfo& info) :
mState(info.mState),
    mCollisionPosition(info.mCollisionPosition),
    mCollisionNormal(info.mCollisionNormal),
    mStartTime(info.mStartTime),
    mSlidingFactor(info.mSlidingFactor),
    mRollingFactor(info.mRollingFactor),
    mSpinSpeedA(info.mSpinSpeedA),
    mSpinSpeedB(info.mSpinSpeedB),
    mSpeedDiff(info.mSpeedDiff),
    mDvN(info.mDvN),
    mDvT(info.mDvT),
    mSpeedA(info.mSpeedA),
    mSpeedB(info.mSpeedB),
    mBounceSpeedThreshold(info.mBounceSpeedThreshold),
    mBounceTimeThreshold(info.mBounceTimeThreshold),
    mRollingThreshold(info.mRollingThreshold),
    mLowerThreshold(info.mLowerThreshold),
    mUpperThreshold(info.mUpperThreshold)
{
}

void CollisionAnalyserInfo::SetBounceSpeedThreshold(float threshold)
{
    if(threshold <= 0)
        return;
    mBounceSpeedThreshold = threshold;
}

void CollisionAnalyserInfo::SetBounceTimeThreshold(float threshold)
{
    if(threshold <= 0)
        return;
    mBounceTimeThreshold = threshold;
}

void CollisionAnalyserInfo::SetRollingThreshold(float threshold)
{
    if(threshold <= 0)
        return;
    
    //get old factors
    float lowerFactor = mLowerThreshold / mRollingThreshold;
    float upperFactor = mUpperThreshold / mRollingThreshold;
    
    //set new thresholds
    mRollingThreshold = threshold;
    mLowerThreshold = mRollingThreshold * lowerFactor;
    mUpperThreshold = mRollingThreshold * upperFactor;
}

void CollisionAnalyserInfo::SetLowerThresholdFactor(float factor)
{
    if(factor >= 1.0f || factor <= 0)
        return;
    mLowerThreshold = mRollingThreshold * factor;
}

void CollisionAnalyserInfo::SetUpperThresholdFactor(float factor)
{
    if(factor <= 1.0f)
        return;
    mUpperThreshold = mRollingThreshold * factor;
}

}