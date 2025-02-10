//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include <stdlib.h>
#include <radstacktrace.hpp>

//
// Win32 Implementation
//

#define prev_fp_from_fp(fp)             (*((uintptr_t *)fp))
#define ret_addr_from_fp(fp)            (*((uintptr_t *)(fp + 4)))

//=============================================================================
// ::radStackTraceWin32Get
//=============================================================================

extern "C" void radStackTraceWin32Get( uintptr_t * results, int max, const void * basePointer )
{
  uintptr_t prevfp;
  uintptr_t curfp;
  uintptr_t last;
	
  prevfp = last = 0x00000000;

  //_asm
  //{
  //  mov   curfp, ebp
  //}

  curfp = ( uintptr_t ) basePointer;

  while (max--)
  {
    if (curfp != 0 &&     // is this frame pointer not NULL
        curfp > prevfp &&    // this frame pointer has to be greater than the previous
        !(curfp & 0x3))      // has to be 4byte aligned
    {
	    (*results++) = last = ret_addr_from_fp( curfp );
		  
      prevfp = curfp;
      curfp = prev_fp_from_fp( curfp );
    }
    else
    {
      (*results++) = last;

      curfp = 0;
    }
  }
}


//=============================================================================
// ::radStackTraceGet
//=============================================================================

extern "C" void radStackTraceGet( uintptr_t *results, int max )
{
    radStackTraceWin32Get( results, max, ( void * )( (uintptr_t)&results - 8 ) );
}
