//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <radload/utility/queue.hpp>

PtrQueue::PtrQueue( unsigned int size )
:
m_pList( NULL ),
m_current( 0 ),
m_tail( 0 ),
m_size( size ),
m_numElements( 0 )
{
    SetInternalMemoryAllocator( s_allocator );
    const radMemoryAllocator allocator = GetMemoryAllocator();
    m_pList = static_cast<void**>(radMemoryAlloc( allocator, m_size*sizeof(void*) ));
}

PtrQueue::~PtrQueue()
{
    s_allocator = GetMemoryAllocator();
    radMemoryFree( m_pList );
}

bool PtrQueue::PtrPush( void* obj )
{
    if( m_numElements == m_size )
    {
        return false;
    }
    m_pList[m_tail++] = obj;
    if( m_tail >= m_size )
    {
        m_tail = 0;
    }
    m_numElements++;
    return true;
}

void* PtrQueue::PtrPop()
{
    if( m_numElements == 0 )
    {
        return NULL;
    }
    void* obj = m_pList[m_current];
    m_pList[m_current++] = NULL;
    if( m_current >= m_size )
    {
        m_current = 0;
    }
    m_numElements--;
    return obj;
}

void* PtrQueue::PtrTop()
{
   return m_pList[m_current];
}

radMemoryAllocator PtrQueue::GetMemoryAllocator() const
{
    return m_allocator;
}

void PtrQueue::SetInternalMemoryAllocator( const radMemoryAllocator a )
{
    m_allocator = a;
}