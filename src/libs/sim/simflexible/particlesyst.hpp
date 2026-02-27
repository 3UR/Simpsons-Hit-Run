
#ifndef _PARTICLESYST_HPP_
#define _PARTICLESYST_HPP_

#include "simflexible/flexparticle.hpp"
#include "simflexible/condition.hpp"
#include "simflexible/particlesystemsolver.hpp"
#include "simflexible/largestepsolver.hpp"
#include "simcommon/tlist.hpp"

namespace sim
{
    
    typedef int* intp;
    typedef int** intpp;
    
    //Next three defines are used to clamp the convergence criteral between a min and a max value.
    #define MAX_EPS_CG   0.1f  
    #define MIN_EPS_CG   0.001f
    #define MAX_ITER_CG  20    //The maximum number of iteration in cg.
    
    class SparseBlockMatrix;
    class PhysicsProperties;
    class SimEnvironment;
    class pm;
    
    typedef FlexParticle* FlexParticlep;
    
    class FlexParticleList
        : public VoidPList
    {
    public:
        FlexParticleList(int i = TListMINSIZE) : VoidPList(i) {}
        inline FlexParticlep& GetAt(int i) { return (FlexParticlep&) (VoidPList::GetAt(i)); }
        inline FlexParticlep& operator[] (int i) { return (FlexParticlep&) (VoidPList::GetAt(i)); }
        inline void Add(const FlexParticlep el) { VoidPList::Add(voidp(el)); }
        inline bool Contains(const FlexParticlep el) const { return VoidPList::Contains(voidp(el)); }
        inline int Index(const FlexParticlep el) const { return VoidPList::Index(voidp(el)); }
        inline void Remove(const FlexParticlep el) { VoidPList::Remove(voidp(el)); }
        
        inline FlexParticleList& operator+=(const FlexParticleList& list) { VoidPList::Add((VoidPList&) list); return *this; }
        inline FlexParticleList& operator=(const FlexParticleList& list) { VoidPList::Set((VoidPList&) list); return *this; }
    };
    
    
    class ParticleSystem 
    {
    public:
        ParticleSystem(int n, float l);
        virtual ~ParticleSystem()=0;
        
        inline void  SetParticle(int n, FlexParticle *par);
        void virtual Scale(float f);
        virtual void SetDt(float dt);
        void SetParticleSystemSolver( eParticleSystemSolverType inType );
        void SetSimEnvironment(SimEnvironment* inSimEnvironment);
        SimEnvironment* GetSimEnvironment() { return mSimEnvironment; }
        
        bool GetHasMoved() const { return mHasMoved; }
        float GetMaxSpeed() const { return mMaxSpeed; }
        
        virtual int GetNbI()const {return mNbp/2;} 
        virtual int GetNbJ()const {return mNbp/2 + mNbp - (mNbp/2 + mNbp/2);}  
        virtual int GetNbK()const {return 0;}  
        
        virtual float GetLen0I()const {return mDeli;} 
        virtual float GetLen0J()const {return mDeli;}  
        virtual float GetLen0K()const {return mDeli;}  
        
        virtual void PreDisplay() {}
        virtual void Display();                      // default is to call render on each particle
        virtual void GetExtension(float& u, float& v) { u=v=0; }
        virtual void PreStepModification() {}
        
        void DrawForce();
        void DrawSpeed();
        
        float ExtMassParticle(int i) const { rAssert(i>=0 && i<mNbp); return mExtDensity * mMass[i]; }
        void  SetExtDensity(float in_val){ mExtDensity = in_val;}
        void  ScaleMassParticle(int i, float f) { rAssert(i>=0 && i<mNbp); mMass[i] *= f; }
        virtual void  SetOneConditionParameter( float inVal, eFlexPhysicalPropertiesEnum inWhich );
        virtual float GetOneConditionParameter( eFlexPhysicalPropertiesEnum inWhich );
        virtual void  SetParams(const ConditionParams &inParams)
        { 
            *mCparams = inParams;
            if (mPSystSolver)
                mPSystSolver->SetNeedPrecomputation(true);
        }
        virtual const ConditionParams & GetParams(){ return *mCparams;}
        virtual bool  SetLambda( float inStretchFact, float inDampingFact, bool inUseLongSide=true );
        virtual bool  SetKappa ( float inCurvatureFact, float inDampingFact, bool inUseLongSide=true );
        virtual bool  SetIota  ( float inFact, float inDampingFact, bool inUseLongSide=true );
        
        virtual void StartUpdate();
        virtual void EndUpdate();
        virtual void ApplyConstraints();
        virtual void ApplyForces();
        void ApplyGravity(float *io_dest);
        void ResetForces();
        virtual void AdvanceSystem();        // And move it along
        
        FlexParticle    **mP;
        Condition       *mConditions;
        ConditionParams *mCparams;
        
        float      *mMass;       // Array of size mVecSize to maintain the mass of each particles.
        float      *mDv;         // Array of size mVecSize to maintain delta speed during an update.
        
        int         mNbp;        // total nb of particle
        bool        mUseGravity;
        float       mTime;       // simulation clock
        float       mDeli;       // average edges length
        float       mExtDensity; // to increase density of the object as seen by the other but not in term of internal stress.
        rmt::Vector mWind;
        
        void FixParticle(int i);
        bool IsParticleFree(int i) const { return mP[i]->mFree; }
        void FreeParticle(int i);
        void SetPosParticle(int n, float x, float y, float z);
        void SetPosParticle(int n, const rmt::Vector& v) { SetPosParticle(n, v.x, v.y, v.z); }
        void SetSpeedParticle(int n, float x, float y, float z);
        void AddSpeedParticle(int n, const rmt::Vector& v);
        void AddSpeedSystem(const rmt::Vector& v);
        void AddAngularSpeedSystem( const rmt::Vector& angularVelocity, const rmt::Vector& origin);
        void AddAngularSpeedParticle(int index, const rmt::Vector& angularVelocity, const rmt::Vector& origin);
        void GetPosParticle(int n, float& x, float& y, float& z) const;
        void GetPosParticle(int n, rmt::Vector& v) const { GetPosParticle(n, v.x, v.y, v.z); }
        void GetVelParticle(int n, float& x, float& y, float& z) const;
        void GetVelParticle(int n, rmt::Vector& v) const{ GetVelParticle(n, v.x, v.y, v.z); };
        void MoveParticle(int n, float x, float y, float z);
        void RotateParticle(int n, int c, float angle);
        void MoveSystem(float x, float y, float z);
        void TransformSystem(const rmt::Matrix& m );
        void RotateParticle(const rmt::Matrix& m, const rmt::Vector& origin);
        void SetSystemSpeed(const rmt::Vector& speed);
        void RotateSystem(const rmt::Matrix& m, const rmt::Vector& origin);
        void RotateParticle(int i, const rmt::Matrix& m, const rmt::Vector& origin);
        void DynMoveParticle(int n, const rmt::Vector& v, float idt);
        void DynRotateSystem(int n, const rmt::Vector& omega, float idt);

        FlexParticleList mInterpList;
        virtual void StartIntermediateParticle() { mInterpList.RemoveAll(); }
        virtual bool GetIntermediateParticle(float pos[]) { return false; }
        virtual bool NeighborParticle(int i, int j) const { return false; }
        
        virtual void BreakAt(int i) { rAssertMsg(false, "TestParticleSystem::BreakAt Not Implemented");}
        bool  TestExtension1D(FlexParticle* a, FlexParticle* b, float len, float maxdef);
        void  SetMaxExtension( float in_val ){ mMaxDef = in_val; }
        float GetMaxExtension( )const{ return mMaxDef; }
        void  ScaleEdge( float val ){return;}
        
        virtual void SetConvergenceCriteral(float inEps, int inNbIter){};
        virtual void BuildConditions(){};
        
        bool  mDrawparticle;
        float mMaxDv;
        float mMaxDef;
        
        float mRest;       // rest coeff for the colliding particle
        float mFrict;      // friction coeff
        float mDvtStatic2; // square of mDv transverse below wich the particle should stop
        
protected:
    
    virtual void ClearVectors();
    void ConstrainParticle(int n);
    
    SimEnvironment       *mSimEnvironment;
    PhysicsProperties    *mPhysicsProperties;
    ParticleSystemSolver *mPSystSolver;
    
    friend class FlexibleObject;
    friend class LargeStepSolver;
    friend class SemiImplicitSolver1D;
    friend class SemiImplicitSolver2D;
    
    ParticleSystem();
    
    virtual void Init();
    
    bool  mHasMoved;  //Set to true if one particle has moved more than a fraction of the collision distance during an update.
    int   mVecSize;
    float mTimeStep;
    float mInvTimeStep;
    float mMaxSpeed;
    
};

} // sim

#endif // _PARTICLESYST_HPP_
