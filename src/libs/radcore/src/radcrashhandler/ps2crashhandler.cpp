//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        ps2crashhandler.cpp
//
// Subsystem:   Foundation Technologies - Crash Handler (Ps2 Platform)
//
// Description: This file contains the implementations of the FTech crash 
//              handling system.
//
// Revisions:   March 14, 2002   Creation
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include <string.h>
#include <eeregs.h>
#include <radfile.hpp>
#include <radcrashhandler.hpp>
#include "crashrecords.hpp"

//=============================================================================
// Definitions
//=============================================================================

#define CRASH_MEM_CARD_PORT         0
#define CRASH_MEM_CARD_SLOT         0
#define CRASH_RECORD_FILENAME       "radCrashReportPs2.crh"
#define RAD_CRASH_TLB_REFILL_VECTOR 0x80000000
#define RAD_CRASH_COMMON_VECTOR     0x80000180
#define SPR                         0x70000000

//=============================================================================
// Global Variable Declaration
//=============================================================================

radMemoryAllocator g_RadCrashHandlerAllocator = RADMEMORY_ALLOC_DEFAULT;
radCrashRecordPs2 * g_pTheRadCrashRecordPs2 = NULL;
unsigned int g_pTheRadCrashRecordPs2FileSize = 0;
ref< IRadDrive > g_pRadCrashMemCard = NULL;
unsigned int g_RadCrashHandlerPs2ReturnAddress = 0;
unsigned int g_RadCrashHandlerPs2CrashCounter = 0;

typedef struct radCrashHandlerTemp
{
    unsigned long long m_k1;
    unsigned int m_ReturnAddress;
};
radCrashHandlerTemp g_RadCrashHandlerPs2Temp;

//=============================================================================
// Imported functions (from ps2crashhandler_asm.s)
//=============================================================================

extern "C" void radCrashHandlerPs2ExceptionHandler_V_TLB_REFILL( void );
extern "C" void radCrashHandlerPs2ExceptionHandler_V_COMMON( void );

//=============================================================================
// ::radExceptionFilterPs2
//=============================================================================

extern "C" void radCrashHandlerPs2WriteReport( void )
{
    unsigned int temp = 0;

    while( temp < 1 )
    {
        temp++;

        rReleasePrintf( "*** radCrashHandlerPs2 handling crash ***\n" );
        rReleasePrintf( "*** CAUSE = [0x%08x]\n", g_pTheRadCrashRecordPs2->m_Registers.cop0r[ 13 ] );
        rReleasePrintf( "*** ErrorEPC = [0x%08x]\n", g_pTheRadCrashRecordPs2->m_Registers.cop0r[ 30 ] );

        //
        // The crash handler report should be all filled up by now.
        // Just add some other general info
        //

        if( g_pTheRadCrashRecordPs2 == NULL )
        {
            rReleasePrintf( "FAIL: radCrashHandlerPs2 found g_pTheRadCrashRecordPs2 == NULL\n" );
            break;
        }
        
        sprintf( g_pTheRadCrashRecordPs2->m_RadCrashRecordInfo.m_Title, RAD_CRASH_RECORD_TITLE );
        sprintf( g_pTheRadCrashRecordPs2->m_RadCrashRecordInfo.m_Platform, RAD_CRASH_RECORD_PS2_PLATFORM );
        sprintf( g_pTheRadCrashRecordPs2->m_RadCrashRecordInfo.m_Version, RAD_CRASH_RECORD_PS2_VERSION );

        //
        // Open a file on the memory card
        //

        ref< IRadFile > pIRadFile = NULL;

        g_pRadCrashMemCard->FileOpen( & pIRadFile, CRASH_RECORD_FILENAME, true, CreateAlways );
        pIRadFile->WaitForCompletion( );
        g_pRadCrashMemCard->WaitForCompletion( );
        if( ! pIRadFile->IsOpen( ) )
        {
            rReleasePrintf( "FAIL: radCrashHandlerPs2 could not open file on memory card\n" );
            break;
        }

        //
        // Write the crash report to the memory card
        //

        pIRadFile->WriteAsync( g_pTheRadCrashRecordPs2, g_pTheRadCrashRecordPs2FileSize );
        pIRadFile->WaitForCompletion( );

        //
        // Close the file.  It's all done.
        //
        pIRadFile = NULL;

        rReleasePrintf( "SUCCESS: radCrashHandlerPs2 wrote crash handler report to memory card\n" );



        radCrashRecordPs2 * pReport = static_cast< radCrashRecordPs2 * >( 
            ::radMemoryAllocAligned( g_RadCrashHandlerAllocator, g_pTheRadCrashRecordPs2FileSize, 256 ) );

        memset( pReport, 0x0C, 8 * 1024 );

        g_pRadCrashMemCard->FileOpen( & pIRadFile, CRASH_RECORD_FILENAME, true, OpenExisting );
        pIRadFile->WaitForCompletion( );
        g_pRadCrashMemCard->WaitForCompletion( );

        pIRadFile->ReadAsync( pReport, g_pTheRadCrashRecordPs2FileSize );
        pIRadFile->WaitForCompletion( );


        pIRadFile->WaitForCompletion( );
    }
    //
    // Might as well hang here and avoid nested exceptions
    //

    unsigned int i;
    while( true )
    {
        i++;
    }
}

//=============================================================================
// ::radCrashHandlerInstallHook
//=============================================================================

void radCrashHandlerInstallHook( unsigned int vectorAddress, unsigned int hookAddress )
{
    //
    // Installing a hook in the exception vector table takes some cleverness.  
    // The vector addresses are not available in user mode.  We can get around this
    // problem by using dma.  We send the existing code at the exception vector
    // to scratch pad memory (spr).  We add the hook to this code in spr.  Then we use
    // dma to transfer the modified code back to the vector.  Pretty fancy.
    //

    //
    // Read the contents of the exception vector
    //

    *D9_SADR = 0x30;            // index into spr (dest)
    *D9_QWC = 5;                // only read in 6 qwords (because we'll be sliding down 3 qwords)
    *D9_MADR = vectorAddress;   // main memory address (source)
    *D_STAT = 1 << 9;           // prepare ch9 dma (main->spr)

    asm volatile( "di" );
    asm volatile( "sync.p" );
    FlushCache( INVALIDATE_ICACHE );

    * D9_CHCR = 0x00000101;              // initiate the transfer
    while( !( * D_STAT & ( 1 << 9 ) ) ); // wait for completion

    asm volatile( "ei" );

    //
    // Add the radCrashHandlerPs2Hook
    //

    unsigned int * pPointer = ( unsigned int * )hookAddress;

    * ( ( unsigned int * ) ( SPR ) ) = * ( pPointer );
    * ( ( unsigned int * ) ( SPR + 4 ) ) = * ( pPointer + 1 );
    * ( ( unsigned int * ) ( SPR + 8 ) ) = * ( pPointer + 2 );
    * ( ( unsigned int * ) ( SPR + 12 ) ) = * ( pPointer + 3 );
    * ( ( unsigned int * ) ( SPR + 16 ) ) = * ( pPointer + 4 );
    * ( ( unsigned int * ) ( SPR + 20 ) ) = * ( pPointer + 5 );
    * ( ( unsigned int * ) ( SPR + 24 ) ) = * ( pPointer + 6 );
    * ( ( unsigned int * ) ( SPR + 28 ) ) = * ( pPointer + 7 );
    * ( ( unsigned int * ) ( SPR + 32 ) ) = * ( pPointer + 8 );
    * ( ( unsigned int * ) ( SPR + 36 ) ) = * ( pPointer + 9 );
    * ( ( unsigned int * ) ( SPR + 40 ) ) = * ( pPointer + 10 );
    * ( ( unsigned int * ) ( SPR + 44 ) ) = * ( pPointer + 11 );


    //
    // Write the new code to the vector
    //

    * D8_SADR = 0;              // index into spr (source)
    * D8_QWC = 8;               // now 8 qwords (the whole space at the vector)
    * D8_MADR = vectorAddress;  // main memory address (dest)
    * D_STAT = 1 << 8;          // prepare the ch8 dma (spr->main)

    asm volatile( "di" );
    asm volatile( "sync.p" );
    FlushCache( INVALIDATE_ICACHE );

    * D8_CHCR = 256;                // initiate the transfer
    while( !( * D_STAT & 256 ) );   // wait for completion

    asm volatile( "ei" );
}

//=============================================================================
// ::radCrashHandlerInitialize
//=============================================================================

void radCrashHandlerInitialize( radMemoryAllocator allocator )
{
    g_RadCrashHandlerAllocator = allocator;  

    //
    // Only call this function once
    //

    if( g_pRadCrashMemCard != NULL )
    {
        rAssert( g_pRadCrashMemCard == NULL );
        return;
    }
    if( g_pTheRadCrashRecordPs2 != NULL )
    {
        rAssert( g_pTheRadCrashRecordPs2 == NULL );
        return;
    }

    //
    // Prepare a memory card IRadDrive object
    //

    IRadDrive::MediaInfo mediaInfo;

    // Open the drive

    ::radDriveOpen( & g_pRadCrashMemCard, "MEMCARDSLOT1:" );
    g_pRadCrashMemCard->GetMediaInfoAsync( & mediaInfo );
    g_pRadCrashMemCard->WaitForCompletion( );

    // Format if necessary

    if( mediaInfo.m_MediaState == IRadDrive::MediaInfo::MediaNotFormatted )
    {
        g_pRadCrashMemCard->FormatAsync( );
        g_pRadCrashMemCard->WaitForCompletion( );

        g_pRadCrashMemCard->GetMediaInfoAsync( & mediaInfo );
        g_pRadCrashMemCard->WaitForCompletion( );
    }

    if( mediaInfo.m_MediaState != IRadDrive::MediaInfo::MediaPresent )
    {
        // If a memorycard is not present, don't really do anything. Just return

        rReleasePrintf( "FAIL: radCrashHandler failed to detect memorycard\n" );
        return;
    }

    //
    // Format always
    //

    g_pRadCrashMemCard->FormatAsync( );
    g_pRadCrashMemCard->WaitForCompletion( );

    //
    // Initialize the crash record
    //

    //
    // The size of the crash record file will have to be a multiple 
    // of the memory card sector size
    //

    g_pTheRadCrashRecordPs2FileSize = sizeof( radCrashRecordPs2 );
    g_pTheRadCrashRecordPs2FileSize = g_pTheRadCrashRecordPs2FileSize -
        ( g_pTheRadCrashRecordPs2FileSize % mediaInfo.m_SectorSize ) + mediaInfo.m_SectorSize;

    //
    // Allocate memory for the crash record.  Make it a very safe alignment.
    //

    g_pTheRadCrashRecordPs2 = static_cast< radCrashRecordPs2 * >( 
        ::radMemoryAllocAligned( g_RadCrashHandlerAllocator, g_pTheRadCrashRecordPs2FileSize, 256 ) );

    //
    // Install the exception hooks.  The code will be executed first when any exceptions occurs
    //

    ::radCrashHandlerInstallHook( RAD_CRASH_TLB_REFILL_VECTOR, (unsigned int) radCrashHandlerPs2ExceptionHandler_V_TLB_REFILL );    
    ::radCrashHandlerInstallHook( RAD_CRASH_COMMON_VECTOR, (unsigned int) radCrashHandlerPs2ExceptionHandler_V_COMMON );    
}

//=============================================================================
// ::radCrashHandlerTerminate
//=============================================================================

void radCrashHandlerTerminate( void )
{
    ::radMemoryFreeAligned( g_RadCrashHandlerAllocator, g_pTheRadCrashRecordPs2 );
}
