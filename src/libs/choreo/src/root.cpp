/*
 * choreo/root.cpp
 */

#include <choreo/root.hpp>
#include <choreo/rootblenddriver.hpp>
#include <choreo/engine.hpp>
#include <choreo/rootblender.hpp>
#include <choreo/constants.hpp>
#include <choreo/utility.hpp>

#include <p3d/error.hpp>


namespace choreo
{


//---------------------------------------------------------------------------
// class RootController
//---------------------------------------------------------------------------

void RootController::GetRootTranslation(const poser::Transform& oldTransform,
                                        rmt::Vector& newTranslation) const
{
    newTranslation = oldTransform.GetTranslation();
}

void RootController::GetRootRotation(const poser::Transform& oldTransform,
                                     rmt::Quaternion& newRotate) const
{
    newRotate = oldTransform.GetQuaternion();
}

PuppetDriver* RootController::NewDriver()
{
    return NewRootDriver();
}

RootDriver* RootController::NewRootDriver()
{
    return new RootDriver(this);
}


//---------------------------------------------------------------------------
// class RootPositionController
//---------------------------------------------------------------------------

RootPositionController::RootPositionController():
    m_Position(0.0f, 0.0f, 0.0f),
    m_GroundFixup(0.0f),
    m_RootBlendInTime(0.0f),
    m_RootBlendOutTime(0.0f),
    m_RootWeight(1.0f),
    m_RootPriority(0)
{
}

RootPositionController::~RootPositionController()
{
}


//---------------------------------------------------------------------------
// class RootRotationController
//---------------------------------------------------------------------------

RootRotationController::RootRotationController():
    m_Quaternion(1.0f, 0.0f, 0.0f, 0.0f),
    m_RootBlendInTime(0.0f),
    m_RootBlendOutTime(0.0f),
    m_RootWeight(1.0f),
    m_RootPriority(0)
{
}

RootRotationController::~RootRotationController()
{
}

float RootRotationController::GetOrientation() const
{
    // do quaternion transform on default facing vector
    rmt::Quaternion q;
    q.Mult(m_Quaternion,
        rmt::Quaternion(1.0f,
            DEFAULT_FACING_VECTOR.x,
            DEFAULT_FACING_VECTOR.y,
            DEFAULT_FACING_VECTOR.z));
    rmt::Quaternion rc;
    rc.Mult(q,
        rmt::Quaternion(m_Quaternion.w,
            -m_Quaternion.x,
            -m_Quaternion.y,
            -m_Quaternion.z));

    return GetWorldAngle(rc.x, rc.z);
}

void RootRotationController::SetOrientation(float orientation)
{
    m_Quaternion.BuildFromAngleY(orientation);
}


//---------------------------------------------------------------------------
// class RootTransformController
//---------------------------------------------------------------------------

RootTransformController::RootTransformController():
    m_GroundFixup(0.0f),
    m_RootBlendInTime(0.0f),
    m_RootBlendOutTime(0.0f),
    m_RootWeight(1.0f),
    m_RootPriority(0)
{
}

RootTransformController::~RootTransformController()
{
}

float RootTransformController::GetOrientation() const
{
    // do quaternion transform on default facing vector
    rmt::Vector facing;
    m_RootTransform.GetMatrix().RotateVector(DEFAULT_FACING_VECTOR, &facing);
    return GetWorldAngle(facing.x, facing.z);
}

void RootTransformController::SetOrientation(float orientation)
{
    rmt::Quaternion q;
    q.BuildFromAngleY(orientation);
    m_RootTransform.SetQuaternion(q);
}


//---------------------------------------------------------------------------
// class RootDriverInternal - interface
//---------------------------------------------------------------------------

class RootDriverInternal: public RootBlendDriver
{
public:
    
    RootDriverInternal(RootController* rootController);
    
    virtual void Reset(RootBlender* rootBlender)
        { m_RootController->Reset(rootBlender); }
    
    // blend times
    virtual float GetRootBlendInTime() const
        { return m_RootController->GetRootBlendInTime(); }
    virtual float GetRootBlendOutTime() const
        { return m_RootController->GetRootBlendOutTime(); }
    
    // weight & priority
    virtual float GetRootWeight() const
        { return m_RootController->GetRootWeight(); }
    virtual int GetRootPriority() const
        { return m_RootController->GetRootPriority(); }
    
    // translation
    virtual bool HasRootTranslation() const
        { return m_RootController->HasRootTranslation(); }
    virtual void GetRootTranslation(const poser::Transform& oldTransform,
                                    rmt::Vector& newTranslation) const
        { m_RootController->GetRootTranslation(oldTransform, newTranslation); }
    
    // rotation
    virtual bool HasRootRotation() const
        { return m_RootController->HasRootRotation(); }
    virtual void GetRootRotation(const poser::Transform& oldTransform,
                                 rmt::Quaternion& newRotation) const
        { m_RootController->GetRootRotation(oldTransform, newRotation); }
    
    // ground fixup
    virtual bool HasRootGroundFixup() const
        { return m_RootController->HasRootGroundFixup(); }
    virtual float GetRootGroundFixup() const
        { return m_RootController->GetRootGroundFixup(); }
    
protected:
    
    virtual ~RootDriverInternal();
    
private:
    
    RootController* m_RootController;
};


//---------------------------------------------------------------------------
// class RootDriverInternal - implementation
//---------------------------------------------------------------------------

RootDriverInternal::RootDriverInternal(RootController* rootController)
{
    P3DASSERT(rootController != 0);
    m_RootController = rootController;
    m_RootController->AddRef();
}

RootDriverInternal::~RootDriverInternal()
{
    m_RootController->Release();
}


//---------------------------------------------------------------------------
// class RootDriver
//---------------------------------------------------------------------------

RootDriver::RootDriver(RootController* rootController)
{
    P3DASSERT(rootController != 0);
    m_RootController = rootController;
    m_RootController->AddRef();
    
    m_RootDriverInternal = new RootDriverInternal(m_RootController);
    m_RootDriverInternal->AddRef();
}

RootDriver::~RootDriver()
{
    m_RootDriverInternal->Release();
    m_RootController->Release();
}

PuppetDriver* RootDriver::Clone() const
{
    return new RootDriver(m_RootController);
}

void RootDriver::Begin(Puppet* puppet)
{
}

void RootDriver::Advance(float deltaTime)
{
    m_RootController->Advance(deltaTime);
}

bool RootDriver::IsFinished() const
{
    return false;
}

void RootDriver::End(Puppet* puppet)
{
}

void RootDriver::Apply(Engine* engine)
{
    engine->GetRootBlender()->Push(m_RootDriverInternal);
}


} // namespace choreo


// End of file.
