
#include "simflexible/flexibleobject.hpp"
#include "simcommon/simtarget.hpp"
#include "simflexible/particlesyst.hpp"
#include "simflexible/particlesystxd.hpp"
#include "simflexible/psystongeom2d.hpp"
#include "simcommon/dLine2.hpp"
#include "simcommon/simstateflexible.hpp"


using namespace RadicalMathLibrary;

bool testPreStepModification=true;//todo(mic) : Remove this.
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
   mPsyst->mSimEnvironment = mSimEnvironment;
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
   *mPsyst->mCparams = p;
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
      (dynamic_cast<SimMatrixTarget*>(mParticleTargets[0].mSimTarget))->GetPos(pos);
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

void FlexibleObject::StartCollision(const rmt::Vector& inPos, const Vector& inDir, const SimulatedObject* inObj, int index)
{
   mPsyst->mP[index]->mDpCns = inDir;
   mPsyst->mP[index]->mCollFilter |= FilterDeltaPosition;
}

#ifdef RAD_DEBUG
#include "simcollision/collisionobject.hpp"//todo(mic) : check if we need this.
#include "simcollision/collisionvolume.hpp"//todo(mic) : check if we need this.
#endif

bool FlexibleObject::AddImpulse(const rmt::Vector& inPos, const rmt::Vector& inImp, const SymMatrix& inSymMatrix, SimulatedObject* otherObj, int index)
{
   //Here we break the paradigm: StartCollision, AddImpulse then EndCollision 
   //in order to save the memory need by the cached impulse (one for each particles).
   rAssert( index>=0 && index<mPsyst->mNbp );
   rAssert(mPsyst->mMass[index]);

   FlexParticle *part = mPsyst->mP[index];
   float invMass;
   if (otherObj)
   {
      invMass = otherObj->InvMass()/mPsyst->mExtDensity;
   }
   else
   {
      invMass = 1.0f/mPsyst->ExtMassParticle(index);
   }
   AddScaleVector( part->m_v, (float*)&inImp, invMass, part->m_v );
   part->mDvCns.ScaleAdd( 1/mPsyst->ExtMassParticle(index), inImp );
   part->mCollFilter |= FilterDeltaSpeed;

   return true;
}

void FlexibleObject::GetMassMatrix(const rmt::Vector& inPos, SymMatrix& oM, SimulatedObject* iObj, int index)
{
   oM.Identity();
   if (iObj)
   {
      oM.Scale(iObj->InvMass()/mPsyst->mExtDensity);
   }
   else
   {
      oM.Scale(1/mPsyst->ExtMassParticle(index));
   }
}

void FlexibleObject::UpdateAttachedParticles(float dt)
{
   Vector pos, v;
   for (int i=0; i<mParticleTargets.GetSize(); i++)
   {
      (dynamic_cast<SimMatrixTarget*>(mParticleTargets[i].mSimTarget))->GetPos(pos);
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
   SimulatedObject::Update(dt_sec);

   if (!mIsActivated)
      return;

   rAssert(dt_sec > 0);

   UpdateAttachedParticles(dt_sec);

   mPsyst->SetDt(dt_sec);
   
   mPsyst->StartUpdate();
   mPsyst->ApplyConstraints();
   mPsyst->ApplyForces();
   mPsyst->AdvanceSystem();
   if (testPreStepModification)
   {
      mPsyst->PreStepModification();
   }
   mPsyst->EndUpdate();

   //There is no SetTransform to do as in the case of rigid bodies. 
   //But we need to inform the collision objects that its collision volumes
   //have been moved.
   mSimState->SetHasMoved(mPsyst->mHasMoved);

}

bool drawforce = false;
bool drawspeed = false;

void FlexibleObject::DebugDisplay()
{
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

void FlexibleObject::ResolveInterpenetration( int in_index, float in_dist, const Vector &inN, Vector &o_imp, bool addconstrain, const SimulatedObject &otherObj)
{
   rAssert( in_index>=0 && in_index<mPsyst->mNbp );

   FlexParticle *l_part = mPsyst->mP[in_index];

   if (in_dist < 0 && l_part->mFree)
   {
      static float sdist = 2.0f;
      float dd = -in_dist;
      if (sdist*dd > mPsyst->mDeli)
      {
         dd =  mPsyst->mDeli/sdist; //Clamp the moving distance to a fraction of the particles distances.
      }
      Vector dx;
      dx.Scale( dd, inN );
      AddVector(l_part->m_x, (float*)&dx, l_part->m_x);
      AddScaleVector(l_part->m_v, (float*)&dx, mPsyst->mInvTimeStep, l_part->m_v);
      static float minFact=1.0f, slopeFact=0.0f;//todo(mic) : Check this.
      float rest = minFact + slopeFact*dd/mPsyst->mDeli;
      float mass;
      mass = mPsyst->mExtDensity/otherObj.InvMass();
      o_imp.Scale(-rest*mPsyst->mInvTimeStep*mass, dx );

      if (addconstrain)
      {
         l_part->mDpCns = inN;
         l_part->mCollFilter |= FilterDeltaPosition;
      }
#ifdef RAD_DEBUG
      static int updateVolPosition=false;
      if (updateVolPosition)
      {
         SimState *l_simState = (SimState*)GetSimState();
         TList<CollisionVolume*>* list = \
         l_simState->GetCollisionObject()->GetCollisionVolume()->SubVolumeList();
         CollisionVolume *vol = list->GetAt(in_index);
         vol->mPosition.Add(dx);
      }
#endif
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
