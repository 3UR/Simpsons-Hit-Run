//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   encapsulates all debug messaging
//
// Description: None
//
// Authors:     Ian Gipson
//
//===========================================================================

// Recompilation protection flag.
#ifndef __SCROOBYDEBUGMESSAGES_H
#define __SCROOBYDEBUGMESSAGES_H

//===========================================================================
// Nested Includes
//===========================================================================
#include "Enums.h"
#include <stdarg.h>

//===========================================================================
// Forward References
//===========================================================================

namespace Scrooby
{
#ifndef RAD_RELEASE
//===========================================================================
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//===========================================================================
extern bool AsyncFileLoaderMessagesOn;

//===========================================================================
// Interface Definitions
//===========================================================================
void ScroobyInfo( const char* message );
void ScroobyMessage( int level, const char* message );
void ScroobyWarning( int level, const char* message );
void ScroobyError( const char* message );

// Add a more general case logger which will handle the log levels automatically
class Log
{
public:
    // static members
    static Log* GetInstance();
    static void Message( ErrorLevel level, const char* message, ... );

    // non-static members
    void Msg( ErrorLevel level, const char* message, va_list argList );
    void Msg( ErrorLevel level, const char* message, ... );

    void SetErrorLevel( ErrorLevel level ) { m_errorLevel = level; };
    ErrorLevel GetErrorLevel() { return m_errorLevel; };
protected:
    Log();
    ~Log();
    ErrorLevel m_errorLevel;
    char* m_errorString;

    static Log* s_instance;
};

#else
    inline void ScroobyInfo( const char* message ){};
    inline void ScroobyMessage( int level, const char* message ){};
    inline void ScroobyWarning( int level, const char* message ){};
    inline void ScroobyError( const char* message ){};

#endif //RAD_RELEASE
}    //end namespace
#endif //__SCROOBYDEBUGMESSAGES_H
