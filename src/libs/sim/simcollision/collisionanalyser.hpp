#ifndef _COLLISIONANALYSER_HPP_
#define _COLLISIONANALYSER_HPP_

#include "simcollision/collision.hpp"

namespace sim
{

enum CollisionAnalyzer_Answer { CollisionAnalyzer_Continue = 0, CollisionAnalyzer_Aborted };

typedef TArray<CollisionAnalyserData> CollisionAnalyserDataList;
typedef TArray<CollisionAnalyserDataIndex> CollisionAnalyserDataIndexList;
typedef TArray<CollisionAnalyserData *> CollisionAnalyserDataPList;

class CollisionAnalyser
{
public:
    CollisionAnalyser();
    virtual ~CollisionAnalyser();
    virtual void StartSolvingCollisionList(TArray<Collision>* inList, float current_time = 0);
    virtual void EndSolvingCollisionList(TArray<Collision>* inList, float current_time = 0);
    
    // this method is called at the beginning before anything is done with the collision
    // returning CollisionAnalizer_Aborted would cause the collision to be ignored. It is sometime
    // necessary for pile of objects, to solve a collisions on more than one pass. inPass 
    // indicate the current pass.
    virtual CollisionAnalyzer_Answer PreCollisionEvent(Collision& inCollision, int inPass);
    
    // this method allows to trigger sounds, animation and modify the objects state.
    virtual CollisionAnalyzer_Answer CollisionEvent( SimState* inSimStateA, int indexA, 
        SimState* inSimStateB, int indexB, 
        const rmt::Vector& inPos, float inDvN, float inDvT);
    
    CollisionAnalyserDataPList& GetCollisionData();
    
    void NotifyShutdown(void);
    void Update(float gameTime);
    void RemoveSimState(SimState *simState);
    void CleanCollisionDataList(void);
private:
    CollisionAnalyserDataIndex* CreateNewDataIndex();
    CollisionAnalyserData* CreateNewData();
    CollisionAnalyserData* GetData(const CollisionAnalyserDataUID& UID);
    void CalculateStateCollisionDataList(void);
    
    CollisionAnalyserDataIndex* mIndexBoundLo;
    CollisionAnalyserDataIndex* mIndexBoundHi;

    float mCachedTime;
    bool mCollectingData;
    CollisionAnalyserDataIndexList mDataIndexList;
    CollisionAnalyserDataPList mFreeDataList;
    CollisionAnalyserDataList mDataList;

    CollisionAnalyserDataPList mCollisionDataArray;

    // this method is called at the beginning before anything is done with the collision
    // returning CollisionAnalizer_Aborted would cause the collision to be ignored. It is sometime
    // necessary for pile of objects, to solve a collisions on more than one pass. inPass 
    // indicate the current pass.
    rmt::Vector mCachedCollisionNormal;
    CollisionAnalyserPossibleEvent mCachedPossibleEventsA;
    CollisionAnalyserPossibleEvent mCachedPossibleEventsB;
    CollisionAnalyserPossibleEvent mCachedPossibleEvents;
};

}

#endif