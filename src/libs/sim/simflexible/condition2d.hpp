
#ifndef _CONDITION2D_HPP_
#define _CONDITION2D_HPP_

#include "simflexible/condition.hpp"

namespace sim
{

// This gets you one stretch.  If the uv matrix is given by
// [[ du1 du2][dv1 dv2]] then you need one condition for
// the first column, one for the second.
// To make the second work right you need to reverse k and j, and swap dv1 and dv2.

class Stretch2DCondition 
    : public Condition 
{
    float Cond() ;
    
    float du1, dv1, du2, dv2;
    float bu, bv;
    float a;
    
    // utility values, would not work in a parallel implementation...
    static float dpji[3];
    static float dpki[3];
    static float Lu;
    static float Lv;
    
public:
    Stretch2DCondition(  FlexParticle *pi, FlexParticle *pj, FlexParticle *pkk,
        ConditionParams* p, bool inUsePartialDerivative=true);
    Stretch2DCondition(  FlexParticle *pi, FlexParticle *pj, FlexParticle *pkk,
        ConditionParams* p,
        float du, float dv, float du22, float dv22, bool inUsePartialDerivative=true);
    FlexParticle *pk;
    
    void Cond(C2DVal& C);
    void Derivative(const float C0, const float L, const float d1, const float d2, float dCi[], float dCj[], float dCk[]);
    void Derivative2(float *x1, float *x2, const C2DVal& dC1p2, C2DVal& ddC, int ind1, int ind2);
    void BuildMatrix(SparseBlockMatrix *A, float *b, float dt);
    
    void Scale(float f) { a *= f*f; /*rAssert(false);*/ /* need to do du1 dv1 etc. */}
};

//
//
//
#define halfPI (3.14159f/2.0f)

class Shear2DCondition
    : public Condition 
{
    float Cond();
    float Cond(float dCi[], float dCj[], float dCk[]);
    float angle0;
    
public:
    Shear2DCondition(FlexParticle *pi, FlexParticle *pj, FlexParticle *pk, ConditionParams* p, float angle0 = halfPI, bool inUsePartialDerivative=true);
    FlexParticle* mPk;
    
    // Derivative 2 is d/dx1(dC/dx2), since we have already dC/dx2, we use it
    void Derivative2(const float C0, float d2Cij[][3]);
    void Derivative(const float C0, float dCi[], float dCj[], float dCk[]);
    void BuildMatrix(SparseBlockMatrix *A, float *b, float dt);
};

//
//
//

class Bend2DCondition
    : public Condition 
{
    float Cond();
    float Cond(float dCi[], float dCj[], float dCk[], float dCl[]);
    float angle0;
    
public:
    Bend2DCondition(FlexParticle *pi, FlexParticle *pj, FlexParticle *pk, FlexParticle *pl, ConditionParams* p, float angle0 = 0, bool inUsePartialDerivative=true);
    FlexParticle* mPk;
    FlexParticle* mPl;
    
    //void Derivative2(const float C0, float d2Cij[][3]);
    void BuildMatrix(SparseBlockMatrix *A, float *b, float dt);
};

class Wind2DCondition
    : public Condition 
{
    float Cond();
    float Cond(float f[]);
    
public:
    Wind2DCondition(FlexParticle *pi, FlexParticle *pj, FlexParticle *pk, ConditionParams* p, rmt::Vector* wind, float area);
    FlexParticle* mPk;
    
    //void Derivative2(const float C0, float d2Cij[][3]);
    void BuildMatrix(SparseBlockMatrix *A, float *b, float dt);
    rmt::Vector* mWind;
    float       mArea;
    
    void Scale(float f) { mArea *= f*f; }
};

} // sim

#endif // _CONDITION2D_HPP_
