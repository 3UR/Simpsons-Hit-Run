//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef RADLOADQUEUE_HPP
#define RADLOADQUEUE_HPP

#include <radload/utility/object.hpp>

class PtrQueue : public radLoadObject
{
public:
    PtrQueue( unsigned int size = 32 );
    virtual ~PtrQueue();

    bool PtrPush( void* obj );
    void* PtrPop();
    // Take a look at the top element of the queue
    void* PtrTop();
    bool Empty() { return m_numElements == 0; }
    unsigned int Size() { return m_numElements; }
protected:
    virtual radMemoryAllocator GetMemoryAllocator() const;
    virtual void SetInternalMemoryAllocator( const radMemoryAllocator a );

    void** m_pList;
    unsigned int m_current;
    unsigned int m_tail;
    unsigned int m_size;
    unsigned int m_numElements;

private:
    radMemoryAllocator m_allocator;
};

template <class T> class Queue : public PtrQueue
{
public:
    Queue( unsigned int size = 32 ) : PtrQueue( size ) {}
    bool Push( T* obj ) { return PtrPush( (void*)obj ); }
    T* Pop() { return (T*)PtrPop(); }
    T* Top() { return (T*)PtrTop(); }
};

template <class T> class RefQueue : public Queue<T>
{
public:
    RefQueue( unsigned int size = 32 ) : Queue<T>( size ) {}
    virtual ~RefQueue() { while( Pop() ); }

    bool Push( T* obj )
    {
        obj->AddRef();
        return Queue<T>::Push( obj );
    }

    T* Pop()
    {
        T* obj = Queue<T>::Pop();
        if( obj )
        {
            obj->Release();
        }
        return obj;
    }
};

#endif

