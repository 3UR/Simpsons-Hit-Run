//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        crashrecords.hpp
//
// Subsystem:   Foundation Technologies - Crash Handler
//
// Description: This central file defines the crash records for each platform
//
// Revisions:   April 1, 2002   Creation
//
//=============================================================================

#ifndef CRASHRECORDS_HPP
#define CRASHRECORDS_HPP

//=============================================================================
// Definitions
//=============================================================================

#define RAD_CRASH_RECORD_TITLE "RADCRASH"

// XBox

#define RAD_CRASH_RECORD_XBOX_PLATFORM                         "XBox"
#define RAD_CRASH_RECORD_XBOX_VERSION                          "V1.0"
#define RAD_CRASH_RECORD_XBOX_MAX_EXCEPTION_PARAMS             15
#define RAD_CRASH_RECORD_XBOX_STACK_DUMP_SIZE                  1280
#define RAD_CRASH_RECORD_XBOX_STACK_DUMP_SP_OFFSET             0
#define RAD_CRASH_RECORD_XBOX_THIS_POINTER_DUMP_SIZE           384
#define RAD_CRASH_RECORD_XBOX_THIS_POINTER_DUMP_POINTER_OFFSET 128
#define RAD_CRASH_RECORD_XBOX_STACK_TRACE_SIZE                 10
#define RAD_CRASH_RECORD_XBOX_STACK_TRACE_NUM_ARGS             7
#define RAD_CRASH_RECORD_SIZE_OF_FX_REGISTERS                  128

// Ps2

#define RAD_CRASH_RECORD_PS2_PLATFORM             "PS2"
#define RAD_CRASH_RECORD_PS2_VERSION              "V1.0"
#define RAD_CRASH_RECORD_PS2_STACK_DUMP_SIZE      1280  // also defined in ps2crashhandler_asm.s
#define RAD_CRASH_RECORD_PS2_STACK_DUMP_SP_OFFSET 0   // Where SP points to
#define RAD_CRASH_RECORD_PS2_STACK_TRACE_SIZE     11

//=============================================================================
// Forward Declarations
//=============================================================================

struct radCrashRecordInfo;
struct radCrashRecordXBox;
struct radCrashRecordPs2;

//=============================================================================
// Cross-platform Declarations
//=============================================================================

struct radCrashRecordInfo
{
    char m_Title[ 8 ];
    char m_Platform[ 16 ];
    char m_Version[ 16 ];
    unsigned int m_Day;    // [1..31]
    unsigned int m_Month;  // [1..12] 
    unsigned int m_Year; 
    unsigned int m_Hour;
    unsigned int m_Minute;
    unsigned int m_Second;
};

//=============================================================================
// XBox Declarations
//=============================================================================

struct radCrashRecordXBox
{

    //
    // XBox defined exception codes
    //

    enum radCrashExceptionCodeXBox
    {
        RAD_STATUS_ACCESS_VIOLATION         = 0xC0000005,
        RAD_STATUS_IN_PAGE_ERROR            = 0xC0000006,
        RAD_STATUS_INVALID_HANDLE           = 0xC0000008,   
        RAD_STATUS_NO_MEMORY                = 0xC0000017,    
        RAD_STATUS_ILLEGAL_INSTRUCTION      = 0xC000001D,    
        RAD_STATUS_NONCONTINUABLE_EXCEPTION = 0xC0000025,    
        RAD_STATUS_INVALID_DISPOSITION      = 0xC0000026,    
        RAD_STATUS_ARRAY_BOUNDS_EXCEEDED    = 0xC000008C,    
        RAD_STATUS_FLOAT_DENORMAL_OPERAND   = 0xC000008D,    
        RAD_STATUS_FLOAT_DIVIDE_BY_ZERO     = 0xC000008E,    
        RAD_STATUS_FLOAT_INEXACT_RESULT     = 0xC000008F,    
        RAD_STATUS_FLOAT_INVALID_OPERATION  = 0xC0000090,    
        RAD_STATUS_FLOAT_OVERFLOW           = 0xC0000091,    
        RAD_STATUS_FLOAT_STACK_CHECK        = 0xC0000092,    
        RAD_STATUS_FLOAT_UNDERFLOW          = 0xC0000093,    
        RAD_STATUS_INTEGER_DIVIDE_BY_ZERO   = 0xC0000094,    
        RAD_STATUS_INTEGER_OVERFLOW         = 0xC0000095,    
        RAD_STATUS_PRIVILEGED_INSTRUCTION   = 0xC0000096,    
        RAD_STATUS_STACK_OVERFLOW           = 0xC00000FD,    
        RAD_STATUS_CONTROL_C_EXIT           = 0xC000013A,    
        RAD_STATUS_FLOAT_MULTIPLE_FAULTS    = 0xC00002B4,    
        RAD_STATUS_FLOAT_MULTIPLE_TRAPS     = 0xC00002B5,    
        RAD_STATUS_REG_NAT_CONSUMPTION      = 0xC00002C9    
    };

    //
    // XBox defined exception info structure
    //

    struct EXCEPTION_RECORD
    {
        long   ExceptionCode;
        long   ExceptionFlags;
        EXCEPTION_RECORD * ExceptionRecord;
        void * ExceptionAddress;
        long   NumberParameters;
        unsigned long ExceptionInformation[ RAD_CRASH_RECORD_XBOX_MAX_EXCEPTION_PARAMS ];
    };

    //
    // XBox defined context structure (see WinNT.h for more details)
    //

    struct FLOATING_SAVE_AREA {
        unsigned short ControlWord;
        unsigned short StatusWord;
        unsigned short TagWord;
        unsigned short ErrorOpcode;
        unsigned int   ErrorOffset;
        unsigned int   ErrorSelector;
        unsigned int   DataOffset;
        unsigned int   DataSelector;
        unsigned int   MXCsr;
        unsigned int   Reserved2;
        unsigned char  RegisterArea[ RAD_CRASH_RECORD_SIZE_OF_FX_REGISTERS ];
        unsigned char  mmRegisterArea[ RAD_CRASH_RECORD_SIZE_OF_FX_REGISTERS ];
        unsigned char  Reserved4[224];
        unsigned int   Cr0NpxState;
    };


    struct CONTEXT 
    {
        long ContextFlags;

        // CONTEXT_FLOATING_POINT

        FLOATING_SAVE_AREA FloatSave;

        // CONTEXT_INTEGER

        long Edi;
        long Esi;
        long Ebx;
        long Edx;
        long Ecx;
        long Eax;

        // CONTEXT_CONTROL

        long Ebp;
        long Eip;
        long SegCs;     
        long EFlags;    
        long Esp;
        long SegSs;
    };

    struct FunctionInfo
    {
        unsigned int m_pFunction;
        unsigned int m_pArgs[ RAD_CRASH_RECORD_XBOX_STACK_TRACE_NUM_ARGS ];
    };

    //
    // Member variables
    //

    radCrashRecordInfo m_RadCrashRecordInfo;
    EXCEPTION_RECORD m_ExceptionRecord;
    CONTEXT m_Context;
    unsigned int m_AddressOfMain;
    FunctionInfo m_StackTrace[ RAD_CRASH_RECORD_XBOX_STACK_TRACE_SIZE ];
    unsigned int m_StackDump[ RAD_CRASH_RECORD_XBOX_STACK_DUMP_SIZE ];
    unsigned int m_ThisPointerDump[ RAD_CRASH_RECORD_XBOX_THIS_POINTER_DUMP_SIZE ];
};

//=============================================================================
// Playstation 2 Declarations
//=============================================================================

struct radCrashRecordPs2
{
    //
    // Ps2 EE Hardware Exception Codes
    //
    // (Note, the values correspond to bits 6:2 of the Cause register)
    //

    enum radCrashExceptionPs2
    {
        RAD_EXC_INTERRUPT = 0,
        RAD_EXC_TLB_MOD,
        RAD_EXC_TLB_MISS_READ,
        RAD_EXC_TLB_MISS_WRITE,
        RAD_EXC_ADDR_ERROR_READ,
        RAD_EXC_ADDR_ERROR_WRITE,
        RAD_EXC_INSTR_BUS_ERROR,
        RAD_EXC_DATA_BUS_ERROR,
        RAD_EXC_SYSCALL,
        RAD_EXC_BREAK,
        RAD_EXC_ILLEGAL_INSTR,
        RAD_EXC_COP_UNUSABLE,
        RAD_EXC_OVERFLOW
    };

    //
    // Registers
    //

    struct Registers
    {
        //
        // CPU Registers
        //
        struct _128Bits     // Provides 128 bit support for windows
        {
            unsigned int pos3; // lsb
            unsigned int pos2;
            unsigned int pos1;
            unsigned int pos0; // msb
        };

        struct _64Bits      // Provides 64 bit support for windows
        {
            unsigned int pos1; // lsb
            unsigned int pos0; // msb
        };

        _128Bits gpr[ 32 ];
        _64Bits hi[ 2 ];
        _64Bits lo[ 2 ];
        
        unsigned int cpuReserved[ 4 ]; // no sa or pc register currently

        //
        // COP0 Registers
        //
        
        unsigned int cop0r[ 32 ];
    };

    //
    // Member variables
    //

    radCrashRecordInfo m_RadCrashRecordInfo;
    Registers m_Registers;
    unsigned int m_StackDump[ RAD_CRASH_RECORD_PS2_STACK_DUMP_SIZE ];
    unsigned int m_StackTrace[ RAD_CRASH_RECORD_PS2_STACK_TRACE_SIZE ];
};

#endif // CRASHRECORDS_HPP