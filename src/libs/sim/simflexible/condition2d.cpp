
#include "simflexible/condition2d.hpp"
#include "simcommon/sbmatrix.hpp"
#include "simflexible/flexparticle.hpp"

using namespace RadicalMathLibrary;

namespace sim
{

float Stretch2DCondition::dpji[3];
float Stretch2DCondition::dpki[3];
float Stretch2DCondition::Lu;
float Stretch2DCondition::Lv;


Stretch2DCondition::Stretch2DCondition(   FlexParticle *mPi, FlexParticle *mPj, FlexParticle *pkk, 
    ConditionParams* p, bool inUsePartialDerivative)
    : Condition(mPi,mPj,p)
{
    //   rAssert(false); // need to be fixed
    mUsePartialDerivative = inUsePartialDerivative;
    
    bu = 1;
    pk = pkk;
    bu = bv = 1.0f;
    a = 10.0f*10.0f/2.0f;//(0.1f*0.1f);
    
    float ddu1, ddv1, ddu2, ddv2;
    float dpji[3], dpki[3];
    SubVector(mPj->m_x, mPi->m_x, dpji);
    SubVector(pk->m_x, mPi->m_x, dpki);
    
    ddu1 = LenVector(dpji);
    ddv1 = LenVector(dpki);
    a = ddu1*ddv1/2;
    
    ddv1 = 0.0f;
    
    ScaleVector(dpji, 1/ddu1, dpji);
    ddu2 = DotVector(dpki, dpji);
    ScaleVector(dpji, ddu2, dpji);
    SubVector(dpki,dpji,dpki);
    ddv2 = LenVector(dpki);
    
    // get the inverse of the du dv matrix so du1 becomes invdu1 and so on
    float den = ddv1*ddu2-ddv2*ddu1; // should not be 0
    du1 = -ddv2/den;
    du2 =  ddv1/den;
    dv1 =  ddu2/den;
    dv2 = -ddu1/den;
    
    bu=bv=0.0f;
    C2DVal cnd;
    float atmp = a;
    a=1.0f;
    
    Cond(cnd);
    bu = cnd.u;
    bv = cnd.v;
    a  = atmp;
}

Stretch2DCondition::Stretch2DCondition(   FlexParticle *mPi, FlexParticle *mPj, FlexParticle *pkk, 
    ConditionParams* p,
    float ddu1, float ddv1, float ddu2, float ddv2, bool inUsePartialDerivative)
    : Condition(mPi,mPj,p) 
{
    rAssert(false); // need to be fixed
    mUsePartialDerivative = inUsePartialDerivative;
    
    bu = 1;
    pk = pkk;
    bu = bv = 1.0f;
    a = 10.0f*10.0f/2.0f;//(0.1f*0.1f);
    
    // get the inverse of the du dv matrix so du1 becomes invdu1 and so on
    float den = ddv1*ddu2-ddv2*ddu1; // should not be 0
    du1 = -ddv2/den;
    du2 =  ddv1/den;
    dv1 =  ddu2/den;
    dv2 = -ddu1/den;
}

float Stretch2DCondition::Cond() 
{
    rAssert(0); // Should not be called for stretch.
    return 0;
}

void Stretch2DCondition::Cond(C2DVal& C) 
{
    SubVector(mPj->m_x, mPi->m_x, dpji);
    SubVector(pk->m_x, mPi->m_x, dpki);
    
    float Wu[3], Wv[3];
    
    AddLinearVector(dpji, du1, dpki, du2, Wu);
    AddLinearVector(dpji, dv1, dpki, dv2, Wv);
    
    Lu = LenVector(Wu);
    Lv = LenVector(Wv);
    
    C.u = a*(Lu - bu);
    C.v = a*(Lv - bv);
}

void Stretch2DCondition::Derivative(const float c0, const float L, const float d1, const float d2, float dCi[], float dCj[], float dCk[]) 
{
    if (c0 == 0)
    {
        ZeroVector(dCk);
        ZeroVector(dCj);
        ZeroVector(dCi);
    }
    else
    {
        float aL = a/L;
        float ji = -(2.0f*Sqr(d1) + d1*d2) * aL;
        float ki = -(2.0f*Sqr(d2) + d1*d2) * aL;
        AddLinearVector(dpji, ji, dpki, ki, dCi);
        
        ji = (2.0f*Sqr(d1) + 0    ) * aL;
        ki = (0            + d1*d2) * aL;
        AddLinearVector(dpji, ji, dpki, ki, dCj);
        
        ji = (0            + d1*d2) * aL;
        ki = (2.0f*Sqr(d2) + 0    ) * aL;
        AddLinearVector(dpji, ji, dpki, ki, dCk);
    }
}

void Stretch2DCondition::Derivative2(float *x1, float *x2, const C2DVal& dC1p2, C2DVal& ddC, int ind1, int ind2)
{
}

void Stretch2DCondition::BuildMatrix(SparseBlockMatrix *A, float *b, float dt) 
{
    // The stretch condition has 3 relevant vertices we need to consider,
    
    // First: Forces (f)
    // fp = -mParams->Stretch2Dkf * C * dC/dxp, for p = i, j, k
    
    // Second: the derivative matrix (F)
    // Fpq = -mParams->Stretch2Dkf * ( dC/dxp * dC/dxq + d2C/dxpdxq * C ), for pq = ij, jk, ki
    // NB: Fpq is symmetric: Fpq == Fqp = dfp/fxq == dfq/dxp
    
    // Third: Damping force (g)
    // gp = -mParams->Stretch2Dkd * dC/dxp * dC/dt
    
    // Fourth: the damping derivative matrix (G)
    // Gqp = -mParams->Stretch2Dkd * d2C/dxpdxq * dC/dt
    // NB: Gpq is symmetric: Gpq == Gqp = -mParams->Stretch2Dkd * dgp/fxq == -mParams->Stretch2Dkd * dgq/dxp
    
    // Fifth: the H = dG/dv = -mParams->Stretch2Dkd * dC/dxp * dC/dxq
    // Which is also symmetric
    
    // since the derivative are used again and again, we pre-compute them all
    C2DVal C0;                     // initial
    Cond(C0);
    
    // first derivate in x, y, z
    C2DVect dCi, dCj, dCk;
    Derivative(C0.u, Lu, du1, du2, dCi.u, dCj.u, dCk.u);
    Derivative(C0.v, Lv, dv1, dv2, dCi.v, dCj.v, dCk.v);
    
    // second derivate in x, y, z for i and j etc.
    /*
    C2DMat d2Cij, d2Cik, d2Cjk;
    for(int ind1=0; ind1 < 3; ind1++) 
    {
    for(int ind2=0; ind2<3; ind2++) 
    {
    Derivative2(pi->x, pj->x, dCj.v[ind2], d2Cij.v[ind1].v[ind2], ind1, ind2);
    Derivative2(pi->x, pk->x, dCk.v[ind2], d2Cik.v[ind1].v[ind2], ind1, ind2);
    Derivative2(pj->x, pk->x, dCk.v[ind2], d2Cjk.v[ind1].v[ind2], ind1, ind2);
    }
    }
    */
    
    // build the dC/dt
    C2DVal Cdot;
    Cdot.u  = DotVector(mPi->m_v, dCi.u);
    Cdot.u += DotVector(mPj->m_v, dCj.u);
    Cdot.u += DotVector(pk->m_v, dCk.u);
    Cdot.v  = DotVector(mPi->m_v, dCi.v);
    Cdot.v += DotVector(mPj->m_v, dCj.v);
    Cdot.v += DotVector(pk->m_v, dCk.v);
    
    // add the force add the damping in f
    float fi[3], fj[3], fk[3];
    AddLinearVector(dCi.u, -mParams->Stretch2Dkf*C0.u, dCi.u, -mParams->Stretch2Dkd*Cdot.u, fi);
    AddAddLinearVector(dCi.v, -mParams->Stretch2Dkf*C0.v, dCi.v, -mParams->Stretch2Dkd*Cdot.v, fi);
    
    AddLinearVector(dCj.u, -mParams->Stretch2Dkf*C0.u, dCj.u, -mParams->Stretch2Dkd*Cdot.u, fj);
    AddAddLinearVector(dCj.v, -mParams->Stretch2Dkf*C0.v, dCj.v, -mParams->Stretch2Dkd*Cdot.v, fj);
    
    AddLinearVector(dCk.u, -mParams->Stretch2Dkf*C0.u, dCk.u, -mParams->Stretch2Dkd*Cdot.u, fk);
    AddAddLinearVector(dCk.v, -mParams->Stretch2Dkf*C0.v, dCk.v, -mParams->Stretch2Dkd*Cdot.v, fk);
    
#if defined(RAD_DEBUG) 
    AddVector(fi, mPi->m_fs, mPi->m_fs);
    AddVector(fj, mPj->m_fs, mPj->m_fs);
    AddVector(fk, pk->m_fs, pk->m_fs);
#endif
    
    if(mUsePartialDerivative)
    {
    /*
    rAssertMsg(0,"This is not implemented","");
    //*************** according to the paper, these are symmetric.. (since Kij and d2Cij are sym)
    float dCidCj[3][3], dCidCk[3][3], dCjdCk[3][3];
    vC_Mult_vCT(dCi, dCj, dCidCj);
    vC_Mult_vCT(dCi, dCk, dCidCk);
    vC_Mult_vCT(dCj, dCk, dCjdCk);
    
      // second derivate in x, y, z for i and j etc.
      C2DMat d2Cij, d2Cik, d2Cjk;
      for(int ind1=0; ind1 < 3; ind1++) 
      {
      for(int ind2=0; ind2<3; ind2++) 
      {
      //            Derivative2(mPi->m_x, mPj->m_x, dCj.v[ind2], d2Cij.v[ind1].v[ind2], ind1, ind2);
      //            Derivative2(mPi->m_x, pk->m_x, dCk.v[ind2], d2Cik.v[ind1].v[ind2], ind1, ind2);
      //            Derivative2(mPj->m_x, pk->m_x, dCk.v[ind2], d2Cjk.v[ind1].v[ind2], ind1, ind2);
      }
      }
      
        // compute the K, which should also be symmetric 
        float Kij[3][3], Kik[3][3], Kjk[3][3];
        mC_Mult_C(d2Cij, C0, Kij);
        mC_Mult_C(d2Cik, C0, Kik);
        mC_Mult_C(d2Cjk, C0, Kjk);
        AddMatrix(Kij, dCidCj, Kij);
        AddMatrix(Kik, dCidCk, Kik);
        AddMatrix(Kjk, dCjdCk, Kjk);
        ScaleMatrix(Kij, -mParams->Stretch2Dkf, Kij);
        ScaleMatrix(Kik, -mParams->Stretch2Dkf, Kik);
        ScaleMatrix(Kjk, -mParams->Stretch2Dkf, Kjk);
        
          // add the Kd contribution to K
          C2DVal Cdot; 
          float m1[3][3];
          mC_Mult_C(d2Cij, Cdot, m1);
          AddScaleMatrix(Kij, m1, -mParams->Stretch2Dkd, Kij);
          mC_Mult_C(d2Cik, Cdot, m1);
          AddScaleMatrix(Kik, m1, -mParams->Stretch2Dkd, Kik);
          mC_Mult_C(d2Cjk, Cdot, m1);
          AddScaleMatrix(Kjk, m1, -mParams->Stretch2Dkd, Kjk);
          
            // setup the b
            float v1[3];
            float* locb = &b[mPi->mIndex*3];
            AddMatrix(Kij, Kik, m1);                     // K's acting on i
            TransformVector(m1, mPi->m_v, v1);              // K*v
            AddScaleVector(fi, v1, dt, v1);           // f + dt*K*v
            AddScaleVector(locb, v1, dt, locb);          // b += dt*(f + dt*K*v)
            
              locb = &b[mPj->mIndex*3];
              AddMatrix(Kij, Kjk, m1);                     // K's acting on j
              TransformVector(m1, mPj->m_v, v1);              // K*v
              AddScaleVector(fj, v1, dt, v1);           // f + dt*K*v
              AddScaleVector(locb, v1, dt, locb);          // b += dt*(f + dt*K*v)
              
                locb = &b[pk->mIndex*3];
                AddMatrix(Kjk, Kik, m1);                     // K's acting on k
                TransformVector(m1, pk->m_v, v1);              // K*v
                AddScaleVector(fk, v1, dt, v1);           // f + dt*K*v
                AddScaleVector(locb, v1, dt, locb);          // b += dt*(f + dt*K*v)
                
                  // get the block matrices
                  pm *Mij = A->GetBlock(mPi->mIndex, mPj->mIndex);
                  pm *Mji = A->GetBlock(mPj->mIndex, mPi->mIndex);
                  pm *Mik = A->GetBlock(mPi->mIndex, pk->mIndex);
                  pm *Mki = A->GetBlock(pk->mIndex, mPi->mIndex);
                  pm *Mjk = A->GetBlock(mPj->mIndex, pk->mIndex);
                  pm *Mkj = A->GetBlock(pk->mIndex, mPj->mIndex);
                  
                    AddLinearMatrix(Kij, dt*dt, dCidCj, -dt*mParams->Stretch2Dkd, m1);
                    AddMatrix(Mij->k, m1, Mij->k);
                    AddMatrix(Mji->k, m1, Mji->k);
                    
                      AddLinearMatrix(Kik, dt*dt, dCidCk, -dt*mParams->Stretch2Dkd, m1);
                      AddMatrix(Mik->k, m1, Mik->k);
                      AddMatrix(Mki->k, m1, Mki->k);
                      
                        AddLinearMatrix(Kjk, dt*dt, dCjdCk, -dt*mParams->Stretch2Dkd, m1);
                        AddMatrix(Mjk->k, m1, Mjk->k);
                        AddMatrix(Mkj->k, m1, Mkj->k);
        */
    }
    else //mUsePartialDerivative
    {
        float* locb = &b[mPi->mIndex*3];
        AddScaleVector(locb, fi, dt, locb);          // b += dt*(f + dt*K*v)
        locb = &b[mPj->mIndex*3];
        AddScaleVector(locb, fj, dt, locb);          // b += dt*(f + dt*K*v)
        locb = &b[pk->mIndex*3];
        AddScaleVector(locb, fk, dt, locb);          // b += dt*(f + dt*K*v)
    }
}

//
//
//

Shear2DCondition::Shear2DCondition(FlexParticle *pii, FlexParticle *pjj, FlexParticle *pkk, ConditionParams* p, float a0, bool inUsePartialDerivative)
: Condition(pii, pjj, p),
angle0(a0),
mPk(pkk)
{
    mUsePartialDerivative = inUsePartialDerivative;
}

float Shear2DCondition::Cond()
{
    // use the next one instead;
    rAssert(false);
    return 0;
}

float Shear2DCondition::Cond(float dCi[], float dCj[], float dCk[])
{
    float dpji[3], dpkj[3];
    float C0 = 0;
    float cosangle = 0;
    
    SubVector(mPj->m_x, mPi->m_x, dpji);
    SubVector(mPk->m_x, mPj->m_x, dpkj);
    
    // if x is cross product
    // the normal at segment ji is: dpji x (dpji x dpkj) where the () is the binormale of the string 
    // the normal at segment kj is: dpkj x (dpji x dpkj)
    // using the identity: a x (b x c ) = b (a dot c) - c (a dot b)
    // we get a simpler problem.
    
    float I = DotVector(dpji, dpji);
    float J = DotVector(dpji, dpkj);
    float K = DotVector(dpkj, dpkj);
    
    float nji[3], nkj[3];
    
    AddLinearVector(dpji, J, dpkj, -I, nji);
    AddLinearVector(dpji, K, dpkj, -J, nkj);
    
    float nnji = NormalizeVector(nji);
    float nnkj = NormalizeVector(nkj);
    
    cosangle = DotVector(nji, nkj);
    if( Fabs(cosangle) > 1.0f ) // just in case, this should not happen
        cosangle = (cosangle > 0 ? 1.0f : -1.0f);
    C0 = float(acos(cosangle)) - angle0;
    
    // we got the material for the derivative a;ready so let's do it!!!
    if (cosangle == 0 || Fabs(C0) < VERY_SMALL_BEND)
    {
        ZeroVector(dCi);
        ZeroVector(dCj);
        ZeroVector(dCk);
        return 0;
    }
    /*
    // we now have to evaluate d(cosangle)/dx with x = mPi,j,k->m_x[0,1,2]
    // for that we will assume that the nnji and nnjk are constant, see doc for justification.
    
      float var = -1.0f / sqrt(1.0f - Sqr(cosangle)); // derivative of acos(cosangle)
      float N = var/(nnji*nnkj);
      float tj2 = 3.0f*J*J;
      float IJ = I*J, IK = I*K, JK = J*K;
      
        // assuming that for non-normalized nij and nkj, (nji dot nkj) = I*J*K - J^3 (see notes)
        
          for (int i=0; i<3; i++)
          {
          dCi[i] = N*( JK*(-2.0f*dpji[i]) + IK*(-dpkj[i])        + 0                  - tj2*(-dpkj[i])         );
          dCk[i] = N*( 0                  + IK*( dpji[i])        + IJ*( 2.0f*dpkj[i]) - tj2*( dpji[i])         );
          dCj[i] = N*( JK*( 2.0f*dpji[i]) + IK*( dpkj[i]-dpji[i])+ IJ*(-2.0f*dpkj[i]) - tj2*( dpkj[i]-dpji[i]) );
          dCj[i] = -dCi[i] - dCk[i];
          }
    */
    // for some reason the derivative doesn't give very good result. But we know where we want the
    // force to be toward: the normal nji and nkj.
    ScaleVector(nji, -1.0f, dCi);
    ScaleVector(nkj, -1.0f, dCk);
    AddVector(nji, nkj, dCj);
    
    return C0;
}

void Shear2DCondition::Derivative(const float C0, float dCi[], float dCj[], float dCk[]) 
{
    rAssert(false); // get the derivative in Cond(...) instead for optimisation
}

void Shear2DCondition::BuildMatrix(SparseBlockMatrix *A, float *b, float dt)
{
    float dCi[3], dCj[3], dCk[3];
    float C0 = Cond(dCi, dCj, dCk);
    
    if (Fabs(C0) < VERY_SMALL_BEND)
        return;
    
    // second derivate in x, y, z for i and j etc.
    //float d2Cij[3][3];
    //Derivative2(C0, d2Cij);
    
    // build the dC/dt = dCi * vi + dCj * vj
    // since dCj = -dCi-dCk, if i and j and k have same v, cDot == 0 which make sens
    float Cdot;
    Cdot  = DotVector(mPi->m_v, dCi);
    Cdot += DotVector(mPj->m_v, dCj);
    Cdot += DotVector(mPj->m_v, dCk);
    
    // add the force add the damping in f
    float fi[3], fj[3], fk[3], kkf = -mParams->Shear2Dkf*C0, kkd = -mParams->Shear2Dkd*Cdot;
    kkf += kkd;
    
    ScaleVector(dCi, kkf, fi);
    ScaleVector(dCj, kkf, fj);
    ScaleVector(dCk, kkf, fk);
    
#if defined(RAD_DEBUG)
    AddVector(fi, mPi->m_nn, mPi->m_nn);
    AddVector(fj, mPj->m_nn, mPj->m_nn);
    AddVector(fk, mPk->m_nn, mPk->m_nn);
#endif
    
    //*************** according to the paper, these are symetric.. (since Kij and d2Cij are sym)
    float dCidCj[3][3];
    MultVectorVectorT(dCi, dCj, dCidCj);
    
    // compute the K total which is K + mParams->Shear2Dkd, which are symetric
    //float Kij[3][3];
    //AddLinearMatrix(dCidCj, -mParams->Shear2Dkf, d2Cij, -mParams->Shear2Dkf*C0, Kij);
    //AddScaleMatrix(Kij, d2Cij, -mParams->Shear2Dkd*Cdot, Kij);
    
    // setup the b
    float* locb = &b[mPi->mIndex*3];
    //TransformVector(Kij, mPi->m_v, v1);          // K*v
    //AddScaleVector(fi, v1, dt, v1);           // f + dt*K*v
    //AddScaleVector(locb, v1, dt, locb);       // b += dt*(f + dt*K*v)
    AddScaleVector(locb, fi, dt, locb);         // f only for now
    
    locb = &b[mPj->mIndex*3];
    AddScaleVector(locb, fj, dt, locb);         // f only for now
    
    locb = &b[mPk->mIndex*3];
    AddScaleVector(locb, fk, dt, locb);         // f only for now
    
    // get the block matrices
    // pm *Mij = A->GetBlock(mPi->mIndex, mPj->mIndex);
    // pm *Mji = A->GetBlock(mPj->mIndex, mPi->mIndex);
    
    //float m1[3][3];
    //AddLinearMatrix(Kij, dt*dt, dCidCj, -dt*mParams->Shear2Dkd, m1);
    //AddMatrix(Mij->k, m1, Mij->k);
    //AddMatrix(Mji->k, m1, Mji->k);
}

//
//
//

Bend2DCondition::Bend2DCondition(FlexParticle *pii, FlexParticle *pjj, FlexParticle *pkk, FlexParticle *pll, ConditionParams* p, float a0, bool inUsePartialDerivative)
: Condition(pii, pjj, p),
angle0(a0),
mPk(pkk),
mPl(pll)
{
    // ignore angle0 for now => equil is flat.
    // considers j is in the middle.
    mUsePartialDerivative = inUsePartialDerivative;
}

float Bend2DCondition::Cond()
{
    // use the next one instead;
    rAssert(false);
    return 0;
}

float Bend2DCondition::Cond(float dCi[], float dCj[], float dCk[], float dCl[])
{
    float dpa[3], dpb[3];
    float n1[3], n2[3];
    float C0 = 0;
    float cosangle = 0;
    
    SubVector(mPj->m_x, mPi->m_x, dpa);
    SubVector(mPk->m_x, mPi->m_x, dpb);
    CrossVector(dpa, dpb, n1);
    
    SubVector(mPk->m_x, mPl->m_x, dpa);
    SubVector(mPj->m_x, mPl->m_x, dpb);
    CrossVector(dpa, dpb, n2);
    
    
    float nn1 = NormalizeVector(n1);
    float nn2 = NormalizeVector(n2);
    
    if (nn1 > parallelLimit && nn2 > parallelLimit)
    {
        cosangle = DotVector(n1, n2);
        if( Fabs(cosangle) > 1.0f ) // just in case, this should not happen
            cosangle = (cosangle > 0 ? 1.0f : -1.0f);
        C0 = float(acos(cosangle));
        
        SubVector(mPi->m_x, mPl->m_x, dpa);
        if (DotVector(dpa, n1) < 0)
            C0 = -C0;
    }
    
    // we got the material for the derivative already so let's do it!!!
    if (cosangle == 0 || Fabs(C0) < VERY_SMALL_BEND)
    {
        ZeroVector(dCi);
        ZeroVector(dCj);
        ZeroVector(dCk);
        ZeroVector(dCl);
        return 0;
    }
    
    // for some reason the derivative doesn't give very good result. But we know where we want the
    // force to be toward: the normal nji and nkj.
    ScaleVector(n1, -1.0f, dCi);
    ScaleVector(n2, -1.0f, dCl);
    AddVector(n1, n2, dCj);
    EqualVector(dCj, dCk);
    
    return C0;
}

void Bend2DCondition::BuildMatrix(SparseBlockMatrix *A, float *b, float dt)
{
    float dCi[3], dCj[3], dCk[3], dCl[3];
    float C0 = Cond(dCi, dCj, dCk, dCl);
    
    if (Fabs(C0) < VERY_SMALL_BEND)
        return;
    
    // build the dC/dt = dCi * vi + dCj * vj
    // since dCj = -dCi-dCk, if i and j and k have same v, cDot == 0 which make sens
    float Cdot;
    Cdot  = DotVector(mPi->m_v, dCi);
    Cdot += DotVector(mPj->m_v, dCj);
    Cdot += DotVector(mPj->m_v, dCk);
    Cdot += DotVector(mPj->m_v, dCl);
    
    // add the force add the damping in f
    float fi[3], fj[3], fk[3], fl[3], kkf = -mParams->Bend2Dkf*C0, kkd = -mParams->Bend2Dkd*Cdot;
    
    kkf += kkd;
    
    ScaleVector(dCi, kkf, fi);
    ScaleVector(dCj, kkf, fj);
    ScaleVector(dCk, kkf, fk);
    ScaleVector(dCl, kkf, fl);
    
#if defined(RAD_DEBUG) 
    AddVector(fi, mPi->m_fb, mPi->m_fb);
    AddVector(fj, mPj->m_fb, mPj->m_fb);
    AddVector(fk, mPk->m_fb, mPk->m_fb);
    AddVector(fl, mPl->m_fb, mPl->m_fb);
#endif
    
    // setup the b
    float* locb = &b[mPi->mIndex*3];
    AddScaleVector(locb, fi, dt, locb);
    
    locb = &b[mPj->mIndex*3];
    AddScaleVector(locb, fj, dt, locb);
    
    locb = &b[mPk->mIndex*3];
    AddScaleVector(locb, fk, dt, locb);
    
    locb = &b[mPl->mIndex*3];
    AddScaleVector(locb, fl, dt, locb);
}

//
//
//

Wind2DCondition::Wind2DCondition(FlexParticle *pii, FlexParticle *pjj, FlexParticle *pkk, ConditionParams* p, Vector* w, float inarea)
: Condition(pii, pjj, p),
mPk(pkk),
mWind(w),
mArea(inarea)
{
}

float Wind2DCondition::Cond()
{
    // use the next one instead;
    rAssert(false);
    return 0;
}

float Wind2DCondition::Cond(float f[])
{
    Vector dpa, dpb, dv, n1;
    float C0 = 0;
    
    if (mParams->Wind2DKf == 0.0f)
        return 0.0f;
    
    AddVector(mPi->m_v, mPj->m_v, (float*)&dv);
    AddVector((float*)&dv, mPk->m_v, (float*)&dv);
    dv.ScaleAdd( *mWind, -0.333333333f, dv); // dv = windspeed - (vi+vj+vk)/3
    
    SubVector(mPj->m_x, mPi->m_x, (float*)&dpa);
    SubVector(mPk->m_x, mPi->m_x, (float*)&dpb);
    n1.CrossProduct(dpa, dpb);
    
    // triangle's normal
    n1.NormalizeSafe();
    
    C0 = n1.DotProduct(dv);
    
    ScaleVector((float*)&n1, C0*mParams->Wind2DKf*mArea, f);
    
    return C0;
}

void Wind2DCondition::BuildMatrix(SparseBlockMatrix *A, float *b, float dt)
{
    float f[3];
    float C0 = Cond(f);
    
    if (C0 == 0)
        return;
    
    ScaleVector(f, dt, f);
    
    // setup the b
    float* locb = &b[mPi->mIndex*3];
    AddVector(locb, f, locb);
    
    locb = &b[mPj->mIndex*3];
    AddVector(locb, f, locb);
    
    locb = &b[mPk->mIndex*3];
    AddVector(locb, f, locb);
}

} // sim
