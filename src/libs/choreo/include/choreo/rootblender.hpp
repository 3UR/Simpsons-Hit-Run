/*
 * choreo/rootblender.hpp
 */

#ifndef CHOREO_ROOTBLENDER_HPP
#define CHOREO_ROOTBLENDER_HPP


#include <poser/posedriver.hpp>
#include <poser/transform.hpp>
#include <choreo/blendtemplate.hpp>


namespace choreo
{


class RootBlendDriver;


//----------------------------------------------------------------------
// class RootBlender
//----------------------------------------------------------------------

class RootBlender: public poser::PoseDriver
{
public:

    enum
    {
        DEFAULT_MAX_DRIVER_COUNT = 32,
            DEFAULT_MAX_BLENDED_PRIORITIES = 4
    };
    
    RootBlender(int rootJointIndex,
                int maxBlendedDrivers = DEFAULT_MAX_DRIVER_COUNT,
                int maxBlendedPriorities = DEFAULT_MAX_BLENDED_PRIORITIES);
    
    int GetRootJointIndex() const
        { return m_RootJointIndex; }
    
    // transform accessors
    const poser::Transform& GetRootTransform() const
        { return m_RootTransform; }
    void SetRootTransform(const poser::Transform& rootTransform);
    
    void SetParentTransform( const poser::Transform& parentTransform );
    const poser::Transform& GetParentTransform() const
        { return m_ParentTransform; }

    const poser::Transform& GetPrevRootTransform() const
        { return m_PrevRootTransform; }
    
    // position accessors
    const rmt::Vector& GetPosition() const
        { return m_RootTransform.GetTranslation(); }
    void SetPosition(const rmt::Vector& position);
    
    const rmt::Vector& GetPrevPosition() const
        { return m_PrevRootTransform.GetTranslation(); }
    
    // orientation accessors
    float GetOrientation() const;
    void SetOrientation(float radians);
    
    float GetPrevOrientation() const;

    // facing vector accessors
    rmt::Vector GetFacingVector() const;
    void SetFacingVector(const rmt::Vector& facingVector);

    rmt::Vector GetPrevFacingVector() const;
    
    // ground point accessors
    void SetGroundPoint(const rmt::Vector& groundPoint);
    const rmt::Vector& GetGroundPoint() const
        { return m_GroundPoint; }
    
    bool IsGroundPointSet() const
        { return m_IsGroundPointSet; }
    void SetIsGroundPointSet(bool isGroundPointSet)
        { m_IsGroundPointSet = isGroundPointSet; }
    
    // FIXME:  can't inline these, gcc-ee barfs! (?)
    float GetGroundFixupWeight() const;
    float GetPrevGroundFixupWeight() const;
    
    // velocity accessors
    const rmt::Vector& GetVelocity() const
        { return m_Velocity; }
    void SetVelocity(const rmt::Vector& velocity)
        { m_Velocity = velocity; }
    float GetTurningVelocity() const
        { return m_TurningVelocity; }
    void SetTurningVelocity(float turningVelocity)
        { m_TurningVelocity = turningVelocity; }
    
    // space conversion
    void WorldToRootSpace(const rmt::Vector& worldSpace, rmt::Vector& rootSpace);
    void RootToWorldSpace(const rmt::Vector& rootSpace, rmt::Vector& worldSpace);
    
    bool Push(RootBlendDriver* rootDriver);
    
    virtual int GetMinimumJointIndex() const
        { return m_RootJointIndex; }
    virtual int GetPriority() const
        { return 0; }
    
    virtual void Advance(float deltaTime);
    virtual void Update(poser::Pose* pose);
    
    void ClearRootDrivers(void);
    int GetRootDriverCount(void) { return m_RootDriverCount;}

protected:
    
    virtual ~RootBlender();
    
private:
    
    void UpdateDriverTranslation(RootBlendDriver* rootDriver,
                                 float rootWeight,
                                 int rootPriority);
    void UpdateDriverRotation(RootBlendDriver* rootDriver,
                              float rootWeight,
                              int rootPriority);
    void UpdateDriverGroundFixup(RootBlendDriver* rootDriver,
                                 float rootWeight,
                                 int rootPriority);
    void UpdateTranslation();
    void UpdateRotation();
    void UpdateGroundFixup();
    
    int m_RootJointIndex;
    
    poser::Transform m_RootTransform;
    poser::Transform m_PrevRootTransform;
    poser::Transform m_ParentTransform;

    rmt::Vector m_GroundPoint;
    bool m_IsGroundPointSet;
    float m_PrevGroundFixupWeight;
    float m_GroundFixupWeight;
    
    float m_DeltaTime;
    
    rmt::Vector m_Velocity;
    float m_TurningVelocity;
    
    int m_MaxRootDrivers;
    
    struct RootDriverStruct
    {
        RootBlendDriver* rootDriver;
        float weight;
        bool pushed;
    };
    
    int m_RootDriverCount;
    RootDriverStruct* m_RootDrivers;
    
    int m_MaxBlendedPriorities;
    
    int m_TranslatePriorityCount;
    BlendPriority<rmt::Vector>* m_TranslatePriorities;
    
    int m_RotatePriorityCount;
    BlendPriority<rmt::Quaternion>* m_RotatePriorities;
    
    int m_GroundFixupPriorityCount;
    BlendPriority<float>* m_GroundFixupPriorities;
};


//----------------------------------------------------------------------
// class RootFixupDriver
//----------------------------------------------------------------------

class RootFixupDriver: public poser::PoseDriver
{
public:
    
    RootFixupDriver(RootBlender* rootBlender);
    
    virtual int GetMinimumJointIndex() const
        { return m_RootBlender->GetMinimumJointIndex(); }
    virtual int GetPriority() const
        { return 0; }
    
    virtual void Advance(float dt);
    virtual void Update(poser::Pose* pose);
    
protected:
    
    virtual ~RootFixupDriver();
    
private:
    
    RootBlender* m_RootBlender;
    float m_DeltaTime;
};


} // namespace choreo


#endif
