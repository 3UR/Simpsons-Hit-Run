
#ifndef _SIMTARGET_HPP_
#define _SIMTARGET_HPP_

#include <radmath/radmath.hpp>
#include <simcommon/symmatrix.hpp>
#include <p3d/refcounted.hpp>
//#include <poser/joint.hpp>

namespace poser
{
    class Pose;
}

namespace sim
{

//
// class SimTarget and its derivate allow to specify a target toward with 
// the sim link will move the joint or object toward SimTarget::GetPos()
//


// position = offset
class SimTarget
    : public tRefCounted
{
public:
    SimTarget() { offset.Set(0, 0, 0); }
    
    void SetOffset(const rmt::Vector& off) { offset = off; }
    const rmt::Vector& GetOffset() const { return offset; }
    
    virtual void GetPos(rmt::Vector& v, float dt=0.0f) const { v = offset; }
    virtual void GetTransform(rmt::Matrix& m, float dt=0.0f) const;
    
    virtual void GetMassMatrix(SymMatrix& mat) const { mat.Zero(); }
    virtual float GetMass() const { return 0; }
    virtual void GetSpeed(rmt::Vector & v) const { v.Set(0, 0, 0); }
    
protected:
    virtual ~SimTarget() {}
    rmt::Vector offset;
};

//
// position = offset + *vector_target
//

class SimVectorTarget
    : public SimTarget
{
public:
    SimVectorTarget(const rmt::Vector* pos);
    
    void ReplaceTarget(const rmt::Vector* pos);
    virtual void GetPos(rmt::Vector& v, float dt=0.0f) const { v.Add(offset, *target); }
    
protected:
    SimVectorTarget() {} // locked
    virtual ~SimVectorTarget() {}
    const rmt::Vector* target;
};

//
// position = offset * *matrix_target
//

class SimMatrixTarget
    : public SimTarget
{
public:
    SimMatrixTarget(const rmt::Matrix* m);
    
    void ReplaceTarget(const rmt::Matrix* m);
    // note: must dynamic cast SimTarget* to SimMatrixTarget* for next two functions
    // otherwise base class is called
    virtual void GetPos(rmt::Vector& v, float dt=0.0f) const { target->Transform(offset, &v); }
    virtual void GetTransform(rmt::Matrix& m, float dt=0.0f) const;
    
    
protected:
    friend class SimMatrixProgressiveTarget;
    SimMatrixTarget() {} // locked
    virtual ~SimMatrixTarget() {}
    const rmt::Matrix* target;
};


class SimJntMatTarget
    : public SimTarget
{
public:
    SimJntMatTarget(poser::Pose* inPose, int index);
    
    void ReplaceTarget(poser::Pose* inPose, int index);
    
    virtual void GetPos(rmt::Vector& v, float dt=0.0f) const; 
    virtual void GetTransform(rmt::Matrix& m, float dt=0.0f) const;
    
    
protected:
    //friend class SimJntMatProgTarget;
    //SimJntMatTarget() {} // locked
    virtual ~SimJntMatTarget() {}
    poser::Pose* mPose;
    int mIndex;
};


} // sim

#endif // simtarget_HPP_