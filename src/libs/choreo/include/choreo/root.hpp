/*
 * choreo/root.hpp
 */

#ifndef CHOREO_ROOT_HPP
#define CHOREO_ROOT_HPP


#include <choreo/driver.hpp>
#include <poser/transform.hpp>


namespace choreo
{


class RootBlender;
class RootDriver;
class RootDriverInternal;


//---------------------------------------------------------------------------
// class RootController
//---------------------------------------------------------------------------

class RootController: public PuppetDriverFactory
{
public:

    virtual void Reset(RootBlender* rootBlender)
        { }
    virtual void Advance(float deltaTime)
        { }
    
    // blend times
    virtual float GetRootBlendInTime() const
        { return 0.0f; }
    virtual float GetRootBlendOutTime() const
        { return 0.0f; }
    
    // weight & priority
    virtual float GetRootWeight() const
        { return 1.0f; }
    virtual int GetRootPriority() const
        { return 0; }
    
    // translation
    virtual bool HasRootTranslation() const
        { return false; }
    virtual void GetRootTranslation(const poser::Transform& oldTransform,
                                    rmt::Vector& newTranslation) const;
    
    // rotation
    virtual bool HasRootRotation() const
        { return false; }
    virtual void GetRootRotation(const poser::Transform& oldTransform,
                                 rmt::Quaternion& newRotate) const;
    
    // ground fixup
    virtual bool HasRootGroundFixup() const
        { return false; }
    virtual float GetRootGroundFixup() const
        { return 1.0f; }
    
    virtual PuppetDriver* NewDriver();
    RootDriver* NewRootDriver();
};


//---------------------------------------------------------------------------
// class RootPositionController
//---------------------------------------------------------------------------

class RootPositionController: public RootController
{
public:
    
    RootPositionController();
    
    const rmt::Vector& GetPosition() const
        { return m_Position; }
    void SetPosition(const rmt::Vector& position)
        { m_Position = position; }
    
    // blend times
    virtual float GetRootBlendInTime() const
        { return m_RootBlendInTime; }
    void SetRootBlendInTime(float blendInTime)
        { m_RootBlendInTime = blendInTime; }
    virtual float GetRootBlendOutTime() const
        { return m_RootBlendOutTime; }
    void SetRootBlendOutTime(float blendOutTime)
        { m_RootBlendOutTime = blendOutTime; }
    
    // weight & priority
    virtual float GetRootWeight() const
        { return m_RootWeight; }
    void SetRootWeight(float weight)
        { m_RootWeight = weight; }
    
    virtual int GetRootPriority() const
        { return m_RootPriority; }
    void SetRootPriority(int priority)
        { m_RootPriority = priority; }
    
    // translation
    virtual bool HasRootTranslation() const
        { return true; }
    virtual void GetRootTranslation(const poser::Transform& oldTransform,
                                    rmt::Vector& newTranslation) const
        { newTranslation = m_Position; }
    
    // ground fixup
    virtual bool HasRootGroundFixup() const
        { return true; }
    virtual float GetRootGroundFixup() const
        { return m_GroundFixup; }
    void SetRootGroundFixup(float groundFixup)
        { m_GroundFixup = groundFixup; }
    
protected:
    
    virtual ~RootPositionController();
    
private:
    
    rmt::Vector m_Position;
    float m_GroundFixup;
    float m_RootBlendInTime;
    float m_RootBlendOutTime;
    float m_RootWeight;
    int m_RootPriority;
};


//---------------------------------------------------------------------------
// class RootRotationController
//---------------------------------------------------------------------------

class RootRotationController: public RootController
{
public:
    
    RootRotationController();
    
    float GetOrientation() const;
    void SetOrientation(float orientation);

    const rmt::Quaternion& GetQuaternion() const
        { return m_Quaternion; }
    void SetQuaternion(const rmt::Quaternion& quat)
        { m_Quaternion = quat; }

    // blend times
    virtual float GetRootBlendInTime() const
        { return m_RootBlendInTime; }
    void SetRootBlendInTime(float blendInTime)
        { m_RootBlendInTime = blendInTime; }
    virtual float GetRootBlendOutTime() const
        { return m_RootBlendOutTime; }
    void SetRootBlendOutTime(float blendOutTime)
        { m_RootBlendOutTime = blendOutTime; }
    
    // weight & priority
    virtual float GetRootWeight() const
        { return m_RootWeight; }
    void SetRootWeight(float weight)
        { m_RootWeight = weight; }
    
    virtual int GetRootPriority() const
        { return m_RootPriority; }
    void SetRootPriority(int priority)
        { m_RootPriority = priority; }
    
    // rotation
    virtual bool HasRootRotation() const
        { return true; }
    virtual void GetRootRotation(const poser::Transform& oldTransform,
                                 rmt::Quaternion& newRotate) const
        { newRotate = m_Quaternion; }
    
protected:
    
    virtual ~RootRotationController();
    
private:
    
    rmt::Quaternion m_Quaternion;
    float m_RootBlendInTime;
    float m_RootBlendOutTime;
    float m_RootWeight;
    int m_RootPriority;
};


//---------------------------------------------------------------------------
// class RootTransformController
//---------------------------------------------------------------------------

class RootTransformController: public RootController
{
public:
    
    RootTransformController();
    
    // transform accessors
    const poser::Transform& GetRootTransform() const
        { return m_RootTransform; }
    void SetRootTransform(const poser::Transform& rootTransform)
        { m_RootTransform = rootTransform; }

    // position accessors
    const rmt::Vector& GetPosition() const
        { return m_RootTransform.GetTranslation(); }
    void SetPosition(const rmt::Vector& position)
        { m_RootTransform.SetTranslation(position); }

    // rotation accessors
    float GetOrientation() const;
    void SetOrientation(float orientation);

    const rmt::Quaternion& GetQuaternion() const
        { return m_RootTransform.GetQuaternion(); }
    void SetQuaternion(const rmt::Quaternion& quat)
        { m_RootTransform.SetQuaternion(quat); }

    // matrix accessors
    const rmt::Matrix& GetMatrix() const
        { return m_RootTransform.GetMatrix(); }
    void SetMatrix(const rmt::Matrix& matrix)
        { m_RootTransform.SetMatrix(matrix); }

    // blend times
    virtual float GetRootBlendInTime() const
        { return m_RootBlendInTime; }
    void SetRootBlendInTime(float blendInTime)
        { m_RootBlendInTime = blendInTime; }
    virtual float GetRootBlendOutTime() const
        { return m_RootBlendOutTime; }
    void SetRootBlendOutTime(float blendOutTime)
        { m_RootBlendOutTime = blendOutTime; }
    
    // weight & priority
    virtual float GetRootWeight() const
        { return m_RootWeight; }
    void SetRootWeight(float weight)
        { m_RootWeight = weight; }
    
    virtual int GetRootPriority() const
        { return m_RootPriority; }
    void SetRootPriority(int priority)
        { m_RootPriority = priority; }
    
    // translation
    virtual bool HasRootTranslation() const
        { return true; }
    virtual void GetRootTranslation(const poser::Transform& oldTransform,
                                    rmt::Vector& newTranslation) const
        { newTranslation = m_RootTransform.GetTranslation(); }
    
    // rotation
    virtual bool HasRootRotation() const
        { return true; }
    virtual void GetRootRotation(const poser::Transform& oldTransform,
                                 rmt::Quaternion& newRotate) const
        { newRotate = m_RootTransform.GetQuaternion(); }
    
    // ground fixup
    virtual bool HasRootGroundFixup() const
        { return true; }
    virtual float GetRootGroundFixup() const
        { return m_GroundFixup; }
    void SetRootGroundFixup(float groundFixup)
        { m_GroundFixup = groundFixup; }
    
protected:
    
    virtual ~RootTransformController();
    
private:
    
    poser::Transform m_RootTransform;
    float m_GroundFixup;
    float m_RootBlendInTime;
    float m_RootBlendOutTime;
    float m_RootWeight;
    int m_RootPriority;
};


//---------------------------------------------------------------------------
// class RootDriver
//---------------------------------------------------------------------------

class RootDriver: public PuppetDriver
{
public:
    
    RootDriver(RootController* rootController);
    
    virtual PuppetDriver* Clone() const;
    
    virtual void Begin(Puppet* puppet);
    virtual void Advance(float deltaTime);
    virtual bool IsFinished() const;
    virtual void End(Puppet* puppet);
    
    virtual void Apply(Engine* engine);
    
protected:
    
    virtual ~RootDriver();
    
private:
    
    RootController* m_RootController;
    RootDriverInternal* m_RootDriverInternal;
};


} // namespace choreo


#endif
