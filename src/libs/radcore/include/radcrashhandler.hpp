//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radcrashhandler.hpp
//
// Subsystem:   Foundation Technologies - Crash Handler
//
// Description: This file contains all the definitions and interfaces of the
//              FTech crash handler.
//
// Notes:       Once installed, the FTech crash handler will handle exceptions
//              and provide debug information to help determine the cause of 
//              the exception.
//
//              The radCrashHandler requires that a memory card be installed
//              in the appriate slot (usually the first slot).
//
//              If possible radCrashHandler will record the details of a crash
//              on the memory card.  The details will be recorded in a ".crh"
//              file.
//
//              Use the radCrashReportExtractor tool to copy this file from 
//              the memory card to your host machine or in the case of the xbox
//              use the XBox Explorer.
//
//              Use the radCrashReportWriter tool to generate a text file that
//              translates and summarizes the crash information contained in 
//              the '.crh' file into a text report.
//
// Revisions:   April 4, 2002        RWS
//
//=============================================================================

#ifndef RADCRASHHANDLER_HPP
#define RADCRASHHANDLER_HPP

//=============================================================================
// Included Files
//=============================================================================

#include <radmemory.hpp>

//=============================================================================
// Functions
//=============================================================================

//
// These functions install and remove the crash handler
//

void radCrashHandlerInitialize( radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );
void radCrashHandlerTerminate( void );

#endif // RADCRASHHANDLER_HPP

