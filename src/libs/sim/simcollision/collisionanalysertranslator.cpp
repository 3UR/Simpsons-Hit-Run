#include "simcollision/collisionanalysertranslator.hpp"
#include "simcommon/simstate.hpp"

namespace sim
{

float CollisionAnalyserTranslator::GetCheapBounceSoundVolume(
    CollisionAnalyserData &data, CollisionAnalyserInfo &info)
{
    return rmt::Min(1.0f, rmt::Fabs(info.mDvN) / 6.0f);
}

float CollisionAnalyserTranslator::GetBounceSoundVolume(
    CollisionAnalyserData &data, CollisionAnalyserInfo &info)
{
    float n = info.mDvN / 6.0f;
    float t = info.mDvT / 24.0f;
    return rmt::Min(1.0f, rmt::Sqrt(n*n + t*t));
}

float CollisionAnalyserTranslator::GetSlidingSoundVolume(
    CollisionAnalyserData &data, CollisionAnalyserInfo &info)
{
    float volume = info.mDvT / 6.0f;
    
    return rmt::Min(1.0f, volume * info.mSlidingFactor);
}

float CollisionAnalyserTranslator::GetRollingASoundVolume(
    CollisionAnalyserData &data, CollisionAnalyserInfo &info)
{
    float volume = data.mSimStateA->GetAngularVelocity().Magnitude() / 4.0f;
    
    return rmt::Min(1.0f, volume * info.mRollingFactor);
}

float CollisionAnalyserTranslator::GetRollingBSoundVolume(
    CollisionAnalyserData &data, CollisionAnalyserInfo &info)
{
    float volume = data.mSimStateB->GetAngularVelocity().Magnitude() / 4.0f;
    
    return rmt::Min(1.0f, volume * info.mRollingFactor);
}

float CollisionAnalyserTranslator::GetSpinningASoundVolume(
    CollisionAnalyserData &data, CollisionAnalyserInfo &info)
{
    return rmt::Min(0.4f, info.mSpinSpeedA / 400.0f);
}

float CollisionAnalyserTranslator::GetSpinningBSoundVolume(
    CollisionAnalyserData &data, CollisionAnalyserInfo &info)
{
    return rmt::Min(0.4f, info.mSpinSpeedB / 400.0f);
}

}