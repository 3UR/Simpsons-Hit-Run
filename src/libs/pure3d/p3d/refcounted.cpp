//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/refcounted.hpp>
#include <p3d/error.hpp>
#include <p3d/utility.hpp>

//#include <radmemory.hpp>

#include <string.h>


// Overloaded allocator/deallocatorry ref counted objects
void* tRefCountedTemp::operator new(size_t size)
{
  void *memPtr = p3d::MallocTemp(size);  // get the memory
#ifdef RADLOAD_HEAP_DEBUGGING
  radLoadHeapDebugAddAddress( memPtr );
#endif
  return memPtr;
}

void tRefCountedTemp::operator delete(void *ptr)
{
  p3d::FreeTemp(ptr);
}                                    

