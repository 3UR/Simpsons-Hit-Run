//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _PDDI_BUILDCONFIG_HPP_
#define _PDDI_BUILDCONFIG_HPP_
// PDDI Build Configuration

// toollib builds part of the ps2 source because of the memory imaging, 
// we need some hacks to deal with that
#ifdef TOOLLIB
#define PDDI_USE_STD_ASSERTS
#endif

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

// ---------- Select FTT components PDDI will use -------------
//#define P3D_USE_FTT_MEMTRACK    // Use memory tracking for object not allocated with new (not working right now)

//#define XBOX_SKIN_FOUR_WEIGHTS          // allow four weights to be blended in skins

// -------- set up individual debugging features ----------------

// PDDI_TRACK_STATS - track and display rendering statistic (pddiContext::EnableStatsOverlay)
//
// PDDI_USE_ASSERTS - Asserts result in error messages (passed through to Pure3D error handling system)
//
// PDDI_USE_STD_ASSERTS - 'PDDIASSERT' #defines to standard C assert. Needed by toolib, and if you don't 
//        want to use the Pure3D error routing system

// Debug build, full debugging support, slow as hell
#ifdef RAD_DEBUG
#define PDDI_TRACK_STATS
#define PDDI_USE_ASSERTS
#else

// Tune Build configuration, some debugging, high speed 
#ifdef RAD_TUNE
#define PDDI_TRACK_STATS
#define PDDI_USE_ASSERTS
#else

// Release build config, no debugging at all, maximum speed
#ifdef RAD_RELEASE
//#define PDDI_TRACK_STATS
//#define PDDI_USE_ASSERTS
#endif
#endif
#endif

#endif