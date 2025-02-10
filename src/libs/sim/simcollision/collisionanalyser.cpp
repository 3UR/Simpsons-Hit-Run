#include "simcommon/simlistinitsize.hpp"
#include "simcollision/collisionanalyser.hpp"
#include "simcollision/collisionanalyserdata.hpp"

namespace sim
{

CollisionAnalyser::CollisionAnalyser()
:mDataIndexList(CollisionAnalyserDataListSize),
mFreeDataList(CollisionAnalyserDataListSize),
mDataList(CollisionAnalyserDataListSize),
mCollisionDataArray(CollisionAnalyserDataListSize)
{
    mDataList.DontUseMemCpy();  //can't use memcpy when CollisionAnalyser contains a TArray
    mIndexBoundHi = NULL;
    mIndexBoundLo = mIndexBoundHi + 1;
    mCachedTime = 0;
    mCollectingData = false;
}

CollisionAnalyser::~CollisionAnalyser()
{
    NotifyShutdown();
}

void CollisionAnalyser::StartSolvingCollisionList(TArray<Collision>* inList, float current_time)
{
    mCachedTime = current_time;
    mCollectingData = true;
}

void CollisionAnalyser::EndSolvingCollisionList(TArray<Collision>* inList, float current_time)
{
    CleanCollisionDataList();
    mCollectingData = false;
    CalculateStateCollisionDataList();
}


CollisionAnalyzer_Answer CollisionAnalyser::PreCollisionEvent(Collision& inCollision, int inPass)
{
    mCachedCollisionNormal = inCollision.mNormal;
    mCachedPossibleEventsA = inCollision.mCollisionObjectA->PossibleCollisionEvents();
    mCachedPossibleEventsB = inCollision.mCollisionObjectB->PossibleCollisionEvents();
    mCachedPossibleEvents = mCachedPossibleEventsA | mCachedPossibleEventsB;

    if(mCachedPossibleEvents == 0)
        return CollisionAnalyzer_Aborted;
    return CollisionAnalyzer_Continue;
}

// this method allows to trigger sounds, animation and modify the objects state.
CollisionAnalyzer_Answer CollisionAnalyser::CollisionEvent(
    SimState* inSimStateA, int indexA, 
    SimState* inSimStateB, int indexB, 
    const rmt::Vector& inPos, float inDvN, float inDvT)
{
    if(mCachedPossibleEvents == 0)
        return CollisionAnalyzer_Aborted;

    //ignore self-collision
    if(inSimStateA == inSimStateB)
        return CollisionAnalyzer_Aborted;

    CollisionAnalyserDataUID UID(inSimStateA, indexA, inSimStateB, indexB);
    CollisionAnalyserData* data = GetData(UID);

    if(data == NULL) {
        //not found; add a new one.
        data = CreateNewData();
        data->Init(inSimStateA, indexA, inSimStateB, indexB, mCachedPossibleEvents | (mCachedPossibleEventsA << 8) | (mCachedPossibleEventsB << 16));

        CollisionAnalyserDataIndex *index = CreateNewDataIndex();
        index->Init(inSimStateA, indexA, inSimStateB, indexB, data);
    }

    if(data->ResetValues())
        data->Set(inPos, mCachedCollisionNormal, inDvN, inDvT, mCachedTime);
    else
        data->Add(inPos, mCachedCollisionNormal, inDvN, inDvT);

    return CollisionAnalyzer_Continue;
}

CollisionAnalyserDataPList& CollisionAnalyser::GetCollisionData()
{
    rAssert(mCollectingData == false);

    mCollisionDataArray.Clear();
    if(mIndexBoundLo <= mIndexBoundHi) {
        CollisionAnalyserDataIndex *p = mIndexBoundLo;
        do {
            mCollisionDataArray.Add(p->mData);
        } while(p++ != mIndexBoundHi);
    }
    return mCollisionDataArray;
}

CollisionAnalyserDataIndex* CollisionAnalyser::CreateNewDataIndex()
{
    CollisionAnalyserDataIndex *newDataIndex = &mDataIndexList.NewElem();
    //get the new boundaries of the array
    mIndexBoundLo = &mDataIndexList[0];
    mIndexBoundHi = &mDataIndexList[mDataIndexList.GetSize()-1];
    return newDataIndex;
}

CollisionAnalyserData* CollisionAnalyser::CreateNewData()
{
    //look in free list
    int size = mFreeDataList.GetSize();
    if(size != 0) {
        CollisionAnalyserData *newData = mFreeDataList[size-1];
        mFreeDataList.RemoveAt(size-1);
        return newData;
    }

    //not in free list: gotta add a new data to the list and return in
    //(it will eventually be returned to the free list, and the free list size will grow then)
    CollisionAnalyserData* newData;
    if(mDataList.GetSize()) {
        CollisionAnalyserData* oldStart = &mDataList[0];
        newData = &mDataList.NewElem();
        CollisionAnalyserData* newStart = &mDataList[0];
        if(oldStart != newStart) {
            //list was moved in memory when it was resized; adjust pointers
            //ideally, a game should never enter this block. if it does,
            //it probably means the initial list size was set too small.

            //fixup data index pointers
            int diff = (char *)newStart - (char *)oldStart;
            if(mIndexBoundLo <= mIndexBoundHi) {
                CollisionAnalyserDataIndex *p = mIndexBoundLo;
                do {
                    p->mData = (CollisionAnalyserData *)( ((char *)(p->mData)) + diff);
                } while(p++ != mIndexBoundHi);
            }
            //fixup free data list
            for(int i = 0; i < mFreeDataList.GetSize(); i++)
                mFreeDataList[i] = (CollisionAnalyserData *)((char *)mFreeDataList[i] + diff);
        }
    }
    else {
        newData = &mDataList.NewElem();
    }

    return newData;
}

static CollisionAnalyserDataIndex* searchpos = NULL;
CollisionAnalyserData* CollisionAnalyser::GetData(const CollisionAnalyserDataUID& UID)
{
    //upwards-searching with last remembered find using indexed structures && object pointers
    if(mIndexBoundLo > mIndexBoundHi)
        return NULL;
    if(searchpos > mIndexBoundHi || searchpos < mIndexBoundLo)
        searchpos = mIndexBoundLo;
    CollisionAnalyserDataIndex* endpos = searchpos;
    int size = mIndexBoundHi - mIndexBoundLo;
    do {
        //check current item in list
        if(UID.Equals(searchpos->mUID))
            return searchpos->mData;

        //go to next item in list
        if(searchpos == mIndexBoundHi)
            searchpos = mIndexBoundLo;
        else
            searchpos++;
    } while(searchpos != endpos);
    return NULL;
}

void CollisionAnalyser::CleanCollisionDataList(void)
{
    if(mIndexBoundLo > mIndexBoundHi)
        return;
    CollisionAnalyserDataIndex* p = mIndexBoundHi;
    //clean old items out of list
    do {
        if(p->mData->ReadyToTerminate()) {
            p->mData->Processed();
            mFreeDataList.Add(p->mData);  //add it to the free list
            mDataIndexList.RemoveAt(p - mIndexBoundLo);
        }
    } while(p-- != mIndexBoundLo);

    int size = mDataIndexList.GetSize();
    if(size == 0) {
        mIndexBoundHi = NULL;
        mIndexBoundLo = mIndexBoundHi + 1;
        return;
    }
    mIndexBoundLo = &mDataIndexList[0];
    mIndexBoundHi = &mDataIndexList[size-1];

    p = mIndexBoundLo;
    //notify old items (ones that haven't changed in a bit) that they should be removed
    do {
        if((p->mData->GetLastModifiedTime() + 0.1f) < mCachedTime)
            p->mData->NotifyShutdown();
    } while(p++ != mIndexBoundHi);
}

void CollisionAnalyser::CalculateStateCollisionDataList(void)
{
    if(mIndexBoundLo > mIndexBoundHi)
        return;
    CollisionAnalyserDataIndex *p = mIndexBoundLo;
    do {
        p->mData->CalculateState();
    } while(p++ != mIndexBoundHi);
}

void CollisionAnalyser::NotifyShutdown(void)
{
    if(mIndexBoundLo > mIndexBoundHi)
        return;
    CollisionAnalyserDataIndex *p = mIndexBoundLo;
    do {
        p->mData->NotifyShutdown();
    } while(p++ != mIndexBoundHi);
}

void CollisionAnalyser::Update(float gameTime)
{
    mCachedTime = gameTime;
    CleanCollisionDataList();
}

void CollisionAnalyser::RemoveSimState(SimState *simState)
{
    if(mIndexBoundLo > mIndexBoundHi)
        return;
    CollisionAnalyserDataIndex *p = mIndexBoundLo;
    do {
        if(p->mUID.mSimStateA == (uintptr_t)simState ||
            p->mUID.mSimStateB == (uintptr_t)simState)
            p->mData->NotifyShutdown();
    } while(p++ != mIndexBoundHi);
}

}