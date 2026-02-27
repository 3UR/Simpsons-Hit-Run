//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _LOCATOR_HPP
#define _LOCATOR_HPP

#include <p3d/entity.hpp>
#include <p3d/loadmanager.hpp>
#include <p3d/chunkfile.hpp>
#include <constants/chunks.h>
#include <constants/chunkids.hpp>


class tLocator : public tEntity
{
public:
    tLocator();
    void SetPosition(rmt::Vector pos);
    const rmt::Vector& GetPosition() {return position;}

protected:
    virtual ~tLocator();
    rmt::Vector position; 
};

class tLocatorLoader : public tSimpleChunkHandler
{
public:
    tLocatorLoader();

protected:
    ~tLocatorLoader() {};
    tEntity* LoadObject(tChunkFile*, tEntityStore*);
};



#endif//_TLOCATOR_HPP

