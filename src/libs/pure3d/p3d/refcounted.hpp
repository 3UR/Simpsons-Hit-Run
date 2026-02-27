//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _REFCOUNTED_HPP_
#define _REFCOUNTED_HPP_


#include <p3d/buildconfig.hpp>
#include <p3d/plat_types.hpp>
#include <stddef.h> 
#include <radload/utility/object.hpp>

class tNonCopyable
{
protected:
    tNonCopyable() {};
    ~tNonCopyable() {};

private: 
    // private, so derived classes (and pretty much anyone else) can't call them
    // plus there are no implimetations of these functios
    // so if someone does figure out how to call them,  they are rewarded with a linker error
    tNonCopyable( const tNonCopyable& );
    const tNonCopyable& operator=( const tNonCopyable& );
};

// a reference counted object
class tRefCounted : public radLoadObject
{

};

// a reference counted object allocated on the temporary heap
class tRefCountedTemp : public tRefCounted 
{
public:
    // overloaded allocators 
    static void *operator new(size_t size);
    static void operator delete(void*);

    static void* operator new( size_t size, int allocator );
    static void  operator delete( void* pMemory, int allocator );
};

// base class for smart pointers
class tPtrBase
{
protected:
    inline void Assign(tRefCounted* o) { if(o) o->AddRef(); if(obj) obj->Release(); obj = o; }
    inline void Release(void) { if(obj) obj->Release(); obj = 0; }
    tRefCounted* obj;
};

// templatized smart pointer
template <class T> class tPtr : public tPtrBase
{
public:
    tPtr()              { obj = 0;}
    tPtr(T* const t)          { obj = 0; Assign(t);}
    tPtr(const tPtr& p)       { obj = 0; Assign(p.obj); }
    ~tPtr()             { Release(); }

    tPtr& operator=(const tPtr& p) { Assign(p.obj); return *this;}
    tPtr& operator=(T* const t)    { Assign(t); return *this;}
    bool  operator==( const tPtr& p) const { return p == p.obj; }

    T* operator->() { return (T*)obj;}
    const T* operator->() const { return (T*)obj;}
    T& operator*()  { return *((T*)obj);}
    const T& operator*() const { return *((T*)obj); }
    operator T*()    { return (T*)obj;}
    operator const T*() const   { return (T*)obj;}

    operator bool() const  { return obj != 0; }
    bool operator !() const     { return obj == 0; }
//   operator void*() { return (void*)obj;}

//   template <class T2> operator tPtr<T2>() { return tPtr<T2>((T*)obj);}
private:
//   static void *operator new(size_t size);
//   static void operator delete(void*);
};

#endif
