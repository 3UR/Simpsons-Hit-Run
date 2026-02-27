//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef RADSTACKTRACE_HPP
#define RADSTACKTRACE_HPP

#include <cstdint>

//=============================================================================
// ::radStackTraceGet
//=============================================================================

//
// Call this function to return an array of addresses which represent the
// current function call stack.
//

extern "C" void radStackTraceGet( uintptr_t * results, int numResults );


//=============================================================================
// ::radStackTraceWin32Get
//=============================================================================

//
// This stack trace function allows the caller to specify the base pointer (EBP)
// of another context
//

#if defined RAD_WIN32 || RAD_UWP

extern "C" void radStackTraceWin32Get( uintptr_t * results, 
    int numResults, const void * basePointer );

#endif // RAD_WIN32 || RAD_UWP

#endif RADSTACKTRACE_HPP
