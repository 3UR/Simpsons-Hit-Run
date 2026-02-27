/*
 * choreo/footblender.hpp
 */

#ifndef CHOREO_FOOTBLENDER_HPP
#define CHOREO_FOOTBLENDER_HPP


#include <poser/posedriver.hpp>
#include <poser/transform.hpp>
#include <choreo/blendtemplate.hpp>


namespace choreo
{


class Rig;
class RigLeg;
class FootBlendDriver;


class FootBlender: public poser::PoseDriver
{
public:

    enum
    {
        DEFAULT_MAX_DRIVER_COUNT = 32,
        DEFAULT_MAX_BLENDED_PRIORITIES = 4
    };
    
    FootBlender(Rig* rig,
                RigLeg* rigLeg,
                int maxBlendedDrivers = DEFAULT_MAX_DRIVER_COUNT,
                int maxBlendedPriorities = DEFAULT_MAX_BLENDED_PRIORITIES);
    
    RigLeg* GetRigLeg() const
        { return m_RigLeg; }

    bool IsIKEnabled() const
        { return m_IsIKEnabled; }
    void SetIsIKEnabled(bool isIKEnabled)
        { m_IsIKEnabled = isIKEnabled; }

    bool IsFootPlanted() const
        { return m_IsFootPlanted; }
    
    const poser::Transform& GetEffector() const
        { return m_Effector; }
    poser::Transform& GetEffector()
        { return m_Effector; }
    
    const rmt::Vector& GetPoleVector() const
        { return m_PoleVector; }
    rmt::Vector& GetPoleVector()
        { return m_PoleVector; }
    
    float GetGroundFixupWeight() const
        { return m_GroundFixupWeight; }
    
    bool Push(FootBlendDriver* footDriver);
    
    virtual int GetMinimumJointIndex() const
        { return m_MinJointIndex; }
    virtual int GetPriority() const
        { return 0; }
    
    virtual void Advance(float deltaTime);
    virtual void Update(poser::Pose* pose);
    
protected:
    
    virtual ~FootBlender();
    
private:
    
    void UpdateFootDriver(FootBlendDriver* footDriver,
                          float footWeight,
                          int footPriority);
    void UpdateFoot();
    
    void UpdateFootDriverNoIK(FootBlendDriver* footDriver,
                              float footWeight,
                              int footPriority);
    void UpdateFootNoIK();
    
    RigLeg* m_RigLeg;
    int m_MinJointIndex;
    bool m_IsIKEnabled;

    rmt::Quaternion m_OrientOffset;
    bool m_ApplyOrientOffsetEffector;
    rmt::Matrix m_OrientOffsetInverse;
    bool m_ApplyOrientOffsetPoleVector;

    bool m_IsFootPlanted;
    poser::Transform m_Effector;
    rmt::Vector m_PoleVector;
    float m_GroundFixupWeight;
    
    int m_MaxFootDrivers;
    
    struct FootDriverStruct
    {
        FootBlendDriver* footDriver;
        float weight;
        bool pushed;
    };
    
    int m_FootDriverCount;
    FootDriverStruct* m_FootDrivers;
    
    int m_MaxBlendedPriorities;
    
    // translate blending
    void UpdateEffectorTranslateDriver(FootBlendDriver* footDriver,
                                       float footWeight,
                                       int footPriority);
    void UpdateEffectorTranslate();
    void UpdatePoleVectorTranslateDriver(FootBlendDriver* footDriver,
                                         float footWeight,
                                         int footPriority);
    void UpdatePoleVectorTranslate();
    
    int m_EffectorTranslatePriorityCount;
    BlendPriority<rmt::Vector>* m_EffectorTranslatePriorities;
    
    int m_PoleVectorTranslatePriorityCount;
    BlendPriority<rmt::Vector>* m_PoleVectorTranslatePriorities;
    
    // rotate blending
    void UpdateEffectorRotateDriver(FootBlendDriver* footDriver,
                                    float footWeight,
                                    int footPriority);
    void UpdateEffectorRotate();
    
    int m_EffectorRotatePriorityCount;
    BlendPriority<rmt::Quaternion>* m_EffectorRotatePriorities;
    
    // float blending
    void UpdateFootPlantDriver(FootBlendDriver* footDriver,
                               float footWeight,
                               int footPriority);
    void UpdateFootPlant();
    
    void UpdateGroundFixupDriver(FootBlendDriver* footDriver,
                                 float footWeight,
                                 int footPriority);
    void UpdateGroundFixup();
    
    int m_FootPlantPriorityCount;
    BlendPriority<float>* m_FootPlantPriorities;
    
    int m_GroundFixupPriorityCount;
    BlendPriority<float>* m_GroundFixupPriorities;
};


} // namespace choreo


#endif
