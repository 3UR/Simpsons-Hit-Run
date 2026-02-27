#include "memory.h"

namespace p3d
{
void* MallocTemp( unsigned int size )
{
    return new unsigned char[size];
}

void FreeTemp( void* mem )
{
    delete [] (unsigned char*)mem;
}

AllocType SetCurrentAlloc( AllocType a )
{
    return (AllocType)0;
}

AllocType GetCurrentAlloc()
{
    return (AllocType)0;
}

}
