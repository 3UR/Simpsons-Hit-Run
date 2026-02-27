//===========================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Radical Text Display Component.
//
// Description: This file implements a simple text display on the console
//              TV screen. This is useful for debugging on comsumer units.
//
// Authors:     Soleil Lapierre
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================

#include "pch.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <radtime.hpp>
#include <radtextdisplay.hpp>     // interface specification
#include <radobject.hpp>

#include "textdisplay.hpp"        // class specification


//===========================================================================
// Globals
//===========================================================================

//
// The display singleton
//
static radTextDisplay s_theDisplay;

bool radTextDisplay::m_Initialized = false;
radMemoryAllocator radTextDisplay::m_Alloc = RADMEMORY_ALLOC_DEFAULT;


//===========================================================================
// radTextDisplayGet
//===========================================================================
// Description: Return an interface handle on the text display object.
//              This addrefs the object as well. If the display has not been
//              initialized yet, that is done here.
//
// Constraints:	The caller is responsible for calling Release on the returned
//              object when finished with it.
//
// Parameters:	pNewWatch - place to store pointer to new object.
//              timeUnit - time unit for the new watch to use.
//
//===========================================================================

void radTextDisplayGet
( 
    IRadTextDisplay** pDisplay,
    radMemoryAllocator alloc
)
{
    rAssert( pDisplay != NULL );

    if( !s_theDisplay.m_Initialized )
    {
        s_theDisplay.Initialize( alloc );
    }

    IRadTextDisplay* display = &s_theDisplay;
    *pDisplay = display;
    radAddRef( *pDisplay, NULL );
}



//=============================================================================
// Function:    radTextDisplay::Initialize
//=============================================================================
// Description: Allocates a frame buffer, initializes the display using 
//              default colors, clears the screen etc.
//
// Constraints: Generally should only be called once at program startup.
//
// Parameters:  alloc - memory allocator to use internally.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::Initialize( radMemoryAllocator alloc )
{
    rAssertMsg( !m_Initialized, "radTextDisplay already initialized." );

    //
    // Initialize easy stuff.
    //
    m_AutoSwap = true;
    m_Alloc = alloc;
    m_TextBuffer = NULL;
    m_BackgroundColorRGB = 0x000000;
    m_TextColorRGB = 0xffffff;
    m_CursorX = 0;
    m_CursorY = 0;
    m_CurFrame = 0;
    m_Width = 80;
    m_Height = 25;

    //
    // Set up the frame buffer.
    //
    m_TextBuffer = ( char* ) radMemoryAlloc( m_Alloc, sizeof( char ) * m_Width * m_Height );
    rAssert( m_TextBuffer != NULL );

    //
    // Clear and paint the display.
    //
    InitDisplay( );
    m_Initialized = true;
    Clear( );

    //
    // If this platform is unsupported, print a warning.
    //
    rDebugString( "WARNING: radTextDisplay is not supported on this platform.\n" );
    rDebugString( "         Screen output will be redirected to the debug channel.\n" );
}

   
//=============================================================================
// Function:    radTextDisplay::Terminate
//=============================================================================
// Description: Frees memory used by the text display and shuts down the video
//              output system.
//
// Constraints: Generally should only be called once, at program shutdown.
//
// Parameters:  None.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::Terminate( void )
{
    rAssertMsg( m_Initialized, "radTextDisplay already terminated." );

    //
    // Do whatever is needed to shut down the hardware.
    //
    CloseDisplay( );

    //
    // Free the frame buffers
    //
    if( m_TextBuffer != NULL )
    {
        radMemoryFree( m_Alloc, m_TextBuffer );
        m_TextBuffer = NULL;
    }

    //
    // Close down other state stuff.
    //
    m_Initialized = false;
}

//=============================================================================
// Function:    radTextDisplay::SetAutoSwap
//=============================================================================

void radTextDisplay::SetAutoSwap( bool on )
{
    m_AutoSwap = on;
}

//=============================================================================
// Function:    radTextDisplay::SwapBuffers
//=============================================================================

void radTextDisplay::SwapBuffers( void )
{
    Paint( );
}

//=============================================================================
// Function:    radTextDisplay::AddRef
//=============================================================================
// Description: Increment the reference count of this object.
//
// Parameters:  None.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::AddRef( void )
{
    m_ReferenceCount++;
}


//=============================================================================
// Function:    radTextDisplay::Release
//=============================================================================
// Description: Decrement reference count of this object.
//
// Parameters:  None.
//              
// Returns:     None.
//
// Notes:       If the reference count reaches zero, the display system
//              is automatically shut down.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::Release( void )
{
    m_ReferenceCount--;

    if( m_ReferenceCount < 1 )
    {
        Terminate( );
    }
}


//=============================================================================
// Function:    radTextDisplay::SetBackgroundColor
//=============================================================================
// Description: Change the background color of the display.
//
// Parameters:  color - the 24-bit RGB color to use.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::SetBackgroundColor( unsigned int color )
{
    m_BackgroundColorRGB = color;
    PaintIfAutoSwapOn( );
}


//=============================================================================
// Function:    radTextDisplay::SetTextColor
//=============================================================================
// Description: Change the text color of the display.
//
// Parameters:  color - the 24-bit RGB color to use.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::SetTextColor( unsigned int color )
{
    m_TextColorRGB = color;
    PaintIfAutoSwapOn( );
}


//=============================================================================
// Function:    radTextDisplay::Clear
//=============================================================================
// Description: Clear all text from the display and home the cursor.
//
// Parameters:  None.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::Clear( void )
{
    rAssertMsg( m_Initialized, "radTextDisplay not initialized!" );

    //
    // Clear the text buffer.
    //
    for( int i = 0; i < m_Width * m_Height; i++ )
    {
        m_TextBuffer[ i ] = '\0';
    }

    //
    // Home the cursor.
    //
    SetCursorPosition( 0, 0 );

    //
    // Redraw the screen.
    //
    PaintIfAutoSwapOn( );
}


//=============================================================================
// Function:    radTextDisplay::GetDimensions
//=============================================================================
// Description: Get the width and height of the usable text display area.
//
// Parameters:  pWidth - place to store width
//              pHeight - place to store height
//              
// Returns:     None.
//
// Notes:       Dimensions are in character units, not pixels.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::GetDimensions( unsigned int* pWidth, unsigned int* pHeight ) const
{
    rAssertMsg( m_Initialized, "radTextDisplay not initialized!" );

    if( pWidth != NULL )
    {
        *pWidth = ( unsigned int ) m_Width;
    }

    if( pHeight != NULL )
    {
        *pHeight = ( unsigned int ) m_Height;
    }
}


//=============================================================================
// Function:    radTextDisplay::SetCursorPosition
//=============================================================================
// Description: Change the cursor position for the next text update.
//
// Constraints: The position will be clamped to legal coordinates.
//
// Parameters:  x, y = new cursor position in character units.
//              
// Returns:     None.
//
// Notes:       (0,0) is the upper left corner. X increases right, Y down.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::SetCursorPosition( unsigned int x, unsigned int y )
{
    rAssertMsg( m_Initialized, "radTextDisplay not initialized!" );

    if( ( int ) x < 0 )
    {
        m_CursorX = 0;
    }
    else if( ( int ) x >= m_Width )
    {
        m_CursorX = m_Width - 1;
    }
    else
    {
        m_CursorX = ( int ) x;
    }

    if( ( int ) y < 0 )
    {
        m_CursorY = 0;
    }
    else if( ( int ) y >= m_Height )
    {
        m_CursorY = m_Height - 1;
    }
    else
    {
        m_CursorY = ( int ) y;
    }
}


//=============================================================================
// Function:    radTextDisplay::TextOut
//=============================================================================
// Description: Prints a string on the display starting at the current cursor
//              position. Long lines will be wrapped at the screen width, and
//              the screen will scroll if the bottom is passed. The cursor
//              position is updated to the end of the string.
//
// Parameters:  pText - string to print.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::TextOut( const char*  pText )
{
    rAssertMsg( m_Initialized, "radTextDisplay not initialized!" );
    rAssert( pText != NULL );

    //
    // If this platform is unsupported, just echo the input to the debug channel.
    //
    rDebugString( pText );

    //
    // Loop over input characters.
    //
    char* ptr = ( char* )pText;
    while( *ptr != '\0' )
    {
        //
        // If it's a printable character, just output it.
        //
        if( isprint( *ptr ) )
        {
            WriteLetter( m_CursorX, m_CursorY, *ptr );
            m_CursorX++;
        }
        else
        {
            //
            // Otherwise, handle special control characters.
            //
            char c = *ptr;
            switch( c )
            {
                case 8:     // backspace
                {
                    if( m_CursorX > 0 )
                    {
                        m_CursorX--;
                    }
                    break;
                }
                case 9:     // tab
                {
                    m_CursorX += 4;
                    break;
                }
                case 10:    // linefeed
                {
                    m_CursorY++;
                    m_CursorX = 0;
                    break;
                }
                case 13:    // carriage return
                {
                    m_CursorY++;
                    m_CursorX = 0;
                    break;
                }
                case 127:   // backspace & delete
                {
                    if( m_CursorX > 0 )
                    {
                        m_CursorX--;
                        WriteLetter( m_CursorX, m_CursorY, 0 );
                    }
                    break;
                }
                default:    // anything else
                {
                    WriteLetter( m_CursorX, m_CursorY, c );
                    m_CursorX++;
                    break;
                }
            }
        }

        //
        // Handle line wrap.
        //
        while( m_CursorX >= m_Width )
        {
            m_CursorX -= m_Width;
            m_CursorY++;
        }

        //
        // Handle scrolling.
        //
        while( m_CursorY >= m_Height )
        {
            ScrollUp( );
        }

        ptr++;
    }

    //
    // Done with input - repaint the screen.
    //
    PaintIfAutoSwapOn( );
}


//=============================================================================
// Function:    radTextDisplay::TextOutAt
//=============================================================================
// Description: Prints a string on the display at a specified starting 
//              coordinate on the display. Wrapping and scrolling will still
//              occur as with TextOut, but the cursor position will not be
//              updated.
//
// Parameters:  pText - string to print.
//              x, y - location to print it at.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::TextOutAt( const char* pText, int x, int y )
{
    int cx = m_CursorX;
    int cy = m_CursorY;
    SetCursorPosition( x, y );
    TextOut( pText );
    SetCursorPosition( cx, cy );
}


//=============================================================================
// Private member functions
//=============================================================================

//=============================================================================
// Function:    radTextDisplay::WriteLetter
//=============================================================================
// Description: Stores a character in the memory map of the text on screen.
//              Does not actually update the visible display.
//
// Parameters:  x, y - location to print it at.
//              c - character to store.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::WriteLetter( int x, int y, char c )
{
    if( x < 0 || x >= m_Width )
    {
        return;
    }

    if( y < 0 || y >= m_Height )
    {
        return;
    }

    if( m_TextBuffer == NULL )
    {
        return;
    }

    m_TextBuffer[ x + y * m_Width ] = c;
}


//=============================================================================
// Function:    radTextDisplay::ScrollUp
//=============================================================================
// Description: Scrolls the text buffer up one line. Does not actually update
///             the visible display. Does update the cursor vertical position.
//
// Parameters:  None.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::ScrollUp( void )
{
    //
    // Cursor scrolls with text.
    //
    m_CursorY--;

    if( m_TextBuffer == NULL )
    {
        return;
    }

    //
    // Copy all lines up one.
    //
    for( int y = 0; y < m_Height - 1; y++ )
    {
        for( int x = 0; x < m_Width; x++ )
        {
            m_TextBuffer[ x + y * m_Width ] = m_TextBuffer[ x + ( y + 1 ) * m_Width ];
        }
    }

    //
    // Clear the bottom line.
    //
    for( int x = 0; x < m_Width; x++ )
    {
        m_TextBuffer[ x + ( m_Height - 1 ) * m_Width ] = '\0';
    }
}

//=============================================================================
// Function:    radTextDisplay::ReDrawScreenIfAutoPaintOn
//=============================================================================

void radTextDisplay::PaintIfAutoSwapOn( void )
{
    if ( m_AutoSwap == true )
    {
        Paint( );
    }
}

//=============================================================================
// Function:    radTextDisplay::ReDrawScreen
//=============================================================================
// Description: Repaints the entire text display with the current contents of
//              the text buffer.
//
// Parameters:  None.
//              
// Returns:     None
//
//------------------------------------------------------------------------------
    
void radTextDisplay::Paint( void )
{
}

//=============================================================================
// Function:    radTextDisplay::InitDisplay
//=============================================================================
// Description: Calls the appropriate lib functions to init display hardware.
//
// Parameters:  None.
//              
// Returns:     None
//
//------------------------------------------------------------------------------
void radTextDisplay::InitDisplay( void )
{
}

//=============================================================================
// Function:    radTextDisplay::CloseDisplay
//=============================================================================
// Description: Calls the appropriate lib functions to shut down display hardware.
//
// Parameters:  None.
//              
// Returns:     None
//
//------------------------------------------------------------------------------

void radTextDisplay::CloseDisplay( void )
{
}