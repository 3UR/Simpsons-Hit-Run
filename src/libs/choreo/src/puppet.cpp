/*
 * choreo/puppet.cpp
 */

#include <choreo/puppet.hpp>
#include <choreo/engine.hpp>
#include <choreo/driver.hpp>
#include <choreo/constants.hpp>
#include <choreo/utility.hpp>
#include <choreo/rootblender.hpp>
#include <choreo/rig.hpp>
#include <choreo/scriptreader.hpp>
#include <choreo/scriptwriter.hpp>
#include <choreo/synchronization.hpp>
#include <choreo/jointblender.hpp>

#include <p3d/error.hpp>
#include <p3d/anim/skeleton.hpp>

#include <string.h>


namespace choreo
{


//---------------------------------------------------------------------------
// class Puppet - constructors
//---------------------------------------------------------------------------

Puppet::Puppet(tPose* pose,
               Bank* bank,
               bool enableReplay,
               int maxDriverCount,
               int maxPropCount):
    m_MaxDriverCount(maxDriverCount),
    m_DriverCount(0)
{
    P3DASSERT(bank != 0);
    m_Bank = bank;
    m_Bank->AddRef();

    m_Engine = new Engine(pose, bank->GetRig(), enableReplay, maxPropCount);
    m_Engine->AddRef();

    m_Drivers = new DriverStruct [m_MaxDriverCount];
}

Puppet::~Puppet()
{
    for (int i = m_DriverCount - 1; i >= 0; --i)
    {
        m_Drivers[i].driver->End(this);
        m_Drivers[i].driver->Release();
    }
    
    delete[] m_Drivers;
    
    m_Engine->Release();
    m_Bank->Release();
}


//---------------------------------------------------------------------------
// class Puppet - bank accessors
//---------------------------------------------------------------------------

poser::Pose* Puppet::GetPose()
{
    return m_Engine->GetPose();
}

const poser::Pose* Puppet::GetPose() const
{
    return m_Engine->GetPose();
}

tPose* Puppet::GetP3DPose() const
{
    return m_Engine->GetP3DPose();
}


//---------------------------------------------------------------------------
// class Puppet - bank accessors
//---------------------------------------------------------------------------

bool Puppet::SetBank(Bank* bank)
{
    /*
    P3DASSERT(bank != 0);
    if (bank == 0)
        return false;
    P3DASSERT(bank->GetRig() == m_Bank->GetRig());
    if (bank->GetRig() != m_Bank->GetRig())
        return false;
    */
    
    tRefCounted::Assign(m_Bank, bank);
    return true;
}


//---------------------------------------------------------------------------
// class Puppet - animations
//---------------------------------------------------------------------------

AnimationDriver* Puppet::PlayAnimation(tUID animUID,
									   float duration,
									   float speed,
									   float startFrame,
									   float endFrame,
                                       float weight,
                                       int priority,
                                       Partition* partition)
{
    Animation* anim = choreo::find<Animation>(m_Bank, animUID);

    //P3DASSERT(anim != 0);
    if (anim == 0)
        return nullptr;
    
	// use temporary memory for drivers
    radMemoryAllocator oldAlloc = ::radMemorySetCurrentAllocator(RADMEMORY_ALLOC_TEMP);

    AnimationDriver* animDriver = anim->NewAnimationDriver();
	animDriver->AddRef();

    animDriver->SetWeight(weight);
    animDriver->SetPriority(priority);
    if (partition != 0)
    {
        animDriver->SetPartition(partition);
    }

	// restore allocator
    ::radMemorySetCurrentAllocator(oldAlloc);

    animDriver->SetPlaybackTimings(duration, speed, startFrame, endFrame);
    
    if (PlayDriver(animDriver, animDriver->GetDuration()))
	{
		animDriver->Release();
		return animDriver;
	}
	else
	{
		animDriver->Release();
		return 0;
	}
}

AnimationDriver* Puppet::PlayAnimation(const char* animName,
                                       float duration,
                                       float speed,
                                       float startFrame,
                                       float endFrame,
                                       float weight,
                                       int priority,
                                       Partition* partition)
{
    return PlayAnimation(tEntity::MakeUID(animName),
                         duration, speed, startFrame, endFrame,
                         weight, priority, partition);
}

bool Puppet::StopAnimation(tUID animUID)
{
    int i;
    for (i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver->GetUID() == animUID)
        {
            if (dynamic_cast<AnimationDriver*>(m_Drivers[i].driver) != 0)
            {
                m_Drivers[i].driver->End(this);
                m_Drivers[i].driver->Release();
                
                --m_DriverCount;
                
                // shift array contents over
                memmove(m_Drivers + i,
                        m_Drivers + i + 1,
                        (m_DriverCount - i) * sizeof(DriverStruct));
                
                return true;
            }
        }
    }
    
    return false;
}

bool Puppet::StopAnimation(const char* animName)
{
    return StopAnimation(tEntity::MakeUID(animName));
}

bool Puppet::IsAnimationFinished(tUID animUID) const
{
    for (int i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver->GetUID() == animUID)
        {
            if (dynamic_cast<AnimationDriver*>(m_Drivers[i].driver) != 0)
            {
                return m_Drivers[i].driver->IsFinished();
            }
        }
    }
    
    return true;
}

bool Puppet::IsAnimationFinished(const char* animName) const
{
    return IsAnimationFinished(tEntity::MakeUID(animName));
}

bool Puppet::IsAnimationPlaying(tUID animUID) const
{
    return !IsAnimationFinished(animUID);
}

bool Puppet::IsAnimationPlaying(const char* animName) const
{
    return !IsAnimationFinished(animName);
}

float Puppet::GetAnimationTimeRemaining(tUID animUID) const
{
    for (int i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver->GetUID() == animUID)
        {
            if (dynamic_cast<AnimationDriver*>(m_Drivers[i].driver) != 0)
            {
                return m_Drivers[i].duration;
            }
        }
    }
    
    return -1.0f;
}

float Puppet::GetAnimationTimeRemaining(const char* animName) const
{
    return GetAnimationTimeRemaining(tEntity::MakeUID(animName));
}

float Puppet::IsAnimationIndefinite(tUID animUID) const
{
    for (int i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver->GetUID() == animUID)
        {
            if (dynamic_cast<AnimationDriver*>(m_Drivers[i].driver) != 0)
            {
                return m_Drivers[i].indefinite;
            }
        }
    }
    
    return false;
}

float Puppet::IsAnimationIndefinite(const char* animName) const
{
    return IsAnimationIndefinite(tEntity::MakeUID(animName));
}


//---------------------------------------------------------------------------
// class Puppet - locomotions
//---------------------------------------------------------------------------

LocomotionDriver* Puppet::PlayLocomotion(tUID locoUID,
                                         float motionAngle,
                                         float facingAngle,
                                         float velocity,
                                         float weight,
                                         int priority,
                                         Partition* partition)
{
    Locomotion* loco = choreo::find<Locomotion>(m_Bank, locoUID);
    P3DASSERT(loco != 0);
    if (loco == 0)
        return nullptr;
    
    LocomotionDriver* locoDriver;
    
    // search for an already-running locomotion
    for (int i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver->GetUID() == locoUID)
        {
            locoDriver = dynamic_cast<LocomotionDriver*>(m_Drivers[i].driver);
            
            if (locoDriver != 0)
            {
                locoDriver->SetDesiredMotionAngle(motionAngle);
                locoDriver->SetDesiredFacingAngle(facingAngle);
                locoDriver->SetDesiredVelocity(velocity);

                locoDriver->SetWeight(weight);
                locoDriver->SetPriority(priority);
                if (partition != 0)
                {
                    locoDriver->SetPartition(partition);
                }
                
                m_Drivers[i].duration = 0.0f;
                m_Drivers[i].indefinite = false;
                return locoDriver;
            }
        }
    }
    
    // check to make sure we can create a new driver slot
    P3DASSERT(m_DriverCount < m_MaxDriverCount);
    if (m_DriverCount >= m_MaxDriverCount)
        return nullptr;
    
	// use temporary memory for drivers
    radMemoryAllocator oldAlloc = ::radMemorySetCurrentAllocator(RADMEMORY_ALLOC_TEMP);

    // didn't find already running locomotion,
    // so create a new one on the fly
    locoDriver = loco->NewLocomotionDriver();
    locoDriver->AddRef();
    
	// restore allocator
    ::radMemorySetCurrentAllocator(oldAlloc);

    // set loco params
    locoDriver->SetDesiredMotionAngle(motionAngle);
    locoDriver->SetDesiredFacingAngle(facingAngle);
    locoDriver->SetDesiredVelocity(velocity);

    // set blending params
    locoDriver->SetWeight(weight);
    locoDriver->SetPriority(priority);
    if (partition != 0)
    {
        locoDriver->SetPartition(partition);
    }
    
    // play for one frame
    if (AddDriver(locoDriver, 0.0f, false))
    {
        locoDriver->Release();
        return locoDriver;
    }
    else
    {
        locoDriver->Release();
        return 0;
    }
}

LocomotionDriver* Puppet::PlayLocomotion(const char* locoName,
                                         float motionAngle,
                                         float facingAngle,
                                         float velocity,
                                         float weight,
                                         int priority,
                                         Partition* partition)
{
    return PlayLocomotion(tEntity::MakeUID(locoName),
                          motionAngle, facingAngle, velocity,
                          weight, priority, partition);
}


//---------------------------------------------------------------------------
// class Puppet - locomotion synchronization
//---------------------------------------------------------------------------

SynchronizationDriver* Puppet::PlaySynchronization(tUID locoUID,
                                                   float duration,
                                                   const rmt::Vector& targetPos,
                                                   float targetOrient,
                                                   float weight,
                                                   int priority,
                                                   Partition* partition)
{
    Locomotion* loco = choreo::find<Locomotion>(m_Bank, locoUID);
    P3DASSERT(loco != 0);
    if (loco == 0)
        return nullptr;
    
    SynchronizationDriver* syncDriver;
    
    // search for an already-running locomotion
    for (int i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver->GetUID() == locoUID)
        {
            syncDriver = dynamic_cast<SynchronizationDriver*>(m_Drivers[i].driver);
            
            if (syncDriver != 0)
            {
                // set sync params
                syncDriver->SetDuration(duration);
                syncDriver->SetTargetPosition(targetPos);
                syncDriver->SetTargetOrientation(targetOrient);

                // set blend params
                syncDriver->SetWeight(weight);
                syncDriver->SetPriority(priority);
                if (partition != 0)
                {
                    syncDriver->SetPartition(partition);
                }
                
                m_Drivers[i].duration = duration;
                m_Drivers[i].indefinite = false;
                return syncDriver;
            }
        }
    }
    
    // check to make sure we can create a new driver slot
    P3DASSERT(m_DriverCount < m_MaxDriverCount);
    if (m_DriverCount >= m_MaxDriverCount)
        return nullptr;
    
	// use temporary memory for drivers
    radMemoryAllocator oldAlloc = ::radMemorySetCurrentAllocator(RADMEMORY_ALLOC_TEMP);

    // didn't find already running locomotion,
    // so create a new one on the fly
    syncDriver = loco->NewSynchronizationDriver();
    syncDriver->AddRef();
    
	// restore allocator
    ::radMemorySetCurrentAllocator(oldAlloc);

    // set sync params
    syncDriver->SetDuration(duration);
    syncDriver->SetTargetPosition(targetPos);
    syncDriver->SetTargetOrientation(targetOrient);
    
    // set blend params
    syncDriver->SetWeight(weight);
    syncDriver->SetPriority(priority);
    if (partition != 0)
    {
        syncDriver->SetPartition(partition);
    }

    // play for duration
    if (AddDriver(syncDriver, duration, false))
    {
        syncDriver->Release();
        return syncDriver;
    }
    else
    {
        syncDriver->Release();
        return 0;
    }
}

SynchronizationDriver* Puppet::PlaySynchronization(const char* locoName,
                                                   float duration,
                                                   const rmt::Vector& targetPos,
                                                   float targetOrient,
                                                   float weight,
                                                   int priority,
                                                   Partition* partition)
{
    return PlaySynchronization(tEntity::MakeUID(locoName),
                               duration, targetPos, targetOrient,
                               weight, priority, partition);
}

bool Puppet::SetSynchronizationTarget(tUID locoUID,
                                      const rmt::Vector& targetPos,
                                      float targetOrient)
{
    for (int i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver->GetUID() == locoUID)
        {
            SynchronizationDriver* syncDriver;
            syncDriver = dynamic_cast<SynchronizationDriver*>(m_Drivers[i].driver);
            
            if (syncDriver != 0)
            {
                syncDriver->SetTargetPosition(targetPos);
                syncDriver->SetTargetOrientation(targetOrient);
                return true;
            }
        }
    }
    
    return false;
}

bool Puppet::SetSynchronizationTarget(const char* locoName,
                                      const rmt::Vector& targetPos,
                                      float targetOrient)
{
    return SetSynchronizationTarget(tEntity::MakeUID(locoName),
                                    targetPos,
                                    targetOrient);
}

bool Puppet::GetSynchronizationTarget(tUID locoUID,
                                      rmt::Vector& targetPos,
                                      float& targetOrient) const
{
    for (int i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver->GetUID() == locoUID)
        {
            SynchronizationDriver* syncDriver;
            syncDriver = dynamic_cast<SynchronizationDriver*>(m_Drivers[i].driver);
            
            if (syncDriver != 0)
            {
                targetPos = syncDriver->GetTargetPosition();
                targetOrient = syncDriver->GetTargetOrientation();
                return true;
            }
        }
    }
    
    return false;
}

bool Puppet::GetSynchronizationTarget(const char* locoName,
                                      rmt::Vector& targetPos,
                                      float& targetOrient) const
{
    return GetSynchronizationTarget(tEntity::MakeUID(locoName),
                                    targetPos,
                                    targetOrient);
}

bool Puppet::StopSynchronization(tUID locoUID)
{
    int i;
    for (i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver->GetUID() == locoUID)
        {
            if (dynamic_cast<SynchronizationDriver*>(m_Drivers[i].driver) != 0)
            {
                m_Drivers[i].driver->End(this);
                m_Drivers[i].driver->Release();
                
                --m_DriverCount;
                
                // shift array contents over
                memmove(m_Drivers + i,
                        m_Drivers + i + 1,
                        (m_DriverCount - i) * sizeof(DriverStruct));
                
                return true;
            }
        }
    }
    
    return false;
}

bool Puppet::StopSynchronization(const char* locoName)
{
    return StopSynchronization(tEntity::MakeUID(locoName));
}

bool Puppet::IsSynchronizationFinished(tUID locoUID) const
{
    for (int i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver->GetUID() == locoUID)
        {
            if (dynamic_cast<SynchronizationDriver*>(m_Drivers[i].driver) != 0)
            {
                return m_Drivers[i].driver->IsFinished();
            }
        }
    }
    
    return true;
}

bool Puppet::IsSynchronizationFinished(const char* locoName) const
{
    return IsSynchronizationFinished(tEntity::MakeUID(locoName));
}

bool Puppet::IsSynchronizationPlaying(tUID locoUID) const
{
    return !IsSynchronizationFinished(locoUID);
}

bool Puppet::IsSynchronizationPlaying(const char* locoName) const
{
    return !IsSynchronizationFinished(locoName);
}

float Puppet::GetSynchronizationTimeRemaining(tUID locoUID) const
{
    for (int i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver->GetUID() == locoUID)
        {
            if (dynamic_cast<SynchronizationDriver*>(m_Drivers[i].driver) != 0)
            {
                return m_Drivers[i].duration;
            }
        }
    }
    
    return -1.0f;
}

float Puppet::GetSynchronizationTimeRemaining(const char* locoName) const
{
    return GetSynchronizationTimeRemaining(tEntity::MakeUID(locoName));
}


//---------------------------------------------------------------------------
// class Puppet - generic driver interface
//---------------------------------------------------------------------------

bool Puppet::AddDriver(PuppetDriver* driver, float duration, bool indefinite)
{
    P3DASSERT(m_DriverCount < m_MaxDriverCount);
    if (m_DriverCount >= m_MaxDriverCount)
        return false;
    
    m_Drivers[m_DriverCount].driver = driver;
    driver->AddRef();
    driver->Begin(this);
    m_Drivers[m_DriverCount].duration = duration;
    m_Drivers[m_DriverCount].indefinite = indefinite;
    
    ++m_DriverCount;
    return true;
}

bool Puppet::PlayDriver(PuppetDriver* driver, float duration, bool restart)
{
    for (int i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver == driver)
        {
            if (restart)
            {
                driver->End(this);
                driver->Begin(this);
            }
            
            m_Drivers[i].duration = duration;
            m_Drivers[i].indefinite = (duration < 0.0f);
            return true;
        }
    }
    
    return AddDriver(driver, duration, (duration < 0.0f));
}

bool Puppet::StopDriver(PuppetDriver* driver)
{
    for (int i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver == driver)
        {
            driver->End(this);
            driver->Release();
            
            --m_DriverCount;
            
            // shift array contents over
            memmove(m_Drivers + i,
                    m_Drivers + i + 1,
                    (m_DriverCount - i) * sizeof(DriverStruct));
            
            return true;
        }
    }
    
    return false;
}

bool Puppet::IsDriverFinished(PuppetDriver* driver) const
{
    for (int i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver == driver)
        {
            // has time run out?
            if ((m_Drivers[i].duration < 0.0f) &&
                (!m_Drivers[i].indefinite))
            {
                return true;
            }

            // otherwise, ask driver
            return m_Drivers[i].driver->IsFinished();
        }
    }
    
    return true;
}

float Puppet::GetDriverTimeRemaining(PuppetDriver* driver) const
{
    for (int i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver == driver)
        {
            return m_Drivers[i].duration;
        }
    }
    
    return -1.0f;
}

bool Puppet::IsDriverIndefinite(PuppetDriver* driver) const
{
    for (int i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver == driver)
        {
            return m_Drivers[i].indefinite;
        }
    }
    
    return false;
}

bool Puppet::GetDriverTimings(PuppetDriver* driver, bool& isIndefinite, float& timeRemaining) const
{
    for (int i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver == driver)
        {
            isIndefinite = m_Drivers[i].indefinite;
            timeRemaining = m_Drivers[i].duration;
            return true;
        }
    }
    
    return false;
}

bool Puppet::SetDriverTimeRemaining(PuppetDriver* driver, float timeRemaining)
{
    for (int i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver == driver)
        {
            m_Drivers[i].duration = timeRemaining;
            return true;
        }
    }
    
    return false;
}

bool Puppet::SetIsDriverIndefinite(PuppetDriver* driver, bool isIndefinite)
{
    for (int i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver == driver)
        {
            m_Drivers[i].indefinite = isIndefinite;
            return true;
        }
    }
    
    return false;
}

bool Puppet::SetDriverTimings(PuppetDriver* driver, bool isIndefinite, float timeRemaining)
{
    for (int i = 0; i < m_DriverCount; ++i)
    {
        if (m_Drivers[i].driver == driver)
        {
            m_Drivers[i].indefinite = isIndefinite;
            m_Drivers[i].duration = timeRemaining;
            return true;
        }
    }
    
    return false;
}

void Puppet::StopAllDrivers()
{
    for (int i = m_DriverCount - 1; i >= 0; --i)
    {
        m_Drivers[i].driver->End(this);
        m_Drivers[i].driver->Release();
    }
    
    m_DriverCount = 0;
}


//---------------------------------------------------------------------------
// class Puppet - driver accessors
//---------------------------------------------------------------------------

PuppetDriver* Puppet::GetDriver(int i) const
{
    P3DASSERT((i >= 0) && (i < m_DriverCount));
    if ((i < 0) || (i >= m_DriverCount))
        return 0;
    
    return m_Drivers[i].driver;
}

bool Puppet::IsBusy() const
{
    for (int i = 0; i < m_DriverCount; ++i)
    {
        // ask driver if finished
        if (!m_Drivers[i].driver->IsFinished())
        {
            // has time run out?
            if (m_Drivers[i].duration >= 0.0f)
            {
                return true;
            }

            if (m_Drivers[i].indefinite)
            {
                return true;
            }
        }
    }

    return false;
}

int Puppet::GetBusyDriverCount() const
{
    int busyDriverCount = 0;

    for (int i = 0; i < m_DriverCount; ++i)
    {
        // ask driver if finished
        if (!m_Drivers[i].driver->IsFinished())
        {
            // has time run out?
            if (m_Drivers[i].duration >= 0.0f)
            {
                ++busyDriverCount;
                continue;
            }

            if (m_Drivers[i].indefinite)
            {
                ++busyDriverCount;
                continue;
            }
        }
    }

    return busyDriverCount;
}

PuppetDriver* Puppet::GetBusyDriver(int i) const
{
    int busyDriverCount = 0;

    for (int d = 0; d < m_DriverCount; ++d)
    {
        // ask driver if finished
        if (!m_Drivers[d].driver->IsFinished())
        {
            // has time run out?
            if (m_Drivers[d].duration >= 0.0f)
            {
                if (i == busyDriverCount)
                {
                    return m_Drivers[d].driver;
                }

                ++busyDriverCount;
                continue;
            }

            if (m_Drivers[d].indefinite)
            {
                if (i == busyDriverCount)
                {
                    return m_Drivers[d].driver;
                }

                ++busyDriverCount;
                continue;
            }
        }
    }

    return 0;
}


//---------------------------------------------------------------------------
// class Puppet - advance/update
//---------------------------------------------------------------------------

void Puppet::Advance(float deltaTime)
{
    // advance drivers
    int i = 0;
    while (i < m_DriverCount)
    {
        // if time has run out, or driver is finished, ditch it
        if (((m_Drivers[i].duration < 0.0f) &&
             (!m_Drivers[i].indefinite)) ||
            m_Drivers[i].driver->IsFinished())
        {
            m_Drivers[i].driver->End(this);
            m_Drivers[i].driver->Release();
        
            --m_DriverCount;
        
            // shift array contents over
            memmove(m_Drivers + i,
                    m_Drivers + i + 1,
                    (m_DriverCount - i) * sizeof(DriverStruct));
        
            // don't increment i
        }
        else
        {
            // advance driver
            m_Drivers[i].driver->Advance(deltaTime);

            if (!m_Drivers[i].indefinite)
            {
                m_Drivers[i].duration -= deltaTime;
            }

            ++i;
        }
    }

    for (i = 0; i < m_DriverCount; ++i)
    {
        m_Engine->Push(m_Drivers[i].driver);
    }
    
    // advance engine
    m_Engine->Advance(deltaTime);
}

void Puppet::UpdateBegin()
{
    // begin pose evaluation
    m_Engine->UpdateBegin();
}

void Puppet::UpdateRoot()
{
    m_Engine->UpdateRoot();
}

void Puppet::UpdatePose()
{
    m_Engine->UpdatePose();
}

void Puppet::UpdateEnd()
{
    // complete pose evaluation
    m_Engine->UpdateEnd();
}

void Puppet::AdvanceReplay(float deltaTime)
{
    for (int i = 0; i < m_DriverCount; ++i)
    {
        m_Engine->Push(m_Drivers[i].driver);
    }
    
    m_Engine->AdvanceReplay(deltaTime);
}

void Puppet::UpdateReplay()
{
    m_Engine->UpdateReplay();
}


//---------------------------------------------------------------------------
// class Puppet - loading
//---------------------------------------------------------------------------

static const char* TOKEN_PUPPET          = "puppet";
static const char* TOKEN_NAME            = "name";
static const char* TOKEN_BANK            = "bank";
static const char* TOKEN_MAXDRIVERCOUNT  = "maxDriverCount";
static const char* TOKEN_MAXPROPCOUNT    = "maxPropCount";
static const char* TOKEN_ISREPLAYENABLED = "isReplayEnabled";
    
bool Puppet::ReadFromScript(ScriptReader* script, BaseBank* bank)
{
    if (!script->IsMoreStructs())
        return false;
    if (strcmp(script->GetNextStructIdentifier(), TOKEN_PUPPET) != 0)
        return false;
    
    script->BeginStruct(TOKEN_PUPPET);
    
    // required params
    char puppetName[256] = "";
    script->RequiredParamString(TOKEN_NAME, puppetName, sizeof(puppetName));
    
    char bankName[256] = "";
    script->RequiredParamString(TOKEN_BANK, bankName, sizeof(bankName));
    
    Bank* puppetBank = choreo::find<Bank>(bank, bankName);
    if (puppetBank == 0)
    {
        script->SetErrorFormat("Could not find bank named \"%s\".", bankName);
    }

    // optional params
    float isReplayEnabledf = 0.0f;
    script->ParamFloat(TOKEN_ISREPLAYENABLED, &isReplayEnabledf);
    
    float maxDriverCountf = DEFAULT_MAX_DRIVER_COUNT;
    script->ParamFloat(TOKEN_MAXDRIVERCOUNT, &maxDriverCountf);

    float maxPropCountf = DEFAULT_MAX_PROP_COUNT;
    script->ParamFloat(TOKEN_MAXPROPCOUNT, &maxPropCountf);

    script->EndStruct();
    
    if (script->IsError())
    {
        return script->HandleError();
    }
    
    // allocate puppet
    Puppet* puppet = new Puppet(puppetBank->GetRig()->GetSkeleton()->NewPose(),
                                puppetBank,
                                !rmt::Epsilon(isReplayEnabledf, 0.0f),
                                (int)maxDriverCountf,
                                (int)maxPropCountf);
    puppet->AddRef();
    puppet->SetName(puppetName);
    
    bank->Store(puppet);

    puppet->Release();
    return true;
}

#ifdef CHOREO_TOOL
bool Puppet::WriteToScriptHandler(ScriptWriter* script, BaseBank* bank)
{
    CHOREO_SCRIPT_WRITE_HANDLER(Puppet, script, bank);
}

bool Puppet::WriteToScript(ScriptWriter* script) const
{
    script->BeginStruct(TOKEN_PUPPET);

    // required params
    script->RequiredParamString(TOKEN_NAME, GetName());
    script->RequiredParamString(TOKEN_BANK, m_Bank->GetName());

    // optional params
    script->ParamFloat(TOKEN_ISREPLAYENABLED, m_Engine->IsReplayEnabled());
    script->ParamFloat(TOKEN_MAXDRIVERCOUNT, m_MaxDriverCount);
    script->ParamFloat(TOKEN_MAXPROPCOUNT, m_Engine->GetJointBlender()->GetMaxAttachPropCount());

    script->EndStruct();
    return true;
}
#endif // CHOREO_TOOL


} // namespace choreo


// End of file.
