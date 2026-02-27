//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _BASE_PLATFORM_HPP
#define _BASE_PLATFORM_HPP

#if defined(RAD_LINUX)
    #include <p3d/platform/linux/platform.hpp>
#elif defined(RAD_WIN32) || defined(RAD_UWP)
    #include <p3d/platform/win32/platform.hpp>
#endif

#endif // _BASE_PLATFORM_HPP