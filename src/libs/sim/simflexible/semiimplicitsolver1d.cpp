
#include "simflexible/semiimplicitsolver1D.hpp"
#include "simflexible/particlesyst.hpp"
#include "simcommon/largesymmetricmatrix.hpp"

namespace sim
{

static bool explicitEuler=false;
static bool filterConstraints=true;

/*
   This class implement the 1D semi-implicit solver method.
   In order to understand what are advantages and disadvantages over other
   methods, we have tested it in different situations.
   First, to really look at what it can delivers we have test it in
   its physical form. This mean whitout any post processing. In the 1D case
   the only post processing is the adjustement for over stretched segements. 
   This post process is turn off.

   Test have been done with the following parameters:
   Five nodes with 2 units spacing. k1df=3675 k1dd=121 dt=5ms (those parameters
   are for 10% of elongation when hanged in the gravity field).

   In this particular case a 5ms time step is the theoritical value that would be good for
   the explicit euler method ( the euler explicit method is one particular case of the
   semi-implicit when W, the filter, is the identity). In euler, we can't tolerate 
   more than about 2 times the theoritical dt. In semi-implicit there is no limit on
   the dt (at least when the number of nodes is not too high. 50 is too many but with 20 
   is still see no limit on the dt). 
   But there is a problem. When we have one fixed point, say the first one, the ellongation
   between the first and the second node is affected by the time step. This is 
   because the force (external and internal) are still applied and the first node and the
   filter distributes it partially over the other nodes. The effect of the 
   gravity, in particular, becomes greater.
   To fix this problem, we add the force of constraint on the fixed node
   and make the force of constraint being part of the filter. 
   We call this: filtering the constraint (surch for filterConstraints flag).   
   When we apply the filter, the force of constraint become partially distributed among 
   all others nodes of the chain and in particular the first segment do not over stretch
   any more. The draw back of this is the fact that the time step can not be made as
   high as we want. Ten time the theoritical euler value becomes the limit.

   What is the best: Filtering the constraint or doing the post-step for overstretching?
   We don't know yet.
   
*/


SemiImplicitSolver1D::SemiImplicitSolver1D(int inNbp, ParticleSystem* inPSyst)
 : ParticleSystemSolver(inNbp, inPSyst)
{
    mSolverType = eSemiImplicitSolver1D;
    InitParticleSystemSolver();
    mImplicitDamping = true;
    SolverClear();
}

SemiImplicitSolver1D::~SemiImplicitSolver1D()
{
    delete mW;
    delete mH;
    delete [] m_b;
    delete [] m_z;
}

void SemiImplicitSolver1D::InitParticleSystemSolver()
{
    mW = new LargeSymmetricMatrix(mSolverNbp);
    mH = new LargeSymmetricMatrix(mSolverNbp);
    m_b  = new float[mSolverVecSize];
    m_z  = new float[mSolverVecSize];
    mWNeedUpdate = true;
}

bool SemiImplicitSolver1D::UpdateW()
{
    bool result=true;
    
    rAssert(mWNeedUpdate==true);
    mWNeedUpdate = false;
    
    mW->SetIdentity();
    float dt = mPSyst->mTimeStep;
    rAssert(mPSyst->mMass[0]);
    
    if (!explicitEuler)
    {
        ComputeH(*mH);
        mH->Scale( dt*dt / mPSyst->mMass[0], *mH );
        mW->Subtract( *mH, *mW );
        result = mW->Invert();
        mH->Scale( mPSyst->mMass[0], *mH );
    }
    
    return result;
}

void SemiImplicitSolver1D::ComputeH(LargeSymmetricMatrix &io_mH) const   
{
    if (mImplicitDamping)
    {
        //We put all damping coefficient to zero to force to not have damping explicitely.
        //Doing so, we use the exact formula (3) as in the DesBruns's paper for the
        //damping. This seems to be uptimal for stability.
        mPSyst->SetOneConditionParameter(0.0f, eFlexStretch1D_d);
        mPSyst->SetOneConditionParameter(0.0f, eFlexStretch2D_d);
        mPSyst->SetOneConditionParameter(0.0f, eFlexBend1D_d);
        mPSyst->SetOneConditionParameter(0.0f, eFlexBend2D_d);
        mPSyst->SetOneConditionParameter(0.0f, eFlexShear2D_d);
    }
    
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

void SemiImplicitSolver1D::SolverApplyForces()
{
    int i;
    if (mWNeedUpdate)
    {
        if ( UpdateW() == false )
            return;
    }
    
    //Compute internal forces on each particles.
    Condition *C=mPSyst->mConditions->next(); // Skip the fencepost NullCondition
    while (C) 
    {
        C->BuildMatrix(NULL, m_b, mPSyst->mTimeStep);
        C = C->next();
    }
    
    
    if (filterConstraints)
    {
        mPSyst->ApplyGravity(m_b); // This add the gravity force*dt into m_b
        for ( i=0 ; i<mSolverNbp ; i++ )
        {
            
            if (!mPSyst->mP[i]->mFree)
            {
                ScaleVector(m_b+3*i, -1.0f, m_b+3*i); //Add the constraint on each fixed particle.
            }
        }
        if (mImplicitDamping)
        {
            //Here we use the exact formula (3) as in the DesBruns's paper.
            
            for( i=0 ; i<mSolverNbp ; i++ )
            {
                EqualVector(mPSyst->mP[i]->m_v, mPSyst->mDv+3*i);
            }
            mH->PreMultiply3x(mPSyst->mDv,m_z);
            for( i=0 ; i<mSolverNbp ; i++ )
            {
                AddVector(m_b+3*i, m_z+3*i, m_b+3*i);
            }
        }
        
        //Filter the forces:
        mW->PreMultiply3x(m_b,m_z); // Here we get in m_z = W^1( fin + fext )dt
    }
    else
    {
        if (mImplicitDamping)
        {
            //Here we use the exact formula (3) as in the DesBruns's paper.
            int i;
            for( i=0 ; i<mSolverNbp ; i++ )
            {
                EqualVector(mPSyst->mP[i]->m_v, mPSyst->mDv+3*i);
            }
            mH->PreMultiply3x(mPSyst->mDv,m_z);
            for( i=0 ; i<mSolverNbp ; i++ )
            {
                AddVector(m_b+3*i, m_z+3*i, m_b+3*i);
            }
        }
        
        //Because the unit vector is an eigenvector of W with eigenvalue of one, 
        //we have rigth to apply the filter after the gravity. 
        //That is W*(G+F) = W*G + W*F = gW*1 + W*F = g + W*F 
        mPSyst->ApplyGravity(m_b); // This add the gravity force*dt into m_b
        mW->PreMultiply3x(m_b,m_z); // Here we get in m_z = W^1( fin + fext )dt
    }
    
}

void SemiImplicitSolver1D::SolverAdvanceSystem(float* io_dv)
{
    int i,i3;
    //Integrate speeds and positions
    for ( i=0,i3=0 ; i<mSolverNbp ; i++, i3=3*i)
    {
        if (mPSyst->mMass[i] )
        {
            ScaleVector(m_z+i3, 1/mPSyst->mMass[i], io_dv+i3);      // Here mDv represents the delta speed of each particles.
        }
    }
}

void SemiImplicitSolver1D::SolverClear()
{
    ZeroVectorN(m_b, mSolverVecSize);
}

} // sim