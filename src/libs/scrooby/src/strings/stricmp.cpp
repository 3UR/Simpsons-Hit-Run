//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   string subsystem
//
// Description: This file contains the implementation of the class UnicodeString
//
// Authors:     Ian Gipson
//
// Revisions        Date        Author    Revision
//                    2000/10/23    IJG        Created
//
//===========================================================================

//---------------------------------------------------------------------------
// Includes
//===========================================================================

#include <ctype.h>
#include "strings/stricmp.h"
#include <stdlib.h>

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
// stricmp
//===========================================================================
// Description: stricmp - case independant string comparison
//
// Constraints:    none
//
// Parameters:    string1, string2 - strings to be compared
//
// Return:      int - <0 string1 < string2
//                    =0 string1 = string2
//                    >0 string1 > string2
//
//===========================================================================
int rstricmp( const char* string1, const char* string2 )
{
    //IMPROVE: if this is slow, then it can be swapped for the built in stricmp
    const char* index1 = string1;
    const char* index2 = string2;

    while( ( *index1 != NULL ) && ( *index2 != NULL ) )
    {
        char c1 = toupper( *index1 );
        char c2 = toupper( *index2 );
        if( c1 > c2 )
        {
            return 1;
        }
        if( c1 < c2 )
        {
            return -1;
        }
        index1++;
        index2++;
    }
    if( (*index1 == NULL) && (*index2 != NULL) )
    {
        return -1;
    }
    else if( (*index1 != NULL) && (*index2 == NULL) )
    {
        return 1;
    }
    return 0;
}

//===========================================================================
// rstrincmp
//===========================================================================
// Description: stricmp - case independant string comparison with a character
//                limit.
//
// Constraints:    none
//
// Parameters:    string1, string2 - strings to be compared
//              tCount - how many characters to compare
//
// Return:      int - <0 string1 < string2
//                    =0 string1 = string2
//                    >0 string1 > string2
//
//===========================================================================
int rstrincmp( const char *string1, const char *string2, int tCount)
{
    //IMPROVE: if this is slow, then it can be swapped for the built in stricmp
    const char* index1 = string1;
    const char* index2 = string2;

    while( ( *index1 != NULL ) && ( *index2 != NULL ) && ( tCount != 0 ) )
    {
        char c1 = toupper( *index1 );
        char c2 = toupper( *index2 );
        if( c1 > c2 )
        {
            return 1;
        }
        if( c1 < c2 )
        {
            return -1;
        }
        index1++;
        index2++;
        tCount--;
    }
    if( (*index1 == NULL) && (*index2 != NULL) )
    {
        return -1;
    }
    else if( (*index1 != NULL) && (*index2 == NULL) )
    {
        return 1;
    }
    return 0;
};
