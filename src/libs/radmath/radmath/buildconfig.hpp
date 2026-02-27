//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _RADMATH_BUILDCONFIG_HPP_
#define _RADMATH_BUILDCONFIG_HPP_
// Radmath Build Configuration

// ---------- Verify build config -------------------------------
// check that we have a build config define
#if !defined(RAD_DEBUG) && !defined(RAD_TUNE) && !defined(RAD_RELEASE) 
#error No build target define (one of RAD_DEBUG, RAD_TUNE or RAD_RELEASE)
#endif

// check that we have a valid platform define
#if !defined(RAD_UWP) && !defined(RAD_WIN32) && !defined(RAD_LINUX)
#error No platform define, define one of RAD_UWP, RAD_WIN32 or RAD_LINUX 
#endif

// ---------- Set legacy defines -------------

// This'll proably be defined, but...
#ifdef RAD_RELEASE
#ifndef NDEBUG
#define NDEBUG
#endif
#endif

// -------- set up individual debugging features ----------------
// Debug build, full debugging support, slow as hell
#ifdef RAD_DEBUG
//#define RADMATH_NOASM
#else

// Tune Build configuration, some debugging, high speed 
#ifdef RAD_TUNE
//#define RADMATH_NOASM
#else

// Release build config, no debugging at all, maximum speed
#ifdef RAD_RELEASE
//#define RADMATH_NOASM
#endif

#endif
#endif

#endif
