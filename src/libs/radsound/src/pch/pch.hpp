//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef RADSOUND_PCH_HPP
#define RADSOUND_PCH_HPP

//
// only win32 and uwp take advantage of the pch at this moment.
//
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
//
// Microsoft header files
//
#define _WIN32_WINNT 0x0602

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>

//
// Standard C header files
//
#include <stdio.h>

#endif // defined RAD_WIN32 || defined RAD_UWP

#endif // RADSOUND_PCH_HPP
