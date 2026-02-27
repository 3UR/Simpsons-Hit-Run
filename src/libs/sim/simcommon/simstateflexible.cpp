/*
#include "simcommon/simstateflexible.hpp"
#include <simcommon/simmath.hpp>
#include "simcommon/dline2.hpp"
#include "simcollision/collisionobject.hpp"
//#include "simflexible/flexibleobject.hpp"
//#include "simflexible/particlesystemsolver.hpp"
//#include "simflexible/particlesystxd.hpp"
#include "p3d/utility.hpp"


using namespace RadicalMathLibrary;


namespace sim
{

SimStateFlexible* SimStateFlexible::CreateSimStateFlexible(tUID inUid, SimStateAttributes inAttrib, tEntityStore* inStore)
{
    // a SimStateFlexible without simulated object doesn't make send
    if (!(inAttrib & SimStateAttribute_Simulated || inAttrib == SimStateAttribute_Default))
        return NULL;
    
    CollisionObject* collObj = NULL;
    FlexibleObject * flexObj = NULL;
    
    if (inAttrib & SimStateAttribute_Collision || inAttrib == SimStateAttribute_Default)
    {
        if (inStore)
            collObj = p3d::find<CollisionObject>(inStore, inUid);
        else
            collObj = p3d::find<CollisionObject>(inUid);
        
        if (!collObj && inAttrib != SimStateAttribute_Default)
        {
            return NULL;
        }
        if (collObj && collObj->GetNumJoint() == 0)
        {
            return NULL;
        }
        if (collObj && collObj->GetSimState())
        {
            return NULL; // can't clone
        }
    }
    
    if (inStore)
        flexObj = p3d::find<FlexibleObject>(inStore, inUid);
    else
        flexObj = p3d::find<FlexibleObject>(inUid);
    
    if (!flexObj)
    {
        return NULL;
    }
    if (flexObj->GetSimState())
    {
        return NULL; // can't clone
    }
    
    SimStateFlexible* simState = new SimStateFlexible();
    
    if (simState == NULL )
    {
        return NULL;
    }
    else
    {
        flexObj->SetSimState(simState);
        simState->SetSimulatedObject(flexObj);
        
        if (collObj)
        {
            collObj->SetSimState(simState);
            simState->SetCollisionObject(collObj);
            collObj->Update();
            simState->mSphereRadius = 2.0f * collObj->GetCollisionVolume()->mSphereRadius;
            
            // temporarilly to make sure that simobject and collobject have same physics properties
            // this will be fixed when the real loaders will be created
            if (collObj)
            {
                collObj->SetPhysicsProperties(flexObj->GetPhysicsProperties());
            }
        }
    }
    return simState;
}

SimStateFlexible* SimStateFlexible::CreateSimStateFlexible(const char* inName, SimStateAttributes inAttrib, tEntityStore* inStore)
{
    return CreateSimStateFlexible(tEntity::MakeUID(inName), inAttrib, inStore);
}

SimStateFlexible* SimStateFlexible::CreateManSimStateFlexible(int m, int n, float size, int inType)
{
    //eLargeStepSolver, eSemiImplicitSolver1D, eSemiImplicitSolver2D
    eParticleSystemSolverType solverType = eLargeStepSolver;
    
    // create the particle system
    ParticleSystem *psyst = NULL;
    float sphereRadius = 0;
    
    if( m==0 && n==0 )
    {
        return NULL;
    }
    else if ( m!=0 && n!=0 )
    {
        if( inType == 1 )
            solverType = eLargeStepSolver;
        else if( inType == 2 )
            solverType = eSemiImplicitSolver2D;
        else if( inType == 3 )
            solverType = eKinematicSolver;
        else
        {
            rAssert(0);
            solverType = eSemiImplicitSolver2D;
        }
        
        psyst = new ParticleSystem2D(m,n,size);
        psyst->SetParticleSystemSolver(solverType);
        
        sphereRadius = Sqrt(Sqr(m*size)+Sqr(n*size));
    }
    else if (m==0)
    {
        if( inType == 1 )
            solverType = eLargeStepSolver;
        else if( inType == 2 )
            solverType = eSemiImplicitSolver1D;
        else if( inType == 3 )
            solverType = eKinematicSolver;
        else
        {
            rAssert(0);
            solverType = eSemiImplicitSolver1D;
        }
        psyst = new ParticleSystem1D( n, size);
        psyst->SetParticleSystemSolver(solverType);//eSemiImplicitSolver1D, eSemiImplicitSolver2D
        
        sphereRadius = n*size;
    }
    else 
    {
        if( inType == 1 )
            solverType = eLargeStepSolver;
        else if( inType == 2 )
            solverType = eSemiImplicitSolver1D;
        else if( inType == 3 )
            solverType = eKinematicSolver;
        else
        {
            rAssert(0);
            solverType = eSemiImplicitSolver1D;
        }
        psyst = new ParticleSystem1D( m, size);
        psyst->SetParticleSystemSolver(solverType);//eSemiImplicitSolver1D, eSemiImplicitSolver2D
        
        sphereRadius = m*size;
    }
    
    psyst->BuildConditions();
    
    //   static tUID MakeUID(const char *s);
    
    // create the flexible object
    FlexibleObject* flexObj = new FlexibleObject(PhysicsProperties::DefaultPhysicsProperties(), psyst);
    
    // create the simstate
    SimStateFlexible* simState = new SimStateFlexible();
    simState->mSphereRadius = 2.0f * sphereRadius;
    
    flexObj->SetSimState(simState);
    simState->SetSimulatedObject(flexObj);
    
    // create the collision object and the collision volume      
    float volumescale = 0.25f;
    rmt::Vector zeropos(0, 0, 0);
    
    CollisionVolume * bbox = new BBoxVolume;
    for (int i=0; i<flexObj->mPsyst->mNbp; i++)
    {
        CollisionVolume * sphere = new SphereVolume(zeropos, flexObj->mPsyst->mDeli*volumescale);
        sphere->SetObjRefIndex(i);
        bbox->AddSubVolume(sphere);
    }
    
    char name[128];
    sprintf( name, "flexible %s", (solverType == eLargeStepSolver)?"LStep":"SemiI");
    
    CollisionObject* collObj = new CollisionObject(bbox);
    collObj->SetName(name);
    collObj->SetNumJoint(flexObj->mPsyst->mNbp);
    collObj->SetSimState(simState);
    
    simState->SetCollisionObject(collObj);
    collObj->Update();
    if (flexObj && collObj)
    {
        collObj->SetPhysicsProperties(flexObj->GetPhysicsProperties());
    }
    return simState;
}

//
//
//
rmt::Matrix sTmpMatrix;

SimStateFlexible::SimStateFlexible(SimControlEnum inControl) 
: SimState(inControl),
  mSphereRadius(0)
{
    sTmpMatrix.Identity();
}

SimStateFlexible::~SimStateFlexible()
{
}

void SimStateFlexible::SetTransform(const rmt::Matrix& inTransform, float dt)
{
    mObjectMoving = !SameMatrix(mTransform, inTransform);
    
    if (mObjectMoving)
    {
        rAssert (Fabs(mScale - ComputeScaleFromMatrix(inTransform))/mScale < MILLI_EPS);
        
        if (dt != 0)
        {
            // must set the velocities for collision purposes.
            ExtractVelocityFromMatrix(mTransform, inTransform, mScale, dt, mVelocityState);
        }
        else
        {
            ResetVelocities();
            
            // object relocation: the safe next step scheme for collision detection doesn't apply anymore
            if (mCollisionObject)
            {
                mCollisionObject->Relocated();
            }
            ((FlexibleObject*) mSimulatedObject)->SynchronizeParticleSystem();
        }
    }
    else
    {
        ResetVelocities();
    }
    
    if (mCollisionObject && (mObjectMoving || ((FlexibleObject*) mSimulatedObject)->IsActivated()))
    {
        MoveCollisionObject(mTransform, inTransform);
    }
    
    mTransform = inTransform;
    
    if (mSimulatedObject)
        mApproxSpeedMagnitude = ((FlexibleObject*)mSimulatedObject)->mPsyst->GetMaxSpeed();
}

const rmt::Matrix& SimStateFlexible::GetTransform(int inIndex) const
{
    // we don't have a matrix for each particle. 
    // this method is dangerous to use since the dame temp matrix might be used
    // more than once in the same equation... the alternative is to allocate an array of matrix
    // with a size = nb of particle.
    if (inIndex >= 0)
    {
        sTmpMatrix.Row(3) = ((FlexibleObject*) mSimulatedObject)->ParticlePosition(inIndex);
        return sTmpMatrix;
    }
    else
        return SimState::GetTransform();
}

const rmt::Vector& SimStateFlexible::GetPosition(int inIndex) const
{
    if (inIndex >= 0)
    {
        return ((FlexibleObject*) mSimulatedObject)->ParticlePosition(inIndex);
    }
    else
        return SimState::GetPosition();
}

void SimStateFlexible::GetVelocity(const rmt::Vector& inPosition, rmt::Vector& oVelocity, int inIndex)
{
    if (inIndex >= 0)
    {
        oVelocity = GetFlexibleObject()->ParticleVelocity(inIndex);
    }
    else
        SimState::GetVelocity(inPosition, oVelocity);
}

void SimStateFlexible::SetHasMoved(bool in_hasMoved)
{
    SimState::SetHasMoved(in_hasMoved);
    
    if (in_hasMoved && GetCollisionObject())
    {
        float volSphereRadius = GetCollisionObject()->GetCollisionVolume()->mSphereRadius;
        if ( volSphereRadius < mSphereRadius )
        {
            GetCollisionObject()->GetCollisionVolume()->mSphereRadius = mSphereRadius;
            GetCollisionObject()->GetCollisionVolume()->mBoxSize.Set(mSphereRadius, mSphereRadius, mSphereRadius);
        }
    }
}

void SimStateFlexible::DebugDisplay(int debugIndex)
{
    DrawLineToggler toggler;

    SimState::DebugDisplay(debugIndex);
    
    if(GetFlexibleObject())
        GetFlexibleObject()->DebugDisplay();
    
    tColour colour(0, 255, 0);
    static float speedScale = 10.0f;
    if ( debugIndex & 1 )
    {
        for (int i=0; i<mSimulatedObject->GetNumSubObj(); i++)
        {
            Vector speed = GetFlexibleObject()->ParticleVelocity(i);
            speed.ScaleAdd(GetFlexibleObject()->ParticlePosition(i), speedScale, speed);
            dStreamLine(GetFlexibleObject()->ParticlePosition(i), speed, colour, colour);
        }

    }
}

} // sim
*/