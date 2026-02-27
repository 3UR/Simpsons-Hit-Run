
#include "simcommon/trajectoryextrapolator.hpp"
#include "simcommon/simenvironment.hpp"

using namespace RadicalMathLibrary;

namespace sim
{

// assumes gravity along Y for optimization


TrajectoryExtrapolator::TrajectoryExtrapolator()
    : mTime(0),
    mSimEnvironment(NULL)
{
    mCurrentPosition.Clear();
    mCurrentSpeed.Clear();
    SetSimEnvironment(SimEnvironment::GetDefaultSimEnvironment());
}

TrajectoryExtrapolator::~TrajectoryExtrapolator()
{
    if (mSimEnvironment)
        mSimEnvironment->Release();
}

void TrajectoryExtrapolator::SetSimEnvironment(SimEnvironment* inSimEnvironment)
{
    if (mSimEnvironment)
        mSimEnvironment->Release();
    
    mSimEnvironment = inSimEnvironment;
    
    if (mSimEnvironment)
        mSimEnvironment->AddRef();
}

void TrajectoryExtrapolator::Init(const rmt::Vector& inInitPosition, const rmt::Vector& inFinalPosition, float inTimeSecond)
{
    rAssert(inTimeSecond > 0 && mSimEnvironment);
    
    Vector dp;
    dp.Sub(inFinalPosition, inInitPosition);
    
    float verticalDist = dp.y;
    dp.y = 0;
    
    float it = 1.0f / inTimeSecond;
    
    mCurrentSpeed.Scale(it, dp);
    mCurrentSpeed.y = -mSimEnvironment->GravityNorm() * Sqr(inTimeSecond) / 2.0f;
    mCurrentSpeed.y = (verticalDist - mCurrentSpeed.y) * it;
    
    mCurrentPosition = inInitPosition;
    mTime = inTimeSecond;
}

void TrajectoryExtrapolator::Update(float dt)
{
    if (mTime <= dt)
        return;
    
    // simple euler
    // v(t+dt) = v(t) + a * dt
    // r(t+dt) = r(t) + [v(t) + v(t+dt)]/2 * dt where [v(t) + v(t+dt)]/2 ~= v(t+dt/2)
    
    Vector dr = mCurrentSpeed;
    mCurrentSpeed.y -= dt * mSimEnvironment->GravityNorm();
    dr.Add(mCurrentSpeed);
    dr.Scale(dt * 0.5f);
    mCurrentPosition.Add(dr);
    
    mTime -= dt;
    
}

void TrajectoryExtrapolator::DebugDisplay()
{
    //   dLine3(mCurrentPosition, 0.5f, tColour(255, 255, 255));
}

} // sim
