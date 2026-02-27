/*
 * choreo/synchronization.hpp
 */

#ifndef CHOREO_SYNCHRONIZATION_HPP
#define CHOREO_SYNCHRONIZATION_HPP


#include <choreo/driver.hpp>


namespace choreo
{


class Locomotion;
class LocomotionDriver;
class SynchronizationRootDriver;
class Puppet;
class Partition;


class SynchronizationDriver: public PuppetDriver
{
public:

    SynchronizationDriver(Locomotion* loco);
    
    virtual PuppetDriver* Clone() const;
    
    // locomotion accessor
    Locomotion* GetLocomotion() const;
    
    // partition, weight & priority
    Partition* GetPartition() const;
    void SetPartition(Partition* partition);
    
    float GetWeight() const;
    void SetWeight(float weight);
    
    int GetPriority() const;
    void SetPriority(int priority);

    // blend times -- overrides those in Locomotion
    float GetRootBlendInTime() const;
    void SetRootBlendInTime(float rootBlendInTime);
    float GetRootBlendOutTime() const;
    void SetRootBlendOutTime(float rootBlendOutTime);
    
    float GetBlendInTime() const;
    void SetBlendInTime(float blendInTime);
    float GetBlendOutTime() const;
    void SetBlendOutTime(float blendOutTime);

    // sync parameters
    float GetDuration() const
        { return m_Duration; }
    void SetDuration(float duration)
        { m_Duration = duration; }
    
    const rmt::Vector& GetTargetPosition() const
        { return m_TargetPosition; }
    void SetTargetPosition(const rmt::Vector& pos)
        { m_TargetPosition = pos; }
    
    float GetTargetOrientation() const
        { return m_TargetOrientation; }
    void SetTargetOrientation(float orient)
        { m_TargetOrientation = orient; }
    
    virtual void Begin(Puppet* puppet);
    virtual void Advance(float deltaTime);
    virtual bool IsFinished() const;
    virtual void End(Puppet* puppet);
    
    virtual void Apply(Engine* engine);
    
protected:
    
    virtual ~SynchronizationDriver();
    
private:
    
    void SetSynchParameters(float deltaTime);
    
    LocomotionDriver* m_LocomotionDriver;
    SynchronizationRootDriver* m_SynchronizationRootDriver;
    Puppet* m_Puppet;
    
    float m_Duration;
    rmt::Vector m_TargetPosition;
    float m_TargetOrientation;
};


} // namespace choreo


#endif
