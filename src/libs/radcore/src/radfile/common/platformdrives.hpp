//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        platformdrives.hpp
//
// Subsystem:	Radical File System
//
// Description:	This file contains the platform specific names of all drives.
//              It also contains a function to give back the name of the
//              default drive. This allows our other classes to be platform
//              independent.
//
// Revisions:	
//
//=============================================================================

#ifndef	PLATFORMDRIVES_HPP
#define PLATFORMDRIVES_HPP

#include <raddebug.hpp>
#include <radmemory.hpp>
#include <radfile.hpp>

class radDrive;

//=============================================================================
// Typedefs
//=============================================================================

#if defined( RAD_WIN32 ) || defined( RAD_UWP )
#include <fstream>
#include <filesystem>
typedef std::fstream* radFileHandle;
typedef std::filesystem::directory_iterator radFileDirHandle;
#endif

//=============================================================================
// Functions
//=============================================================================

//
// Default default drive
//
void PlatformDrivesGetDefaultDrive( char* driveSpec );

//
// Indicate whether we have a valid drive
//
bool PlatformDrivesValidateDriveName( const char* driveSpec );

//
// Get the drive object, name should be validated.
//
void PlatformDrivesFactory( class radDrive** ppDrive, const char* driveSpec, radMemoryAllocator alloc );

#endif // PLATFORMDRIVES_HPP
