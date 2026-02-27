#include "simcommon/simlistinitsize.hpp"
#include "simcommon/simstate.hpp"
#include "simcollision/collisionanalyserdata.hpp"

namespace sim
{

CollisionAnalyserPossibleEvent CollisionAnalyserData::sDefaultCollisionType = cCollisionAnalyserPossibleEventNone;

CollisionAnalyserData::CollisionAnalyserData()
:mEventList(CollisionAnalyserEventListSize),
mInfo(),
mSimStateA(NULL),
mSimStateB(NULL),
mIndexA(0),
mIndexB(0),
mPossibleEvent(cCollisionAnalyserPossibleEventNone),
mNumBatchedValues(0),
mRunningDvN(0),
mRunningDvT(0),
mThisTime(0),
mLastTime(0),
mLastBounceForce(0),
mTerminatingStatus(cTerminatingStatusLive)
{
    //set mPossibleEvent status
    if(mPossibleEvent & (cCollisionAnalyserPossibleEventSliding | cCollisionAnalyserPossibleEventRolling)) {
        //if only one or the other is wanted, pre-set factors
        if(!(mPossibleEvent & cCollisionAnalyserPossibleEventRolling)) {
            //no rolling wanted; sliding only
            mInfo.mSlidingFactor = 1;
            mInfo.mRollingFactor = 0;
        }
        else if(!(mPossibleEvent & cCollisionAnalyserPossibleEventSliding)) {
            //no sliding wanted; rolling only
            mInfo.mSlidingFactor = 0;
            mInfo.mRollingFactor = 1;
        }
    }
    mEventList.DontUseMemCpy();
}

CollisionAnalyserData::CollisionAnalyserData(const CollisionAnalyserData& data)
:mEventList(CollisionAnalyserEventListSize),
mInfo(data.mInfo),
mSimStateA(data.mSimStateA),
mSimStateB(data.mSimStateB),
mIndexA(data.mIndexA),
mIndexB(data.mIndexB),
mPossibleEvent(data.mPossibleEvent),
mNumBatchedValues(data.mNumBatchedValues),
mRunningDvN(data.mRunningDvN),
mRunningDvT(data.mRunningDvT),
mThisTime(data.mThisTime),
mLastTime(data.mLastTime),
mLastBounceForce(data.mLastBounceForce),
mTerminatingStatus(data.mTerminatingStatus)
{
    //set mPossibleEvent status
    if(mPossibleEvent & (cCollisionAnalyserPossibleEventSliding | cCollisionAnalyserPossibleEventRolling)) {
        //if only one or the other is wanted, pre-set factors
        if(!(mPossibleEvent & cCollisionAnalyserPossibleEventRolling)) {
            //no rolling wanted; sliding only
            mInfo.mSlidingFactor = 1;
            mInfo.mRollingFactor = 0;
        }
        else if(!(mPossibleEvent & cCollisionAnalyserPossibleEventSliding)) {
            //no sliding wanted; rolling only
            mInfo.mSlidingFactor = 0;
            mInfo.mRollingFactor = 1;
        }
    }

    mEventList.DontUseMemCpy();
}

CollisionAnalyserData::CollisionAnalyserData(
    SimState *newSimStateA, int newIndexA,
    SimState *newSimStateB, int newIndexB,
    CollisionAnalyserPossibleEvent newPossibleEvent)
:mEventList(CollisionAnalyserEventListSize),
mInfo(),
mSimStateA(newSimStateA),
mSimStateB(newSimStateB),
mIndexA(newIndexA),
mIndexB(newIndexB),
mPossibleEvent(newPossibleEvent),
mNumBatchedValues(0),
mRunningDvN(0),
mRunningDvT(0),
mThisTime(0),
mLastTime(0),
mLastBounceForce(0),
mTerminatingStatus(cTerminatingStatusLive)
{
    //set mPossibleEvent status
    if(newPossibleEvent & (cCollisionAnalyserPossibleEventSliding | cCollisionAnalyserPossibleEventRolling)) {
        //if only one or the other is wanted, pre-set factors
        if(!(newPossibleEvent & cCollisionAnalyserPossibleEventRolling)) {
            //no rolling wanted; sliding only
            mInfo.mSlidingFactor = 1;
            mInfo.mRollingFactor = 0;
        }
        else if(!(newPossibleEvent & cCollisionAnalyserPossibleEventSliding)) {
            //no sliding wanted; rolling only
            mInfo.mSlidingFactor = 0;
            mInfo.mRollingFactor = 1;
        }
    }
    mEventList.DontUseMemCpy();
}

CollisionAnalyserData& CollisionAnalyserData::operator=(const CollisionAnalyserData& rhs)
{
    if(this == &rhs)
        return *this;

    int i;
    Processed();
    rAssert(mEventList.GetSize() == 0);
    mEventList.Clear();
    for(i = 0; i < rhs.mEventList.GetSize(); i++) {
        CollisionAnalyserData* data = (CollisionAnalyserData *)&rhs;
        mEventList.Add(CollisionAnalyserEvent(data->mEventList.GetAt(i)));
    }
    mInfo = rhs.mInfo;
    mSimStateA = rhs.mSimStateA;
    mSimStateB = rhs.mSimStateB;
    mIndexA = rhs.mIndexA;
    mIndexB = rhs.mIndexB;
    mPossibleEvent = rhs.mPossibleEvent;

    mResetValues = rhs.mResetValues;
    mNumBatchedValues = rhs.mNumBatchedValues;
    mRunningDvN = rhs.mRunningDvN;
    mRunningDvT = rhs.mRunningDvT;
    mThisTime = rhs.mThisTime;
    mLastTime = rhs.mLastTime;

    mLastBounceForce = rhs.mLastBounceForce;

    mTerminatingStatus = rhs.mTerminatingStatus;
    return *this;
}

CollisionAnalyserData::~CollisionAnalyserData()
{
    //clean out the event list
    Processed();
}

void CollisionAnalyserData::Init(
    SimState *newSimStateA, int newIndexA,
    SimState *newSimStateB, int newIndexB,
    CollisionAnalyserPossibleEvent newPossibleEvent)
{
    mEventList.Clear();
    mSimStateA = newSimStateA;
    mSimStateB = newSimStateB;
    mIndexA = newIndexA;
    mIndexB = newIndexB;
    mPossibleEvent = newPossibleEvent;
    mNumBatchedValues = 0;
    mRunningDvN = 0;
    mRunningDvT = 0;
    mThisTime = 0;
    mLastTime = 0;
    mLastBounceForce = 0;
    mTerminatingStatus = cTerminatingStatusLive;

    //set mPossibleEvent status
    if(newPossibleEvent & (cCollisionAnalyserPossibleEventSliding | cCollisionAnalyserPossibleEventRolling)) {
        //if only one or the other is wanted, pre-set factors
        if(!(newPossibleEvent & cCollisionAnalyserPossibleEventRolling)) {
            //no rolling wanted; sliding only
            mInfo.mSlidingFactor = 1;
            mInfo.mRollingFactor = 0;
        }
        else if(!(newPossibleEvent & cCollisionAnalyserPossibleEventSliding)) {
            //no sliding wanted; rolling only
            mInfo.mSlidingFactor = 0;
            mInfo.mRollingFactor = 1;
        }
    }
}

void CollisionAnalyserData::CalculateState(void)
{
    //if there's no data to evaluate, just leave it
    if(mNumBatchedValues == 0)
        return;

    if(mNumBatchedValues > 1) {
        float inverse = 1.0f / mNumBatchedValues;
        mRunningDvT *= inverse;
        mRunningDvN *= inverse;
        mInfo.mCollisionPosition.Scale(inverse);
        mInfo.mCollisionNormal.Scale(inverse);
    }

    mInfo.mDvT = (0.95f * mInfo.mDvT) + (0.05f * mRunningDvT);
    if(mInfo.mDvT == 0.0f)    //guard against division by zero
        mInfo.mDvT = 0.00001f;
    
    mInfo.mDvN = mRunningDvN;

    float timeDiff = mThisTime - mLastTime;

    //check for bouncing
    if(timeDiff > mInfo.mBounceTimeThreshold ||
        (rmt::Fabs(mRunningDvN) > (mLastBounceForce * 2.0f) &&
        (rmt::Fabs(mRunningDvN) > mInfo.mBounceSpeedThreshold))) {
        if(mPossibleEvent & cCollisionAnalyserPossibleEventBouncing) {
            //do a bounce event
            if(mInfo.mState == cCollisionAnalyserStateInteraction) {
                mInfo.mState = cCollisionAnalyserStateUnknown;
                AddEvent(cCollisionAnalyserEventEndInteraction, mInfo);
            }

            CollisionAnalyserInfo newInfo;
            newInfo.mState = cCollisionAnalyserStateBouncing;
            newInfo.mCollisionPosition = mInfo.mCollisionPosition;
            newInfo.mCollisionNormal = mInfo.mCollisionNormal;
            newInfo.mSpeedDiff.Sub(
                mSimStateA->GetLinearVelocity(),
                mSimStateB->GetLinearVelocity());
            newInfo.mDvN = mRunningDvN;
            newInfo.mDvT = mRunningDvT;
            newInfo.mStartTime = mThisTime;

            mLastBounceForce = rmt::Fabs(mRunningDvN);

            AddEvent(cCollisionAnalyserEventBounce, newInfo);

            mNumBatchedValues = 0;
            mResetValues = true;
        }
    }
    else {
        //not bouncing; do interaction calculations
        if(mPossibleEvent & cCollisionAnalyserPossibleEventInteracting) {
            rmt::Vector speedDiff;
            speedDiff.Sub(
                mSimStateA->GetLinearVelocity(),
                mSimStateB->GetLinearVelocity());

            mInfo.mSpeedDiff.Scale(0.95f);
            mInfo.mSpeedDiff.ScaleAdd(0.05f, speedDiff);

            //calculate spinning speeds
            if(mPossibleEvent & cCollisionAnalyserPossibleEventSpinning) {
                if(mPossibleEvent & (cCollisionAnalyserPossibleEventSpinning) << 8) {
                    float speed = (
                        mSimStateA->UpApproxSpeedMagnitude() +
                        mSimStateA->DownApproxSpeedMagnitude()) / 2.0f;
                    mInfo.mSpeedA =
                        (0.95f * mInfo.mSpeedA) + (0.05f * speed);
                    float avgSpeed = (rmt::Fabs(mInfo.mDvT) + mInfo.mSpeedA) / 2.0f;
                    float mag = mSimStateA->GetAngularVelocity().Magnitude() / avgSpeed;
                    if(mag > 15.0f)
                        mInfo.mSpinSpeedA = mag;
                    else
                        mInfo.mSpinSpeedA = 0.0f;
                }

                if(mPossibleEvent & (cCollisionAnalyserPossibleEventSpinning) << 16) {
                    float speed = (
                        mSimStateB->UpApproxSpeedMagnitude() +
                        mSimStateB->DownApproxSpeedMagnitude()) / 2.0f;
                    mInfo.mSpeedB =
                        (0.95f * mInfo.mSpeedB) + (0.05f * speed);
                    float avgSpeed = (rmt::Fabs(mInfo.mDvT) + mInfo.mSpeedB) / 2.0f;
                    float mag = mSimStateB->GetAngularVelocity().Magnitude() / avgSpeed;
                    if(mag > 15.0f)
                        mInfo.mSpinSpeedB = mag;
                    else
                        mInfo.mSpinSpeedB = 0.0f;
                }
            }

            CollisionAnalyserPossibleEvent doCalc = mPossibleEvent & (cCollisionAnalyserPossibleEventSliding | cCollisionAnalyserPossibleEventRolling);
            if(doCalc) {
                if(doCalc == (cCollisionAnalyserPossibleEventSliding | cCollisionAnalyserPossibleEventRolling)) {
                    //need to differentiate between sliding and rolling; calculate sliding/rolling speeds
                    float motionDiff = mInfo.mSpeedDiff.Magnitude() / mInfo.mDvT;
                    if(motionDiff < mInfo.mLowerThreshold) {
                        mInfo.mSlidingFactor = 1.0f;
                        mInfo.mRollingFactor = 0.0f;
                    }
                    else if(motionDiff > mInfo.mUpperThreshold) {
                        mInfo.mSlidingFactor = 0.0f;
                        mInfo.mRollingFactor = 1.0f;
                    }
                    else {
                        float range = mInfo.mUpperThreshold - mInfo.mLowerThreshold;
                        mInfo.mRollingFactor = (motionDiff - mInfo.mLowerThreshold) / range;
                        mInfo.mSlidingFactor = 1.0f - mInfo.mRollingFactor;
                    }
                }
            }

            if(mInfo.mState != cCollisionAnalyserStateInteraction) {
                mInfo.mStartTime = mThisTime;
                mInfo.mState = cCollisionAnalyserStateInteraction;
                AddEvent(cCollisionAnalyserEventStartInteraction, mInfo);
            }

            mLastBounceForce = 0;
        }
    }
    
    mNumBatchedValues = 0;
    mResetValues = true;
    mLastTime = mThisTime;
}

void CollisionAnalyserData::Processed(void)
{
    mEventList.Clear();
    if(mTerminatingStatus == cTerminatingStatusTerminating)
        mTerminatingStatus = cTerminatingStatusReadyToTerminate;
}

void CollisionAnalyserData::AddEvent(CollisionAnalyserEventEnum newEvent, CollisionAnalyserInfo &newInfo)
{
    if(ReadyToTerminate())
        return; //alternatively, you could un-terminate yourself

    if(newEvent == cCollisionAnalyserEventBounce) {
        rAssert(newInfo.mState == cCollisionAnalyserStateBouncing);
    }

    CollisionAnalyserEvent event(newEvent, newInfo);
    mEventList.Add(event);
}

void CollisionAnalyserData::NotifyShutdown(void)
{
    if(mTerminatingStatus != cTerminatingStatusLive)
        return;

    if(mInfo.mState == cCollisionAnalyserStateInteraction) {
        mInfo.mState = cCollisionAnalyserStateUnknown;
        AddEvent(cCollisionAnalyserEventEndInteraction, mInfo);
    }

    mTerminatingStatus = cTerminatingStatusTerminating;
}

bool CollisionAnalyserData::ReadyToTerminate(void)
{
    return (mTerminatingStatus == cTerminatingStatusReadyToTerminate) ? true : false;
}

}