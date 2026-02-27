#ifndef _SemiImplicitSolver2D_hpp_
#define _SemiImplicitSolver2D_hpp_

#include "simflexible/semiimplicitsolver1D.hpp"

namespace sim
{
    
    class Condition;
    class LargeSymmetricMatrix;
    
    class SemiImplicitSolver2D : public SemiImplicitSolver1D
    {
        
    public:
        SemiImplicitSolver2D( int inNbp, ParticleSystem* inPSyst );
        ~SemiImplicitSolver2D(){};
        
        virtual void SolverApplyForces();
        
    protected:
        SemiImplicitSolver2D()
        {
            mSolverType = eSemiImplicitSolver2D;
            mPos_cm.Clear();
            mTorque0.Clear();
        }
        
        virtual void ComputeH(LargeSymmetricMatrix &io_mH) const;
        
        rmt::Vector mPos_cm;
        rmt::Vector mTorque0;
        
    };
    
} // sim

#endif //_SemiImplicitSolver2D_hpp_
