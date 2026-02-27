//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _UNICODE_HPP
#define _UNICODE_HPP

#include <p3d/p3dtypes.hpp>

namespace p3d
{
    inline P3D_UNICODE ConvertCharToUnicode(char a)
    {
        return( (P3D_UNICODE)(a) & 0x00FF ); // zero higher-order byte for ascii
    }

    int UnicodeStrLen (const P3D_UNICODE* str);
    P3D_UNICODE* UnicodeStrCpy (const P3D_UNICODE* src, P3D_UNICODE* dst, int bufferLen = -1);
    P3D_UNICODE* UnicodeStrUpr (P3D_UNICODE* str);
    int UnicodeToAscii (const P3D_UNICODE* unicode, char* ascii, int bufferLen);
    int AsciiToUnicode (const char* ascii, P3D_UNICODE* unicode, int bufferLen);
}

#endif

