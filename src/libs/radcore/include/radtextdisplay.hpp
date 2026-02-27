//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radtextdisplay,hpp
//
// Subsystem:	Radical Debug Text Output Window
//
// Description:	This file contains all definitions and interfaces
//				required to use the Radical Text Display interface. The object
//              prints ASCII strings on the game console screen for the purpose
//              of debugging on non-devkit units. It does not depend on Pure3D.
//
//              Note that the text display is a singleton but multiple handles
//              to it can be obtained. It is not thread-safe.
//
// Revisions:	V1.00	Nov 27, 2001	    Creation
//
//=============================================================================

#ifndef RADTEXTDISPLAY_HPP
#define RADTEXTDISPLAY_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radobject.hpp>
#include <radmemory.hpp>

//=============================================================================
// Forward Declarations
//=============================================================================

struct IRadTextDisplay;

//=============================================================================
// Macros
//=============================================================================


//=============================================================================
// Functions
//=============================================================================

//
// Use this global function to initialize the debug output system and obtain
// a handle on it. The output screen is a singleton.
//
void radTextDisplayGet( IRadTextDisplay** pDisplay, 
                        radMemoryAllocator alloc = RADMEMORY_ALLOC_DEFAULT );


//=============================================================================
// Interfaces
//=============================================================================

//
// This is the main iterface used to print debug text on the TV screen.
//
struct IRadTextDisplay : public IRefCount
{
    virtual void SetAutoSwap( bool on ) = 0;
    virtual void SwapBuffers( void ) = 0;

    virtual void SetBackgroundColor( unsigned int color ) = 0;  
	virtual void SetTextColor( unsigned int color ) = 0;
    virtual void Clear( void ) = 0;

    virtual void GetDimensions( unsigned int* pWidth, unsigned int* pHeight ) const = 0;
    virtual void SetCursorPosition( unsigned int x, unsigned int y ) = 0;

    virtual void TextOutAt( const char* pText, int x, int y ) = 0;
    virtual void TextOut( const char*  pText ) = 0;
};

#endif 