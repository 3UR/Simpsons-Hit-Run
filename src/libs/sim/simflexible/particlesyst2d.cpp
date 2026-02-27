
#include "simflexible/particlesystxd.hpp"
#include "simflexible/condition1d.hpp"
#include "simflexible/condition2d.hpp"
#include "simcommon/dline2.hpp"
#include "pddi/pddi.hpp"

using namespace RadicalMathLibrary;

namespace sim
{

ParticleSystem2D::ParticleSystem2D(int r, int c, float l) 
: ParticleSystem(r*c, l),
  mNbi(r),
  mNbj(c),
  mDelj(l)
{
    // need to set particle normal and uv to display...
    // sv = new pddiVertex_NT[mNbp];
    
    mMaxDef = 0.1f;
    
    mShader = 0;
    
    int i, j;
    
    // these triangles are used for display
    mNbTri = 2*(mNbi-1)*(mNbj-1);
    mTriIndex = new unsigned short[mNbTri*3];
    mTriNormals = new float[mNbTri*3];
    
    // Set initial positions
    FlexParticle* pp;
    mMapIndex = new intp[mNbi];
    for(i=0; i<mNbi; i++) 
    {
        mMapIndex[i] = new int[mNbj];
        
        for(j=0; j<mNbj; j++) 
        {
            int k = i * mNbj + j;
            mMapIndex[i][j] = k;
            pp = mP[k];
            
            pp->m_x[0] = mDelj * j;
            pp->m_x[1] = 0;
            pp->m_x[2] = mDeli * i;
            
            /*
            sv[k].uv0.u = float(j)/float(mNbj);
            sv[k].uv0.v = float(i)/float(mNbi);
            sv[k].uv1.u = float(j)/float(mNbj);
            sv[k].uv1.v = float(i)/float(mNbi);
            */
        }
    }
    
    int index = 0;
    for(i=0;i<mNbi-1;i++)
    {
        for(j=0;j<mNbj-1;j++) 
        {
            mTriIndex[index++] = mMapIndex[i][j];
            mTriIndex[index++] = mMapIndex[i+1][j];
            mTriIndex[index++] = mMapIndex[i][j+1];
            
            mTriIndex[index++] = mMapIndex[i+1][j+1];
            mTriIndex[index++] = mMapIndex[i][j+1];
            mTriIndex[index++] = mMapIndex[i+1][j];
        }
    }
    
    PreDisplay();
}

ParticleSystem2D::~ParticleSystem2D() 
{
    delete mTriIndex;
    //   delete sv;
    SetShader(0);
    
    for(int i=0; i<mNbi; i++)
        delete mMapIndex[i];
    delete mMapIndex;
}

void ParticleSystem2D::GetExtension(float& u, float& v)
{
    u=v=0;
}

void ParticleSystem2D::Scale(float f)
{
    mDelj *= f;
    ParticleSystem::Scale(f);
}

void ParticleSystem2D::PreStepModification()
{
    static int maxiter = 20;
    int n = 0;
    bool moved = true;
    
    while (moved && n < maxiter)
    {
        moved = false;
        for(int i=0;i<mNbi-1;i++)
        {
            for(int j=0;j<mNbj-1;j++) 
            {
                FlexParticle* p00 = P(i, j);
                FlexParticle* p01 = P(i, j+1);
                FlexParticle* p10 = P(i+1, j);
                FlexParticle* p11 = P(i+1, j+1);
                if (TestExtension1D(p00, p10, mDeli, mMaxDef))
                {
                    moved = true;
                }
                if (TestExtension1D(p00, p01, mDelj, mMaxDef))
                {
                    moved = true;
                }
                if (j == mNbj-2 && TestExtension1D(p01, p11, mDeli, mMaxDef))
                {
                    moved = true;
                }
                if (i == mNbi-2 && TestExtension1D(p10, p11, mDelj, mMaxDef))
                {
                    moved = true;
                }
            }
        }
        n++;
    }
    
}

void ParticleSystem2D::BuildConditions() 
{
    int i,j;
    float area = mDeli*mDelj/2.0f;
    
    // Build all the conditions we need
    eParticleSystemSolverType solver = mPSystSolver->GetSolverType();
    switch (solver)
    {
    case eLargeStepSolver:
        {
            FlexParticle *p00, *p01, *p10, *p11; 
            for(i=0;i<mNbi-1;i++)
            {
                for(j=0;j<mNbj-1;j++) 
                {
                    p00 = P(i, j);
                    p01 = P(i, j+1);
                    p10 = P(i+1, j);
                    p11 = P(i+1, j+1);
                    mConditions->Insert( new Stretch1DCondition(p00, p01, mCparams, mDelj));
                    mConditions->Insert( new Stretch1DCondition(p00, p10, mCparams, mDeli));
                    mConditions->Insert( new Wind2DCondition(p01, p00, p10, mCparams, &mWind, area));
                    mConditions->Insert( new Wind2DCondition(p10, p11, p01, mCparams, &mWind, area));
                    mConditions->Insert( new Shear2DCondition(p01, p00, p10, mCparams));
                    mConditions->Insert( new Shear2DCondition(p10, p11, p01, mCparams));
                    mConditions->Insert( new Bend2DCondition(p00, p01, p10, p11, mCparams));
                    if (i == mNbi-2)
                    {
                        mConditions->Insert( new Stretch1DCondition(p10, p11, mCparams, mDelj));
                    }
                    else
                    {
                        mConditions->Insert( new Bend2DCondition(p01, p11, p10, P(i+2, j), mCparams));
                    }
                    if (j == mNbj-2)
                    {
                        mConditions->Insert( new Stretch1DCondition(p01, p11, mCparams, mDeli));
                    }
                    else
                    {
                        mConditions->Insert( new Bend2DCondition(p10, p11, p01, P(i, j+2), mCparams));
                    }
                }
            }
        }
        break;
        
    case eSemiImplicitSolver1D:
        rAssertMsg(0,"Invalid solver");
        break;
        
    case eSemiImplicitSolver2D:
        {
            FlexParticle* p00, *p01, *p10, *p11, *p02, *p20, *p22, *p12, *p21;
            for(i=0;i<mNbi-1;i++)
            {
                for(j=0;j<mNbj-1;j++) 
                {
                    p00 = P(i, j);
                    p01 = P(i, j+1);
                    p10 = P(i+1, j);
                    p11 = P(i+1, j+1);
                    mConditions->Insert( new SpringConditionForStretch(p00, p01, mCparams, mDelj));
                    mConditions->Insert( new SpringConditionForStretch(p00, p10, mCparams, mDeli));
                    if (i == mNbi-2)
                    {
                        mConditions->Insert( new SpringConditionForStretch(p10, p11, mCparams, mDelj));
                    }
                    if (j == mNbj-2)
                    {
                        mConditions->Insert( new SpringConditionForStretch(p01, p11, mCparams, mDeli));
                    }
                }
            }
            
            float lenth0 = rmt::Sqrt(mDeli*mDeli+mDelj*mDelj);
            for(i=0;i<mNbi-1;i++)
            {
                for(j=0;j<mNbj-1;j++) 
                {
                    p00 = P(i, j);
                    p01 = P(i, j+1);
                    p10 = P(i+1, j);
                    p11 = P(i+1, j+1);
                    mConditions->Insert( new SpringConditionForShear(p00, p11, mCparams, lenth0));
                    mConditions->Insert( new SpringConditionForShear(p10, p01, mCparams, lenth0));
                }
            }
            
            for( i=0;i<mNbi-2;i++)
            {
                for( j=0;j<mNbj-2;j++) 
                {
                    p00 = P(i, j);
                    p01 = P(i, j+1);
                    p02 = P(i, j+2);
                    p10 = P(i+1, j);
                    p12 = P(i+1, j+2);
                    p20 = P(i+2, j);
                    p21 = P(i+2, j+1);
                    p22 = P(i+2, j+2);
                    mConditions->Insert( new Bend1DCondition(p00, p01, p02, mCparams, false));
                    mConditions->Insert( new Bend1DCondition(p20, p21, p22, mCparams, false));
                    mConditions->Insert( new Bend1DCondition(p00, p10, p20, mCparams, false));
                    mConditions->Insert( new Bend1DCondition(p02, p12, p22, mCparams, false));
                }
            }
            i = mNbi-1;
            for( j=0 ; j<mNbj-2 ; j++ )
            {
                p00 = P(i, j);
                p01 = P(i, j+1);
                p02 = P(i, j+2);
                mConditions->Insert( new Bend1DCondition(p00, p01, p02, mCparams, false));
            }
            j = mNbj-1;
            for( i=0 ; i<mNbi-2 ; i++ )
            {
                p00 = P(i, j);
                p10 = P(i+1, j);
                p20 = P(i+2, j);
                mConditions->Insert( new Bend1DCondition(p00, p10, p20, mCparams, false));
            }
            
            mPSystSolver->SetNeedPrecomputation(true);
        }
        break;
        
    case eKinematicSolver:
    default:
        rAssertMsg(0,"Solver type must be set before to call BuildCondition or invalid sovlver type for this kind of particle system.");
        break;
   }
   
}

void ParticleSystem2D::PreDisplay()
{
    int i;
    /*
    for(i=0; i<mNbp; i++) 
    {
    sv[i].n.Set(0, 0, 0);
    }
    */
    for(i=0; i<mNbTri; i++) 
    {
        int j = 3*i;
        float* n = &mTriNormals[j];
        unsigned short* k = &mTriIndex[j];
        
        ComputeTriNormal(mP[k[0]]->m_x, mP[k[1]]->m_x, mP[k[2]]->m_x, n);
        /*
        sv[k[0]].n.x += n[0];
        sv[k[0]].n.y += n[1];
        sv[k[0]].n.z += n[2];
        
          sv[k[1]].n.x += n[0];
          sv[k[1]].n.y += n[1];
          sv[k[1]].n.z += n[2];
          
            sv[k[2]].n.x += n[0];
            sv[k[2]].n.y += n[1];
            sv[k[2]].n.z += n[2];
        */
    }
    
    for(i=0; i<mNbp; i++) 
    {
        //      sv[i].v.Set(mP[i]->m_x[0], mP[i]->m_x[1], mP[i]->m_x[2]);
        Vector tmp;
        //      tmp.Set(sv[i].n.x, sv[i].n.y, sv[i].n.z);
        tmp.Normalize();
        
        //      sv[i].n.Set(tmp.x, tmp.y, tmp.z);
    }
    
}

void ParticleSystem2D::SetShader(pddiShader* shad)
{
    if (mShader)
        mShader->Release();
    
    mShader = shad;
    
    if (mShader)
        mShader->AddRef();
}

void ParticleSystem2D::Display() 
{
    tColour col(200, 0, 100);
    
    DrawLineToggler toggler;

    Condition *C=mConditions->next(); // Skip the fencepost NullCondition
    while (C) 
    {
        dStreamLine(*((rmt::Vector*)C->mPi->m_x), *((rmt::Vector*)C->mPj->m_x), col, col);
        C = C->next();
    }
}

bool ParticleSystem2D::GetIntermediateParticle(float pos[])
{
    mInterpList.RemoveAll();
    if (mInteri >= mNbi-1)
    {
        if (mInterj >= mNbj-2)
        {
            return false;
        }
        else
        {
            mInteri = 0;
            mInterj++;
        }
    }
    FlexParticle* p00 = P(mInteri, mInterj);
    FlexParticle* p01 = P(mInteri, mInterj+1);
    FlexParticle* p10 = P(mInteri+1, mInterj);
    FlexParticle* p11 = P(mInteri+1, mInterj+1);
    
    if (!p00->mFree || !p10->mFree || !p01->mFree || !p11->mFree)
    {
        mInteri++;
        return GetIntermediateParticle(pos);
    }
    else
    {
        mInterpList.Add(p00);
        mInterpList.Add(p01);
        mInterpList.Add(p10);
        mInterpList.Add(p11);
        
        AddVector(p00->m_x, p01->m_x, pos);
        AddVector(p10->m_x, pos, pos);
        AddVector(p11->m_x, pos, pos);
        ScaleVector(pos, 0.25f, pos);
        mInteri++;
        return true;
    }
}

} // sim