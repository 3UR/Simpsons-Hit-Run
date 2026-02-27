

#ifndef _SIMSTATEFLEXIBLE_HPP_
#define _SIMSTATEFLEXIBLE_HPP_

#include "simcommon/simstate.hpp"
#include <raddebug.hpp>

namespace sim
{

class FlexibleObject;

class SimStateFlexible
    : public SimState
{
public:
    static SimStateFlexible* CreateSimStateFlexible(tUID inUID,         SimStateAttributes inAttrib = SimStateAttribute_Default, tEntityStore* inStore=NULL );
    static SimStateFlexible* CreateSimStateFlexible(const char* inName, SimStateAttributes inAttrib = SimStateAttribute_Default, tEntityStore* inStore=NULL );
    static SimStateFlexible* CreateManSimStateFlexible(int m, int n, float size, int inType);
    
    void GetVelocity(const rmt::Vector& inPosition, rmt::Vector& oVelocity, int inIndex);
    
    virtual void SetTransform(const rmt::Matrix& inTransform, float dt = 0);

    //using SimState::GetPosition;  // so that it is not hidden by GetPosition(int inIndex)
    //using SimState::GetTransform;  // so that it is not hidden by GetTransform(int inIndex)
    const rmt::Vector& GetPosition() const { return SimState::GetPosition(); }
    const rmt::Matrix& GetTransform() const { return SimState::GetTransform(); }
    
    virtual const rmt::Matrix& GetTransform(int inIndex) const;
    virtual const rmt::Vector& GetPosition(int inIndex)  const;
    
    virtual void SetHasMoved(bool in_hasMoved);
    
    virtual void DebugDisplay(int debugIndex = 0);
    
    virtual bool RequiresPushTransforForDisplay() const { return false; }
    
    FlexibleObject* GetFlexibleObject() { return (FlexibleObject*) mSimulatedObject; }
    
    SimStateFlexible(SimControlEnum inControl = simAICtrl);
    
protected:
    
    ~SimStateFlexible();
    
    float mSphereRadius;
    
private:
    SimStateFlexible& operator=(SimStateFlexible& inObj) { rAssert(false); return *this; }
    SimStateFlexible(const SimStateFlexible& inObj) { rAssert(false); }
};

} // sim

#endif // _SIMSTATEFLEXIBLE_HPP_
