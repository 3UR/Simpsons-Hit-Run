
#include "simphysics/restingdetector.hpp"
#include "simphysics/physicsobject.hpp"

namespace sim
{
    
PhysicsRestingDetector::PhysicsRestingDetector() :
    mTypicalLinSpeed(0.0f),
    mTypicalAngSpeed(0.0f),
    mTypicalLength(0.0f),
    mTypicalPeriod(0.0f),
    mPeriodSensitivity(1.0f),
    mLinSpeedSensitivity(1.0f),
    mAngSpeedSensitivity(1.0f),
    mNeedToComputeTypicalValues(true),
    mAtRest(false),
    mPhysicsObject(NULL)
{
}

PhysicsRestingDetector::~PhysicsRestingDetector()
{
}

void PhysicsRestingDetector::SetObj( const PhysicsObject* inPhysicsObj)
{ 
    rAssert(inPhysicsObj);
    mPhysicsObject = inPhysicsObj; 
}

bool PhysicsRestingDetector::UpdateRestingCondition( float in_dt )
{
    Vector l_v;
    float l_wm;
    
    ComputeTypicalValues(in_dt);
    
    l_v = mPhysicsObject->GetCMVelocity(l_v);
    l_wm = ((SimState*)mPhysicsObject->GetSimState())->GetAngularVelocity().Magnitude();
    
    mLinRestCond.Estimate( &l_v, in_dt);
    mAngRestCond1D_mag.Estimate(Fabs(l_wm), in_dt);
    
    return true;
}

//The fact that the component of the velocity along the gravity is filtered separately
//make the parametrisation none sensitive to the geometry of the objects. 
//If for exemple we decided to filter simply the magnitude of the velocity, 
//large objects introduce tangential component in the error velocity by jittering 
//which becomes difficult to keep into account in the parametrisation. 

void PhysicsRestingDetector::TestRestingCondition(float in_dt )
{
    rAssert( mPhysicsObject->GetSimState()->GetControl() != simAICtrl );
    
    if (mAtRest == true)
        return;
    
    const float fact = 1.5f;
    Vector l_estimateV     = mLinRestCond.GetEstimate( );
    float  l_TypicalValue  = GetTypicalLinSpeed();
    if( l_estimateV.y > l_TypicalValue || l_estimateV.x > fact*l_TypicalValue || l_estimateV.z > fact*l_TypicalValue )
    {
        float l_TypicalPeriod = GetTypicalPeriod();
        float l_TypicalAngV   = GetTypicalAngSpeed();
        static float fact = 0.5f;
        mAngRestCond1D_mag.Init( l_TypicalAngV, fact*l_TypicalPeriod, 0 );
        return; //Not at rest.
    }
    
    // Now, perform the test on the angular domain.
    float l_estimateW = mAngRestCond1D_mag.GetEstimate();
    l_TypicalValue = GetTypicalAngSpeed();
    if( l_estimateW > l_TypicalValue )
        return; //Not at rest.
    
    mAtRest = true;
}


/* This method reset the kalmann filter use to detect resting objects. 

  The first parameter passed in the SimpleFilter::Init is the threshold 
  velocity. The filter is now much more wise and it will compute the
  first initial guess uptimal for us. In fact, the first estimate will
  be exactly two time the threshold value. This guaranty that the filter
  will never start somewhere bellow the threshold or somewhere excessively
  far which could add some indesirable delay in the response.
  
    The second argument is the delay we want between
    the real speed and the estimate. We want this period to be the smallest possible
    as we want the fastest time of response in detecting objects at rest. 
    But if the period is too small, the estimate could be too close to the data and
    we can get bad estimate. Note that the typical period is sensitive to
    the size of the object but is not sensitive to the collision distance. For exemple, the
    period is about of 5ms for an object with a scale factor of ten as it is about 1.3ms for
    a unit sphere.
*/
bool PhysicsRestingDetector::ResetRestingCondition()
{
    mAtRest = false;

    if (mNeedToComputeTypicalValues)
        return false;

    //First, reset the filter in the linear domain.
    float l_Typicalvel	  = GetTypicalLinSpeed();
    float l_TypicalPeriod = GetTypicalPeriod();
    
    Vector l_vTypicalvel, l_vTypicalPeriod;
    l_vTypicalvel.Set(l_Typicalvel, l_Typicalvel, l_Typicalvel);
    l_vTypicalPeriod.Set( l_TypicalPeriod, l_TypicalPeriod, l_TypicalPeriod );
    mLinRestCond.Init( &l_vTypicalvel, &l_vTypicalPeriod, &l_vTypicalPeriod );
    
    //Now, reset the filter in the angular domain.
    float l_TypicalAngV   = GetTypicalAngSpeed();
    
    static float fact = 0.5f;
    return mAngRestCond1D_mag.Init( l_TypicalAngV, fact*l_TypicalPeriod, 0 );
}

//The periode of oscillation of the vertical jittering does depend
//on the gravity and the collision distance. We also have found an empirecal 
//linear dependency on the time scale ratio (see below).

//If the user want to get an object in 
//resting faster or slower, he can call SetRestingSensitivity. For exemple if
//he pass in a value of .5, he can expect to get resting in about two time fater. 
//But note that this is really approximative.

//The typical velocity is used to set the threshold speed for resting. It depends on 
//the colldist, and the gravity, but is not sensitive to the size of the
//object. Several test had showed that there is some dependency on the shape of the object 
//but we did not find the dependency explicitely. We can play with sensitive parameters
//for that. The typical velocity depends also on the time step. We have find a linear
//dependency over the time ratio between the natural time of the system and the 
//current time step. The time ratio is : tr = dt / Sqrt(2*d/g)

//Note: there is no special case in real time mode. Typical value will be computed
//according to the time step received at the first computation.

//The typical angular velocity is used to set the threshold speed for resting detection
//in the angular domain.
//Because the shape of a cylinder make it rotating naturally even if it stands up, 
//its radius size is a better choise for its typical length than the value returned
//by GetTypicalLength. But we do not make special case for particular geometry.
//The sensitivity on the threshold angular speed could be use instead.


bool PhysicsRestingDetector::ComputeTypicalValues( float in_dt )
{
    if (!mNeedToComputeTypicalValues)
        return true;

    SimEnvironment* env = mPhysicsObject->GetSimEnvironment();

    if( env && env->GravityNorm() > 0.0f )
    {
        rAssert(env->CollisionDistance() > 0.0f);
        float l_timeRatio = 2.0f * env->CollisionDistance() / env->GravityNorm();
        l_timeRatio = in_dt / Sqrt(l_timeRatio);

        static float av=5.0f, bv=0.5f, at=.06f;
        float a, b;
        a = at;
        b = 1.0f - 10*a;
        mTypicalPeriod = 5 * (a*l_timeRatio + b) * Sqrt( env->CollisionDistance() / env->GravityNorm() );
        rAssert(mTypicalPeriod>0);
        mTypicalPeriod    *= mPeriodSensitivity; 
        
        a = av;
        b = bv;
        mTypicalLinSpeed = Sqrt( 2.0f*env->CollisionDistance()*env->GravityNorm() ) * (a*l_timeRatio + b)/10.0f;
        rAssert(mTypicalLinSpeed>0);
        mTypicalLinSpeed  *= mLinSpeedSensitivity;
    }
    else
    {
        mTypicalLinSpeed  = 0.0f;
        mTypicalPeriod = VERY_LARGE;
    }
    
    SimLength length;
    if (mPhysicsObject->GetSimState())
    {
        length.SetValueCGS(mPhysicsObject->GetSimState()->GetSphereRadius());
    }
    else 
    {
        length.SetValueCGS(100);
    }
    mTypicalLength = length.ValueCGS();
    
    //Typical angular speed is computed as follow:
    //A variation of the angular momentum changes the angular speed:
    //dW = Iinv*dL, 
    //under an impact, change in L is dL=rxp 
    //which is in the order of r.M.v . This gives: dW = Iinv.r.M.v
    //Assuming that impact are around end of objects, and that near resting condition
    //v becomes the threshold speed, we get, 
    //dW = Iinv.L.M.V ( L is the typical length, M the mass and V the threshold speed).
    //As Iinv is in the order of : (L^2 * M)^-1, we finally get our typical angular
    //speed around resting conditions.
    //dW = V.L^-1
    //What is important to see is that there is no dependendy on the exact inertia
    //of the object cause mass canceled. It only remain some dependency on the
    //inertia through the geometry of the object.
    
    mTypicalAngSpeed = mAngSpeedSensitivity * 2 * mTypicalLinSpeed/GetTypicalLength();

    mNeedToComputeTypicalValues = false;

    if (mPhysicsObject->GetSimState() )
        ResetRestingCondition();

    return true;
}


} // sim
