/*
 * choreo/foot.hpp
 */

#ifndef CHOREO_FOOT_HPP
#define CHOREO_FOOT_HPP


#include <choreo/driver.hpp>


namespace choreo
{


class FootDriver;
class FootDriverInternal;


//---------------------------------------------------------------------------
// class FootController
//---------------------------------------------------------------------------

class FootController: public PuppetDriverFactory
{
public:

    virtual int GetLegIndex() const = 0;
    
    virtual void Advance(float deltaTime)
        { }
    
    // blend times
    virtual float GetFootBlendInTime() const
        { return 0.0f; }
    virtual float GetFootBlendOutTime() const
        { return 0.0f; }
    
    // weight & priority
    virtual float GetFootWeight() const
        { return 1.0f; }
    virtual int GetFootPriority() const
        { return 0; }
    
    // effector
    virtual bool HasFootEffectorTranslation() const
        { return false; }
    virtual void GetFootEffectorTranslation(rmt::Vector& effTranslate) const
        { }
    
    virtual bool HasFootEffectorRotation() const
        { return false; }
    virtual void GetFootEffectorRotation(rmt::Quaternion& effRotate) const
        { }
    
    // pole vector
    virtual bool HasFootPoleVectorTranslation() const
        { return false; }
    virtual void GetFootPoleVectorTranslation(rmt::Vector& pvTranslate) const
        { }
    
    // foot plant
    virtual bool HasFootPlanted() const
        { return false; }
    virtual bool IsFootPlanted() const
        { return false; }
    
    // ground fixup
    virtual bool HasFootGroundFixup() const
        { return false; }
    virtual float GetFootGroundFixup() const
        { return 0.0f; }
    
    virtual PuppetDriver* NewDriver();
    FootDriver* NewFootDriver();
};


//---------------------------------------------------------------------------
// class FootEffectorPositionController
//---------------------------------------------------------------------------

class FootEffectorPositionController: public FootController
{
public:
    
    FootEffectorPositionController();
    
    // foot index
    virtual int GetLegIndex() const
        { return m_LegIndex; }
    void SetLegIndex(int legIndex)
        { m_LegIndex = legIndex; }
    
    // effector position
    const rmt::Vector& GetEffectorPosition() const
        { return m_EffectorPosition; }
    void SetEffectorPosition(const rmt::Vector& effectorPosition)
        { m_EffectorPosition = effectorPosition; }
    
    // blend times
    virtual float GetFootBlendInTime() const
        { return m_FootBlendInTime; }
    void SetFootBlendInTime(float blendInTime)
        { m_FootBlendInTime = blendInTime; }
    virtual float GetFootBlendOutTime() const
        { return m_FootBlendOutTime; }
    void SetFootBlendOutTime(float blendOutTime)
        { m_FootBlendOutTime = blendOutTime; }
    
    // weight & priority
    virtual float GetFootWeight() const   
        { return m_FootWeight; }
    void SetFootWeight(float weight)
        { m_FootWeight = weight; }
    
    virtual int GetFootPriority() const
        { return m_FootPriority; }
    void SetFootPriority(int priority)
        { m_FootPriority = priority; }
    
    // effector
    virtual bool HasFootEffectorTranslation() const
        { return true; }
    virtual void GetFootEffectorTranslation(rmt::Vector& effTranslate) const;
    
    // foot plant
    virtual bool HasFootPlanted() const
        { return true; }
    virtual bool IsFootPlanted() const
        { return false; }
    
    // ground fixup
    virtual bool HasFootGroundFixup() const
        { return true; }
    virtual float GetFootGroundFixup() const
        { return m_GroundFixup; }
    void SetFootGroundFixup(float groundFixup)
        { m_GroundFixup = groundFixup; }
    
protected:
    
    virtual ~FootEffectorPositionController();
    
private:
    
    int m_LegIndex;
    rmt::Vector m_EffectorPosition;
    float m_GroundFixup;
    float m_FootBlendInTime;
    float m_FootBlendOutTime;
    float m_FootWeight;
    int m_FootPriority;
};


//---------------------------------------------------------------------------
// class FootDriver
//---------------------------------------------------------------------------

class FootDriver: public PuppetDriver
{
public:
    
    FootDriver(FootController* footController);
    
    virtual PuppetDriver* Clone() const;
    
    virtual void Begin(Puppet* puppet);
    virtual void Advance(float deltaTime);
    virtual bool IsFinished() const;
    virtual void End(Puppet* puppet);
    
    virtual void Apply(Engine* engine);
    
protected:
    
    virtual ~FootDriver();
    
private:
    
    FootController* m_FootController;
    FootDriverInternal* m_FootDriverInternal;
};


} // namespace choreo


#endif
