//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        xboxcrashhandler.cpp
//
// Subsystem:   Foundation Technologies - Crash Handler (XBox Platform)
//
// Description: This file contains the implementations of the FTech crash 
//              handling system.
//
// Revisions:   March 6, 2002   Creation
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <raddebug.hpp>
#include <radmemory.hpp>
#include <radstacktrace.hpp>
#include "xboxcrashhandler.hpp"

extern __cdecl main( void );

//=============================================================================
// Definitions
//=============================================================================

#define CRASH_RECORD_FILENAME "z:\\radCrashReportXBox.crh" // The 'z' will get replaced

//=============================================================================
// Static Member Initialization
//=============================================================================

radCrashHandler * radCrashHandler::s_pRadCrashHandler = NULL;

//=============================================================================
// Component radCrashHandler
//=============================================================================

//=============================================================================
// radCrashHandler::radCrashHandler
//=============================================================================

radCrashHandler::radCrashHandler( void )
    :
    m_State( NO_CRASH ),
    m_MemCardLetter( 0 )
{
    //
    // Get a nice piece of aligned memory for the crash record
    //

    m_pRadCrashRecordXBox = static_cast< radCrashRecordXBox * >( ::radMemoryAllocAligned( GetThisAllocator( ),
        sizeof( radCrashRecordXBox ), 64 ) );
    
    memset( m_pRadCrashRecordXBox, 0, sizeof( radCrashRecordXBox ) );

    //
    // Mount the memory card (that must be located in the bottom slot of the first controller)
    // and get the letter associated with it
    //

    for( unsigned int i = 0; i < 100000000; i++ )
    {
        ;;  // This seems to be the only way to find the memory card always
    }

    long ret = XMountMU( XDEVICE_PORT0, XDEVICE_BOTTOM_SLOT, & m_MemCardLetter );


    // 
    // If mounting the memory card worked, then XBoxExceptionHandlerFunction can
    // be set as an UnHandledExceptionFilter
    //

    if( ret == ERROR_SUCCESS )
    {
        rDebugPrintf( "SUCCESS: radCrashHandler detected memory card in BOTTOM SLOT of PORT 0\n" );

        // Note that the following function returns the previous exception filter.
        // It will probably be null and I'm just throwing it away

        SetUnhandledExceptionFilter( XBoxExceptionHandlerFunction );
    }
    else
    {
        rDebugPrintf( "WARNING: radCrashHandler requires a memory card in BOTTOM SLOT of PORT 0\n" );
    }
}

//=============================================================================
// radCrashHandler::~radCrashHandler
//=============================================================================

radCrashHandler::~radCrashHandler( void )
{
    //
    // Clean up the crash record
    //

    ::radMemoryFreeAligned( GetThisAllocator( ), m_pRadCrashRecordXBox );

    //
    // Unmount the memory unit
    //

    if( m_MemCardLetter != 0x00 )
    {
        long ret = XUnmountMU( XDEVICE_PORT0, XDEVICE_BOTTOM_SLOT );

        if( ret != ERROR_SUCCESS )
        {
            rDebugPrintf( "WARNING: radCrashHandler could not unmount its memory unit\n" );
        }
    }  
}

//=============================================================================
// radCrashHandler::Initialize
//=============================================================================

/* static */ void radCrashHandler::Intialize( radMemoryAllocator allocator )
{
    rAssert( s_pRadCrashHandler == NULL );

    s_pRadCrashHandler = new( allocator )radCrashHandler( );
}

//=============================================================================
// radCrashHandler::Terminate
//=============================================================================

/* static */ void radCrashHandler::Terminate( void )
{
    rAssert( s_pRadCrashHandler != NULL );

    delete s_pRadCrashHandler;
}

//=============================================================================
// radCrashHandler::XBoxExceptionHandlerFunction
//=============================================================================

/* static */ LONG __stdcall radCrashHandler::XBoxExceptionHandlerFunction
( 
    EXCEPTION_POINTERS * pExceptionInfo 
)
{
    return s_pRadCrashHandler->HandleCrash( pExceptionInfo );
}

//=============================================================================
// radCrashHandler::HandleCrash
//=============================================================================

LONG radCrashHandler::HandleCrash
( 
    EXCEPTION_POINTERS * pExceptionInfo 
)
{
    if( m_MemCardLetter != 0x00 )
    {
        //
        // We keep track of state so that we don't handle nested crashes
        //

        switch( m_State )
        {
            case NO_CRASH:
            {
                //
                // Update the state
                //
                m_State = CRASH;

                //
                // Create a file on the memory card
                //

                char buf[ 17 ];
                sprintf( buf, CRASH_RECORD_FILENAME );
                buf[ 0 ] = m_MemCardLetter;         

                HANDLE handle = CreateFile( buf, 
                    GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

                if( handle == INVALID_HANDLE_VALUE )
                {

                    unsigned long error = GetLastError( );
                    
                    // Nothing to do
                        
                    break;
                }

                //
                // Fill the header of the crash record
                //

                sprintf( m_pRadCrashRecordXBox->m_RadCrashRecordInfo.m_Title,    RAD_CRASH_RECORD_TITLE );
                sprintf( m_pRadCrashRecordXBox->m_RadCrashRecordInfo.m_Platform, RAD_CRASH_RECORD_XBOX_PLATFORM );
                sprintf( m_pRadCrashRecordXBox->m_RadCrashRecordInfo.m_Version,  RAD_CRASH_RECORD_XBOX_VERSION );

                SYSTEMTIME systemTime;
                GetLocalTime( & systemTime );

                m_pRadCrashRecordXBox->m_RadCrashRecordInfo.m_Day = systemTime.wDay;
                m_pRadCrashRecordXBox->m_RadCrashRecordInfo.m_Month = systemTime.wMonth;
                m_pRadCrashRecordXBox->m_RadCrashRecordInfo.m_Year = systemTime.wYear;
                m_pRadCrashRecordXBox->m_RadCrashRecordInfo.m_Hour = systemTime.wHour;
                m_pRadCrashRecordXBox->m_RadCrashRecordInfo.m_Minute = systemTime.wMinute;
                m_pRadCrashRecordXBox->m_RadCrashRecordInfo.m_Second = systemTime.wSecond;
                
                //
                // Hang on the exception info that we were passed
                //

                m_pRadCrashRecordXBox->m_ExceptionRecord = 
                    * reinterpret_cast< radCrashRecordXBox::EXCEPTION_RECORD * >( pExceptionInfo->ExceptionRecord );
                m_pRadCrashRecordXBox->m_Context =
                    * reinterpret_cast< radCrashRecordXBox::CONTEXT * >( pExceptionInfo->ContextRecord );

                //
                // Now for the stack dump
                //

                unsigned int stackDumpSize = RAD_CRASH_RECORD_XBOX_STACK_DUMP_SIZE * sizeof( unsigned int );

                while( IsBadReadPtr( (void *)m_pRadCrashRecordXBox->m_Context.Esp, stackDumpSize ) )
                {
                    if( stackDumpSize < 0x1000 ) 
                    {
                        stackDumpSize = 0;
                        break;
                    }

                    if( ( ( stackDumpSize + m_pRadCrashRecordXBox->m_Context.Esp ) & 0x0fff ) != 0 )
                    {
                        stackDumpSize -= ( stackDumpSize + m_pRadCrashRecordXBox->m_Context.Esp ) & 0x0fff;
                    }
                    else
                    {
                        stackDumpSize -= 0x1000;
                    }
                }

                if( stackDumpSize != 0 )
                {
                    ::memcpy( m_pRadCrashRecordXBox->m_StackDump, 
                        (void *)m_pRadCrashRecordXBox->m_Context.Esp, 
                        stackDumpSize );
                }

                //
                // We'll grab the this pointer data if it exists
                //

                int thisPointerData = ( m_pRadCrashRecordXBox->m_Context.Ecx ) - ( RAD_CRASH_RECORD_XBOX_THIS_POINTER_DUMP_POINTER_OFFSET * 4);

                if( thisPointerData < 0 )
                {
                    thisPointerData = 0;
                }

                unsigned int stackTraceSize = RAD_CRASH_RECORD_XBOX_THIS_POINTER_DUMP_SIZE * sizeof( unsigned int );

                while( IsBadReadPtr( ( void * ) thisPointerData, stackTraceSize ) )
                {
                    if( stackTraceSize < 0x1000 )
                    {
                        stackTraceSize = 0;
                        break;
                    }

                    if( ( ( stackDumpSize + thisPointerData ) & 0x0fff ) != 0 )
                    {
                        stackTraceSize -= ( stackTraceSize + thisPointerData ) & 0x0fff;
                    }
                    else
                    {
                        stackTraceSize -= 0x1000;
                    }
                }

                if( stackTraceSize != 0 )
                {
                    ::memcpy( m_pRadCrashRecordXBox->m_StackTrace, 
                        ( void * ) thisPointerData, 
                        stackTraceSize );
                }

                //
                // Now to use our trace the function calls back
                //

                TraceFunctions( m_pRadCrashRecordXBox->m_Context.Ebp );

                //
                // Remember the address of main()
                //

                m_pRadCrashRecordXBox->m_AddressOfMain = ( unsigned int )main;

                //
                // Write out the crash record to memory unit
                //

                int ret = 0;
                unsigned long written = 0;
                ret = WriteFile( handle, m_pRadCrashRecordXBox, sizeof( radCrashRecordXBox ), & written, NULL );
                ret = CloseHandle( handle );
                handle = 0;

                break;
            }
            default:
            {
                rDebugPrintf( "ERROR: radCrashHandler FAILED. radCrashHandler generated an exception\n" );
                break;
            }
        }
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

//=============================================================================
// radCrashHandler::TraceFunctions
//=============================================================================

void radCrashHandler::TraceFunctions( unsigned int bp )
{
    unsigned int  prevfp;
    unsigned int  curfp;

    prevfp = 0x00000000;

    curfp = bp;

    for( unsigned int i = 0; i < RAD_CRASH_RECORD_XBOX_STACK_TRACE_SIZE; i++ )
    {
        if (curfp != NULL &&     // is this frame pointer not NULL
            curfp > prevfp &&    // this frame pointer has to be greater than the previous
            !(curfp & 0x3))      // has to be 4byte aligned
        {
            // The return address is on step above the base pointer
            m_pRadCrashRecordXBox->m_StackTrace[ i ].m_pFunction = * ( (unsigned int *)( curfp + 4 ) );

            // The arguments are located between the current base pointer and the next one
            // So we'll interate the frame pointers to get at the args

            prevfp = curfp;
            curfp = * ( ( int * ) curfp );

            // Get the arguments

            unsigned int numArgs = curfp - prevfp;
            if( numArgs > RAD_CRASH_RECORD_XBOX_STACK_TRACE_NUM_ARGS )
            {
                numArgs = RAD_CRASH_RECORD_XBOX_STACK_TRACE_NUM_ARGS;
            }

            ::memcpy( m_pRadCrashRecordXBox->m_StackTrace[ i ].m_pArgs,
                ( void * )( prevfp + 8 ), numArgs * sizeof( unsigned int ) );
        }
    }
}

//=============================================================================
// Global Functions
//=============================================================================

//=============================================================================
// ::radCrashHandlerInitialize
//=============================================================================

void radCrashHandlerInitialize( radMemoryAllocator allocator )
{
    radCrashHandler::Intialize( allocator );
}

//=============================================================================
// ::radCrashHandlerTerminate
//=============================================================================

void radCrashHandlerTerminate( void )
{
    radCrashHandler::Terminate( );
}
 
   
