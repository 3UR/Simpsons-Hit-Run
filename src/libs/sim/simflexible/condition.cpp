
#include "simflexible/condition.hpp"
#include "simcommon/simmath.hpp"

using namespace RadicalMathLibrary;

namespace sim
{

/* 
This method allows you to set the physical stretch parameter (force and damping)
with two intuitives parameters. They are not entented to achive realistic behaviors
but to help parametrisation. They are free from units. 

  inStretchFact represents the tolerance we want to stretch when the flexible object 
  is suspended in the given gravity. It is a percenttage. If you pass in 10, the
  object will strech 10% from its resting length. In case where length0 == 0 the relative
  length is not defined. In that case inStretchFact is considered as the length
  the string will have at equilibrum in the gravity when suspended. 
  
    inDampingFact represents how the object will return to its equilibrum state from a deformed 
    state. With value bellow of one, objects will oscillate. With a value of one, the
    object will get back its equilibrum shape whitout oscillation. With higher value
    object will get its equilibrum shape more slowly (could be unstable).
    
      NOTE: Formulas are not exact although they are very accurate for a large set
      of parameters providing that the time step is sufficiently small.
      For exemple, if you use inStretchFact=50 and inDampingFact=1, the object will 
      be at rest with two time its original length, and will not oscillate.
      But you will see that segments near the attached point will be more stretched 
      than others. This is normal as there is more weight pulling on top than at the 
      bottom of the string (Turn off TestExtension1D to see this).
      
        It is usefull to know how depends the numerical stability over all input parameters.
        Sqrt(2*inStretchFact*length0/G/(nbPart-1))/nbPart;
        is a measure of the stiffness of the equations system. 
        The smaller this number the higher the stifiness. 
        Useable set of parameters is (for a given gravity field) when:
        -the rest legnth is large
        -the number of particle is low
        -tolerance to stretching is high (high inStretchFact).
*/
bool ConditionParams::ComputeStretch1DFromLambda  
    ( 
    float inStretchFact,    //Percentage of tolerance to stretch. Ex: 10 ( mean 10%)
    float inDampingFact,    //Coefficient of damping. 1 mean no oscillation.
    int   inNbp, 
    float inLength0,        
    float inGravity,
    float &o_force, 
    float &o_damping,
    float inMass
    )
{
    if (inStretchFact < MICRO_EPS)
    {
        o_force = 0.0f; 
        o_damping = 0.0f;
        return true;
    }

    if (inLength0==0.0f)
    {
        //Rest length is zero. use lambda as an absolute length.
        inLength0 = inStretchFact;
        o_force = inMass*inGravity*inNbp*(inNbp-1.0f) / (2.0f*inLength0);
    }
    else
    {
        //Rest length is not zero use lambda as a relative length.
        inStretchFact /= 100.0f;
        o_force = inMass*inGravity*inNbp / (2.0f*inStretchFact*inLength0);
    }
    
    
    //Now the best coefficient of damping is given by:
    o_damping = 2.0f*inDampingFact*Sqrt(inMass*o_force);
    
    float eulerTimeStep2 = Sqrt( 2.0f*inStretchFact*inLength0/inGravity/inNbp);
    
    return true;
}

/* 
This method allows you to set the physical banding parameter (force and damping)
with two intuitives parameters. They are not entented to achive realistic behaviors
but to help parametrisation. They are free from units.

  inCurvatureFact control how much the shape of a string will be at equilibrum when
  suspended under the gravity by its midle particle. Small value will result on low
  curvature (flat curve) while large value will result in a large curved shape. 
  When inCurvatureFact equal one, we get the natural curvature (the curve takes 
  approximately the shape of a circle with radius equal to its length).
  
    inDampingFact represents how the object will return to its equilibrum shape from a 
    deformed state. With value bellow of one, objects will oscillate. With a value of one, 
    the object will get back its equilibrum shape whitout oscillation. With higher value
    object will get its equilibrum shape more slowly (could be unstable).
    
      It is usefull to know how depends the numerical stability over all input parameters.
      2*Sqrt(inCurvatureFact*length0 * cos(pi/2)/G) / ( N-1 )
      is a measure of the stiffness of the equations system. 
      The smaller this number the higher the stifiness. 
      Useable set of parameters (for a given gravity field) is when:
      -the rest legnth is large, 
      -the number of particle is low
      -the object has low bending resistance (high inCurvatureFact).
      
*/
bool ConditionParams::ComputeBend1DFromKappa      
    ( 
    float inCurvatureFact,  //Curvature factor from natural curvature. Pass in zero to get no bending forces.
    float inDampingFact,    //Coefficient of damping. 1 mean no oscillation.
    int   inNbp, 
    float inLength0,  
    float inGravity, 
    float &o_force, 
    float &o_damping,
    float inMass
    ) 
{
    
    if (inCurvatureFact < MICRO_EPS)
    {
        o_force   = 0.0f;
        o_damping = 0.0f;
        return true; 
    }
    
    float Length;
    Length  = inLength0*(inNbp-1.0f);
    
    rAssert(Length>=MICRO_EPS);
    if (Length<MICRO_EPS)
    {
        o_force   = 0.0f;
        o_damping = 0.0f;
        return false;
    }
    
    float curvature = 1.0f/Length; //This is the "natural" curvature.
    
    //But user wants to have inCurvatureFact time the default curvature.
    curvature *= inCurvatureFact;
    
    float teta; //angle of cotangence.
    //For a given curvature, the local angle of cotangence will be:
    teta = 2.0f * curvature * inLength0;
    
    //Then the force bending coefficient is:
    float k;
    k = inMass*inGravity*(inNbp-1.0f);
    k /= 2.0f*teta*Cos(ClampBetween(teta/2.0f, 0.0f, 3.1416f/2.0f/(inNbp-1.0f) ));
    o_force = k;  // [F]
    
    //The preceeding expression could be re-writen, 
    //as k = m*g*(n-1)^2 / (4*inCurvatureFact*cos(inCurvatureFact/(n-1) ) ) , 
    //it shows that it depends only on the number of nodes. 
    
    //Now the best coefficient of damping is given by:
    o_damping = 2.0f * inDampingFact * Sqrt(inMass*o_force/inLength0) / (inNbp-1.0f) ; // [F/V]
    
    float eulerTimeStep = Sqrt( inMass*inLength0/o_force);
    return true;
}




Condition::Condition(FlexParticle *pii, FlexParticle *pjj, ConditionParams* par)
    : mNextCond(NULL),
    mPi(pii),
    mPj(pjj),
    mParams(par),
    mUsePartialDerivative(true)
{
    rAssert(mParams != NULL);
}

Condition::~Condition() 
{ 
}

void Condition::RemoveNext() 
{ 
    Condition* c = mNextCond; 
    mNextCond = c->mNextCond; 
    c->mNextCond = NULL; 
    delete c; 
}


void vC_Mult_vCT(const C2DVect& vC, const C2DVect& vCt, float m[][3])
{
    m[0][0] = vC.u[0]*vC.u[0] + vCt.v[0]*vCt.v[0];
    m[0][1] = vC.u[0]*vC.u[1] + vCt.v[0]*vCt.v[1];
    m[0][2] = vC.u[0]*vC.u[2] + vCt.v[0]*vCt.v[2];
    m[1][0] = vC.u[1]*vC.u[0] + vCt.v[1]*vCt.v[0];
    m[1][1] = vC.u[1]*vC.u[1] + vCt.v[1]*vCt.v[1];
    m[1][2] = vC.u[1]*vC.u[2] + vCt.v[1]*vCt.v[2];
    m[2][0] = vC.u[2]*vC.u[0] + vCt.v[2]*vCt.v[0];
    m[2][1] = vC.u[2]*vC.u[1] + vCt.v[2]*vCt.v[1];
    m[2][2] = vC.u[2]*vC.u[2] + vCt.v[2]*vCt.v[2];
}

void mC_Mult_C(const C2DMat& mC, const C2DVal& C, float res[][3])
{
}

} // sim
