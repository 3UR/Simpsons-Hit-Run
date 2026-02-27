//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   XMLSaver
//
// Description: This contains the XMLSaver class.
//
// Authors:     Eric Tong
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================

#include "XMLSaver.h"
#include <utility/memory.h>

//===========================================================================
// Local Constants, Typedefs, and Macros
//===========================================================================

const int XMLSaver::m_tabSize = 4;
const int XMLSaver::m_maxValueLength = 15;

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================

//===========================================================================
// Member Functions
//===========================================================================

//===========================================================================
// XMLSaver::XMLSaver
//===========================================================================
// Description: XMLSaver's constructor
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      Nothing
//
//===========================================================================
XMLSaver::XMLSaver()
{
    m_numTabs = 0;
    m_justPushed = false;
    m_justPopped = true;
}

//===========================================================================
// XMLSaver::~XMLSaver
//===========================================================================
// Description: XMLSaver's destructor
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      Nothing
//
//===========================================================================
XMLSaver::~XMLSaver()
{
}

//===========================================================================
// XMLSaver::OpenFile
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
bool XMLSaver::OpenFile( const char* filename )
{
    m_numTabs = 0;
    m_justPushed = false;
    m_justPopped = true;

    PascalCString testFileName = filename;

    m_hFile = ::CreateFile
    (
        testFileName,              // the path
        GENERIC_WRITE,         // open for writing
        0,                     // prevent sharing
        0,                     // security is ignored in Win95
        CREATE_ALWAYS,         // opens an existing file, creates a new file
        FILE_ATTRIBUTE_NORMAL, // normal file attributes
        0                      // no file to copy attributes from
    );
    
    if( m_hFile == INVALID_HANDLE_VALUE )
    {
        return( false );
    }

    Print( "<?xml version='1.0'?>" );
    return PrintNewLine();
}

//===========================================================================
// XMLSaver::CloseFile
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
bool XMLSaver::CloseFile()
{
    CloseHandle( m_hFile );

    return true;
}

//===========================================================================
// XMLSaver::PushElement
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
bool XMLSaver::PushElement( const char* name )
{
    bool returnValue;

    if( m_justPushed )
    {
        FlushAttributes();
        Print( ">" );
        PrintNewLine();
        m_numTabs++;
    }

    PrintTabs();
    
    Print( "<" );
    returnValue = Print( name );

    PascalCString* newString = new PascalCString( name );
    m_elementNames.push_back( newString );

    m_justPushed = true;
    m_justPopped = false;

    return returnValue;
}

//===========================================================================
// XMLSaver::PopElement
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
bool XMLSaver::PopElement()
{
    if( m_justPopped )
    {
        m_numTabs--;
        PrintTabs();
    }
    else
    {
        FlushAttributes();
        Print( ">" );
    }

    m_justPushed = false;
    m_justPopped = true;

    if( m_elementNames.size() > 0 )
    {
        PascalCString* elementName = m_elementNames.back();
        m_elementNames.pop_back();

        Print( "</" );
        Print( *elementName );
        Print( ">" );

        delete elementName;

        bool returnValue = PrintNewLine();

        return returnValue;
    }
    else
    {
        return false;
    }
}

//===========================================================================
// XMLSaver::AddAttribute
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
bool XMLSaver::AddAttribute( const char* name, const char* value )
{
    AttributeStruct* attribute = new AttributeStruct;
    attribute->m_name = name;
    attribute->m_value = value;

    m_attributes.push_back( attribute );

    return true;
}

bool XMLSaver::AddAttribute( const char* name, int value )
{
    char valueString[m_maxValueLength];
    sprintf( valueString, "%d", value );

    AttributeStruct* attribute = new AttributeStruct;
    attribute->m_name = name;
    attribute->m_value = valueString;

    m_attributes.push_back( attribute );

    return true;
}

bool XMLSaver::AddAttribute( const char* name, double value )
{
    char valueString[m_maxValueLength];
    sprintf( valueString, "%f", value );

    AttributeStruct* attribute = new AttributeStruct;
    attribute->m_name = name;
    attribute->m_value = valueString;

    m_attributes.push_back( attribute );

    return true;
}

bool XMLSaver::AddAttribute( const char* name, bool value )
{
    char valueString[m_maxValueLength];
    sprintf( valueString, "%s", ( value ? "true" : "false" ) );

    AttributeStruct* attribute = new AttributeStruct;
    attribute->m_name = name;
    attribute->m_value = valueString;

    m_attributes.push_back( attribute );

    return true;
}

//===========================================================================
// XMLSaver::FlushAttributes
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
bool XMLSaver::FlushAttributes()
{
    bool returnValue = true;
    
    while( m_attributes.size() > 0 )
    {
        AttributeStruct* attribute = m_attributes.front();
        m_attributes.pop_front();

        Print( " " );
        Print( attribute->m_name );
        Print( "=\"" );
        Print( attribute->m_value );
        returnValue = Print( "\"" );

        delete attribute;
        attribute = NULL;
    }

    return returnValue;
}

//===========================================================================
// XMLSaver::PrintTabs
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
bool XMLSaver::PrintTabs()
{
    bool returnValue = true;
    
    int totalTabs = m_tabSize * m_numTabs;
    for( int i=0; i<totalTabs; i++ )
    {
        returnValue = Print( " " );
    }

    return returnValue;
}

//===========================================================================
// XMLSaver::PrintNewLine
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
bool XMLSaver::PrintNewLine()
{
    return Print( "\r\n" );
}

//===========================================================================
// XMLSaver::Print
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
bool XMLSaver::Print( const char* str )
{
    // Get the number of bytes to be written
    DWORD dwBytesWritten;
    DWORD dwBytesToWrite = strlen( str );

    ::WriteFile
    ( 
        m_hFile,             // handle of file to write to
        (LPVOID)str,         // pointer to data to write to file
        dwBytesToWrite,      // number of bytes to write 
        &dwBytesWritten,     // pointer to number of bytes written 
        0                    // address of structure for data (must be non-zero if
                             // m_hfile is created with FILE_FLAG_OVERLAPPED set.
    );

    // The number of bytes actually written MUST equal the number of bytes requested,
    // otherwise we have some bad data (or a bad data file).
    if( dwBytesWritten != dwBytesToWrite )
    {
        return false;
    }
    else
    {
        return true;
    }
}
