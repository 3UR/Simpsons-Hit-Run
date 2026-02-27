/*
 * choreo/transition.hpp
 */

#ifndef CHOREO_TRANSITION_HPP
#define CHOREO_TRANSITION_HPP


#include <choreo/buildconfig.hpp>
#include <choreo/animation.hpp>


namespace choreo
{


//---------------------------------------------------------------------------
// class Transition
//  - basically, an Animation with some extra entry/exit parameters
//---------------------------------------------------------------------------

class Transition: public Animation
{
public:

    Transition(Rig* rig,
               PURE3D_ANIMATION* anim,
               Partition* partition = 0);
    
    // motion angle relative to facing angle
    float GetCurrentMotionAngle() const
        { return m_CurrentMotionAngle; }
    void SetCurrentMotionAngle(float angle)
        { m_CurrentMotionAngle = angle; }
    
    float GetCurrentMotionAngleArc() const
        { return m_CurrentMotionAngleArc; }
    void SetCurrentMotionAngleArc(float arc)
        { m_CurrentMotionAngleArc = arc; }
    
    // new motion angle relative to current motion angle
    float GetDesiredMotionAngle() const
        { return m_DesiredMotionAngle; }
    void SetDesiredMotionAngle(float angle)
        { m_DesiredMotionAngle = angle; }
    
    float GetDesiredMotionAngleArc() const
        { return m_DesiredMotionAngleArc; }
    void SetDesiredMotionAngleArc(float arc)
        { m_DesiredMotionAngleArc = arc; }
    
    // new facing angle relative to current motion angle
    float GetDesiredFacingAngle() const
        { return m_DesiredFacingAngle; }
    void SetDesiredFacingAngle(float angle)
        { m_DesiredFacingAngle = angle; }
    
    float GetDesiredFacingAngleArc() const
        { return m_DesiredFacingAngleArc; }
    void SetDesiredFacingAngleArc(float arc)
        { m_DesiredFacingAngleArc = arc; }
    
    // current velocity
    float GetMinCurrentVelocity() const
        { return m_MinCurrentVelocity; }
    void SetMinCurrentVelocity(float velocity)
        { m_MinCurrentVelocity = velocity; }
    
    float GetMaxCurrentVelocity() const
        { return m_MaxCurrentVelocity; }
    void SetMaxCurrentVelocity(float velocity)
        { m_MaxCurrentVelocity = velocity; }
    
    // current acceleration
    float GetMinCurrentAcceleration() const
        { return m_MinCurrentAcceleration; }
    void SetMinCurrentAcceleration(float acceleration)
        { m_MinCurrentAcceleration = acceleration; }
    
    float GetMaxCurrentAcceleration() const
        { return m_MaxCurrentAcceleration; }
    void SetMaxCurrentAcceleration(float acceleration)
        { m_MaxCurrentAcceleration = acceleration; }
    
    // desired velocity
    float GetMinDesiredVelocity() const
        { return m_MinDesiredVelocity; }
    void SetMinDesiredVelocity(float velocity)
        { m_MinDesiredVelocity = velocity; }
    
    float GetMaxDesiredVelocity() const
        { return m_MaxDesiredVelocity; }
    void SetMaxDesiredVelocity(float velocity)
        { m_MaxDesiredVelocity = velocity; }
    
    // matching
    bool Match(float currentMotionAngle,
               float currentFacingAngle,
               float currentVelocity,
               float currentAcceleration,
               float desiredMotionAngle,
               float desiredFacingAngle,
               float desiredVelocity) const;

    // script loading
    static bool ReadFromScript(ScriptReader* script, BaseBank* bank);
    
#ifdef CHOREO_TOOL
    static bool WriteToScriptHandler(ScriptWriter* script, BaseBank* bank);
    bool WriteToScript(ScriptWriter* script) const;
#endif // CHOREO_TOOL

protected:
    
    virtual ~Transition();
    
private:
    
    static inline bool MatchArc(float test, float angle, float arc);
    static inline bool MatchRange(float test, float min, float max);
    
    float m_CurrentMotionAngle;
    float m_CurrentMotionAngleArc;
    
    float m_DesiredMotionAngle;
    float m_DesiredMotionAngleArc;
    
    float m_DesiredFacingAngle;
    float m_DesiredFacingAngleArc;
    
    float m_MinCurrentVelocity;
    float m_MaxCurrentVelocity;
    
    float m_MinCurrentAcceleration;
    float m_MaxCurrentAcceleration;
    
    float m_MinDesiredVelocity;
    float m_MaxDesiredVelocity;
};


} // namespace choreo


#endif
