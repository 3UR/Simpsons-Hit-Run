//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   FeEntity
//
// Description: The base class for our FE objects.
//
// Authors:     Wilkin Ng
//
// Revisions        Date            Author        Revision
//                  2000/08/29        WNg         Created
//                  2002/01/21      MJP         Now uses tEntity as base
//
//===========================================================================

#ifndef __FeEntity__
#define __FeEntity__

//#define SCROOBY_MEMORY_CHECKING

//===========================================================================
// Nested Includes
//===========================================================================
#include "utility/memory.h"
#include <p3d/entity.hpp>

//===========================================================================
// Forward References
//===========================================================================

//===========================================================================
// Interface Definitions
//===========================================================================
class FeEntity 
: 
    //virtual public FeAllocatesMemory,
    public tEntity
{
    public:
        FeEntity();

#ifdef SCROOBY_MEMORY_CHECKING
        void SetName( const tName& name );          
        void AddRef();
        void Release();
#endif

        virtual bool IsDrawable() = 0;
        virtual bool IsOwner() const;

    protected:
        virtual ~FeEntity();

};

class FeEntityDummy : public FeEntity
{
public:
    virtual bool IsDrawable() { return false; }

};

#endif
