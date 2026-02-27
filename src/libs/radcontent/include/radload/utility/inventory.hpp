/**
  * \file inventory.hpp
  *
  * \author Mike Perzel
  * \attention Copyright 2002 Radical Entertainment Ltd.  All Rights Reserved.
  */

#ifndef RADLOAD_INVENTORY_HPP
#define RADLOAD_INVENTORY_HPP

//#include <radload/radload.hpp>
#include <radload/utility/hashtable.hpp>

/// Adds the ability to store elements in an inventory to the basic interface
class radLoadStore : public radLoadInventory
{
public:
    virtual void StoreHandlingCollisions( radKey id, IRefCount* obj ) = 0;
    virtual void Store( radKey id, IRefCount* obj ) = 0;
};

/// An implemented radLoadStore which keeps it's elements in a hash table
class radLoadHashedStore : public radLoadStore
{
public:
    /// Grabs raw elements one by one starting at the beginning.
    class RawIterator : public RefHashTable<IRefCount>::RawIterator
    {
    public:
        RawIterator( radLoadHashedStore* store )
            : RefHashTable<IRefCount>::RawIterator( store->m_hashTable ),
              m_pStore( store )
              {
              }
    protected:
        radLoadHashedStore* m_pStore;
    };

    /// Grabs elements associated with a particular key
    class Iterator : public RefHashTable<IRefCount>::Iterator
    {
    public:
        Iterator( radLoadHashedStore* store )
            : RefHashTable<IRefCount>::Iterator( store->m_hashTable ),
              m_pStore( store )
              {
              }
    protected:
        radLoadHashedStore* m_pStore;
    };

    /// \param startSize Must be a power of two
    radLoadHashedStore( int startSize = 1024 );

    virtual IRefCount* Find( SafeCastBase& caster, const radKey id );
    virtual void Store( radKey id, IRefCount* obj );
    virtual void StoreHandlingCollisions( radKey id, IRefCount* obj );
    /// Dumps all of the items from the specified store into the current one.
    virtual void Dump( radLoadStore* store );
    virtual void Remove( IRefCount* obj );
    virtual void RemoveAll();
    virtual int GetElementCount();

    virtual void RePack();
    virtual void SetAutoRepack(bool enable);
    virtual bool GetAutoRepack();

    bool TestCollision(radKey id, IRefCount* obj);
    void SetAutoCollisionTest(bool enable);
    bool GetAutoCollisionTest();

protected:
    friend class RawIterator;
    friend class Iterator;

    virtual ~radLoadHashedStore();
    bool autoRepack;
    bool autoCollisionTest;
    RefHashTable<IRefCount>* m_hashTable;
};

#endif

