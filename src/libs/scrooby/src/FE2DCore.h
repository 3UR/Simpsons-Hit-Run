// FE2DCore.h
// Created by wng on Wed, Apr 05, 2000 @ 12:39 PM.

#ifndef __Fe2DCore__
#define __Fe2DCore__

#include "p2d/core.hpp"
#include "utility/memory.h"

class Fe2DCore 
: 
    public Pure2D::Core,
    public FeAllocatesMemory
{
    public:
        // Accessor to this singleton.
        //
        static Fe2DCore* GetInstance();

        // Client must call this to clean up when finished with this singleton.
        //
        static void DeleteInstance();
        
        
        void AddSection(char* sectionname);
        
        void RemoveSectionElement(char* name);
        
        tDrawable* GetHandlePrimitive(unsigned p);
        
        void DeleteSection(char* sectionname);

    private:
        
        static Fe2DCore* sInstance;

        Fe2DCore();
        virtual ~Fe2DCore();
};

#endif
