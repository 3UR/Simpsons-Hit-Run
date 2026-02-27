/*
 * choreo/foot.cpp
 */

#include <choreo/foot.hpp>
#include <choreo/footblenddriver.hpp>
#include <choreo/engine.hpp>
#include <choreo/footblender.hpp>

#include <poser/transform.hpp>

#include <p3d/error.hpp>


namespace choreo
{


//---------------------------------------------------------------------------
// class FootController
//---------------------------------------------------------------------------

PuppetDriver* FootController::NewDriver()
{
    return NewFootDriver();
}

FootDriver* FootController::NewFootDriver()
{
    return new FootDriver(this);
}


//---------------------------------------------------------------------------
// class FootEffectorPositionController
//---------------------------------------------------------------------------

FootEffectorPositionController::FootEffectorPositionController():
    m_LegIndex(0),
    m_EffectorPosition(0.0f, 0.0f, 0.0f),
    m_GroundFixup(0.0f),
    m_FootBlendInTime(0.0f),
    m_FootBlendOutTime(0.0f),
    m_FootWeight(1.0f),
    m_FootPriority(0)
{
}

FootEffectorPositionController::~FootEffectorPositionController()
{
}

void FootEffectorPositionController::GetFootEffectorTranslation(rmt::Vector& effTranslate) const
{
    effTranslate = m_EffectorPosition;
}


//---------------------------------------------------------------------------
// class FootDriverInternal - interface
//---------------------------------------------------------------------------

class FootDriverInternal: public FootBlendDriver
{
public:
    
    FootDriverInternal(FootController* footController);
    
    // blending
    virtual float GetFootBlendInTime() const
        { return m_FootController->GetFootBlendInTime(); }
    virtual float GetFootBlendOutTime() const
        { return m_FootController->GetFootBlendOutTime(); }
    
    // weight & priority
    virtual float GetFootWeight() const
        { return m_FootController->GetFootWeight(); }
    virtual int GetFootPriority() const
        { return m_FootController->GetFootPriority(); }
    
    // effector
    virtual bool HasFootEffectorTranslation() const
        { return m_FootController->HasFootEffectorTranslation(); }
    virtual void GetFootEffectorTranslation(rmt::Vector& effTranslate) const
        { m_FootController->GetFootEffectorTranslation(effTranslate); }
    
    virtual bool HasFootEffectorRotation() const
        { return m_FootController->HasFootEffectorRotation(); }
    virtual void GetFootEffectorRotation(rmt::Quaternion& effRotate) const
        { m_FootController->GetFootEffectorRotation(effRotate); }
    
    // pole vector
    virtual bool HasFootPoleVectorTranslation() const
        { return m_FootController->HasFootPoleVectorTranslation(); }
    virtual void GetFootPoleVectorTranslation(rmt::Vector& pvTranslate) const
        { m_FootController->GetFootPoleVectorTranslation(pvTranslate); }
    
    // plant
    virtual bool HasFootPlanted() const
        { return m_FootController->HasFootPlanted(); }
    virtual bool IsFootPlanted() const
        { return m_FootController->IsFootPlanted(); }
    
    // ground fixup
    virtual bool HasFootGroundFixup() const
        { return m_FootController->HasFootGroundFixup(); }
    virtual float GetFootGroundFixup() const
        { return m_FootController->GetFootGroundFixup(); }
    
protected:
    
    virtual ~FootDriverInternal();
    
private:
    
    FootController* m_FootController;
};


//---------------------------------------------------------------------------
// class FootDriverInternal - implementation
//---------------------------------------------------------------------------

FootDriverInternal::FootDriverInternal(FootController* footController)
{
    P3DASSERT(footController != 0);
    m_FootController = footController;
    m_FootController->AddRef();
}

FootDriverInternal::~FootDriverInternal()
{
    m_FootController->Release();
}


//---------------------------------------------------------------------------
// class FootDriver
//---------------------------------------------------------------------------

FootDriver::FootDriver(FootController* footController)
{
    P3DASSERT(footController != 0);
    m_FootController = footController;
    m_FootController->AddRef();
    
    m_FootDriverInternal = new FootDriverInternal(m_FootController);
    m_FootDriverInternal->AddRef();
}

FootDriver::~FootDriver()
{
    m_FootDriverInternal->Release();
    m_FootController->Release();
}

PuppetDriver* FootDriver::Clone() const
{
    return new FootDriver(m_FootController);
}

void FootDriver::Begin(Puppet* puppet)
{
    // do nothing
}

void FootDriver::Advance(float deltaTime)
{
    m_FootController->Advance(deltaTime);
}

bool FootDriver::IsFinished() const
{
    return false;
}

void FootDriver::End(Puppet* puppet)
{
    // do nothing
}

void FootDriver::Apply(Engine* engine)
{
    int legIndex = m_FootController->GetLegIndex();
    
    if ((legIndex >= 0) && (legIndex < engine->GetLegCount()))
    {
        FootBlender* footBlender = engine->GetFootBlender(legIndex);
        if (footBlender != 0)
        {
            footBlender->Push(m_FootDriverInternal);
        }
    }
}


} // namespace choreo


// End of file.
