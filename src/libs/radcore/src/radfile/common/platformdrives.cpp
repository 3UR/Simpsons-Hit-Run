//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include <string.h>
#include <radstring.hpp>
#include "platformdrives.hpp"
#include "remotedrive.hpp"

#if defined(RAD_WIN32) || defined(RAD_UWP)
#include "../win32/win32drive.hpp"
#endif

//=============================================================================
// Function:    PlatformDrivesGetDefaultDrive
//=============================================================================
// Description: Copy over the name of the default drive.
//
// Parameters:  char* string
//
// Returns:   
//
//------------------------------------------------------------------------------
void PlatformDrivesGetDefaultDrive( char* driveSpec )
{
//
// Get the current drive
//
#if defined(RAD_WIN32) || defined(RAD_UWP)
    char bigDir[ radFileFilenameMax + 1 ];
    ::GetCurrentDirectory( radFileFilenameMax, bigDir );
    strncpy( driveSpec, bigDir, 2 );
    driveSpec[ 2 ] = '\0';
    strupr( driveSpec );
#endif // RAD_WIN32 || RAD_UWP
}

//=============================================================================
// Function:    PlatformDrivesValidateDriveName
//=============================================================================

bool PlatformDrivesValidateDriveName( const char* driveSpec )
{
    if ( strcmp( driveSpec, s_RemoteDriveName ) == 0 )
    {
        return true;
    }

#if defined(RAD_WIN32) || defined(RAD_UWP)
    if( (strlen( driveSpec ) == 2) && (*driveSpec >= 'A') && (*driveSpec <= 'Z') )
    {
        unsigned int index = *driveSpec - 'A';
        DWORD drives = GetLogicalDrives( );
        drives >>= index;
        return ( drives & 1 );
    }
    else
    {
        return false;
    }

#endif // RAD_WIN32 || RAD_UWP
}

//=============================================================================
// Function:    PlatformDrivesFactory
//=============================================================================

void PlatformDrivesFactory( radDrive** ppDrive, const char* driveSpec, radMemoryAllocator alloc )
{
    rAssert( ppDrive != NULL );
    rAssert( driveSpec != NULL );

    if ( strcmp( driveSpec, s_RemoteDriveName ) == 0 )
    {
        radRemoteDriveFactory( ppDrive, driveSpec, alloc );
        return;
    }

#if defined(RAD_WIN32) || defined(RAD_UWP)
    radWin32DriveFactory( ppDrive, driveSpec, alloc );
#endif
}
