/*===========================================================================
    inventory.hpp
    18-Oct-95 Created by Neall
    23-Dec-99 Completely revamped for v12

    Copyright (c)1999  Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#include <p3d/inventory.hpp>
#include <p3d/utility.hpp>
#include <p3d/error.hpp>

#include <typeinfo>

const int P3D_INVALID_SECTION = -1;

tInventory::tInventory() :
    autoRepack(true),
    nSection(0),
    currentSection(0),
    currentSectionOnly(false),
    sectionPathEnabled(false),
    nSectionPath(0),
    sectionStack(P3D_MAX_CURRENT_SECTIONS)
{
    AddSection(P3D_DEFAULT_INV_SECTION);
    for(int i=0; i < P3D_MAX_SECTION_PATH_SECTIONS; i++)
    {
        sectionPath[i] = 0;
    }
}

tInventory::~tInventory()
{
    DeleteAllSections();
    section[0]->Release();
}

//-------------------------------------------------------------------
IRefCount* tInventory::Find(radLoadInventory::SafeCastBase& caster, const tUID uid)
{
    // first search the current section
    tEntity* obj = SearchSection(currentSection, caster, uid);
    
    // if the object wasn't found, search the remaining sections (if we're allowed to)
    if(!obj && !currentSectionOnly)
    {
        if(sectionPathEnabled)
        {
            for(int i=0; i < nSectionPath; i++)
            {
                int sectionIndex = FindSection(sectionPath[i]);
                if(sectionIndex != P3D_INVALID_SECTION)
                {
                    if((obj = SearchSection(sectionIndex, caster, uid)))
                    {
                        return obj;
                    }
                }
                #ifdef RAD_DEBUG
                else
                {
                    static bool once = true;
                    if(once)
                    {
                        once = false;
                        p3d::print("Pure3D Warning:  Unknown section specified in AddSectionPath()\n");
                    }
                }
                #endif
            }
        }
        else
        {
            for(unsigned i = 0; i < nSection; i++)
            {
                if(i == currentSection) // we've already searched this one
                    continue;

                if((obj = SearchSection(i, caster, uid)))
                {
                    return obj;
                }
            }
        }
    }
    return obj;
}

//-------------------------------------------------------------------
void tInventory::Store(tEntity *obj)
{
    Store(obj->GetUID(),obj);
}
//-------------------------------------------------------------------
void tInventory::Store(radKey id, IRefCount* obj)
{
    section[currentSection]->Store(id,obj);
}
//-------------------------------------------------------------------
void tInventory::StoreHandlingCollisions( tEntity* obj )
{
    tUID uid = obj->GetUID();
    StoreHandlingCollisions( uid, obj );
}
//-------------------------------------------------------------------
void tInventory::StoreHandlingCollisions( radKey id, IRefCount* obj )
{
    section[ currentSection ]->StoreHandlingCollisions( id, obj );
}
//-------------------------------------------------------------------
void tInventory::Remove(IRefCount* obj)
{
    if(currentSectionOnly)
    {
        section[currentSection]->Remove(obj);
    }
    else
    {
        for(unsigned i = 0; i < nSection; i++)
        {
            section[i]->Remove(obj);
        }
    }
}

//-------------------------------------------------------------------
class tSafeCasterObj : public radLoadInventory::SafeCastBase
{
public:
    tSafeCasterObj(IRefCount* e) { obj = e;}
    IRefCount* safe_cast(IRefCount* c) { return typeid(*c) == typeid(*obj) ? c : NULL; }

private:
    IRefCount* obj;
};

bool tInventory::TestCollision(tEntity* obj)
{
    tSafeCasterObj cast(obj);
    return SearchSection(currentSection, cast, obj->GetUID()) != NULL;;
}

//-------------------------------------------------------------------
void tInventory::SetSectionPath(const char* p0, const char* p1, const char* p2, const char* p3,
                                const char* p4, const char* p5, const char* p6, const char* p7 )
{
    nSectionPath = 0;
    if(p0) { sectionPath[0] = tEntity::MakeUID(p0); nSectionPath++; }
    if(p1) { sectionPath[1] = tEntity::MakeUID(p1); nSectionPath++; }
    if(p2) { sectionPath[2] = tEntity::MakeUID(p2); nSectionPath++; }
    if(p3) { sectionPath[3] = tEntity::MakeUID(p3); nSectionPath++; }
    if(p4) { sectionPath[4] = tEntity::MakeUID(p4); nSectionPath++; }
    if(p5) { sectionPath[5] = tEntity::MakeUID(p5); nSectionPath++; }
    if(p6) { sectionPath[6] = tEntity::MakeUID(p6); nSectionPath++; }
    if(p7) { sectionPath[7] = tEntity::MakeUID(p7); nSectionPath++; }
}

void tInventory::AddSection(const char* name)
{
    int sectionIndex = FindSection(tEntity::MakeUID(name));
    if (sectionIndex == P3D_INVALID_SECTION)
    {
         P3DASSERT(nSection < (unsigned)P3D_MAX_INVENTORY_SECTIONS);
         section[nSection] = new tEntityTable;
         section[nSection]->AddRef();
         section[nSection]->SetName(name);
         section[nSection]->SetAutoRepack(autoRepack);
         nSection++;
    }
}


void tInventory::DeleteSection(tUID uid)
{
    int sectionIndex = FindSection(uid);
    if(sectionIndex == 0)
    {

        p3d::printf("inventory : cannot delete default section\n");

    } else  if(sectionIndex == P3D_INVALID_SECTION)
    {

        p3d::printf("inventory : tried to delete invalid section\n");

    } else 
    {
        section[sectionIndex]->Release();
        section[sectionIndex] = NULL;
        if((unsigned)sectionIndex == currentSection)
        {
            currentSection = 0;
        }


        for(int i=sectionIndex; i < P3D_MAX_INVENTORY_SECTIONS-1; i++)
        {
            section[i] = section[i+1];
        }

        if(currentSection > (unsigned)sectionIndex)
            currentSection--;

        section[nSection+1] = NULL;
        nSection--;
    }
}

tEntityTable* tInventory::GetSection(tUID uid)
{
    int sectionIndex = FindSection(uid);
    if(sectionIndex >= 0)
    {
        return section[sectionIndex];
    }
    else
    {
        p3d::printf("inventory : tried to retreive invalid section, reverting to default\n");
        return section[0];
    }
}

void tInventory::RemoveSectionElements(tUID uid)
{
    int sectionIndex = FindSection(uid);
    if(sectionIndex > 0)
    {
        section[sectionIndex]->RemoveAll();
    }
    else
    {
        rDebugString("inventory : tried to remove elements form invalid section, remove ignored\n");
        return;
    }
}

void tInventory::SelectSection( const tName& name )
{
    // Check if trying to select the current selection
    if (section[currentSection]->GetName() == name )
    {
        return;
    }

    int sectionIndex = FindSection( name );
    if(sectionIndex != P3D_INVALID_SECTION)
    {
        currentSection = sectionIndex;
        return;
    }
    else
    {
        p3d::printf("inventory : selected invalid section, using default section\n");
        currentSection = 0;
        return;
    }
}



void tInventory::PushSection(void)
{
    sectionStack.Push( section[ currentSection ]->GetName() );
}


void tInventory::PopSection()
{
    SelectSection(sectionStack.Pop());
}


int tInventory::FindSection( const tName& name )
{
    for(unsigned i = 0; i < nSection; i++)
    {
        if( section[i]->GetName() == name )
        {
            return i;
        }
    }
    return P3D_INVALID_SECTION;
}

tEntity* tInventory::SearchSection(unsigned s, radLoadInventory::SafeCastBase& caster, const tName& name )
{
    tEntityTable::Iterator iterator(section[s]);
    IRefCount* obj = iterator.First( name.GetUID() );
    while(obj)
    {
        if(caster.safe_cast(obj))
        {
            return static_cast<tEntity*>(obj);
        }
        obj = iterator.Next();
    }
    return NULL;
}

//-------------------------------------------------------------------
void tInventory::RePack()
{
    for(unsigned i = 0; i < nSection; i++)
    {
        section[i]->RePack();
    }
}

void tInventory::SetAutoRepack(bool enable)
{
    autoRepack = enable;
    for(unsigned i = 0; i < nSection; i++)
    {
        section[i]->SetAutoRepack(autoRepack);
    }
}

bool tInventory::GetAutoRepack()
{
    return autoRepack;
}

//-------------------------------------------------------------------
void tInventory::RemoveAllElements()
{
    for(unsigned i = 0; i < nSection; i++)
    {
        section[i]->RemoveAll();
    }
}

void tInventory::DeleteAllSections()
{
    // we don't allow the first (default) section to be deleted
    for(unsigned i = 1; i < nSection; i++)
    {
        section[i]->Release();
        section[i] = NULL;
    }
    nSection = 1;
    currentSection = 0;
}

#ifndef RAD_RELEASE
void tInventory::Dump (bool detail)
{
    rDebugString ("\nInventory Dump:\n");
    for (unsigned i = 0; i < nSection; i++)
    {
        tEntityTable* table = section[i];
        rDebugPrintf ("Section %p: %d elements\n", table->GetUID(), table->GetElementCount ()); 

        if (detail)
        {
            SelectSection (table->GetUID());

            tInventory::Iterator<tEntity> iterator;
            tEntity* entity;
            for (entity = iterator.First(); entity; entity = iterator.Next ())
            {
                rDebugPrintf ("    %p %s\n", entity, entity->GetName ());
            }
        }
    }
    rDebugString ("\n");
}
#endif
//-------------------------------------------------------------------
void tEntityStore::Store( tEntity* entity )
{   
    bool collision = TestCollision( entity->GetUID(), entity );
    //rAssert( collision == false );
    if( !collision )
    {
        Store( entity->GetUID(), entity );
    }
    else
    {
        entity->AddRef();
        entity->Release();
    }
}
//-------------------------------------------------------------------
void tEntityStore::Store( radKey id, IRefCount* obj )
{
    radLoadHashedStore::Store(id, obj);
}
//-------------------------------------------------------------------
void tEntityStore::StoreHandlingCollisions( tEntity* entity )
{
    bool collision = TestCollision( entity->GetUID(), entity );
    tUID uid = entity->GetUID();
    StoreHandlingCollisions( uid, entity );
}
//-------------------------------------------------------------------
void tEntityStore::StoreHandlingCollisions( radKey id, IRefCount* obj  )
{
    radLoadHashedStore::StoreHandlingCollisions( id, obj );
}