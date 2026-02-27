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

//===========================================================================
// Includes
//===========================================================================

#include <assert.h>
#include <direct.h>
#include "StreamReader.h"

//===========================================================================
// Local Constants, Typedefs, and Macros
//===========================================================================

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================

//===========================================================================
// Member Functions
//===========================================================================

//===========================================================================
// StreamReader::StreamReader
//===========================================================================
// Description: StreamReader's constructor
//
// Constraints:    None
//
// Parameters:    filename - the name of the file
//              bufferSize - the number of bytes to read from file at a time
//
// Return:      Nothing
//
//===========================================================================
StreamReader::StreamReader( const char* filename, const int bufferSize )
{
    m_pos = 0;
    m_charsRead = 0;
    m_bufferSize = bufferSize;

    m_buffer = new char[ m_bufferSize ];
    bool result = OpenFile( filename );
    if( result == false )
    {
        char currentDir[ 256 ] = "";
        ::_getcwd( currentDir, 256 );
        assert( result == true );
    }
}

//===========================================================================
// StreamReader::~StreamReader
//===========================================================================
// Description: StreamReader's destructor
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      Nothing
//
//===========================================================================
StreamReader::~StreamReader()
{
    delete [] m_buffer;

    CloseFile();
}
        
//===========================================================================
// StreamReader::GetNextChar
//===========================================================================
// Description: Returns next available character
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      char - the next char - '\0' if nothing
//
//===========================================================================
char StreamReader::GetNextChar()
{
    if( m_fileError )
    {
        return '\0';
    }
    else if( m_pos >= m_charsRead )
    {
        // Finished with the current buffer, let's read more from the file
        m_charsRead = GetNextBufferFromFile();
        m_pos = 1;
        
        return m_buffer[ 0 ];
    }
    else
    {
        return m_buffer[ m_pos++ ];
    }
}

//===========================================================================
// StreamReader::OpenFile
//===========================================================================
// Description: Opens the file for read
//
// Constraints:    None
//
// Parameters:    filename - the name of the file
//
// Return:      bool - whether the open was successful
//
//===========================================================================
bool StreamReader::OpenFile( const char* filename )
{
    m_fileHandle = ::CreateFile
    (
        filename,              // the path
        GENERIC_READ,          // open for reading
        FILE_SHARE_READ,       // prevent sharing
        0,                     // security is ignored in Win95
        OPEN_EXISTING,         // opens an existing file, creates a new file
        FILE_ATTRIBUTE_NORMAL, // normal file attributes
        0                      // no file to copy attributes from
    );
    
    if( m_fileHandle == INVALID_HANDLE_VALUE )
    {
        m_fileError = true;
        return( false );
    }

    m_fileError = false;
    return true;
}

//===========================================================================
// TextFileWriter::CloseFile
//===========================================================================
// Description: Closes file
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
void StreamReader::CloseFile()
{
    CloseHandle( m_fileHandle );
}

//===========================================================================
// StreamReader::GetNextBufferFromFile
//===========================================================================
// Description: Reads next buffer from file
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      int - number of bytes read
//
//===========================================================================
int StreamReader::GetNextBufferFromFile()
{
    // Variables to hold the number of requested bytes and the actual bytes
    // that were read from the file.
    DWORD dwBytesRead;
    DWORD dwBytesToRead = m_bufferSize;

    ::ReadFile
    ( 
        m_fileHandle,        // handle of file to read 
        m_buffer,            // address of buffer that receives data  
        dwBytesToRead,       // number of bytes to read 
        &dwBytesRead,        // address of number of bytes read 
        0                    // address of structure for data (must be non-zero if
                             // m_hfile is created with FILE_FLAG_OVERLAPPED set.
    );
    
    m_charsRead = dwBytesRead;

    // Was there nothing to read?
    m_fileError = ( m_charsRead == 0 ); 
    return m_charsRead;
}
