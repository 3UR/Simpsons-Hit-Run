/*
 * choreo/footplanter.cpp
 */

#include <choreo/footplanter.hpp>
#include <choreo/rig.hpp>
#include <choreo/rootblender.hpp>
#include <choreo/footblender.hpp>
#include <choreo/utility.hpp>
#include <choreo/constants.hpp>

#include <poser/pose.hpp>
#include <poser/joint.hpp>

#include <p3d/error.hpp>
#include <p3d/anim/skeleton.hpp>

#include <float.h>
#include <string.h>


namespace choreo
{


FootPlanter::FootPlanter(Rig* rig,
                         RootBlender* rootBlender,
                         FootBlender** footBlenders,
                         int balanceRootIndex):
    m_BalanceRootIndex(balanceRootIndex),
    m_MinimumJointIndex(balanceRootIndex),
    m_DeltaTime(0.0f)
{
    P3DASSERT(rig != 0);
    m_Rig = rig;
    m_Rig->AddRef();
    
    P3DASSERT(rootBlender != 0);
    m_RootBlender = rootBlender;
    m_RootBlender->AddRef();
    
    P3DASSERT(footBlenders != 0);
    int legCount = m_Rig->GetLegCount();
    m_FootBlenders = new FootBlender* [legCount];
    m_FootYRates = new FootYRate [legCount];
    
    for (int i = 0; i < legCount; ++i)
    {
        m_FootBlenders[i] = footBlenders[i];
        
        if (m_FootBlenders[i] != 0)
        {
            m_FootBlenders[i]->AddRef();
        }

        RigLeg* leg = m_Rig->GetLeg(i);
        m_FootYRates[i].minYRate = leg->GetMinYRate();
        m_FootYRates[i].yRateVelocityScale = leg->GetYRateVelocityScale();
    }

    m_BalanceRootMinYRate = m_Rig->GetBalanceRootMinYRate();
    m_BalanceRootYRateVelocityScale = m_Rig->GetBalanceRootYRateVelocityScale();
    m_MaxBalanceRootCompress = m_Rig->GetMaxBalanceRootCompress();
    
    m_GroundPoints = new rmt::Vector [legCount];
    memset(m_GroundPoints, 0, legCount * sizeof(rmt::Vector));
    
    m_IsGroundPointSets = new bool [legCount];
    memset(m_IsGroundPointSets, 0, legCount * sizeof(bool));
    
    m_GroundYs = new float [legCount];
    
    Reset();
}

FootPlanter::~FootPlanter()
{
    delete[] m_GroundYs;
    delete[] m_IsGroundPointSets;
    delete[] m_GroundPoints;
    
    for (int i = m_Rig->GetLegCount() - 1; i >= 0; --i)
    {
        tRefCounted::Release(m_FootBlenders[i]);
    }
    
    delete[] m_FootYRates;
    delete[] m_FootBlenders;
    m_RootBlender->Release();
    m_Rig->Release();
}

float FootPlanter::GetFootMinYRate(int foot) const
{
    int legCount = m_Rig->GetLegCount();
    P3DASSERT((foot >= 0) && (foot < legCount));
    if ((foot < 0) || (foot >= legCount))
        return 0.0f;

    return m_FootYRates[foot].minYRate;
}

void FootPlanter::SetFootMinYRate(int foot, float minYRate)
{
    int legCount = m_Rig->GetLegCount();
    P3DASSERT((foot >= 0) && (foot < legCount));
    if ((foot < 0) || (foot >= legCount))
        return;

    m_FootYRates[foot].minYRate = minYRate;
}

float FootPlanter::GetFootYRateVelocityScale(int foot) const
{
    int legCount = m_Rig->GetLegCount();
    P3DASSERT((foot >= 0) && (foot < legCount));
    if ((foot < 0) || (foot >= legCount))
        return 0.0f;

    return m_FootYRates[foot].yRateVelocityScale;
}

void FootPlanter::SetFootYRateVelocityScale(int foot, float yRateVelocityScale)
{
    int legCount = m_Rig->GetLegCount();
    P3DASSERT((foot >= 0) && (foot < legCount));
    if ((foot < 0) || (foot >= legCount))
        return;

    m_FootYRates[foot].yRateVelocityScale = yRateVelocityScale;
}

void FootPlanter::Reset()
{
    float y = m_RootBlender->GetPosition().y;
    
    float balanceYRest;
    if (m_BalanceRootIndex >= 0)
    {
        balanceYRest = m_Rig->GetSkeleton()->GetJoint(m_BalanceRootIndex)->restPose.Row(3).y;
    }
    else
    {
        balanceYRest = 0.0f;
    }
    
    m_BalanceY = y + balanceYRest;
    
    for (int i = 0; i < m_Rig->GetLegCount(); ++i)
    {
        m_GroundPoints[i].y = y;
        m_GroundYs[i] = y;
    }
}

void FootPlanter::SetGroundPoint(int foot, const rmt::Vector& groundPoint)
{
    int legCount = m_Rig->GetLegCount();
    P3DASSERT((foot >= 0) && (foot < legCount));
    if ((foot < 0) || (foot >= legCount))
        return;
    
    m_GroundPoints[foot] = groundPoint;
    m_IsGroundPointSets[foot] = true;
}

rmt::Vector FootPlanter::GetGroundPoint(int foot) const
{
    int legCount = m_Rig->GetLegCount();
    P3DASSERT((foot >= 0) && (foot < legCount));
    if ((foot < 0) || (foot >= legCount))
        return rmt::Vector(0.0f, 0.0f, 0.0f);

    return m_GroundPoints[foot];
}

void FootPlanter::Advance(float deltaTime)
{
    m_DeltaTime += deltaTime;
}

void FootPlanter::Update(poser::Pose* pose)
{
    float rootGroundFixupWeight = m_RootBlender->GetGroundFixupWeight();
    if ((m_RootBlender->GetPrevGroundFixupWeight() <= 0.0f) &&
        (rootGroundFixupWeight > 0.0f))
    {
        Reset();
    }

    float rootY = m_RootBlender->GetPosition().y;

    int legCount = m_Rig->GetLegCount();
    
    if (m_BalanceRootIndex >= 0)
    {
        bool gotMinDeltaY = false;
        float minDeltaY = FLT_MAX;
        float maxDeltaY = -FLT_MAX;
        
        float velocity = m_RootBlender->GetVelocity().Magnitude();
        
        for (int i = 0; i < legCount; ++i)
        {
            // blend foot ground point
            float footVelocity = velocity * m_FootYRates[i].yRateVelocityScale;
            
            float footDistMax;
            if (footVelocity < m_FootYRates[i].minYRate)
            {
                footDistMax = m_FootYRates[i].minYRate * m_DeltaTime;
            }
            else
            {
                footDistMax = footVelocity * m_DeltaTime;
            }
            
            if (!m_IsGroundPointSets[i])
            {
                m_GroundPoints[i].y = rootY;
            }
            m_IsGroundPointSets[i] = false;
            m_GroundYs[i] = ApproachFloat(m_GroundYs[i], m_GroundPoints[i].y, footDistMax);
            
            // generate foot delta
            FootBlender* footBlender = m_FootBlenders[i];
            
            if (footBlender == 0)
                continue;
            if (!footBlender->IsEnabled())
                continue;
            
            float groundFixupWeight = footBlender->GetGroundFixupWeight();
            if (groundFixupWeight <= 0.0f)
                continue;
            
            float deltaY = (m_GroundYs[i] - rootY);
            if (deltaY < -m_MaxBalanceRootCompress)
            {
                deltaY = -m_MaxBalanceRootCompress;
            }
            else if (deltaY > m_MaxBalanceRootCompress)
            {
                deltaY = m_MaxBalanceRootCompress;
            }
            
            deltaY *= groundFixupWeight;
            
            gotMinDeltaY = true;
            
            if (deltaY < minDeltaY)
            {
                minDeltaY = deltaY;
            }
            if (deltaY > maxDeltaY)
            {
                maxDeltaY = deltaY;
            }
        }
        
        if (gotMinDeltaY)
        {
            float balanceVelocity = velocity * m_BalanceRootYRateVelocityScale;
            
            float balanceDistMax;
            if (balanceVelocity < m_BalanceRootMinYRate)
            {
                balanceDistMax = m_BalanceRootMinYRate * m_DeltaTime;
            }
            else
            {
                balanceDistMax = balanceVelocity * m_DeltaTime;
            }
            
            rmt::Vector balancePos;
            balancePos = pose->GetJoint(m_BalanceRootIndex)->GetWorldTranslation();
            float balancePosYTarget = balancePos.y + minDeltaY;
            
            m_BalanceY = ApproachFloat(m_BalanceY, balancePosYTarget, balanceDistMax);
            
            // clamp to avoid hyperextension
            float deltaY = (m_BalanceY - balancePos.y);
            float minCompress = minDeltaY - m_MaxBalanceRootCompress;
            if (deltaY < minCompress)
            {
                m_BalanceY = balancePos.y + minCompress;
            }
            else if (deltaY > maxDeltaY)
            {
                m_BalanceY = balancePos.y + maxDeltaY;
            }
            
            float actualDeltaY = (m_BalanceY - balancePos.y);
            balancePos.y = m_BalanceY;
            pose->GetJoint(m_BalanceRootIndex)->SetWorldTranslation(balancePos);
            
            int motionRootIndex = m_Rig->GetJointIndex(RIG_JOINT_MOTION_ROOT);
            int aiRootIndex = m_Rig->GetJointIndex(RIG_JOINT_AI_ROOT);

            for (int i = 0; i < m_Rig->GetLegCount(); ++i)
            {
                FootBlender* footBlender = m_FootBlenders[i];
                
                if (footBlender == 0)
                    continue;
                if (!footBlender->IsEnabled())
                    continue;
                
                float groundFixupWeight = footBlender->GetGroundFixupWeight();
                if (groundFixupWeight <= 0.0f)
                    continue;
                
                float deltaY = (m_GroundYs[i] - rootY) * groundFixupWeight;
                
                if (deltaY < actualDeltaY)
                {
                    deltaY = actualDeltaY;
                    m_GroundYs[i] = (deltaY * groundFixupWeight) + rootY;
                }
                else if (deltaY > m_MaxBalanceRootCompress)
                {
                    deltaY = m_MaxBalanceRootCompress;
                    m_GroundYs[i] = (deltaY * groundFixupWeight) + rootY;
                }
                
                // fix up effector in foot blender
                rmt::Vector effectorPos;
                effectorPos = footBlender->GetEffector().GetTranslation();
                effectorPos.y += deltaY;
                footBlender->GetEffector().SetTranslation(effectorPos);

                int effectorJointIndex = footBlender->GetRigLeg()->GetEffectorJointIndex();

                if (!((aiRootIndex < effectorJointIndex) && (motionRootIndex > effectorJointIndex)))
                {
                    footBlender->GetPoleVector().y += deltaY;

                    int poleVectorJointIndex = footBlender->GetRigLeg()->GetPoleVectorJointIndex();
                    if (poleVectorJointIndex >= 0)
                    {
                        pose->GetJoint(poleVectorJointIndex)->SetObjectTranslation(footBlender->GetPoleVector());
                    }
                }

                // fix up pose as well, if necessary
                if (effectorJointIndex >= 0)
                {
                    pose->GetJoint(effectorJointIndex)->SetObjectTranslation(effectorPos);
                }
            }
        }
    }

    m_DeltaTime = 0.0f;
}


} // namespace choreo


// End of file.
