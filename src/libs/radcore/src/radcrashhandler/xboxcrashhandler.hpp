//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        xboxcrashhandler.hpp
//
// Subsystem:	Foundation Technologies - Crash Handler (XBox Platform)
//
// Description:	This file declares implementations of the FTech crash handling
//              system.
//
// Revisions:  	March 6, 2002   Creation
//
//=============================================================================

#ifndef	XBOXCRASHHANDLER_HPP
#define XBOXCRASHHANDLER_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radcrashhandler.hpp>
#include <radobject.hpp>
#include "crashrecords.hpp"

#include <xtl.h>

//=============================================================================
// Forward Class Declarations
//=============================================================================

class radCrashHandler;

//=============================================================================
// Defintions
//=============================================================================

//=============================================================================
// Class Declarations
//=============================================================================

//
// The handler class
//
class radCrashHandler : public radObject
{
    public:

        static void Intialize( radMemoryAllocator allocator );
        static void Terminate( void );
        static LONG __stdcall XBoxExceptionHandlerFunction( EXCEPTION_POINTERS * pExceptionInfo );

    private:

        radCrashHandler( void );
        ~radCrashHandler( void );

        long HandleCrash( EXCEPTION_POINTERS * pExceptionInfo );
        void TraceFunctions( unsigned int bp );

        enum State { NO_CRASH, CRASH};
        State m_State;
        
        char m_MemCardLetter;

        radCrashRecordXBox * m_pRadCrashRecordXBox;

        // The singleton
        static radCrashHandler * s_pRadCrashHandler;
};




#endif // XBOXCRASHHANDLER_HPP