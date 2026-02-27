
/*
These class are used to keep integrity between the units and the physical constants.
All physical values given to the SimClasses should be in CGS (centimeter, gram, second).
Two exceptions there: 
-SimWorld::SetTimeStep(RMFLOAT) takes internal time 
ex.:  In SetTimeStep(SimTime), SimWorld will use SimTime::iValue() to get the  
correct transform value. One could also set TScale = 1000 and use 
SimWorld::SetTimeStep(RMFLOAT frameTime)where frameTime is in msec. 
Or else, TScalecould be set to 30 and the time unit would be the 
number of frame (at 30fps).

-the geometry and transform are in geometry units, not in centimeter
ex.:  If the strees' geometry size are ~5 units tall. In reality 
these guys sould be about 2 m tall or 200 cm. We choose LSCALE = 0.025 
so that 200 * LSCALE = 5. All the physical constants will be internally 
scaled to act on this scale. For this reason, the physical constants  
used in the program don't have to be adapted by the programmer.


  The class SimUnits is the base class who contains the static units scaling factor used to 
  compute the internal value from the CGS entry value. 
  
    SimUnitsManager allow to modify the units scaling factor at the beginning of the program
    
      The classes iClass (i for internal) defined in SimEnvironment.hpp are the ones see by the Sim classes, 
      to ensure that the internal calculation use the internal values.
*/

#ifndef _SIMENV_HPP_
#define _SIMENV_HPP_

#include <radmath/radmath.hpp>
#include <p3d/entity.hpp>
#include <simcommon/tlist.hpp>


namespace sim
{

class SimWind;

class SimUnits
{
public:
    virtual ~SimUnits();
    
    virtual void UpdateUnits() {}
    
    static void Initialize();
    static bool UnitSet() { return sUnitSet; }
    static void Reset();
    
protected:
    SimUnits(); // this class can't be instantiate by itself
    
    static bool  sUnitSet;
    static float sL;  // length scale
    static float sM;  // mass scale
    static float sT;  // time scale
    
    static TList<SimUnits*>* sList;
    
};

//
//
//

class SimulatedObject;

class SimEnvironment
    : public tEntity, 
    protected SimUnits
{
public:
    SimEnvironment();
    
    // collision distance is static
    static float CollisionDistanceCGS()  { return sCollisionDistanceCGS; }
    static float CollisionDistance()     { return sCollisionDistance; }
    static void SetCollisionDistanceCGS(float d);
    
    void SetGravityCGS(float gx, float gy, float gz);
    void SetViscosityCGS(float v);
    const rmt::Vector& GravityCGS()  const { return mGravityCGS; }
    const rmt::Vector& Gravity()     const { return mGravity; }
    float GravityNorm()              const { return mGravityNorm; }
    
    void SetWind(SimWind* inWind);
    SimWind* Wind()                  const { return mSimWind; }
    float Viscosity()                const { return mViscosity; }
    
    void Update(float inDt_sec);
    SimEnvironment & operator = (const SimEnvironment &in_env )
    {
        mGravityCGS           = in_env.mGravityCGS;      
        mGravity              = in_env.mGravity;         
        mGravityNorm          = in_env.mGravityNorm;     
        mViscosityCGS         = in_env.mViscosityCGS;    
        mViscosity            = in_env.mViscosity;       
        
        SetWind( in_env.Wind());
        return *this;
    }

    static SimEnvironment* GetDefaultSimEnvironment();
    static SimEnvironment* GetCurrentDefaultSimEnvironment() { return sDefaultSimEnvironment; }
    
#ifndef RAD_RELEASE
    static TList<SimulatedObject*>* sListOfActiveSimulatedObject;
    static void AddObject(SimulatedObject* inObject) { sListOfActiveSimulatedObject->Add(inObject); }
    static void RemoveObject(SimulatedObject* inObject) { sListOfActiveSimulatedObject->Remove(inObject); }
#else
    static void AddObject(SimulatedObject* inObject) { }
    static void RemoveObject(SimulatedObject* inObject) { }
#endif

protected:

    virtual ~SimEnvironment();

    virtual void UpdateUnits();
    
    rmt::Vector mGravityCGS;      // gravity
    rmt::Vector mGravity;         // gravity, gram*cm / sec^2
    float mGravityNorm;     
    
    float mViscosityCGS;          // ambiant viscosity, gram / sec / cm^2
    float mViscosity;             // 
    SimWind* mSimWind;
    
    static float sCollisionDistanceCGS;
    static float sCollisionDistance;
    
    static SimEnvironment* sDefaultSimEnvironment;
};

//
//
//

class SimCGSValue
    : protected SimUnits
{
public:
    virtual void SetValueCGS(float f) = 0;
    virtual float ValueCGS() const { return mValueCGS; }
    virtual float Value() const { return mValue; }
    
protected:
    SimCGSValue() {}
    virtual ~SimCGSValue() {}
    
    float mValue;
    float mValueCGS;  // gram / cm^3
};

//

class SimTime
    : public SimCGSValue
{
public:
    SimTime(float r=0);
    void SetValueCGS(float t);
};

//

class SimLength
    : public SimCGSValue
{
public:
    SimLength(float r=0);
    void SetValueCGS(float t);
};

//

class SimSpeed
    : public SimCGSValue
{
public:
    SimSpeed(float r=0);
    void SetValueCGS(float t);
};

// cm/sec is not intuitive so this function converts km/hour to cm/sec
inline float KmHourToCmSec(float inKmHour) { return inKmHour * 27.7778f; }

} // sim

#endif // _SimENV_HPP_




