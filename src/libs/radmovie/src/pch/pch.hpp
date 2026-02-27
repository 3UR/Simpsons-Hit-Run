//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef RADMOIVE_PCH_HPP
#define RADMOIVE_PCH_HPP

//
// only win32 and UWP take advantage of the pch at this moment.
//
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
//
// Microsoft header files
//
#define _WIN32_WINNT 0x0602
#include <windows.h>

#include <strmif.h>
#include <control.h>
#include <uuids.h>
#include <evcode.h>
#include <vfwmsgs.h>

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

#endif // RADMOIVE_PCH_HPP
