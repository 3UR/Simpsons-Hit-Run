//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radobject.cpp
//
// Subsystem:   Fountation Technologies Base Class Implementation
//
// Description: Implemenation of radRefCount; an implemented base class
//
// Revisions:   V1.00 Mar 9, 2001       Creation
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include <radobject.hpp>
#include <typeinfo>
#include <string.h>

//=============================================================================
// Implementation
//=============================================================================

radMemoryAllocator radRefCount::s_allocator = -1;

radRefCount::radRefCount(unsigned int count)
:
m_allocator(s_allocator),
m_refCount(count)
{
}

radRefCount::~radRefCount()
{
    s_allocator = m_allocator;
}

void radRefCount::AddRef(void)
{
    m_refCount++;
}

void radRefCount::Release(void)
{
    rAssertMsg(m_refCount != 0, "Trying to release an object with no references.  Missing AddRef()\n");
    if(m_refCount < 2)
    {
        m_refCount = 0x0000FFFF;
        delete this;
        return;
    }
    m_refCount--;
}

template <class T> void radRefCount::Assign(T*& oldRef, T* newRef)
{
    if(newRef)
    {
        newRef->AddRef();
    }
    if(oldRef)
    {
        oldRef->Release();
    }
    oldRef = newRef;
}

template <class T> void radRefCount::Release(T*& obj)
{
    if(obj)
    {
        obj->Release();
        obj = NULL;
    }
}

void* radRefCount::operator new(size_t size, radMemoryAllocator alloc)
{
    void* rValue = radMemoryAlloc(alloc, size);
    s_allocator = alloc;
    return rValue;
}

void* radRefCount::operator new[](size_t size, radMemoryAllocator alloc)
{
    void* rValue = radMemoryAlloc(alloc, size);
    s_allocator = alloc;
    return rValue;
}

void radRefCount::operator delete(void* ptr)
{
    radMemoryFree(s_allocator, ptr);
}

void radRefCount::operator delete[](void* ptr)
{
    radMemoryFree(s_allocator, ptr);
}
