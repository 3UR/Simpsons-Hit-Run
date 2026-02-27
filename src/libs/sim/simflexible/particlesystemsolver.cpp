
#include "simflexible/particlesystemsolver.hpp"
#include "simcommon/arraymath.hpp"  //for USE_FAST_MATH4

namespace sim
{
    
ParticleSystemSolver::ParticleSystemSolver( int inNbp, ParticleSystem* inPSyst )
: mSolverType(eUndefinedSolver),
  mPSyst(inPSyst),
  mSolverNbp(inNbp)
{
    mSolverVecSize = 3*mSolverNbp;
#ifdef USE_FAST_MATH4
    MakeMultipleOf4(mSolverVecSize);
#endif
    rAssert(mPSyst);
}

} // sim