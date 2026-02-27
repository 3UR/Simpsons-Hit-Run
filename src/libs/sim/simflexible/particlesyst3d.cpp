
#include "simflexible/particlesystxd.hpp"
#include "simflexible/condition1d.hpp"
#include "simflexible/condition2d.hpp"
#include "simcommon/dline2.hpp"

using namespace RadicalMathLibrary;


ParticleSystem3D::ParticleSystem3D(int a, int b, int c, float l) 
:  ParticleSystem(a*b*c, l),
	mNbi(a),
	mNbj(b),
   mNbk(c),
	mDelj(l),
   mDelk(l)
{
   mMaxDef = 0.5f;

   FlexParticle* pp;
   int mNbij = mNbi*mNbj;

   mMapIndex = new intpp[mNbi];

   // Set initial positions
   for(int i=0; i<mNbi; i++) 
   {
      mMapIndex[i] = new intp[mNbj];

      for(int j=0; j<mNbj; j++) 
      {
         mMapIndex[i][j] = new int[mNbk];

         for(int k=0; k<mNbk; k++) 
         {
            int m = i + j*mNbi + k*mNbij;

            mMapIndex[i][j][k] = m;
            pp = mP[m];
         
            pp->m_x[0] = mDeli * i;
            pp->m_x[1] = mDelj * j;
            pp->m_x[2] = mDelk * k;
         }
      }
   }

   mDiagonalLength = Sqrt(Sqr(mDeli) + Sqr(mDelj) + Sqr(mDelk));

   PreDisplay();
}

ParticleSystem3D::~ParticleSystem3D() 
{
   for(int i=0; i<mNbi; i++) 
   {
      for(int j=0; j<mNbj; j++) 
      {
         delete mMapIndex[i][j];
      }
      delete mMapIndex[i];
   }
   delete mMapIndex;
}

void ParticleSystem3D::GetExtension(float& u, float& v)
{
   u=v=0;
}

void ParticleSystem3D::Scale(float f)
{
   mDelj *= f;
   mDelk *= f;
   ParticleSystem::Scale(f);
}

void ParticleSystem3D::PreStepModification()
{
   static int maxiter = 20;
   int n = 0;
   bool moved = true;

   while (moved && n < maxiter)
   {
      moved = false;
      Condition *C=mConditions->next(); // Skip the fencepost NullCondition
      while (C) 
      {
         if (TestExtension1D(C->mPi, C->mPj, ((Stretch1DCondition*)C)->Dl(), mMaxDef))
         {
            moved = true;
         }
         C = C->next();
      }
      maxiter++;
   }
}

void ParticleSystem3D::BuildConditions() 
{
   float diagij = Sqrt(Sqr(mDeli) + Sqr(mDelj));

   for(int i=0;i<mNbi-1;i++)
   {
      for(int j=0;j<mNbj-1;j++) 
      {
         for(int k=0;k<mNbk-1;k++) 
         {
            FlexParticle* p000 = P(i, j, k);
            FlexParticle* p001 = P(i, j, k+1);
            FlexParticle* p010 = P(i, j+1, k);
            FlexParticle* p011 = P(i, j+1, k+1);
            FlexParticle* p100 = P(i+1, j, k);
            FlexParticle* p101 = P(i+1, j, k+1);
            FlexParticle* p110 = P(i+1, j+1, k);
            FlexParticle* p111 = P(i+1, j+1, k+1);

            mConditions->Insert( new Stretch1DCondition(p000, p001, mCparams, mDelk));
            mConditions->Insert( new Stretch1DCondition(p000, p010, mCparams, mDelj));
            mConditions->Insert( new Stretch1DCondition(p000, p100, mCparams, mDeli));

            mConditions->Insert( new Stretch1DCondition(p000, p110, mCparams, diagij));
            mConditions->Insert( new Stretch1DCondition(p000, p011, mCparams, diagij));
            mConditions->Insert( new Stretch1DCondition(p000, p101, mCparams, diagij));

            //mConditions->Insert( new Shear2DCondition(p010, p000, p100, mCparams)); // ij
            //mConditions->Insert( new Shear2DCondition(p100, p110, p010, mCparams));

            //mConditions->Insert( new Shear2DCondition(p000, p001, p101, mCparams)); // ik
            //mConditions->Insert( new Shear2DCondition(p101, p100, p000, mCparams));

            //mConditions->Insert( new Shear2DCondition(p010, p000, p001, mCparams)); // jk
            //mConditions->Insert( new Shear2DCondition(p001, p011, p010, mCparams));

            mConditions->Insert( new Stretch1DCondition(p000, p111, mCparams, mDiagonalLength));
            mConditions->Insert( new Stretch1DCondition(p010, p101, mCparams, mDiagonalLength));
            mConditions->Insert( new Stretch1DCondition(p001, p110, mCparams, mDiagonalLength));
            mConditions->Insert( new Stretch1DCondition(p100, p011, mCparams, mDiagonalLength));
            

            if (i == mNbi-2)
            {
               mConditions->Insert( new Stretch1DCondition(p100, p101, mCparams, mDeli));
               mConditions->Insert( new Stretch1DCondition(p100, p110, mCparams, mDeli));
               mConditions->Insert( new Stretch1DCondition(p100, p111, mCparams, diagij));

               //mConditions->Insert( new Shear2DCondition(p110, p100, p101, mCparams)); // jk
               //mConditions->Insert( new Shear2DCondition(p101, p111, p110, mCparams));


               if (j == mNbj-2)
               {
                  mConditions->Insert( new Stretch1DCondition(p110, p111, mCparams, mDeli));
               }
               if (k == mNbk-2)
               {
                  mConditions->Insert( new Stretch1DCondition(p101, p111, mCparams, mDeli));
               }
            }
            if (j == mNbj-2)
            {
               mConditions->Insert( new Stretch1DCondition(p010, p110, mCparams, mDeli));
               mConditions->Insert( new Stretch1DCondition(p010, p011, mCparams, mDeli));
               mConditions->Insert( new Stretch1DCondition(p010, p111, mCparams, diagij));

               //mConditions->Insert( new Shear2DCondition(p010, p011, p111, mCparams)); // ik
               //mConditions->Insert( new Shear2DCondition(p111, p110, p010, mCparams));
               
               if (k == mNbk-2)
               {
                  mConditions->Insert( new Stretch1DCondition(p011, p111, mCparams, mDeli));
               }
            }
            if (k == mNbk-2)
            {
               mConditions->Insert( new Stretch1DCondition(p001, p011, mCparams, mDeli));
               mConditions->Insert( new Stretch1DCondition(p001, p101, mCparams, mDeli));

               mConditions->Insert( new Stretch1DCondition(p001, p111, mCparams, diagij));
               //mConditions->Insert( new Shear2DCondition(p011, p001, p101, mCparams)); // ij
               //mConditions->Insert( new Shear2DCondition(p101, p111, p011, mCparams));
            }
         }
      }
   }
}

void ParticleSystem3D::PreDisplay()
{
}

void ParticleSystem3D::Display() 
{
   tColour color(100, 0, 200);

   Condition *C=mConditions->next(); // Skip the fencepost NullCondition
   while (C) 
   {
      DrawLine(C->mPi->m_x, C->mPj->m_x, color);
      C = C->next();
   }
}

bool ParticleSystem3D::GetIntermediateParticle(float pos[])
{
   return false;

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
   rAssert(false);
   return false;
}
