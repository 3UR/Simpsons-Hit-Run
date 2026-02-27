#include <memory/classsizetracker.h>
#include "simcommon/simenvironment.hpp"
#include "simcommon/simwind.hpp"
#include "simcommon/simlistinitsize.hpp"
#include "simcollision/collisionmanager.hpp"

using namespace RadicalMathLibrary;

namespace sim
{

#define GRAVITYCGS -980.0f

//
// SimUnits class 
//

bool  SimUnits::sUnitSet = false;
float SimUnits::sM = 1.0f;
float SimUnits::sL = 1.0f;
float SimUnits::sT = 1.0f;

void SimUnits::Initialize()
{
    sList = new TList<SimUnits*>( 512 );
}

void SimUnits::Reset()
{
    rAssertMsg(sList == NULL, "SimUnits::Reset: there is still some instances of SimUnits, can't reset");
    sUnitSet = false;
    sM = 1.0f;
    sL = 1.0f;
    sT = 1.0f;
}

TList<SimUnits*>* SimUnits::sList = NULL;

SimUnits::SimUnits()
{
    CLASSTRACKER_CREATE( SimUnits );
    if( sList == NULL )
    {
        Initialize();
    }
    sList->Add(this);
}

SimUnits::~SimUnits()
{
    CLASSTRACKER_DESTROY( SimUnits );
    sList->Remove( this );
    
    //
    // Destroying this list altogether seems like a bad idea
    //
    //if (sList->GetSize() == 0)
    //{
    //    delete sList;
    //    sList = NULL;
    //}
}

//
// SimEnvironment class
//

SimEnvironment* SimEnvironment::sDefaultSimEnvironment = NULL;

#ifndef RAD_RELEASE
TList<SimulatedObject*>* SimEnvironment::sListOfActiveSimulatedObject = NULL;
#endif

float SimEnvironment::sCollisionDistanceCGS = 2.0f;
float SimEnvironment::sCollisionDistance = 2.0f;

void SimEnvironment::SetCollisionDistanceCGS(float d)
{
    sCollisionDistanceCGS = d;

    if (SimUnits::UnitSet())
    {
        sCollisionDistance = sCollisionDistanceCGS * sL;

        if (CollisionManager::GetCurrentInstance())
        {
            CollisionManager::GetCurrentInstance()->GetImpulseBasedCollisionSolver()->UpdateForSimEnvironment();
            CollisionManager::GetCurrentInstance()->GetCollisionDetector()->SetCollisionDistance(sCollisionDistance);
        }
    }
}

//

SimEnvironment* SimEnvironment::GetDefaultSimEnvironment()
{
    if (!sDefaultSimEnvironment)
    {
        sDefaultSimEnvironment = new SimEnvironment;
        sDefaultSimEnvironment->AddRef();
        sDefaultSimEnvironment->SetName("defaultEnvName");
    }
    return sDefaultSimEnvironment;
}

//

SimEnvironment::SimEnvironment()
{
    rAssert(SimUnits::UnitSet() == true);

    mGravityCGS.Set(0, GRAVITYCGS, 0); // 9.8 m/sec^2 == 980 cm/sec^2
    
    mSimWind = NULL;
    mViscosityCGS = 0;
    
    UpdateUnits();
}

SimEnvironment::~SimEnvironment()
{
    if (mSimWind)
        mSimWind->Release();

    if (this == sDefaultSimEnvironment)
        sDefaultSimEnvironment = NULL;
}

void SimEnvironment::Update(float inDt_sec)
{
    if (mSimWind)
        mSimWind->UpdateWind(inDt_sec);
}

void SimEnvironment::UpdateUnits()
{
    mGravity.Scale(1.0f / Sqr(sT), mGravityCGS);
    mGravity.Scale(sL, mGravityCGS);
    mViscosity = (mViscosityCGS * sM) / (sT * Sqr(sL));
    mGravityNorm = mGravity.Magnitude();
    sCollisionDistance = sCollisionDistanceCGS * sL;
}

void SimEnvironment::SetGravityCGS(float gx, float gy, float gz)
{
    mGravityCGS.Set(gx, gy, gz);
    mGravity.Scale(sL / Sqr(sT), mGravityCGS);
    mGravityNorm = mGravity.Magnitude();

    if (this == sDefaultSimEnvironment && CollisionManager::GetCurrentInstance())
    {
        CollisionManager::GetCurrentInstance()->GetImpulseBasedCollisionSolver()->UpdateForSimEnvironment();
    }
}

void SimEnvironment::SetViscosityCGS(float v)
{
    mViscosityCGS = v;
    mViscosity = (mViscosityCGS * sM) / (sT * Sqr(sL));
}

void SimEnvironment::SetWind(SimWind* inWind)
{
    tRefCounted::Assign(mSimWind, inWind);
}


//
//
//

SimSpeed::SimSpeed(float r)
{
    SetValueCGS(r);
}

void SimSpeed::SetValueCGS(float r)
{
    mValueCGS = r;
    mValue = mValueCGS * sL;
    mValue = mValue / sT;
}

SimLength::SimLength(float r)
{
    SetValueCGS(r);
}

void SimLength::SetValueCGS(float r)
{
    mValueCGS = r;
    mValue = mValueCGS * sL;
}

SimTime::SimTime(float r)
{
    SetValueCGS(r);
}

void SimTime::SetValueCGS(float r)
{
    mValueCGS = r;
    mValue = mValueCGS * sT;
}

} // sim


