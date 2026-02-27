#ifndef _COLLISIONANALYSERDATAUID_HPP_
#define _COLLISIONANALYSERDATAUID_HPP_

#include <vadefs.h> // -_-

namespace sim
{

    class SimState;

class CollisionAnalyserDataUID {
public:
    CollisionAnalyserDataUID(const CollisionAnalyserDataUID& UID) {
        mSimStateA = UID.mSimStateA;
        mSimStateB = UID.mSimStateB;
        mIndexA = UID.mIndexA;
        mIndexB = UID.mIndexB;
    };
    CollisionAnalyserDataUID(
        SimState *newSimStateA, int newIndexA,
        SimState *newSimStateB, int newIndexB)
    {
        //make sure the pairs are always stored in lowest-first order
        //this is to make sure pairs that are otherwise the same (just
        //passed in a different order) evaluate to the same physical pair.
        if(newSimStateA < newSimStateB) {
            mSimStateA = (uintptr_t)newSimStateA;
            mSimStateB = (uintptr_t)newSimStateB;
            mIndexA = (unsigned)newIndexA;
            mIndexB = (unsigned)newIndexB;
        }
        else {
            mSimStateA = (uintptr_t)newSimStateB;
            mSimStateB = (uintptr_t)newSimStateA;
            mIndexA = (unsigned)newIndexB;
            mIndexB = (unsigned)newIndexA;
        }
    }

    int diff(const CollisionAnalyserDataUID& UID) const
    {
        if(mSimStateA != UID.mSimStateA) {
            if(mSimStateA < UID.mSimStateA)
                return -1;
            return 1;
        }
        if(mSimStateB != UID.mSimStateB) {
            if(mSimStateB < UID.mSimStateB)
                return -1;
            return 1;
        }
        if(mIndexA != UID.mIndexA) {
            if(mIndexA < UID.mIndexA)
                return -1;
            return 1;
        }
        if(mIndexB != UID.mIndexB) {
            if(mIndexB < UID.mIndexB)
                return -1;
            return 1;
        }
        return 0;
    }

    bool operator<(const CollisionAnalyserDataUID& UID) const
    {
        if(mSimStateA < UID.mSimStateA)
            return true;
        if(mSimStateB < UID.mSimStateB)
            return true;
        if(mIndexA < UID.mIndexA)
            return true;
        if(mIndexB < UID.mIndexB)
            return true;
        return false;
    }

    bool Equals(const CollisionAnalyserDataUID& UID) const
    {
        if(mSimStateA != UID.mSimStateA)
            return false;
        if(mSimStateB != UID.mSimStateB)
            return false;
        if(mIndexA != UID.mIndexA)
            return false;
        if(mIndexB != UID.mIndexB)
            return false;
        return true;
    }

    uintptr_t mSimStateA, mIndexA;
    unsigned mSimStateB, mIndexB;
};

}

#endif