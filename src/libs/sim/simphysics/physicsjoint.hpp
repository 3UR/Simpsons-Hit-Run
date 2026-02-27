
#ifndef _PHYSICSJOINT_HPP_
#define _PHYSICSJOINT_HPP_

#include <memory/classsizetracker.h>
#include "poser/posedriver.hpp"
#include "poser/pose.hpp"
#include "poser/joint.hpp"
#include "simcommon/simmath.hpp"
#include "simcommon/tlist.hpp"

namespace sim
{

class ArticulatedPhysicsObject;

static float CRAZYW = 100000.0f;

enum DeformationDissipationMethod { LinearDeformationDissipation = 0, AmortizedDeformationDissipation };

class PhysicsJoint
{
public:
    PhysicsJoint(const rmt::Vector& inCM, float inVolume, int inIndex);
    virtual ~PhysicsJoint();
    
    static PhysicsJoint* CreatePhysicsJoint(int inDOF, const rmt::Vector& inCM, float inVolume, int inIndex);
    
    virtual void ResetDeformationSpeed()               = 0;
    virtual void ResetDeformation()                    = 0;
    virtual void ResetCache()                          = 0;
    
    virtual bool DeformationEmpty()              const = 0;
    virtual float Deformation()                  const = 0;
    virtual void Deformation(rmt::Vector& v)     const = 0;
    
    int Index()                                  const { return mIndex; }
    float LocalVolume()                          const { return mLocalVolume; }
    float LocalMass()                            const { return mLocalMass; }
    const SymMatrix& LocalInertiaMatrix()        const { return mLocalInertiaMatrix0; }
    virtual float Mass()                         const { return mMass; }
    virtual float InvMass()                      const { return mInvMass; }
    
    const rmt::Vector& Origin()                  const { return mTransform.Row(3); }
    const rmt::Vector& OriginSpeed()             const;
    void CMPosition(rmt::Vector& cmPosition)     const { cmPosition.Add(Origin(), mDCMPosition); }
    float NormDCMPosition()                      const { return mNormDCMPosition; }
    
    const rmt::Vector& DCMPosition()             const { return mDCMPosition; }
    const rmt::Vector& DCMPositionLocal()        const { return mDCMPositioni; }

    bool IsVisible()                             const { return mVisible; }
    short GetNumDOF()                            const { return mNumDOF; }
    ArticulatedPhysicsObject* GetParentObject()        { return mParentObject; }
    PhysicsJoint* GetParentJoint()                     { return mParentSimJoint; }
    
    void SetParentObject(ArticulatedPhysicsObject* inParent);
    bool IsParentOf(PhysicsJoint* inJoint);
    
    void SetInvStiffness(float inInvStiffness)         { if (mNumDOF > 0) mInvStiffness = inInvStiffness; }
    float GetInvStiffness()                      const { return mInvStiffness; }
    virtual void SetConstraintAngles(float inMinAngle, float inMaxAngle);
    void SetInertiaMatrix(const SymMatrix& inInertia);

protected:
    friend class PhysicsJoint0D;
    friend class PhysicsJoint1D;
    friend class PhysicsJoint3D;
    friend class ArticulatedPhysicsObject;
    friend class PhysicsJointMatrixModifier;
    friend class PhysicsJointInertialEffector;
    
    void SetVisible(bool inVisible)                    { mVisible = inVisible; }
    virtual void SetTransform(const rmt::Matrix& m);
    virtual void UpdateDeformation(float , float , float )  = 0;
    virtual void UpdateInertialDeformation(float , float , float , float , const rmt::Vector&, const rmt::Vector&, float ) = 0;
    
    void UpdateMatrix(rmt::Matrix& inMatrix);
    
    void UpdateForPhysicsProperties();
    virtual void GetMassMatrix(const rmt::Vector& inPos, SymMatrix& oM);
    void ComputeAbsorptionFactor(const rmt::Vector& pos, const rmt::Vector& dir);
    virtual bool AddImpulse(const rmt::Vector& inPos, rmt::Vector& inImp, const SymMatrix& inSymMatrix);
    virtual void GetRotAxis(rmt::Vector& axis)                                          const = 0;
    virtual void PerpAxisProjection(const rmt::Vector& v, rmt::Vector& vp)              const = 0;
    virtual void VelocityProjection(const rmt::Vector& dr, float ndr, rmt::Vector& dv)  const = 0;

    virtual void ComputeAbsorbedImpulse(const rmt::Vector& impulse, rmt::Vector& absImpulse)  = 0;
    virtual void SetCache(const rmt::Vector& cache)                                           = 0;
    virtual float TryAddDeformationSpeed(const rmt::Vector& w)                                = 0;
    virtual void TryFixOverFlow(const rmt::Vector& w)                                         = 0;
    void JointAngle(bool parentRelative, float& angle);                                       
    virtual void GetCache(rmt::Vector& cache) const                                           = 0;
    virtual void AddCacheToDeformationSpeed()                                                 = 0;
    virtual void AddToDeformationSpeed(const rmt::Vector& w)                                  = 0;
    virtual void TestJointAngle()                                                             = 0;
    void StartIncludeCache();
    void IncludeCache(rmt::Vector v, rmt::Vector w);
    void IncludeCacheReaction(rmt::Vector v, rmt::Vector w, bool collancestor = true);
    virtual bool ImpulseReaction(const rmt::Vector& dvcm, rmt::Vector& wabs) = 0;
    float TryAddValue(float value, float addvalue, float minvalue, float maxvalue);
    
    virtual PhysicsJoint* Clone()                                                             = 0;
    virtual void SetClone(PhysicsJoint* clone);
    
    float ML2()                                  const { return mML2; }
    float InvInertia()                           const { return rmt::Inverse(mML2); }
    
    bool                       mVisible;
    short                      mNumDOF;
    int                        mIndex;
    int                        mParentIndex;
    float                      mMinAngle;
    float                      mMaxAngle;
    
    ArticulatedPhysicsObject*  mParentObject;
    PhysicsJoint*              mParentSimJoint;
    TList<PhysicsJoint*>       mChildrenJoints;
    
    // we introduce below the concept of property and local property
    // local property applies to the joint itself while property is
    // the addition of a joint local property with the local properties
    // of non-simulated sub-joints
    float                      mLocalVolume;          // local volume.
    float                      mVolume;               // volume.
    float                      mLocalMass;            // local mass
    float                      mMass;                 // mass
    float                      mInvMass;
    float                      mNormDCMPosition;      // distance between the joint origin and the cm
    SymMatrix                  mInertiaMatrix0;
    SymMatrix                  mLocalInertiaMatrix0;
    rmt::Vector                mDCMPosition;          // vector from joint origin to joint cm 
    rmt::Vector                mDCMPositioni;         // vector from joint origin to joint cm in joint space
    rmt::Vector                mAxis;
    rmt::Matrix                mTransform;
    
    float                      mInvStiffness;         // relates to muscle stiffness
    float                      mAbsorptionFactor;     // the amount of the incoming impulse absorbed by the joint
    bool                       mOverFlowCache;        // indicates a joint that can't absorb the added impulse because it is too stong
    bool                       mCacheCompleted;       // indicated that the impulse has been added successfully to the joint
    bool                       mMaxSpeedReached;
    bool                       mSimBranch;            // indicates if there is any joint with mAbsorptionFactor > 0 between this joint and the root
    float                      mML2;                  // 

    DeformationDissipationMethod mDeformationDissipationMethod;

    PhysicsJoint();                                   // for clone
};

typedef PhysicsJoint* pPhysicsJoint;

//

class PhysicsJoint0D
    : public PhysicsJoint
{
public:
    PhysicsJoint0D(const rmt::Vector& inCM, float inVolume, int inIndex);
    ~PhysicsJoint0D();
    
    void ResetDeformationSpeed()                                            {}
    void ResetCache()                                                       {}
    void ResetDeformation()                                                 {}
    bool DeformationEmpty()                                           const { return true; }
    float Deformation()                                               const { return 0; }
    void Deformation(rmt::Vector& v)                                  const { v.Clear(); }
    
protected:
    friend class ArticulatedPhysicsObject;
    
    void UpdateDeformation(float , float , float )                          {}
    void UpdateInertialDeformation(float , float , float , float , const rmt::Vector&, const rmt::Vector&, float )  {}
    void SetCache(const rmt::Vector& cache)                                 { rAssert(false); }
    void ComputeAbsorbedImpulse(const rmt::Vector& , rmt::Vector& )         { rAssert(false); }
    float TryAddDeformationSpeed(const rmt::Vector& w)                      { rAssert(false); return 0; }
    void TryFixOverFlow(const rmt::Vector& w)                               { rAssert(false); }
    void GetRotAxis(rmt::Vector& axis)                                const { rAssert(false); }
    void PerpAxisProjection(const rmt::Vector& v, rmt::Vector& vp)    const { rAssert(false); }
    void VelocityProjection(const rmt::Vector& dr, float ndr, rmt::Vector& dv) const { rAssert(false); }
    void GetCache(rmt::Vector& cache)                                 const { rAssert(false); }
    void AddCacheToDeformationSpeed()                                       { rAssert(false); }
    void AddToDeformationSpeed(const rmt::Vector& w)                        { rAssert(false); }
    void TestJointAngle()                                                   { rAssert(false); }
    bool ImpulseReaction(const rmt::Vector& dvcm, rmt::Vector& wabs)        { rAssert(false); return false; }
    
    PhysicsJoint* Clone();
    
    PhysicsJoint0D()                                                        {} // for clone
};

//
//
//

class PhysicsJoint1D
    : public PhysicsJoint
{
public:
    PhysicsJoint1D(const rmt::Vector& inCM, float inVolume, int inIndex);
    ~PhysicsJoint1D();
    
    virtual void SetConstraintAngles(float inMinAngle, float inMaxAngle);
    
    void ResetDeformationSpeed()                    { mDeformationSpeed = mDeformationSpeed0 = 0; }
    void ResetDeformation()                         { mDeformationSpeed = mDeformationSpeed0 = mDeformation = mDeformation0 = 0; }
    void ResetCache()                               { mDeformationSpeedCache = 0; }
    bool DeformationEmpty()                   const { return mDeformation == 0 ? true : false; }
    bool DeformationSpeedEmpty()              const { return mDeformationSpeed == 0 ? true : false; }
    float Deformation()                       const { return rmt::Fabs(mDeformation); }
    void Deformation(rmt::Vector& v)          const { v.Scale(mDeformation, mRotAxis); }
    
protected:
    friend class ArticulatedPhysicsObject;
    virtual PhysicsJoint* Clone();
    
    void GetRotAxis(rmt::Vector& axis)         const { axis = mRotAxis; }
    void PerpAxisProjection(const rmt::Vector& v, rmt::Vector& vp) const
    {
        vp.ScaleAdd(v, -v.DotProduct(mRotAxis), mRotAxis); // keep only component of imp in the plane perp to the axis
    }
    void VelocityProjection(const rmt::Vector& dr, float ndr, rmt::Vector& dv) const;
    void SetTransform(const rmt::Matrix& m);
    void UpdateDeformation(float deformationRate, float deformationSpeedRate, float dt);
    void UpdateInertialDeformation(float speedRate, float accelRate, float gravityRate, float centrifugalRate, const rmt::Vector& inSpeed, const rmt::Vector& inDSpeed, float dt);
    bool ImpulseReaction(const rmt::Vector& dvcm, rmt::Vector& wabs);
    void ComputeAbsorbedImpulse(const rmt::Vector& impulse, rmt::Vector& absImpulse);
    void SetCache(const rmt::Vector& cache)          { mDeformationSpeedCache = cache.DotProduct(mRotAxis); rAssert(rmt::Sqr(mDeformationSpeedCache) < CRAZYW); }
    float TryAddDeformationSpeed(const rmt::Vector& w);
    float TryAddDeformationSpeed(float w);
    void TryFixOverFlow(const rmt::Vector& w);
    void ComputeJointAngle();
    void GetCache(rmt::Vector& cache) const          { cache.Scale(mDeformationSpeedCache, mRotAxis); }
    void AddCacheToDeformationSpeed()                { mDeformationSpeed += mDeformationSpeedCache; }
    void AddToDeformationSpeed(const rmt::Vector& w) { mDeformationSpeed += w.DotProduct(mRotAxis); }
    void TestJointAngle();
    
    bool mParentRelative;
    rmt::Vector mRotAxis;
    float mDeformationSpeedCache;
    float mDeformationSpeed;
    float mDeformationSpeed0;
    float mDeformation;
    float mDeformation0;
    float mCurrentAngle;
    
    PhysicsJoint1D() {} // for clone
};

//
//
//

class PhysicsJoint3D
    : public PhysicsJoint
{
public:
    PhysicsJoint3D(const rmt::Vector& inCM, float inVolume, int inIndex);
    ~PhysicsJoint3D();
    
    void SetConstraintAngles(float inMinAngle, float inMaxAngle);
    
    void ResetDeformationSpeed()              { mDeformationSpeed.Clear(); mDeformationSpeed0.Clear(); mDeformationSpeedEmpty = true; }
    void ResetDeformation()                   { ResetDeformationSpeed(); mDeformation.Clear(); mDeformation0.Clear(); mDeformationEmpty = true; }
    void ResetCache()                         { mDeformationSpeedCache.Clear(); }
    
    bool DeformationEmpty()             const { return mDeformationEmpty; }
    bool DeformationSpeedEmpty()        const { return mDeformationSpeedEmpty; }
    float Deformation()                 const { return mDeformation.Magnitude(); }
    void Deformation(rmt::Vector& v)    const { v = mDeformation; }
    
protected:
    friend class ArticulatedPhysicsObject;
    PhysicsJoint* Clone();
    
    void GetRotAxis(rmt::Vector& axis)  const { axis = mCurrentRotAxis; }
    void PerpAxisProjection(const rmt::Vector& v, rmt::Vector& vp) const { vp = v; }
    void VelocityProjection(const rmt::Vector& dr, float ndr, rmt::Vector& dv) const;
    void SetTransform(const rmt::Matrix& m);
    void UpdateDeformation(float deformationRate, float deformationSpeedRate, float dt);
    void UpdateInertialDeformation(float speedRate, float accelRate, float gravityRate, float centrifugalRate, const rmt::Vector& inSpeed, const rmt::Vector& inDSpeed, float dt);
    bool ImpulseReaction(const rmt::Vector& dvcm, rmt::Vector& wabs);
    void SetCache(const rmt::Vector& cache)                  { mDeformationSpeedCache = cache; rAssert(cache.DotProduct(cache) < CRAZYW);}
    void ComputeAbsorbedImpulse(const rmt::Vector& impulse, rmt::Vector& absImpulse) { absImpulse.Scale(mAbsorptionFactor, impulse); }
    float TryAddDeformationSpeed(const rmt::Vector& w);
    void TryFixOverFlow(const rmt::Vector& w);
    void ComputeJointAngle();
    void GetCache(rmt::Vector& cache)                  const { cache = mDeformationSpeedCache; }
    void AddCacheToDeformationSpeed()                        { mDeformationSpeed.Add(mDeformationSpeedCache); mDeformationSpeedEmpty = false; }
    void AddToDeformationSpeed(const rmt::Vector& w)         { mDeformationSpeed.Add(w); mDeformationSpeedEmpty = false; }
    void SubTryAddDeformationSpeed(const rmt::Vector& w, float B, float C, float maxw, float& f);
    void TestJointAngle();
    
    bool mParentRelative;
    rmt::Vector mCurrentRotAxis;
    rmt::Vector mCurrentSecRotAxis;
    rmt::Vector mDeformationSpeedCache;
    rmt::Vector mDeformationSpeed;
    rmt::Vector mDeformationSpeed0;
    rmt::Vector mDeformation;
    rmt::Vector mDeformation0;
    bool mDeformationEmpty;
    bool mDeformationSpeedEmpty;
    float mCurrentAngle;
    float mCurrentTorsionAngle;
    
    PhysicsJoint3D() {} // for clone
};

//
//
//

class PhysicsJointMatrixModifier
    : public poser::PoseDriver
{
public:
    PhysicsJointMatrixModifier(PhysicsJoint* inJoint)
        : mPhysicsJoint(inJoint)
    {
    } 
    
    virtual int GetMinimumJointIndex()  const { return mPhysicsJoint->Index(); }
    virtual int GetPriority()           const { return poser::PhysicsJointPriority; }
    
    virtual void Advance(float ) {}
    
    void Update(poser::Pose* pose)
    {
        if (IsEnabled())
        {
            poser::Joint*     j = pose->GetJoint(mPhysicsJoint->Index());
            poser::Transform  pt = j->GetWorldTransform();
            rmt::Matrix m =   pt.GetMatrix();
            mPhysicsJoint->UpdateMatrix(m);
            pt.SetMatrix(m);
            j->SetWorldTransform(pt);
        }
    }
    
protected:
    PhysicsJoint* mPhysicsJoint;
};

//
//
//

class PhysicsJointInertialEffector
    : public poser::PoseDriver
{
public:
    PhysicsJointInertialEffector(PhysicsJoint* inJoint)
        : mPhysicsJoint(inJoint), 
          mPreviousSpeed(0.0f, 0.0f, 0.0f), 
          mDt(0.0f),
          mInertialSpeedRate(5.0f),
          mInertialAccelRate(20.0f),
          mGravityRate(4.0f),
          mCentrifugalRate(0.0f)
    {
    } 
    
    virtual int GetMinimumJointIndex()  const { return mPhysicsJoint->Index(); }
    virtual int GetPriority()           const { return poser::PhysicsInertialJointPriority; }
    
    virtual void SetIsEnabled(bool isEnabled) 
    { 
        PoseDriver::SetIsEnabled(isEnabled); 
        if (isEnabled)
            mPreviousSpeed = mPhysicsJoint->OriginSpeed();
    }
    virtual void Advance(float inDt) { mDt = inDt; }
    
    void Update(poser::Pose* pose)
    {
        if (IsEnabled() && mPhysicsJoint->GetInvStiffness())
        {
            rmt::Vector s = mPhysicsJoint->OriginSpeed();
            mPreviousSpeed.Sub(s, mPreviousSpeed); // used temp as dv
            mPhysicsJoint->UpdateInertialDeformation(mInertialSpeedRate, mInertialAccelRate, mGravityRate, mCentrifugalRate, s, mPreviousSpeed, mDt);
            mPreviousSpeed = s;
        }
    }

    // the rule for thoses rates: 0 for no effect, the larger the value the bigger the effect.

    // rules how speed affects deformation, simulates air friction
    void SetSpeedRate(float inRate)         { mInertialSpeedRate = inRate; }

    // rules how acceleration affects deformation
    void SetAccelRate(float inRate)         { mInertialAccelRate = inRate; }
    
    // rules how gravity affects deformation
    void SetGravityRate(float inRate)       { mGravityRate = inRate; }

    // rules how centrifugal forces affects deformation
    void SetCentrifugalRate(float inRate)   { mCentrifugalRate = inRate; }

private:
    PhysicsJoint* mPhysicsJoint;
    rmt::Vector mPreviousSpeed;
    float mDt;
    float mInertialSpeedRate; 
    float mInertialAccelRate; 
    float mGravityRate; 
    float mCentrifugalRate; 
};

} // sim


#endif // _PHYSICSJOINT_HPP_