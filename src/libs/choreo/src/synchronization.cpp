/*
 * choreo/synchronization.cpp
 */

#include <choreo/synchronization.hpp>
#include <choreo/locomotion.hpp>
#include <choreo/puppet.hpp>
#include <choreo/utility.hpp>
#include <choreo/rootblenddriver.hpp>
#include <choreo/rig.hpp>

#include <p3d/error.hpp>


namespace choreo
{


//---------------------------------------------------------------------------
// class SynchronizationRootDriver - interface
//---------------------------------------------------------------------------

class SynchronizationRootDriver: public RootBlendDriver
{
public:

    SynchronizationRootDriver(LocomotionDriver* locoDriver);

    // synchronization targets
    void SetPosition(const rmt::Vector& position);
    void SetOrientation(float orientation);
    
    virtual void Reset(RootBlender* rootBlender)
        { }
    
    // blend times
    virtual float GetRootBlendInTime() const;
    virtual float GetRootBlendOutTime() const;
    
    // weight & priority
    virtual float GetRootWeight() const;
    virtual int GetRootPriority() const;
    
    // translation
    virtual bool HasRootTranslation() const;
    virtual void GetRootTranslation(const poser::Transform& oldTransform,
                                    rmt::Vector& newTranslation) const;
    
    // rotation
    virtual bool HasRootRotation() const;
    virtual void GetRootRotation(const poser::Transform& oldTransform,
                                 rmt::Quaternion& newRotate) const;
    
    // ground fixup
    virtual bool HasRootGroundFixup() const;
    virtual float GetRootGroundFixup() const;
    
protected:
    
    virtual ~SynchronizationRootDriver();
    
private:

    LocomotionDriver* m_LocomotionDriver;

    rmt::Vector m_Position;
    rmt::Quaternion m_Orientation;
};


//---------------------------------------------------------------------------
// class SynchronizationRootDriver - implementation
//---------------------------------------------------------------------------

SynchronizationRootDriver::SynchronizationRootDriver(LocomotionDriver* locoDriver)
{
    P3DASSERT(locoDriver != 0);
    m_LocomotionDriver = locoDriver;
    m_LocomotionDriver->AddRef();
}

SynchronizationRootDriver::~SynchronizationRootDriver()
{
    m_LocomotionDriver->Release();
}

void SynchronizationRootDriver::SetPosition(const rmt::Vector& position)
{
    m_Position = position;
}

void SynchronizationRootDriver::SetOrientation(float orientation)
{
    m_Orientation.BuildFromAngleY(orientation);
}
    
float SynchronizationRootDriver::GetRootBlendInTime() const
{
    return m_LocomotionDriver->GetRootBlendInTime();
}

float SynchronizationRootDriver::GetRootBlendOutTime() const
{
    return m_LocomotionDriver->GetRootBlendOutTime();
}

float SynchronizationRootDriver::GetRootWeight() const
{
    int aiRootIndex = m_LocomotionDriver->GetLocomotion()->GetRig()->GetJointIndex(RIG_JOINT_AI_ROOT);
    // FIXME:  doesn't include LocomotionPhase::GetWeight()!
    //         compare to LocomotionRootDriver::GetRootWeight()
    return m_LocomotionDriver->GetJointWeight(aiRootIndex);
}

int SynchronizationRootDriver::GetRootPriority() const
{
    int aiRootIndex = m_LocomotionDriver->GetLocomotion()->GetRig()->GetJointIndex(RIG_JOINT_AI_ROOT);
    return m_LocomotionDriver->GetJointPriority(aiRootIndex);
}

bool SynchronizationRootDriver::HasRootTranslation() const
{
    return true;
}

void SynchronizationRootDriver::GetRootTranslation(const poser::Transform& oldTransform,
                                                   rmt::Vector& newTranslation) const
{
    newTranslation = m_Position;
}

bool SynchronizationRootDriver::HasRootRotation() const
{
    return true;
}

void SynchronizationRootDriver::GetRootRotation(const poser::Transform& oldTransform,
                                                rmt::Quaternion& newRotate) const
{
    newRotate = m_Orientation;
}

bool SynchronizationRootDriver::HasRootGroundFixup() const
{
    return true;
}

float SynchronizationRootDriver::GetRootGroundFixup() const
{
    // FIXME
    return 1.0f;
}


//---------------------------------------------------------------------------
// class SynchronizationDriver - implementation
//---------------------------------------------------------------------------

SynchronizationDriver::SynchronizationDriver(Locomotion* loco):
    m_Puppet(0),
    m_Duration(0.0f),
    m_TargetPosition(0.0f, 0.0f, 0.0f),
    m_TargetOrientation(0.0f)
{
    P3DASSERT(loco != 0);
    CopyName(loco);
    
    m_LocomotionDriver = loco->NewLocomotionDriver();
    m_LocomotionDriver->AddRef();
    m_LocomotionDriver->SetIsRootDriverEnabled(false);

    m_SynchronizationRootDriver = new SynchronizationRootDriver(m_LocomotionDriver);
    m_SynchronizationRootDriver->AddRef();
}

SynchronizationDriver::~SynchronizationDriver()
{
    m_SynchronizationRootDriver->Release();
    m_LocomotionDriver->Release();
}

PuppetDriver* SynchronizationDriver::Clone() const
{
    return new SynchronizationDriver(m_LocomotionDriver->GetLocomotion());
}

Locomotion* SynchronizationDriver::GetLocomotion() const
{
    return m_LocomotionDriver->GetLocomotion();
}

Partition* SynchronizationDriver::GetPartition() const
{
    return m_LocomotionDriver->GetPartition();
}

void SynchronizationDriver::SetPartition(Partition* partition)
{
    m_LocomotionDriver->SetPartition(partition);
}

float SynchronizationDriver::GetWeight() const
{
    return m_LocomotionDriver->GetWeight();
}

void SynchronizationDriver::SetWeight(float weight)
{
    m_LocomotionDriver->SetWeight(weight);
}

int SynchronizationDriver::GetPriority() const
{
    return m_LocomotionDriver->GetPriority();
}

void SynchronizationDriver::SetPriority(int priority)
{
    m_LocomotionDriver->SetPriority(priority);
}

float SynchronizationDriver::GetRootBlendInTime() const
{
    return m_LocomotionDriver->GetRootBlendInTime();
}

void SynchronizationDriver::SetRootBlendInTime(float rootBlendInTime)
{
    m_LocomotionDriver->SetRootBlendInTime(rootBlendInTime);
}

float SynchronizationDriver::GetRootBlendOutTime() const
{
    return m_LocomotionDriver->GetRootBlendOutTime();
}

void SynchronizationDriver::SetRootBlendOutTime(float rootBlendOutTime)
{
    m_LocomotionDriver->SetRootBlendOutTime(rootBlendOutTime);
}

float SynchronizationDriver::GetBlendInTime() const
{
    return m_LocomotionDriver->GetBlendInTime();
}

void SynchronizationDriver::SetBlendInTime(float blendInTime)
{
    m_LocomotionDriver->SetBlendInTime(blendInTime);
}

float SynchronizationDriver::GetBlendOutTime() const
{
    return m_LocomotionDriver->GetBlendOutTime();
}

void SynchronizationDriver::SetBlendOutTime(float blendOutTime)
{
    m_LocomotionDriver->SetBlendOutTime(blendOutTime);
}

void SynchronizationDriver::SetSynchParameters(float deltaTime)
{
    P3DASSERT(m_Puppet != 0);
    if (m_Puppet == 0)
        return;
    
    if (m_Duration <= 0.0f)
        return;
    
    rmt::Vector velocity = m_TargetPosition;
    rmt::Vector pos = m_Puppet->GetPosition();
    velocity.Sub(pos);
    velocity.y = 0.0f;
    
    float orientation = m_Puppet->GetOrientation();
    float turningVelocity = GetSmallestArc(orientation, m_TargetOrientation);
    
    float invDuration = 1.0f / m_Duration;
    velocity.x *= invDuration;
    velocity.z *= invDuration;
    turningVelocity *= invDuration;
    
    m_LocomotionDriver->SetActualVelocityVector(velocity);
    m_LocomotionDriver->SetDesiredFacingAngle(m_TargetOrientation);
    m_LocomotionDriver->SetTurningVelocity(rmt::Fabs(turningVelocity));

    velocity.x *= deltaTime;
    velocity.z *= deltaTime;
    pos.Add(velocity);
    m_SynchronizationRootDriver->SetPosition(pos);

    turningVelocity *= deltaTime;
    orientation += turningVelocity;
    m_SynchronizationRootDriver->SetOrientation(orientation);
}

void SynchronizationDriver::Begin(Puppet* puppet)
{
    P3DASSERT(m_Puppet == 0);
    tRefCounted::Assign(m_Puppet, puppet);
    
    SetSynchParameters(0.0f);
    
    m_LocomotionDriver->Begin(puppet);
}

void SynchronizationDriver::Advance(float deltaTime)
{
    if (m_Duration >= 0.0f)
    {
        if (m_Duration < deltaTime)
        {
            m_Duration = 0.0f;

            m_SynchronizationRootDriver->SetPosition(m_TargetPosition);
            m_SynchronizationRootDriver->SetOrientation(m_TargetOrientation);
        }
        else
        {
            SetSynchParameters(deltaTime);
            m_Duration -= deltaTime;
        }
    }

    m_LocomotionDriver->Advance(deltaTime);
}

bool SynchronizationDriver::IsFinished() const
{
    return (m_Duration <= 0.0f);
}

void SynchronizationDriver::End(Puppet* puppet)
{
    m_LocomotionDriver->End(puppet);
    
    P3DASSERT(m_Puppet == puppet);
    tRefCounted::Release(m_Puppet);
}

void SynchronizationDriver::Apply(Engine* engine)
{
    m_LocomotionDriver->Apply(engine);
    engine->GetRootBlender()->Push(m_SynchronizationRootDriver);
}


} // namespace choreo


// End of file.
