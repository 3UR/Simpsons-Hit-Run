/*
 * choreo/buildconfig.hpp
 */

#ifndef CHOREO_BUILDCONFIG_HPP
#define CHOREO_BUILDCONFIG_HPP


//----------------------------------------------------------------------
// Build config verification
//----------------------------------------------------------------------

// check that we have a build config define
#if !defined(RAD_DEBUG) && !defined(RAD_TUNE) && !defined(RAD_RELEASE) 
#error No build target defined (one of RAD_DEBUG, RAD_TUNE or RAD_RELEASE)
#define RAD_DEBUG
#endif

// check that we have a valid platform define
#if !defined(RAD_UWP) && !defined(RAD_WIN32) && !defined(RAD_LINUX)
#error No platform defined, define one of RAD_UWP, RAD_WIN32 or RAD_LINUX 
#endif

#if !defined(RAD_WIN32) && !defined(RAD_CONSOLE) 
#error Must define one of RAD_WIN32, RAD_CONSOLE
#endif


//----------------------------------------------------------------------
// Choreo conditional compilation flags
//----------------------------------------------------------------------

// ---------- Select Pure3D components Choreo will use -------------
#define CHOREO_USE_FILE_P3D


// ---------- Select FTT components Choreo will use -------------
#define CHOREO_USE_FILE_FTT


// ---------- Select Choreo components to compile in -------------

#ifndef CHOREO_TOOL
// compile as tool on pc
#if (defined(RAD_WIN32) && !defined(RAD_RELEASE))
#define CHOREO_TOOL
#endif
#endif
#endif