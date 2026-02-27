//===========================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Radical Text Display Console.
//
// Description: This file implements a text display on the console TV screen.
//
// Authors:     Soleil Lapierre
//
//===========================================================================

// Recompilation protection flag.
#ifndef __TEXTDISPLAY_H
#define __TEXTDISPLAY_H

//===========================================================================
// Nested Includes
//===========================================================================

#include <radmemory.hpp>
#include <raddebug.hpp>

//===========================================================================
// Class Definitions
//===========================================================================

//===========================================================================
//
// Description: This class implements the IRadTextDisplay interface.
//
//===========================================================================
class radTextDisplay : public IRadTextDisplay
{
    public:

    //
    // Setup and tear-down functions.
    //
    void Initialize( radMemoryAllocator alloc );
    void Terminate( void );

    //
    // IRefCount stuff
    //
    virtual void AddRef( void );
    virtual void Release( void );

    //
    // IRadTextDisplay stuff.
    //

    virtual void SetAutoSwap( bool on );
    virtual void SwapBuffers( void );

    virtual void SetBackgroundColor( unsigned int color );  
	virtual void SetTextColor( unsigned int color );
    virtual void Clear( void );

    virtual void GetDimensions( unsigned int* pWidth, unsigned int* pHeight ) const;
    virtual void SetCursorPosition( unsigned int x, unsigned int y );

    virtual void TextOutAt( const char* pText, int x, int y );
    virtual void TextOut( const char*  pText );

    //
    // Initialization flag.
    //
    static bool m_Initialized;

    //
    // Allocator for internal memory buffers.
    //
    static radMemoryAllocator m_Alloc;

    private:

    //
    // Store a character in the text buffer.
    //
    void WriteLetter( int x, int y, char c );

    //
    // Scroll the screen up one line.
    //
    void ScrollUp( void );

    //
    // Init/shut down display hardware.
    //
    void InitDisplay( void );
    void CloseDisplay( void );

    //
    // Redraw and redisplay video screen.
    //
    void PaintIfAutoSwapOn( void );
    void Paint( void );

    //
    // ===========================================
    // Data members.
    //

    //
    // Reference counting member variable.
    //
    int m_ReferenceCount;


    bool m_AutoSwap;

    //
    // Buffer for storing the displayed text.
    //
    char* m_TextBuffer;

    //
    // Display controls.
    //
    unsigned int m_BackgroundColorRGB; // Background color.
    unsigned int m_TextColorRGB;       // Text color.
    int          m_CursorX;   // Cursor location, 0-left-0-top-based.
    int          m_CursorY;
    int          m_Width;     // Screen width in characters.
    int          m_Height;    // Screen height in characters.

    //
    // Current redraw frame number (controls selection of frame buffer.
    //
    int m_CurFrame;
};

#endif // file