/*
 * choreo/footplanter.hpp
 */

#ifndef CHOREO_FOOTPLANTER_HPP
#define CHOREO_FOOTPLANTER_HPP


#include <poser/posedriver.hpp>


namespace choreo
{


class Rig;
class RootBlender;
class FootBlender;


class FootPlanter: public poser::PoseDriver
{
public:

    FootPlanter(Rig* rig,
                RootBlender* rootBlender,
                FootBlender** footBlenders,
                int balanceRootIndex);

    float GetBalanceRootMinYRate() const
        { return m_BalanceRootMinYRate; }
    void SetBalanceRootMinYRate(float minYRate)
        { m_BalanceRootMinYRate = minYRate; }
    
    float GetBalanceRootYRateVelocityScale() const
        { return m_BalanceRootYRateVelocityScale; }
    void SetBalanceRootYRateVelocityScale(float yRateVelocityScale)
        { m_BalanceRootYRateVelocityScale = yRateVelocityScale; }
    
    float GetMaxBalanceRootCompress() const
        { return m_MaxBalanceRootCompress; }
    void SetMaxBalanceRootCompress(float maxCompress)
        { m_MaxBalanceRootCompress = maxCompress; }

    float GetFootMinYRate(int foot) const;
    void SetFootMinYRate(int foot, float minYRate);

    float GetFootYRateVelocityScale(int foot) const;
    void SetFootYRateVelocityScale(int foot, float yRateVelocityScale);

    void Reset();
    void SetGroundPoint(int foot, const rmt::Vector& groundPoint);
    rmt::Vector GetGroundPoint(int foot) const;
    
    virtual int GetMinimumJointIndex() const
        { return m_MinimumJointIndex; }
    virtual int GetPriority() const
        { return poser::PRIORITY_DEFAULT; }
    
    virtual void Advance(float deltaTime);
    virtual void Update(poser::Pose* pose);
    
protected:
    
    virtual ~FootPlanter();
    
private:
    
    Rig* m_Rig;
    RootBlender* m_RootBlender;
    FootBlender** m_FootBlenders;
    int m_BalanceRootIndex;
    int m_MinimumJointIndex;

    float m_BalanceRootMinYRate;
    float m_BalanceRootYRateVelocityScale;
    float m_MaxBalanceRootCompress;

    struct FootYRate
    {
        float minYRate;
        float yRateVelocityScale;
    };

    FootYRate* m_FootYRates;
    
    float m_DeltaTime;
    
    rmt::Vector* m_GroundPoints;
    bool* m_IsGroundPointSets;
    float* m_GroundYs;
    float m_BalanceY;
};


} // namespace choreo


#endif
