
#include "simphysics/physicsobject.hpp"
#include "simphysics/articulatedphysicsobject.hpp"

#include "simcommon/simutility.hpp"

// for the loader
#include <p3d/utility.hpp>
#include <stdio.h>
#include <string.h>
#include <p3d/chunkfile.hpp>
#include <p3d/file.hpp>
#include <constants/chunkids.hpp>


using namespace RadicalMathLibrary;

namespace sim
{

//
// PhysicsObject methods
//

static float MaxAngularVelocity = 10.0f;
static float MinAngularVelocity = 0.01f;
static float MaxAngularVelocity2 = MaxAngularVelocity*MaxAngularVelocity;
static float MinAngularVelocity2 = MinAngularVelocity*MinAngularVelocity;

float PhysicsObject::sSmall_Speed = 0;
float PhysicsObject::sThresholdTimeForRestingTest = 0;
float PhysicsObject::sThresholdTimeForContactFriction = 0;

void PhysicsObject::ResetUnitsRelatedValues()
{
    rAssert(SimUnits::UnitSet() == false);
    
    sSmall_Speed = 0;
    sThresholdTimeForRestingTest = 0;
    sThresholdTimeForContactFriction = 0;
}

//
//
//


PhysicsObject::PhysicsObject( PhysicsProperties* inProperties, 
    const Vector& inPos, 
    const SymMatrix& inInertia, 
    float inVolume)
    : SimulatedObject(inProperties),
    mVolumei(inVolume),
    mVolume(inVolume),
    mMass(inVolume),
    mInvMass(1.0f),
    mAxisOfRevolution(NO_AXIS_OF_REVOLUTION),
    mInertiaMatrix0(inInertia),
    mCacheEmpty(true),
    mCacheV(0, 0, 0),
    mCacheL(0, 0, 0),
    mTimeSinceCollide(0),
    mTorqueApplied(false)
{
    mType = RigidObjectType;
    mGeomRotAxis.Clear();
    
    
    SimTime t(2.0f);
    mInvTWDissip = Inverse(t.Value()); 

    if (sSmall_Speed == 0)
    {
        // need to set a small speed, independent of the Units, 0.1 cm/sec is very small...
        SimLength l(0.1f);
        t.SetValueCGS(1.0f);
        sSmall_Speed = l.Value() / t.Value();
        
        t.SetValueCGS(0.5f);
        sThresholdTimeForRestingTest = t.Value();
        
        t.SetValueCGS(0.1f);
        sThresholdTimeForContactFriction = t.Value();
    }

    mInertiaMatrix = mInertiaMatrix0;
    
    mRestingDetector.SetObj(this);
    mRotMat.Identity();
    mCMOffset0.Clear();
    mExternalCMOffset0.Clear();
    mCMPosition = inPos;
    mCMOffset0 = mCMPosition;

    if (mCMOffset0.MagnitudeSqr() < RELA_SMALL)
        mCMOffsetEmpty = true;
    else
        mCMOffsetEmpty = false;
    
    if (mSimState)
        SyncSimObj(false);

    mQ.Identity();
    ResetCache();
    
    mAngularMomentum.Clear();
    ResetAppliedForces();
    
    rAssert(mMass > 0);
    BodyI();
    UpdateForPhysicsProperties();
}

PhysicsObject::PhysicsObject(PhysicsProperties* inProperties)
    : SimulatedObject(inProperties),
    mVolumei(0),
    mVolume(0),
    mMass(0),
    mInvMass(1.0f),
    mAxisOfRevolution(NO_AXIS_OF_REVOLUTION),
    mCacheEmpty(true),
    mCacheV(0, 0, 0),
    mCacheL(0, 0, 0),
    mTimeSinceCollide(0),
    mTorqueApplied(false)
{
    mType = RigidObjectType;
    mGeomRotAxis.Clear();

    SimTime t(10.0f);
    mInvTWDissip = Inverse(t.Value());
    
    if (sSmall_Speed == 0)
    {
        // need to set a small speed, independent of the Units, 0.1 cm/sec is very small...
        SimLength l(0.1f);
        t.SetValueCGS(1.0f);
        sSmall_Speed = l.Value() / t.Value();
        
        t.SetValueCGS(0.5f);
        sThresholdTimeForRestingTest = t.Value();
        
        t.SetValueCGS(0.1f);
        sThresholdTimeForContactFriction = t.Value();
    }
    
    mCMOffset0.Clear();
    mExternalCMOffset0.Clear();
    mCMPosition.Clear();
    
    mRestingDetector.SetObj(this);
    mRotMat.Identity();
    mQ.Identity();
    ResetCache();
    
    mAngularMomentum.Clear();
    ResetAppliedForces();
}

PhysicsObject::~PhysicsObject()
{
}

SimulatedObject* PhysicsObject::Clone(SimState* inSimState)
{
    rAssert(inSimState); // SimState is unique for each object
    
    PhysicsObject* clone = new PhysicsObject();
    SetClone(clone, inSimState);
    return clone;
}

void PhysicsObject::SetClone(PhysicsObject* clone, SimState* inSimState) //const
{
    SimulatedObject::SetClone(clone, inSimState);
    
    clone->mStringData          = mStringData;
    clone->mVolumei             = mVolumei;
    clone->mVolume              = mVolume;
    clone->mMass                = mMass;
    clone->mInvMass             = mInvMass;
    clone->mGeomRotAxis         = mGeomRotAxis;
    clone->mAxisOfRevolution    = mAxisOfRevolution;
    clone->mRotMat              = mRotMat;
    clone->mCMPosition          = mCMPosition;
    clone->mCMOffsetEmpty       = mCMOffsetEmpty;
    clone->mCMOffset0           = mCMOffset0;
    clone->mExternalCMOffset0   = mExternalCMOffset0;
    clone->mAngularMomentum     = mAngularMomentum;
    clone->mQ                   = mQ;
    clone->mInertiaMatrix0      = mInertiaMatrix0;
    clone->mInertiaMatrix       = mInertiaMatrix;
    clone->mInvInertiaMatrix    = mInvInertiaMatrix;
    clone->mCacheEmpty          = true;
    clone->mRestingDetector     = mRestingDetector;
    clone->mRestingDetector.SetObj(clone);
    clone->mAngularMomentum.Clear();
    clone->ResetAppliedForces();
    clone->mTimeSinceCollide    = 0;
    clone->mTorqueApplied       = mTorqueApplied;
    clone->mInvTWDissip         = mInvTWDissip;
}

void PhysicsObject::SyncSimObj(bool inResetState)
{
    mRotMat = mSimState->GetTransform();
    if (mSimState->HasScale())
        mRotMat.OrthoNormal();
    mRotMat.IdentityTranslation();
    
    BodyI();
    mQ.BuildFromMatrix(mRotMat);
    
    if (mCMOffsetEmpty)
    {
        mCMPosition = mSimState->GetPosition();
    }
    else
    {
        Vector off;
        off.Add(mCMOffset0, mExternalCMOffset0);
        mSimState->GetTransform().Transform(off, &mCMPosition);
    }
    
    if (inResetState)
    {
        ResetAppliedForces();
        mAngularMomentum.Clear();
        mSimState->ResetVelocities();
    }
    else
    {
        mInertiaMatrix.TransformVector(mSimState->GetAngularVelocity(), mAngularMomentum);
    }
}

void PhysicsObject::SetExternalCMOffset(const Vector& off)
{
    mExternalCMOffset0 = off;

    Vector totoff;
    totoff.Add(mCMOffset0, mExternalCMOffset0);
    
    if (totoff.MagnitudeSqr() < RELA_SMALL)
        mCMOffsetEmpty = true;
    else
        mCMOffsetEmpty = false;
    
    if (mSimState)
        SyncSimObj(false);
}

void PhysicsObject::UpdateForSimEnvironment()
{
    mRestingDetector.SetNeedReset();
}

void PhysicsObject::UpdateForPhysicsProperties()
{
    rAssert(mVolumei > 0);
    rAssert(mPhysicsProperties->Density() > 0);

    if (mVolume > 0)
    {
        float oldMass = mMass;
        mMass = mVolume * mPhysicsProperties->Density();
        mInvMass = Inverse(mMass);
        mInertiaMatrix0.Scale(mMass / oldMass);
        BodyI();
    }
    else
    {
        // this will happen if an artuculated object has all joints invisible
        // this ibject cannot simulate.
        rAssert(mSimState->GetControl() != simSimulationCtrl);
        mInvMass = mMass = 0.0f;
    }
    
    mRestingDetector.SetNeedReset();
}

void PhysicsObject::UpdateScale()
{
    //   rAssert(mSimState->HasScale());
    float s = mSimState->GetScale();
    mVolume = Sqr(s) * s * mVolumei;
    mRestingDetector.AffectTypicalValuesByScaleFactor(s);
}

void PhysicsObject::Update(float dt_sec)
{
    if (mSimState->GetControl() == simSimulationCtrl && !IsAtRest() && dt_sec > 0)
    {
        SimStats::StartSimulation();

        if (mSimEnvironment)
        {
            if (TestResting(dt_sec))
            {
                mSimState->GetLinearVelocity().Clear();
                mSimState->GetAngularVelocity().Clear();
                return;
            }
            
            mForce.ScaleAdd(mMass, Gravity());
            
            if (mSimEnvironment->Wind())
            {
                // todo: add the wind force
            }
        }
        
        SolveAppliedForces(dt_sec);
        ResetAppliedForces();
        
        mTimeSinceCollide += dt_sec;
    
        SimStats::EndSimulation();
    }
}

bool PhysicsObject::TestResting(float dt)
{
    if (mUseRestingDetector && !IsAtRest() && TimeSinceCollide() < sThresholdTimeForRestingTest)
    {
        mRestingDetector.UpdateRestingCondition(dt);
        mRestingDetector.TestRestingCondition(dt);
    }
    return IsAtRest();
}

void PhysicsObject::WakeUp()
{
    rAssertMsg(mSimState->GetControl() != simAICtrl, "PhysicsObject::WakeUp: Control has to be given to the simulator before calling EndCollision.");
    
    mRestingDetector.ResetRestingCondition();
}

void PhysicsObject::SetCMVelocity(const rmt::Vector& inCMVelocity) 
{ 
    if (mCMOffsetEmpty)
    {
        mSimState->GetLinearVelocity() = inCMVelocity;
    }
    else
    {
        rmt::Vector dp, v;
        dp.Sub(mSimState->GetPosition(), mCMPosition);
        v.CrossProduct(mSimState->GetAngularVelocity(), dp);
        v.Add(inCMVelocity, v);
        mSimState->GetLinearVelocity() = v;
    }
}

void PhysicsObject::AddForce(const Vector& inForce, const Vector* inPos)
{
    mForce.Add(inForce);
    
    if (inPos)
    {
        Vector r;
        r.Sub(*inPos, mCMPosition);
        r.CrossProduct(inForce);
        mTorque.Add(r);
        mTorqueApplied = true;
    }
    WakeUp();
}

void PhysicsObject::AddTorque(const Vector& inTorque)
{
    mTorque.Add(inTorque);
    mTorqueApplied = true;
    WakeUp();
}

void PhysicsObject::SolveAppliedForces(float dt)
{
    rAssert(dt != 0);
    
    // add an small internal friction on the angular velocity. 
    float gam = ClampBetween(1.0f - dt * mInvTWDissip, 0.0f, 1.0f );
    if(gam > 0.0f && TimeSinceCollide() < sThresholdTimeForContactFriction && Type()==RigidObjectType && mAxisOfRevolution<THREE_AXES_OF_REVOLUTION )
    {
        Vector axis, dw, dL;
        switch (mAxisOfRevolution)
        {
            case AXIS_OF_REVOLUTION_ABOUT_X:
            case AXIS_OF_REVOLUTION_ABOUT_Y:
            case AXIS_OF_REVOLUTION_ABOUT_Z:
                axis = mRotMat.Row(mAxisOfRevolution);
                break;
            case ONE_AXIS_OF_REVOLUTION:
                rAssert(mGeomRotAxis.Magnitude()>MILLI_EPS);//Make sure that the axis of revolution is defined in this case.
                mRotMat.RotateVector(mGeomRotAxis,&axis);
                break;
            default:
            case THREE_AXES_OF_REVOLUTION: // to get rid of warning
            case NO_AXIS_OF_REVOLUTION:
                break;
        }
        dw = mSimState->GetAngularVelocity();
        dw.Scale(-(1.0f-gam)*dw.DotProduct(axis),axis);
        mSimState->GetAngularVelocity().Add(dw);
        mInertiaMatrix.TransformVector(mSimState->GetAngularVelocity(), mAngularMomentum);
    }
    else if (gam > 0.0f)
    {
        mAngularMomentum.Scale(gam);
    }
    
    // new velocity and cm
    Vector dr, currentCMVelocity;
    mSimState->GetVelocity(mCMPosition, currentCMVelocity, -1);
    dr = currentCMVelocity;
    currentCMVelocity.ScaleAdd(dt * mInvMass, mForce);
    dr.Add(currentCMVelocity);
    dr.Scale(dt * 0.5f);
    
    // angular velocity and orientation
    Vector w = mSimState->GetAngularVelocity();

    if (mTorqueApplied)
        mAngularMomentum.ScaleAdd(dt, mTorque);

    mInvInertiaMatrix.TransformVector(mAngularMomentum, mSimState->GetAngularVelocity());

    bool wNot0 = true;

    // cap angular velocity
    float nw2 = mSimState->GetAngularVelocity().MagnitudeSqr();
    if (nw2 > MaxAngularVelocity2)
    {
        mSimState->GetAngularVelocity().Scale(MaxAngularVelocity/Sqrt(nw2));
        mInertiaMatrix.TransformVector(mSimState->GetAngularVelocity(), mAngularMomentum);
    }
    else if (nw2 < MinAngularVelocity2)
    {
        if (nw2 > 0)
        {
            mSimState->GetAngularVelocity().Clear();
            mAngularMomentum.Clear();
        }
        nw2 = w.MagnitudeSqr();
        if (nw2 < MinAngularVelocity2)
        {
            w.Clear();
            wNot0 = false;
        }
    }

    if (wNot0)
    {
        w.Add(mSimState->GetAngularVelocity());   // w(t+dt/2) ~ w(t) + w(t+dt) / 2
        w.Scale(dt * 0.5f);                       // w(t+dt/2) * dt
    
        Quaternion q;
        if (WDtToQuat(w, q))
        {
            q.Mult(mQ);
            mQ = q;
            mQ.Normalize();
            mQ.ConvertToMatrix(&mRotMat);
        }
    
        BodyI();
    }
    
    if(!mTorqueApplied)//No torque mean no angular momentum change.
    {
        mInvInertiaMatrix.TransformVector(mAngularMomentum, mSimState->GetAngularVelocity());
    }
    else
    {
        mInertiaMatrix.TransformVector(mSimState->GetAngularVelocity(), mAngularMomentum);
    }
    
    // new center of mass position
    mCMPosition.Add(dr);
    
    // compute new simstate position
    Matrix m;
    if (mSimState->HasScale())
    {
        ScaleRotMatrix(mSimState->GetScale(), mRotMat, m);
    }
    else
    {
        m = mRotMat;
    }
    
    if (mCMOffsetEmpty)
    {
        m.FillTranslate(mCMPosition);
    }
    else
    {
        Vector l_CMOffset;
        l_CMOffset.Add(mCMOffset0, mExternalCMOffset0);
        l_CMOffset.Rotate(mRotMat);
        l_CMOffset.Scale(mSimState->GetScale());
        l_CMOffset.Sub(mCMPosition, l_CMOffset);
        m.FillTranslate(l_CMOffset);
    }
    
    mSimState->SetTransform(m, dt);
    
    // must convert linear velocity of center of mass to velocity of simstate position
    SetCMVelocity(currentCMVelocity);
}

void PhysicsObject::BodyI()
{
    mInertiaMatrix = mInertiaMatrix0;
    mInertiaMatrix.TransformSelf(mRotMat);
    mInvInertiaMatrix.Invert(mInertiaMatrix);
}

void PhysicsObject::ResetAppliedForces()
{
    mForce.Clear();
    mTorque.Clear();
    mTorqueApplied = false;
}

bool PhysicsObject::AddImpulse(const Vector& inPos, const Vector& inImp, const SymMatrix& , SimulatedObject* , int, int )
{
    mCacheEmpty = false;
    mCacheV.Scale(mInvMass, inImp);
    mCacheL.Sub(inPos, mCMPosition);
    mCacheL.CrossProduct(inImp);
    return true;
}

bool PhysicsObject::GetCollisionCache(rmt::Vector& cacheV, rmt::Vector& cacheW, int) const
{
    if (mCacheEmpty)
    {
        return false;
    }
    else
    {
        cacheV = mCacheV;
        mInvInertiaMatrix.TransformVector(mCacheL, cacheW);
        return true;
    }
}

void PhysicsObject::EndCollision(bool fdyn, int, int)
{
    if (!mCacheEmpty)
    {
        if (IsAtRest())
            WakeUp();
        
        Vector dw, v, p;
        mInvInertiaMatrix.TransformVector(mCacheL, dw); //rotation speed increment.
        p.Sub(mCMPosition, mSimState->GetPosition());
        v.CrossProduct(p,dw); // this is the same as v.CrossProduct(dw,p).Scale(-1) to save the scale call
        
        v.Add(mCacheV);
        mSimState->GetLinearVelocity().Add(v);
        mAngularMomentum.Add(mCacheL);
        mInvInertiaMatrix.TransformVector(mAngularMomentum, mSimState->GetAngularVelocity());
        
        mTimeSinceCollide = 0;
    }
    mCacheEmpty = true;
}

void PhysicsObject::GetMassMatrix(const rmt::Vector& inPos, SymMatrix& oM, SimulatedObject* , int, int )
{
    // compute the SkewIinv
    Vector r;
    r.Sub(inPos, mCMPosition);
    oM = mInvInertiaMatrix;
    oM.TransformSelf(r);
    
    // add the mass
    oM.xx += mInvMass;
    oM.yy += mInvMass;
    oM.zz += mInvMass;
}

float PhysicsObject::GetKineticEnergy() const 
{ 
    if (IsAtRest())
    {
        Vector currentCMVelocity;
        GetCMVelocity(currentCMVelocity);
        return mMass * currentCMVelocity.DotProduct(currentCMVelocity) / 2; 
    }
    else
    {
        return 0;
    }
}


// This is a position constraint. To be satisfied, the object is rotated to
// alligned the constrained offset toward the target position. If the constraint is
// still not satified, a translation is applied.
bool PhysicsObject::UpdateConstraint( const LocalTarget &in_LocalTarget, const SimTarget &in_Target, const Vector *in_VelocityTarget, float in_dt )
{
    if ( !(mSimState->GetControl() == simSimulationCtrl && !IsAtRest() && in_dt > 0) )
    {
        return false;
    }
    
    Matrix      l_mat = mSimState->GetTransform();
    Quaternion  l_RotQ;
    Vector      l_vctFrom, l_vctTo, l_vctCnsPosition;
    
    rAssert(mSimState->GetControl() != simSimulationCtrl || SameRotation(l_mat,mRotMat));
    if (!SameRotation(l_mat,mRotMat))
    {
        mRotMat=l_mat;
        mQ.BuildFromMatrix(mRotMat);
    }
    
    //Locate the constrained point on the object:
    l_mat.RotateVector(in_LocalTarget.GetOffset(), &l_vctFrom);//Wolrd constrained direction
    
    //Vector from the object to the constraining position:
    l_vctTo.Sub(in_Target.GetOffset(), mSimState->GetPosition() );//World constraining direction
    
    //Build the rotation to allign the constrained direction on the constraining direction.
    bool l_flip;
    static float maxang=0.01f;
    BuildRotationFromTo( l_vctFrom, l_vctTo, l_RotQ, l_flip, 0.0f, maxang );
    if (l_flip)
    {
        l_RotQ.Identity();
    }
    
    rAssert(l_RotQ.DotProduct(l_RotQ));
    l_RotQ.Mult(mQ);
    rAssert(l_RotQ.DotProduct(l_RotQ));
    mQ = l_RotQ;
    mQ.Normalize();
    mQ.ConvertToMatrix(&mRotMat);
    
    BodyI();
    
    CopyRotation( mRotMat, l_mat );
    
    Vector l_Error;
    l_mat.Transform( in_LocalTarget.GetOffset(), &l_Error );
    l_Error.Sub(in_Target.GetOffset(), l_Error );
    
    l_Error.Add(mSimState->GetPosition());
    l_mat.FillTranslate(l_Error);
    
    mSimState->SetTransform(l_mat);
    
    // At this point, the position contraint is satisfied.
    
    // Now constraint the velocity. The current velocity of the constrained
    // point is:
    //  Vp = Vcm + wXr, where r is the constrained point r = R.Offset
    // The velocity constraint is satisfy if we found a Vcm' and a w' 
    // for which, the new velocity of the point become:
    //  Vp' = Vc. So we have to solve the equation:
    //  Vc - Vp = Vcm' + w'Xr 
    // We get 6 unknows and only one equation. We could probably find other equations
    // as for exemple conservation equation to resolve the system. This has not been test 
    // We can also fix arbitrary one of the two unknows and solve for the other. We
    // could also choose to not satisfy the constraint in velocity also.
    
    static enum { Fix_Vcm, Fix_W, UnConstraintVelocity, other_choose } choose = UnConstraintVelocity;
    switch ( choose )
    {
    case Fix_Vcm:
        {
            //To be done:
            //Arbitrarely fix Vcm' to zero. //Have to solve Vc-Vp = wXr for w.
        }
        break;
    case Fix_W:
        {
            //Arbitrarely fix w' to zero.
            //I test this case and the result is not good.
            Vector vp,r;
            l_mat.Transform(in_LocalTarget.GetOffset(),&r);
            mSimState->GetVelocity(r,vp, -1);
            if (in_VelocityTarget)
            {
                vp.Sub(*in_VelocityTarget,vp);
            }
            else
            {
                vp.Scale(-1.0f);
            }
            mSimState->GetLinearVelocity().Add(vp);
        }
        break;
    case UnConstraintVelocity:
        {
            //The behavior is not "dynamic like" but is stable.
            mSimState->GetLinearVelocity().Clear();
            mSimState->GetAngularVelocity().Clear();
        }
    case other_choose:
    default:
        {
            //Find other ways to solve the system of equations.
        }
        break;
    }
    
    mInertiaMatrix.TransformVector(mSimState->GetAngularVelocity(), mAngularMomentum);
    
#ifdef P3DDEBUG
    Vector l_V2, tmp; //Initial velocity of the constrained point.
    l_mat.Transform( in_LocalTarget.GetOffset(), &tmp);
    mSimState->GetVelocity( tmp , l_V2 , -1);
    if (in_VelocityTarget)
    {
        l_V2.Sub(*in_VelocityTarget,l_V2); //Relative velocity. Should be zero.
    }
    else
    {
        l_V2.Scale(-1.0f);
    }
    float m = l_V2.Magnitude();
    m=m;
#endif
    
    return true;
}

void PhysicsObject::AddConstraint( const LocalTarget &in_LocalTarget )
{
    mSimState->ResetVelocities();
}

//
// PhysicsObjectLoader class
//

static const int PHYSICSOBJECT_VERSION = 1;

void ReadVectorFromPhysicsVectorChunk(tChunkFile* file, Vector& v)
{
    v.x = file->GetFloat();
    v.y = file->GetFloat();
    v.z = file->GetFloat();
}

void ReadSymMatrixFromPhysicsInertiaMatrixChunk(tChunkFile* file, SymMatrix& m)
{
    m.xx = file->GetFloat();
    m.xy = file->GetFloat();
    m.xz = file->GetFloat();
    m.yy = file->GetFloat();
    m.yz = file->GetFloat();
    m.zz = file->GetFloat();
}

//
//
//

PhysicsObjectLoader::PhysicsObjectLoader()
: tSimpleChunkHandler(Simulation::Physics::OBJECT)
{
}

tEntity* PhysicsObjectLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    char name[255], stringData[255];
    
    f->GetPString(name);
    
    int version = f->GetLong();
    rAssert(version == PHYSICSOBJECT_VERSION);
    
    f->GetPString(stringData);
    
    int numJoint = f->GetLong();
    float volume = f->GetFloat();
    float restingSensitivity = f->GetFloat();
    
    pPhysicsJoint* physicsJoint = NULL;
    int numSimJoints = 0;
    
    if (numJoint > 0)
    {
        physicsJoint = new pPhysicsJoint[numJoint];
    }
    
    Vector cm;
    SymMatrix inertia;
    
    while(f->ChunksRemaining())
    {      
        f->BeginChunk();
        switch(f->GetCurrentID())
        {
        case Simulation::Physics::VECTOR:
            {
                ReadVectorFromPhysicsVectorChunk(f, cm);
                break;
            }
        case Simulation::Physics::IMAT:
            {
                ReadSymMatrixFromPhysicsInertiaMatrixChunk(f, inertia);
                break;
            }
        case Simulation::Physics::JOINT:
            {
                int index         = f->GetLong();
                float volume      = f->GetFloat();
                float stiffness   = f->GetFloat();
                float maxAngle    = f->GetFloat();
                float minAngle    = f->GetFloat();
                int dof           = f->GetLong();
                Vector cm(0, 0, 0);
                inertia.Zero();
                
                while(f->ChunksRemaining())
                {      
                    f->BeginChunk();
                    switch(f->GetCurrentID())
                    {
                    case Simulation::Physics::VECTOR:
                        {
                            ReadVectorFromPhysicsVectorChunk(f, cm);
                            break;
                        }
                    case Simulation::Physics::IMAT:
                        {
                            ReadSymMatrixFromPhysicsInertiaMatrixChunk(f, inertia);
                            break;
                        }
                    }
                    f->EndChunk();
                }
                physicsJoint[numSimJoints] = PhysicsJoint::CreatePhysicsJoint(dof, cm, volume, index);
                physicsJoint[numSimJoints]->SetInvStiffness(stiffness);
                physicsJoint[numSimJoints]->SetConstraintAngles(minAngle, maxAngle);
                physicsJoint[numSimJoints]->SetInertiaMatrix(inertia);
                numSimJoints++;
                break;
            }
        }
        f->EndChunk();
    }
    
    //Dont do this search... it eats up loading time
    //PhysicsProperties* tmpprop = PhysicsProperties::FindPhysicsProperty(stringData, store);
    PhysicsProperties* tmpprop = NULL;

    if (!tmpprop)
    {
        if (numSimJoints == 0)
        {
            tmpprop = PhysicsProperties::DefaultPhysicsProperties(store);
        }
        else
        {
            tmpprop = PhysicsProperties::DefaultArtPhysicsProperties(store);
        }
    }
    
    PhysicsObject* newPhysicsObject = NULL;
    
    if (numJoint == 0)
    {
        newPhysicsObject = new PhysicsObject(tmpprop, cm, inertia, volume);
    }
    else if (numSimJoints > 0)
    {
        newPhysicsObject = new ArticulatedPhysicsObject(tmpprop, physicsJoint, numSimJoints);
    }
    if (newPhysicsObject)
    {
        newPhysicsObject->SetName(name);
        newPhysicsObject->mStringData.SetText(stringData);
    }
    newPhysicsObject->SetSimEnvironment(SimEnvironment::GetDefaultSimEnvironment());
    
    return newPhysicsObject;
}

} // sim
