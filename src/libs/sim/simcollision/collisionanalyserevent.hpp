#ifndef _COLLISIONANALYSEREVENT_HPP_
#define _COLLISIONANALYSEREVENT_HPP_

#include "simcollision\collisionanalyserinfo.hpp"

namespace sim
{

enum CollisionAnalyserEventEnum {
    cCollisionAnalyserEventUnknown=0,
    cCollisionAnalyserEventBounce,
    cCollisionAnalyserEventStartInteraction,
    cCollisionAnalyserEventEndInteraction
};

class CollisionAnalyserEvent {
public:
    CollisionAnalyserEventEnum mEvent;
    CollisionAnalyserInfo mInfo;

    CollisionAnalyserEvent():
    mEvent(cCollisionAnalyserEventUnknown),
    mInfo()
    {
    }

    CollisionAnalyserEvent(const CollisionAnalyserEvent& event):
    mEvent(event.mEvent),
    mInfo(event.mInfo)
    {
    }

    CollisionAnalyserEvent(CollisionAnalyserEventEnum newEvent, CollisionAnalyserInfo &newInfo):
    mEvent(newEvent),
    mInfo(newInfo)
    {
    }

    CollisionAnalyserEvent& operator=(const CollisionAnalyserEvent& rhs);
    ~CollisionAnalyserEvent()
    {
    }
};

}

#endif