#ifndef _PARTICLESYSTEMSOLVER_hpp_
#define _PARTICLESYSTEMSOLVER_hpp_

#include "radmath/radmath.hpp"
#include <string.h>

namespace sim
{
    
    class Condition;
    class ParticleSystem;
    
    typedef enum { eLargeStepSolver=1, eSemiImplicitSolver1D, eSemiImplicitSolver2D, eEulerSolver, eKinematicSolver, eUndefinedSolver } eParticleSystemSolverType;
    
    //Abstract base class that define all services required for any flexible object solver.
    class ParticleSystemSolver
    {
        
    public:
        ParticleSystemSolver( int inNbp, ParticleSystem* inPSyst );
        virtual ~ParticleSystemSolver(){};
        
        //Generic services:
        virtual void InitParticleSystemSolver()=0;
        virtual void SolverApplyForces()=0;
        virtual void SolverAdvanceSystem(float*){};
        virtual void SolverClear(){};
        
        // Large step solver specific:
        virtual void SetConvergenceCriteral(float inEps, int inNbIter){};
        virtual void ConstrainParticle(int n, int inNbDof, const rmt::Vector *dir1, const rmt::Vector *dir2){};
        
        // Semi-implicit solver specific:
        virtual bool NeedPrecomputation(void) const {return false;} //Filter have to be updated when dt and/or other stuff changed.
        virtual void SetNeedPrecomputation(bool){}
        virtual void Precompute(){}
        
        eParticleSystemSolverType GetSolverType() const {return mSolverType;}
        
    protected:
        ParticleSystemSolver():
             mSolverType(eUndefinedSolver),
                 mPSyst(NULL),
                 mSolverNbp(0),
                 mSolverVecSize(0){}
             
             eParticleSystemSolverType mSolverType;
             ParticleSystem           *mPSyst;
             int                       mSolverNbp;
             int                       mSolverVecSize;
    };
    
} // sim

#endif //_ParticleSystemSolver_hpp_
