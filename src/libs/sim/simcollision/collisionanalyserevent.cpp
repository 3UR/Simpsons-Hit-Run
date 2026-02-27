#include "simcollision\collisionanalyserevent.hpp"

namespace sim
{

CollisionAnalyserEvent& CollisionAnalyserEvent::operator=(const CollisionAnalyserEvent& rhs)
{
    if(this == &rhs)
        return *this;
    
    mEvent = rhs.mEvent;
    mInfo = rhs.mInfo;
    
    return *this;
}

}