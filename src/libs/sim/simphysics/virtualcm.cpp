
#include "simphysics/virtualcm.hpp"
#include "simcommon/dline2.hpp"

#if defined(RAD_DEBUG) && defined(RAD_WIN32)
#include <stdio.h>
#include "windows.h"
#endif

using namespace RadicalMathLibrary;

namespace sim
{

// default setting definition
float VirtualCM::sDefault_invTA = 10.0f; // 
float VirtualCM::sDefault_invTV = 30.0f; // units: inverse of time
float VirtualCM::sDefault_invTP = 1.0f;  // No units: < 1 => oscillation ; == 1 => critical;  > 1 => over damped. 

float VirtualCM::sDefault_restP = 4.0f; // no unit
float VirtualCM::sDefault_restV = 4.0f; // no unit


//
// the class
//

VirtualCM::VirtualCM()
: mPosition(0, 0, 0),
  mVelocity(0, 0, 0),
  mAngularDeviation(0, 0, 0)
{
    mInvTA = sDefault_invTA;
    mInvTV = sDefault_invTV; // increasing this makes the vcm pos slower to reach the pos (lower spring stiffness)
    mInvTP = sDefault_invTP;
    mInvTP = 2*sDefault_invTP*Sqrt(mInvTV);
    
    mRestP = sDefault_restP;
    mRestV = sDefault_restV;

    SetLinearMode(true);
    SetVerticalMode(false);
    SetActive(true);
}

VirtualCM::VirtualCM(VirtualCMMode inBits)
: mPosition(0, 0, 0),
  mVelocity(0, 0, 0),
  mAngularDeviation(0, 0, 0),
  mModeFlag(inBits)
{
    mInvTA = sDefault_invTA;
    mInvTV = sDefault_invTV; // increasing this makes the vcm pos slower to reach the pos (lower spring stiffness)
    mInvTP = sDefault_invTP;
    mInvTP = 2*sDefault_invTP*Sqrt(mInvTV);
    
    mRestP = sDefault_restP;
    mRestV = sDefault_restV;
}

void VirtualCM::InitLinear(const Vector& inPos, const Vector& inVelocity )
{
    if (!GetActive() || !GetLinearMode())
        return;

    mPosition = inPos;
    mVelocity = inVelocity;
}

void VirtualCM::InitAngular(const Vector& inAng, const Vector& inVelocity)
{
    rAssertMsg(0,"Not Implemented");
    if (!GetActive() || !GetAngularMode())
        return;

    mAngularVelocity  = inAng;
    mAngularDeviation = inVelocity;
}

void VirtualCM::Update(const Vector& pos, const Vector& speed, float inDt)
{
    Vector dv;
    float hdt = inDt * 0.5f;
    
    if (GetLinearMode() && GetActive())
    {
        // compute a constant spring type force to bring the virtual rcm to the cm
        dv.Sub(pos, mPosition);
        
        // start updating the virtual pos using the previous speed
        mPosition.ScaleAdd(hdt, mVelocity);
        
        // update the virtual speed with
        mVelocity.ScaleAdd(inDt * mInvTV, dv);
        
        // add some friction for stability
        dv.Sub(speed, mVelocity);
        mVelocity .ScaleAdd(inDt * mInvTP, dv);
        
        // complete updating the virtual rcm using the new speed
        // so that p += (premVelocity + newS)*inDt/2, modified mid-point, little better than euler
        mPosition.ScaleAdd(hdt, mVelocity);
    }
    
    if (GetAngularMode())
    {
    }

    static bool displayOutput=false;
    if (displayOutput)
    {
        PrintOut(inDt);
    }
}

void VirtualCM::PrintOut(float inDt)const
{
#if defined(RAD_DEBUG) && defined(RAD_WIN32)
    if (!GetActive())
        return;

    static float dt=0;
    dt+=inDt;
    Vector l_p = GetPosition();
    Vector l_v = GetVelocity();
    char buff[250]; buff[0]='\0';
    static enum { positionXYZ, positionModule, velocityXYZ, velocityModule} toOut=velocityModule;
    switch(toOut)
    {
    case positionXYZ:
        {  //position xyz only
            if (GetVerticalMode())
                sprintf(buff,"\n%10.5f %10.5f %10.5f %10.5f ", dt, l_p.x, l_p.y, l_p.z );
            else
                sprintf(buff,"\n%10.5f %10.5f %10.5f ", dt, l_p.x, l_p.z );
        }
        break;
    case positionModule:
        {  //position module only
            sprintf(buff,"\n%10.5f %10.5f", dt, l_p.Magnitude() );
        }
        break;
    case velocityXYZ:
        {  //Velocity xyz only.
            if (GetVerticalMode())
                sprintf(buff,"\n%10.5f %10.5f %10.5f %10.5f ", dt, l_v.x, l_v.y, l_v.z );
            else
                sprintf(buff,"\n%10.5f %10.5f %10.5f ", dt, l_v.x, l_v.z );
        }
        break;
    case velocityModule:
        {  //position module only
            sprintf(buff,"\n%10.5f %10.5f", dt, l_v.Magnitude() );
        }
    default:
        {
        }
        break;
    }
    OutputDebugString(buff);
#endif
}

void VirtualCM::AddObjectCache(const Vector& inV, const Vector& inW)
{
    if (!GetActive())
        return;

    if (GetLinearMode())
        mVelocity.Add(inV);
    if (GetAngularMode())
        mAngularVelocity.Add(inW);
}

void VirtualCM::DebugDisplay() const
{
    if(!GetActive())
        return;

    DrawLineToggler toggler;

    //Display the vcm's speed
    tColour colour(0, 255, 255);
    static float speedScale = 0.1f;
    Vector speed = GetVelocity();
    
    speed.ScaleAdd(GetPosition(), speedScale, speed);
    dLine2(GetPosition(), speed, colour);

    //Display the vcm.
    static float sizef=0.1f;
    static Vector size(sizef,sizef,sizef);
    dBox3(GetPosition(), size, colour);
}

void JointVirtualCM::PrintOut(float inDt)const
{
#if defined(RAD_DEBUG) && defined(RAD_WIN32)
    char buff[250]; buff[0]='\0';

    VirtualCM::PrintOut(inDt);

    sprintf( buff, "%8ld", mIndex );
    OutputDebugString(buff);
#endif
}

} // sim
