/*===========================================================================
    opticloader.hpp

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _OPTIC_LOADER_HPP
#define _OPTIC_LOADER_HPP

#include <p3d/loadmanager.hpp>

class tLensFlare;

//*****************************************************************************
//
// Class tLensFlareGroupLoader - loads lens flare group 
//
//*****************************************************************************
class tLensFlareGroupLoader: public tSimpleChunkHandler
{
public:
    tLensFlareGroupLoader();
protected:
    ~tLensFlareGroupLoader() {};
    tEntity* LoadObject(tChunkFile* chunk, tEntityStore* store);
    tLensFlare* LoadLensFlare(tChunkFile* chunk, tEntityStore* store);
};

#endif;
