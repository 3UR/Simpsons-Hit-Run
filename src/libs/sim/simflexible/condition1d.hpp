#ifndef _CONDITION1D_HPP_
#define _CONDITION1D_HPP_

#include "simflexible/condition.hpp"

namespace sim
{

class Stretch1DCondition 
    : public Condition 
{
    float Cond() ;
    
    float bu;
    float dl; // length of segment
    
    // utility values, would work in a parallel implementation...
    static float dp[3];
    static float ndp;
    
public:
    Stretch1DCondition(FlexParticle *pi, FlexParticle *pj, ConditionParams* p, float ddl, bool inUsePartialDerivative=true);
    
    // Derivative 2 is d/dx1(dC/dx2), since we have already dC/dx2, we use it
    void Derivative2(const float C0, float d2Cij[][3]);
    void Derivative(const float C0, float dCi[], float dCj[]);
    void BuildMatrix(SparseBlockMatrix *A, float *b, float dt);
    inline float Dl() const { return dl; }
    
    void Scale(float f) { dl *= f; }
};

//
//
//

class Bend1DCondition 
: public Condition 
{
    float Cond();
    float Cond(float dCi[], float dCj[], float dCk[]);
    float angle0;
    float prevC0;
    
public:
    Bend1DCondition(FlexParticle *pi, FlexParticle *pj, FlexParticle *pk, ConditionParams* p, bool calcRestAngle = false, bool inUsePartialDerivative=true);
    FlexParticle* mPk;
    
    inline virtual bool RelateToParticle(const FlexParticle* p) { return (Condition::RelateToParticle(p) || p == mPk); }
    
    // Derivative 2 is d/dx1(dC/dx2), since we have already dC/dx2, we use it
    void Derivative2(const float C0, float d2Cij[][3]);
    void Derivative(const float C0, float dCi[], float dCj[], float dCk[]);
    void BuildMatrix(SparseBlockMatrix *A, float *b, float dt);
};

//
//
//

class Wind1DCondition
: public Condition 
{
    float Cond();
    float Cond(float f[]);
    
public:
    Wind1DCondition(FlexParticle *pi, FlexParticle *pj, ConditionParams* p, rmt::Vector* wind);
    
    void BuildMatrix(SparseBlockMatrix *A, float *b, float dt);
    rmt::Vector* mWind;
    float       mLen;
    
    void Scale(float f) { mLen *= f; }
};


//Next three classes are use in conjonction with the semi-implicit
//flexible object solver. There pupose is to reuse the Stretch1DCondition
//code transparently for bending and shearing.
class SpringConditionForStretch : public Stretch1DCondition
{
public:
    SpringConditionForStretch(FlexParticle *pi, FlexParticle *pj, ConditionParams* p, float ddl)
        :Stretch1DCondition(pi, pj, p, ddl, false){}
    
    virtual float GetStretch1Dkf() const { return mParams->Stretch1Dkf;}
    virtual float GetStretch1Dkd() const { return mParams->Stretch1Dkd;}
};

class SpringConditionForBend : public Stretch1DCondition
{
public:
    SpringConditionForBend(FlexParticle *pi, FlexParticle *pj, ConditionParams* p, float ddl)
        :Stretch1DCondition(pi, pj, p, ddl, false){}
    virtual float GetStretch1Dkf() const { return mParams->Bend1Dkf;}
    virtual float GetStretch1Dkd() const { return mParams->Bend1Dkd;}
};

class SpringConditionForShear : public Stretch1DCondition
{
public:
    SpringConditionForShear(FlexParticle *pi, FlexParticle *pj, ConditionParams* p, float ddl)
        :Stretch1DCondition(pi, pj, p, ddl, false){}
    virtual float GetStretch1Dkf() const { return mParams->Shear2Dkf;}
    virtual float GetStretch1Dkd() const { return mParams->Shear2Dkd;}
};

} // sim

#endif // _CONDITION1D_HPP_
