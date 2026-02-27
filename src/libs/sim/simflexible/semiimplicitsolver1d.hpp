#ifndef _SemiImplicitSolver1D_hpp_
#define _SemiImplicitSolver1D_hpp_

#include "simflexible/particlesystemsolver.hpp"

namespace sim
{
    
    class Condition;
    class LargeSymmetricMatrix;
    
    
    class SemiImplicitSolver1D : public ParticleSystemSolver
    {
        
    public:
        SemiImplicitSolver1D( int inNbp, ParticleSystem* inPSyst );
        ~SemiImplicitSolver1D();
        
        //Generic services:
        virtual void InitParticleSystemSolver();
        virtual void SolverApplyForces();
        virtual void SolverAdvanceSystem(float*);
        virtual void SolverClear();
        
        //Specific to the semi-implicit solver:
        virtual bool NeedPrecomputation(void) const {return mWNeedUpdate;}
        virtual void SetNeedPrecomputation(bool inNeed){mWNeedUpdate=inNeed;}
        virtual void Precompute(){UpdateW();}
        
        //By default, the solver uses the implicit damping. This mean that it will
        //reset to zero all coefficients of damping and will do damping by itself
        //in the solving process. This is because that way, the
        //particle system is the must stable. But if you want to use your own 
        //damping values. Call this method before the first update of particle system.
        virtual void SetUseExplicitDamping( ) {mImplicitDamping=false;}
        virtual bool GetUseExplicitDamping( ) const {return mImplicitDamping;}
        
    protected:
        SemiImplicitSolver1D() :
             mW(NULL), 
                 mH(NULL),
                 m_b(NULL),
                 m_z(NULL),
                 mWNeedUpdate(true)
             {
                 mSolverType = eSemiImplicitSolver1D;
             }
             
             
             virtual bool UpdateW();
             virtual void ComputeH(LargeSymmetricMatrix &io_mH) const;
             
             LargeSymmetricMatrix *mW, *mH;
             float                *m_b, *m_z;
             bool                 mWNeedUpdate; //If true, the filter mW need to be recomputed.
             bool                 mImplicitDamping;
             
    };
    
} // sim

#endif //_SemiImplicitSolver1D_hpp_
