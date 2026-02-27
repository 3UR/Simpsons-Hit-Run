#ifndef _LARGESTEPSOLVER_HPP_
#define _LARGESTEPSOLVER_HPP_

#include "simflexible/particlesystemsolver.hpp"
#include "simcommon/sbmatrix.hpp"

namespace sim
{
    
class LargeStepSolver : public ParticleSystemSolver
{
    
public:
    
    LargeStepSolver( int inNbp, ParticleSystem* inPSyst );
    ~LargeStepSolver();
    
    //Generic services:
    virtual void InitParticleSystemSolver();
    virtual void SolverApplyForces();
    virtual void SolverAdvanceSystem(float* io_dest)
    {
        mA->modified_pcg(m_b, &mSfilter, m_z, io_dest); 
    }
    virtual void SolverClear();
    
    //Specidic to the large step solver:
    virtual void SetConvergenceCriteral(float inEps, int inNbIter);
    virtual void ConstrainParticle(int index, int inNbDof, const rmt::Vector *in_dir1, const rmt::Vector *dir2);
    
protected:
    
    void LineConstrainParticle(int n, const rmt::Vector &in_dir1, const rmt::Vector &dir2, bool inCnsVel=true);
    void PlaneConstrainParticle(int n, const rmt::Vector &in_dir, bool inCnsVel=true);
    void FixParticle(int n, bool inCnsVel=true);
    void FreeParticle(int n) { mSfilter.RemoveBlock(n); }
    
    LargeStepSolver();
    friend class ParticleSystem;
    
    SparseBlockMatrix *mA;
    BlockArray mSfilter;
    float *m_b, *m_z;
    
};
    
} // sim

#endif //_LargeStepSolver_hpp_
