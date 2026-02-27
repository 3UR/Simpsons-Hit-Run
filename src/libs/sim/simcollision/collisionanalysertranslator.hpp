#ifndef _COLLISIONANALYSERTRANSLATOR_HPP_
#define _COLLISIONANALYSERTRANSLATOR_HPP_

#include "simcollision/collisionanalyserinfo.hpp"
#include "simcollision/collisionanalyserdata.hpp"

namespace sim
{

class CollisionAnalyserTranslator {
public:
    static float GetCheapBounceSoundVolume(CollisionAnalyserData &data, CollisionAnalyserInfo &info);
    static float GetBounceSoundVolume(CollisionAnalyserData &data, CollisionAnalyserInfo &info);
    static float GetSlidingSoundVolume(CollisionAnalyserData &data, CollisionAnalyserInfo &info);
    static float GetRollingASoundVolume(CollisionAnalyserData &data, CollisionAnalyserInfo &info);
    static float GetRollingBSoundVolume(CollisionAnalyserData &data, CollisionAnalyserInfo &info);
    static float GetSpinningASoundVolume(CollisionAnalyserData &data, CollisionAnalyserInfo &info);
    static float GetSpinningBSoundVolume(CollisionAnalyserData &data, CollisionAnalyserInfo &info);
};

}

#endif