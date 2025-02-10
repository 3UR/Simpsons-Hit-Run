/** \file radloadconfig.hpp
  *
  *  Sets the compile time options for radload.
  *
  *  \author Mike Perzel
  *  \attention Copyright (C) 2002 Radical Entertainment Ltd.  All Rights Reserved.
  */

#ifndef RADLOAD_CONFIG_HPP
#define RADLOAD_CONFIG_HPP

//@{
/// Define the memory allocators that radLoad will use internally
#define RADLOAD_ALLOC_DEFAULT RADMEMORY_ALLOC_DEFAULT
#define RADLOAD_ALLOC_TEMP RADMEMORY_ALLOC_TEMP 
//@}

/// Do checking on AddRef's and Releases on objects to make sure they are valid
#ifdef RAD_DEBUG
#ifndef TOOLS
#ifndef RAD_PS2  // SIMPSONS: PS2 debug is already stinky slow, this isn't helping
//#define RADLOAD_HEAP_DEBUGGING
//#define RADLOAD_HEAP_DEBUGGING_PARANOID
#endif
#endif
#endif

/// Allow the collection of load stats. 
#define RADLOAD_GATHER_STATS

///  Allow radLoadHashStore collisions tests
//#define RADLOAD_COLLISION_TESTS

// Use the radcore debugging tools

/// Enable the memory monitor functions.  Also requires that the memory monitor be
/// enabled in radcore
//#define RADLOAD_USE_RADMEMORYMONITOR

/// Use the watcher to keep track of the gathered runtime stats.  Requires RADLOAD_GATHER_STATS
//#define RADLOAD_USE_WATCHER


#endif

