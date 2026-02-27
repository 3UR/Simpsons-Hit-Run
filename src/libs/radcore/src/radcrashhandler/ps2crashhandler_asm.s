#=============================================================================
#
# File:         ps2crashhanlder_asm.s
#
# Subsystem:    Radical CrashHandler - ps2 
#
# Description:  This file implements the low level functionality of the ps2
#               radCrashHandler system.  This is where the system installs
#               its exception vector.  The vector will ignore most exceptions,
#               passing them on to the kernel for normal processing.  In the
#               case of a crash, the vector will record context information,etc.
#               and launch the crash handler.
#
# Revisions:    April 11, 2002  RWS  Creation
#
#=============================================================================

#=============================================================================
# Definitions
#=============================================================================

#
# CPU Register definitions
#

#ifndef mips_regdef_h
#ifndef _CPUREG_H_
#define zero    $0  /* wired zero */
#define AT	    $at	/* assembler temp */
#define v0	    $2	/* return value */
#define v1	    $3
#define a0	    $4	/* argument registers */
#define a1      $5
#define a2  	$6
#define a3	    $7
#define t0	    $8	/* caller saved */
#define t1	    $9
#define t2	    $10
#define t3	    $11
#define t4	    $12
#define t5	    $13
#define t6	    $14
#define t7	    $15
#define s0	    $16	/* callee saved */
#define s1	    $17
#define s2	    $18
#define s3	    $19
#define s4	    $20
#define s5	    $21
#define s6	    $22
#define s7	    $23
#define t8	    $24	/* code generator */
#define t9	    $25
#define k0	    $26	/* kernel temporary */
#define k1	    $27
#define gp	    $28	/* global pointer */
#define sp	    $29	/* stack pointer */
#define fp	    $30	/* frame pointer */
#define ra  	$31	/* return address */
#endif /* mips_regdef_h */
#endif /* _CPUREG_H_ */

#define pc      $32

#
# COP0 Registers
#

#define status   $12
#define cause    $13
#define epc      $14
#define errorEPC $30

#
# Other useful definitions
#

#define RAD_CRASH_RECORD_PS2_STACK_DUMP_SIZE 1280 # also defined int crashrecords.hpp
#define RAD_CRASH_RECORD_PS2_STACK_TRACE_SIZE 11  # ditto
#define MAX_SP_VAL    0x08000000    # sp should be smaller than this
#define m_Registers   0x40
#define m_StackDump   0x2f0
#define m_StackTrace  0x16f0
#define WRITE_THROUGH_D_CACHE 0x20000000
#define V_COMMON_RETURN_ADDRESS 0x800001b0
#define V_TLB_REFILL_RETURN_ADDRESS 0x80000030

#
# Cause register exception codes
#

#define ExcCode       0x7C

#define ExcCode_Mod   4     # TLB Modified
#define ExcCode_TLBL  8     # TLB Refill (instruction fetch or load)
#define ExcCode_TLBS  12    # TLB Refill (store)
#define ExcCode_AdEL  16    # Address Error (instruction fetch or load)
#define ExcCode_AdES  20    # Address Error (store)
#define ExcCode_IBE   24    # Bus Error (instruction)
#define ExcCode_DBE   28    # Bus Error (data)
#define ExcCode_CpU   44    # Coprocessor Unusable

#
# g_RadCrashHandlerPs2Temp member offsets
#

#define m_k1            0x00
#define m_ReturnAddress 0x10

#=============================================================================
# Imported Functions
#=============================================================================


#=============================================================================
# radCrashHandlerPs2Launcher( void )
#=============================================================================
#
# When a fatal exception occurs, this code will replace the offending code.
# That way, when the control returns to the program the radCrashHandlerPs2 
# will be launched and a crash report will be written to the memory card
#
    .ent    radCrashHandlerPs2Launcher

radCrashHandlerPs2Launcher:
            
    jal     radCrashHandlerPs2WriteReport
    nop

    .size   radCrashHandlerPs2Launcher,.-radCrashHandlerPs2Launcher
    .end    radCrashHandlerPs2Launcher


#=============================================================================
# radCrashHandlerPs2ExceptionHandler( void )
#=============================================================================
#
# The radCrashHandlerPs2ExceptionHandler only processes exceptions that are
# fatal.  It will pass control of all others back to the original exception
# handler.
#
# When fatal crash occurs, this handler will record all of the state
# information (registers, etc) that will be needed to make a crash report.
#
# In order to write the crash report to a memory card, the program needs
# to be in user mode.  In order to create this condition, the handler will
# replace the code where the exception occurred with the radCrashHandlerPs2
# code that will write to the memory card.  After replacing the offending code,
# the handler returns control to the user program at the location where the new
# code is written.
#

    .global radCrashHandlerPs2ExceptionHandler
    .ent    radCrashHandlerPs2ExceptionHandler
    .set    noreorder
    .set    noat # this allows access to the 'at' register without a compiler warning

radCrashHandlerPs2ExceptionHandler:

    # Check the cause of this exception.  If the cause means
    # the program crashed, then handle it, otherwise return 
    # control to the kernel.
    #
    mfc0    k0,cause
    nop
    andi    k0,k0,ExcCode 

    # TLB Modified

    li      k1,ExcCode_Mod
    beq     k0,k1,.handleCrash
    nop

    # TLB Refill (instruction fetch or load)

    li      k1,ExcCode_TLBL
    beq     k0,k1,.handleCrash
    nop

    # TLB Refill (store)

    li      k1,ExcCode_TLBS
    beq     k0,k1,.handleCrash
    nop

    # Address Error (instruction fetch or load)

    li      k1,ExcCode_AdEL
    beq     k0,k1,.handleCrash
    nop

    # Address Error (store)

    li      k1,ExcCode_AdES
    beq     k0,k1,.handleCrash
    nop

    # Bus Error (instruction)

    li      k1,ExcCode_IBE
    beq     k0,k1,.handleCrash
    nop

    # Bus Error (data)

    li      k1,ExcCode_DBE
    beq     k0,k1,.handleCrash
    nop

    # Coprocessor Unusable

    li      k1,ExcCode_CpU
    beq     k0,k1,.handleCrash
    nop

    # For unrecognized codes, let the program return
    # to the control of the kernel

####
#
.returnToKernel:
    
    # This is not an exception for us.  Return to 
    # the vector table
    #    

    # Restore k0

    la      k0,g_RadCrashHandlerPs2Temp
    lq      k1,m_k1(k0)
    nop

    # Load and jump to kernel handler address

    lw      k0,m_ReturnAddress(k0)
    nop
    jr      k0
    nop

####
#
.handleCrash:

    # See how many crashes have occured thus far.
    # Only handle if this is the first crash, otherwise
    # give control back to the kernel
    #
    la      k0,g_RadCrashHandlerPs2CrashCounter
    lw      k1,0(k0)
    nop
    bne     k1,zero,.returnToKernel
    nop

    # Get ready to handle the crash.  Increment the crash
    # counter.  We will not worry about nested crashes.
    #
    la      k0,g_RadCrashHandlerPs2CrashCounter
    lw      k1,0(k0)
    nop
    addiu   k1,k1,0x01
    sw      k1,0(k0)

    # Prepare the Crashrecord.  Save the contents of 
    # all of the registers and the stack, etc.
    #
    lw      k0,g_pTheRadCrashRecordPs2
    nop


    # The EE general-purpose registers are 128 bits.  Note that the instructions
    # to deal with this are part of the extended EE core instruction set

    addiu   k0,k0,m_Registers

    sq      $0,0x000(k0)     # I am not using the friendly names here because
    sq      $1,0x010(k0)     # it is easier for me to keep track of things
    sq      $2,0x020(k0)
    sq      $3,0x030(k0)
    sq      $4,0x040(k0)
    sq      $5,0x050(k0)
    sq      $6,0x060(k0)
    sq      $7,0x070(k0)
    sq      $8,0x080(k0)
    sq      $9,0x090(k0)
    sq      $10,0x0a0(k0)
    sq      $11,0x0b0(k0)
    sq      $12,0x0c0(k0)
    sq      $13,0x0d0(k0)
    sq      $14,0x0e0(k0)
    sq      $15,0x0f0(k0)
    sq      $16,0x100(k0)
    sq      $17,0x110(k0)
    sq      $18,0x120(k0)
    sq      $19,0x130(k0)
    sq      $20,0x140(k0)
    sq      $21,0x150(k0)
    sq      $22,0x160(k0)
    sq      $23,0x170(k0)
    sq      $24,0x180(k0)
    sq      $25,0x190(k0)
    sq      $26,0x1a0(k0)
    sq      $27,0x1b0(k0)
    sq      $28,0x1c0(k0)
    sq      $29,0x1d0(k0)
    sq      $30,0x1e0(k0)
    sq      $31,0x1f0(k0)

    # The hi and lo registers are 64 bit

    mfhi    k1
    sd      k1,0x200(k0)
    mfhi1   k1
    sd      k1,0x208(k0)
    mflo    k1
    sd      k1,0x210(k0)
    mflo1   k1
    sd      k1,0x218(k0)

    # I have saved room for the pc and sa registers
    # but can not get a hold of them.  This space
    # takes up 64 bits

    # The cop0 registers are 32 bit

    mfc0    k1,$0
    nop
    sw      k1,0x230(k0)
    mfc0    k1,$1
    nop
    sw      k1,0x234(k0)
    mfc0    k1,$2
    nop
    sw      k1,0x238(k0)
    mfc0    k1,$3
    nop
    sw      k1,0x23c(k0)

    mfc0    k1,$4
    nop
    sw      k1,0x240(k0)
    mfc0    k1,$5
    nop
    sw      k1,0x244(k0)
    mfc0    k1,$6
    nop
    sw      k1,0x248(k0)
    mfc0    k1,$7
    nop
    sw      k1,0x24c(k0)

    mfc0    k1,$8
    nop
    sw      k1,0x250(k0)
    mfc0    k1,$9
    nop
    sw      k1,0x254(k0)
    mfc0    k1,$10
    nop
    sw      k1,0x258(k0)
    mfc0    k1,$11
    nop
    sw      k1,0x25c(k0)

    mfc0    k1,$12
    nop
    sw      k1,0x260(k0)
    mfc0    k1,$13
    nop
    sw      k1,0x264(k0)
    mfc0    k1,$14
    nop
    sw      k1,0x268(k0)
    mfc0    k1,$15
    nop
    sw      k1,0x26c(k0)

    mfc0    k1,$16
    nop
    sw      k1,0x270(k0)
    mfc0    k1,$17
    nop
    sw      k1,0x274(k0)
    mfc0    k1,$18
    nop
    sw      k1,0x278(k0)
    mfc0    k1,$19
    nop
    sw      k1,0x27c(k0)

    mfc0    k1,$20
    nop
    sw      k1,0x280(k0)
    mfc0    k1,$21
    nop
    sw      k1,0x284(k0)
    mfc0    k1,$22
    nop
    sw      k1,0x288(k0)
    mfc0    k1,$23
    nop
    sw      k1,0x28c(k0)

    mfc0    k1,$24
    nop
    sw      k1,0x290(k0)
    mfc0    k1,$25
    nop
    sw      k1,0x294(k0)
    mfc0    k1,$26
    nop
    sw      k1,0x298(k0)
    mfc0    k1,$27
    nop
    sw      k1,0x29c(k0)

    mfc0    k1,$28
    nop
    sw      k1,0x2a0(k0)
    mfc0    k1,$29
    nop
    sw      k1,0x2a4(k0)
    mfc0    k1,$30
    nop
    sw      k1,0x2a8(k0)
    mfc0    k1,$31
    nop
    sw      k1,0x2ac(k0)

    # Store the content of memory around the stack pointer 
    #
    # (Just grab RAD_CRASH_RECORD_PS2_STACK_DUMP_SIZE words)

    move    t0,sp                                   # t0 = real stack
    lw      k0,g_pTheRadCrashRecordPs2
    nop
    addiu   k0,k0,m_StackDump                       # k0 = stack dump
    li      t1,RAD_CRASH_RECORD_PS2_STACK_DUMP_SIZE # t1 = count 
    li      t4,MAX_SP_VAL                           # keep sp less than this

####
#
.stackDumpBegin:

    beq     t1,zero,.stackTraceBegin #>while( t1 > 0 )
    nop                              #>{
    lw      t2,0(t0)                 #>   t2 = * t0;
    nop                              #>
    sw      t2,0(k0)                 #>   * k0 = t2;
    addiu   k0,k0,0x04               #>   k0 += 4;           
    addiu   t0,t0,0x04               #>   t0 += 4;
    sub     t1,t1,0x01               #>   t1--;
                                     #>   if( t0 == MAX_SP_VAL) 
    beq     t0,t4,.stackTraceBegin   #>       break;
    nop                              #>
    b       .stackDumpBegin          #>
    nop                              #>}

####
#
.stackTraceBegin:

    # Trace the stack using radStackTracePs2Get( ).  The registers
    # have all been saved, so we can blow them away without worrying.
    # 
    
    lw      a0,g_pTheRadCrashRecordPs2  # a0 = g_pTheRadCrashRecordPs2.m_StackTrace
    nop
    addiu   a0,a0,m_StackTrace
    
    li      a1,RAD_CRASH_RECORD_PS2_STACK_TRACE_SIZE
    addu   a2,zero,sp
    addu   a3,zero,ra

    jal radStackTracePs2Get # ( g_pTheRadCrashRecordPs2.m_StackTrace, 
    nop                     #   RAD_CRASH_RECORD_PS2_STACK_TRACE_SIZE, sp, ra );

####
#
.radCrashHandlerPs2LauncherBegin:


    # Now that the state information is saved, some crazy things can 
    # happen.  We can blow away the old code that was causing the exception
    # and replace it with the radCrashHandlerPs2Launcher code.
    #

    # The radCrashHanndlerPs2Launcher code is two lines long. Load the
    # code into registers

    la      t1,radCrashHandlerPs2Launcher
    lw      t2,0(t1)                        # jal radCrashHandlerPs2WriteReport
    lw      t3,4(t1)                        # nop

    # Now write the two lines of code to the exception address.
    # The new instructions can be written through to memory, bypassing the d-cache
    # by setting the write-through bit as follows

    mfc0    t1,epc
    li      t3,WRITE_THROUGH_D_CACHE
    or      t1,t3,t1

    # Preceed the 2 lines of code with some nops for good luck

    sw      zero,0(t1)  # nop
    sw      zero,4(t1)  # nop
    sw      zero,8(t1)  # nop
    sw      zero,12(t1) # nop

    sw      t1,16(t1)   # jal radCrashHandlerPs2WriteReport
    sw      t2,20(t1)   # nop   

    # After blowing away the old instructions, invalidate those 6
    # lines in the i-cache

    mfc0    t1,epc
    nop

        # Notice that we are not flushing the data cache because
        # we already wrote through to the memory by set the
        # WRITE_THROUGH_D_CACHE bit.  We could have flushed 
        # the d-cache using the following instructions.  I know that
        # one day I will want to see this example again, so I am 
        # leaving it in.
        #
        # sync.l
        # cache   0x14,0(t1)
        # cache   0x14,4(t1)
        # cache   0x14,8(t1)
        # cache   0x14,12(t1)
        # cache   0x14,16(t1)
        # cache   0x14,20(t1)

    # Invalidate the i-cache as follows

    sync.p
    cache   0x0b,0(t1)  # cache IHIN (hit invalidate)
    cache   0x0b,4(t1)
    cache   0x0b,8(t1)
    cache   0x0b,12(t1)
    cache   0x0b,16(t1)
    cache   0x0b,20(t1)

    # Return from the exception handler to the offending address.
    # Only now, the program will jump and link to our radCrashHandler
    # function.
    #
    eret
    nop

    .set    at
    .set    reorder
    .size   radCrashHandlerPs2ExceptionHandler,.-radCrashHandlerPs2ExceptionHandler
    .end    radCrashHandlerPs2ExceptionHandler
                            

#=============================================================================
# radCrashHandlerPs2ExceptionHandler_V_TLB_REFILL( void )
#=============================================================================
#
# This provides a seperate entrypoint for v_tlb_refill exceptions
# that allows a return address to be specified
#

    .global     radCrashHandlerPs2ExceptionHandler_V_TLB_REFILL
    .ent        radCrashHandlerPs2ExceptionHandler_V_TLB_REFILL
    .set        noreorder
    .set        noat

radCrashHandlerPs2ExceptionHandler_V_TLB_REFILL:
    
    # The sony kernel only lets us blow away the contents
    # of k0.  The contents of other required registers
    # must be saved
    #
    la      k0,g_RadCrashHandlerPs2Temp 
    sq      k1,m_k1(k0)

    # Store the address of the kernel handler
    #
    li      k1,V_TLB_REFILL_RETURN_ADDRESS
    sw      k1,m_ReturnAddress(k0)     

    # Jump to our main exception handler code
    #
    la      k0,radCrashHandlerPs2ExceptionHandler
    jr      k0
    
    # Put in some filler nops to get the total 
    # number of actual lines of code up to 12
    #
    nop
    nop
    nop

    .set    at
    .set    reorder
    .size   radCrashHandlerPs2ExceptionHandler_V_TLB_REFILL,.-radCrashHandlerPs2ExceptionHandler_V_TLB_REFILL
    .end    radCrashHandlerPs2ExceptionHandler_V_TLB_REFILL

#=============================================================================
# radCrashHandlerPs2ExceptionHandler_V_COMMON( void )
#=============================================================================
#
# This provides a seperate entrypoint for v_common exceptions
# that allows a return address to be specified
#

    .global     radCrashHandlerPs2ExceptionHandler_V_COMMON
    .ent        radCrashHandlerPs2ExceptionHandler_V_COMMON
    .set        noreorder
    .set        noat

radCrashHandlerPs2ExceptionHandler_V_COMMON:
   
    # The sony kernel only lets us blow away the contents
    # of k0.  The contents of other required registers
    # must be saved
    #
    la      k0,g_RadCrashHandlerPs2Temp 
    sq      k1,m_k1(k0)

    # Store the address of the kernel handler
    #
    li      k1,V_COMMON_RETURN_ADDRESS
    sw      k1,m_ReturnAddress(k0)     

    # Jump to our main exception handler code
    #
    la      k0,radCrashHandlerPs2ExceptionHandler
    jr      k0
    
    # Put in some filler nops to get the total 
    # number of actual lines of code up to 12
    #
    nop
    nop
    nop

    .set    at
    .set    reorder
    .size   radCrashHandlerPs2ExceptionHandler_V_COMMON,.-radCrashHandlerPs2ExceptionHandler_V_COMMON
    .end    radCrashHandlerPs2ExceptionHandler_V_COMMON

