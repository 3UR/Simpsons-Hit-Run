//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <radload/radload.hpp>
#include <radload/utility/inventory.hpp>
#include <radmath/radmath.hpp>
#include <typeinfo>

//-------------------------------------------------------------------
class SafeCasterObj : public radLoadInventory::SafeCastBase
{
public:
    SafeCasterObj(IRefCount* e) { obj = e;}
    IRefCount* safe_cast(IRefCount* c) { return typeid(*c) == typeid(*obj) ? c : NULL; }

private:
    IRefCount* obj;
};

radLoadHashedStore::radLoadHashedStore( int startSize )
{
    autoRepack = true;
    autoCollisionTest = true;
    m_hashTable = new RefHashTable<IRefCount>( rmt::NextPow2( startSize ) ); 
    m_hashTable->AddRef();
    m_hashTable->SetAutoRepack(autoRepack);
}

radLoadHashedStore::~radLoadHashedStore()
{
    m_hashTable->Release();
}

void radLoadHashedStore::Dump( radLoadStore* store )
{
    RefHashTable<IRefCount>::RawIterator it( m_hashTable );
    IRefCount* obj = it.First();
    while( obj )
    {
        store->Store( it.GetKey(), obj );
        obj = it.Next();
    }
    RemoveAll();
}

IRefCount* radLoadHashedStore::Find( SafeCastBase& caster, radKey id )
{
    HashTable<IRefCount>::Iterator it( m_hashTable );
    IRefCount* obj = it.First( id );
    while( !caster.safe_cast( obj ) && obj )
    {
        obj = it.Next();
    }
    return obj; 
}

int radLoadHashedStore::GetElementCount()
{
    return m_hashTable->GetElementCount();
}

//=============================================================================
// radLoadHashedStore::Store
//=============================================================================
void radLoadHashedStore::Store( radKey id, IRefCount* obj )
{
#ifdef RADLOAD_COLLISION_TESTS
    if ((autoCollisionTest)&&(TestCollision(id,obj)))
    {
        rReleasePrintf("WARNING: radLoadHashStore collision.  Type : \"%s\"  RadKey \"%d\"\n", typeid(obj).name(), id); 
    }
#endif
    bool collision = TestCollision( id, obj );
    //rAssert( !collision );
    if( collision )
    {
        obj->AddRef();
        obj->Release();
        return;
    }
    m_hashTable->Store( id, obj );
}
//=============================================================================
// radLoadHashedStore::StoreHandlingCollisions
//
// stores an item in the inventory - if something with the same name is already
// there, then it releases the object
//
//=============================================================================
void radLoadHashedStore::StoreHandlingCollisions( radKey id, IRefCount* obj )
{
    bool collision = TestCollision( id, obj );
    if( collision )
    {
        obj->AddRef();
        obj->Release();
        return;
    }
    m_hashTable->Store( id, obj );
}

void radLoadHashedStore::Remove( IRefCount* obj )
{
    m_hashTable->Remove( obj );
}

void radLoadHashedStore::RemoveAll()
{
    m_hashTable->RemoveAll();
}

void radLoadHashedStore::RePack()
{
    m_hashTable->RePack();
}

void radLoadHashedStore::SetAutoRepack(bool enable)
{
    autoRepack = enable;
    m_hashTable->SetAutoRepack(autoRepack);
}

bool radLoadHashedStore::GetAutoRepack()
{
    return autoRepack;
}

void radLoadHashedStore::SetAutoCollisionTest(bool enable)
{
    autoCollisionTest = enable;
}

bool radLoadHashedStore::GetAutoCollisionTest()
{
    return autoCollisionTest;
}

bool radLoadHashedStore::TestCollision(radKey id, IRefCount* orig)
{
    SafeCasterObj caster(orig);

    HashTable<IRefCount>::Iterator it( m_hashTable );
    IRefCount* obj = it.First( id );
    while(obj)
    {
        if (caster.safe_cast(obj))
        {
            return true;
        }
        obj = it.Next();
    }
    return false;
}
