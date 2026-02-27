
#ifndef _TRAJECTORY_EXTRAPOLATOR_HPP_
#define _TRAJECTORY_EXTRAPOLATOR_HPP_

#include <radmath/radmath.hpp>

namespace sim
{

class SimEnvironment;

class TrajectoryExtrapolator
{
public:
    TrajectoryExtrapolator();
    ~TrajectoryExtrapolator();
    void SetSimEnvironment(SimEnvironment* INSimEnvironment);
    void Init(const rmt::Vector& inInitPosition, const rmt::Vector& inFinalPosition, float inTimeSecond);
    void Update(float dt);
    
    const rmt::Vector& GetCurrentPos() const { return mCurrentPosition; }
    const rmt::Vector& GetCurrentSpeed() const { return mCurrentSpeed; }
    void DebugDisplay();
    
private:
    rmt::Vector mCurrentPosition;
    rmt::Vector mCurrentSpeed;
    float mTime;
    float mGravityY;
    
    SimEnvironment* mSimEnvironment;
};

} // sim

#endif // _TrajectoryExtrapolator_hpp_