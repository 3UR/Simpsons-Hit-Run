//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Stream reader 
//
// Description: This contains the file reader class that reads chunks of file
//              and return to program one char at a time
//
// Authors:     Eric Tong
//
//===========================================================================

// Recompilation protection flag.
#ifndef _STREAMREADER_H_
#define _STREAMREADER_H_

//===========================================================================
// Nested Includes
//===========================================================================

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

//===========================================================================
// Forward References
//===========================================================================

//===========================================================================
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//===========================================================================

//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: This class is used to read a file and return one char at a time
//
// Constraints: None
//
//===========================================================================

class StreamReader
{
    public:
        StreamReader( const char* filename, const int bufferSize = 1024 );
        virtual ~StreamReader();
        
        char GetNextChar();

    private:
        bool OpenFile( const char* filename );
        void CloseFile();

        int GetNextBufferFromFile();

        int m_pos;
        int m_charsRead;
        int m_bufferSize;
        char* m_buffer;

        bool m_fileError;
        
        HANDLE m_fileHandle;
};

#endif                                  // End conditional inclusion
