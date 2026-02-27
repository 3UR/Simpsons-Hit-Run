
#include "simflexible/particlesystxd.hpp"
#include "simflexible/condition1d.hpp"
#include "simcommon/dline2.hpp"

using namespace RadicalMathLibrary;

namespace sim
{

ParticleSystem1D::ParticleSystem1D(int n, float l) 
: ParticleSystem(n, l)
{
    // Set initial positions
    for(int i=0; i<mNbp; i++) 
    {
        FlexParticle* pp = mP[i];
        pp->m_x[0] = mDeli * float(i);
        pp->m_x[1] = 0;
        pp->m_x[2] = 0;
    }
    
    mMaxDef = 0.1f;
}

ParticleSystem1D::~ParticleSystem1D() 
{
};

void ParticleSystem1D::GetExtension(float& u, float& v)
{
    u=v=0;
    int i;
    float dp[3];
    for(i=0; i<mNbp-1; i++)
    {
        SubVector(mP[i+1]->m_x, mP[i]->m_x, dp);
        u += LenVector(dp);
    }
    float len = (mNbp-1) * mDeli;
    u = (u-len)/len;
}

void ParticleSystem1D::PreStepModification()
{
    
    static int maxiter = 10;
    int i, n = 0;
    bool moved = true;
    
    while (moved && n < maxiter)
    {
        moved = false;
        for( i=0; i<mNbp-1; i++)
        {
            if (!mP[i]->mBroken)
            {
                if (TestExtension1D(mP[i], mP[i+1], mDeli, mMaxDef))
                {
                    moved = true;
                }
            }
        }
        n++;
    }
    
}

void ParticleSystem1D::BuildConditions() 
{
    int i;
    
    // Build all the conditions we need
    switch (mPSystSolver->GetSolverType())
    {
    case eLargeStepSolver:
        {
            for( i=0;i<mNbp-1;i++)
            {
                mConditions->Insert( new Stretch1DCondition(mP[i], mP[i+1], mCparams, mDeli));
                mConditions->Insert( new Wind1DCondition(mP[i], mP[i+1], mCparams, &mWind));
            }
            
            if (mNbp > 2 )
            {
                for( i=0;i<mNbp-2;i++)
                {
                    mConditions->Insert( new Bend1DCondition(mP[i], mP[i+1], mP[i+2], mCparams) );
                }
            }
        }
        break;
        
    case eSemiImplicitSolver1D:
    case eKinematicSolver:
        {
            for( i=0;i<mNbp-1;i++)
            {
                mConditions->Insert( new SpringConditionForStretch(mP[i], mP[i+1], mCparams, mDeli));
            }
            
            if (mPSystSolver->GetSolverType()!=eKinematicSolver)
            {
                if (mNbp > 2)
                {
                    for(int i=0;i<mNbp-2;i++)
                    {
                        mConditions->Insert( new Bend1DCondition(mP[i], mP[i+1], mP[i+2], mCparams, false));
                    }
                }
            }
            mPSystSolver->SetNeedPrecomputation(true);
        }
        break;
        
    case eSemiImplicitSolver2D:
        rAssertMsg(0,"Invalid solver");
        break;
        
    default:
        rAssertMsg(0,"Solver type must be set before to call BuildCondition or invalid sovlver type for this kind of particle system.");
        break;
    }
}

void ParticleSystem1D::Display() 
{
    tColour col(200, 0, 100);

    DrawLineToggler toggler;

    for (int i=0; i<mNbp-1; i++)
    {
        if (!mP[i]->mBroken)
            dStreamLine(*((rmt::Vector*)mP[i]->m_x), *((rmt::Vector*)mP[i+1]->m_x), col, col);
    }
    
    if (mDrawparticle)
        ParticleSystem::Display();
}

bool ParticleSystem1D::GetIntermediateParticle(float pos[])
{
    mInterpList.RemoveAll();
    if (mInteri < mNbp-1)
    {
        if (!mP[mInteri]->mFree || !mP[mInteri+1]->mFree)
        {
            mInteri++;
            return GetIntermediateParticle(pos);
        }
        else
        {
            mInterpList.Add(mP[mInteri]);
            mInterpList.Add(mP[mInteri+1]);
            
            AddVector(mP[mInteri]->m_x, mP[mInteri+1]->m_x, pos);
            mInteri++;
            ScaleVector(pos, 0.5f, pos);
            return true;
        }
    }
    else
        return false;
}

void ParticleSystem1D::BreakAt(int i)
{
    if (i > 1 && i < mNbp - 2)
    {
        mP[i]->mBroken = true;
        Condition *C=mConditions;
        while (C && C->next()) 
        {
            // we have to use the next here since the condition is single linked list.
            if (  C->next()->RelateToParticle(mP[i]) && 
                C->next()->RelateToParticle(mP[i+1])
                )
            {
                C->RemoveNext();
            }
            else
            {
                C = C->next();
            }
        }
    }
}

} // sim
