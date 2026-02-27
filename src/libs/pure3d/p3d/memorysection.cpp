//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================



#include <p3d/memorysection.hpp>
#include <p3d/chunkfile.hpp>
#include <p3d/utility.hpp>
#include <constants/chunkids.hpp>
#include <radmemory.hpp>



//----------------------------------------------------------------------------------------------
tMemorySection::tMemorySection(void) : 
    mMemoryUsed(0),
    mAllocations(0),
    mBeginAllocatorFree(0),
    mBeginAllocations(0),
    mBeginTaskedOutMemoryDelta(0),
    mBeginTaskedOutAllocationsDelta(0)
{
}


//----------------------------------------------------------------------------------------------
tMemorySection::~tMemorySection()
{                    
    mMemoryUsed = 0xFFFFFFFFUL; 
}



//----------------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------------
tMemorySectionLoader::tMemorySectionLoader() : tSimpleChunkHandler(MemorySection::MEMORYSECTION)
{

}

//----------------------------------------------------------------------------------------------
tEntity* tMemorySectionLoader::LoadObject(tChunkFile *f, tEntityStore *store)
{
#ifdef P3D_TRACK_LOAD_STATS
    char name[256];
    f->GetPString(name);

    bool created = false;
    tMemorySection *section = p3d::find<tMemorySection>(store, name);
    if (section == NULL)
    {
        section = new tMemorySection;
        section->SetName(name);
        created = true;
    }

    bool begin = (f->GetLong() != 0);
    bool end   = (f->GetLong() != 0);

    P3DASSERT(begin != end);

    IRadMemoryAllocator *allocator = radMemoryGetAllocator(radMemoryGetCurrentAllocator());

    unsigned unused;
    unsigned totalFree, allocations;
    allocator->GetStatus(&totalFree, &unused, &allocations, &unused);       

    if (begin)
    {
        P3DASSERT(section->mBeginAllocatorFree == 0);
        section->mBeginAllocatorFree = totalFree;
        section->mBeginAllocations   = allocations;
        section->mBeginTaskedOutMemoryDelta      = tLoadManager::TaskedOutMemoryDelta();
        section->mBeginTaskedOutAllocationsDelta = tLoadManager::TaskedOutAllocationsDelta();
    }
    else // end
    {
        P3DASSERT(section->mBeginAllocatorFree != 0);
        section->mMemoryUsed  += (totalFree - section->mBeginAllocatorFree);
        section->mAllocations += (allocations - section->mBeginAllocatorFree);

        section->mMemoryUsed  -= (tLoadManager::TaskedOutMemoryDelta()      - section->mBeginTaskedOutMemoryDelta);
        section->mAllocations -= (tLoadManager::TaskedOutAllocationsDelta() - section->mBeginTaskedOutAllocationsDelta);
        
        section->mBeginAllocatorFree = 0;
        section->mBeginAllocations   = 0;
        section->mBeginTaskedOutMemoryDelta      = 0;
        section->mBeginTaskedOutAllocationsDelta = 0;
    }

    if (created) return section;
#endif
    return NULL;
}

















