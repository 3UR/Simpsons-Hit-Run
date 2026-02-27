/*
 * choreo/internalanimation.hpp
 */

#ifndef CHOREO_INTERNALANIMATION_HPP
#define CHOREO_INTERNALANIMATION_HPP


#include <choreo/buildconfig.hpp>

#include <p3d/anim/animate.hpp>


class tSkeleton;


namespace choreo
{


typedef tAnimation PURE3D_ANIMATION;


//----------------------------------------------------------------------
// class InternalAnimation
//   - wraps usage of different p3d anim versions
//----------------------------------------------------------------------

class InternalAnimation
{
public:

    InternalAnimation(tSkeleton* skeleton, PURE3D_ANIMATION* anim);
    ~InternalAnimation();

    PURE3D_ANIMATION* GetAnimation() const
        { return m_Animation; }

    tUID GetUID() const
        { return m_Animation->GetUID(); }

    const char* GetName() const
        { return m_Animation->GetName(); }

    bool IsCyclic() const
        { return m_Animation->GetCyclic(); }

    float GetFramesPerSecond() const
        { return m_Animation->GetSpeed(); }

    float GetFrameCount() const
        { return m_Animation->GetNumFrames(); }

    // joint-based channel accessors
    bool HasJointTranslation(int jointIndex) const;
    void GetJointTranslation(int jointIndex, float frame, rmt::Vector& translation) const;
    bool HasJointRotation(int jointIndex) const;
    void GetJointRotation(int jointIndex, float frame, rmt::Quaternion& rotation) const;
    void GetJointRotationEuler(int jointIndex, float frame, rmt::Vector& rotation) const;

    // uid-based channel accessors
    bool HasGroupTranslation(tUID groupUID) const;
    void GetGroupTranslation(tUID groupUID, float frame, rmt::Vector& translation) const;
    bool HasGroupRotation(tUID groupUID) const;
    void GetGroupRotation(tUID groupUID, float frame, rmt::Quaternion& rotation) const;
    void GetGroupRotationEuler(tUID groupUID, float frame, rmt::Vector& rotation) const;

private:

    PURE3D_ANIMATION* m_Animation;

    struct JointChannels
    {
        const tVectorChannel* vectorChannel;
        const tRotationChannel* rotationChannel;
    };

    int m_JointMin;
    int m_JointMax;
    JointChannels* m_JointChannels;
};


} // namespace choreo


#endif
