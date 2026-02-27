//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        memorymonitorprotocol.hpp
//
// Subsystem:   Radical memory monitor server - Host Communication Protocol
//
// Description: This file contains the definitions of the packets used for
//              communication between the host and the target.
//
// Revisions:   09-Nov-2001      Creation   James Tan
//
//=============================================================================

#ifndef MEMORYMONITORPROTOCOL_HPP
#define MEMORYMONITORPROTOCOL_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radmemorymonitor.hpp>

//=============================================================================
// Defintions
//=============================================================================

//
// This is the protocol number we use for connect with the host via the IOP
//
#define MEM_MONITOR_PROTOCOL    0xE123 + 8

//
// This is the size of the normal reciever buffer
//
#define MM_MAX_RX_BUFFER        5 * 1024

//
// largest block memory content can be request at a time
//
#define MM_MAX_MEMORY_BLOCK_SIZE    128

//
// Max length of name
// 
#define MM_MAX_NAMELENGTH           64

//
// initial connection timeout
//
#define MM_CONNECTION_TIME_OUT      15000

//
// Enum used to identify client platform
//
enum MM_ClientPlatform
{
    MM_Platform_Unknown = 0,
    MM_Platform_WIN = 4 // UWP also can be this
};

//
// enum used to identify client memory space
//
enum MM_ClientMemorySpace
{
    MM_UNKNOWN,
    MM_WIN_MAIN = 4 // UWP also can be this
};

//
// The following enumeration is used to identify type of data sends through
// wire.
//
//******************IMPORTANT****************
// Note: if new protocal is added
// Update radMemoryMonitorGetPacketSize( ) in memmonitorclient.cpp
//
enum MM_DataHeader
{
    DummyType,                                      // for dummy data sending if ever needed, debug purpose

    DeclarePlatformType,                            // what the client platform is

    DeclareSpaceDataType,                           // 

    DeclareSectionType,                             // See DeclareSection()
    RescindSectionType,                             // See RescindSection()
    IdentifySectionType,                            // See IdentifySection()

    DeclareAllocationType,                          // See DeclareAllocation()
    RescindAllocationType,                          // See RescindAllocation()
    IdentifyAllocationType,                         // See IdentifyAllocation()

    ReportAddRefType,                               // See ReportAddRef()
    ReportReleaseType,                              // See ReportRelease()

    IssueFlagType,                                  // See IssueFlag()

    MemoryBlockType,                                // Sending raw memory block

    RequestSuspendType,                             // request a suspend of game
    RequestResumeType,                              // request a resume of game

    RequestRefCountType,                            // request a reference count for an object

    ReportRefCountType,                             // report a reference count for an object

    RequestMemoryTableType,                         // request a entire memory table include latest section and allocation under section

    RequestMemoryBlockType,                         // request a memory block

    RequestSuspendStateType,                        // request suspended state, ack is ReportSuspendStateType

    ReportSuspendStateType,                         // is client suspended or not, send after a successful RequestSuspendType/RequestResumeType

    RequestStackUsageType,                          // request where the current stack pointer is

    ReportStackUsageType,                           // report where is the current stack pointer is

    AcknowledgementType                            // ack a send, don't ack back an ack :)
};

//
// data packet has header information
//
struct MM_DataPacket
{
    MM_DataHeader       header;                     // type of data to send over
};

//
// time stamped packet
//
struct MM_DataPacketTimeStamped :
    public MM_DataPacket
{
    unsigned int        eventID;                    // incremeneted 1 for each event
    unsigned int        timeStamp;                  // some type of time index to keep track, in ms
};

//
// requesting reference count data
//
struct MM_RequestRefCountData :
    public MM_DataPacket
{
    MM_ClientMemorySpace    memorySpace;            // memory space of the memory
    void*                   nObjectPtr;             // location to an object
    unsigned int*           nRefCountPtr;
};

//
// reporting reference count data
//
struct MM_ReportRefCountData :
    public MM_DataPacket
{
    MM_ClientMemorySpace    memorySpace;            // memory space of the memory
    void*                   nObjectPtr;             // location to an object
    unsigned int*           nRefCountPtr;
    unsigned int            nRefCount;              // ref count value
};

//
// suspend the operation of the game
//
struct MM_RequestSuspendedData :
    public MM_DataPacket
{
};

//
// resume the operation of the game
//
struct MM_RequestResumedData :
    public MM_DataPacket
{
};

//
// the following struct is used for host to request memory block
//
struct MM_RequestMemoryBlockData :
    public MM_DataPacket
{
    MM_ClientMemorySpace      memorySpace;          // memory space of the memory
    uintptr_t           memStartPos;                // starting memory address
    unsigned int        memLength;                  // length of memory address, must less than MM_MAX_MEMORY_BLOCK_SIZE
};

//
// the following struct is used for host to request memory block
//
struct MM_SendMemoryBlockData :
    public MM_DataPacket
{
    MM_ClientMemorySpace      memorySpace;          // memory space of the memory
    uintptr_t           memStartPos;                // starting memory address
    unsigned int        memLength;                  // length of memory address
    char                memBlock[ MM_MAX_MEMORY_BLOCK_SIZE ];   // memory block
};

//
// the following struct is used for ack a data sending
//
struct MM_Acknowledgement :
    public MM_DataPacket
{
    unsigned int        userData;
};

//
// the following struct is used for report if client is suspended or not
//
struct MM_ReportSuspend :
    public MM_DataPacket
{
    bool                isSuspended;
};

//
// the following struct is used for request a stack usage
//
struct MM_RequestStackUsageData :
    public MM_DataPacket
{
    MM_ClientMemorySpace      memorySpace;          // memory space of the memory
    uintptr_t           address;                    // start address of the stack
};

//
// the following struct is used for report a stack usage
//
struct MM_ReportStackUsageData :
    public MM_DataPacket
{
    MM_ClientMemorySpace      memorySpace;          // memory space of the memory
    uintptr_t           address;                    // start address of the stack
    unsigned int        highWaterMark;              // current stack High water mark,
                                                    // if highWaterMark == -1,
                                                    // then there's an error while determine stack usage
};

//
// the following struct is used for report client platform
//
struct MM_DeclarePlatformData :
    public MM_DataPacketTimeStamped
{
    MM_ClientPlatform   platform;
    uintptr_t           userData;                   // used to store address of main( ) in XBox
};

//
// the following struct is used for store data when a space is declared
//
struct MM_DeclareSpaceData :
    public MM_DataPacketTimeStamped
{
    uintptr_t           addrStart;
    unsigned int        size;                       // size of section
    MM_ClientMemorySpace      memorySpace;          // memory space type
};

//
// the following struct is used for store data when a section is declared
//
struct MM_DeclareSectionData :
    public MM_DataPacketTimeStamped
{
    uintptr_t           address;                    // address of section
    unsigned int        size;                       // size of section
    IRadMemoryMonitor::MemorySectionType sectionType;        // section type
    MM_ClientMemorySpace      memorySpace;          // memory space type
};

//
// the following struct is used for store data when a section needs to be identified
//
struct MM_IdenitfySectionData :
    public MM_DataPacketTimeStamped
{
    uintptr_t           address;                    // address of section
    MM_ClientMemorySpace      memorySpace;          // memory space type
    char                name[MM_MAX_NAMELENGTH];    // name of the section
};

//
// the following struct is used for store data when a section data needs
// to be removed.
//
struct MM_RescindSectionData :
    public MM_DataPacketTimeStamped
{
    uintptr_t           address;                    // address of section
    MM_ClientMemorySpace      memorySpace;          // memory space type
};

//
// the following struct is used for store data when an allocation is declared
//
struct MM_DeclareAllocationData :
    public MM_DataPacketTimeStamped
{
    uintptr_t           address;                    // address of allocation see DeclareAllocation()
    unsigned int        size;                       // size of allocation see DeclareAllocation()
    MM_ClientMemorySpace      memorySpace;          // memory space used by allocation

    unsigned int        callStackDepth;             // call stack depth
    uintptr_t           callStack[MM_MAX_CALLSTACK_DEPTH];
};

//
// the following struct is used for store data when a allocation data needs
// to be removed.
//
struct MM_RescindAllocationData :
    public MM_DataPacketTimeStamped
{
    uintptr_t           address;                    // address of section
    MM_ClientMemorySpace      memorySpace;          // memory space type
};

//
// the following struct is used for store data when a allocation data
// needs to be identified
//
struct MM_IdentifyAllocationData :
    public MM_DataPacketTimeStamped
{
    uintptr_t           address;                    // address of allocation see DeclareAllocation()
    MM_ClientMemorySpace      memorySpace;          // memory space used by allocation
    char                group[MM_MAX_NAMELENGTH/4]; // group id see DeclareAllocation()
    char                name[MM_MAX_NAMELENGTH];    // name of the allocation
    unsigned int        referenceCount;             // inital reference count
    unsigned int*       referenceCountPtr;          // reference counter pointer used by allocation
};

//
// the following struct is used for store issued flag
//
struct MM_IssueFlagData :
    public MM_DataPacketTimeStamped
{
    char                name[MM_MAX_NAMELENGTH];
};

//
// the following struct is used for store addref relation
//
struct MM_ReportAddRefData :
    public MM_DataPacketTimeStamped
{
    MM_ClientMemorySpace    memorySpaceObject;   // memory space type
    uintptr_t               object;
    MM_ClientMemorySpace    memorySpaceRefObject;// memory space type
    uintptr_t               refObject;
    unsigned int            callStackDepth;             // call stack depth
    uintptr_t               callStack[MM_MAX_CALLSTACK_DEPTH];
};

//
// the following struct is used for store release relation
//
struct MM_ReportReleaseData :
    public MM_DataPacketTimeStamped
{
    MM_ClientMemorySpace    memorySpaceObject;   // memory space type
    uintptr_t               object;
    MM_ClientMemorySpace    memorySpaceRefObject;// memory space type
    uintptr_t               refObject;
    unsigned int            callStackDepth;             // call stack depth
    uintptr_t               callStack[MM_MAX_CALLSTACK_DEPTH];
};

union MM_DataPacketBlock
{
    MM_DataPacket               commandPacket;
    MM_DataPacketTimeStamped    timeStampedData;

    MM_ReportRefCountData       reportRefCountData;

    MM_RequestSuspendedData     requestSuspendedData;
    MM_RequestResumedData       requestResumedData;
    MM_RequestMemoryBlockData   requestMemoryBlockData;
    MM_SendMemoryBlockData      sendMemoryBlockData;

    MM_Acknowledgement          acknowledgementData;
    MM_ReportSuspend            reportSuspend;

    MM_DeclarePlatformData      declarePlatform;
    MM_DeclareSpaceData         declareSpaceData;
    MM_DeclareSectionData       declareSectionData;
    MM_RescindSectionData       rescindSectionData;
    MM_IdenitfySectionData      identifySectionData;
    MM_DeclareAllocationData    declareAllocationData;
    MM_RescindAllocationData    rescindAllocationData;
    MM_IdentifyAllocationData   identifyAllocationData;
    MM_ReportAddRefData         reportAddRefData;
    MM_ReportReleaseData        reportReleaseData;

    MM_IssueFlagData            issueFlagData;

    MM_SendMemoryBlockData      sendMemBlockData;

    MM_RequestStackUsageData    requestStackUsageData;
    MM_ReportStackUsageData     reportStackUsageData;

    MM_RequestRefCountData      requestRefCountData;
};

union MM_DataStorable
{
    MM_DataPacketTimeStamped    baseClassData;

    MM_IssueFlagData            issueFlagData;

    MM_DeclarePlatformData      declarePlatform;
    MM_DeclareSpaceData         declareSpaceData;
    MM_DeclareSectionData       declareSectionData;
    MM_RescindSectionData       rescindSectionData;
    MM_IdenitfySectionData      identifySectionData;
    MM_DeclareAllocationData    declareAllocationData;
    MM_RescindAllocationData    rescindAllocationData;
    MM_IdentifyAllocationData   identifyAllocationData;
    MM_ReportAddRefData         reportAddRefData;
    MM_ReportReleaseData        reportReleaseData;
};

//
// global function
//
unsigned int radMemoryMonitorGetPacketSize( const MM_DataHeader header );

#endif // MEMORYMONITORPROTOCOL_HPP