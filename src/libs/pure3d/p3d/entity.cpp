//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

#include <p3d/entity.hpp>
#include <p3d/error.hpp>
#include <p3d/utility.hpp>
#include <p3d/table.hpp>
#include <string.h>

#include <radmemorymonitor.hpp>
#include <memory/classsizetracker.h>


radMemoryAllocator tName::s_allocator = RADMEMORY_ALLOC_DEFAULT;

// tName Constructors / Destructor ------------------------------------------
tName::tName():
#ifndef RAD_RELEASE
textName(NULL),
#endif
uid( 0 )
{
}

tName::tName(const char* name):
#ifndef RAD_RELEASE
    textName(NULL),
#endif
    uid( 0 )
{
    SetText(name);
}

tName::tName(tUID iUid) 
#ifndef RAD_RELEASE
 :    textName(NULL)
#endif
{
    SetUID(iUid);
}

tName::tName(const tName& other) 
#ifndef RAD_RELEASE
 :    textName(NULL)
#endif
{
    SetUID(other.uid);
#ifndef RAD_RELEASE
    SetTextOnly(other.textName);
#endif
}

tName::~tName()
{
#ifndef RAD_RELEASE
    if(textName)
    {
        #ifndef RAD_MW
            radMemoryFree (s_allocator, (void*)textName);
        #else
            free( textName );
        #endif
    }
#endif
}

void tName::SetAllocator (radMemoryAllocator allocator)
{
    s_allocator = allocator;
}

// tName Set / Get  -----------------------------------------------------------

void tName::SetText(const char* name)
{
    if(name)
        SetUID(tEntity::MakeUID(name));
    else
        SetUID((tUID)0);
    SetTextOnly(name);
}

void tName::SetTextOnly(const char* name)
{
#ifdef P3D_USE_ENTITY_NAMES
    if(textName)
    {
        #ifndef RAD_MW
            radMemoryFree (s_allocator, (void*)textName);
        #else
            free ( (void*)textName);
        #endif
        textName = NULL;
    }
    
    if(name)
    {
        radMemoryMonitorIssueFlag (name);
        #ifndef RAD_MW
            textName = (char*)radMemoryAlloc (s_allocator, strlen(name)+1);
        #else
            textName = (char*)malloc ( strlen(name)+1);
        #endif
        strcpy(textName, name);
    }
#else
    #ifndef RAD_RELEASE
        textName = NULL;
    #endif
#endif
}

void tName::SetUID(tUID u)
{
    uid = u;
}

#ifdef P3D_ALLOW_ENTITY_GETNAME
const char* tName::GetText(void) const
{
    static char unnamed[] = "NO_NAME_TEXT";

    #ifndef RAD_RELEASE
        if( textName )
        {
            return textName;
        }
    #endif

    // otherwise return a dummy name, so that this will never return NULL
    return unnamed;
}
#endif

// tName operators  -----------------------------------------------------------
bool tName::operator==(const tName& other) const
{
    return GetUID()==other.GetUID();//(((tUID)uid[0])<<32|((tUID)uid[1])) == (((tUID)other.uid[0])<<32|((tUID)other.uid[1])) ;
}

bool tName::operator!=( const tName& other ) const
{
    return !( *this == other );
}

const tName& tName::operator=(const tName& other)
{
    //Assignment of self to self, strcpy doesn't like that.
    if(this == &other)
        return *this;

    #ifndef RAD_RELEASE
        SetTextOnly( other.textName );
    #endif
        SetUID(other.uid);
    return *this;
}

//---------------------------------------------------------------------------
// Hashing function that uses the x65599 algorithm
// Taken from Dragon book, p435
tUID tName::MakeUID(const char *x)
{
    return radMakeKey(x);
}

// tEntity --------------------------------------------------------------------
tEntity::tEntity()
{
    CLASSTRACKER_CREATE( tEntity );
}

tEntity::tEntity(tEntity* other)
{
    CLASSTRACKER_CREATE( tEntity );
    CopyName(other);
}

tEntity::~tEntity()
{
    CLASSTRACKER_DESTROY( tEntity );
}

void tEntity::CopyName(tEntity* other)
{
    name = other->name;
}

const char* tEntity::GetNameDangerous(void) 
{
#ifdef P3D_ALLOW_ENTITY_GETNAME
    return GetName();
#else
    static char unnamed[] = "unnamed";
    return unnamed;
#endif
}


