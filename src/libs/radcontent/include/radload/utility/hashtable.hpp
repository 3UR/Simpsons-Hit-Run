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

#ifndef HASHTABLE_HPP
#define HASHTABLE_HPP

#include <radkey.hpp>
#include <radload/utility/object.hpp>

class PtrHashTable : public radLoadObject
{
public:
    PtrHashTable( 
            unsigned int size = 1024, // Must be a power of two
            unsigned int maxDensity = 80, // Percentage full the table should be before resizing (put to something big (1000) to never resize)
            unsigned int maxDeletedItems = 100 ); // num of allowable delete markers in the table

    // standard iterator - find all objects with a given hash key
    class Iterator
    {
    public:
        Iterator( PtrHashTable* table );
        ~Iterator();

        void* PtrFirst( radKey key );
        void* PtrNext();
        unsigned int First( radKey key );
        unsigned int FirstFree( radKey key );

    private:
        PtrHashTable* m_hashTable;
        radKey m_key;
        unsigned int m_hash;
        unsigned int m_increment; 
        inline unsigned int Hash( const radKey key ) { return (unsigned int)( key & (m_hashTable->m_tableSize - 1) ); }
        inline unsigned int Increment( radKey key ) { return (unsigned int)(( key >> 9 ) & static_cast< radKey >( 0xFF ) ) | 1; }
    };

    // raw iterator - finds all objects in table - returns NULL when done
    class RawIterator
    {
    public:
        RawIterator( PtrHashTable* table );
        ~RawIterator();
        void* PtrFirst();
        void* PtrLast();
        void* PtrNext();
        void* PtrPrev();

        radKey GetKey();

    private:
        PtrHashTable* m_hashTable;
        int m_index;
    };

    friend class Iterator;
    friend class RawIterator;

    int GetElementCount() { return m_numElements; }

    void Store( radKey key, void* obj );
    bool StoreUnlessAlreadyPresent( radKey key, void* obj );
    bool Remove( void* obj );
    void RemoveAll();

    void RePack();

    void SetAutoRepack(bool enable) 
    { 
        m_autoRepack = enable;     
        if ((m_autoRepack) && (m_numDeletedItems>m_maxDeletedItems))
        {
            RePack();
        }
    }

    bool GetAutoRepack() 
    { 
        return m_autoRepack; 
    }

protected:
    virtual radMemoryAllocator GetMemoryAllocator() const;
    virtual void SetInternalMemoryAllocator( const radMemoryAllocator a );
    virtual ~PtrHashTable();

private:
    void ReSize( unsigned int tableSize );

    // Bookkeeping member variables
    unsigned int m_tableSize;
    unsigned int m_numElements;
    unsigned int m_numDeletedItems;
    
    // Resize / Repack values
    bool m_autoRepack;
    unsigned int m_maxDeletedItems;
    unsigned int m_maxDensity;

    // Table elements
    void** m_elements;
    radKey* m_keys;
    int m_deleteMarker;

    radMemoryAllocator m_allocator;
};


// Templatized hash table
template<class T> class HashTable : public PtrHashTable
{
public:
    HashTable( 
            unsigned int size = 1024, // Must be a power of two
            unsigned int maxDensity = 80, // Percentage full the table should be before resizing (put to something big (1000) to never resize)
            unsigned int maxDeletedItems = 100 ) // num of allowable delete markers in the table
            : PtrHashTable( size, maxDensity, maxDeletedItems ) {}

    class Iterator : public PtrHashTable::Iterator
    {
    public:
        Iterator( PtrHashTable* table ) : PtrHashTable::Iterator( table ) { /**/ }
        T* First( radKey key ) { return (T*)PtrFirst( key ); }
        T* Next() { return (T*)PtrNext(); }
    };

    class RawIterator : public PtrHashTable::RawIterator
    {
    public:
        RawIterator( PtrHashTable* table ) : PtrHashTable::RawIterator( table ) { /**/ }
        T* First() { return (T*)PtrFirst(); }
        T* Last() { return (T*)PtrLast(); }
        T* Next() { return (T*)PtrNext(); }
        T* Prev() { return (T*)PtrPrev(); }
    };

    T* Find( radKey key ) { Iterator it( this ); return it.First( key ); }
    void Store( radKey key, T* obj ) { PtrHashTable::Store( key, static_cast<void*>(obj) ); }
    bool StoreUnlessAlreadyPresent( radKey key, T* obj ) { return PtrHashTable::StoreUnlessAlreadyPresent( key, static_cast<void*>(obj) ); }
    bool Remove( T* obj ) { return PtrHashTable::Remove( (void*)obj ); }

protected:
    
};

template <class T> class RefHashTable : public HashTable<T>
{
public:
    RefHashTable( 
            unsigned int size = 1024, // Must be a power of two
            unsigned int maxDensity = 80, // Percentage full the table should be before resizing (put to something big (1000) to never resize)
            unsigned int maxDeletedItems = 100 ) // num of allowable delete markers in the table
            : HashTable<T>( size, maxDensity, maxDeletedItems ) {}

    virtual ~RefHashTable()
    {
        RemoveAll();
    }

    void Store( radKey key, T* obj )
    {
        obj->AddRef();
        HashTable<T>::Store( key, obj );
    }
    bool StoreUnlessAlreadyPresent( radKey key, T* obj )
    {
        bool success = HashTable<T>::StoreUnlessAlreadyPresent( key, obj );
        if( success )
        {
            obj->AddRef();
        }
        return success;
    }
    bool Remove( T* obj )
    {
        if( HashTable<T>::Remove( obj ) )
        {
            obj->Release();
            return true;
        }
        return false;
    }
    void RemoveAll()
    {
        typename HashTable<T>::RawIterator it( this );
        T* obj = it.First();
        while( obj )
        {
            obj->Release();
            obj = it.Next();
        }
        PtrHashTable::RemoveAll();
    }
};

#endif 

