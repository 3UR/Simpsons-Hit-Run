//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _ERROR_HPP
#define _ERROR_HPP
// Pure3D error logging and debugging support

#include <p3d/buildconfig.hpp>
#include <p3d/p3dtypes.hpp>


#include <raddebug.hpp>

class tDebug
{
public:
    static void CapturePDDIMessages(pddiDevice*);
};


#ifdef P3D_USE_ASSERTS
    #define P3DASSERT(c) rTuneAssert(c)
    #define P3DASSERTMSG(c, msg, mod) rTuneAssertMsg(c, const_cast<char*>(msg))
#else
    #define P3DASSERT(c)
    #define P3DASSERTMSG(c, msg, mod)
#endif //P3D_USE_ASSERTS

/*---------------------------------------------------------------------------
    Functions (always are compiled in):
---------------------------------------------------------------------------*/
// P3DVERIFY() - this function works like P3DASSERTMSG, but it never is compiled
// out on the PC.
void P3DVERIFY(int cond, const char *msg1, 
                         const char *msg2=NULL, 
                         const char *msg3=NULL, 
                         const char *msg4=NULL, 
                         const char *msg5=NULL);

#endif//_ERROR_HPP

