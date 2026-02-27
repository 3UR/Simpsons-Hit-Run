/*
 * choreo/internalanimation.cpp
 */

#include <choreo/internalanimation.hpp>

#include <p3d/anim/skeleton.hpp>
#include <p3d/error.hpp>


namespace choreo
{


InternalAnimation::InternalAnimation(tSkeleton* skeleton, PURE3D_ANIMATION* anim)
{
    P3DASSERT(anim != 0);
    m_Animation = anim;
    m_Animation->AddRef();

    m_JointMin = 0;
    m_JointMax = -1;

    int jointCount = skeleton->GetNumJoint();

    int i;
    // find min animated joint
    for (i = 0; i < jointCount; ++i)
    {
        tSkeleton::Joint* joint = skeleton->GetJoint(i);
        const tAnimationGroup* animGroup = m_Animation->GetGroupByUID(joint->uid);

        if (animGroup != 0)
        {
            m_JointMin = i;
            m_JointMax = i;
            break;
        }
    }

    // find max animated joint
    for (int j = (jointCount - 1); j > i; --j)
    {
        tSkeleton::Joint* joint = skeleton->GetJoint(j);
        const tAnimationGroup* animGroup = m_Animation->GetGroupByUID(joint->uid);

        if (animGroup != 0)
        {
            m_JointMax = j;
            break;
        }
    }

    if (m_JointMax > 0)
    {
        int jointChannelCount = m_JointMax - m_JointMin + 1;
        m_JointChannels = new JointChannels [jointChannelCount];

        for (i = m_JointMin; i <= m_JointMax; ++i)
        {
            JointChannels* jointChannels = (m_JointChannels + i - m_JointMin);

            tSkeleton::Joint* joint = skeleton->GetJoint(i);
            const tAnimationGroup* animGroup = m_Animation->GetGroupByUID(joint->uid);

            if (animGroup != 0)
            {
                jointChannels->vectorChannel = animGroup->GetVectorChannel(Pure3DAnimationChannels::Pose::TRANSLATION_TRAN);
                jointChannels->rotationChannel = animGroup->GetRotationChannel(Pure3DAnimationChannels::Pose::ROTATION_ROT);
            }
            else
            {
                jointChannels->vectorChannel = 0;
                jointChannels->rotationChannel = 0;
            }
        }
    }
    else
    {
        m_JointChannels = 0;
    }
}

InternalAnimation::~InternalAnimation()
{
    delete[] m_JointChannels;
    m_Animation->Release();
}

bool InternalAnimation::HasJointTranslation(int jointIndex) const
{
    if ((jointIndex < m_JointMin) || (jointIndex > m_JointMax))
        return false;
    return (m_JointChannels[jointIndex - m_JointMin].vectorChannel != 0);
}

void InternalAnimation::GetJointTranslation(int jointIndex, float frame, rmt::Vector& translation) const
{
    m_JointChannels[jointIndex - m_JointMin].vectorChannel->GetValue(frame, &translation);
}

bool InternalAnimation::HasJointRotation(int jointIndex) const
{
    if ((jointIndex < m_JointMin) || (jointIndex > m_JointMax))
        return false;
    return (m_JointChannels[jointIndex - m_JointMin].rotationChannel != 0);
}

void InternalAnimation::GetJointRotation(int jointIndex, float frame, rmt::Quaternion& rotation) const
{
    m_JointChannels[jointIndex - m_JointMin].rotationChannel->GetQuaternion(frame, &rotation);
}

void InternalAnimation::GetJointRotationEuler(int jointIndex, float frame, rmt::Vector& rotation) const
{
    m_JointChannels[jointIndex - m_JointMin].rotationChannel->GetEuler(frame, &rotation);
}

bool InternalAnimation::HasGroupTranslation(tUID groupUID) const
{
    const tAnimationGroup* animGroup = m_Animation->GetGroupByUID(groupUID);
    if (animGroup == 0)
        return false;
    return (animGroup->GetVectorChannel(Pure3DAnimationChannels::Pose::TRANSLATION_TRAN) != 0);
}

void InternalAnimation::GetGroupTranslation(tUID groupUID, float frame, rmt::Vector& translation) const
{
    const tAnimationGroup* animGroup = m_Animation->GetGroupByUID(groupUID);
    const tVectorChannel* vectorChannel = animGroup->GetVectorChannel(Pure3DAnimationChannels::Pose::TRANSLATION_TRAN);
    vectorChannel->GetValue(frame, &translation);
}

bool InternalAnimation::HasGroupRotation(tUID groupUID) const
{
    const tAnimationGroup* animGroup = m_Animation->GetGroupByUID(groupUID);
    if (animGroup == 0)
        return false;
    return (animGroup->GetRotationChannel(Pure3DAnimationChannels::Pose::ROTATION_ROT) != 0);
}

void InternalAnimation::GetGroupRotation(tUID groupUID, float frame, rmt::Quaternion& rotation) const
{
    const tAnimationGroup* animGroup = m_Animation->GetGroupByUID(groupUID);
    const tRotationChannel* rotationChannel = animGroup->GetRotationChannel(Pure3DAnimationChannels::Pose::ROTATION_ROT);
    rotationChannel->GetQuaternion(frame, &rotation);
}

void InternalAnimation::GetGroupRotationEuler(tUID groupUID, float frame, rmt::Vector& rotation) const
{
    const tAnimationGroup* animGroup = m_Animation->GetGroupByUID(groupUID);
    const tRotationChannel* rotationChannel = animGroup->GetRotationChannel(Pure3DAnimationChannels::Pose::ROTATION_ROT);
    rotationChannel->GetEuler(frame, &rotation);
}


} // namespace choreo


// End of file.
