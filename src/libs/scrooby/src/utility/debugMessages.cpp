//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Global utility functions
//
// Description: None
//
// Authors:     Eric Tong
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================
#include "utility/debugMessages.h"
#include <raddebug.hpp>
#include <stdio.h>
#include <stdarg.h>

//#define SCROOBY_VERBOSE
//===========================================================================
// Local Constants, Typedefs, and Macros
//===========================================================================
#ifndef RAD_RELEASE

namespace Scrooby
{
Log* Log::s_instance = NULL;


//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================

bool AsyncFileLoaderMessagesOn = true;

//===========================================================================
// ScroobyInfo
//===========================================================================
// Description: outputs an informational message to the debug console
//
// Constraints:    None
//
// Parameters:    message - the string to output
//
// Return:      NONE
//
//===========================================================================
void ScroobyInfo( const char* message )
{
    Log::Message( LVL_INFO, message );
}

void ScroobyMessage( int level, const char* message )
{
    Scrooby::ErrorLevel errorlevel = static_cast< Scrooby::ErrorLevel >( level );
    Log::Message( errorlevel, message );
}

//===========================================================================
// ScroobyWarning
//===========================================================================
// Description: outputs a warning message to the debug console
//
// Constraints:    None
//
// Parameters:    level - the warning level of this message
//              message - the string to output
//
// Return:      None
//
//===========================================================================
void ScroobyWarning( int level, const char* message )
{
    Log::Message( LVL_WARNING, message );
}

//===========================================================================
// ScroobyError
//===========================================================================
// Description: outputs an error message to the debug console
//
// Constraints:    None
//
// Parameters:    message - the string to output
//
// Return:      None
//
//===========================================================================
void ScroobyError( const char* message )
{
    Log::Message( LVL_ERROR, message );
}

//===========================================================================
// Member Functions
//===========================================================================

//===========================================================================
// Log::Log
//===========================================================================
// Description: basic Log constructor
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
Log::Log() :
#ifdef RAD_RELEASE
m_errorLevel( LVL_SILENT )
#elif defined RAD_TUNE
m_errorLevel( LVL_WARNING )
#elif defined SCROOBY_VERBOSE
m_errorLevel( LVL_DEBUG )
#else
m_errorLevel( LVL_INFO )
#endif
{
    m_errorString = new char[256];
}

//===========================================================================
// Log::~Log
//===========================================================================
// Description: basic Log deconstructor
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
Log::~Log()
{
    delete m_errorString;
}

//===========================================================================
// Log::GetInstance
//===========================================================================
// Description: public method used to get an instance of Log
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
Log* Log::GetInstance()
{
    if( s_instance == NULL )
    {
        s_instance = new Log();
    }
    return s_instance;
}

//===========================================================================
// Log::ScroobyMessage
//===========================================================================
// Description: static message logging method
//
// Constraints:    None
//
// Parameters:    level - error level of message; see ErrorLevel enum
//              message - the message to be sent
//
// Return:      None
//
//===========================================================================
void Log::Message( ErrorLevel level, const char* message, ... )
{
    va_list argList;
    va_start( argList, message );
    Log::GetInstance()->Msg( level, message, argList );
    va_end( argList );
}

//===========================================================================
// Log::Msg
//===========================================================================
// Description: non-static message logging method
//
// Constraints:    None
//
// Parameters:    level - error level of message; see ErrorLevel enum
//              message - the message to be sent
//              va_list - an argument list generated using the stdarg funcions
//
// Return:      None
//
//===========================================================================
void Log::Msg( ErrorLevel level, const char* message, va_list argList )
{
    
    if( level >= m_errorLevel )
    {
        char temp[256];
        vsprintf( temp, message, argList );
        sprintf( m_errorString, "Scrooby Message (Lvl %d) %s\n", level, temp );
        rDebugString( m_errorString );
    }
}

//===========================================================================
// Log::Msg
//===========================================================================
// Description: non-static message logging method
//
// Constraints:    None
//
// Parameters:    level - error level of message; see ErrorLevel enum
//              message - the message to be sent
//              ... - printf style variable argument list
//
// Return:      None
//
//===========================================================================
void Log::Msg( ErrorLevel level, const char* message, ... )
{
    if( level >= m_errorLevel )
    {
        va_list argList;
        va_start( argList, message );
        char temp[256];
        vsprintf( temp, message, argList );
        va_end( argList );
        sprintf( m_errorString, "Scrooby Message (Lvl %d) %s\n", level, temp );
        rDebugString( m_errorString );
    }
}
}    //end namespace

#endif