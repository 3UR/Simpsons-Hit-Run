
#include "simphysics/physicsjoint.hpp"
#include "simphysics/articulatedphysicsobject.hpp"

using namespace RadicalMathLibrary;

namespace sim
{

static float FRACTSIZE = 0.1f;          // defines an area around joint->fR inside wich a collision impulse is simply transmitted to the parent joint
static float PARALLEL = 0.999f;         // defines when a joint orientation is considered as parallel to the collision normal
static float MINREACT = 1.0f;           // coeff. defining the minimum action causing a reaction
static float ALMOST_ONE = 0.99f;

static bool LIMITACCEL = true;

static float MaxDefSafe = 0.5f;           // defines the max angle allowed in UpdateRotMatSafe
static float TotDefSafe = 5.0f;           // defines the max angle allowed in UpdateRotMatSafe
static float MinDefAngle = 0.0001f;       //0.015);
static float MaxTorsionAngle = 1.5f;      // watchout...0.57f;
static float MAXDEFSPEED = 100.0f;           // stabilizer: limit wnew 
static float MAXDEFSPEED2 = MAXDEFSPEED*MAXDEFSPEED;
static bool LimitJointAngle = true;
static float AngleRecoveryRate = 0.01f;

static float gammaDeformation = 0.01f / 0.016f; // equivalent to 1%/frame at 60fps
static float gammaDeformationSpeed = 0.01f / 0.016f;

static float MIN_ANGLE_ACCURACY = 0.1f; // min angle between joint that can be computed with the actual math. radian
static void TestAngleAccuracy(float &a)
{
    if (Fabs(a) < MIN_ANGLE_ACCURACY)
    {
        a = 2.0f * MIN_ANGLE_ACCURACY * Sign(a);
    }
}

static float MAXWDRDT = 0.5f;       // stabilizer: used to compute the max speed at a point

static float MaxDA = 1.0f;          // 0.01
static float MaxDA2 = 1.0f;        // 0.0001

static float ImpulseReactionRate1D = 40.0f;
static float ImpulseReactionRate3D = 2.0f;


PhysicsJoint* PhysicsJoint::CreatePhysicsJoint(int inDOF, const rmt::Vector& inCM, float inVolume, int inIndex)
{
    PhysicsJoint* newJoint = NULL;
    
    if (inDOF == 0)
        newJoint = new PhysicsJoint0D(inCM, inVolume, inIndex);
    else if (inDOF == 1)
        newJoint = new PhysicsJoint1D(inCM, inVolume, inIndex);
    else if (inDOF == 3)
        newJoint = new PhysicsJoint3D(inCM, inVolume, inIndex);
    else
        rAssert(false);
    
    return newJoint;
}

PhysicsJoint::PhysicsJoint():
    mVisible(true), 
    mNumDOF(0),
    //mIndex(inIndex),
    mParentIndex(-1),
    mMinAngle(0),
    mMaxAngle(0),
    mParentObject(NULL), 
    mParentSimJoint(NULL),
    //mLocalVolume(inVolume),
    //mVolume(inVolume),
    //mMass(inVolume),
    //mDCMPosition(inCM),
    //mDCMPositioni(inCM),
    mInvStiffness(0),
    mAbsorptionFactor(0),
    mOverFlowCache(false),
    mCacheCompleted(true),
    mMaxSpeedReached(false),
    mSimBranch(false)
{
    CLASSTRACKER_CREATE( PhysicsJoint );
}

PhysicsJoint::PhysicsJoint(const rmt::Vector& inCM, float inVolume, int inIndex): 
    mVisible(true), 
    mNumDOF(0),
    mIndex(inIndex),
    mParentIndex(-1),
    mMinAngle(0),
    mMaxAngle(0),
    mParentObject(NULL), 
    mParentSimJoint(NULL),
    mLocalVolume(inVolume),
    mVolume(inVolume),
    mMass(inVolume),
    mDCMPosition(inCM),
    mDCMPositioni(inCM),
    mInvStiffness(0),
    mAbsorptionFactor(0),
    mOverFlowCache(false),
    mCacheCompleted(true),
    mMaxSpeedReached(false),
    mSimBranch(false)
{
    CLASSTRACKER_CREATE( PhysicsJoint );
    if (mVolume > 0)
        mInvMass = rmt::Inverse(mMass); // default density = 1
    else
        mInvMass = 0;

    mNormDCMPosition = mDCMPositioni.Magnitude();
    mTransform.Identity();
    mInertiaMatrix0.Zero();
    mLocalInertiaMatrix0.Zero();
    mLocalMass = mMass;
    mML2 = 0;
    mDeformationDissipationMethod = LinearDeformationDissipation;

}

PhysicsJoint::~PhysicsJoint()
{
    CLASSTRACKER_DESTROY( PhysicsJoint );
}

void PhysicsJoint::SetInertiaMatrix(const SymMatrix& inInertia)
{
    mInertiaMatrix0 = mLocalInertiaMatrix0 = inInertia;
    mML2 = (mInertiaMatrix0.xx + mInertiaMatrix0.yy + mInertiaMatrix0.zz) * 0.33f;
}

void PhysicsJoint::SetConstraintAngles(float inMinAngle, float inMaxAngle)
{
    rAssert(inMinAngle <= PI);
    mMinAngle = inMinAngle;
    rAssert(inMinAngle <= PI);
    mMaxAngle = inMaxAngle;
}

void PhysicsJoint::SetParentObject(ArticulatedPhysicsObject* inParent)
{
    rAssert(inParent);
    mParentObject = inParent;
    UpdateForPhysicsProperties();
}

void PhysicsJoint::UpdateForPhysicsProperties()
{
    if (mMass > 0)
    {
        float scale = mLocalMass;
        mLocalMass = mLocalVolume * mParentObject->Density();
        scale = mLocalMass/scale;
        mMass *= scale;
        mInvMass = rmt::Inverse(mMass);
        mML2 *= scale;
        mInertiaMatrix0.Scale(scale);
        mLocalInertiaMatrix0.Scale(scale);
    }
}

void PhysicsJoint::SetTransform(const Matrix& m)
{
    // could extract the scale and adjust the volume/mass right away
    mTransform = m;
    mTransform.RotateVector(mDCMPositioni, &mDCMPosition);
    mTransform.RotateVector(mParentObject->JointInfo(mIndex).mAxis, &mAxis);
}

void PhysicsJoint::SetClone(PhysicsJoint* clone)
{
    clone->mParentSimJoint = NULL;
    clone->mParentObject = NULL;
    clone->mChildrenJoints.RemoveAll();
}

void PhysicsJoint::ComputeAbsorptionFactor(const Vector& pos, const Vector& dir)
{
    mAbsorptionFactor = 0;
    mMaxSpeedReached = false;
    mOverFlowCache = false;
    
    if (this != mParentObject->SelfCollisionCenter())
    {
        if (mInvStiffness > 0)
        {
            Vector dr;
            dr.Sub(pos, Origin()); // vector from the join origin to the contact point
            
            // keep part of dr able to apply a torque
            PerpAxisProjection(dr, dr);
            float ndr = dr.Magnitude();
            float jointSize = mParentObject->JointInfo(mIndex).mLength;

            if (ndr > FRACTSIZE * jointSize)
            {
                // given the normalized test impulse nimp, compute the joint absorption
                dr.Scale(Inverse(ndr));
                
                if (mParentSimJoint != NULL && mParentSimJoint == mParentObject->SelfCollisionCenter())
                {
                    mAbsorptionFactor = 1.0f;
                }
                else if (Fabs(dr.DotProduct(dir)) >= PARALLEL)
                {
                    mAbsorptionFactor = 0;
                }
                else
                {
                    mAbsorptionFactor = mInvStiffness;
                }
            }
        }
        if (mParentSimJoint)
            mParentSimJoint->ComputeAbsorptionFactor(pos, dir);
    }
    mCacheCompleted = (mAbsorptionFactor == 0);
}

void PhysicsJoint::GetMassMatrix(const rmt::Vector& inPos, SymMatrix& oM)
{
    if (this != mParentObject->SelfCollisionCenter())
    {
        // GetSelfCollisionCenter return NULL or the center in case of self-collision
        // if the is a self collision, we want to stop there.
        if (mParentSimJoint != NULL)
        {
            mParentSimJoint->GetMassMatrix(inPos, oM);
        }
        
        if (mAbsorptionFactor > RELA_SMALL && mVisible) // add the local contribution to the mass matrix
        {
            oM.Scale(1.0f-mAbsorptionFactor);
            float tmp = mAbsorptionFactor * ML2();
            oM.xx += tmp;
            oM.yy += tmp;
            oM.zz += tmp;
        }
    }
}

bool PhysicsJoint::AddImpulse(const rmt::Vector& inPos, rmt::Vector& inImp, const SymMatrix& inSymMatrix)
{
    if (this == mParentObject->SelfCollisionCenter())
    {
        return true; // in the case of a self collision, the collision center should never be affected
    }
    
    // mAbsorptionFactor represents the amount of impulse the joint will absorb. Several test are done on
    // the joint angle and speed to make sure we don't introduce instability in the joint. If any
    // of these test fails, mAbsorptionFactor is readjusted and we return false. The mass matrix will
    // be recomputed and a new impulse will be applied to the contact point.
    if (mAbsorptionFactor > 0 && !mCacheCompleted && mVisible)
    {
        Vector absimp, dr, vec;
        dr.Sub(inPos, Origin()); // vector from the join origin to the contact point
        
        // keep part of dr able to apply a torque
        PerpAxisProjection(dr, dr);
        float ndr = dr.Magnitude();

        Vector dveq, dvtest;
        inSymMatrix.TransformVector(inImp, dveq); //
        dveq.Scale(mAbsorptionFactor);
        
        VelocityProjection(dr, ndr, dveq); // dv we are looking for
        float ndveq = dveq.Magnitude();

        // maxv defines the max speed the contact point can get. To this dv will correspond
        // a larger dv at the end of the joint (of its subjoint), we compensate for that
        
        float lmax = mParentObject->JointInfo(mIndex).mLength;
        //float lmax = mNormDCMPosition * 2.0f; //**********use to be fLMax**********
        float maxv = Max(lmax, ndr) * MAXWDRDT / mParentObject->TimeStep();
        
        if (ndveq > maxv) // make sure we don't reach an crazy speed (compare to the distance dr)
        {
            mAbsorptionFactor = 0.99f * mAbsorptionFactor * maxv / ndveq;
            mMaxSpeedReached = true;
            return false;
        }
        
        float impDot = inImp.DotProduct(dveq)/ndveq;
        if (impDot > 0 && ndveq > PhysicsObject::SmallSpeed()) // the joint can absorb some of the imp
        {
            // compute how much of imp the joint will absorb, this absimp must be parallel to dveq
            //ComputeAbsorbedImpulse(inImp, absimp);
            absimp.Scale(impDot * mAbsorptionFactor / ndveq, dveq);

            // dv = dw cross dr = imp * invInertia ==> dw = imp * invInertia * inv(dr)
            // since dw and dr are ortho, we can say dw = dr cross dv / Sqr(ndr)
            Vector cache;
            cache.CrossProduct(dr, dveq); // dw test 
            cache.Scale(Inverse(Sqr(ndr)));
            
            float ratio = TryAddDeformationSpeed(cache);
            if (ratio < ALMOST_ONE) // stop at the max angle
            {
                mAbsorptionFactor = mAbsorptionFactor * ratio;
                if (ratio == 0)
                {
                    // cache is probably too big to be absorbed by the joint
                    TryFixOverFlow(cache);
                }
                return false;
            }
            inImp.Sub(absimp);
            SetCache(cache);
        }
        else
        {
            mAbsorptionFactor = 0;
            return false;
        }
        mCacheCompleted = true;
    }
    else if (mAbsorptionFactor > 0)
    {
        inImp.Scale(1.0f - mAbsorptionFactor); // the remaining imp
    }
    
    // propagate the remaining impulse, if any
    if (Fabs(inImp.x)+Fabs(inImp.y)+Fabs(inImp.z) > VERY_SMALL)
    {
        if (mParentSimJoint != NULL)
            return mParentSimJoint->AddImpulse(inPos, inImp, inSymMatrix);
        else
            return true;
    }
    else
    {
        // make sure we reset teh cache
        if (mParentSimJoint != NULL)
            mParentSimJoint->ResetCache();
        else
            mParentObject->ResetCache();
        return true;
    }
}

// the IncludeCache and IncludeCacheReaction methods allow to include the added impulse in the 
// local joint state until next step where angular and linear velocities will be recomputed. This is needed 
// to be able to use GetLocalSpeed after some impulses have been added.
void PhysicsJoint::StartIncludeCache()
{
    if (mChildrenJoints.GetSize() > 0)
    {
        Vector w, v(0, 0, 0);
        
        if ((mAbsorptionFactor > 0 || mOverFlowCache) && mVisible)
        {
            GetCache(w);
            AddCacheToDeformationSpeed();
            mAbsorptionFactor = 0;
            mParentObject->GetJointAngularVelocity(mIndex).Add(w);
        }
        else
        {
            // will get here during a self-Collision
            w.Set(0, 0, 0);
            ResetCache();
        }
        for (int i=0; i<mChildrenJoints.GetSize(); i++)
            mChildrenJoints[i]->IncludeCache(v, w);
    }
}

void PhysicsJoint::IncludeCache(Vector v, Vector w)
{
    Vector pos;

    if (mParentSimJoint != NULL)
        pos = mParentSimJoint->Origin();
    else
        mParentObject->CMPosition(pos);

    Vector dv, dr, cache;
    dr.Sub(Origin(), pos);
    dv.CrossProduct(w, dr);
    v.Add(dv);
    mParentObject->GetJointLinearVelocity(mIndex).Add(v); // new velocity
    
    if ((mAbsorptionFactor > 0 || mOverFlowCache) && mVisible)
    {
        GetCache(cache);
        AddCacheToDeformationSpeed();
        
        w.Add(cache); // add to actual angular velocity
        mAbsorptionFactor = 0;
    }
    mParentObject->GetJointAngularVelocity(mIndex).Add(w);
    
    if (mChildrenJoints.GetSize() > 0)
    {
        for (int i=0; i<mChildrenJoints.GetSize(); i++)
            mChildrenJoints[i]->IncludeCache(v, w);
    }
    ResetCache();
}

void PhysicsJoint::IncludeCacheReaction(Vector v, Vector w, bool collancestor)
{
    Vector pos;

    if (mParentSimJoint != NULL)
        pos = mParentSimJoint->Origin();
    else
        mParentObject->CMPosition(pos);

    Vector dv, dr;
    dr.Sub(Origin(), pos);
    dv.CrossProduct(w, dr);
    v.Add(dv);
    mParentObject->GetJointLinearVelocity(mIndex).Add(v);
    
    // collancestor true means that the joint parent is ancestor of the colliding joint
    // so we need to test for (this). Else, if the joint parent is not ancestor, (this) isn't either
    if (collancestor == true)
    {
        collancestor = IsParentOf(mParentObject->CollidingJoint());
    }
    
    if (mVisible)
    {
        if (!mOverFlowCache && !collancestor && GetNumDOF() > 0 && this != mParentObject->CollidingJoint())
        {
            Vector dw;
            dv.CrossProduct(w, mDCMPosition);
            dv.Add(v); // dv of the cm
            if (dv.DotProduct(dv) > Sqr(MINREACT * mParentObject->TimeStep())) 
            {
                if (ImpulseReaction(dv, dw))
                {
                    w.Add(dw);
                    AddToDeformationSpeed(dw);
                }
            }
        }
        if (mAbsorptionFactor > 0 || mOverFlowCache)
        {
            Vector cache;
            GetCache(cache);
            AddCacheToDeformationSpeed();
        
            w.Add(cache); // add to actual angular velocity
            mAbsorptionFactor = 0;
        }
    }
    mParentObject->GetJointAngularVelocity(mIndex).Add(w);
    
    if (mChildrenJoints.GetSize() > 0)
    {
        for (int i=0; i<mChildrenJoints.GetSize(); i++)
            mChildrenJoints[i]->IncludeCacheReaction(v, w, collancestor);
    }
    ResetCache();
}

bool PhysicsJoint::IsParentOf(PhysicsJoint* inJoint)
{
    bool ret = false;

    if (mIndex < inJoint->mIndex)
    {
        PhysicsJoint* tmp = inJoint;
        while(tmp && tmp->mIndex > mIndex)
        {
            tmp = tmp->mParentSimJoint;
            if (tmp && tmp->mIndex == mIndex)
            {
                ret = true;
                break;
            }
        }
    }
    return ret;
}

void PhysicsJoint::JointAngle(bool parentRelative, float& angle)
{
    if (!parentRelative)
    {
        angle = Deformation();
    }
    else
    {
        Vector parentAxis;
        mParentObject->JointAxis(mParentIndex, parentAxis, false);
        angle = mAxis.DotProduct(parentAxis);
        angle = ACosSafe(angle);
        
        if (Fabs(angle) < MIN_ANGLE_ACCURACY)
            angle = 0;
    }
}

float PhysicsJoint::TryAddValue(float value, float addvalue, float minvalue, float maxvalue)
{
    float scaleAddvalue = 1.0f;

    if (Fabs(addvalue) > 0)
    {
        float totvalue = value + addvalue;

        if (totvalue <= maxvalue && totvalue >= minvalue)
        {
            scaleAddvalue = 1.0f;
        }
        else if (value >= maxvalue && addvalue > 0)
        {
            scaleAddvalue = 0;
        }
        else if (value <= minvalue && addvalue < 0)
        {
            scaleAddvalue = 0;
        }
        else if (addvalue > 0)
        {
            scaleAddvalue = (maxvalue - value) / addvalue;
            rAssert(scaleAddvalue > 0);
            if (scaleAddvalue > 1.0f)
                scaleAddvalue = 1.0f;
        }
        else
        {
            scaleAddvalue = (minvalue - value) / addvalue;
            rAssert(scaleAddvalue > 0);
            if (scaleAddvalue > 1.0f)
                scaleAddvalue = 1.0f;
        }
    }
    return scaleAddvalue;
}

const rmt::Vector& PhysicsJoint::OriginSpeed() const 
{ 
    return mParentObject->GetJointLinearVelocity(mIndex); 
}

//
//
//

PhysicsJoint0D::PhysicsJoint0D(const rmt::Vector& inCM, float inVolume, int inIndex)
: PhysicsJoint(inCM, inVolume, inIndex)
{
}

PhysicsJoint0D::~PhysicsJoint0D()
{
}

PhysicsJoint* PhysicsJoint0D::Clone()
{
    PhysicsJoint0D* clone = new PhysicsJoint0D;
    *clone = *this;
    SetClone(clone);
    return clone;
}

void PhysicsJoint::UpdateMatrix(rmt::Matrix& inMatrix)
{
    if (!DeformationEmpty())
    {
        Vector def;
        Deformation(def);
        UpdateRotMatrixSafe(def, inMatrix, MaxDefSafe);
        SetTransform(inMatrix);
    }
}

//
//
//

PhysicsJoint1D::PhysicsJoint1D(const rmt::Vector& inCM, float inVolume, int inIndex)
: PhysicsJoint(inCM, inVolume, inIndex),
mParentRelative(false),
mDeformationSpeedCache(0),
mCurrentAngle(0)
{
    mNumDOF = 1;
    ResetDeformation();
    mRotAxis.Clear();
}

PhysicsJoint1D::~PhysicsJoint1D()
{
}

PhysicsJoint* PhysicsJoint1D::Clone()
{
    PhysicsJoint1D* clone = new PhysicsJoint1D;
    *clone = *this;
    SetClone(clone);
    return clone;
}

void PhysicsJoint1D::SetConstraintAngles(float inMinAngle, float inMaxAngle)
{
    PhysicsJoint::SetConstraintAngles(inMinAngle, inMaxAngle);
    TestAngleAccuracy(inMinAngle);
    
    // stupid hack for now... will do something only at the first call
    if (mMaxAngle == mMinAngle && mMaxAngle > 0)
    {
        mParentRelative = false;
        mMinAngle = -mMaxAngle;
    }
    else
    {
        mParentRelative = true;
    }
}

void PhysicsJoint1D::SetTransform(const Matrix& m)
{
    PhysicsJoint::SetTransform(m);
    mParentObject->JointTransform(mParentIndex).RotateVector(mParentObject->JointInfo(mIndex).mRotAxis, &mRotAxis);
    ComputeJointAngle();
    TestJointAngle();
}

void PhysicsJoint1D::UpdateDeformation(float deformationRate, float deformationSpeedRate, float dt)
{
    if (mDeformationSpeed == 0 && mDeformation == 0)
        return;
    
    mDeformationSpeed += (mDeformationSpeed0 - mDeformationSpeed) * gammaDeformationSpeed * dt;  
    mDeformation      += (mDeformation0      - mDeformation)      * gammaDeformation * dt;       
    
    mDeformationSpeed *= deformationSpeedRate;
    
    switch (mDeformationDissipationMethod)
    {
        case LinearDeformationDissipation:
            {
                float da = (1.0f - deformationRate)*Sign(mDeformation)*MaxDA;
                if (Fabs(da) >= Fabs(mDeformation))
                    mDeformation = 0;
                else
                    mDeformation -= da;
            }
            break;
        case AmortizedDeformationDissipation:
            {
                if (MaxDA > (1.0f-deformationRate) * Fabs(mDeformation))
                {
                    mDeformation *= deformationRate;
                }
                else
                {
                    float r = 1.0f - MaxDA / Fabs(mDeformation);
                    mDeformation *= r;
                }
            }
            break;
    }

    // increment deformation with the speed
    mDeformation += dt * mDeformationSpeed;

    if (mDeformation > PI)
    {
        while (mDeformation > PI)
            mDeformation -= PI;
    }
    else if (mDeformation < PI)
    {
        while (mDeformation < -PI)
            mDeformation += PI;
    }
    
    // test for deformation becoming too small or too large
    if (Fabs(mDeformation) > MinDefAngle)
    {
        if (Fabs(mDeformation) > TotDefSafe)
        {
            mDeformation = Sign(mDeformation) * TotDefSafe;
        }
        if (mParentRelative)
        {
            float da = mMaxAngle - mMinAngle;
            if (mDeformation > da || mDeformation < -da)
            {
                mDeformation *= (1.0f-AngleRecoveryRate);
            }
            else if (mDeformation > mMaxAngle || mDeformation < -mMinAngle)
            {
                mDeformation *= (1.0f-AngleRecoveryRate);
            }
        }
        else if (mDeformation > mMaxAngle || mDeformation < -mMinAngle)
        {
            mDeformation *= (1.0f-AngleRecoveryRate);
        }
    }
    else
    {
        ResetDeformation();
    }
    
    mDeformation0 = mDeformation;
    mDeformationSpeed0 = mDeformationSpeed;
}

void PhysicsJoint1D::UpdateInertialDeformation(float speedRate, float accelRate, float gravityRate, float centrifugalRate, const rmt::Vector& inSpeed, const rmt::Vector& inDSpeed, float dt)
{
    Vector b;

    b.CrossProduct(mRotAxis, mDCMPosition);
    float bn = b.Magnitude();

    float fact  = -speedRate * b.DotProduct(inSpeed);
    float fact2 = -accelRate * b.DotProduct(inDSpeed);
    float fact3 = gravityRate * b.DotProduct(mParentObject->Gravity());

    if (centrifugalRate != 0)
    {
        float w2 = mParentObject->GetJointAngularVelocity(0).MagnitudeSqr();

        if (w2 > RELA_SMALL)
        {
            // v = w cross r. since w is ortho to dr, 
            // we can approx dr = (v cross w).Scale(1 / (w dot w))
            // then centrifugal force f = dr.Scale(w dot w)
            Vector dr;
            dr.CrossProduct(inSpeed, mParentObject->GetJointAngularVelocity(0));

            float fact4 = centrifugalRate * b.DotProduct(dr);
            fact3 += fact4;
        }
    }

    fact = (fact + fact2 + fact3) * mInvStiffness * dt / bn;
    mDeformationSpeed += (fact * TryAddDeformationSpeed(fact));
}

// since the cm's inertia try to keep the cm at constant speed when the parent is 
// accelerating, we try to add a new w to compensate for the that dvcm
bool PhysicsJoint1D::ImpulseReaction(const Vector& dvcm, Vector& wabs)
{
    bool ret = false;
    Vector b;

    b.CrossProduct(mRotAxis, mDCMPosition);
    float bn = b.Magnitude();

    float fact  = -ImpulseReactionRate1D * b.DotProduct(dvcm) * mInvStiffness * bn;

    if (Fabs(fact) > 0)
    {
        fact *= TryAddDeformationSpeed(fact);
        {
            wabs.Scale(fact, mRotAxis);
            ret = true;
        }
    }
    return ret;
}

void PhysicsJoint1D::TestJointAngle()
{
    if (Fabs(mDeformationSpeed) > MAXDEFSPEED)
    {
        mDeformationSpeed = MAXDEFSPEED * Sign(mDeformationSpeed);
    }
    if (LimitJointAngle)
    {
        static float bounce = -0.1f;

        if (mCurrentAngle > mMaxAngle)
        {
            mDeformation -= (mCurrentAngle - mMaxAngle)*AngleRecoveryRate;
            mCurrentAngle = mMaxAngle;

            if (mDeformationSpeed > 0)
                mDeformationSpeed *= bounce;
        }
        else if (mCurrentAngle < mMinAngle)
        {
            mDeformation -= (mCurrentAngle - mMinAngle)*AngleRecoveryRate;
            mCurrentAngle = mMinAngle;

            if (mDeformationSpeed < 0)
                mDeformationSpeed *= bounce;
        }
    }
}

void PhysicsJoint1D::ComputeAbsorbedImpulse(const rmt::Vector& impulse, rmt::Vector& absImpulse)
{
    PerpAxisProjection(impulse, absImpulse);
    absImpulse.Scale(mAbsorptionFactor);
}

void PhysicsJoint1D::VelocityProjection(const rmt::Vector& dr, float ndr, rmt::Vector& dv) const
{
    Vector axis, vec;
    GetRotAxis(axis);
    
    // since v = w cross r and w is parallel to axis for 1 dof joint
    vec.CrossProduct(axis, dr);
    vec.Scale(Inverse(ndr)); // should be unit vector parallel to desired velocity
    rAssert(Fabs(vec.MagnitudeSqr()-1.0f) < 0.02f);

    dv.Scale(vec.DotProduct(dv), vec); // projects dv on vec
}

float PhysicsJoint1D::TryAddDeformationSpeed(const Vector& w)
{
    float addSpeed = w.DotProduct(mRotAxis);
    
    if (addSpeed == 0)
        return 1.0f;
    else
        return TryAddDeformationSpeed(addSpeed);
}

float PhysicsJoint1D::TryAddDeformationSpeed(float addSpeed)
{
    float totSpeed = mDeformationSpeed + addSpeed;
    float scaleAddSpeed = 1.0f;    
    float scaleAddSpeed2 = 1.0f;

    // potential problem:
    //   ComputeJointAngle(???);
    
    // want to maintain total deformation speed below MAXDEFSPEED
    // and wat to maintain angle between minangle and maxangle

    if (addSpeed != 0 && totSpeed != 0)
    {
        // want to maintain mMinAngle <= mCurrentAngle <= mMaxAngle
        float angle = mDeformationSpeed * mParentObject->TimeStep() + mCurrentAngle;
        float addangle = addSpeed * mParentObject->TimeStep();
        scaleAddSpeed = TryAddValue(angle, addangle, mMinAngle, mMaxAngle);
        if (scaleAddSpeed > 0)
            scaleAddSpeed2 = TryAddValue(mDeformationSpeed, addSpeed, -MAXDEFSPEED, MAXDEFSPEED);
    }
    
    return Min(scaleAddSpeed, scaleAddSpeed2);
}

void PhysicsJoint1D::TryFixOverFlow(const Vector& w)
{
    float addSpeed = w.DotProduct(mRotAxis);
    
    if (addSpeed * mDeformationSpeed < 0)
    {
        mOverFlowCache = true;
        mDeformationSpeedCache = -mDeformationSpeed;
    }
}

void PhysicsJoint1D::ComputeJointAngle()
{
    if (mParentRelative)
    {
        JointAngle(mParentRelative, mCurrentAngle);

        // get the sign of the angle using the Axis of rotation
        Vector v, parentAxis;
        mParentObject->JointAxis(mParentIndex, parentAxis, false);
        v.CrossProduct(parentAxis, mAxis);
        if (mRotAxis.DotProduct(v) <= 0)
            mCurrentAngle = -mCurrentAngle;
    }
    else
    {
        mCurrentAngle = Deformation();
    }
}

//
//
//

PhysicsJoint3D::PhysicsJoint3D(const rmt::Vector& inCM, float inVolume, int inIndex)
: PhysicsJoint(inCM, inVolume, inIndex),
mParentRelative(false),
mCurrentAngle(0),
mCurrentTorsionAngle(0)
{
    mNumDOF = 3;
    ResetDeformation();
    mCurrentRotAxis.Clear();
    mDeformationSpeedCache.Clear();
}

PhysicsJoint3D::~PhysicsJoint3D()
{
}

PhysicsJoint* PhysicsJoint3D::Clone()
{
    PhysicsJoint3D* clone = new PhysicsJoint3D;
    *clone = *this;
    SetClone(clone);
    return clone;
}

void PhysicsJoint3D::SetConstraintAngles(float inMinAngle, float inMaxAngle)
{
    PhysicsJoint::SetConstraintAngles(inMinAngle, inMaxAngle);
    TestAngleAccuracy(inMinAngle);
    TestAngleAccuracy(inMaxAngle);
    
    // stupid hack for now... will do something only at the first call
    if (mMaxAngle == mMinAngle && mMaxAngle > 0)
    {
        mParentRelative = false;
    }
    else
    {
        mParentRelative = true;
    }
    // no direct support of mMinAngle... cause lack of reference
    mMinAngle = -mMaxAngle;
}

void PhysicsJoint3D::SetTransform(const Matrix& m)
{
    PhysicsJoint::SetTransform(m);
    Vector v, parentAxis;
    mParentObject->JointAxis(mParentIndex, parentAxis, false);
    mCurrentRotAxis.CrossProduct(parentAxis, mAxis);
    mCurrentRotAxis.NormalizeSafe();
    
    static Vector refPerp0(0, 1.0f, 0);
    Vector ref;
    mParentObject->JointTransform(mParentIndex).RotateVector(refPerp0, &ref);
    if (ref.DotProduct(mCurrentRotAxis) < 0) // forget about the isolate case == 0
        mCurrentRotAxis.Scale(-1.0f);
    
    ComputeJointAngle();
    TestJointAngle();
}

void PhysicsJoint3D::UpdateDeformation(float deformationRate, float deformationSpeedRate, float dt)
{
    if (mDeformationSpeedEmpty == true && mDeformationEmpty == true)
        return;
    
    Vector d;
    d.Sub(mDeformationSpeed0, mDeformationSpeed);
    mDeformationSpeed.ScaleAdd(mDeformationSpeed, gammaDeformationSpeed*dt, d);
    d.Sub(mDeformation0, mDeformation);
    mDeformation.ScaleAdd(mDeformation, gammaDeformation*dt, d);
    
    mDeformationSpeed.Scale(deformationSpeedRate);
    
    switch (mDeformationDissipationMethod)
    {
        case LinearDeformationDissipation:
            {
                float da = (1.0f - deformationRate)*MaxDA;
                float r = mDeformation.DotProduct(mDeformation);
                if (Sqr(da) >= r)
                {
                    mDeformation.Clear();
                }
                else
                {
                    mDeformation.ScaleAdd(-da/Sqrt(r), mDeformation);
                }
            }
            break;
        case AmortizedDeformationDissipation:
            {
                float r = mDeformation.DotProduct(mDeformation);
                if (MaxDA2 > Sqr(1.0f-deformationRate) * r)
                {
                    mDeformation.Scale(deformationRate);
                }
                else
                {
                    r = 1.0f-Sqrt(MaxDA2 / r);
                    mDeformation.Scale(r);
                }
            }
            break;
    }

    mDeformation.ScaleAdd(dt, mDeformationSpeed);
    
    float dot = mDeformation.DotProduct(mDeformation);
    
    if (dot > Sqr(MinDefAngle))
    {
        if (dot > Sqr(TotDefSafe)) // cap it
        {
            mDeformation.Scale(TotDefSafe*Inverse(Sqrt(dot)));
        }
        else if (dot > Sqr(mMaxAngle - mMinAngle))
        {
            mDeformation.Scale(1.0f-AngleRecoveryRate);
        }
        mDeformationEmpty = false;
    }
    else
    {
        ResetDeformation();
    }
    
    mDeformation0 = mDeformation;
    mDeformationSpeed0 = mDeformationSpeed;
}

void PhysicsJoint3D::UpdateInertialDeformation(float speedRate, float accelRate, float gravityRate, float centrifugalRate, const rmt::Vector& inSpeed, const rmt::Vector& inDSpeed, float dt)
{
    Vector axis;
    mParentObject->JointAxis(mIndex, axis, false);

    Vector fact;
    fact.CrossProduct(axis, inSpeed);
    fact.Scale(-speedRate);

    Vector fact2;
    fact2.CrossProduct(axis, inDSpeed);
    fact.ScaleAdd(-accelRate, fact2);

    fact2.CrossProduct(axis, mParentObject->Gravity());
    fact.ScaleAdd(gravityRate, fact2);

    if (centrifugalRate != 0)
    {
        float w2 = mParentObject->GetJointAngularVelocity(0).MagnitudeSqr();

        if (w2 > RELA_SMALL)
        {
            // v = w cross r. since w is ortho to dr, 
            // we can approx dr = (v cross w).Scale(1 / (w dot w))
            // then centrifugal force f = dr.Scale(w dot w)
            Vector dr;
            dr.CrossProduct(inSpeed, mParentObject->GetJointAngularVelocity(0));
            fact2.CrossProduct(axis, dr);
            fact.ScaleAdd(centrifugalRate, fact2);
        }
    }

    fact.Scale(dt * mInvStiffness);

    float scale = TryAddDeformationSpeed(fact);
    if (scale > 0)
    {
        mDeformationSpeed.ScaleAdd(scale, fact);
        mDeformationSpeedEmpty = false;
    }
}

// since the cm's inertia try to keep the cm at constant speed when the parent is 
// accelerating, we try to add a new w to compensate for the that dvcm
bool PhysicsJoint3D::ImpulseReaction(const Vector& dvcm, Vector& wabs)
{
    bool ret = false;

    Vector axis;
    mParentObject->JointAxis(mIndex, axis, false);

    wabs.CrossProduct(axis, dvcm);
    wabs.Scale(-ImpulseReactionRate3D * mInvStiffness);
    float scale = TryAddDeformationSpeed(wabs);

    float fact = TryAddDeformationSpeed(wabs);
    if (fact > 0)
    {
        wabs.Scale(fact);
        ret = true;
    }
    return ret;
}

void PhysicsJoint3D::VelocityProjection(const rmt::Vector& dr, float ndr, rmt::Vector& dv) const
{
    // remove component parallel to dr
    dv.ScaleAdd(dv, -dv.DotProduct(dr)/Sqr(ndr), dr);
}

void PhysicsJoint3D::TestJointAngle()
{
    float mag2 = mDeformationSpeed.MagnitudeSqr();
    if (mag2 > MAXDEFSPEED2)
    {
        mDeformationSpeed.Scale(MAXDEFSPEED/Sqrt(mag2));
    }
    if (LimitJointAngle)
    {
        if (Fabs(mCurrentTorsionAngle) > MaxTorsionAngle)
        {
            float delta = (mCurrentTorsionAngle > 0 ? mCurrentTorsionAngle - MaxTorsionAngle : mCurrentTorsionAngle + MaxTorsionAngle);
            delta *= AngleRecoveryRate;
            mDeformation.ScaleAdd(-delta, mAxis);
            mCurrentTorsionAngle -= delta;
        }
        if (mCurrentAngle > mMaxAngle)
        {
            mDeformation.Scale(mMaxAngle/mCurrentAngle);
            mCurrentAngle = mMaxAngle;
            //float delta = (mCurrentAngle > 0 ? mCurrentAngle - mMaxAngle : mCurrentAngle + mMaxAngle);
            //delta *= AngleRecoveryRate;
            //mDeformation.ScaleAdd(-delta, mCurrentRotAxis);
            //mCurrentAngle -= delta;
        }
        else if (mCurrentAngle < mMinAngle && mMinAngle < 0)
        {
            mDeformation.Scale(mMaxAngle/mCurrentAngle);
            mCurrentAngle = mMinAngle;
        }
    }
}

float PhysicsJoint3D::TryAddDeformationSpeed(const Vector& addSpeed)
{
    Vector totSpeed = addSpeed;
    totSpeed.Add(mDeformationSpeed);
    
    float torsion = mDeformationSpeed.DotProduct(mAxis);
    float addedtorsion = addSpeed.DotProduct(mAxis);
    float scaleTorsion = 1.0f;
    float scaleTorsionw = 1.0f;

    if (addedtorsion != 0)
    {
        scaleTorsion = TryAddValue(mCurrentTorsionAngle + torsion * mParentObject->TimeStep(), addedtorsion * mParentObject->TimeStep(), -MaxTorsionAngle, MaxTorsionAngle);
        if (scaleTorsion > 0)
            scaleTorsionw = TryAddValue(torsion, addedtorsion, -MAXDEFSPEED, MAXDEFSPEED);
        else
            return 0;
        if (scaleTorsionw == 0)
            return 0;
    }

    float angle = mDeformationSpeed.DotProduct(mCurrentRotAxis);
    float addedangle = addSpeed.DotProduct(mCurrentRotAxis);
    float scaleAngle = 1.0f;
    float scaleAnglew = 1.0f;

    if (addedangle != 0)
    {
        scaleAngle = TryAddValue(mCurrentAngle + angle * mParentObject->TimeStep(), addedangle * mParentObject->TimeStep(), mMinAngle, mMaxAngle);
        if (scaleAngle != 0)
            scaleAnglew = TryAddValue(angle, addedangle, -MAXDEFSPEED, MAXDEFSPEED);
        else
            return 0;
        if (scaleAnglew == 0)
            return 0;
    }

    mCurrentSecRotAxis.CrossProduct(mCurrentRotAxis, mAxis);

    float secangle = mDeformationSpeed.DotProduct(mCurrentSecRotAxis);
    float addedsecangle = addSpeed.DotProduct(mCurrentSecRotAxis);
    float scalesecAngle = 1.0f;
    float scalesecAnglew = 1.0f;

    if (addedsecangle != 0)
    {
        scalesecAngle = TryAddValue(secangle * mParentObject->TimeStep(), addedsecangle * mParentObject->TimeStep(), mMinAngle, mMaxAngle);
        if (scalesecAngle != 0)
            scalesecAnglew = TryAddValue(secangle, addedsecangle, -MAXDEFSPEED, MAXDEFSPEED);
        else
            return 0;
        if (scalesecAnglew == 0)
            return 0;
    }
    float res = Min(scalesecAngle, scalesecAnglew);
    res = Min(res, scaleAngle);
    res = Min(res, scaleAnglew);
    res = Min(res, scaleTorsion);
    res = Min(res, scaleTorsionw);

    return res;
}

void PhysicsJoint3D::TryFixOverFlow(const Vector& addSpeed)
{
    if (addSpeed.DotProduct(mDeformationSpeed) < 0)
    {
        mOverFlowCache = true;
        mDeformationSpeedCache.x = -mDeformationSpeed.x;
        mDeformationSpeedCache.y = -mDeformationSpeed.y;
        mDeformationSpeedCache.z = -mDeformationSpeed.z;
    }
}

void PhysicsJoint3D::ComputeJointAngle()
{
    JointAngle(mParentRelative, mCurrentAngle);
    
    if (mParentRelative)
    {
        mCurrentTorsionAngle = mDeformation.DotProduct(mAxis);
        
        if (mDeformation.DotProduct(mCurrentRotAxis) < 0)
            mCurrentAngle = -mCurrentAngle;
    }
    else // no reference, don't know too much what to do...
    {
        mCurrentTorsionAngle = mDeformation.DotProduct(mAxis);
        // keep mCurrentAngle always positive for now, we let mCurrentRotAxis flip instead
    }
}

} // sim
