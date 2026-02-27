/*
 * choreo/basebank.cpp
 */

#include <choreo/basebank.hpp>

#include <p3d/error.hpp>


namespace choreo
{


//---------------------------------------------------------------------------
// find routines
//---------------------------------------------------------------------------

static FindHook g_FindHook = 0;
static void* g_FindHookContext = 0;

void InstallFindHook(FindHook findHook, void* context)
{
    g_FindHook = findHook;
    g_FindHookContext = context;
}

FindHook GetFindHook()
{
    return g_FindHook;
}

void* GetFindHookContext()
{
    return g_FindHookContext;
}


//---------------------------------------------------------------------------
// class MultiBank
//---------------------------------------------------------------------------

MultiBank::MultiBank(BaseBank* storeBank, BaseBank* secBank)
{
    P3DASSERT(storeBank != 0);
    m_StoreBank = storeBank;
    m_StoreBank->AddRef();

    P3DASSERT(secBank != 0);
    m_SecondaryBank = secBank;
    m_SecondaryBank->AddRef();
}

MultiBank::~MultiBank()
{
    m_SecondaryBank->Release();
    m_StoreBank->Release();
}

IRefCount* MultiBank::Find(radLoadInventory::SafeCastBase& safeCaster, tUID uid) const
{
    IRefCount* entity = m_StoreBank->Find(safeCaster, uid);
    if (entity != 0)
        return entity;
    return m_SecondaryBank->Find(safeCaster, uid);
}

void MultiBank::Store(tEntity* entity)
{
    m_StoreBank->Store(entity);
}


//---------------------------------------------------------------------------
// class P3DEntityStoreWrapperBank
//---------------------------------------------------------------------------

P3DEntityStoreWrapperBank::P3DEntityStoreWrapperBank(tEntityStore* entityStore)
{
    P3DASSERT(entityStore != 0);
    m_EntityStore = entityStore;
    m_EntityStore->AddRef();
}

P3DEntityStoreWrapperBank::~P3DEntityStoreWrapperBank()
{
    m_EntityStore->Release();
}

IRefCount* P3DEntityStoreWrapperBank::Find(radLoadInventory::SafeCastBase& safeCaster, tUID uid) const
{
    return (tEntity*)m_EntityStore->Find(safeCaster, uid);
}

void P3DEntityStoreWrapperBank::Store(tEntity* entity)
{
    m_EntityStore->Store(entity);
}

BaseBank::RawIterator* P3DEntityStoreWrapperBank::NewRawIterator()
{
    // tEntityStores don't have raw iterators
    return 0;
}


//---------------------------------------------------------------------------
// class P3DInventoryWrapperBank::RawIterator
//---------------------------------------------------------------------------

P3DInventoryWrapperBank::RawIterator::RawIterator(P3DInventoryWrapperBank* p3dInvBank):
    m_RawIterator(p3dInvBank->GetInventory()->GetCurrentSection())
{
}

IRefCount* P3DInventoryWrapperBank::RawIterator::First()
{
	return (tEntity*)m_RawIterator.First();
}

IRefCount* P3DInventoryWrapperBank::RawIterator::Last()
{
	return (tEntity*)m_RawIterator.Last();
}

IRefCount* P3DInventoryWrapperBank::RawIterator::Next()
{
	return (tEntity*)m_RawIterator.Next();
}

IRefCount* P3DInventoryWrapperBank::RawIterator::Prev()
{
	return (tEntity*)m_RawIterator.Prev();
}


//---------------------------------------------------------------------------
// class P3DInventoryWrapperBank
//---------------------------------------------------------------------------

P3DInventoryWrapperBank::P3DInventoryWrapperBank(tInventory* inventory)
{
    P3DASSERT(inventory != 0);
    m_Inventory = inventory;
    m_Inventory->AddRef();
}

P3DInventoryWrapperBank::~P3DInventoryWrapperBank()
{
    m_Inventory->Release();
}

IRefCount* P3DInventoryWrapperBank::Find(radLoadInventory::SafeCastBase& safeCaster, tUID uid) const
{
    return (tEntity*)m_Inventory->Find(safeCaster, uid);
}

void P3DInventoryWrapperBank::Store(tEntity* entity)
{
    m_Inventory->Store(entity);
}

BaseBank::RawIterator* P3DInventoryWrapperBank::NewRawIterator()
{
	return new RawIterator(this);
}


} // namespace choreo


// End of file.
