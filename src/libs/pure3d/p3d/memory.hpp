//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _PURE3D_MEMORY_HPP_
#define _PURE3D_MEMORY_HPP_

namespace p3d
{
    enum AllocType
    {
        ALLOC_DEFAULT = 0,
        ALLOC_PERMANENT,
        ALLOC_TEMPORARY,
        ALLOC_LOAD_TEMPORARY,
        ALLOC_LOADED,
        ALLOC_MAX_VALUE
    };

    void MemSetup(void);
    void MemShutdown(void);

    // Retreive current memory type
    AllocType GetCurrentAlloc(void);

    // Set current memory type, and return previous 
    AllocType SetCurrentAlloc(AllocType);

    // route alloc types to FTech allocators
    int GetMemAllocator(AllocType);
    void SetMemAllocator(AllocType, int);

    // overide default routing behavior
    class MemoryRouter
    {
        public:
           // Retreive current memory type
           virtual AllocType GetCurrent(void) = 0;

           // Set current memory type, and return previous 
           virtual void SetCurrent(AllocType) = 0;
    };

    void SetMemRouter(MemoryRouter*);
    MemoryRouter* GetMemRouter(void);

    // legacy temporary allocation functions
    void* MallocTemp(unsigned size);
    void  FreeTemp(void*);

    void  UsePermanentMem( bool flag );
}

#endif

