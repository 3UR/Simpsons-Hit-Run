//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _UTILITY_HPP
#define _UTILITY_HPP

#include <stdarg.h>
#include <stdio.h>

#include <p3d/context.hpp>
#include <p3d/inventory.hpp>
#include <p3d/file.hpp>
#include <p3d/error.hpp>
#include <p3d/memory.hpp>

#ifdef P3D_DEBUG_FTT_MESSAGES
#include <raddebug.hpp>
#endif

struct IRadFileCompletionCallback;

#if defined( RAD_WIN32 ) || defined ( RAD_UWP )
    #include <string.h> // for strlen()
#endif

extern bool gFruitless;

namespace p3d
{
    extern tPlatform*           platform;
    extern tContext*            context;
    extern tInventory*          inventory;
    extern tMatrixStack*        stack;
    extern tLoadManager*        loadManager;
    extern pddiRenderContext*   pddi;
    extern pddiDevice*          device;
    extern pddiDisplay*         display;

    tFile* openFile(const char* name, bool sync = true);

    inline bool load(const char* name)
    {
        return loadManager->Load(new tLoadRequest(openFile(name))) == LOAD_OK;
    }

    inline bool load(tFile* file)
    {
        return loadManager->Load(new tLoadRequest(file)) == LOAD_OK;
    }


    inline bool load(unsigned char* buffer, unsigned size, unsigned pool = 0, const char* filename = "tmp.p3d")
    {
        tFileMem* file = new tFileMem(buffer,size);
        file->SetFilename(filename);
        radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
        tLoadRequest* load = new tLoadRequest (file);
        ::radMemorySetCurrentAllocator( old );
        load->SetMemoryAllocator(pool);
        bool rc = loadManager->Load(load) == LOAD_OK;
        return rc;
    }

    inline tLoadRequest* loadAsync(const char* name, const char* section = NULL, tLoadRequest::Callback* done = NULL, void* user = NULL, unsigned pool = 0)
    {
        radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
        tLoadRequest* load = new tLoadRequest(openFile(name,false));
        ::radMemorySetCurrentAllocator( old );
        load->SetAsync(true);
        load->SetInventorySection(section);
        load->SetCallback(done);
        load->SetUserData(user);
        load->SetMemoryAllocator(pool);
        loadManager->Load(load);
        return load;
    }

    inline tLoadRequest*  loadAsync(tFile* file, const char* section = NULL,  tLoadRequest::Callback* done = NULL)
    {
        tLoadRequest* load = new tLoadRequest(file);
        load->SetAsync(true);
        load->SetInventorySection(section);
        load->SetCallback(done);
        loadManager->Load(load);
        return load;
    }

    template <class T> inline T* find( tEntityStore* inventory, const tName& name )
    {
        if( name.GetUID() == tName::MakeUID("") || name.GetUID() == tName::MakeUID("NO_NAME_TEXT") )
        {
            return NULL;
        }
        else
        {
            T* obj = NULL;
            radLoadInventory::SafeCast<T> foo;
            obj = (T*)inventory->Find( foo, name.GetUID() );

            tEntityStore* mainInventory = tPlatform::GetPlatform()->GetActiveContext()->GetInventory();
            
            if(!obj && mainInventory != inventory ) 
            {
                //
                // Do a second search only if the first search was for an inventory
                // other than the main p3d one. This case arises when we're loading,
                // but not usually, if someone does a find during their game thread.
                //
                obj = (T*)mainInventory->Find(foo, name.GetUID() );
            }
#ifdef RAD_DEBUG
            if(obj == NULL)
            {
                if ( gFruitless )
                {
                    rDebugPrintf("WARNING: Fruitless Search for %s entity.\n", name.GetText());
                }
            }
            //rAssert(strlen(name.GetText()) < 70);
#endif
            return obj;
        }
    }

    template <class T> inline T* find(tEntityStore* inventory, const char* name)
    {
        tName tname = name;
        return find<T>(inventory, tname );
    }

    template <class T> inline T* find(const char* name)
    {
        return find<T>(tPlatform::GetPlatform()->GetActiveContext()->GetInventory(), name);
    }

    template <class T> inline T* find(const tUID uid)
    {
        return find<T>(tPlatform::GetPlatform()->GetActiveContext()->GetInventory(), uid);
    }

    void printf(const char* s, ...);
#ifdef P3D_DEBUG_MESSAGES
    void print(const char* s);

    // use this function just like printf
    // e.g. p3d::printf("x = %f\n", fX);
    // VC: Also, double clicking on  the line in the output window will take you
    // to the calling print command in the source code
#else
    inline void print(const char*) {return;};
#endif

    bool ClassName(tEntity* obj, char* buf, unsigned size);

    void InstallDefaultLoaders();
}

#endif /* UTILITY_HPP */

