
#include "simflexible/largestepsolver.hpp"
#include "simflexible/particlesyst.hpp"
#include "simcommon/simmath.hpp"

using namespace rmt;

namespace sim
{

LargeStepSolver:: LargeStepSolver( int inNbp, ParticleSystem* inPSyst )
    :  ParticleSystemSolver( inNbp, inPSyst ),
    mA(NULL),
    m_b(NULL),
    m_z(NULL)
{
    mSolverType = eLargeStepSolver;
    InitParticleSystemSolver();
}

LargeStepSolver::~LargeStepSolver()
{
    delete mA;
    delete[] m_b;
    delete[] m_z;
}

void LargeStepSolver::InitParticleSystemSolver()
{
    static int use_test=1;
    if( use_test == 0 )
        mA = new SparseBlockMatrix(mSolverNbp);
    else if( use_test == 1 )
        mA = new SparseBlockMatrixMonotonous(mSolverNbp);
    else
        rAssert( 0 );
    
    mSfilter.SetNbCol(mSolverNbp);
    
    m_b = new float[mSolverVecSize];
    m_z = new float[mSolverVecSize];
    
    SolverClear();
    
}

void LargeStepSolver::SetConvergenceCriteral(float inEps, int inNbIter)
{
    rAssert(mA);
    mA->SetConvergenceCriteral(inEps,inNbIter);
}

void LargeStepSolver::SolverApplyForces() 
{
    
    mA->SetBlockDiagonal(mPSyst->mMass);
    
    mPSyst->ApplyGravity(m_b);
    
    Condition *C=mPSyst->mConditions->next(); // Skip the fencepost NullCondition
    while (C) 
    {
        C->BuildMatrix(mA, m_b, mPSyst->mTimeStep);     // compute mA and m_b for mTimeStep
        C = C->next();
    }
}


void LargeStepSolver::SolverClear()
{
    mSfilter.Clear();
    mA->ClearMatrix();
    
    ZeroVectorN(m_z, mSolverVecSize);
    ZeroVectorN(m_b, mSolverVecSize); 
    
    int i;
    for ( i=0 ; i< mPSyst->mNbp ; i++ )
    {
        if (mPSyst->mP[i]->mFree == false)
        {
            FixParticle(i);
        }
    }
    
}

//This method is publicaly exposed. LineConstrainParticle, PlaneConstrainParticle, FixParticle 
//and FreeParticle are not.
void LargeStepSolver::ConstrainParticle(int index, int inNbDof, const Vector *in_dir1, const Vector *in_dir2 )
{
    //We can't use this way to constraint particle. This mecanism glue the particle on
    //the contraint. It works as cns==0. 
    //In our case we want a behavior like cns >= 0 
    return;
#if 0
    switch(inNbDof)
    {
    case 0:
        //0Dof. Particle constrained to not move.
        FixParticle(index);
        break;
    case 1:
        //1Dof. Particle constrained to move along a line.
        LineConstrainParticle(index,*in_dir1,*in_dir2);
        break;
    case 2:
        //2Dof. Particle constrained to move on a plane.
        PlaneConstrainParticle(index,*in_dir1);
        break;
    default:
        //3Dof. Free the particle.
        FreeParticle(index);
    }
#endif
}

void LargeStepSolver::LineConstrainParticle(int n, const Vector &in_dir1, const Vector &in_dir2, bool inCnsVel)
{
    pm* block = mSfilter.GetBlock(n);
    
    float dir1[3], dir2[3], tmp[3];
    EqualVector((float*)&in_dir1, dir1);
    EqualVector((float*)&in_dir2, dir2);
    
    float test = NormalizeVector(dir1);
    rAssert(test>MILLI_EPS);
    CrossVector(dir2,dir1,tmp);
    CrossVector(tmp,dir1,dir2);
    test = NormalizeVector(dir2);
    rAssert(test>MILLI_EPS);//dir1 and dir2 are parallel, this case is a candidate for a 2dof constraint.
    
    //At this point dir1 and dir2 are two ortho-normed vector.
    
    float m[3][3];
    MultVectorVectorT(dir1, m);
    m[0][0] = 1.0f - m[0][0];
    m[1][1] = 1.0f - m[1][1];
    m[2][2] = 1.0f - m[2][2];
    EqualMatrix( m, block->k);
    MultVectorVectorT(dir2, m);
    SubMatrix(block->k, m, block->k);
    if (inCnsVel)
    {   
        //Now clean the current velocity of the particle along the constraining direction.
        TransformVector(block->k, mPSyst->mP[n]->m_v);
    }
    block->iszero = false;
}

void LargeStepSolver::PlaneConstrainParticle(int n, const Vector &in_dir, bool inCnsVel)
{
    pm* block = mSfilter.GetBlock(n);
    
    float dir[3];
    EqualVector((float*)&in_dir, dir);
    float test = NormalizeVector(dir);
    rAssert(test>MILLI_EPS);
    
    // built the filter matrix for the constraint
    MultVectorVectorT(dir, block->k);
    block->k[0][0] = 1.0f - block->k[0][0];
    block->k[1][1] = 1.0f - block->k[1][1];
    block->k[2][2] = 1.0f - block->k[2][2];
    if (inCnsVel)
    {   
        //Now clean the current velocity of the particle along the constraining direction.
        TransformVector(block->k, mPSyst->mP[n]->m_v);
    }
    block->iszero = false;
}

void LargeStepSolver::FixParticle(int n, bool inCnsVel/*=true*/)
{
    pm* block = mSfilter.GetBlock(n);
    ZeroMatrix(block->k);
    block->iszero = true;

    if (inCnsVel)
    {
        //Now clean the current velocity of the particle along the constraining direction.
        ZeroVector(mPSyst->mP[n]->m_v);
    }
}

} // sim
