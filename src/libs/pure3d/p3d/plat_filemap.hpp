//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _BASE_PLAT_FILEMAP_HPP
#define _BASE_PLAT_FILEMAP_HPP

#if defined(RAD_LINUX)
    #include <p3d/platform/linux/plat_filemap.hpp>
#elif defined(RAD_WIN32) || defined(RAD_UWP)
    #include <p3d/platform/win32/plat_filemap.hpp>
#endif

#endif // _BASE_PLAT_FILEMAP_HPP