
#include "simflexible/particlesyst.hpp"
#include "simflexible/largestepsolver.hpp"
#include "simflexible/semiimplicitsolver1D.hpp"
#include "simflexible/semiimplicitsolver2D.hpp"
#include "simflexible/kinematicsolver.hpp"
#include "simcommon/simwind.hpp"
#include "simcommon/dline2.hpp"


using namespace RadicalMathLibrary;

namespace sim
{

// display scaling values
static float scaleSforce = 0.0f, scaleBforce = 0.02f;
static float scaleSpeed = 0.25f, scaleCustum = .0001f;


// stabilizer
static float DVSTATIC2 = 715.0f;
static float MAXDV = 1336.0f;

bool moveInColl = false;      // indicates if we should move manually the colliding particle in order to esolve interpenetration


ParticleSystem::ParticleSystem(int n, float l) 
: mConditions(NULL),
  mCparams(NULL),
  mNbp(n),
  mUseGravity(true),
  mTime(0),
  mDeli(l),
  mExtDensity(1.0f),
  mDrawparticle(false),
  mMaxDv(MAXDV),
  mMaxDef(0.1f),
  mRest(1.2f),
  mFrict(-10.0f),
  mDvtStatic2(DVSTATIC2),
  mSimEnvironment(NULL),
  mPhysicsProperties(NULL),
  mPSystSolver(NULL),
  mHasMoved(false),
  mMaxSpeed(0)
{
    Init();
}

ParticleSystem::ParticleSystem() 
: mConditions(NULL),
  mCparams(NULL),
  mNbp(0),
  mUseGravity(true),  //! this isn't used.  should be removed.
  mTime(0),
  mDeli(0),
  mExtDensity(1.0f),
  mDrawparticle(false),
  mMaxDv(MAXDV),
  mMaxDef(0.1f),
  mRest(1.2f),
  mFrict(-10.0f),
  mDvtStatic2(DVSTATIC2),
  mSimEnvironment(NULL),
  mPhysicsProperties(NULL),
  mPSystSolver(NULL),
  mHasMoved(false),
  mMaxSpeed(0)
{
}

void ParticleSystem::Init()
{
    SimLength tmpl(1.0f);
    SimTime tmpt(1.0f);
    
    mDrawparticle = false;
    mCparams = new ConditionParams;
    
    mWind.Clear();
    
    // all the particle has the same mMass. 
    // this wouldn't work with inhomogeneous lattice
    mMass = new float[mNbp];
    mP    = new FlexParticlep[mNbp];
    
    //float tmpm = tmpl.iValue()*Sqr(tmpl.iValue());
    for(int i=0; i<mNbp; i++) 
    {
        mP[i] = new FlexParticle(i);
        mMass[i] = 1.0f;
    }
    
    mMaxDv = MAXDV * tmpl.Value()/tmpt.Value();
    mDvtStatic2 = DVSTATIC2 * Sqr(tmpl.Value() / tmpt.Value());
    
    mVecSize = 3*mNbp;
    
#ifdef USE_FAST_MATH4
    MakeMultipleOf4(mVecSize);
#endif
    
    mDv = new float[mVecSize];
    
    
    mConditions = new NullCondition(mCparams);
    SetDt(0.016f);
    
    ClearVectors();
}

void ParticleSystem::SetParticleSystemSolver( eParticleSystemSolverType inType )
{
    
    switch (inType)
    {
    case eLargeStepSolver:
        mPSystSolver = new LargeStepSolver(mNbp, this);
        break;
    case eSemiImplicitSolver1D:
        mPSystSolver = new SemiImplicitSolver1D(mNbp, this);
        break;
    case eSemiImplicitSolver2D:
        mPSystSolver = new SemiImplicitSolver2D(mNbp, this);
        break;
    case eKinematicSolver:
        mPSystSolver = new KinematicSolver(mNbp, this);
        break;
    case eEulerSolver:
        rAssertMsg(0,"Not Implemented");
        break;
    default:
        rAssertMsg(0,"Invalid Solver Type");
    }
}

void ParticleSystem::SetSimEnvironment(SimEnvironment* inSimEnvironment)
{
    rAssert(inSimEnvironment);
    
    if (mSimEnvironment != inSimEnvironment)
    {
        if (mSimEnvironment != NULL)
            mSimEnvironment->Release();
        
        mSimEnvironment = inSimEnvironment;
        mSimEnvironment->AddRef();
    }
}

void ParticleSystem::Scale(float f)
{
    mDeli *= f;
    
    ScaleEdge( f );
    
    Condition *C=mConditions->next(); // Skip the fencepost NullCondition
    while (C) 
    {
        C->Scale(f);
        C = C->next();
    }
    
    SetConvergenceCriteral( Max(Min(MAX_EPS_CG,mDeli/mNbp),MIN_EPS_CG), MAX_ITER_CG );
    
}

void ParticleSystem::SetDt(float inDt) 
{ 
    rAssert(inDt > 0); 
    
    if( inDt != mTimeStep && mPSystSolver)
    {
        mPSystSolver->SetNeedPrecomputation(true);
    }
    
    mTimeStep = inDt; 
    mInvTimeStep = 1.0f/mTimeStep; 
}

void ParticleSystem::SetParticle(int n, FlexParticle *par) 
{ 
    mP[n] = par;
    if (par)
        par->mIndex = n;
}


ParticleSystem::~ParticleSystem()
{
    if (mConditions != NULL)
    {
        while(mConditions->next())
            mConditions->RemoveNext();
        delete mConditions;
    }
    delete mCparams;
    delete[] mDv;
    delete[] mMass;
    
    for(int i=0;i<mNbp;i++)
    {
        delete mP[i];
    }
    delete []mP;
    delete mPSystSolver;
    
    if (mSimEnvironment)
        mSimEnvironment->Release();
}

void ParticleSystem::DrawForce()
{
#ifdef RAD_DEBUG
    float f[3];
    tColour c1(0, 50, 200);
    tColour c2(200, 50, 0);
    tColour c3(100, 100, 100);
    
    DrawLineToggler toggler;

    for(int i=0;i<mNbp;i++) 
    {
        FlexParticle* pp = mP[i];
        AddScaleVector(pp->m_x, pp->m_fs, scaleSforce, f);
        dStreamLine(*((rmt::Vector*)pp->m_x), *((rmt::Vector*)f), c1, c1);
        
        AddScaleVector(pp->m_x, pp->m_fb, scaleBforce, f);
        dStreamLine(*((rmt::Vector*)pp->m_x), *((rmt::Vector*)f), c2, c2);
        
        AddScaleVector(pp->m_x, pp->m_nn, scaleCustum, f);   
        dStreamLine(*((rmt::Vector*)pp->m_x), *((rmt::Vector*)f), c3, c3);
    }
#endif
}

void ParticleSystem::DrawSpeed()
{
#ifdef RAD_DEBUG
    float f[3];
    tColour c1(0, 200, 50);
    
    DrawLineToggler toggler;

    for(int i=0;i<mNbp;i++) 
    {
        FlexParticle* pp = mP[i];
        AddScaleVector(pp->m_x, pp->m_v, scaleSpeed, f);
        dStreamLine(*((rmt::Vector*)pp->m_x), *((rmt::Vector*)f), c1, c1);
    }
#endif
}

void  ParticleSystem::SetOneConditionParameter( float inVal, eFlexPhysicalPropertiesEnum inWhich )
{
    switch( inWhich )
    {
    case eFlexStretch1D_f : 
        if (mCparams->Stretch1Dkf != inVal)
        {
            mPSystSolver->SetNeedPrecomputation(true);
            mCparams->Stretch1Dkf   = inVal; 
        }
        break;
    case eFlexStretch1D_d : 
        if (mCparams->Stretch1Dkd != inVal)
        {
            mPSystSolver->SetNeedPrecomputation(true);
            mCparams->Stretch1Dkd   = inVal; 
        }
        break;
    case eFlexStretch2D_f : 
        if (mCparams->Stretch2Dkf != inVal)
        {
            mPSystSolver->SetNeedPrecomputation(true);
            mCparams->Stretch2Dkf   = inVal; 
        }
        break;
    case eFlexStretch2D_d : 
        if (mCparams->Stretch2Dkd != inVal)
        {
            mPSystSolver->SetNeedPrecomputation(true);
            mCparams->Stretch2Dkd   = inVal; 
        }
        break;
    case eFlexBend1D_f    : 
        if (mCparams->Bend1Dkf != inVal)
        {
            mPSystSolver->SetNeedPrecomputation(true);
            mCparams->Bend1Dkf      = inVal; 
        }
        break;
    case eFlexBend1D_d    : 
        if (mCparams->Bend1Dkd != inVal)
        {
            mPSystSolver->SetNeedPrecomputation(true);
            mCparams->Bend1Dkd      = inVal; 
        }
        break;
    case eFlexBend2D_f    : 
        if (mCparams->Bend2Dkf != inVal)
        {
            mPSystSolver->SetNeedPrecomputation(true);
            mCparams->Bend2Dkf      = inVal; 
        }
        break;
    case eFlexBend2D_d    : 
        if (mCparams->Bend2Dkd != inVal)
        {
            mPSystSolver->SetNeedPrecomputation(true);
            mCparams->Bend2Dkd      = inVal; 
        }
        break;
    case eFlexShear2D_f   : 
        if (mCparams->Shear2Dkf != inVal)
        {
            mPSystSolver->SetNeedPrecomputation(true);
            mCparams->Shear2Dkf     = inVal; 
        }
        break;
    case eFlexShear2D_d   : 
        if (mCparams->Shear2Dkd != inVal)
        {
            mPSystSolver->SetNeedPrecomputation(true);
            mCparams->Shear2Dkd     = inVal; 
        }
        break;
    case eFlexWind1D_f    : 
        if (mCparams->Wind1DKf != inVal)
        {
            mPSystSolver->SetNeedPrecomputation(true);
            mCparams->Wind1DKf      = inVal; 
        }
        break;
    case eFlexWind2D_f    : 
        if (mCparams->Wind2DKf != inVal)
        {
            mPSystSolver->SetNeedPrecomputation(true);
            mCparams->Wind2DKf      = inVal; 
        }
        break;      
    }
    
}

float ParticleSystem::GetOneConditionParameter( eFlexPhysicalPropertiesEnum inWhich )
{
    switch( inWhich )
    {
    case eFlexStretch1D_f : return mCparams->Stretch1Dkf;
    case eFlexStretch1D_d : return mCparams->Stretch1Dkd;
    case eFlexStretch2D_f : return mCparams->Stretch2Dkf;
    case eFlexStretch2D_d : return mCparams->Stretch2Dkd;
    case eFlexBend1D_f    : return mCparams->Bend1Dkf   ;
    case eFlexBend1D_d    : return mCparams->Bend1Dkd   ;
    case eFlexBend2D_f    : return mCparams->Bend2Dkf   ;
    case eFlexBend2D_d    : return mCparams->Bend2Dkd   ;
    case eFlexShear2D_f   : return mCparams->Shear2Dkf  ;
    case eFlexShear2D_d   : return mCparams->Shear2Dkd  ;
    case eFlexWind1D_f    : return mCparams->Wind1DKf   ;
    case eFlexWind2D_f    : return mCparams->Wind2DKf   ;      
    }
    rAssert(0);//Invalid case.
    return 1;
}

void ParticleSystem::ResetForces()
{
#ifdef RAD_DEBUG
    for(int i=0;i<mNbp;i++)
    {
        ZeroVector(mP[i]->m_fs);  // reset the force vector
        ZeroVector(mP[i]->m_fb);  // reset the force vector
        ZeroVector(mP[i]->m_nn);  // reset the force vector
        ZeroVector(mP[i]->m_fc);  // reset the force vector
    }
#endif
}

void ParticleSystem::StartUpdate()
{
    mHasMoved = false;
    ResetForces();          // set the p[i].f to 0. This is for debug only.
}

void ParticleSystem::EndUpdate()
{
    for( int i=0 ; i<mNbp ; i++ )
    {
        mP[i]->ResetDirectionOfConstraint();
    }
}

//Effectively apply constraint on each particles if any.
void ParticleSystem::ApplyConstraints()
{
    for( int i=0 ; i<mNbp ; i++)
    {
        ConstrainParticle(i);
    }
}   

void ParticleSystem::ApplyForces()
{
    mPSystSolver->SolverApplyForces();
}

void ParticleSystem::ApplyGravity(float *io_dest)
{
    float* pb = io_dest;
    const float* g = &(mSimEnvironment->Gravity().x);
    for(int i=0; i<mNbp; i++) 
    {
        if (mP[i]->mFree)
            AddScaleVector(pb, g, mTimeStep, pb);
        pb += 3;
    }
}

void ParticleSystem::AdvanceSystem() 
{
    
    if (mSimEnvironment && mSimEnvironment->Wind())
        mWind = mSimEnvironment->Wind()->GetWind();
    
    mPSystSolver->SolverAdvanceSystem(mDv);
    
    int i;
    mMaxSpeed = 0;
    
    for( i=0; i<mNbp; i++) 
    {
        FlexParticle* pp = mP[i];
        if (pp->mFree)
        {
            int i3 = i*3;
            float dot = DotVector(&mDv[i3], &mDv[i3]);
            if (dot > mMaxDv)
            {
                ScaleVector(&mDv[i3], Sqrt(mMaxDv/dot), &mDv[i3]);
            }
            if (pp->mNbCollConstraint)
            {
                pp->CleanForConstraint((Vector*)&mDv[i3]);
            }
            
            Vector dx;
            dx = *(Vector*)pp->m_v;
            AddVector(pp->m_v, &mDv[i3], pp->m_v);
            
            dx.Add(*(Vector*)pp->m_v);
            dx.Scale(mTimeStep/2);
            
            //We have cleaned the current velocity at the time we have
            //instaled the constraint, we have cleaned the dv, 
            //dx do not have something to clean at this point.
            
            AddVector(pp->m_x, (float*)&dx, pp->m_x);
            float dx2 = dx.DotProduct(dx);
            mMaxSpeed = Max(mMaxSpeed, dx2);
            
            if (!mHasMoved && dx2 > Sqr(mSimEnvironment->CollisionDistance()/10))
            {
                mHasMoved=true;
            }
        }
        else
        {
            ZeroVector(pp->m_v);
        }
    }
    
    mTime += mTimeStep;
    
    mMaxSpeed = Sqrt(mMaxSpeed)/mTimeStep;
    
    PreDisplay();
    
    //Clean forces and all vector to be ready for the next step.
    ClearVectors();
}

void ParticleSystem::ClearVectors()
{
    if (mPSystSolver)
    {
        mPSystSolver->SolverClear();
    }
    ZeroVectorN(mDv, mVecSize);
}

void ParticleSystem::Display() 
{
    //   for(int i=0; i<mNbp; i++)
    //      DrawParticle(mP[i]->m_x);
}

void ParticleSystem::MoveSystem(float x, float y, float z)
{
    for(int i=0; i<mNbp; i++)
    {
        mP[i]->m_x[0] += x;
        mP[i]->m_x[1] += y;
        mP[i]->m_x[2] += z;
    }
}

void ParticleSystem::TransformSystem(const Matrix& m)
// Transform the system as if it were a rigid body.  The speed of particles is
// not affected except that the velocities are rotated (magnitudes are constant).
{
    // could assert that m is orthogonal
    Vector tmp, tmp2;
    for(int i=0; i<mNbp; i++)
    {
        tmp.Set(mP[i]->m_x[0], mP[i]->m_x[1], mP[i]->m_x[2]);
        tmp2.Transform(tmp, m);
        SetVector(mP[i]->m_x, tmp2.x, tmp2.y, tmp2.z);
        
        /*
        // Now transform the speed
        tmp.Set(mP[i]->m_v[0], mP[i]->m_v[1], mP[i]->m_v[2]);
        tmp2.Rotate(tmp, m);
        SetVector(mP[i]->m_v, tmp2.x, tmp2.y, tmp2.z);
    */
    }
    
}

void ParticleSystem::RotateParticle(int i, const Matrix& m, const Vector& origin)
{
    // could assert that m is orthogonal
    Vector tmp, tmp2;
        // mP[i]->m_x is a point in worldspace so we need 
        // subtract from a point on the axis of rotation
        tmp.Set(mP[i]->m_x[0], mP[i]->m_x[1], mP[i]->m_x[2]);
        tmp.Sub(origin);
        tmp2.Rotate(tmp, m);
        tmp2.Add(origin);
        SetVector(mP[i]->m_x, tmp2.x, tmp2.y, tmp2.z);
        
        // Now transform the speed
        // Since mP[i]->m_v is a vector, we don't need to subtract
        tmp.Set(mP[i]->m_v[0], mP[i]->m_v[1], mP[i]->m_v[2]);
        tmp2.Rotate(tmp, m);
        SetVector(mP[i]->m_v, tmp2.x, tmp2.y, tmp2.z);
    
}

void ParticleSystem::RotateSystem(const Matrix& m, const Vector& origin)
// Transform the system as if it were a rigid body.  The speed of particles is
// not affected except that the velocities are rotated (magnitudes are constant).
{
    // could assert that m is orthogonal
    Vector tmp, tmp2;
    for(int i=0; i<mNbp; i++)
    {
        // mP[i]->m_x is a point in worldspace so we need 
        // subtract from a point on the axis of rotation
        tmp.Set(mP[i]->m_x[0], mP[i]->m_x[1], mP[i]->m_x[2]);
        tmp.Sub(origin);
        tmp2.Rotate(tmp, m);
        tmp2.Add(origin);
        SetVector(mP[i]->m_x, tmp2.x, tmp2.y, tmp2.z);
        
        // Now transform the speed
        // Since mP[i]->m_v is a vector, we don't need to subtract
        tmp.Set(mP[i]->m_v[0], mP[i]->m_v[1], mP[i]->m_v[2]);
        tmp2.Rotate(tmp, m);
        SetVector(mP[i]->m_v, tmp2.x, tmp2.y, tmp2.z);
    }
    
}

void ParticleSystem::SetSystemSpeed(const Vector& speed)
{
    for(int i=0; i<mNbp; i++)
    {
        SetVector(mP[i]->m_v, speed.x, speed.y, speed.z);     
    }
}

void ParticleSystem::MoveParticle(int i, float x, float y, float z)
{
    rAssert(i < mNbp);
    mP[i]->m_x[0] += x;
    mP[i]->m_x[1] += y;
    mP[i]->m_x[2] += z;
}

void ParticleSystem::SetPosParticle(int i, float x, float y, float z)
{
    rAssert(i < mNbp);
    mP[i]->m_x[0] = x;
    mP[i]->m_x[1] = y;
    mP[i]->m_x[2] = z;
}

void ParticleSystem::DynMoveParticle(int i, const Vector& v, float idt)
{
    rAssert(i < mNbp);
    
    SubVector((float*)&v, mP[i]->m_x, mP[i]->m_v);
    ScaleVector(mP[i]->m_v, idt, mP[i]->m_v);
    SetVector(mP[i]->m_x, v.x, v.y, v.z);
}

void ParticleSystem::SetSpeedParticle(int i, float x, float y, float z)
{
    rAssert(i < mNbp);
    mP[i]->m_v[0] = x;
    mP[i]->m_v[1] = y;
    mP[i]->m_v[2] = z;
}

void ParticleSystem::AddSpeedParticle(int i, const rmt::Vector& v)
{
    rAssert(i < mNbp);

    mP[i]->m_v[0] += v.x;
    mP[i]->m_v[1] += v.y;
    mP[i]->m_v[2] += v.z;
}

void ParticleSystem::AddSpeedSystem(const rmt::Vector& v)
{
    for ( int i=0; i<mNbp; i++ )
    {
        mP[i]->m_v[0] += v.x;
        mP[i]->m_v[1] += v.y;
        mP[i]->m_v[2] += v.z;
    }
}


void ParticleSystem::AddAngularSpeedParticle(int i, const Vector& angularVelocity, const Vector& origin)
{

    // Need to rotate about root particle
    // pos is a particle's position in the root particle frame
    Vector vel, radius;
    SubVector(mP[i]->m_x, (float*)&origin, (float*)&radius);
    vel.CrossProduct( radius, angularVelocity );
    SetVector(mP[i]->m_v, vel.x + mP[i]->m_v[0], vel.y + mP[i]->m_v[1], vel.z + mP[i]->m_v[2]);
}

void ParticleSystem::AddAngularSpeedSystem( const Vector& angularVelocity, const Vector& origin)
{

    // Need to rotate about root particle
    // pos is a particle's position in the root particle frame
    for(int i=0; i<mNbp; i++)
    {
        Vector vel, radius;
        SubVector(mP[i]->m_x, (float*)&origin, (float*)&radius);
        vel.CrossProduct( radius, angularVelocity );
        SetVector(mP[i]->m_v, vel.x + mP[i]->m_v[0], vel.y + mP[i]->m_v[1], vel.z + mP[i]->m_v[2]);
    }
}

void ParticleSystem::GetPosParticle(int i, float& x, float& y, float& z) const
{
    rAssert(i < mNbp);
    x = mP[i]->m_x[0];
    y = mP[i]->m_x[1];
    z = mP[i]->m_x[2];
}

void ParticleSystem::GetVelParticle(int i, float& x, float& y, float& z) const
{
    rAssert(i < mNbp);
    x = mP[i]->m_v[0];
    y = mP[i]->m_v[1];
    z = mP[i]->m_v[2];
}

// rotate particle n around particle c of angle around z
void ParticleSystem::RotateParticle(int n, int c, float angle)
{
    if (n < mNbp && c < mNbp && n != c)
    {
        float v[3], rotatedv[3];
        float sa = float(sin(angle));
        float ca = float(cos(angle));
        SubVector(mP[n]->m_x, mP[c]->m_x, v);
        rotatedv[0] =  ca*v[0] + sa*v[1];
        rotatedv[1] = -sa*v[0] + ca*v[1];
        rotatedv[2] = v[2];
        AddVector(mP[c]->m_x, rotatedv, mP[n]->m_x);
    }
    
}

void ParticleSystem::ConstrainParticle(int index)
{
    rAssert(index < mNbp);
    
    if (mP[index]->mFree)
    {
        switch( mP[index]->mNbCollConstraint )
        {
        case 3:
            //0Dof. Particle constrained to not move.
            mPSystSolver->ConstrainParticle( index, 0, NULL, NULL);
            //Clean the current velocity of the particle along the set of constraining direction.
            mP[index]->CleanForConstraint((Vector *)mP[index]->m_v);
            break;
        case 2:
            //1Dof. Particle constrained to move along a line.
            mPSystSolver->ConstrainParticle( index, 1, &mP[index]->mCollConstraint[0], &mP[index]->mCollConstraint[1]);
            //Clean the current velocity of the particle along the set of constraining direction.
            mP[index]->CleanForConstraint((Vector *)mP[index]->m_v);
            break;
        case 1:
            //2Dof. Particle constrained to move on a plane.
            mPSystSolver->ConstrainParticle( index, 2, &mP[index]->mCollConstraint[0], NULL);
            //Clean the current velocity of the particle along the set of constraining direction.
            mP[index]->CleanForConstraint((Vector *)mP[index]->m_v);
            break;
        }
    }
}


void ParticleSystem::FixParticle(int n)
{
    rAssert(n < mNbp);
    mPSystSolver->ConstrainParticle( n, 0, NULL, NULL);
    mP[n]->mFree  = false;
    ZeroVector(mP[n]->m_v); // reset the particle's speed
}

void ParticleSystem::FreeParticle(int n)
{
    rAssert(n < mNbp);
    mPSystSolver->ConstrainParticle( n, 3, NULL, NULL );
    mP[n]->mFree  = true;
}

inline bool IsFreeParticle(FlexParticle* p)
{
    return (moveInColl ? p->mFree : p->mNbCollConstraint==0 && p->mFree);
}

bool ParticleSystem::TestExtension1D(FlexParticle* a, FlexParticle* b, float len, float mxdef)
{
    rAssert(mxdef>0 && mxdef<1);
    if (!IsFreeParticle(a) && !IsFreeParticle(b))
        return false;
    
    float dp[3];
    
    SubVector(a->m_x, b->m_x, dp);
    
    float ndp = LenVector(dp);
    float def = (ndp - len)/len;
    
    if (Fabs(def) > mxdef)
    {
        ndp = 1 - len*( 1.0f + Sign(def)*mxdef )/ndp;
        
        if (IsFreeParticle(a) && IsFreeParticle(b))
        {
            ScaleVector(dp, ndp/2.0f, dp);
            b->MoveSelf(dp, mInvTimeStep);
            a->UnMoveSelf(dp, mInvTimeStep);
        }
        else if (IsFreeParticle(a))
        {
            ScaleVector(dp, ndp, dp);
            a->UnMoveSelf(dp, mInvTimeStep);
        }
        else if (IsFreeParticle(b))
        {
            ScaleVector(dp, ndp, dp);
            b->MoveSelf(dp, mInvTimeStep);
        }
        return true;
    }
    else
        return false;
}

/*
This method is to help the parametrisation the stretching of a particle system.

  See implementation of ConditionParams::ComputeStretch1DFromLambda for more details.
  
    For 2D particle system, formulas are computed based on the longest side.
    Use inUseLongSide=false to reverse this.
    For flexible on geometry or a manual flexible object, 
    the number of particles could not be related to the length of 
    the object. Formulas could become not accurate. But the relation between
    the force and the damping factor remains. 
*/
bool  ParticleSystem::SetLambda
( 
 float inStretchFact,     //Percentage of tolerance to stretch. Ex: 10 ( mean 10%)
 float inDampingFact,     //Coefficient of damping. 1 mean no oscillation.
 bool  inUseLongSide      //If true, computation are done with the shortest side. (default to true)
 )
{
    
    float LengthI, LengthJ, length0;
    int   nbPart;
    
    //Estimate side lengths.
    LengthI = GetLen0I()*(GetNbI()-1);
    LengthJ = GetLen0J()*(GetNbJ()-1);
    if (inUseLongSide)
    {
        if (LengthI >= LengthJ )
        {
            nbPart  = GetNbI();
            length0 = GetLen0I();
        }
        else
        {
            nbPart  = GetNbJ();
            length0 = GetLen0J();
        }
    }
    else
    {
        if (LengthI>0 && LengthJ>0)
        {
            if (LengthI < LengthJ )
            {
                nbPart  = GetNbI();
                length0 = GetLen0I();
            }
            else
            {
                nbPart  = GetNbJ();
                length0 = GetLen0J();
            }
        }
        else if (LengthI>0 )
        {
            nbPart  = GetNbI();
            length0 = GetLen0I();
        }
        else
        {
            nbPart  = GetNbJ();
            length0 = GetLen0J();
        }
    }
    
    float mass, gravity;
    mass = mMass[0];
    if ( GetSimEnvironment()==NULL )
        return false;
    
    gravity = GetSimEnvironment()->GravityNorm();
    
    bool result;
    result = 0;//mCparams->SetStretch1DFromLambda(inStretchFact, inDampingFact, nbPart, length0, gravity, mass);
    
    if (mPSystSolver)
        mPSystSolver->SetNeedPrecomputation(true);
    
    return result;
}

/* 
This method is to help the parametrisation the bending of a particle system.

  See ParticleSystem::SetLambda above and comments at the implementation of 
  ConditionParams::ComputeBend1DFromKappa for more details.
  
*/
bool  ParticleSystem::SetKappa 
( 
 float inCurvatureFact,  //Curvature factor from natural curvature.
 float inDampingFact,    //Coefficient of damping. 1 mean no oscillation.
 bool  inUseLongSide     //If true, computation are done with the shortest side. (default to true)
 )
{
    float LengthI, LengthJ, length0;
    int   nbPart;
    
    //Estimate side lengths.
    LengthI = GetLen0I()*(GetNbI()-1);
    LengthJ = GetLen0J()*(GetNbJ()-1);
    if (inUseLongSide)
    {
        if (LengthI >= LengthJ )
        {
            nbPart  = GetNbI();
            length0 = GetLen0I();
        }
        else
        {
            nbPart  = GetNbJ();
            length0 = GetLen0J();
        }
    }
    else
    {
        if (LengthI>0 && LengthJ>0)
        {
            if (LengthI < LengthJ )
            {
                nbPart  = GetNbI();
                length0 = GetLen0I();
            }
            else
            {
                nbPart  = GetNbJ();
                length0 = GetLen0J();
            }
        }
        else if (LengthI>0 )
        {
            nbPart  = GetNbI();
            length0 = GetLen0I();
        }
        else
        {
            nbPart  = GetNbJ();
            length0 = GetLen0J();
        }
    }
    
    float mass, gravity;
    mass = mMass[0];
    if ( GetSimEnvironment()==NULL )
        return false;
    
    gravity = GetSimEnvironment()->GravityNorm();
    
    float f,d;
    bool result1, result2;
    result1 = 0;//mCparams->SetBend1DFromKappa(inCurvatureFact, inDampingFact, nbPart, length0, gravity, mass);
    result2 = ConditionParams::ComputeBend1DFromKappa(inCurvatureFact, inDampingFact, nbPart, length0, gravity, f, d, mass);
    
    SetOneConditionParameter(f, eFlexBend2D_f);
    SetOneConditionParameter(d, eFlexBend2D_d);
    
    if (mPSystSolver)
        mPSystSolver->SetNeedPrecomputation(true);
    
    return (result1 && result2);
}

/*
Usually, good value for inFact should be greater than 10 time the percentage of ellongation
passed in the method SetLambda.
*/
bool  ParticleSystem::SetIota  ( float inFact, float inDampingFact, bool )
{
    float LengthI, LengthJ, length0;
    int   nbPart;
    
    //Estimate side lengths.
    LengthI = GetLen0I()*(GetNbI()-1);
    LengthJ = GetLen0J()*(GetNbJ()-1);
    if (LengthI >= LengthJ )
    {
        nbPart  = GetNbI();
        length0 = GetLen0I();
    }
    else
    {
        nbPart  = GetNbJ();
        length0 = GetLen0J();
    }
    
    float mass, gravity;
    mass = mMass[0];
    if ( GetSimEnvironment()==NULL )
        return false;
    
    gravity = GetSimEnvironment()->GravityNorm();
    
    float f, d;
    bool result = ConditionParams::ComputeStretch1DFromLambda(inFact, inDampingFact, nbPart, length0, gravity, f, d, mass);
    
    SetOneConditionParameter(f, eFlexShear2D_f);
    SetOneConditionParameter(d, eFlexShear2D_d);
    return result;
    
}

} // sim
