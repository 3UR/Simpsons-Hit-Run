
#ifndef _CONDITION_HPP_
#define _CONDITION_HPP_

#include "p3d/p3dtypes.hpp"

namespace sim
{

class FlexParticle;
class SparseBlockMatrix;

typedef enum {  eFlexStretch1D_f=1,   eFlexStretch1D_d,
                eFlexStretch2D_f,     eFlexStretch2D_d,
                eFlexBend1D_f,        eFlexBend1D_d,
                eFlexBend2D_f,        eFlexBend2D_d,
                eFlexShear2D_f,       eFlexShear2D_d,
                eFlexWind1D_f,        eFlexWind2D_f
             } eFlexPhysicalPropertiesEnum;

typedef short CondParamsFlags;
const CondParamsFlags cParamSetBit          = 0x01;
const CondParamsFlags cParamUseLongSideBit  = 0x02;
const CondParamsFlags cParamSetBitMask      = 0xfe;
const CondParamsFlags cParamUseLongSideMask = 0xfd;

// Conditions are expressed for each pair of particles that depend on the
// condition.  This lets us differentiate easily.

class ConditionParams
{

public:
    void SetParamSet(bool val)
    { 
        if(val==true)
            mParamFlag = mParamFlag|cParamSetBit;
        else
            mParamFlag = mParamFlag&cParamSetBitMask;
    }

    ConditionParams() :
      Stretch1Dkf(10000.0f),
          Stretch1Dkd(10.0f),
          Bend1Dkf(2000.0f),
          Bend1Dkd(2.0f),
          Wind1DKf(0.3f),
          Stretch2Dkf(1.0f),
          Stretch2Dkd(0.1f),
          Shear2Dkf(400.0f),
          Shear2Dkd(2.5f),
          Bend2Dkf(200.0f),
          Bend2Dkd(1.0f),
          Wind2DKf(0.02f),
          LambdaF(10.0f),
          LambdaD(1.0f),
          KappaF(10.0f),
          KappaD(1.0f),
          IotaF(10.0f),
          IotaD(1.0f),
          mParamFlag(0),
          mUID(0)
        {
            SetParamSet(true);
        }
        virtual ~ConditionParams() {}

        static bool ComputeStretch1DFromLambda  ( float inStretchFact,   float inDampingFact, int inNbp, float inLength0, float inGravity, float &o_force, float &o_damping, float inMass=1.0f );
        static bool ComputeBend1DFromKappa      ( float inCurvatureFact, float inDampingFact, int inNbp, float inLength0, float inGravity, float &o_force, float &o_damping, float inMass=1.0f );
        void SetUseLongSide(bool val)
        { 
            if(val==true)
                mParamFlag = mParamFlag|cParamUseLongSideBit;
            else
                mParamFlag = mParamFlag&cParamUseLongSideMask;
        }
        bool GetParamSet() const { return (cParamSetBit & mParamFlag)?true:false; }
        bool GetUseLongSide() const { return (cParamUseLongSideBit & mParamFlag)?true:false; }
    
        // stretching stiffness. with gravity = 980cm/s^2, the resting stretch is kd * dl = g.
        // for a 10cm string, if we want dl = 0.1cm, we must have kd = g/dl ~ 1000/0.01 = 10000
        float Stretch1Dkf;
        float Stretch1Dkd;
        float Bend1Dkf;
        float Bend1Dkd;
        float Wind1DKf;
        float Stretch2Dkf;
        float Stretch2Dkd;
        float Shear2Dkf;
        float Shear2Dkd;
        float Bend2Dkf;
        float Bend2Dkd;
        float Wind2DKf;

        float LambdaF;
        float LambdaD;
        float KappaF;
        float KappaD;
        float IotaF;
        float IotaD;
      
        CondParamsFlags mParamFlag;

        tUID  mUID;
};

class Condition {
    Condition *mNextCond;
    
public:
    Condition(FlexParticle *pii, FlexParticle *pjj, ConditionParams* p);
    virtual ~Condition();
    
    virtual void BuildMatrix(SparseBlockMatrix *A, float *b, float dt) = 0;
    virtual float Cond()=0;
    
    Condition *next() { return mNextCond; };
    inline void Insert(Condition *f) { f->mNextCond = mNextCond; mNextCond = f; }
    void RemoveNext();
    
    inline virtual bool RelateToParticle(const FlexParticle* p) { return (p == mPi || p == mPj); }
    
    virtual void Scale(float f) {}
    
    virtual float GetStretch1Dkf() const { return mParams->Stretch1Dkf;}
    virtual float GetStretch1Dkd() const { return mParams->Stretch1Dkd;}
    virtual float GetStretch2Dkf() const { return mParams->Stretch2Dkf;}
    virtual float GetStretch2Dkd() const { return mParams->Stretch2Dkd;}
    virtual float GetBend1Dkf   () const { return mParams->Bend1Dkf;   }
    virtual float GetBend1Dkd   () const { return mParams->Bend1Dkd;   }
    virtual float GetBend2Dkf   () const { return mParams->Bend2Dkf;   }
    virtual float GetBend2Dkd   () const { return mParams->Bend2Dkd;   }
    virtual float GetShear2Dkf  () const { return mParams->Shear2Dkf;  }
    virtual float GetShear2Dkd  () const { return mParams->Shear2Dkd;  }
    virtual float GetWind1DKf   () const { return mParams->Wind1DKf;   }
    virtual float GetWind2DKf   () const { return mParams->Wind2DKf;   }
    
    FlexParticle      *mPi, *mPj;
    ConditionParams   *mParams;
    bool               mUsePartialDerivative;
};

//
//
//

//
//
//

class NullCondition : public Condition 
{
    float Cond() { return 0; };
    
public:
    NullCondition(ConditionParams* p) : Condition(NULL,NULL, p) {};
    void BuildMatrix(SparseBlockMatrix *A, float *b, float dt) {};
};

//
//
//

// the contition return a vector of size n = np-1 with np the amount of particles
// the condition acts on. the following structure are used for np=3 conditions.
struct C2DVal
{
    float u;
    float v;
};

struct C2DVect
{
    float u[3];
    float v[3];
};

struct C2DMat
{
    C2DVect u[3];
    C2DVect v[3];
};

/*
// some special math utility for the 2 elem condition, 
// a condition is C2DVal,
// a vector condition is C2DVal vC[3], 
// a matrix condition is C2DVal mC[3][3]
// a vector is float v[3]
// a matrix is float m[3][3]
*/

// mult a vC[3][2] to Transpose of vCt[3][2] res is m[3][3]
void vC_Mult_vCT(const C2DVect& vC, const C2DVect& vCt, float m[][3]);

// mult a vC[3][2] to a C[2], res is v[3]
void vC_Mult_C(const C2DVect& vC, const C2DVal& C, float v[]);

// mult a mC[3][3][2] to a C[2], res is m[3][3]
void mC_Mult_C(const C2DMat& mC, const C2DVal& C, float res[][3]);

// Cdot[2] = dC/dt += dC/dx[3][2] * p->v[3]
void AddToCdot2(float v[], const C2DVect& dCi, C2DVal& Cdoti);

static float VERY_SMALL_BEND = 0.01f;
static float VERY_SMALL_STRETCH = 0.0001f;
static float parallelLimit = 0.01f;

} // sim

#endif // _CONDITION_HPP_
