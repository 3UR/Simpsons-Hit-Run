

#include "simflexible/condition1d.hpp"
#include "simflexible/flexparticle.hpp"
#include "simcommon/sbmatrix.hpp"
#include "simcommon/simmath.hpp"

using namespace RadicalMathLibrary;

namespace sim
{

float Stretch1DCondition::dp[3];
float Stretch1DCondition::ndp;

Stretch1DCondition::Stretch1DCondition(FlexParticle *pii, FlexParticle *pjj, ConditionParams* p, float ddl, bool inUsePartialDerivative)
    : Condition(pii,pjj,p) 
{
    bu = 1.0f;
    dl = ddl;   // segment lenth at equil.
    mUsePartialDerivative = inUsePartialDerivative;
}

float Stretch1DCondition::Cond() 
{
    return (ndp - dl*bu); // dl*(ndp/dl - bu);
}

void Stretch1DCondition::Derivative(const float C0, float dCi[], float dCj[]) 
{
    if (C0 == 0)
    {
        ZeroVector(dCj);
        ZeroVector(dCi);
    }
    else
    {
        ScaleVector(dp, 1.0f/ndp, dCj);
        ScaleVector(dCj, -1.0f, dCi);
    }
}

void Stretch1DCondition::Derivative2(const float C0, float d2Cij[][3])
{
    if (C0 == 0)
    {
        ZeroVector(d2Cij[0]);
        ZeroVector(d2Cij[1]);
        ZeroVector(d2Cij[2]);
    }
    else
    {
        float invC0 = 1.0f/ndp;
        float invC03 = invC0*invC0*invC0;
        
        d2Cij[0][1] = d2Cij[1][0] = dp[0]*dp[1] * invC03;
        d2Cij[0][2] = d2Cij[2][0] = dp[0]*dp[2] * invC03;
        d2Cij[1][2] = d2Cij[2][1] = dp[1]*dp[2] * invC03;
        
        d2Cij[0][0] = -invC0 + dp[0] * dp[0] * invC03;
        d2Cij[1][1] = -invC0 + dp[1] * dp[1] * invC03;
        d2Cij[2][2] = -invC0 + dp[2] * dp[2] * invC03;
        
        // it has been verified in Maple V that d2Cii = d2Cjj = -d2Cij which kind of make sens
    }
}

void Stretch1DCondition::BuildMatrix(SparseBlockMatrix *A, float *b, float dt) 
{
    // these are used everywhere
    SubVector(mPj->m_x, mPi->m_x, dp);
    ndp = LenVector(dp);
    
    float C0 = Cond();
    
    if (Fabs(C0) < VERY_SMALL_STRETCH)
        return;
    
    float dCi[3], dCj[3];
    Derivative(C0, dCi, dCj);
    
    // build the dC/dt = dCi * vi + dCj * vj
    // since dCi = -dCj, if i and j have same v, cDot == 0 which make sens
    float Cdot;
    Cdot  = DotVector(mPi->m_v, dCi);
    Cdot += DotVector(mPj->m_v, dCj);
    
    // add the force in p->f
    float v1[3], fi[3], fj[3];
    float kkf = GetStretch1Dkf()*C0, kkd = GetStretch1Dkd();
    kkf += kkd*Cdot;
    
    ScaleVector(dCi, -kkf, fi);
    ScaleVector(dCj, -kkf, fj);
    
#if defined(RAD_DEBUG)
    float ff[3];
    ScaleVector(dCi, -kkf, ff);
    AddVector(fi, mPi->m_fs, mPi->m_fs);//force
    ScaleVector(dCj, -kkf, ff);
    AddVector(fj, mPj->m_fs, mPj->m_fs); 
    
    ScaleVector(dCi, -kkd, ff);
    AddVector(ff, mPi->m_fc, mPi->m_fc);//damping
    ScaleVector(dCj, -kkd, ff);
    AddVector(ff, mPj->m_fc, mPj->m_fc);
    
    static float debugmax = 100000.0f;
    if (DotVector(mPi->m_fs, mPi->m_fs) > debugmax)
        int debugi = 1;
    if (DotVector(mPj->m_fs, mPj->m_fs) > debugmax)
        int debugi = 1;
#endif
    
    if( mUsePartialDerivative )
    {
        // second derivate in x, y, z for i and j etc.
        float d2Cij[3][3];
        Derivative2(C0, d2Cij);
        
        float dCidCj[3][3]; // since dCi = -dCj, dCidCj is sym and dCidCi=dCjdCj=-dCidCj
        MultVectorVectorT(dCi, dCj, dCidCj);
        
        // compute the K total which is K + Kd, which are symetric and Kii = Kjj = -Kij
        float Kij[3][3];
        AddLinearMatrix(dCidCj, -GetStretch1Dkf(), d2Cij, -GetStretch1Dkf()*C0 -GetStretch1Dkd()*Cdot, Kij);
        
        // setup the b
        float kvdt[3];
        AddScaleVector(mPj->m_v, mPi->m_v, -1.0f, v1);     // Kii vi + Kij vj = Kij(-vi+vj)
        TransformVector(Kij, v1, kvdt);              //
        ScaleVector(kvdt, dt, kvdt);
        
        float* locb = &b[mPi->mIndex*3];
        AddVector(fi, kvdt, v1);                     // v1 = f + dt*K*v
                                                     /*
                                                     if (mPi->bdxc)
                                                     {
                                                     TransformVector(Kij, mPi->dxc, tmp);
                                                     AddVector(v1, tmp, v1);
                                                     }
        */
        AddScaleVector(locb, v1, dt, locb);          // b += dt*(f + dt*K*v)
        
        locb = &b[mPj->mIndex*3];                      // Kij vi + Kjj vj = Kij(vi-vj) = -kv
        SubVector(fj, kvdt, v1);                     // v1 = f + dt*K*v
                                                     /*
                                                     if (mPj->bdxc)
                                                     {
                                                     TransformVector(Kij, mPj->dxc, tmp);
                                                     SubVector(v1, tmp, v1);
                                                     }
        */
        AddScaleVector(locb, v1, dt, locb);          // b += dt*(f + dt*K*v)
        
        float m1[3][3];
        AddLinearMatrix(Kij, -dt*dt, dCidCj, dt*GetStretch1Dkd(), m1);
        
        pm *block = A->GetBlock(mPi->mIndex, mPj->mIndex);
        AddMatrix(block->k, m1, block->k);
        block = A->GetBlock(mPj->mIndex, mPi->mIndex);
        AddMatrix(block->k, m1, block->k);
        
        ScaleMatrix(m1, -1.0f, m1); // since Kii = Kjj = -Kij
        block = A->GetBlock(mPi->mIndex, mPi->mIndex);
        AddMatrix(block->k, m1, block->k);
        block = A->GetBlock(mPj->mIndex, mPj->mIndex);
        AddMatrix(block->k, m1, block->k);
    }
    else // mUsePartialDerivative
    {
        float* locb = &b[mPi->mIndex*3];
        AddScaleVector(locb, fi, dt, locb);          // b += dt*(f + dt*K*v)
        locb = &b[mPj->mIndex*3];
        AddScaleVector(locb, fj, dt, locb);          // b += dt*(f + dt*K*v)
    } // mUsePartialDerivative
}


//
//
//

Bend1DCondition::Bend1DCondition(FlexParticle *pii, FlexParticle *pjj, FlexParticle *pkk, ConditionParams* p, bool calcRestAngle, bool inUsePartialDerivative)
: Condition(pii, pjj, p),
  angle0(0),
  prevC0(0),
  mPk(pkk)
{
    mUsePartialDerivative = inUsePartialDerivative;
    // ignore angle0 for now => equil is flat.
    // considers j is in the middle.
    if (calcRestAngle)
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
        angle0 = float(ACos(cosangle));
    }
}

float Bend1DCondition::Cond()
{
    // use the next one instead;
    rAssert(false);
    return 0;
}

float Bend1DCondition::Cond(float dCi[], float dCj[], float dCk[])
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

	if (Sqr(nnji) < I*Sqr(MILLI_EPS) || Sqr(nnkj) < I*Sqr(MILLI_EPS))
		return 0;
    
#ifdef RAD_DEBUG
    //EqualVector(nji, mPi->nn);
    //EqualVector(nkj, pk->nn);
    //AddVector(nji, nkj, mPj->nn);
#endif
    
    cosangle = DotVector(nji, nkj);
    if( Fabs(cosangle) > 1.0f ) // just in case, this should not happen
        cosangle = (cosangle > 0 ? 1.0f : -1.0f);
    C0 = float(ACos(cosangle));
    C0 -= angle0;
    
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

void Bend1DCondition::Derivative(const float C0, float dCi[], float dCj[], float dCk[]) 
{
    rAssert(false); // get the derivative in Cond(...) instead for optimisation
}

void Bend1DCondition::BuildMatrix(SparseBlockMatrix *A, float *b, float dt)
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
    Cdot += DotVector(mPk->m_v , dCk);
    
    // add the force add the damping in f
    float fi[3], fj[3], fk[3]; 
    float kkf = -mParams->Bend1Dkf*C0, kkd = -mParams->Bend1Dkd*Cdot;
    
    ScaleVector(dCi, kkf, fi);  
    ScaleVector(dCj, kkf, fj);
    ScaleVector(dCk, kkf, fk);
#if defined(RAD_DEBUG)
    AddVector(fi, mPi->m_fb, mPi->m_fb);
    AddVector(fj, mPj->m_fb, mPj->m_fb);
    AddVector(fk, mPk->m_fb, mPk->m_fb);
#endif
    AddScaleVector(fj, dCj, kkd, fj);//Add damping on the j iem node
    AddScaleVector(fi, dCi, kkd, fi);//Add damping on the i iem node
    AddScaleVector(fk, dCk, kkd, fk);//Add damping on the k iem node
#if defined(RAD_DEBUG)
    AddScaleVector(mPj->m_nn, dCj, kkd, mPj->m_nn);
    AddScaleVector(mPi->m_nn, dCi, kkd, mPi->m_nn);
    AddScaleVector(mPk->m_nn, dCk, kkd, mPk->m_nn);
#endif
    
    //*************** according to the paper, these are symetric.. (since Kij and d2Cij are sym)
    //float dCidCj[3][3];
    //MultVectorVectorT(dCi, dCj, dCidCj);
    
    // compute the K total which is K + Kd, which are symetric
    //float Kij[3][3];
    //AddLinearMatrix(dCidCj, -mParams->Bend1Dkf, d2Cij, -mParams->Bend1Dkf*C0, Kij);
    //AddScaleMatrix(Kij, d2Cij, -mParams->Bend1Dkd*Cdot, Kij);
    
    // setup the b
    float* locb = &b[mPi->mIndex*3];
    //TransformVector(Kij, mPi->m_v, v1);             // K*v
    //AddScaleVector(fi, v1, dt, v1);           // f + dt*K*v
    //AddScaleVector(locb, v1, dt, locb);          // b += dt*(f + dt*K*v)
    AddScaleVector(locb, fi, dt, locb);         // f only for now
    
    locb = &b[mPj->mIndex*3];
    AddScaleVector(locb, fj, dt, locb);         // f only for now
    
    locb = &b[mPk->mIndex*3];
    AddScaleVector(locb, fk, dt, locb);         // f only for now
    
    // get the block matrices
    // pm *Mij = A->GetBlock(mPi->mIndex, mPj->mIndex);
    // pm *Mji = A->GetBlock(mPj->mIndex, mPi->mIndex);
    
    //float m1[3][3];
    //AddLinearMatrix(Kij, dt*dt, dCidCj, -dt*mParams->Bend1Dkd, m1);
    //AddMatrix(Mij->k, m1, Mij->k);
    //AddMatrix(Mji->k, m1, Mji->k);
}

//
//
//

Wind1DCondition::Wind1DCondition(FlexParticle *pii, FlexParticle *pjj, ConditionParams* p, Vector* w)
: Condition(pii, pjj, p),
mWind(w)
{
    float dl[3];
    SubVector(pii->m_x, pjj->m_x, dl);
    mLen = float(sqrt(DotVector(dl, dl)));
}

float Wind1DCondition::Cond()
{
    // use the next one instead;
    rAssert(false);
    return 0;
}

float Wind1DCondition::Cond(float f[])
{
    Vector dp, dv, nn;
    Vector n;
    float C0 = 0;
    
    AddVector(mPi->m_v, mPj->m_v, (float*)&dv);
    dv.ScaleAdd(*mWind, -0.5f, dv); // dv = windspeed - (vi+vj)/2
    SubVector(mPj->m_x, mPi->m_x, (float*)&dp);

    if (dp.NormalizeSafe() > ARRAY_MATH_VERY_SMALL)
    {
        nn.CrossProduct(dp, dv);
        n.CrossProduct(dp, nn );
        n.NormalizeSafe();
    
        C0 = n.DotProduct(dv);
    
        //ScaleVector(n, C0*mParams->Wind1DKf*len, f); // could also include the diameter
        ScaleVector((float*)&n, C0*mParams->Wind1DKf, f); // could also include the diameter
    }
    
    return C0;
}

void Wind1DCondition::BuildMatrix(SparseBlockMatrix *A, float *b, float dt)
{
    float f[3];
    float C0 = Cond(f);
    
    if (C0 == 0)
        return;
    
    ScaleVector(f, dt, f);
    
    // setup the b
    float* locb = &b[mPi->mIndex*3];
    AddVector(locb, f, locb);         // f only for now
    
    locb = &b[mPj->mIndex*3];
    AddVector(locb, f, locb);         // f only for now
}

} // sim
