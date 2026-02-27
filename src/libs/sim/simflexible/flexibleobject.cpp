
#include "simflexible/flexibleobject.hpp"
#include "simflexible/psystongeom2d.hpp"
#include "simcommon/simstate.hpp"
#include "simcommon/simutility.hpp"

using namespace RadicalMathLibrary;

namespace sim
{

static bool SET_FIX_SPEED = false;

FlexibleObject::FlexibleObject(PhysicsProperties* inProperties, ParticleSystem* ps) : 
SimulatedObject(inProperties),
    mPsyst(ps),
    mSystemScale(1.0f),
    mImpScale(-0.1f),
    mIsActivated(true)
{
    rAssert(mPsyst != NULL);
    
    mType = FlexibleObjectType;
    
    mPsyst->mPhysicsProperties = mPhysicsProperties;
    
    mParticleTargets.DontUseMemCpy();

    SetSimEnvironment(SimEnvironment::GetDefaultSimEnvironment());
    UpdateForSimEnvironment();
}

FlexibleObject::FlexibleObject(PhysicsProperties* inProperties) : 
SimulatedObject(inProperties),
    mPsyst(NULL),
    mSystemScale(1.0f),
    mImpScale(-0.1f),
    mIsActivated(true)
{
    mType = FlexibleObjectType;
    mParticleTargets.DontUseMemCpy();
}

FlexibleObject::~FlexibleObject()
{
    delete mPsyst;
}

SimulatedObject* FlexibleObject::Clone( SimState *inSimState)
{
    // this is not possible!
    return NULL;
}

void FlexibleObject::UpdateForPhysicsProperties() 
{
}

void FlexibleObject::UpdateForSimEnvironment() 
{
    mPsyst->SetSimEnvironment(mSimEnvironment);
}

bool FlexibleObject::IsAtRest() const
{
    return false;
}

bool FlexibleObject::TestResting(float dt)
{
    return true;
}

void FlexibleObject::WakeUp()
{
}

int FlexibleObject::GetNumSubObj() const
{
    return mPsyst->mNbp;
}

void FlexibleObject::SetScale(float f)
{
    rAssert(f > 0);
    
    float ratio = f/mSystemScale;
    mSystemScale = f;
    mPsyst->Scale(ratio);
    
    for (int i=0; i<mParticleTargets.GetSize(); i++)
    {
        Vector v = mParticleTargets[i].mSimTarget->GetOffset();
        v.Scale(ratio);
        mParticleTargets[i].mSimTarget->SetOffset(v);
    }
}

const rmt::Vector& FlexibleObject::ParticlePosition(int p) const
{
    rAssert(p >= 0 && p<mPsyst->mNbp);
    return *(Vector*)mPsyst->mP[p]->m_x;
}

const rmt::Vector& FlexibleObject::ParticleVelocity(int p) const
{
    rAssert(p >= 0 && p<mPsyst->mNbp);
    return *(Vector*)mPsyst->mP[p]->m_v;
}

void FlexibleObject::SetInternalParams(const ConditionParams& p)
{
    mPsyst->SetParams(p);
}

void FlexibleObject::AddFixParticleTarget(int p, SimTarget* t)
{
    rAssert(t != NULL);
    rAssert(p >= 0 && p < mPsyst->mNbp);
    
    if (mPsyst->IsParticleFree(p))
    {
        mParticleTargets.NewElem().Set(p, t);
        mPsyst->FixParticle(p);
    }
    else
    {
        bool found = false;
        for (int i=0; i<mParticleTargets.GetSize(); i++)
        {
            if (mParticleTargets[i].mParticleIndex == p)
            {
                mParticleTargets[i].Set(p, t);
                found = true;
            }
        }
        if (!found) // should not get there...
        {
            mParticleTargets.NewElem().Set(p, t);
            mPsyst->FixParticle(p);
        }
    }
}

void FlexibleObject::FreeParticle(int p)
{
    if (!mPsyst->IsParticleFree(p))
    {
        for (int i=0; i<mParticleTargets.GetSize(); i++)
        {
            if (mParticleTargets[i].mParticleIndex == p)
            {
                mParticleTargets.RemoveAt(i);
                mPsyst->FreeParticle(p);
            }
        }
    }
}

void FlexibleObject::ResetPos()
{
    ParticleSystemOnGeom2D* pg = dynamic_cast<ParticleSystemOnGeom2D*>(mPsyst);
    if (pg)
        pg->ResetPos();
    SynchronizeParticleSystem();
}

void FlexibleObject::SynchronizeParticleSystem()
{
    // sync only with first particle target
    if (mParticleTargets.GetSize() > 0)
    {
        Vector p;
        mPsyst->GetPosParticle(mParticleTargets[0].mParticleIndex, p.x, p.y, p.z);
        
        Vector pos;
        Matrix m;
        mParticleTargets[0].mSimTarget->GetPos(pos);
        pos.Sub(p);
        
        mPsyst->MoveSystem(pos.x, pos.y, pos.z);
        
        mPsyst->PreDisplay();
    }
}

bool FlexibleObject::NeighborFixParticle(int p)
{
    bool ret = false;
    for (int i=0; i<mParticleTargets.GetSize(); i++)
    {
        if (mPsyst->NeighborParticle(p, mParticleTargets[i].mParticleIndex))
        {
            ret = true;
            break;
        }
    }
    return ret;
}

void FlexibleObject::SetSimState( SimState *inSimState)
{
    SimulatedObject::SetSimState(inSimState);
    
    if (mSimState==NULL)
        return;

    for (int i=0; i<mPsyst->mNbp; i++)
    {
        if (!mPsyst->IsParticleFree(i))
        {
            SimMatrixTarget* t = new SimMatrixTarget(&mSimState->GetTransform());
            t->SetOffset(ParticlePosition(i));
            mParticleTargets.NewElem().Set(i, t);
        }
    }
}

bool FlexibleObject::AddCollisionConstraint(const rmt::Vector& inDir, int inIndex)
{
    rAssert( inIndex>=0 && inIndex<mPsyst->mNbp );
    return mPsyst->mP[inIndex]->AddDirectionOfConstraint(inDir);
}

void FlexibleObject::AddSolverConstraint( const rmt::Vector& inDir, int inIndex )
{
    AddCollisionConstraint(inDir, inIndex);
}

void FlexibleObject::StartCollision(const rmt::Vector& inPos, const Vector& inDir, const SimulatedObject* inObj, int inIndex, int )
{
    rAssert( inIndex>=0 && inIndex<mPsyst->mNbp );
    Vector dp;
    dp.Sub(inPos, *(Vector*)mPsyst->mP[inIndex]->m_x);
    if( dp.DotProduct(inDir) > 0.0f )
    {
        Vector dir = inDir;
        dir.Scale(-1.0f);
        AddCollisionConstraint(dir, inIndex);
    }
    else
    {
        AddCollisionConstraint(inDir, inIndex);
    }
}

bool FlexibleObject::AddImpulse(const rmt::Vector& inPos, const rmt::Vector& inImp, const SymMatrix& inSymMatrix, SimulatedObject* , int index, int otherIndex)
{
    //Here we break the paradigm: StartCollision, AddImpulse then EndCollision 
    //in order to save the memory need by the cached impulse (one for each particles).
    rAssert( index>=0 && index<mPsyst->mNbp );
    rAssert(mPsyst->mMass[index]);
    
    FlexParticle *part = mPsyst->mP[index];
    float invMass;
    invMass = Inverse(mPsyst->ExtMassParticle(index));
    AddScaleVector( part->m_v, (float*)&inImp, invMass, part->m_v );
    
    return true;
}

void FlexibleObject::GetMassMatrix(const rmt::Vector& inPos, SymMatrix& oM, SimulatedObject* iObj, int index, int )
{
    oM.Identity();
    oM.Scale(Inverse(mPsyst->ExtMassParticle(index)));
}

void FlexibleObject::UpdateAttachedParticles(float dt)
{
    Vector pos, v;
    for (int i=0; i<mParticleTargets.GetSize(); i++)
    {
        mParticleTargets[i].mSimTarget->GetPos(pos);
        int index = mParticleTargets[i].mParticleIndex;
        if (SET_FIX_SPEED)
        {
            mPsyst->GetPosParticle(index, v.x, v.y, v.z);
            v.Sub(pos, v);
            v.Scale(1.0f/dt);
            mPsyst->SetSpeedParticle(index, v.x, v.y, v.z);
        }
        mPsyst->SetPosParticle(index, pos.x, pos.y, pos.z);
    }
}

void FlexibleObject::Update(float dt_sec)
{
    if (!mIsActivated)
        return;
    
    SimStats::StartFlexSimulation();

    rAssert(dt_sec > 0);
    
    UpdateAttachedParticles(dt_sec);
    
    mPsyst->SetDt(dt_sec);
    
    mPsyst->StartUpdate();
    mPsyst->ApplyConstraints();
    
    mPsyst->ApplyForces();
    mPsyst->AdvanceSystem();
    
    mPsyst->PreStepModification();
    mPsyst->EndUpdate();
    
    //There is no SetTransform to do as in the case of rigid bodies. 
    //But we need to inform the collision objects that its collision volumes
    //have been moved.
    static bool moveAllways=true;
    mSimState->SetHasMoved( mPsyst->GetHasMoved() || moveAllways );

    SimStats::EndFlexSimulation();
}

void FlexibleObject::DebugDisplay()
{
    static bool drawforce = false;
    static bool drawspeed = false;
    
    if (drawforce)
        mPsyst->DrawForce();
    if (drawspeed)
        mPsyst->DrawSpeed();
}


//
//
//

FlexibleObject::ParticleTarget::~ParticleTarget()
{
    if (mSimTarget)
        mSimTarget->Release();
}

void FlexibleObject::ParticleTarget::Set(int inParticleIndex, SimTarget* inSimTarget)
{
    if (mSimTarget)
        mSimTarget->Release();
    
    mSimTarget = inSimTarget;
    mParticleIndex = inParticleIndex;
    
    if (mSimTarget)
        mSimTarget->AddRef();
}

void FlexibleObject::ResolveInterpenetration( int in_index, float &io_dist, const Vector &inN, Vector &o_imp, bool addconstrain, const SimulatedObject &)
{
    rAssert( in_index>=0 && in_index<mPsyst->mNbp );
    
    FlexParticle *l_part = mPsyst->mP[in_index];
    
    if (io_dist < 0 && l_part->mFree)
    {
        static float sdist = 2.0f;
        float dd = -io_dist;
        if (sdist*dd > mPsyst->mDeli)
        {
            dd =  mPsyst->mDeli/sdist; //Clamp the moving distance to a fraction of the particles distances.
        }
        io_dist = dd;
        Vector dx;
        dx.Scale( dd, inN );
        AddVector(l_part->m_x, (float*)&dx, l_part->m_x);
        AddScaleVector(l_part->m_v, (float*)&dx, mPsyst->mInvTimeStep, l_part->m_v);
        static float minFact=1.0f, slopeFact=0.0f;//todo : Check this.
        float rest = minFact + slopeFact*dd/mPsyst->mDeli;
        float mass;
        mass = mPsyst->ExtMassParticle(in_index);
        o_imp.Scale(-rest*mPsyst->mInvTimeStep*mass, dx );
        if (addconstrain)
        {
            AddCollisionConstraint(inN, in_index);
        }
    }
}

bool FlexibleObject::UpdateConstraintTarget( const LocalTarget &in_target, Vector *o_VelocityTarget, SimTarget &io_target) const
{
    //Note: for now we don't support offset relative to the target particle.
    rAssert(in_target.GetIndex()>=0 && in_target.GetIndex()<mPsyst->mNbp);
    if ( !(in_target.GetIndex()>=0 && in_target.GetIndex()<mPsyst->mNbp) )
        return false;
    
    io_target.SetOffset(*(Vector*)mPsyst->mP[in_target.GetIndex()]->m_x);
    if (o_VelocityTarget )
    {
        *o_VelocityTarget = *(Vector*)mPsyst->mP[in_target.GetIndex()]->m_v;
    }
    return true;//Success
}

} // sim
