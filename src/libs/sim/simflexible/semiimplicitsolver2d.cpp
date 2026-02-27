
#include "simflexible/semiimplicitsolver2D.hpp"
#include "simflexible/particlesyst.hpp"
#include "simcommon/largesymmetricmatrix.hpp"

using namespace rmt;

namespace sim
{

static bool doAngMomCorrection=true; //todo :Check this angular momentum post correction.

SemiImplicitSolver2D::SemiImplicitSolver2D(int inNbp, ParticleSystem* inPSyst)
 : SemiImplicitSolver1D( inNbp, inPSyst )
{
    mImplicitDamping = false; //The implicit damping is not implemented in the 2D case.
    mSolverType = eSemiImplicitSolver2D;
    mPos_cm.Clear();
    mTorque0.Clear();
}

void SemiImplicitSolver2D::ComputeH(LargeSymmetricMatrix &io_mH) const   
{
    io_mH.Zero();
    Condition *C=mPSyst->mConditions->next(); // Skip the fencepost NullCondition
    while (C) 
    {
        io_mH(C->mPi->mIndex, C->mPj->mIndex) += C->GetStretch1Dkf();
        io_mH(C->mPi->mIndex, C->mPi->mIndex) -= C->GetStretch1Dkf();
        io_mH(C->mPj->mIndex, C->mPj->mIndex) -= C->GetStretch1Dkf();
        C = C->next();
    }
}

void SemiImplicitSolver2D::SolverApplyForces()
{
    
    int i, i3;
    //Compute the center of mMass position.
    Vector pos;
    mPos_cm.Clear();
    for ( i=0 ; i<mSolverNbp ; i++ )
    {
        mPSyst->GetPosParticle(i, pos);
        mPos_cm.Add(pos);
    }
    mPos_cm.Scale(1.0f/mSolverNbp); //Current center of mass position.
    
    if (mWNeedUpdate)
    {
        if ( UpdateW() == false )
            return;
    }
    
    float  dt=mPSyst->mTimeStep;
    
    //Compute internal forces on each particles.
    Condition *C=mPSyst->mConditions->next(); // Skip the fencepost NullCondition
    while (C) 
    {
        C->BuildMatrix(NULL, m_b, dt);
        C = C->next();
    }
    
    //Pre-Multiply by the W matrix and store the result in m_z.
    mW->PreMultiply3x(m_b,m_z); // m_z contains dt time the filtered internal forces.
    
    if (doAngMomCorrection)
    {
        //This post-correction for the angular momentum is not correct. It 
        //really decrease the angular momentum but affect also the net forces
        //which is not correct. Keep it for now because it smooth the solver.
        static float deltaForceMagnetude=0.3f;
        Vector deltaForce;
        for( i=0, i3=0 ; i<mSolverNbp ; i++, i3=3*i)
        {
            deltaForce = *(Vector*)(m_z+i3);
            deltaForce.Scale(-deltaForceMagnetude);
            AddVector(m_z+i3, (float*)&deltaForce, m_z+i3 );
        }
    }
    
    mPSyst->ApplyGravity(m_z); // This add the gravity*dt into m_z
}

} // sim