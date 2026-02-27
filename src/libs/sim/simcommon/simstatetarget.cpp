#include <simcommon/simstatetarget.hpp>


using namespace RadicalMathLibrary;


namespace sim
{

SimStateTarget::SimStateTarget(SimState* inSimState)
{
    mSimState = inSimState;
}

void SimStateTarget::ReplaceTarget(SimState* inSimState)
{
    mSimState = inSimState;
}

void SimStateTarget::GetAngularVelocity(Vector& v) const
{
    v = mSimState->GetAngularVelocity(); 
}

void SimStateTarget::GetSpeed(Vector& v) const
{
    v = mSimState->GetLinearVelocity(); 
}



SimVectorProgressiveTarget::SimVectorProgressiveTarget(const Vector* inCurrent, 
    const Vector* inTarget, 
    float inMAXSPEED)     
    :SimVectorTarget(inTarget),
    mCurrentPosition(inCurrent),
    mMAXSPEED(inMAXSPEED)
{
    
}

void SimVectorProgressiveTarget::GetPos(rmt::Vector& v, float dt) const
{
    //untested
    Vector difference;
    float  distance;
    SimVectorTarget::GetPos(difference);
    difference.Sub(*mCurrentPosition);
    distance = difference.Magnitude();
    float step = mMAXSPEED * dt;
    if(step < distance)
    { 
        v = *mCurrentPosition;
        v.ScaleAdd(mMAXSPEED*dt/distance, difference);
    }
    else
    {
        SimVectorTarget::GetPos(v);
    }
}



SimMatrixProgressiveTarget::SimMatrixProgressiveTarget(const Matrix* inCurrent, 
    const Matrix* inTarget, 
    float inMAXSPEED,
    float inMAXANGSPEED)     
    :SimMatrixTarget(inTarget),
    mCurrentOrientation(inCurrent),
    mMAXSPEED(inMAXSPEED),
    mMAXANGSPEED(inMAXANGSPEED)
{
    
}

SimMatrixProgressiveTarget::SimMatrixProgressiveTarget(const rmt::Matrix* inCurrent,
    const SimMatrixTarget* inTarget,
    float inMAXSPEED,
    float inMAXANGSPEED)
    :SimMatrixTarget(inTarget->target),
    mCurrentOrientation(inCurrent),
    mMAXSPEED(inMAXSPEED),
    mMAXANGSPEED(inMAXANGSPEED)
{
    
}

void SimMatrixProgressiveTarget::GetPos(rmt::Vector& v, float dt) const
{
    Vector difference;
    float  distance;
    Vector v1,v2;
    Matrix m2 = *mCurrentOrientation;  //? nicer way to do this? can't use Row(3).
    v2 = m2.Row(3);
    SimMatrixTarget::GetPos( v1 );
    difference.Sub( v1, v2 );  
    distance = difference.Magnitude();
    float step = mMAXSPEED * dt;
    if( step < distance )
    { 
        v = v2;
        v.ScaleAdd(mMAXSPEED*dt/distance, difference);
    }
    else
    {
        v = v1;
    }
}

void SimMatrixProgressiveTarget::GetTransform(rmt::Matrix& m, float dt) const
{
    // Not ready yet
    
    Matrix m1;
    Matrix m2;
    m1 = *mCurrentOrientation;
    m2 = *target;
    Quaternion q1, q2;
    q1.BuildFromMatrix(m1);
    q2.BuildFromMatrix(m2);
    
    float distance= 1.0f;  //? temporary 
    // calculate angular distance between quaternions
    float step = mMAXANGSPEED * dt;
    if(step < distance)
    {  
        q1.Slerp(q2, step/distance);
        q1.ConvertToMatrix(&m);
    }
    else
    {
        SimMatrixTarget::GetPos(m.Row(3));
    }
    m = *target;  // for now just return target orientation with interpolated position
    GetPos(m.Row(3), dt);
}

} // sim