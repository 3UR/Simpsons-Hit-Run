#ifndef _COLLISIONANALYSERDATA_HPP_
#define _COLLISIONANALYSERDATA_HPP_

#include "simcollision/collisionanalyserdataUID.hpp"
#include "simcollision/collisionanalyserevent.hpp"
#include "simcommon/tarray.hpp"

namespace sim
{

typedef TArray<CollisionAnalyserEvent> CollisionAnalyserEventList;

typedef int CollisionAnalyserPossibleEvent;
const CollisionAnalyserPossibleEvent cCollisionAnalyserPossibleEventBouncing = 0x01;
const CollisionAnalyserPossibleEvent cCollisionAnalyserPossibleEventSliding  = 0x02;
const CollisionAnalyserPossibleEvent cCollisionAnalyserPossibleEventRolling  = 0x04;
const CollisionAnalyserPossibleEvent cCollisionAnalyserPossibleEventSpinning = 0x08;
const CollisionAnalyserPossibleEvent cCollisionAnalyserPossibleEventInteracting =
    cCollisionAnalyserPossibleEventSliding |
    cCollisionAnalyserPossibleEventRolling |
    cCollisionAnalyserPossibleEventSpinning;
const CollisionAnalyserPossibleEvent cCollisionAnalyserPossibleEventAll =
    cCollisionAnalyserPossibleEventBouncing |
    cCollisionAnalyserPossibleEventSliding |
    cCollisionAnalyserPossibleEventRolling |
    cCollisionAnalyserPossibleEventSpinning;
const CollisionAnalyserPossibleEvent cCollisionAnalyserPossibleEventNone = 0;

typedef int CollisionAnalyserTerminatingStatus;
const CollisionAnalyserTerminatingStatus cTerminatingStatusLive = 0;
const CollisionAnalyserTerminatingStatus cTerminatingStatusTerminating = 1;
const CollisionAnalyserTerminatingStatus cTerminatingStatusReadyToTerminate = 2;

class CollisionAnalyserData {
    //hold data about a particular collision pair
public:
    CollisionAnalyserData();
    CollisionAnalyserData(const CollisionAnalyserData& data);
    CollisionAnalyserData(
        SimState *newSimStateA, int newIndexA,
        SimState *newSimStateB, int newIndexB,
        CollisionAnalyserPossibleEvent newPossibleEvent);
    CollisionAnalyserData& operator=(const CollisionAnalyserData& rhs);
    ~CollisionAnalyserData();

    void Init(
        SimState *newSimStateA, int newIndexA,
        SimState *newSimStateB, int newIndexB,
        CollisionAnalyserPossibleEvent newPossibleEvent);

    CollisionAnalyserDataUID GetUID(void)
        { return CollisionAnalyserDataUID(mSimStateA, mIndexA, mSimStateB, mIndexB); }
    void CalculateState(void);
    void Processed(void);

    void AddEvent(CollisionAnalyserEventEnum newEvent, CollisionAnalyserInfo &newInfo);
    float GetLastModifiedTime(void) { return mThisTime; }

    bool ResetValues() { return mResetValues; }
    void Set(
        const rmt::Vector& newCollisionPosition,
        const rmt::Vector& newCollisionNormal,
        float newDvN, float newDvT, float newTime) {
        mInfo.mCollisionPosition = newCollisionPosition;
        mInfo.mCollisionNormal = newCollisionNormal;
        mRunningDvN = newDvN;
        mRunningDvT = newDvT;
        mThisTime = newTime;
        mNumBatchedValues = 1;
        mResetValues = false;
    }
    void Add(
        const rmt::Vector& newCollisionPosition,
        const rmt::Vector& newCollisionNormal,
        float newDvN, float newDvT) {
        mInfo.mCollisionPosition.Add(newCollisionPosition);
        mInfo.mCollisionNormal.Add(newCollisionNormal);
        mRunningDvN += newDvN;
        mRunningDvT += newDvT;
        mNumBatchedValues++;
    }

    void NotifyShutdown(void);
    bool ReadyToTerminate(void);

    CollisionAnalyserEventList mEventList;
    CollisionAnalyserInfo mInfo;
    SimState* mSimStateA;  //which simstate this data refers to
    SimState* mSimStateB;  //which simstate this data refers to
    int mIndexA;
    int mIndexB;

    CollisionAnalyserPossibleEvent mPossibleEvent;
    static CollisionAnalyserPossibleEvent sDefaultCollisionType;
private:
    bool mResetValues;
    int mNumBatchedValues;
    float mRunningDvN, mRunningDvT;
    float mThisTime, mLastTime;
    float mLastBounceForce;

    CollisionAnalyserTerminatingStatus mTerminatingStatus;
};

class CollisionAnalyserDataIndex {
public:
    CollisionAnalyserDataIndex()
        :mUID(0, 0, 0, 0), mData(NULL) {;}
    CollisionAnalyserDataIndex(
        SimState *newSimStateA, int newIndexA,
        SimState *newSimStateB, int newIndexB,
        CollisionAnalyserData* newData)
        :
        mUID(newSimStateA, newIndexA, newSimStateB, newIndexB),
        mData(newData)
    {;}
    CollisionAnalyserDataIndex(
        CollisionAnalyserDataUID UID,
        CollisionAnalyserData* newData)
        :
        mUID(UID),
        mData(newData)
    {;}
    void Init(
        SimState *newSimStateA, int newIndexA,
        SimState *newSimStateB, int newIndexB,
        CollisionAnalyserData* newData)
    {
        mUID = CollisionAnalyserDataUID(newSimStateA, newIndexA, newSimStateB, newIndexB);
        mData = newData;
    }

    CollisionAnalyserDataUID mUID;
    CollisionAnalyserData *mData;
};

}

#endif