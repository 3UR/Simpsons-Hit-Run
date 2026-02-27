//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _PURE3D_BUILDCONFIG_HPP_
#define _PURE3D_BUILDCONFIG_HPP_
// Pure3D Build Configuration

// ---------- Verify build config -------------------------------
// check that we have a build config define
#if !defined(RAD_DEBUG) && !defined(RAD_TUNE) && !defined(RAD_RELEASE) 
#error No build target defined (define one of RAD_DEBUG, RAD_TUNE or RAD_RELEASE)
#define RAD_DEBUG
#endif

// check that we have a valid platform define
#if !defined(RAD_UWP) && !defined(RAD_WIN32) && !defined(RAD_LINUX)
#error No platform defined (define one of RAD_UWP, RAD_WIN32 or RAD_LINUX) 
#endif

#if !defined(RAD_WIN32) && !defined(RAD_CONSOLE) 
#error No platform type defined (Define one of RAD_WIN32, RAD_CONSOLE)
#endif

#if defined(RAD_WIN32) && defined(RAD_CONSOLE) 
#error Cannot define RAD_WIN32 and RAD_CONSOLE at the same time!
#endif

// ---------- Select FTT components Pure3D will use -------------
#define P3D_USE_FTT_FILE    // Use tFileFFT as default Pure3D file
#define P3D_USE_FTT_THREADING  // Use threading support for Async loading
#define P3D_USE_FTT_MEM // Use standard memory allocater
#define P3D_USE_FTT_ASSERTS // use rAssert instead of Pure3D asserts
#define P3D_DEBUG_FTT_MESSAGES // use rPrintf to print debug messages

// -------- Set up general library features ---------------------
// Currently suported are 
//
// P3D_USE_LOW_PRECISION_ANGLES - at load time converts quaternion channels to compressed quaternion channels (halves the space usage)
//

//#define P3D_USE_LOW_PRECISION_ANGLES 


// -------- set up individual debugging features ----------------
// Currently suported are 
// P3D_REFCOUNTED_HEAP_DEBUGGING - Track addresses of valid tRefCounted 
//          objects. Helps catch double deletes, accidental stack allocations, 
//          directly calling delete, and so on
// 
// P3D_REFCOUNTED_HEAP_DEBUGGING_PARANOID - P3D_REFCOUNTED_HEAP_DEBUGGING must be defined. 
//        Tracks objects more agressivly (performs checks in every AddRef and Release call). 
//        Slow but useful when the memory allocator/memory protection stops 
//        P3D_REFCOUNTED_HEAP_DEBUGGING from working properly
//
// P3D_USE_ENTITY_NAMES - Store full text strings names in tEntity objects (if not defiend only UIDs are stores)
//
// P3D_USE_ASSERTS - Failures of P3DASSERT and P3DASSERTMGS result in appropriate erro messages (if nde define 
//       they are compiled out).P3D_DEBUG_MESSAGES must be defined as well.
//
// P3D_DEBUG_MESSAGES - Debug text messages (p3d::print, p3d::printf, tDebug) work. 
//
// P3D_FILL_MEMHEAP - tMemHeap memory is filled with a pattern 

// Values are stored in seperate headers to minimize rebuilds

// Debug build, full debugging support, may be quite slow
#ifdef RAD_DEBUG
#include<p3d/buildconfig_debug.hpp>
#else

// Tune Build configuration, some debugging, high speed 
#ifdef RAD_TUNE
#include<p3d/buildconfig_tune.hpp>
#else

// Release build config, no debugging at all, maximum speed
#ifdef RAD_RELEASE
#include<p3d/buildconfig_release.hpp>
#endif

#endif
#endif

#endif
