//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef RADCORE_PCH_HPP
#define RADCORE_PCH_HPP

//
// only win32 and uwp take advantage of the pch at this moment.
//
#if defined RAD_WIN32 || defined RAD_UWP

//
// Microsoft header files
//
#define _WIN32_WINNT 0x0602
#include <windows.h>

#ifndef RAD_UWP
#define DIRECTINPUT_VERSION 0x800
#include <dinput.h>
#endif
#include <crtdbg.h>

//
// Standard C header files
//

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

//
// FTech header files
//
#include "radoptions.hpp"

#endif // defined RAD_WIN32 || defined RAD_UWP

#endif // RADCORE_PCH_HPP
