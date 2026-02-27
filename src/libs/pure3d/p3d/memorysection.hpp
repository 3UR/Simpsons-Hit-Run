//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef MEMORYSECTION_HPP
#define MEMORYSECTION_HPP

#include <p3d/entity.hpp>
#include <p3d/loadmanager.hpp>

//-------------------------------------------------------------------
class tMemorySection : public tEntity
{
  public:
    tMemorySection(void);
   ~tMemorySection();

    unsigned MemoryUsed(void)    { return mMemoryUsed; }
    unsigned Allocations(void)   { return mAllocations; }

  protected:

    unsigned mMemoryUsed;
    unsigned mAllocations;

    unsigned mBeginAllocatorFree;
    unsigned mBeginAllocations;
    unsigned mBeginTaskedOutMemoryDelta;
    unsigned mBeginTaskedOutAllocationsDelta;

    friend class tMemorySectionLoader;
};


//-------------------------------------------------------------------
class tMemorySectionLoader : public tSimpleChunkHandler
{
public:
    tMemorySectionLoader();

protected:
    ~tMemorySectionLoader() {}
    tEntity* LoadObject(tChunkFile*, tEntityStore*);
};



#endif //MEMORYSECTION_HPP
