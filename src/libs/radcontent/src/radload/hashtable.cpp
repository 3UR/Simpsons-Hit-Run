//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All Rights Reserved
//
// Component   : radLoad
//
// Description : Hash Table
//
// Author(s)   : Mike Perzel (04/15/02)
//
//=============================================================================

// Based on the tTable class from Pure3D

#include <radload/utility/hashtable.hpp>
#include <radmath/radmath.hpp>
#include <raddebug.hpp>
#include <typeinfo>


PtrHashTable::PtrHashTable( unsigned int tableSize, unsigned int maxDensity, unsigned int maxDeletedItems )
:
m_tableSize( tableSize ),
m_numElements( 0 ),
m_numDeletedItems( 0 ),
m_autoRepack( true ),
m_maxDeletedItems( maxDeletedItems ),
m_maxDensity( maxDensity ),
m_elements( NULL ),
m_keys( NULL ),
m_deleteMarker( 0 )
{
    SetInternalMemoryAllocator( s_allocator );
    ReSize( m_tableSize );
}

PtrHashTable::~PtrHashTable()
{
    s_allocator = GetMemoryAllocator();
    radMemoryFree( m_elements );
    radMemoryFree( m_keys );
}

bool PtrHashTable::Remove( void* obj )
{
    bool found = false;
    unsigned int i = 0;
    for(; i < m_tableSize; i++ )
    {
        if( m_elements[i] == obj )
        {
            found = true;
            m_elements[i] = &m_deleteMarker;
            m_numDeletedItems++;
        }
    }
    if ((m_autoRepack) && (m_numDeletedItems>m_maxDeletedItems))
    {
        RePack();
    }
    return found;
}

void PtrHashTable::RemoveAll()
{
    unsigned int i = 0;
    for(; i < m_tableSize; i++ )
    {
        m_elements[i] = NULL;
    }
    m_numElements = 0;
}

void PtrHashTable::RePack()
{
    ReSize( m_tableSize ); 
}

void PtrHashTable::ReSize( unsigned int tableSize )
{
    rAssert( (tableSize & (tableSize - 1)) == 0 );

    void** oldElements = m_elements;
    radKey* oldKeys = m_keys;
    unsigned int oldTableSize = m_tableSize;
    m_tableSize = tableSize;

    const radMemoryAllocator allocator = GetMemoryAllocator();
    m_elements = static_cast<void**>(radMemoryAlloc( allocator, m_tableSize*sizeof(void*) ));
    m_keys = static_cast<radKey*>(radMemoryAlloc( allocator, m_tableSize*sizeof(radKey) ));
    m_numDeletedItems = 0;
    m_numElements = 0;
    unsigned int i = 0;
    for(; i < m_tableSize; i++)
    {
        m_elements[i] = NULL;
        m_keys[i] = 0;
    }
    if( oldElements )
    {
        for( i = 0; i < oldTableSize; i++ )
        {
            if( oldElements[i] && (oldElements[i] != &m_deleteMarker) )
            {
                Store( oldKeys[i], oldElements[i] );
            }
        }
        radMemoryFree( oldElements );
        radMemoryFree( oldKeys );
        
    }

}

void PtrHashTable::Store( radKey key, void* obj )
{
    Iterator it( this );
    int index = it.FirstFree( key );
    m_elements[index] = obj;
    m_keys[index] = key;
    m_numElements += (m_numElements < m_tableSize) ? 1 : 0;

    if (((m_numElements * 100) / m_tableSize) > m_maxDensity )
    {
        ReSize( rmt::NextPow2( m_tableSize + 1 ) );
    }
}

//=============================================================================
// PtrHashTable::StoreUnlessAlreadyPresent
//
// stores an item in the inventory unless there is something already there.  If 
// there is something there then the function returns false
//
//=============================================================================
bool PtrHashTable::StoreUnlessAlreadyPresent( radKey key, void* obj )
{
    Iterator it( this );

    //
    // Need to walk the array until we find an em
    //
    IRefCount* refCountObj = reinterpret_cast< IRefCount* >( obj );

    unsigned int index;
    unsigned int originalIndex = it.First( key );
    for( index = originalIndex; index < m_tableSize; ( ++index ) %= m_tableSize )
    {
        if( m_keys[ index ] == key )
        {
            IRefCount* elementObj = reinterpret_cast< IRefCount* >( m_elements[ index ] );
            if( typeid( *refCountObj ) == typeid( *elementObj ) )
            {
                return false;
            }
        }
        
        if( m_elements[ index ] == NULL )
        {
            break;
        }
    }
    rAssert( index != m_tableSize );

    m_elements[ index ] = obj;
    m_keys[ index ] = key;
    m_numElements += ( m_numElements < m_tableSize ) ? 1 : 0;

    if (((m_numElements * 100) / m_tableSize) > m_maxDensity )
    {
        ReSize( rmt::NextPow2( m_tableSize + 1 ) );
    }
    return true;
}

PtrHashTable::Iterator::Iterator( PtrHashTable* table )
:
m_hashTable( table ),
m_key( 0 ),
m_hash( 0 ),
m_increment( 0 )
{
    m_hashTable->AddRef();
}

PtrHashTable::Iterator::~Iterator()
{
    m_hashTable->Release();
}

void* PtrHashTable::Iterator::PtrFirst( radKey key )
{
    unsigned int nSearch = 0;
    m_key = key;
    m_hash = Hash( key );
    m_increment = Increment( key );
    while( ((m_hashTable->m_elements[m_hash] != NULL) && (m_hashTable->m_keys[m_hash] != m_key)) || (m_hashTable->m_elements[m_hash] == &m_hashTable->m_deleteMarker) )
    {
        nSearch++;
        if( nSearch > m_hashTable->m_tableSize - 1 )
        {
            return NULL;
        }
        m_hash += m_increment;
        m_hash &= m_hashTable->m_tableSize - 1;
    }
    return m_hashTable->m_elements[m_hash];
}

void* PtrHashTable::Iterator::PtrNext()
{
    unsigned int nSearch = 0;
    m_hash += m_increment;
    m_hash &= m_hashTable->m_tableSize - 1;
    while( ((m_hashTable->m_elements[m_hash] != NULL) && (m_hashTable->m_keys[m_hash] != m_key)) || (m_hashTable->m_elements[m_hash] == &m_hashTable->m_deleteMarker) )
    {
        nSearch++;
        if( nSearch > m_hashTable->m_tableSize - 1 )
        {
            return NULL;
        }
        m_hash += m_increment;
        m_hash &= m_hashTable->m_tableSize - 1;
    }
    return m_hashTable->m_elements[m_hash];
}

unsigned int PtrHashTable::Iterator::First( radKey key )
{
    PtrFirst( key );
    return m_hash;
}

unsigned int PtrHashTable::Iterator::FirstFree( radKey key )
{
    void* obj = PtrFirst( key );
    while( obj )
    {
        obj = PtrNext();
    }
    return m_hash; 
}

PtrHashTable::RawIterator::RawIterator( PtrHashTable* table )
:
m_hashTable( table ),
m_index( 0 )
{
// Don't like doing this, but it does interfere with the heap
// debugging stuff, because it AddRefs after the address has
// been released.
#ifndef RADLOAD_HEAP_DEBUGGING
    m_hashTable->AddRef();
#endif
}

PtrHashTable::RawIterator::~RawIterator()
{
// Don't like doing this, but it does interfere with the heap
// debugging stuff, because it AddRefs after the address has
// been released.
#ifndef RADLOAD_HEAP_DEBUGGING
    m_hashTable->Release();
#endif
}

void* PtrHashTable::RawIterator::PtrFirst()
{
    m_index = -1;
    return PtrNext();
}

void* PtrHashTable::RawIterator::PtrLast()
{   
    m_index = m_hashTable->m_tableSize;
    return PtrPrev();
}

void* PtrHashTable::RawIterator::PtrNext()
{
    do
    {
        m_index++;
    } 
    while( (static_cast<unsigned int>(m_index) < m_hashTable->m_tableSize) && ((!m_hashTable->m_elements[m_index]) || (m_hashTable->m_elements[m_index] == &m_hashTable->m_deleteMarker)) );

    if( static_cast<unsigned int>( m_index ) >= m_hashTable->m_tableSize )
    {
        return NULL;
    }
    return m_hashTable->m_elements[m_index];
}

void* PtrHashTable::RawIterator::PtrPrev()
{
    do
    {
        m_index--;
    }
    while( (m_index >= 0) && (!m_hashTable->m_elements[m_index]) && (m_hashTable->m_elements[m_index] != &m_hashTable->m_deleteMarker) );

    if( m_index < 0 )
    {
        return NULL;
    }
    return m_hashTable->m_elements[m_index];
}

radKey PtrHashTable::RawIterator::GetKey()
{
    if( (m_index >= 0) && (static_cast<unsigned int>(m_index) < m_hashTable->m_tableSize) )
    {
        return m_hashTable->m_keys[m_index];
    }
    return 0;
}

radMemoryAllocator PtrHashTable::GetMemoryAllocator() const
{
    return m_allocator;
}

void PtrHashTable::SetInternalMemoryAllocator( const radMemoryAllocator a )
{
    m_allocator = a;
}