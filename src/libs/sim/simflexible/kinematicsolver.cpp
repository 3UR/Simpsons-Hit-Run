#include "simflexible/kinematicsolver.hpp"
#include "simflexible/particlesyst.hpp"

namespace sim
{

//Generic services:

KinematicSolver::KinematicSolver(int inNbp, ParticleSystem* inPSyst)
: ParticleSystemSolver(inNbp, inPSyst)
{
    mSolverType = eKinematicSolver;
    InitParticleSystemSolver();
}

KinematicSolver::~KinematicSolver()
{
    delete [] m_b;
}

void KinematicSolver::InitParticleSystemSolver()
{
    m_b  = new float[mSolverVecSize];
}



void KinematicSolver::SolverApplyForces()
{
    
    mPSyst->ApplyGravity(m_b); // This add the gravity force*dt into m_b
    
}

void KinematicSolver::SolverAdvanceSystem(float *io_dv)
{
    int i,i3;
    //Integrate speeds and positions
    for ( i=0,i3=0 ; i<mSolverNbp ; i++, i3=3*i)
    {
        if (mPSyst->mMass[i] )
        {
            ScaleVector(m_b+i3, 1/mPSyst->mMass[i], io_dv+i3);      // Here mDv represents the delta speed of each particles.
        }
    }
    
}

void KinematicSolver::SolverClear()
{
    ZeroVectorN(m_b, mSolverVecSize);
}

} // sim