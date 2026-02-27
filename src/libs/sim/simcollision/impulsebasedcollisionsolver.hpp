
#ifndef _IMPULSEBASEDCOLLISIONSOLVER_HPP_
#define _IMPULSEBASEDCOLLISIONSOLVER_HPP_

//
// class ImpulseBasedCollisionSolver
//
// This class loop around the collisions in the collisions list
// in order to 
// -  solved all the interpenetration for invM != 0.0 (may need many iteration in case of
//    multiple object collision
// -  compute the impulse transmitted in the collision and add it into the object
//

#include <p3d/refcounted.hpp>
#include <radmath/radmath.hpp>
#include "simcommon/symmatrix.hpp"
#include "simcommon/tarray.hpp"
#include "simcollision/collision.hpp"
#include "simcollision/collisionanalyser.hpp"

namespace sim
{

class Collision;
class PhysicsObject;
class SimulatedObject;

enum Solving_Answer { Solving_Continue = 0, Solving_Aborted };

class CollisionSolverAgent
    : public tRefCounted
{
public:
    
    CollisionSolverAgent();
    virtual ~CollisionSolverAgent();
    
    virtual void StartSolvingCollisionList(TArray<Collision>* inList, float current_time = 0);
    virtual void EndSolvingCollisionList(TArray<Collision>* inList, float current_time = 0);
    
    // this method is called at the beginning before anything is done with the collision
    // returning Solving_Aborted would cause the collision to be ignored. It is sometime
    // necessary for pile of objects, to solve a collisions on more than one pass. inPass 
    // indicate the current pass.
    virtual Solving_Answer PreCollisionEvent(Collision& inCollision, int inPass);
    
    // this method allows to trigger sounds, animation and modify the objects state.
    virtual Solving_Answer CollisionEvent(  SimState* inSimStateA, int indexA, 
                                            SimState* inSimStateB, int indexB, 
                                            const rmt::Vector& inPos, float inDvN, float inDvT,
                                            SimulatedObject** simA, SimulatedObject** simB);
    
    // this is called everytime an impulse is computed, before it gets added to the objects
    virtual Solving_Answer TestImpulse(rmt::Vector& mImpulse, Collision& inCollision);
    
    // this is called everytime an impulse is added to an object's cache.
    // Testing the cache allow to get an idea of the effect of the impulse before
    // it will really added to the objects at the EndObjectCollision call.
    // Return Solving_Continue if the effects seems good. Return Solving_Aborted
    // to tell the solver to try again to compute better impulse.
    virtual Solving_Answer TestCache  (SimState* inSimState, int index);

    // the impulse has been added to the object's cache, testing that cache tells 
    // the consequence of adding the cache to the object's state. It is also a good 
    // time for adding the cache of the root node of an articulated object to its
    // virtual center of mass.
    virtual Solving_Answer EndObjectCollision(SimState* inSimState, int inIndex);
    
    void Update(float inDt, float gameTime) { if(mAnalyser) mAnalyser->Update(gameTime); }

    void EnableCollisionAnalyser(void);
    void DisableCollisionAnalyser(void);
    bool CollisionAnalyserEnabled(void) const { return mAnalyser != NULL ? true : false; }
    
    CollisionAnalyser *GetCollisionAnalyser(void) { return mAnalyser; }
    CollisionAnalyserDataPList& GetCollisionAnalyserData(bool calculateState = true)
    {
        rAssert(mAnalyser);
        return mAnalyser->GetCollisionData();
    }

protected:
    CollisionAnalyser *mAnalyser;
};

class ImpulseBasedCollisionSolver
{
public:
    ImpulseBasedCollisionSolver();
    virtual ~ImpulseBasedCollisionSolver();
    
    inline void DoQuickSolve(bool b)             { mQuickSolve = b; }
    
    void SetCollisionSolverAgent(CollisionSolverAgent* inAgent);
    CollisionSolverAgent* GetCollisionSolverAgent() { return mAgent; }
    
    void SolveCollisions(TArray<Collision>* inList, float dt_sec, float gameTime);
    
    void SetMaxPasses(int inMaxPasses) { mMaxPasses = inMaxPasses; }

    void UpdateForSimEnvironment();
    
protected:
    friend class SimEnvironment;

    void SetCollisionDistance(float inCollDist)  { rAssert(inCollDist >= 0); mCollisionDistance = inCollDist; }
    void SetGravityNorm(float inGravityNorm)     { rAssert(mGravityNorm >= 0); mGravityNorm = inGravityNorm; }
    
    CollisionSolverAgent* mAgent;
    
    void ExtractVelocities();
    virtual void SolveCollisionImpulse();
    void           StartObj(SimulatedObject* inObject, SimulatedObject* otherObject, const rmt::Vector& pos, SymMatrix& inSymMatrix, int index, int otherIndex);
    Solving_Answer ComputeImpulse();
    virtual bool   AddImpulse(SimulatedObject* inObject, SimulatedObject* otherObject, const rmt::Vector& inPosition, SymMatrix& inSymMatrix, int index, int otherIndex);
    virtual void   EndObj(SimulatedObject* inObject, int index, int otherIndex, bool fdyn);
    virtual bool   PositionCorrectionForInterpenetration( float &io_dist);
    
    virtual float RestCoeff();
    float FrictCoeff();
    float TangRestCoeff();
    
    TArray<Collision>* mCollisionList;  // current collision list
    
    // current collision data
    Collision mCollision;         // the collision being solved
    rmt::Vector mImpulse;         //
    
    rmt::Vector mVelocityA;       // velocity of object A at the contact point
    rmt::Vector mVelocityB;       // velocity of object B at the contact point
    rmt::Vector mDVelocity;       // mVelocityA - mVelocityB
    float mNormDVelocityN;        // norm of mDVelocity along the collision normal
    
    SimState* mSimStateA;         //
    SimState* mSimStateB;         //
    SimulatedObject* mSimObjectA; // 
    SimulatedObject* mSimObjectB; //
    
    SymMatrix mMassMatrixA;       //
    SymMatrix mMassMatrixB;       //
    SymMatrix mMassMatrixTot;     //
    
    float mCollisionDistance;     // 
    float mGravityNorm;           //
    bool mAllSolved;              // true when all collision have been solved
    bool mFirstPass;              // true during first pass of the solvecollision loop
    bool mFullDynColl;            // indicate a full dynamic collision, used in articulated object
    bool mSelfCollision;          // indicate a collision between articulation of an articulated object
    bool mFlexCollision;          // indicates a collision involving a flexible object
    
    bool mQuickSolve;             // resolve the collision quickly (no friction)
    int  mMaxPasses;           // defines maximum number of passes for the collision solver
};

} // sim

#endif // _ImpulseBasedCollisionSolver_HPP_
