//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


/*
    Platform-independent tMemHeap class
    This class is useful for situations where many small memory allocations
    are needed.  The RADlib malloc function incurs an 8-12 byte overhead per
    call, where the tMemHeap malloc function incurs almost none.
    The heap has the following restrictions:
        - Allocations cannot be larger than the current block size (which is
          usually the same as the heap granularity.
        - Individual elements cannot be deleted from the heap.  The only way
          to recover the memory is to delete the entire heap
        - Heap blocks may be only partially filled, resulting in some (ususally
          small) inefficiency.
    The heap can grow to be as large as available memory.
    */

#ifndef _MEMHEAP_HPP
#define _MEMHEAP_HPP

#include <p3d/p3dtypes.hpp>

#define DEFAULT_INITIAL_HEAP_SIZE 4096
#define DEFAULT_HEAP_GRANULARITY 4096

struct MemHeapBlock
{
    unsigned         size;
    MemHeapBlock *next;
    char         *data;
};


class tMemHeap
{
public:
    tMemHeap(unsigned initialSize = DEFAULT_INITIAL_HEAP_SIZE);
    virtual ~tMemHeap();

    void *malloc(unsigned size);
    void DeleteHeap();
    void ResetHeap();
    void SetupHeap(unsigned initialSize = DEFAULT_INITIAL_HEAP_SIZE);

    inline void SetGranularity(unsigned gran) { granularity = gran; }
    inline unsigned GetGranularity() { return granularity; }
    inline unsigned GetHeapSize() { return heapSize; }

private:   
    unsigned granularity;
    unsigned topOfHeap;
    unsigned heapSize;
    unsigned blockCount;
    unsigned mallocCount;
    unsigned blockSlop;
    unsigned endSlop;
    MemHeapBlock *heap;
    MemHeapBlock *currentBlock;

    MemHeapBlock *CreateBlock(unsigned blockSize);
};

#endif // __MEMHEAP_HPP

