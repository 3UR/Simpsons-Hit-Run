

#include "simcollision/impulsebasedcollisionsolver.hpp"
#include "simcommon/simulatedobject.hpp"
#include "simcommon/simstatearticulated.hpp"


using namespace RadicalMathLibrary;

namespace sim
{

const int MAX_PASSES = 3;

float FACTMINV = 2.0f; // scale the min speed from g acceleration on a dist of mCollisionDistance, to determine static collision
float MAXREST = 50.0f;
float MINREST = 1.0f;
float maxFactRest = 1.1f; //4.0f
float minFactRest = 0.1f;

int   useCorrectPosition = 2;


ImpulseBasedCollisionSolver::ImpulseBasedCollisionSolver()
    : mAgent(NULL),
    mCollisionList(NULL),
    mQuickSolve(false),
    mMaxPasses(MAX_PASSES)
{
    UpdateForSimEnvironment();

    // IAN - i don't think this needs to be called
    //CollisionSolverAgent* tmpAgent = new CollisionSolverAgent;
    //SetCollisionSolverAgent(tmpAgent);
}

ImpulseBasedCollisionSolver::~ImpulseBasedCollisionSolver()
{
    rAssert( mAgent != NULL );
    mAgent->Release();
}

void ImpulseBasedCollisionSolver::UpdateForSimEnvironment()
{
    mCollisionDistance = SimEnvironment::GetDefaultSimEnvironment()->CollisionDistance();
    mGravityNorm = SimEnvironment::GetDefaultSimEnvironment()->GravityNorm();
}

void ImpulseBasedCollisionSolver::SetCollisionSolverAgent(CollisionSolverAgent* inAgent)
{
    rAssert(inAgent);
    
    if (mAgent)
        mAgent->Release();
    
    mAgent = inAgent;
    mAgent->AddRef();
}

void ImpulseBasedCollisionSolver::SolveCollisions(TArray<Collision>* inList, float dt_sec, float gameTime)
{
    rAssert( mAgent != NULL );
    if( mAgent == NULL )
    {
        return;
    }
    mAgent->Update(dt_sec, gameTime);
    
    if (inList->GetSize() == 0) 
        return;
    
    mCollisionList = inList;
    
    mCollisionList->Lock(); // can't add or remove collisions while in here
    
    mAgent->StartSolvingCollisionList(mCollisionList, gameTime);
    
    int nbiter = 0;
    mFirstPass = true;
    
    do
    {
        mAllSolved = true;
        for (int i=0; i<mCollisionList->GetSize(); i++)
        {
            if (!(mCollisionList->GetAt(i).mStatus & Collision_PhysicsSolved))
            {
                mCollision = mCollisionList->GetAt(i); // take a copy of the current collision
                
                if (mAgent->PreCollisionEvent(mCollision, nbiter+1) == Solving_Aborted)
                {
                    mCollisionList->GetAt(i).mStatus |= Collision_PhysicsSolved;
                    continue;
                }
                
                mSimStateA = mCollision.mCollisionObjectA->GetSimState();
                mSimStateB = mCollision.mCollisionObjectB->GetSimState();
                mSimObjectA = mSimStateA->GetSimulatedObject(mCollision.mIndexA);
                mSimObjectB = mSimStateB->GetSimulatedObject(mCollision.mIndexB);

                mFlexCollision = false;
                bool aflex = false, bflex = false;
                if (mSimObjectA && mSimObjectA->Type() == FlexibleObjectType)
                    mFlexCollision = aflex = true;
                if (mSimObjectB && mSimObjectB->Type() == FlexibleObjectType)
                    mFlexCollision = bflex = true;

                if (mSimStateA == mSimStateB)
                {
                    if (aflex && !bflex)
                        mSimObjectB = NULL;
                    if (bflex && !aflex)
                        mSimObjectA = NULL;
                }
                
                ExtractVelocities();
                
                if (mFirstPass)
                {
                    Vector tmp;
                    tmp.ScaleAdd(mDVelocity, -mNormDVelocityN, mCollision.mNormal);
                    float ndvt = tmp.Magnitude();
                    
                    if (mAgent->CollisionEvent(   mSimStateA, mCollision.mIndexA, 
                                                  mSimStateB, mCollision.mIndexB, 
                                                  mCollision.GetPositionA(), 
                                                  mNormDVelocityN, ndvt,
                                                  &mSimObjectA, &mSimObjectB
                                                  )     == Solving_Aborted)
                    {
                        mCollisionList->GetAt(i).mStatus |= Collision_PhysicsSolved;
                        continue;
                    }
                }
                
                if (mSimObjectA != NULL || mSimObjectB != NULL)
                {
                    SolveCollisionImpulse();
                    mCollisionList->GetAt(i).mStatus = mCollision.mStatus;
                }
                else
                {
                    mCollisionList->GetAt(i).mStatus |= Collision_PhysicsSolved;
                }
            }
        }
        nbiter++;
        mFirstPass = false;
    } while (mAllSolved == false && nbiter < mMaxPasses);
    
    mAgent->EndSolvingCollisionList(mCollisionList, 0);
    
    mCollisionList->Unlock();
}

void ImpulseBasedCollisionSolver::ExtractVelocities()
{
    mSimStateA->GetVelocity(mCollision.GetPositionA(), mVelocityA, mCollision.mIndexA);
    mSimStateB->GetVelocity(mCollision.GetPositionB(), mVelocityB, mCollision.mIndexB);
    mDVelocity.Sub(mVelocityA, mVelocityB);
    mNormDVelocityN = mDVelocity.DotProduct(mCollision.mNormal);
}

void ImpulseBasedCollisionSolver::SolveCollisionImpulse()
{
    rAssert( mAgent != NULL );
    mSelfCollision = (mSimObjectA == mSimObjectB);
    
    if (mNormDVelocityN >= -VERY_SMALL) 
        return;  // objects are moving apart already, no impulse needed
    
    if (mFirstPass)
    {
        if (mSimObjectA && mSimStateA->GetControl() != simSimulationCtrl)
        {
            if (mCollision.mIndexA == 0 && mSimStateA->GetControl() != simSimulationCtrl)
            {
                // if non articulated object and can't switch full physics, the object can't be affected
                mSimObjectA = NULL;
            }
        }
        if (mSimObjectB && mSimStateB->GetControl() != simSimulationCtrl)
        {
            if (mCollision.mIndexB == 0 && mSimStateB->GetControl() != simSimulationCtrl)
            {
                // if non articulated object and can't switch full physics, the object can't be affected
                mSimObjectB = NULL;
            }
        }
        
        if (mSimObjectA == NULL && mSimObjectB == NULL)
        {
            mCollision.mStatus |= Collision_PhysicsSolved;
            return;
        }
    }
    
    StartObj(mSimObjectA, mSimObjectB, mCollision.GetPositionA(), mMassMatrixA, mCollision.mIndexA, mCollision.mIndexB);
    StartObj(mSimObjectB, mSimObjectA, mCollision.GetPositionB(), mMassMatrixB, mCollision.mIndexB, mCollision.mIndexA);
    
    bool oka = false, okb = false;
    bool testCacheA, testCacheB;
    testCacheA = testCacheB = mFirstPass;

    mAllSolved = false;
    
    do
    {
        if (mCollision.mDistance<0.0f)
        {
            float d_dist=mCollision.mDistance;
            if (mFlexCollision && PositionCorrectionForInterpenetration(d_dist) )
            {
                //When PositionCorrectionForInterpenetration return true, 
                //relative velocy may have been altered. Must recomputed it.
                ExtractVelocities();
                mCollision.mDistance += d_dist;
                if (mNormDVelocityN >= -VERY_SMALL) 
                    return;  // objects are moving apart already, no impulse needed
            }
        }
        
        if (ComputeImpulse() == Solving_Aborted)
        {
            mCollision.mStatus |= Collision_PhysicsSolved;
            return;
        }
        
        oka = AddImpulse(mSimObjectA, mSimObjectB, mCollision.GetPositionA(), mMassMatrixA, mCollision.mIndexA, mCollision.mIndexB);
        if (oka)
        {
            if (testCacheA)
            {
                oka = mAgent->TestCache(mSimStateA, mCollision.mIndexA)==Solving_Aborted?false:true;
                testCacheA = false;
            }
            if (oka)
            {
                mImpulse.Scale(-1.0f);
                okb = AddImpulse(mSimObjectB, mSimObjectA, mCollision.GetPositionB(), mMassMatrixB, mCollision.mIndexB, mCollision.mIndexA);
                if (okb)
                {
                    if (testCacheB)
                    {
                        okb = mAgent->TestCache(mSimStateB, mCollision.mIndexB)==Solving_Aborted?false:true;
                        testCacheB = false;
                    }
                }
                else if (mSimObjectB->SpeedAdjustmentFromCache(mCollision.GetPositionB(), mVelocityB, mCollision.mIndexB))
                {
                    mDVelocity.Sub(mVelocityA, mVelocityB);
                    mNormDVelocityN = mDVelocity.DotProduct(mCollision.mNormal);
                }
            }
        }
        else if (mSimObjectA->SpeedAdjustmentFromCache(mCollision.GetPositionA(), mVelocityA, mCollision.mIndexA))
        {
            mDVelocity.Sub(mVelocityA, mVelocityB);
            mNormDVelocityN = mDVelocity.DotProduct(mCollision.mNormal);
        }
        /*
        if ((!oka && mSimStateA->IsArticulated()) || (!okb && mSimStateB->IsArticulated()))
        {
            ExtractVelocity()
            if (mNormDVelocityN >= -VERY_SMALL) 
                return;  // objects are moving apart already, no impulse needed
        }
        */

    } while( (!oka || !okb) && !(mCollision.mStatus & Collision_PhysicsSolved));
    
    if (mSimObjectA == mSimObjectB)
    {
        EndObj(mSimObjectA, mCollision.mIndexA, mCollision.mIndexB, mFullDynColl);
        EndObj(mSimObjectB, mCollision.mIndexB, mCollision.mIndexA, mFullDynColl);
    }
    else
    {
        EndObj(mSimObjectA, mCollision.mIndexA, -1, mFullDynColl);
        EndObj(mSimObjectB, mCollision.mIndexB, -1, mFullDynColl);
    }
}

void ImpulseBasedCollisionSolver::StartObj(SimulatedObject* inObject, SimulatedObject* otherObject, const Vector& inPosition, SymMatrix& inMassMatrix, int inIndex, int otherIndex)
{
    if (inObject)
    {
        inObject->StartCollision(inPosition, mCollision.mNormal, otherObject, inIndex, otherIndex);
        inObject->GetMassMatrix(inPosition, inMassMatrix, otherObject, inIndex, otherIndex);
    }
    else
    {
        inMassMatrix.Zero();
    }
}

bool ImpulseBasedCollisionSolver::AddImpulse(SimulatedObject* inObject, SimulatedObject* otherObject, const Vector& inPosition, SymMatrix& inMassMatrix, int inIndex, int otherIndex)
{
    bool ret = true;
    
    if (inObject)
    {
        if (!inObject->AddImpulse(inPosition, mImpulse, inMassMatrix, otherObject, inIndex, otherIndex))
        {
            inObject->GetMassMatrix(inPosition, inMassMatrix, otherObject, inIndex, otherIndex);
            ret = false;
        }
    }
    return ret;
}

void ImpulseBasedCollisionSolver::EndObj(SimulatedObject* inObject, int inIndex, int otherIndex, bool fdyn)
{
    rAssert( mAgent != NULL );
    if (inObject && !(mCollision.mStatus & Collision_PhysicsSolved))
    {
        if (mAgent->EndObjectCollision(inObject->GetSimState(), inIndex) != Solving_Aborted)
            inObject->EndCollision(fdyn, inIndex, otherIndex);
    }
}

//Compute impulse with the help of Chatterjee formulas.
Solving_Answer ImpulseBasedCollisionSolver::ComputeImpulse()
{
    rAssert( mAgent != NULL );
    mImpulse.Clear();
    
    mMassMatrixTot.Sum(mMassMatrixA, mMassMatrixB);
    if (mMassMatrixTot.xx == 0.0f)
    {
        return Solving_Aborted;
    }
    
    float e   = RestCoeff()-1.0f; //Normally e should be between 0 and 1 however we can use the ACR.
    float mu  = FrictCoeff();   //Classical friction coefficient.
    float e_t = TangRestCoeff();   // Tangencial restitution coeff. must be -1 <= e_t <= 1.
    // For -1 there is no friction allowed.
    // if -1 < e_t <= 0 no reversal speed possible. 
    // else reversal speed is possible. The behavior depend also on mu.
    rAssert( e>=0.0f );
    rAssert( mu>=0.0f );
    rAssert(e_t>=-1.0f && e_t<=1.0f );
    
    float nMn;
    Vector l_Normal, l_v, P_1, P_2;
    
    l_Normal = mCollision.mNormal;
    
    //In Chatterjee M has unit of mass while in Mirtich it has unit of inverse of mass.
    SymMatrix invMx;
    invMx.Invert(mMassMatrixTot);
    
    mMassMatrixTot.TransformVector( l_Normal, l_v );
    nMn = l_Normal.DotProduct(l_v);
    
    P_1 = l_Normal;
    P_1.Scale( -mNormDVelocityN/nMn );
    
    if (mQuickSolve)
    {
        if( mu == 0 )
        {
            mImpulse = P_1;
            return mAgent->TestImpulse(mImpulse, mCollision);
        }
        else
        {
            //This is probably not the same behavior as in the preceeding
            //impulse evaluation for the quick case. But it seems to 
            //to be a good compromise between the full evaluation and the fact
            //that we want some friction cheaply. 
            //Note: This is the impulse candidate evaluated on the 
            //stiking line ( e_t = 0 ). This choise must be tested.
            invMx.TransformVector( mDVelocity, P_2 );
            mImpulse.Add( P_2, P_1 );
            mImpulse.Scale( -1.0f );
            mImpulse.ScaleAdd( 1.0f+e, P_1 );
            return mAgent->TestImpulse(mImpulse, mCollision);
        }
    }
    
    invMx.TransformVector( mDVelocity, P_2 );
    mImpulse.Add( P_2, P_1 );
    mImpulse.Scale( -(1.0f+e_t) );
    mImpulse.ScaleAdd( 1.0f+e, P_1 );
    
    //At this point mImpulse is the candidate impulse. We have to look if
    //it is inside the friction cone.
    
    float kappa;
    
    l_v.ScaleAdd( mImpulse, -l_Normal.DotProduct(mImpulse), l_Normal );
    float test = l_v.DotProduct(l_v);
    if( test > Sqr(mu*l_Normal.DotProduct(mImpulse)) )
    {
        l_v.ScaleAdd( P_2, l_Normal.DotProduct(P_2), l_Normal );
        kappa = l_v.Magnitude();
        l_v.Add(P_2,P_1);
        kappa = kappa + mu*l_Normal.DotProduct(l_v) ;
        rAssert(kappa != 0.0f );
        if( kappa == 0.0f )
        {
            mImpulse.Clear();
            return Solving_Aborted;
        }
        kappa = mu*(1.0f+e)*l_Normal.DotProduct(P_1) / kappa;
    }
    else
        kappa = 1.0f + e_t;
    
    mImpulse.Add(P_2, P_1);
    mImpulse.Scale(-kappa);
    mImpulse.ScaleAdd( (1.0f+e), P_1 );
    
    return mAgent->TestImpulse(mImpulse, mCollision);
}

float ImpulseBasedCollisionSolver::RestCoeff()
{
    float rest;
    static bool dynrest = true;
    float dist = mCollision.mDistance;
    
    /*
    The coeff of restitution is artificially increase to ensure a static collision impulse 
    in gravity won't interpenetrate. Test following test will modify the coeff if the collision normal
    is oriented along the gravity vector and the dvn is smaller than minv, the speed the gravity
    would give to an object when applied on a distance ~ mCollisionDistance (so we don't need the time step here)
    */
    
    float trueRest = PhysicsProperties::CombinedRestCoeff(mCollision.mCollisionObjectA->GetPhysicsProperties(), mCollision.mCollisionObjectB->GetPhysicsProperties());
    trueRest = Max(MINREST, trueRest);
    
    float trueRestDist = mCollisionDistance;
    float f1 = 1.0f;

    if (mFlexCollision)
    {
        static float flexdistscale = 0.0f;
        trueRestDist *= flexdistscale;
        //float f1 = Fabs(mCollision.mNormal.y);
    }

    if (dist < trueRestDist)
    {
        float minv;
        dist = trueRestDist - dist;
        minv = Sqrt(2.0f*dist*mGravityNorm);
        static int test = 0;
        if (Fabs(mNormDVelocityN) * (trueRest-1.0f) <= minv * f1 )
        {
            mFullDynColl = dynrest;
            rest = f1 * 0.95f * minv/Fabs(mNormDVelocityN);
            return rest+1.0f;
        }
    }
    mFullDynColl = true;
    
    return trueRest;
}

float ImpulseBasedCollisionSolver::FrictCoeff()
{
    if (mSelfCollision)
        return 0.0f;
    else
    {
        return PhysicsProperties::CombinedFrictCoeff(mCollision.mCollisionObjectA->GetPhysicsProperties(), mCollision.mCollisionObjectB->GetPhysicsProperties());
    }
}

float ImpulseBasedCollisionSolver::TangRestCoeff()
{
    if (mSelfCollision)
        return 0;
    else
    {
        if( mSimObjectA && mSimObjectB )
        {
            return PhysicsProperties::CombinedTangRestCoeff(mCollision.mCollisionObjectA->GetPhysicsProperties(), mCollision.mCollisionObjectB->GetPhysicsProperties());
        }
        else if (mSimObjectA)
        {
            return mCollision.mCollisionObjectA->TangRestCoeff();
        }
        else
        {
            return mCollision.mCollisionObjectB->TangRestCoeff();
        }
    }
}

bool ImpulseBasedCollisionSolver::PositionCorrectionForInterpenetration( float &io_dist )
{
    Collision          l_Collision;
    SimulatedObject   *l_ObjectA, *l_ObjectB;  
    SymMatrix         *l_MassMatrixA, *l_MassMatrixB;
    Vector            l_positionB, l_Normal;
    int               l_indexA, l_indexB;
    
    if ( (mSimObjectA!=NULL && mSimObjectA->Type()==FlexibleObjectType) && (mSimObjectB!=NULL && mSimObjectB->Type()==FlexibleObjectType) )
    {
        static int error=1;
        //rAssertMsg(error--,"Not Implemented","");
        return false;
    }
    else if ( mSimObjectA!=NULL && mSimObjectA->Type()==FlexibleObjectType )
    {
        l_ObjectA=mSimObjectA;
        l_ObjectB=mSimObjectB;
        l_MassMatrixA=&mMassMatrixA;
        l_MassMatrixB=&mMassMatrixB;
        l_indexA=mCollision.mIndexA;
        l_indexB=mCollision.mIndexB;
        l_positionB=mCollision.GetPositionB();
        l_Normal = mCollision.mNormal;
    }
    else if ( mSimObjectB!=NULL && mSimObjectB->Type()==FlexibleObjectType )
    {
        l_ObjectA=mSimObjectB;
        l_ObjectB=mSimObjectA;
        l_MassMatrixA=&mMassMatrixB;
        l_MassMatrixB=&mMassMatrixA;
        l_indexA=mCollision.mIndexB;
        l_indexB=mCollision.mIndexA;
        l_positionB=mCollision.GetPositionA();
        l_Normal = mCollision.mNormal;
        l_Normal.Scale(-1.0f);
    }
    else
    {
        return false;
    }
    
    Vector l_P;
    //If there is interpenetration, 
    //the colliding particle will be moved to avoid interpenetration. 
    //This displacement will be done through a virtual impulse applied on the particle.
    //This impulse is get back in mImpulse (ready to apply on the rigid body), 
    //and then applied on the rigid body in order to satisfy the law of 
    //momentum preservation.
    l_ObjectA->ResolveInterpenetration(l_indexA, io_dist, l_Normal, mImpulse, true, *l_ObjectB);
    
    if (l_ObjectB)
    {
        StartObj(l_ObjectB, l_ObjectA, l_positionB, *l_MassMatrixB, l_indexB, l_indexA);
        AddImpulse(l_ObjectB, l_ObjectA, l_positionB, *l_MassMatrixB, l_indexB, l_indexA);
        EndObj(l_ObjectB, l_indexB, -1, mFullDynColl);
    }
    mImpulse.Clear();
    return true; //Relative velocity have changed, return true.
}

//
//
//

CollisionSolverAgent::CollisionSolverAgent()
{
    mAnalyser = NULL;
}

CollisionSolverAgent::~CollisionSolverAgent()
{
    DisableCollisionAnalyser();
}

void CollisionSolverAgent::StartSolvingCollisionList(TArray<Collision>* inList, float current_time)
{
    if(mAnalyser)
        mAnalyser->StartSolvingCollisionList(inList, current_time);
}

void CollisionSolverAgent::EndSolvingCollisionList(TArray<Collision>* inList, float current_time)
{
    if(mAnalyser)
        mAnalyser->EndSolvingCollisionList(inList, current_time);
}

Solving_Answer CollisionSolverAgent::PreCollisionEvent(Collision& inCollision, int inPass) 
{
    Solving_Answer ret = Solving_Continue;
    // as far as simulation go, there is no need to do anything else 
    // with this collision if the is no simulated object associated with it.
    if (inCollision.GetSimulatedObjectA() == NULL && inCollision.GetSimulatedObjectB() == NULL)
    {
        if(!mAnalyser) // or you may have other reason to keep going...
            ret = Solving_Aborted;
    }
    
    if(mAnalyser)
        mAnalyser->PreCollisionEvent(inCollision, inPass);
    
    return ret;
}

inline void AdjustSimStateControl(SimState* inSimState, SimulatedObject* simObject)
{
    if (simObject->Type() == RigidObjectType)
    {
        if (inSimState->GetControl() == simAICtrl)
        {
            inSimState->SetControl(simSimulationCtrl);
        }
    }
    else if (simObject->Type() == ArticulatedObjectType)
    {
        // this allow to enable full physics articulated objects
        /*
        if (inSimState->GetControl() == sim::simAICtrl)
        {
            inSimState->SetControl(sim::simSimulationCtrl);
        }
        */
    }
}

Solving_Answer CollisionSolverAgent::CollisionEvent( SimState* inSimStateA, int indexA, 
    SimState* inSimStateB, int indexB, 
    const rmt::Vector& inPos, 
    float inDvN, float inDvT,
    SimulatedObject** simA, SimulatedObject** simB)
{
    // this is the most important callback. 
    // This is the time to set the rigid body control to simulationControl if not the case
    // this is also the time to generate appropriate sound if necessary
    
    // the index indicates on which articulation of the skeleton the collision occured or,
    // or if index == -1, in indicates that the object is a rigid body.
    
    if (*simA)
    {
        //AdjustSimStateControl(inSimStateA, *simA);
    }
    if (*simB)
    {
        //AdjustSimStateControl(inSimStateB, *simB);
    }
    
    if(mAnalyser)
        mAnalyser->CollisionEvent(inSimStateA, indexA, inSimStateB, indexB, inPos, inDvN, inDvT);
    
    return Solving_Continue;
}

Solving_Answer CollisionSolverAgent::TestImpulse(rmt::Vector& mImpulse, Collision& inCollision)
{
    // this is called everytime an impulse is computed, before it gets added to the objects
    return Solving_Continue;
}

Solving_Answer CollisionSolverAgent::TestCache(SimState* inSimState, int inIndex)
{
    // this is called everytime an impulse is added on an object. Getting the cache give information 
    // on how object's speed will be affected by the last impulse added into the cache.
    
    SimulatedObject* simobj = inSimState->GetSimulatedObject();
    

    if ( simobj && (simobj->Type() == RigidObjectType ))//|| simobj->Type() == ArticulatedObjectType) )
    {
        static float thresholdfactor1 = 100.0f;
        Vector currentv, vcmv, cachev, cachew;
        currentv = inSimState->GetLinearVelocity(); //current speed of the object
        if (inSimState->GetVirtualCM()==NULL) 
            vcmv.Clear();
        else
            vcmv = inSimState->GetVirtualCM()->GetVelocity(); //The current speed of the vcm of the object
        simobj->GetCollisionCache(cachev, cachew, -1); //retrieve the collision cache of the physical object.
        cachev.Add(vcmv); //The combined vcm's speed and the cached speed.
        float testSpeed = Max(currentv.DotProduct(currentv),Sqr(simobj->GetMinimumLinSpeed()));
        if ( cachev.DotProduct(cachev) > thresholdfactor1*testSpeed )
        {
            inSimState->SetControl(simSimulationCtrl);
            return Solving_Aborted;
        }
    }

    if (simobj && simobj->Type() == ArticulatedObjectType)
    {
        //Looks if there is a vcm installed on this joint:
        SimStateArticulated *simStateArt = (SimStateArticulated*)inSimState;
        JointVCMpArray *vcma = &simStateArt->mVirtualCMList;
        for (int i=0 ; i<vcma->GetSize() ; i++)
        {
            JointVirtualCM *vcm = vcma->GetAt(i);
            if (vcm->GetIndex() == inIndex)
            {
                static float thresholdfactor2=100.0f;
                Vector currentv, vcmv, cachev, cachew;
                simStateArt->GetVelocity(vcm->GetPosition(), currentv, inIndex);
                vcmv = vcm->GetVelocity();
                simobj->GetCollisionCache(cachev, cachew, inIndex);
                if ( cachev.DotProduct(cachev) > thresholdfactor2*currentv.DotProduct(currentv) )
                {
                    simStateArt->SetControl(simSimulationCtrl);
                    return Solving_Aborted;
                }
            }
        }
    }
    return Solving_Continue;
}

Solving_Answer CollisionSolverAgent::EndObjectCollision(SimState* inSimState, int inIndex)
{
    // the impulse has been added to the object's cache, testing that cache tells 
    // the consequence of adding the cache to the object's state.
    
    // the index indicates the collision articulation if index > 0 
    // or collision on a rigid body if index == 0
    
    SimulatedObject* object = inSimState->GetSimulatedObject(inIndex);
    
    if (object->Type() != FlexibleObjectType)
    {
        if (inSimState->GetControl() == simSimulationCtrl)
        {
            if (object->IsAtRest() && inIndex <= 0)
            {
                Vector v, w;
                object->GetCollisionCache(v, w, -1);

                VirtualCM *vcm = inSimState->GetVirtualCM(-1);
                if (vcm)
                {
                    inSimState->AddCacheToVirtualCM(v, w, inIndex);
                    v = vcm->GetVelocity();
                    w = vcm->GetAngularVelocity();
                }
                //Do not add an impulse on an object being at rest if the contribution of
                //the impulse will change its speed less than some threshold value.
                if (v.DotProduct(v) <= Sqr(2.0f*object->GetMinimumLinSpeed()) && w.DotProduct(w) <= Sqr(2.0f*object->GetMinimumAngSpeed()))
                {
                    object->ResetCache();
                    return Solving_Aborted;
                }
            }
        }
        else
        {
            object->ResetCache();
        }
    }
    return Solving_Continue;
}


void CollisionSolverAgent::EnableCollisionAnalyser(void)
{
    if(!mAnalyser) {
        mAnalyser = new CollisionAnalyser();
    }
}

void CollisionSolverAgent::DisableCollisionAnalyser(void)
{
    if(mAnalyser) {
        delete mAnalyser;
        mAnalyser = NULL;
    }
}

} // sim

