//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/locator.hpp>


//----------------------------------------------------------------
tLocator::tLocator()
{
    
}

tLocator::~tLocator()
{
    
}

void tLocator::SetPosition(rmt::Vector pos)
{
    position = pos;
}

tLocatorLoader::tLocatorLoader() : tSimpleChunkHandler(Pure3D::Locator::LOCATOR)
{
}

tEntity* tLocatorLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    char name[256];
    f->GetPString(name);
    unsigned long version = f->GetLong();
    rmt::Vector pos;
    pos.x = f->GetFloat();
    pos.y = f->GetFloat();
    pos.z = f->GetFloat();

    tLocator *locator = new tLocator;
    locator->SetName(name);
    locator->SetPosition(pos);

    return locator;
}
