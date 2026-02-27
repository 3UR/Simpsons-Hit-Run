

#include "simcommon/simtarget.hpp"
#include <raddebug.hpp>

#include <poser/pose.hpp>
#include <poser/joint.hpp>

using namespace RadicalMathLibrary;

namespace sim
{

SimVectorTarget::SimVectorTarget(const Vector* pos)
    : target(pos) 
{
    rAssert(target != NULL);
}

void SimTarget::GetTransform(rmt::Matrix& m, float dt) const
{ 
    m.Identity(); 
    GetPos(m.Row(3));
}



void SimVectorTarget::ReplaceTarget(const Vector* pos)
{
    target = pos;
    rAssert(target != NULL);
}

//
//
//

SimMatrixTarget::SimMatrixTarget(const Matrix* pos)
    : target(pos) 
{
    rAssert(target != NULL);
}

void SimMatrixTarget::ReplaceTarget(const Matrix* pos)
{
    target = pos;
    rAssert(target != NULL);
}

void SimMatrixTarget::GetTransform(rmt::Matrix& m, float dt) const
{ 
    m = *target; 
    GetPos(*(Vector*)m.m[3]);
}

// SimJntMatTarget         SimJointMatrixTarget
SimJntMatTarget::SimJntMatTarget(poser::Pose* inPose, int index)
    : mPose(inPose),
    mIndex(index) 
{
    rAssert(mPose != NULL);
}

void SimJntMatTarget::ReplaceTarget(poser::Pose* inPose, int index)
{
    mPose = inPose;
    mIndex = index;
    rAssert(mPose != NULL);
}

void SimJntMatTarget::GetPos(Vector& v, float dt) const
{ 
    mPose->GetJoint(mIndex)->GetWorldMatrix().Transform(offset, &v);
}

void SimJntMatTarget::GetTransform(rmt::Matrix& m, float dt) const
{ 
    m = mPose->GetJoint(mIndex)->GetWorldMatrix(); 
    GetPos(*(Vector*)m.m[3]);
}


} // sim