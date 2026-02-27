#ifndef RADMUSIC_PCH_HPP
#define RADMUSIC_PCH_HPP

//
// only win32 and UWP take advantage of the pch at this moment.
//
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
//
// Microsoft header files
//
#define _WIN32_WINNT 0x0602
#include <windows.h>

//
// Standard C header files
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <radoptions.hpp>
#include <radfile.hpp>
#include <radmemory.hpp>
#include <radsound.hpp>

#endif // defined RAD_WIN32 || defined RAD_UWP
#endif // RADMUSIC_PCH_HPP