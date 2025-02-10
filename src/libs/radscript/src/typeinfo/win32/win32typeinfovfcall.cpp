//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
//
// (C) 2001 Radical Entertainment
//
// Author: Tim Hinds
// 
// Win32 Intel 386+ only

#pragma warning ( disable : 4035 ) // no return value

int InvokeVf( void * pThat, unsigned int vtbloffset, void * pParams, int numParams )
{
#if defined(WIN32) && !defined(_WIN64)
    __asm
    {

            mov  ebx, vtbloffset            ; put method number in ebx
            shl  ebx, 2                     ; multiply by 4 to get offset into vtable

            mov ecx, numParams             ; if there are no params to push just go ahead and call
            //cmp ecx, 0                     ;
            //jz callVirtualFunction
    
            
            shl ecx, 2                     ; num bytes to move is ecx( numParams ) * 4
            sub esp, ecx                   ; move the stack pointer back this many bytes
            shr ecx, 2                     ; now shift back to copy doublewords
                                 
            mov edi, esp                   ; start copying at the new stack pointer
            mov esi, pParams               ; ...and the parameter array ( for ecx bytes )
            cld                            ; move left to right in memory              

            rep movsd                      ; execute
          
        //callVirtualFunction:              

            mov  eax, dword ptr [pThat]        ; move address off vtbl pointer to eax
            mov  ecx, eax                      ; move this pointer to ecx ( calling convention )
            mov  edx, dword ptr [eax]          ; move address of vtbl to edx
            add  edx, ebx                      ; add the offset into vtable (was multiplied by 4 above )

            call dword ptr [edx]               ; ok, do it, return ( if any ) will be in eax.
    }
#else
    // TODO(3ur): how hard would this be?
    #pragma message("NOTE: InvokeVf is unsupported on x64")
    return 0;
#endif
}