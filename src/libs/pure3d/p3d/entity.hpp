//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _ENTITY_HPP
#define _ENTITY_HPP

#include <p3d/p3dtypes.hpp>
#include <p3d/refcounted.hpp>
#include <radmemory.hpp>

// a class to uniquly identify things, stores a 64 bit UID (hash) value and possibly a real name 
// 'invalid' in the context of the following comments means not usable, 
// get functions will never assert or return an unusable value just one that might not be meaningful
class tName
{
public:
    // various constructors
    tName(); // default constructor, both uid and text will be invalid
    tName(const char* name); // sets text name and uid
    tName(tUID uid); // sets uid, text will be invalid
    tName(const tName&); // copy from another name

    ~tName(); // the destructor

    // Set the heap that this class will use for allocations
    static void SetAllocator (radMemoryAllocator allocator);

    // set the name from a text string, will also set uid 
    void  SetText(const char* name);

    // set UID directly, text name will be invalid
    void  SetUID(tUID uid);

    // assignment from another name
    const tName& operator=(const tName& other);

    // retreive the real text string of the name
    // there will be no name stored if P3D_USE_ENTITY_NAMES is not defined in buildconfig.hpp (generally in release mode) 
    // you SHOULD NOT call this funciton unless:
    //     it is debugging code, where an incorrect string value will only result in invalid debugging/profiling info
    //     P3D_USE_ENTITY_NAMES is defined in all you build configurations
    // if P3D_USE_ENTITY_NAMES is not defined (in release mode) the string "NO_NAME_TEXT" will always be returned
    // use GetUID instead for non-debugging code 
#ifdef P3D_ALLOW_ENTITY_GETNAME
    const char* GetText(void) const;
#endif

    // retreive the uid
    tUID  GetUID(void)  const 
    { 
        return uid;
    }

    // test if two names are the same (have the same uid)
    bool operator==(const tName& other)  const;
    bool operator!=( const tName& other ) const;

    // generate a hash value
    static tUID MakeUID(const char *s);

    class TextName;

private:
    void SetUID(const int* u);
    void SetTextOnly(const char*); //set the text without changing the UID
    // the real data
    //tUID uid;
    tUID uid;
#ifndef RAD_RELEASE
    char* textName;
#endif

    static radMemoryAllocator s_allocator;
};

// the base class of most Pure3D objects : a reference counted object with a name
class tEntity : public tRefCounted
{
public:
    tEntity();               // constructs with invalid name
    tEntity(tEntity* other); // construct, copying the name from another entity

    void CopyName(tEntity *ent); // copy a name directly from another object post construction

    // set and retrive name data
    void SetName(const char *n)        { name.SetText(n); }
    void SetUID(tUID uid)              { name.SetUID(uid); }
    void SetNameObject(const tName& n) { name = n; }
    tUID         GetUID(void)  const { return name.GetUID(); }
    const tName& GetNameObject(void) const { return name; }

    // retreive the text string of the entity's name
    // there will be no name stored if P3D_USE_ENTITY_NAMES is not defined in buildconfig.hpp (generally in release mode) 
    // you SHOULD NOT call this funciton unless:
    //     it is debugging code, where an incorrect string value will only result in invalid debugging/profiling info
    //     P3D_USE_ENTITY_NAMES is defined in all you build configurations
    // if P3D_USE_ENTITY_NAMES is not defined (in release mode) the string "NO_NAME_TEXT" will always be returned
    // use GetUID instead for non-debugging code 
#ifdef P3D_ALLOW_ENTITY_GETNAME
    const char*  GetName(void) const { return name.GetText(); }
#endif

    // like GetName, only even more eXtreme
    const char* GetNameDangerous();

    // generate a hash value, just calls into tName, bit is left here for lagacy reasons
    static tUID MakeUID(const char *s) { return tName::MakeUID(s);}

protected:
    virtual ~tEntity();

private:
    tName name;
};

#endif /* _ENTITY_HPP */

