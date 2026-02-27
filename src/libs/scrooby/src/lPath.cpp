//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime
//
// Description: This contains the implementation of the lpath object
//
// Authors:     Wilkin Ng, Ian Gipson
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================
#include "lPath.h"
#include <string.h>

//===========================================================================
// Local Constants, Typedefs, and Macros
//===========================================================================

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================

//===========================================================================
// Member Functions
//===========================================================================

//===========================================================================
// lPath::lPath
//===========================================================================
// Description: constructor
//
// Constraints:    None
//
// Parameters:    full_path - the string representing the full path
//
// Return:      None
//
//===========================================================================
lPath::lPath(const char* full_path) 
{
//    _splitpath(full_path,mDrive,mDir,mFname,mExt);
    strcpy( mFullPath, full_path );

    //build the path without the filename
    strcpy( mFullDir, full_path );
    int i = strlen( mFullDir ) - 1;
    for( ; i >= 0; i-- )
    {
        if( ( mFullDir[ i ] != '/' ) && ( mFullDir[ i ] != '\\' ) )
        {
            mFullDir[ i ] = NULL;
        }
        else
        {
            break;
        }
    }

    int lastSlashCharacter = i;

    //build the file name
    //IAN IMPROVE: this would be better to use a strcpy
    int filenameSize = strlen( full_path ) - lastSlashCharacter - 1;
    for( i = 0; i < filenameSize; i++ )
    {
        char addme = full_path[ i + lastSlashCharacter + 1 ];
        if( addme != '.' )
        {
            mFname[ i ] = full_path[ i + lastSlashCharacter + 1 ];
        }
        else
        {
            break;
        }
    }
    mFname[ i ] = NULL;

    int lastDotCharacter = i + lastSlashCharacter + 1;

    //build the extension name
    //IAN IMPROVE: this would be better to use a strcpy
    int extensionSize = strlen( full_path ) - lastDotCharacter - 1;
    for( i = 0; i < extensionSize; i++ )
    {
        char addme = full_path[ i + lastDotCharacter + 1 ];
        mExt[ i ] = full_path[ i + lastDotCharacter + 1 ];
    }
    mExt[ i ] = NULL;

//    strcat(mFullDir,mDir);
}


lPath::~lPath()
{
}
