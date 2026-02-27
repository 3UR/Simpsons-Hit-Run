//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include <radtypeinfo.hpp>
#include <radtypeinfoutil.hpp>
#include <radfile.hpp>

//============================================================================
// Function: SyncLoadCallback
//============================================================================

static unsigned int g_SyncLoadCount = 0;

static void SyncLoadCallback( void * pUserData )
{
    rAssert( g_SyncLoadCount > 0 );

    g_SyncLoadCount--;
}
