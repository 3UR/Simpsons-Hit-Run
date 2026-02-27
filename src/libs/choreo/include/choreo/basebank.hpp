/*
 * choreo/basebank.hpp
 */

#ifndef CHOREO_BASEBANK_HPP
#define CHOREO_BASEBANK_HPP


#include <p3d/inventory.hpp>
#include <typeinfo>


namespace choreo
{


class BaseBank;
class Rig;


//---------------------------------------------------------------------------
// convenience routines
//---------------------------------------------------------------------------

typedef void (*FindHook)(const BaseBank* bank, const std::type_info& typeInfo, const char* name, tUID uid, void* context);
void InstallFindHook(FindHook findHook, void* context);
FindHook GetFindHook();
void* GetFindHookContext();

template <class T>
inline T* find(const BaseBank* bank, tUID uid);

template <class T>
inline T* find(const BaseBank* bank, const char* name);


//---------------------------------------------------------------------------
// interface BaseBank
//---------------------------------------------------------------------------

class BaseBank: public tEntity
{
public:

    // allowed to return 0 here
    virtual Rig* GetRig() const = 0;
    
    virtual IRefCount* Find(radLoadInventory::SafeCastBase& safeCaster, tUID uid) const = 0;
    IRefCount* Find(radLoadInventory::SafeCastBase& safeCaster, const char* name) const
        { return Find(safeCaster, tEntity::MakeUID(name)); }

    virtual void Store(tEntity* entity) = 0;

    // raw iterator
    class RawIterator: public tRefCounted
    {
    public:

        virtual IRefCount* First() = 0;
        virtual IRefCount* Last() = 0;
        virtual IRefCount* Next() = 0;
        virtual IRefCount* Prev() = 0;
    };

    virtual RawIterator* NewRawIterator() = 0;
};

template <class T>
inline T* find(const BaseBank* bank, tUID uid)
{
    FindHook findHook = GetFindHook();
    if (findHook != 0)
    {
        (*findHook)(bank, typeid(T), "", uid, GetFindHookContext());
    }

    radLoadInventory::SafeCast<T> safeCaster;
    return (T*)bank->Find(safeCaster, uid);
}

template <class T>
inline T* find(const BaseBank* bank, const char* name)
{
    tUID uid = tEntity::MakeUID(name);

    FindHook findHook = GetFindHook();
    if (findHook != 0)
    {
        (*findHook)(bank, typeid(T), name, uid, GetFindHookContext());
    }

    radLoadInventory::SafeCast<T> safeCaster;
    return (T*)bank->Find(safeCaster, uid);
}


//---------------------------------------------------------------------------
// class MultiBank
//---------------------------------------------------------------------------

class MultiBank: public BaseBank
{
public:

    MultiBank(BaseBank* storeBank, BaseBank* secBank);
    
    virtual Rig* GetRig() const
        { return m_StoreBank->GetRig(); }
    
    virtual IRefCount* Find(radLoadInventory::SafeCastBase& safeCaster, tUID uid) const;
    virtual void Store(tEntity* entity);
    
    virtual BaseBank::RawIterator* NewRawIterator()
		{ return m_StoreBank->NewRawIterator(); }

protected:

    virtual ~MultiBank();

private:
    
    BaseBank* m_StoreBank;
    BaseBank* m_SecondaryBank;
};


//---------------------------------------------------------------------------
// class P3DEntityStoreWrapperBank
//---------------------------------------------------------------------------

class P3DEntityStoreWrapperBank: public BaseBank
{
public:

    P3DEntityStoreWrapperBank(tEntityStore* entityStore);

    virtual Rig* GetRig() const
        { return 0; }
	tEntityStore* GetEntityStore() const
		{ return m_EntityStore; }

    virtual IRefCount* Find(radLoadInventory::SafeCastBase& safeCaster, tUID uid) const;
    virtual void Store(tEntity* entity);

    virtual BaseBank::RawIterator* NewRawIterator();

protected:

    virtual ~P3DEntityStoreWrapperBank();

private:
    
    tEntityStore* m_EntityStore;
};


//---------------------------------------------------------------------------
// class P3DInventoryWrapperBank
//---------------------------------------------------------------------------

class P3DInventoryWrapperBank: public BaseBank
{
public:

    P3DInventoryWrapperBank(tInventory* inventory);

    virtual Rig* GetRig() const
        { return 0; }
	tInventory* GetInventory() const
		{ return m_Inventory; }

    virtual IRefCount* Find(radLoadInventory::SafeCastBase& safeCaster, tUID uid) const;
    virtual void Store(tEntity* entity);

    // raw iterator
    class RawIterator: public BaseBank::RawIterator
    {
    public:

        RawIterator(P3DInventoryWrapperBank* p3dInvBank);

        virtual IRefCount* First();
        virtual IRefCount* Last();
        virtual IRefCount* Next();
        virtual IRefCount* Prev();

	protected:

		virtual ~RawIterator()
			{ }

    private:

        tEntityTable::RawIterator m_RawIterator;
    };

    virtual BaseBank::RawIterator* NewRawIterator();

protected:

    virtual ~P3DInventoryWrapperBank();

private:
    
    tInventory* m_Inventory;
};


} // namespace choreo


#endif
