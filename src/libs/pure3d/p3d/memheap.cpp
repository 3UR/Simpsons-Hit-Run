//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/memheap.hpp>
#include <p3d/p3dtypes.hpp>
#include <p3d/error.hpp>

#include <string.h> 

tMemHeap::tMemHeap(unsigned initialSize/*=DEFAULT_INITIAL_HEAP_SIZE*/)
{
    SetGranularity(DEFAULT_HEAP_GRANULARITY);
    heap = NULL;
    SetupHeap(0);
}

tMemHeap::~tMemHeap()
{
    DeleteHeap();
}

void tMemHeap::SetupHeap(unsigned initialSize/*=DEFAULT_INITIAL_HEAP_SIZE*/)
{
    if(heap)
    {
        DeleteHeap();
    }
    if(initialSize > 0)
    {
        heap = CreateBlock(initialSize);
        P3DASSERT(heap);
    }
    currentBlock = heap;
    topOfHeap = 0;
    blockCount = 0;
    mallocCount = 0;
    blockSlop = 0;
    heapSize = initialSize;
    endSlop = initialSize;
}

MemHeapBlock *tMemHeap::CreateBlock(unsigned blockSize)
{
    // create MemHeapBlock structure, and data field in one step
    MemHeapBlock *block = (MemHeapBlock *)new char[sizeof(MemHeapBlock) + blockSize];
    P3DASSERT(block);

    block->size = blockSize;
    block->data = (char *)(block+1);
    #ifndef P3D_FILL_MEMHEAP
        // fill heap block with some data
        memset((unsigned char *)block->data, 0xAB, blockSize);
    #endif
    block->next = NULL;
    heapSize += blockSize;
    blockCount++;
    return block;
}

void tMemHeap::DeleteHeap()
{
    MemHeapBlock *block = heap;

    while(block)
    {
        MemHeapBlock *deleteMe = block;
        block = block->next;
        delete deleteMe;
    }

    heap = currentBlock = NULL;
    heapSize = 0;
    blockCount = 0;
    mallocCount = 0;
    blockSlop = 0;
    endSlop = 0;
}

void tMemHeap::ResetHeap()
{
    currentBlock = heap;
    topOfHeap = 0;
    mallocCount = 0;
    blockSlop = 0;
    endSlop = heap ? heap->size : 0;
}

void *tMemHeap::malloc(unsigned size)
{
    if(!heap)
    {
        heap = currentBlock = CreateBlock(granularity);
    }

    P3DASSERTMSG(size <= currentBlock->size, "Heap allocation failed, size > granularity.", "tMemHeap::malloc()");
    char *returnAddr;

    if(topOfHeap + size < currentBlock->size)
    {
        // return pointer to data
        returnAddr = currentBlock->data + topOfHeap;
        topOfHeap += (size + 3) & (~3L); // make sure the return address is long aligned!
    }
    else
    {
        // current block is full, grow heap
        // if the heap has been reset, a next block may already exist
        blockSlop += currentBlock->size - topOfHeap;
        if(!currentBlock->next)
        {
            currentBlock->next = CreateBlock(granularity);
        }
        currentBlock = currentBlock->next;
        returnAddr = currentBlock->data;
        topOfHeap = (size + 3) & (~3L); // long aligned!
    }

    mallocCount++;
    endSlop = currentBlock->size - topOfHeap;
    return returnAddr;
}

