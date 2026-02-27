//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/unicode.hpp>
#include <stdlib.h>
#include <string.h>

namespace p3d
{

int UnicodeStrLen (const P3D_UNICODE* str)
{
    if (!str)
    {
        return 0;
    }

    int len=0;
    while (str[len])
    {
        len++;
    }
    return len;
}

P3D_UNICODE* UnicodeStrCpy (const P3D_UNICODE* src, P3D_UNICODE* dst, int bufferLen)
{
    if( src != NULL && dst != NULL )
    {
        // bufferLen < 0 --> copy src to dst until end of src
        // bufferLen > 0 --> copy bufferLen characters from src to dst, unless end of src reached
        //                   before bufferLen characters
        //
        for( int i = 0; i < bufferLen || bufferLen < 0; i++ )
        {
            // copy the character
            dst[ i ] = src[ i ];

            if( src[ i ] == '\0' )
            {
                // end or src reached
                break;
            }
        }
    }

    // return copied string
    return dst;
}

P3D_UNICODE* UnicodeStrUpr( P3D_UNICODE* str )
{
    // NOTE: this should only be used for unicode strings that are
    //       really just ASCII-equivalents

    if( str != NULL )
    {
        // convert all (ascii) characters in unicode string to uppercase
        //
        const P3D_UNICODE UPPERCASE_OFFSET = 'A' - 'a';

        for( int i = 0; str[ i ] != '\0'; i++ )
        {
            if( str[ i ] >= 'a' && str[ i ] <= 'z' )
            {
                str[ i ] += UPPERCASE_OFFSET;
            }
        }
    }

    return str; // return the converted unicode string
}

int UnicodeToAscii (const P3D_UNICODE* unicode, char* ascii, int bufferLen)
{
    if (!unicode || !ascii)
    {
        return 0;
    }

    int i=0;
    while ((unicode[i]) && (i<(bufferLen-1)))
    {
        ascii[i]=(char)(unicode[i]);
        i++;
    }
    ascii[i]=0;
    return i;
}

int AsciiToUnicode (const char* ascii, P3D_UNICODE* unicode, int bufferLen)
{
    if (!unicode || !ascii)
    {
        return 0;
    }

    int i=0;
    while ((ascii[i]) && (i<(bufferLen-1)))
    {
        unicode[i]=ConvertCharToUnicode(ascii[i]);
        i++;
    }
    unicode[i]=0;
    return i;
}

}

