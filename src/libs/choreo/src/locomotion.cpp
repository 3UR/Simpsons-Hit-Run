/*
 * choreo/locomotion.cpp
 */

#include <choreo/locomotion.hpp>
#include <choreo/rig.hpp>
#include <choreo/transition.hpp>
#include <choreo/utility.hpp>
#include <choreo/synchronization.hpp>
#include <choreo/scriptreader.hpp>
#include <choreo/scriptwriter.hpp>
#include <choreo/basebank.hpp>
#include <choreo/load.hpp>
#include <choreo/partition.hpp>
#include <choreo/rootblenddriver.hpp>
#include <choreo/engine.hpp>

#include <poser/transform.hpp>

#include <p3d/error.hpp>

#include <string.h>
#include <float.h>


namespace choreo
{


//---------------------------------------------------------------------------
// class LocomotionSlot
//---------------------------------------------------------------------------

LocomotionSlot::LocomotionSlot(Rig* rig, PURE3D_ANIMATION* animation):
    m_Animation(rig->GetSkeleton(), animation)
{
    m_FootInfo = new BlendSlotFootInfo(rig, this);
    m_FootInfo->AddRef();

    // calculate velocity
    int motionRootIndex = rig->GetJointIndex(RIG_JOINT_MOTION_ROOT);
    rmt::Vector dist(0.0f, 0.0f, 0.0f);

    if (motionRootIndex >= 0)
    {
        if (m_Animation.HasJointTranslation(motionRootIndex))
        {
            rmt::Vector initDist;
            m_Animation.GetJointTranslation(motionRootIndex, 0.0f, initDist);
            m_Animation.GetJointTranslation(motionRootIndex, m_Animation.GetFrameCount() - 1.0f, dist);
            dist.Sub(initDist);
        }
    }

    float cycleDuration = GetCycleDuration();
    if (cycleDuration > 0.0f)
    {
        m_Velocity = dist.Magnitude() / cycleDuration;
    }
    else
    {
        m_Velocity = 0.0f;
    }
}

LocomotionSlot::~LocomotionSlot()
{
    m_FootInfo->Release();
}


//---------------------------------------------------------------------------
// class Locomotion - implementation
//---------------------------------------------------------------------------

Locomotion::Locomotion(Rig* rig,
                       int directionCount,
                       int velocityCount,
                       PURE3D_ANIMATION** animations,
                       Partition* partition,
                       int transitionCount,
                       Transition** transitions):
    m_DirectionCount(directionCount),
    m_VelocityCount(velocityCount),

    m_Acceleration(0.0f),

    m_TurningVelocity(0.0f),

    m_TransitionCount(transitionCount)
{
    P3DASSERT(m_DirectionCount >= 1);
    P3DASSERT(m_VelocityCount >= 2);

    int paramSlotCounts[] = { m_VelocityCount, m_DirectionCount + 1 };
    m_Blend = new Blend(rig, 2, paramSlotCounts, partition);
    m_Blend->AddRef();

    P3DASSERT(animations != 0);
    int slotCount = GetLocomotionSlotCount();
    m_LocomotionSlots = new LocomotionSlot* [slotCount];
    memset(m_LocomotionSlots, 0, sizeof(LocomotionSlot*) * slotCount);
    m_LocomotionSlotVelocities = new float [slotCount];

    int animIndex = 0;
    for (int j = 0; j < m_VelocityCount; ++j)
    {
        for (int i = 0; i < m_DirectionCount; ++i)
        {
            SetLocomotionSlot(j, i, animations[animIndex]);
            ++animIndex;
        }
    }

    FillNullVelocities();

    if (m_TransitionCount > 0)
    {
        m_Transitions = new Transition* [m_TransitionCount];
        
        for (int i = 0; i < m_TransitionCount; ++i)
        {
            m_Transitions[i] = transitions[i];
            m_Transitions[i]->AddRef();
        }
    }
    else
    {
        m_Transitions = 0;
    }
}

Locomotion::~Locomotion()
{
    int i;
    
    for (i = m_TransitionCount - 1; i >= 0; --i)
    {
        m_Transitions[i]->Release();
    }
    delete[] m_Transitions;
    
    delete[] m_LocomotionSlotVelocities;

    int slotCount = GetLocomotionSlotCount();
    for (i = slotCount - 1; i >= 0; --i)
    {
        tRefCounted::Release(m_LocomotionSlots[i]);
    }
    delete[] m_LocomotionSlots;

    m_Blend->Release();
}

float Locomotion::GetAnimMaxVelocity(float relativeFacingAngle) const
{
    float dirAngleInc = rmt::PI_2 / m_DirectionCount;
    
    // calculate U
    int Ua = (int)(relativeFacingAngle / dirAngleInc) % m_DirectionCount;
    int Ub = (Ua + 1) % m_DirectionCount;
    float U = (relativeFacingAngle - (Ua * dirAngleInc)) / dirAngleInc;
    
    int base_inc = ((m_VelocityCount - 1) * m_DirectionCount);
    Ua += base_inc;
    Ub += base_inc;
    
    float vel_Ua = m_LocomotionSlotVelocities[Ua];
    float vel_Ub = m_LocomotionSlotVelocities[Ub];
    
    return ((vel_Ua * (1.0f - U)) +
            (vel_Ub * U));
}

float Locomotion::GetAnimMaxVelocity(float motionAngle, float facingAngle) const
{
    float relAngle = Wrap(0.0f, rmt::PI_2, GetSmallestArc(motionAngle, facingAngle));
    return GetAnimMaxVelocity(relAngle);
}

int Locomotion::GetLocomotionSlotCount() const
{
    return (m_DirectionCount * m_VelocityCount);
}

LocomotionSlot* Locomotion::GetLocomotionSlot(int i) const
{
    int slotCount = GetLocomotionSlotCount();
    P3DASSERT((i >= 0) && (i < slotCount));
    if ((i < 0) || (i >= slotCount))
        return 0;
    
    return m_LocomotionSlots[i];
}

LocomotionSlot* Locomotion::GetLocomotionSlot(int velIndex, int dirIndex) const
{
    P3DASSERT((velIndex >= 0) && (velIndex < m_VelocityCount));
    if ((velIndex < 0) || (velIndex >= m_VelocityCount))
        return 0;
    P3DASSERT((dirIndex >= 0) && (dirIndex < m_DirectionCount));
    if ((dirIndex < 0) || (dirIndex >= m_DirectionCount))
        return 0;

    return m_LocomotionSlots[(velIndex * m_DirectionCount) + dirIndex];
}

float Locomotion::GetLocomotionSlotVelocity(int i) const
{
    int slotCount = GetLocomotionSlotCount();
    P3DASSERT((i >= 0) && (i < slotCount));
    if ((i < 0) || (i >= slotCount))
        return -1.0f;

    return m_LocomotionSlotVelocities[i];
}

float Locomotion::GetLocomotionSlotVelocity(int velIndex, int dirIndex) const
{
    P3DASSERT((velIndex >= 0) && (velIndex < m_VelocityCount));
    if ((velIndex < 0) || (velIndex >= m_VelocityCount))
        return -1.0f;
    P3DASSERT((dirIndex >= 0) && (dirIndex < m_DirectionCount));
    if ((dirIndex < 0) || (dirIndex >= m_DirectionCount))
        return -1.0f;

    return m_LocomotionSlotVelocities[(velIndex * m_DirectionCount) + dirIndex];
}

void Locomotion::FillNullVelocities()
{
    // fill in nulls for all missing animations
    int slotCount = GetLocomotionSlotCount();
    for (int i = 0; i < slotCount; ++i)
    {
        if (m_LocomotionSlots[i] != 0)
            continue;

        int velIndex = (i / m_DirectionCount);
        int baseVelIndex = velIndex * m_DirectionCount;
        int dirIndex = (i % m_DirectionCount);

        float velocity = 0.0f;
        int count = 0;

        // find adjacent slots
        int j;
        for (j = 1; j < (m_DirectionCount - 1); ++j)
        {
            LocomotionSlot* locoSlot = m_LocomotionSlots[baseVelIndex +
                ((dirIndex + j) % m_DirectionCount)];
            if (locoSlot != 0)
            {
                velocity += locoSlot->GetVelocity();
                ++count;
                break;
            }
        }
        
        for (j = (m_DirectionCount - 1); j >= 1; --j)
        {
            LocomotionSlot* locoSlot = m_LocomotionSlots[baseVelIndex +
                ((dirIndex + j) % m_DirectionCount)];
            if (locoSlot != 0)
            {
                velocity += locoSlot->GetVelocity();
                ++count;
            }
        }

        if (count > 0)
        {
            velocity /= count;
        }

        m_LocomotionSlotVelocities[i] = velocity;
    }
}

bool Locomotion::SetLocomotionSlot(int velIndex, int dirIndex, PURE3D_ANIMATION* animation)
{
    P3DASSERT((velIndex >= 0) && (velIndex < m_VelocityCount));
    if ((velIndex < 0) || (velIndex >= m_VelocityCount))
        return false;
    P3DASSERT((dirIndex >= 0) && (dirIndex < m_DirectionCount));
    if ((dirIndex < 0) || (dirIndex >= m_DirectionCount))
        return false;

    LocomotionSlot* locoSlot = 0;
    int locoSlotIndex = (velIndex * m_DirectionCount) + dirIndex;

    if (animation != 0)
    {
        int slotCount = GetLocomotionSlotCount();
        for (int i = 0; i < slotCount; ++i)
        {
            if (m_LocomotionSlots[i] != 0)
            {
                if (m_LocomotionSlots[i]->GetAnimation() == animation)
                {
                    locoSlot = m_LocomotionSlots[i];
                    break;
                }
            }
        }

        if (locoSlot == 0)
        {
            locoSlot = new LocomotionSlot(GetRig(), animation);
        }

        tRefCounted::Assign(m_LocomotionSlots[locoSlotIndex], locoSlot);
        m_LocomotionSlotVelocities[locoSlotIndex] = locoSlot->GetVelocity();
    }
    else
    {
        tRefCounted::Assign(m_LocomotionSlots[locoSlotIndex], locoSlot);
        m_LocomotionSlotVelocities[locoSlotIndex] = -1.0f;
    }

    if (dirIndex == 0)
    {
        // wrap zeroth direction index
        m_Blend->SetBlendSlot((velIndex * (m_DirectionCount + 1)), locoSlot);
        m_Blend->SetBlendSlot(((velIndex + 1) * (m_DirectionCount + 1)) - 1, locoSlot);
    }
    else
    {
        m_Blend->SetBlendSlot((velIndex * (m_DirectionCount + 1)) + dirIndex, locoSlot);
    }

    return true;
}

Transition* Locomotion::GetTransition(int i) const
{
    P3DASSERT((i >= 0) && (i < m_TransitionCount));
    if ((i < 0) || (i >= m_TransitionCount))
        return 0;
    
    return m_Transitions[i];
}

PuppetDriver* Locomotion::NewDriver()
{
    return NewLocomotionDriver();
}

LocomotionDriver* Locomotion::NewLocomotionDriver()
{
    return new LocomotionDriver(this);
}

SynchronizationDriver* Locomotion::NewSynchronizationDriver()
{
    return new SynchronizationDriver(this);
}

static const char* TOKEN_LOCOMOTION            = "locomotion";
static const char* TOKEN_NAME                  = "name";
static const char* TOKEN_VELOCITYCOUNT         = "velocityCount";
static const char* TOKEN_DIRECTIONCOUNT        = "directionCount";
static const char* TOKEN_IDLE                  = "idle";
static const char* TOKEN_ANIMATION             = "animation";
static const char* TOKEN_PARTITION             = "partition";
static const char* TOKEN_WEIGHT                = "weight";
static const char* TOKEN_PRIORITY              = "priority";
static const char* TOKEN_ROOTBLENDINTIME       = "rootBlendInTime";
static const char* TOKEN_ROOTBLENDOUTTIME      = "rootBlendOutTime";
static const char* TOKEN_BLENDINTIME           = "blendInTime";
static const char* TOKEN_BLENDOUTTIME          = "blendOutTime";
static const char* TOKEN_ACCELERATION          = "acceleration";
static const char* TOKEN_TURNINGVELOCITY       = "turningVelocity";
static const char* TOKEN_TRANSITION            = "transition";
static const char* TOKEN_DIRECTION             = "direction";
static const char* TOKEN_SPEED                 = "speed";

bool Locomotion::ReadFromScript(ScriptReader* script, BaseBank* bank)
{
    if (!script->IsMoreStructs())
        return false;
    if (strcmp(script->GetNextStructIdentifier(), TOKEN_LOCOMOTION) != 0)
        return false;
    
    // check for rig
    Rig* rig = bank->GetRig();
    if (rig == 0)
    {
        script->SetErrorString("Locomotion specified without rig.");
    }
    
    script->BeginStruct(TOKEN_LOCOMOTION);
    
    // required params
    char locoName[256] = "";
    script->RequiredParamString(TOKEN_NAME, locoName, sizeof(locoName));
    
    float velCountf = 0.0f;
    script->RequiredParamFloat(TOKEN_VELOCITYCOUNT, &velCountf);
    
    int velCount = (int)velCountf;
    if (velCount < 2)
    {
        script->SetErrorString("Degenerate locomotion, velocity count is less than 2.");
    }
    
    float dirCountf = 0.0f;
    script->RequiredParamFloat(TOKEN_DIRECTIONCOUNT, &dirCountf);
    
    int dirCount = (int)dirCountf;
    if (dirCount < 1)
    {
        script->SetErrorString("Degenerate locomotion, direction count is less than 1.");
    }
    
    int locoCount = dirCount * velCount;
    static const int MAX_ANIMATIONS = 128;
    PURE3D_ANIMATION* anims[MAX_ANIMATIONS];
    if (locoCount > MAX_ANIMATIONS)
    {
        script->SetErrorString("Too many animations required to build this locomotion.");
    }
    
    int animCount = script->GetStructCount(TOKEN_ANIMATION);
    if (animCount != (locoCount - dirCount))
    {
        script->SetErrorFormat("Incorrect number of animations specified for locomotion; %d are required.", locoCount);
    }
    animCount = dirCount;
    
    static const int MAX_TRANSITIONS = 128;
    Transition* transitions[MAX_TRANSITIONS];
    int transitionCount = script->GetStructCount(TOKEN_TRANSITION);
    if (transitionCount > MAX_TRANSITIONS)
    {
        script->SetErrorString("Too many transitions required to build this locomotion.");
    }
    transitionCount = 0;
    
    // retrieve idle
    char idleName[256] = "";
    script->RequiredParamString(TOKEN_IDLE, idleName, sizeof(idleName));
    
    PURE3D_ANIMATION* idleAnim = 0;
    if (!script->IsError())
    {
        idleAnim = choreo::find<PURE3D_ANIMATION>(bank, idleName);
        
        if (idleAnim == 0)
        {
            script->SetErrorFormat("Could not find idle animation named \"%s\".",
                idleName);
        }
        else
        {
            for (int i = 0; i < animCount; ++i)
            {
                anims[i] = idleAnim;
            }
        }
    }
    
    // optional params
    char partitionName[256] = "";
    script->ParamString(TOKEN_PARTITION, partitionName, sizeof(partitionName));
    
    float weight = 1.0f;
    script->ParamFloat(TOKEN_WEIGHT, &weight);
    
    float priority = 0.0f;
    script->ParamFloat(TOKEN_PRIORITY, &priority);
    
    float rootBlendInTime = 0.0f;
    script->ParamFloat(TOKEN_ROOTBLENDINTIME, &rootBlendInTime);
    
    float rootBlendOutTime = 0.0f;
    script->ParamFloat(TOKEN_ROOTBLENDOUTTIME, &rootBlendOutTime);
    
    float blendInTime = 0.0f;
    script->ParamFloat(TOKEN_BLENDINTIME, &blendInTime);
    
    float blendOutTime = 0.0f;
    script->ParamFloat(TOKEN_BLENDOUTTIME, &blendOutTime);
    
    float acceleration = 0.0f;
    script->ParamFloat(TOKEN_ACCELERATION, &acceleration);
    
    float turningVelocity = 0.0f;
    script->ParamFloat(TOKEN_TURNINGVELOCITY, &turningVelocity);
    
    if (!script->IsError())
    {
        while (script->IsMoreStructs())
        {
            if (strcmp(script->GetNextStructIdentifier(), TOKEN_ANIMATION) == 0)
            {
                script->BeginStruct(TOKEN_ANIMATION);
                
                char p3dAnimName[256];
                script->RequiredParamString(TOKEN_NAME, p3dAnimName, sizeof(p3dAnimName));
                
                script->EndStruct();
                
                if (!script->IsError())
                {
                    if (strlen(p3dAnimName) == 0)
                    {
                        // no animation specified for this slot
                        anims[animCount] = 0;
                        ++animCount;
                    }
                    else
                    {
                        PURE3D_ANIMATION* p3dAnim = choreo::find<PURE3D_ANIMATION>(bank, p3dAnimName);
                        if (p3dAnim == 0)
                        {
                            script->SetErrorFormat("Could not find Pure3D animation named \"%s\".",
                                p3dAnimName);
                        }
                        else
                        {
                            anims[animCount] = p3dAnim;
                            ++animCount;
                        }
                    }
                }
            }
            else if (strcmp(script->GetNextStructIdentifier(), TOKEN_TRANSITION) == 0)
            {
                script->BeginStruct(TOKEN_TRANSITION);
                
                char transitionName[256];
                script->RequiredParamString(TOKEN_NAME, transitionName, sizeof(transitionName));
                
                script->EndStruct();
                
                if (!script->IsError())
                {
                    Transition* transition = choreo::find<Transition>(bank, transitionName);
                    if (transition == 0)
                    {
                        script->SetErrorFormat("Could not find transition named \"%s\".",
                            transitionName);
                    }
                    else
                    {
                        transitions[transitionCount] = transition;
                        ++transitionCount;
                    }
                }
            }
            else
            {
                script->SetErrorFormat("Unexpected token \"%s\", expected \"%s\".",
                    script->GetNextStructIdentifier(),
                    TOKEN_ANIMATION);
            }
        }
    }
    
    script->EndStruct();
    
    // resolve partition (if any)
    Partition* partition = 0;
    if (strlen(partitionName) > 0)
    {
        partition = choreo::find<Partition>(bank, partitionName);
        if (partition == 0)
        {
            script->SetErrorFormat("Could not find partition named \"%s\".", partitionName);
        }
    }
    
    if (script->IsError())
    {
        return script->HandleError();
    }
    
    Locomotion* loco = new Locomotion(rig,
                                      dirCount,
                                      velCount,
                                      anims,
                                      partition,
                                      transitionCount,
                                      transitions);
    loco->AddRef();
    loco->SetName(locoName);
    loco->SetWeight(weight);
    loco->SetPriority((int)priority);
    loco->SetRootBlendInTime(rootBlendInTime);
    loco->SetRootBlendOutTime(rootBlendOutTime);
    loco->SetBlendInTime(blendInTime);
    loco->SetBlendOutTime(blendOutTime);
    loco->SetAcceleration(acceleration);
    loco->SetTurningVelocity(rmt::DegToRadian(turningVelocity));
    
    bank->Store(loco);
    
    loco->Release();
    return true;
}

bool Locomotion::ReadFromScript4(ScriptReader* script, BaseBank* bank)
{
    static const int DIR_COUNT = 4;
    static const char* TOKEN_LOCOMOTION4 = "locomotion4";
    
    static const char* TOKEN_DIRVALUES[DIR_COUNT] =
    {
        "n",
        "w",
        "s",
        "e"
    };
    
    if (!script->IsMoreStructs())
        return false;
    if (strcmp(script->GetNextStructIdentifier(), TOKEN_LOCOMOTION4) != 0)
        return false;
    
    // check for rig
    Rig* rig = bank->GetRig();
    if (rig == 0)
    {
        script->SetErrorString("Locomotion specified without rig.");
    }
    
    script->BeginStruct(TOKEN_LOCOMOTION4);
    
    // required params
    char locoName[256] = "";
    script->RequiredParamString(TOKEN_NAME, locoName, sizeof(locoName));
    
    static const int MAX_ANIMATIONS = 128;
    PURE3D_ANIMATION* anims[MAX_ANIMATIONS];
    memset(anims, 0, sizeof(tAnimation*) * MAX_ANIMATIONS);
    
    static const int MAX_TRANSITIONS = 128;
    Transition* transitions[MAX_TRANSITIONS];
    int transitionCount = script->GetStructCount(TOKEN_TRANSITION);
    if (transitionCount > MAX_TRANSITIONS)
    {
        script->SetErrorString("Too many transitions required to build this locomotion.");
    }
    transitionCount = 0;
    
    // retrieve idle
    char idleName[256] = "";
    script->RequiredParamString(TOKEN_IDLE, idleName, sizeof(idleName));
    
    PURE3D_ANIMATION* idleAnim = 0;
    if (!script->IsError())
    {
        idleAnim = choreo::find<PURE3D_ANIMATION>(bank, idleName);
        
        if (idleAnim == 0)
        {
            script->SetErrorFormat("Could not find idle animation named \"%s\".",
                idleName);
        }
        else
        {
            for (int i = 0; i < DIR_COUNT; ++i)
            {
                anims[i] = idleAnim;
            }
        }
    }
    
    // optional params
    char partitionName[256] = "";
    script->ParamString(TOKEN_PARTITION, partitionName, sizeof(partitionName));
    
    float weight = 1.0f;
    script->ParamFloat(TOKEN_WEIGHT, &weight);
    
    float priority = 0.0f;
    script->ParamFloat(TOKEN_PRIORITY, &priority);
    
    float rootBlendInTime = 0.0f;
    script->ParamFloat(TOKEN_ROOTBLENDINTIME, &rootBlendInTime);
    
    float rootBlendOutTime = 0.0f;
    script->ParamFloat(TOKEN_ROOTBLENDOUTTIME, &rootBlendOutTime);
    
    float blendInTime = 0.0f;
    script->ParamFloat(TOKEN_BLENDINTIME, &blendInTime);
    
    float blendOutTime = 0.0f;
    script->ParamFloat(TOKEN_BLENDOUTTIME, &blendOutTime);
    
    float acceleration = 0.0f;
    script->ParamFloat(TOKEN_ACCELERATION, &acceleration);
    
    float turningVelocity = 0.0f;
    script->ParamFloat(TOKEN_TURNINGVELOCITY, &turningVelocity);
    
    int velCount = -1;
    
    if (!script->IsError())
    {
        while (script->IsMoreStructs())
        {
            if (strcmp(script->GetNextStructIdentifier(), TOKEN_ANIMATION) == 0)
            {
                script->BeginStruct(TOKEN_ANIMATION);
                
                // get direction
                char dirName[256] = "";
                script->RequiredParamString(TOKEN_DIRECTION, dirName, sizeof(dirName));
                int dirIndex = 0;
                int i;
                for (i = 0; i < DIR_COUNT; ++i)
                {
                    if (strcmp(TOKEN_DIRVALUES[i], dirName) == 0)
                        break;
                }
                
                if (i >= DIR_COUNT)
                {
                    script->SetErrorFormat("Incorrect direction specification \"%s\".", dirName);
                }
                else
                {
                    dirIndex = i;
                }
                
                // get speed grade
                float speed = 0.0f;
                script->RequiredParamFloat(TOKEN_SPEED, &speed);
                int speedIndex = (int)speed;
                
                if (speedIndex < 0)
                {
                    script->SetErrorFormat("Speed grade of %d is less than zero.", speedIndex);
                }
                if (speedIndex > velCount)
                {
                    velCount = speedIndex;
                }
                
                // get index
                int animIndex = ((speedIndex + 1) * DIR_COUNT) + dirIndex;
                if (animIndex > MAX_ANIMATIONS)
                {
                    script->SetErrorString("Too many animations required for locomotion.");
                }
                else if ((animIndex >= 0) && (anims[animIndex] != 0))
                {
                    script->SetErrorFormat("Direction \"%s\" and speed %d animation already specified.",
                        dirName, speedIndex);
                }
                
                // get animation
                char p3dAnimName[256] = "";
                script->RequiredParamString(TOKEN_NAME, p3dAnimName, sizeof(p3dAnimName));
                PURE3D_ANIMATION* p3dAnim = 0;
                
                if (strlen(p3dAnimName) != 0)
                {
                    p3dAnim = choreo::find<PURE3D_ANIMATION>(bank, p3dAnimName);
                    if (p3dAnim == 0)
                    {
                        script->SetErrorFormat("Could not find Pure3D animation named \"%s\".",
                            p3dAnimName);
                    }
                }
                
                script->EndStruct();
                
                if (!script->IsError())
                {
                    anims[animIndex] = p3dAnim;
                }
            }
            else if (strcmp(script->GetNextStructIdentifier(), TOKEN_TRANSITION) == 0)
            {
                script->BeginStruct(TOKEN_TRANSITION);
                
                char transitionName[256];
                script->RequiredParamString(TOKEN_NAME, transitionName, sizeof(transitionName));
                
                script->EndStruct();
                
                if (!script->IsError())
                {
                    Transition* transition = choreo::find<Transition>(bank, transitionName);
                    if (transition == 0)
                    {
                        script->SetErrorFormat("Could not find transition named \"%s\".",
                            transitionName);
                    }
                    else
                    {
                        transitions[transitionCount] = transition;
                        ++transitionCount;
                    }
                }
            }
            else
            {
                script->SetErrorFormat("Unexpected token \"%s\", expected \"%s\".",
                    script->GetNextStructIdentifier(),
                    TOKEN_ANIMATION);
            }
        }
    }
   
    script->EndStruct();

    velCount += 2;
    if (velCount < 2)
    {
        script->SetErrorFormat("Degenerate locomotion, velocity count of %d is less than 2.", velCount);
    }

    // resolve partition (if any)
    Partition* partition = 0;
    if (strlen(partitionName) > 0)
    {
        partition = choreo::find<Partition>(bank, partitionName);
        if (partition == 0)
        {
            script->SetErrorFormat("Could not find partition named \"%s\".", partitionName);
        }
    }
   
    if (script->IsError())
    {
        return script->HandleError();
    }
   
    Locomotion* loco = new Locomotion(rig,
                                      DIR_COUNT,
                                      velCount,
                                      anims,
                                      partition,
                                      transitionCount,
                                      transitions);
    loco->AddRef();
    loco->SetName(locoName);
    loco->SetWeight(weight);
    loco->SetPriority((int)priority);
    loco->SetRootBlendInTime(rootBlendInTime);
    loco->SetRootBlendOutTime(rootBlendOutTime);
    loco->SetBlendInTime(blendInTime);
    loco->SetBlendOutTime(blendOutTime);
    loco->SetAcceleration(acceleration);
    loco->SetTurningVelocity(rmt::DegToRadian(turningVelocity));

    bank->Store(loco);

    loco->Release();
    return true;
}

bool Locomotion::ReadFromScript8(ScriptReader* script, BaseBank* bank)
{
    static const int DIR_COUNT = 8;
    static const char* TOKEN_LOCOMOTION8 = "locomotion8";
    
    static const char* TOKEN_DIRVALUES[DIR_COUNT] =
    {
        "n",
        "nw",
        "w",
        "sw",
        "s",
        "se",
        "e",
        "ne"
    };
    
    if (!script->IsMoreStructs())
        return false;
    if (strcmp(script->GetNextStructIdentifier(), TOKEN_LOCOMOTION8) != 0)
        return false;
    
    // check for rig
    Rig* rig = bank->GetRig();
    if (rig == 0)
    {
        script->SetErrorString("Locomotion specified without rig.");
    }
    
    script->BeginStruct(TOKEN_LOCOMOTION8);
    
    // required params
    char locoName[256] = "";
    script->RequiredParamString(TOKEN_NAME, locoName, sizeof(locoName));
    
    static const int MAX_ANIMATIONS = 128;
    PURE3D_ANIMATION* anims[MAX_ANIMATIONS];
    memset(anims, 0, sizeof(tAnimation*) * MAX_ANIMATIONS);
    
    static const int MAX_TRANSITIONS = 128;
    Transition* transitions[MAX_TRANSITIONS];
    int transitionCount = script->GetStructCount(TOKEN_TRANSITION);
    if (transitionCount > MAX_TRANSITIONS)
    {
        script->SetErrorString("Too many transitions required to build this locomotion.");
    }
    transitionCount = 0;
    
    // retrieve idle
    char idleName[256] = "";
    script->RequiredParamString(TOKEN_IDLE, idleName, sizeof(idleName));
    
    PURE3D_ANIMATION* idleAnim = 0;
    if (!script->IsError())
    {
        idleAnim = choreo::find<PURE3D_ANIMATION>(bank, idleName);
        
        if (idleAnim == 0)
        {
            script->SetErrorFormat("Could not find idle named \"%s\".",
                idleName);
        }
        else
        {
            for (int i = 0; i < DIR_COUNT; ++i)
            {
                anims[i] = idleAnim;
            }
        }
    }
    
    // optional params
    char partitionName[256] = "";
    script->ParamString(TOKEN_PARTITION, partitionName, sizeof(partitionName));
    
    float weight = 1.0f;
    script->ParamFloat(TOKEN_WEIGHT, &weight);
    
    float priority = 0.0f;
    script->ParamFloat(TOKEN_PRIORITY, &priority);
    
    float rootBlendInTime = 0.0f;
    script->ParamFloat(TOKEN_ROOTBLENDINTIME, &rootBlendInTime);
    
    float rootBlendOutTime = 0.0f;
    script->ParamFloat(TOKEN_ROOTBLENDOUTTIME, &rootBlendOutTime);
    
    float blendInTime = 0.0f;
    script->ParamFloat(TOKEN_BLENDINTIME, &blendInTime);
    
    float blendOutTime = 0.0f;
    script->ParamFloat(TOKEN_BLENDOUTTIME, &blendOutTime);
    
    float acceleration = 0.0f;
    script->ParamFloat(TOKEN_ACCELERATION, &acceleration);
    
    float turningVelocity = 0.0f;
    script->ParamFloat(TOKEN_TURNINGVELOCITY, &turningVelocity);
    
    int velCount = -1;
    
    if (!script->IsError())
    {
        while (script->IsMoreStructs())
        {
            if (strcmp(script->GetNextStructIdentifier(), TOKEN_ANIMATION) == 0)
            {
                script->BeginStruct(TOKEN_ANIMATION);
                
                // get direction
                char dirName[256] = "";
                script->RequiredParamString(TOKEN_DIRECTION, dirName, sizeof(dirName));
                int dirIndex = 0;
                int i;
                for (i = 0; i < DIR_COUNT; ++i)
                {
                    if (strcmp(TOKEN_DIRVALUES[i], dirName) == 0)
                        break;
                }
                
                if (i >= DIR_COUNT)
                {
                    script->SetErrorFormat("Incorrect direction specification \"%s\".", dirName);
                }
                else
                {
                    dirIndex = i;
                }
                
                // get speed grade
                float speed = 0.0f;
                script->RequiredParamFloat(TOKEN_SPEED, &speed);
                int speedIndex = (int)speed;
                
                if (speedIndex < 0)
                {
                    script->SetErrorFormat("Speed grade of %d is less than zero.", speedIndex);
                }
                if (speedIndex > velCount)
                {
                    velCount = speedIndex;
                }
                
                // get index
                int animIndex = ((speedIndex + 1) * DIR_COUNT) + dirIndex;
                if (animIndex > MAX_ANIMATIONS)
                {
                    script->SetErrorString("Too many animations required for locomotion.");
                }
                if ((animIndex >= 0) && (anims[animIndex] != 0))
                {
                    script->SetErrorFormat("Direction \"%s\" and speed %d animation already specified.",
                        dirName, speedIndex);
                }
                
                // get animation
                char p3dAnimName[256] = "";
                script->RequiredParamString(TOKEN_NAME, p3dAnimName, sizeof(p3dAnimName));
                PURE3D_ANIMATION* p3dAnim = 0;
                
                if (strlen(p3dAnimName) != 0)
                {
                    p3dAnim = choreo::find<PURE3D_ANIMATION>(bank, p3dAnimName);
                    if (p3dAnim == 0)
                    {
                        script->SetErrorFormat("Could not find Pure3D animation named \"%s\".",
                            p3dAnimName);
                    }
                }
                
                script->EndStruct();
                
                if (!script->IsError())
                {
                    anims[animIndex] = p3dAnim;
                }
            }
            else if (strcmp(script->GetNextStructIdentifier(), TOKEN_TRANSITION) == 0)
            {
                script->BeginStruct(TOKEN_TRANSITION);
                
                char transitionName[256];
                script->RequiredParamString(TOKEN_NAME, transitionName, sizeof(transitionName));
                
                script->EndStruct();
                
                if (!script->IsError())
                {
                    Transition* transition = choreo::find<Transition>(bank, transitionName);
                    if (transition == 0)
                    {
                        script->SetErrorFormat("Could not find transition named \"%s\".",
                            transitionName);
                    }
                    else
                    {
                        transitions[transitionCount] = transition;
                        ++transitionCount;
                    }
                }
            }
            else
            {
                script->SetErrorFormat("Unexpected token \"%s\", expected \"%s\".",
                    script->GetNextStructIdentifier(),
                    TOKEN_ANIMATION);
            }
        }
    }

    script->EndStruct();

    velCount += 2;
    if (velCount < 2)
    {
        script->SetErrorFormat("Degenerate locomotion, velocity count of %d is less than 2.", velCount);
    }

    // resolve partition (if any)
    Partition* partition = 0;
    if (strlen(partitionName) > 0)
    {
        partition = choreo::find<Partition>(bank, partitionName);
        if (partition == 0)
        {
            script->SetErrorFormat("Could not find partition named \"%s\".", partitionName);
        }
    }

    if (script->IsError())
    {
        return script->HandleError();
    }

    Locomotion* loco = new Locomotion(rig,
                                      DIR_COUNT,
                                      velCount,
                                      anims,
                                      partition,
                                      transitionCount,
                                      transitions);
    loco->AddRef();
    loco->SetName(locoName);
    loco->SetWeight(weight);
    loco->SetPriority((int)priority);
    loco->SetRootBlendInTime(rootBlendInTime);
    loco->SetRootBlendOutTime(rootBlendOutTime);
    loco->SetBlendInTime(blendInTime);
    loco->SetBlendOutTime(blendOutTime);
    loco->SetAcceleration(acceleration);
    loco->SetTurningVelocity(rmt::DegToRadian(turningVelocity));

    bank->Store(loco);

    loco->Release();
    return true;
}

#ifdef CHOREO_TOOL
bool Locomotion::WriteToScriptHandler(ScriptWriter* script, BaseBank* bank)
{
    CHOREO_SCRIPT_WRITE_HANDLER(Locomotion, script, bank);
}

bool Locomotion::WriteToScript(ScriptWriter* script) const
{
    script->BeginStruct(TOKEN_LOCOMOTION);

    // required params
    script->RequiredParamString(TOKEN_NAME, GetName());
    script->RequiredParamFloat(TOKEN_VELOCITYCOUNT, (float)m_VelocityCount);
    script->RequiredParamFloat(TOKEN_DIRECTIONCOUNT, (float)m_DirectionCount);
    script->RequiredParamString(TOKEN_IDLE, m_LocomotionSlots[0]->GetAnimation()->GetName());

    // optional params
	if (GetPartition() != 0)
	{
		script->ParamString(TOKEN_PARTITION, GetPartition()->GetName());
	}

    script->ParamFloat(TOKEN_WEIGHT, GetWeight());
    script->ParamFloat(TOKEN_PRIORITY, (float)GetPriority());
    script->ParamFloat(TOKEN_ROOTBLENDINTIME, GetRootBlendInTime());
    script->ParamFloat(TOKEN_ROOTBLENDOUTTIME, GetRootBlendOutTime());
    script->ParamFloat(TOKEN_BLENDINTIME, GetBlendInTime());
    script->ParamFloat(TOKEN_BLENDOUTTIME, GetBlendOutTime());
    script->ParamFloat(TOKEN_ACCELERATION, GetAcceleration());
    script->ParamFloat(TOKEN_TURNINGVELOCITY, rmt::RadianToDeg(GetTurningVelocity()));

	// animations
    int animCount = GetLocomotionSlotCount();
	int i;
	for (i = m_DirectionCount; i < animCount; ++i)
	{
		script->BeginStruct(TOKEN_ANIMATION);

		// required params
		script->RequiredParamString(TOKEN_NAME, GetLocomotionSlot(i)->GetAnimation()->GetName());

		script->EndStruct();
	}

	// transitions
	for (i = 0; i < m_TransitionCount; ++i)
	{
		script->BeginStruct(TOKEN_TRANSITION);

		// required params
		script->RequiredParamString(TOKEN_NAME, m_Transitions[i]->GetName());

		script->EndStruct();
	}

    script->EndStruct();
	return true;
}
#endif // CHOREO_TOOL


//---------------------------------------------------------------------------
// class LocomotionRootDriver - interface
//   - internal class
//---------------------------------------------------------------------------

class LocomotionRootDriver: public RootBlendDriver
{
public:
    
    LocomotionRootDriver(BlendDriver* blendDriver);

    float GetDeltaTime() const
        { return m_DeltaTime; }
    void SetDeltaTime(float deltaTime)
        { m_DeltaTime = deltaTime; }

    float GetVelocity() const
        { return m_Velocity; }
    void SetVelocity(float velocity)
        { m_Velocity = velocity; }

    float GetMotionAngle() const
        { return m_MotionAngle; }
    void SetMotionAngle(float motionAngle)
        { m_MotionAngle = motionAngle; }

    float GetFacingAngle() const
        { return m_FacingAngle; }
    void SetFacingAngle(float facingAngle)
        { m_FacingAngle = facingAngle; }
    
    virtual void Reset(RootBlender* rootBlender);
    
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
    
    virtual ~LocomotionRootDriver();
    
private:

    BlendDriver* m_BlendDriver;

    float m_DeltaTime;
    float m_Velocity;
    float m_MotionAngle;
    float m_FacingAngle;
};


//---------------------------------------------------------------------------
// class LocomotionRootDriver - implementation
//---------------------------------------------------------------------------

LocomotionRootDriver::LocomotionRootDriver(BlendDriver* blendDriver):
    m_DeltaTime(0.0f),
    m_Velocity(0.0f),
    m_MotionAngle(0.0f),
    m_FacingAngle(0.0f)
{
    P3DASSERT(blendDriver != 0);
    m_BlendDriver = blendDriver;
    m_BlendDriver->AddRef();
}

LocomotionRootDriver::~LocomotionRootDriver()
{
    m_BlendDriver->Release();
}

void LocomotionRootDriver::Reset(RootBlender* rootBlender)
{
    rmt::Vector velocity = rootBlender->GetVelocity();
    velocity.y = 0.0f;
    m_Velocity = velocity.Magnitude();
    m_MotionAngle = GetWorldAngle(velocity.x, velocity.z);
    m_FacingAngle = rootBlender->GetOrientation();
}

float LocomotionRootDriver::GetRootBlendInTime() const
{
    return m_BlendDriver->GetRootBlendInTime();
}

float LocomotionRootDriver::GetRootBlendOutTime() const
{
    return m_BlendDriver->GetRootBlendOutTime();
}

float LocomotionRootDriver::GetRootWeight() const
{
    int aiRootIndex = m_BlendDriver->GetBlend()->GetRig()->GetJointIndex(RIG_JOINT_AI_ROOT);
    return m_BlendDriver->GetJointWeight(aiRootIndex);
}

int LocomotionRootDriver::GetRootPriority() const
{
    int aiRootIndex = m_BlendDriver->GetBlend()->GetRig()->GetJointIndex(RIG_JOINT_AI_ROOT);
    return m_BlendDriver->GetJointPriority(aiRootIndex);
}

bool LocomotionRootDriver::HasRootTranslation() const
{
    return true;
}

void LocomotionRootDriver::GetRootTranslation(const poser::Transform& oldTransform,
                                              rmt::Vector& newTranslation) const
{
    rmt::Vector motionVector = DEFAULT_FACING_VECTOR;
    motionVector.Scale(m_Velocity * m_DeltaTime);
    RotateYVector(m_MotionAngle, motionVector);
    
    newTranslation = oldTransform.GetTranslation();
    newTranslation.Add(motionVector);
}

bool LocomotionRootDriver::HasRootRotation() const
{
    return true;
}

void LocomotionRootDriver::GetRootRotation(const poser::Transform& oldTransform,
                                           rmt::Quaternion& newRotation) const
{
    newRotation.BuildFromAngleY(m_FacingAngle);
}

bool LocomotionRootDriver::HasRootGroundFixup() const
{
    return true;
}

float LocomotionRootDriver::GetRootGroundFixup() const
{
    // FIXME
    return 1.0f;
}


//---------------------------------------------------------------------------
// class LocomotionDriver
//---------------------------------------------------------------------------

LocomotionDriver::LocomotionDriver(Locomotion* loco):
    m_Weight(1.0f),
    m_AnimWeight(1.0f),

    m_DesiredMotionAngle(0.0f),
    m_DesiredFacingAngle(0.0f),
    m_DesiredVelocity(0.0f),
    m_ActualAcceleration(0.0f),

    m_TransitionDriver(0),

    m_IsRootDriverEnabled(true)
{
    P3DASSERT(loco != 0);
    CopyName(loco);

    m_Locomotion = loco;
    m_Locomotion->AddRef();

    m_BlendDriver = loco->GetBlend()->NewBlendDriver();
    m_BlendDriver->AddRef();
    m_BlendDriver->SetIsCyclic(true);

    m_LocomotionRootDriver = new LocomotionRootDriver(m_BlendDriver);
    m_LocomotionRootDriver->AddRef();

    Rig* rig = m_Locomotion->GetRig();
    int legCount = m_Locomotion->GetRig()->GetLegCount();
    m_FootPos = new rmt::Vector [legCount];

    m_Acceleration = m_Locomotion->GetAcceleration();
    m_TurningVelocity = m_Locomotion->GetTurningVelocity();
}

LocomotionDriver::~LocomotionDriver()
{
    tRefCounted::Release(m_TransitionDriver);

    delete[] m_FootPos;

    m_LocomotionRootDriver->Release();
    m_BlendDriver->Release();
    m_Locomotion->Release();
}

PuppetDriver* LocomotionDriver::Clone() const
{
    return new LocomotionDriver(m_Locomotion);
}

Partition* LocomotionDriver::GetPartition() const
{
    return m_BlendDriver->GetPartition();
}

void LocomotionDriver::SetPartition(Partition* partition)
{
    m_BlendDriver->SetPartition(partition);
}

float LocomotionDriver::GetWeight() const
{
    return m_Weight;
}

void LocomotionDriver::SetWeight(float weight)
{
    m_Weight = weight;
    m_BlendDriver->SetWeight(m_Weight * m_AnimWeight);
}

int LocomotionDriver::GetPriority() const
{
    return m_BlendDriver->GetPriority();
}

void LocomotionDriver::SetPriority(int priority)
{
    m_BlendDriver->SetPriority(priority);
}

float LocomotionDriver::GetRootBlendInTime() const
{
    return m_BlendDriver->GetRootBlendInTime();
}

void LocomotionDriver::SetRootBlendInTime(float rootBlendInTime)
{
    m_BlendDriver->SetRootBlendInTime(rootBlendInTime);
}

float LocomotionDriver::GetRootBlendOutTime() const
{
    return m_BlendDriver->GetRootBlendOutTime();
}

void LocomotionDriver::SetRootBlendOutTime(float rootBlendOutTime)
{
    m_BlendDriver->SetRootBlendOutTime(rootBlendOutTime);
}

float LocomotionDriver::GetBlendInTime() const
{
    return m_BlendDriver->GetBlendInTime();
}

void LocomotionDriver::SetBlendInTime(float blendInTime)
{
    m_BlendDriver->SetBlendInTime(blendInTime);
}

float LocomotionDriver::GetBlendOutTime() const
{
    return m_BlendDriver->GetBlendOutTime();
}

void LocomotionDriver::SetBlendOutTime(float blendOutTime)
{
    m_BlendDriver->SetBlendOutTime(blendOutTime);
}

float LocomotionDriver::GetActualMotionAngle() const
{
    return m_LocomotionRootDriver->GetMotionAngle();
}

void LocomotionDriver::SetActualMotionAngle(float motionAngle)
{
    m_LocomotionRootDriver->SetMotionAngle(motionAngle);
    m_DesiredMotionAngle = m_LocomotionRootDriver->GetMotionAngle();
}

void LocomotionDriver::SetDesiredMotionAngle(float desiredMotionAngle)
{
    m_DesiredMotionAngle = Wrap(0.0f, rmt::PI_2, desiredMotionAngle);
}

rmt::Vector LocomotionDriver::GetActualMotionVector() const
{
    rmt::Vector motionVector = DEFAULT_FACING_VECTOR;
    RotateYVector(GetActualMotionAngle(), motionVector);
    return motionVector;
}

void LocomotionDriver::SetActualMotionVector(const rmt::Vector& motion)
{
    SetActualMotionAngle(GetWorldAngle(motion.x, motion.z));
}

rmt::Vector LocomotionDriver::GetDesiredMotionVector() const
{
    rmt::Vector motionVector = DEFAULT_FACING_VECTOR;
    RotateYVector(GetDesiredMotionAngle(), motionVector);
    return motionVector;
}

void LocomotionDriver::SetDesiredMotionVector(const rmt::Vector& motion)
{
    SetDesiredMotionAngle(GetWorldAngle(motion.x, motion.z));
}

float LocomotionDriver::GetActualFacingAngle() const
{
    return m_LocomotionRootDriver->GetFacingAngle();
}

void LocomotionDriver::SetActualFacingAngle(float facingAngle)
{
    m_LocomotionRootDriver->SetFacingAngle(facingAngle);
    m_DesiredFacingAngle = m_LocomotionRootDriver->GetFacingAngle();
}

void LocomotionDriver::SetDesiredFacingAngle(float desiredFacingAngle)
{
    m_DesiredFacingAngle = Wrap(0.0f, rmt::PI_2, desiredFacingAngle);
}

rmt::Vector LocomotionDriver::GetActualFacingVector() const
{
    rmt::Vector facingVector = DEFAULT_FACING_VECTOR;
    RotateYVector(GetActualFacingAngle(), facingVector);
    return facingVector;
}

void LocomotionDriver::SetActualFacingVector(const rmt::Vector& facing)
{
    SetActualFacingAngle(GetWorldAngle(facing.x, facing.z));
}

rmt::Vector LocomotionDriver::GetDesiredFacingVector() const
{
    rmt::Vector facingVector = DEFAULT_FACING_VECTOR;
    RotateYVector(GetDesiredFacingAngle(), facingVector);
    return facingVector;
}

void LocomotionDriver::SetDesiredFacingVector(const rmt::Vector& facing)
{
    SetDesiredFacingAngle(GetWorldAngle(facing.x, facing.z));
}

float LocomotionDriver::GetActualVelocity() const
{
    return m_LocomotionRootDriver->GetVelocity();
}

void LocomotionDriver::SetActualVelocity(float velocity)
{
    m_LocomotionRootDriver->SetVelocity(velocity);
    m_DesiredVelocity = m_LocomotionRootDriver->GetVelocity();
}

void LocomotionDriver::SetDesiredVelocity(float desiredVelocity)
{
    m_DesiredVelocity = desiredVelocity;
    
    if (m_DesiredVelocity < 0.0f)
    {
        m_DesiredVelocity = 0.0f;
    }
}

rmt::Vector LocomotionDriver::GetActualVelocityVector() const
{
    rmt::Vector velVector = DEFAULT_FACING_VECTOR;
    velVector.Scale(GetActualVelocity());
    RotateYVector(GetActualMotionAngle(), velVector);
    return velVector;
}

void LocomotionDriver::SetActualVelocityVector(const rmt::Vector& velocity)
{
    float velMag = rmt::Sqrt((velocity.x * velocity.x) + (velocity.z * velocity.z));
    SetActualVelocity(velMag);
    SetActualMotionAngle(GetWorldAngle(velocity.x, velocity.z));
}

rmt::Vector LocomotionDriver::GetDesiredVelocityVector() const
{
    rmt::Vector velVector = DEFAULT_FACING_VECTOR;
    velVector.Scale(GetDesiredVelocity());
    RotateYVector(GetDesiredMotionAngle(), velVector);
    return velVector;
}

void LocomotionDriver::SetDesiredVelocityVector(const rmt::Vector& velocity)
{
    float velMag = rmt::Sqrt((velocity.x * velocity.x) + (velocity.z * velocity.z));
    SetDesiredVelocity(velMag);
    SetDesiredMotionAngle(GetWorldAngle(velocity.x, velocity.z));
}

float LocomotionDriver::GetActualAnimMaxVelocity() const
{
    float actualFacingAngle = m_LocomotionRootDriver->GetFacingAngle();
    float actualMotionAngle = m_LocomotionRootDriver->GetMotionAngle();
    float relAngle = Wrap(0.0f, rmt::PI_2, GetSmallestArc(actualMotionAngle, actualFacingAngle));
    return m_Locomotion->GetAnimMaxVelocity(relAngle);
}

float LocomotionDriver::GetDesiredAnimMaxVelocity() const
{
    float relAngle = Wrap(0.0f, rmt::PI_2, GetSmallestArc(m_DesiredMotionAngle, m_DesiredFacingAngle));
    return m_Locomotion->GetAnimMaxVelocity(relAngle);
}

float LocomotionDriver::GetAnimMaxVelocity(float motionAngle, float facingAngle) const
{
    float relAngle = Wrap(0.0f, rmt::PI_2, GetSmallestArc(motionAngle, facingAngle));
    return m_Locomotion->GetAnimMaxVelocity(relAngle);
}

float LocomotionDriver::GetPhase() const
{
    return m_BlendDriver->GetPhase();
}

void LocomotionDriver::SetPhase(float phase)
{
    m_BlendDriver->SetPhase(phase);
}

float LocomotionDriver::GetFootPlant(int foot) const
{
    return m_BlendDriver->GetFootPlant(foot);
}

float LocomotionDriver::GetFootLift(int foot) const
{
    return m_BlendDriver->GetFootLift(foot);
}

float LocomotionDriver::GetFootScale(int foot) const
{
    return m_BlendDriver->GetFootScale(foot);
}

float LocomotionDriver::GetFootPhase(int foot) const
{
    return m_BlendDriver->GetFootPhase(foot);
}

bool LocomotionDriver::IsFootPlanted(int foot) const
{
    return m_BlendDriver->IsFootPlanted(foot);
}

float LocomotionDriver::GetMaxFrameCount() const
{
    float frameCount = 0.0f;

    int blendSlotCount = m_BlendDriver->GetBlendSlotCount();
    for (int i = 0; i < blendSlotCount; ++i)
    {
        float curFrameCount = m_BlendDriver->GetBlendSlot(i)->GetFrameCount();
        
        if (curFrameCount > frameCount)
        {
            frameCount = curFrameCount;
        }
    }

    return frameCount;
}

float LocomotionDriver::GetJointWeight(int jointIndex) const
{
    return m_BlendDriver->GetJointWeight(jointIndex);
}

int LocomotionDriver::GetJointPriority(int jointIndex) const
{
    return m_BlendDriver->GetJointPriority(jointIndex);
}

void LocomotionDriver::InitMotion(Engine* engine)
{
    // initialize motion angle, velocity from puppet
    rmt::Vector velocityVector = engine->GetRootBlender()->GetVelocity();
    float velocity = velocityVector.Magnitude();
    float motionAngle = GetWorldAngle(velocityVector.x, velocityVector.z);
    SetActualVelocity(velocity);
    SetActualMotionAngle(motionAngle);
}

void LocomotionDriver::InitFacing(Engine* engine)
{   
    // initialize facing angle from puppet
    SetActualFacingAngle(engine->GetOrientation());
}

void LocomotionDriver::Init(Engine* engine)
{
    InitMotion(engine);
    InitFacing(engine);
}

void LocomotionDriver::FootMatch(Engine* engine)
{
    // first, setup blended animations
    SetPhase(0.0f);
    Advance(0.0f);

    // find max frame count
    int frameCount = (int)GetMaxFrameCount();
    if (frameCount <= 0)
        return;

    // get current foot positions
    poser::Transform invRootTransform = engine->GetRootBlender()->GetRootTransform();
    invRootTransform.Invert();
    
    Rig* rig = m_Locomotion->GetRig();
    int legCount = rig->GetLegCount();
    for (int foot = 0; foot < legCount; ++foot)
    {
        RigLeg* rigLeg = rig->GetLeg(foot);
        rmt::Vector tmp = engine->GetFootPosition(rigLeg->GetIndex());
        
        invRootTransform.GetMatrix().Transform(tmp, m_FootPos + foot);
    }
    
    // iterate over each frame to see which matches best
    float bestPhase = 0.0f;
    float bestMag = FLT_MAX;
    float phaseDelta = 1.0f / (float)frameCount;
    rmt::Vector footPos;
    
    for (int i = 0; i < frameCount; ++i)
    {
        float mag = 0.0f;
        
        for (int foot = 0; foot < legCount; ++foot)
        {
            footPos.Clear();
            m_BlendDriver->GetFootEffectorTranslation(foot, footPos);
            
            footPos.Sub(m_FootPos[foot]);
            
            mag += (footPos.x * footPos.x);
            mag += (footPos.y * footPos.y);
            mag += (footPos.z * footPos.z);
        }
        
        if (mag < bestMag)
        {
            bestPhase = GetPhase();
            bestMag = mag;
        }
        
        // next frame
        SetPhase(GetPhase() + phaseDelta);
        Advance(0.0f);
    }
    
    // set best phase
    SetPhase(bestPhase);
}

void LocomotionDriver::Begin(Puppet* puppet)
{
    // do nothing
}

void LocomotionDriver::UpdateTransition(float deltaTime)
{
    // search for transition to execute if none active
    if (m_TransitionDriver == 0)
    {
        int transitionCount = m_Locomotion->GetTransitionCount();
        for (int i = 0; i < transitionCount; ++i)
        {
            Transition* transition = m_Locomotion->GetTransition(i);
            
            if (transition->Match(GetActualMotionAngle(),
                                  GetActualFacingAngle(),
                                  GetActualVelocity(),
                                  GetActualAcceleration(),
                                  GetDesiredMotionAngle(),
                                  GetDesiredFacingAngle(),
                                  GetDesiredVelocity()))
            {
                AnimationDriver* animDriver = transition->NewAnimationDriver();
                tRefCounted::Assign(m_TransitionDriver, animDriver);
                break;
            }
        }
    }

    // advance transition
    if (m_TransitionDriver != 0)
    {
        if (m_TransitionDriver->IsFinished())
        {
            tRefCounted::Release(m_TransitionDriver);
            m_AnimWeight = 1.0f;
        }
        else
        {
            m_TransitionDriver->Advance(deltaTime);
            
            float blendIn = GetBlendInTime();
            if (m_TransitionDriver->GetTimeRemaining() < blendIn)
            {
                if (blendIn > 0.0f)
                {
                    m_AnimWeight += deltaTime / blendIn;
                    
                    if (m_AnimWeight > 1.0f)
                    {
                        m_AnimWeight = 1.0f;
                    }
                }
                else
                {
                    m_AnimWeight = 1.0f;
                }
            }
            else
            {
                float blendOut = GetBlendOutTime();
                
                if (blendOut > 0.0f)
                {
                    m_AnimWeight -= deltaTime / blendOut;
                    
                    if (m_AnimWeight < 0.0f)
                    {
                        m_AnimWeight = 0.0f;
                    }
                }
                else
                {
                    m_AnimWeight = 0.0f;
                }
            }
        }
    }

    m_BlendDriver->SetWeight(m_Weight * m_AnimWeight);
}

void LocomotionDriver::UpdateFacing(float deltaTime)
{
    // generate actual facing angle
    float actualFacingAngle = GetActualFacingAngle();

    // perform actual turning
    if (m_TurningVelocity <= 0.0f)
    {
        actualFacingAngle = m_DesiredFacingAngle;
    }
    else
    {
        // get desired turn arc
        float arc = GetSmallestArc(actualFacingAngle, m_DesiredFacingAngle);

        float radians = m_TurningVelocity * deltaTime;
        
        if (rmt::Fabs(arc) <= radians)
        {
            actualFacingAngle = m_DesiredFacingAngle;
        }
        else
        {
            if (arc < 0.0f)
            {
                actualFacingAngle -= radians;
            }
            else
            {
                actualFacingAngle += radians;
            }
        }
    }
    
    SetActualFacingAngle(actualFacingAngle);
}

void LocomotionDriver::UpdateMotion(float deltaTime)
{
    // generate actual motion angle + velocity
    float actualMotionAngle = GetActualMotionAngle();
    float actualVelocity = GetActualVelocity();
//    rAssert( actualVelocity < 5.0f );

    if (rmt::Epsilon(actualMotionAngle, m_DesiredMotionAngle) &&
        rmt::Epsilon(actualVelocity, m_DesiredVelocity))
    {
        actualMotionAngle = m_DesiredMotionAngle;
        actualVelocity = m_DesiredVelocity;
        
        m_ActualAcceleration = 0.0f;
    }
    else
    {
        rmt::Vector curDir = DEFAULT_FACING_VECTOR;
        curDir.Scale(actualVelocity);
        RotateYVector(actualMotionAngle, curDir);
        
        rmt::Vector desDir = DEFAULT_FACING_VECTOR;
        desDir.Scale(m_DesiredVelocity);
        RotateYVector(m_DesiredMotionAngle, desDir);
        
        desDir.Sub(curDir);
        float mag = desDir.NormalizeSafe();
        
        if (GetAcceleration() <= 0.0f)
        {
            actualMotionAngle = m_DesiredMotionAngle;
            actualVelocity = m_DesiredVelocity;
            
            m_ActualAcceleration = mag;
        }
        else
        {
            float rate = (GetAcceleration() * deltaTime);
            
            if (mag <= rate)
            {
                actualMotionAngle = m_DesiredMotionAngle;
                actualVelocity = m_DesiredVelocity;
                
                if (deltaTime > 0.0f)
                {
                    m_ActualAcceleration = mag / deltaTime;
                }
            }
            else
            {
                desDir.Scale(rate);
                desDir.Add(curDir);
                
                actualMotionAngle = GetWorldAngle(desDir.x, desDir.z);
                actualVelocity = desDir.Magnitude();
                
                m_ActualAcceleration = GetAcceleration();
            }
        }
    }
    
    SetActualMotionAngle(actualMotionAngle);
    
    SetActualVelocity(actualVelocity);
}

float LocomotionDriver::UpdateBlends()
{
    // get direction count, velocity count, loco animation count
    int directionCount = m_Locomotion->GetDirectionCount();
    int velocityCount = m_Locomotion->GetVelocityCount();
    int locoCount = directionCount * velocityCount;

    // get motion parameters
    float actualMotionAngle = GetActualMotionAngle();
    float actualFacingAngle = GetActualFacingAngle();
    float actualVelocity = GetActualVelocity();

    // establish angle offsets
    float relAngle = Wrap(0.0f, rmt::PI_2, GetSmallestArc(actualMotionAngle, actualFacingAngle));
    float dirAngleInc = rmt::PI_2 / m_Locomotion->GetDirectionCount();

    // blend params
    float params[2] = { 0.0f, 0.0f };

    // calculate U
    int Ua = (int)(relAngle / dirAngleInc) % directionCount;
    int Ub = (Ua + 1) % directionCount;
    float u = UnitWrap((relAngle - (Ua * dirAngleInc)) / dirAngleInc);
    params[1] = Ua + u;

    // calculate V
    float v = 0.0f;
    for (int i = velocityCount - 1; i >= 0; --i)
    {
        int base_inc = (directionCount * i);
        int Ua_base = (Ua + base_inc);
        int Ub_base = (Ub + base_inc);

        float slotVel_Ua_base = m_Locomotion->GetLocomotionSlotVelocity(Ua_base);
        float slotVel_Ub_base = m_Locomotion->GetLocomotionSlotVelocity(Ub_base);

        float curVelocity = ((slotVel_Ua_base * (1.0f - u)) + (slotVel_Ub_base * u));

        if (actualVelocity >= curVelocity)
        {
            int Ua_top = Ua_base + directionCount;

            if (Ua_top >= locoCount)
            {
                // we have surpassed max velocity at this angle,
                // so only blend two animations

                if (curVelocity <= 0.0f)
                {
                    v = 1.0f;
                }
                else
                {
                    v = actualVelocity / curVelocity;
                }

                params[0] = (float)i;
            }
            else
            {
                int Ub_top = Ub_base + directionCount;

                float slotVel_Ua_top = m_Locomotion->GetLocomotionSlotVelocity(Ua_top);
                float slotVel_Ub_top = m_Locomotion->GetLocomotionSlotVelocity(Ub_top);

                float topVelocity = ((slotVel_Ua_top * (1.0f - u)) + (slotVel_Ub_top * u));
                v = (actualVelocity - curVelocity) / (topVelocity - curVelocity);

                params[0] = (float)(i + v);
            }

            break;
        }
    }

    m_BlendDriver->SetParamValue(0, params[0]);
    m_BlendDriver->SetParamValue(1, params[1]);

    return v;
}

void LocomotionDriver::Advance(float deltaTime)
{
    //UpdateTransition(deltaTime);

    // store delta time
    m_LocomotionRootDriver->SetDeltaTime(deltaTime);

    // skip if we've dropped to zero
    if (m_BlendDriver->GetWeight() <= 0.0f)
        return;

    UpdateFacing(deltaTime);
    UpdateMotion(deltaTime);

    float v = UpdateBlends();
    if (v > 1.0f)
    {
        // greater than unit velocity, so speed up time
        deltaTime *= v;
    }

    m_BlendDriver->Advance(deltaTime);
}

bool LocomotionDriver::IsFinished() const
{
    // cyclic, never finishes
    return false;
}

void LocomotionDriver::End(Puppet* puppet)
{
    // do nothing
}

void LocomotionDriver::Apply(Engine* engine)
{
    if (m_BlendDriver->GetWeight() > 0.0f)
    {
        if (m_IsRootDriverEnabled)
        {
            engine->GetRootBlender()->Push(m_LocomotionRootDriver);
        }

        m_BlendDriver->Apply(engine);
    }
    
    if (m_TransitionDriver != 0)
    {
        engine->Push(m_TransitionDriver);
    }
}


} // namespace choreo


// End of file.
