#ifndef _KINEMATICSOLVER_HPP_
#define _KINEMATICSOLVER_HPP_

#include "simflexible/particlesystemsolver.hpp"

namespace sim
{
    
    class KinematicSolver : public ParticleSystemSolver
    {
        
    public:
        KinematicSolver( int inNbp, ParticleSystem* inPSyst );
        virtual ~KinematicSolver();
        
        //Generic services:
        virtual void InitParticleSystemSolver();
        virtual void SolverApplyForces();
        virtual void SolverAdvanceSystem(float*);
        virtual void SolverClear();
        
    protected:
        KinematicSolver ()
        {
            mSolverType = eKinematicSolver;
        }
        
        float *m_b, *m_z;
        
    };
    
} // sim

#endif //_KinematicSolver_hpp_
