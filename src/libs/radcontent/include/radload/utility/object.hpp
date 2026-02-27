/** \file object.hpp
  *
  * Helper implementation of a reference counted object
  *
  * \author Mike Perzel 
  * \attention Copyright (C) 2002 Radical Entertainment Ltd.  All Rights Reserved.
  */

#ifndef RADLOADOBJECT_HPP
#define RADLOADOBJECT_HPP

#include <radload/radloadconfig.hpp>
#include <radobject.hpp>
#include <radmemory.hpp>

#ifdef RADLOAD_HEAP_DEBUGGING
void radLoadHeapDebugAddAddress( const void* obj );
void radLoadHeapDebugRemoveAddress( const void* obj );
#endif

/// Implementation of IRefCount.  This can be used as a base class for
/// other objects in order to give them implemented reference counting,
/// and be storable in an inventory.
class radLoadObject : public IRefCount
{
public:
    radLoadObject();

#ifdef RADLOAD_USE_RADMEMORYMONITOR
    virtual
#endif
    void AddRef();

#ifdef RADLOAD_USE_RADMEMORYMONITOR
    virtual
#endif
    void Release();

    /// Releases, and asserts if the reference count is not zero.  Useful
    /// if you want to make sure this is the last release
    void ReleaseVerified();

    /// Utility function to assign a new value to a pointer, and
    /// take care of the reference counting
    template <class T> static void Assign( T*& oldRef, T* newRef )
    {
        if( newRef )
        {
            newRef->AddRef();
        }
        if( oldRef )
        {
            oldRef->Release();
        }
        oldRef = newRef;
    }
    /// Releases the object, and sets the pointer to NULL.
    template <class T> static void Release( T*& obj )
    {
        if( obj )
        {
            obj->Release();
            obj = NULL;
        }
    }

    // new / delete overloading
    static void* operator new( size_t size );
    static void* operator new[]( size_t size );
    ///@{ Creates the object with the given allocator, and stores the allocator for later use.
    static void* operator new( size_t size, radMemoryAllocator alloc );
    static void* operator new[]( size_t size, radMemoryAllocator alloc ); //@}
    static void operator delete( void* ptr );
    static void operator delete[]( void* ptr );

protected:
    virtual ~radLoadObject();
    unsigned int m_refCount;
#ifdef RADLOAD_USE_RADMEMORYMONITOR
    bool m_bMemMonitorInitialized;
#endif

    static radMemoryAllocator s_allocator;

private:
    radLoadObject( const radLoadObject& );
    const radLoadObject& operator= ( const radLoadObject& );
};

#endif

