//=============================================================================
//
// File:        rad1394server.c
//
// Subsystem:	Radical Fire Wire
//
// Description:	This file contains the implementation of the 1394 I-Link (FireWire)
//              server. It provides a bridge between the IOP iLink IRX running
//              on the IOP and the EE. 
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	June 11, 2001
//
// Notes:       This file is intented to be built only for the PS2 IOP
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include <kernel.h>
#include <sys/types.h>
#include <stdio.h>
#include <sifrpc.h>
#include </usr/local/sce/common/include/ilink.h>
#include <memory.h>
#include "rad1394server.h"

//=============================================================================
// Local Definitions
//=============================================================================

//
// This is our Unit Spec Id and Unit Sw Version. These numbers must match 
// those of our NT device driver.
//
#define RadUnitSpecId       0x993346            // Not sure what these mean
#define RadUnitSwVersion    0x1A1122            // Just made some numbers up

//
// Used to indicate we do not know the node id of the master communicating with
// us.
//
#define INVALID_NODEID      (-1)

//
// Size we use for the payload.
//
#define PayLoadSize         (0x200 * 4)
#define PayLoadSizeMaster   (0x200)

//
// Priority of the thread used to service RPC requests.
//
#define RPCThreadPriority   ((( USER_HIGHEST_PRIORITY + USER_LOWEST_PRIORITY ) / 2 ) + 1) //10
        
//
// The following is the Unit directory. I do not completely understand it and arrived
// at its definition through alot of experimentation. There is more information that
// can follow this structure but I found this was enough to get things working.
//
struct UnitDirectory
{
    unsigned short  m_CRC;                  // Calculated by the unit add call.
    unsigned short  m_NumberOfQuadlets;     // Number of 32 bit words in table.
    unsigned char   m_UnitSpecID[ 3 ];      // Set to match of Win2k driver
    unsigned char   m_UnitSpecIDCode;       // Magic Number
    unsigned char   m_UnitSwVersion[ 3 ];   // Set to match of Win2k driver
    unsigned char   m_UnitSwVersionCode;    // Magic Number
};          

//=============================================================================
// Local Function Prototypes
//=============================================================================

static int ILinkReadCallback( unsigned int offsetLow, unsigned int size, unsigned char* payload, int pb);
static int ILinkWriteCallback( unsigned int offsetLow, unsigned int size, unsigned char* payload, int pb);
static int ILinkInitialize( unsigned int sharedMemoryRegionSize );
static void ILinkTerminate( void );
static int RPCInitialize( void );
static void RPCThread( void );
static void* RPCFunctionHandler( unsigned int functionNumber, void* pData, int size );
static void ReadSharedMemory( unsigned int address, unsigned int numBytes, unsigned char* ptr );
static void WriteSharedMemory( unsigned int address, unsigned int numBytes, unsigned char* ptr );
static void WriteToMaster( unsigned int address, unsigned int numBytes, unsigned char* ptr );

//=============================================================================
// Statics
//=============================================================================

//
// This is the Module Information Block. All IOP modules must have one of these.
//
ModuleInfo g_Module = { "Radical1394Server", 0x0100 };

//
// Unit directory. Built at runtime to prevent others from hacking like I did.
//
static struct UnitDirectory s_UnitDirectory;

//
// IDs of ILink resources.
//
static int s_UnitId;
static int s_DataIndId;

//
// This is used for issuing writes to the master. 
// We get the node id of the master when we receive a request, initialially set to 
// undefined.
//
static int s_TransactionContext;
static int s_MasterNodeId = INVALID_NODEID;

//
// Pointer to the shared memory used to communicate between the PS2 and the host
// over FireWire. Also maintained is the size of this block.
//
static unsigned char*   s_pSharedMemory = NULL;
static unsigned int     s_SharedMemorySize = 0;

//
// This static holds the address of where the read/write is to occur. We receive
// this is in a seperate transaction than the actual data.
//
static unsigned int     s_ReadWriteAddress;
static unsigned int     s_ReadWriteSize = 0;
static unsigned int     s_Atomic;
static unsigned int     s_LocalWrite;

//
// This static is used for transfering RPC requests. It must be big enough 
// for all of the various request sizes.
//
static unsigned char   s_RpcData[ RPCMaxReadWriteSize ];

//=============================================================================
// Public Functions
//=============================================================================

//=============================================================================
// Function:    start
//=============================================================================
// Description: This is the main entry point of the ilink server. This routine
//              is invoked by the ilink loader. Here initializaiton is performed.
//
// Parameters:  command line args
//              
// Returns:     RESIDENT_END - successful
//              NO_RESIDENT_END - failure
//
// Notes:       Currently this system does not have much in the way of termination
//              code. System does not seem to support IRX unloading so no point.
//------------------------------------------------------------------------------

int start( int argc, char* argv[ ] )
{
    //
    // Display a banner message.
    //
    printf( "Foundation Tech - 1394 FireWire Server  V2.00\n" );

    //
    // Flush the data cache and reenable interrupts. Note sure why but other IRXs do
    // this.
    //
    FlushDcache();
  
    CpuEnableIntr();
    
    //
    // Initialize the RPC component.
    //
    if( !RPCInitialize( ) )
    {
        printf( "Error initializing ILink RPC\n");
        return( NO_RESIDENT_END );
    }

    //
    // Inform the system that we are staying around.
    //
    return( RESIDENT_END );
}


//=============================================================================
// Function:    RPCInitialize
//=============================================================================
// Description: Initialize the RPC server part of this module.
//
// Parameters:  n/a
//              
// Returns:     0 - failure
//              1 - sucess
//------------------------------------------------------------------------------

int RPCInitialize( void )
{
    //
    // All we do is create a thread. This thread will be responsible for fielding
    // RPC requests from the EE.
    //
    struct ThreadParam threadParam;
    int                threadId;

    //
    // Set up the thread data
    //
    threadParam.attr = TH_C;
    threadParam.entry = RPCThread;
    threadParam.initPriority = RPCThreadPriority ;
    threadParam.stackSize = 0x500;
    threadParam.option = 0;

    threadId = CreateThread( &threadParam );

    if( threadId <= 0 )
    {
        return( 0 );
    }

    //
    // Start the thread and we are done.
    //
    StartThread( threadId, 0 );
    
    return( 1 );
}

//=============================================================================
// Function:    RPCThread
//=============================================================================
// Description: This routine is responsible for initializing the RPC server and
//              fielding rpc requests.
//
// Parameters:  n/a
//              
// Returns:     n/a
//
//------------------------------------------------------------------------------

void RPCThread( void )
{
    sceSifQueueData queueData;
    sceSifServeData serverData;
    
    //
    // Make sure the RPC system is initialized. 
    //
    sceSifInitRpc( 0 );

    //
    // Set up RPC queue.
    //
    sceSifSetRpcQueue( &queueData, GetThreadId( ) );

    //
    // Add our RPC function. We currently only use one function.
    //
    sceSifRegisterRpc( &serverData, rad1394FunctionId, RPCFunctionHandler, (void*)&s_RpcData, 0, 0, &queueData );
  
    //
    // Enter Loop which sleeps until an RPC request comes in. We never return from this function.
    //
    sceSifRpcLoop( &queueData );
}

//=============================================================================
// Function:    RPCFunctionHandler
//=============================================================================
// Description: This routine is invoked when an RPC request is issued on the EE.
//
// Parameters:  function number 
//              pointer to data
//              amount of data.
//              
// Returns:     pointer to data 
//
//------------------------------------------------------------------------------

void* RPCFunctionHandler( unsigned int functionNumber, void* pData, int size )
{
    //
    // Switch on the function.
    //
    switch( functionNumber )
    {
        case rad1394SetMemorySpace :
        {
            //
            // This function invoked to set the memory space. Check if we have memory
            // If so, free terminiate the ILink and free the memory. Then allocate memory
            // and re-initialize.
            //
            struct RPCGetSetMemorySize* pInfo = (struct RPCGetSetMemorySize*) pData;
    
            if( s_SharedMemorySize != 0 )
            {
                ILinkTerminate( );
                FreeSysMemory( s_pSharedMemory );
            }

            //
            // Save the size and allocate memory. Zero it to make things work easier
            // for the protocol.
            //
            s_SharedMemorySize = pInfo->m_Size;
            
            if( s_SharedMemorySize == 0 )
            {
                break;
            }

            s_pSharedMemory = (unsigned char*) AllocSysMemory( 0, s_SharedMemorySize, NULL );
            if( s_pSharedMemory == NULL )
            {
                printf( "Error allocating memory. Terminating.\n");
                while( 1 );
            }        

            memset( s_pSharedMemory, 0, s_SharedMemorySize );
            
            //
            // Initialize the Ilink;
            //
            if( !ILinkInitialize( s_SharedMemorySize ) )
            {
                printf( "Error initializing ILink driver\n");
            } 
            break;
        };
    
        case rad1394GetMemorySpace :
        {
            //
            // This function invoked to get the memory space. Just return current
            // size setting.
            //
            struct RPCGetSetMemorySize* pInfo = (struct RPCGetSetMemorySize*) pData;

            pInfo->m_Size = s_SharedMemorySize;

            break;
        }

        case rad1394ReadWriteInfo :
        {
            //
            // This function is invoked to provide us with the size and address of
            // the subsequent read/write that will occur. Just save the data.
            //
            struct RPCReadWriteInfo* pInfo = (struct RPCReadWriteInfo*) pData;
    
            s_ReadWriteAddress = pInfo->m_Address;
            s_ReadWriteSize = pInfo->m_Size;
            s_Atomic = pInfo->m_Atomic;
            s_LocalWrite = pInfo->m_LocalWrite;
                      
            break;
        }

        case rad1394ReadAsync :
        {
            //
            // Just copy the data to the pointer provided using the address and size received
            // earlier. Check if interrupts should be disabled during the operation.
            //
            if( s_Atomic )
            {
                CpuDisableIntr( );
            }

            ReadSharedMemory( s_ReadWriteAddress, s_ReadWriteSize, pData );

            if( s_Atomic )
            {
                CpuEnableIntr( );
            }

            break;
        }

        case rad1394WriteAsync :
        {
            //
            // Write the data using data provided earlier.
            //
            if( s_Atomic )
            {
                CpuDisableIntr( );
            }

            WriteSharedMemory( s_ReadWriteAddress, s_ReadWriteSize, pData );

            if( s_Atomic )
            {
                CpuEnableIntr( );
            }
            
            //
            // Send the data to the host if we can.
            //
            if( !s_LocalWrite )
            {
                WriteToMaster( s_ReadWriteAddress, s_ReadWriteSize, pData );
            }
            break;
        }

        default :
        {
            printf( "rad1394: Invalid function number received\n");
            break;
        }
    }

    //
    // Return the data. Not always needed by the client.
    //
    return( pData );
}

//=============================================================================
// Function:    ReadSharedMemory
//=============================================================================
// Description: This routine is a simple accessor to the shared memory.
//
// Parameters:  address - where in shared memory to read from.
//              numBytes - number of bytes to read.
//              ptr      - where to copy data to
//              
// Returns:     n/a
//
//------------------------------------------------------------------------------

void ReadSharedMemory( unsigned int address, unsigned int numBytes, unsigned char* ptr )
{
    //
    // Check if the parameters make sense.
    //
    if( (address < s_SharedMemorySize ) && ((address + numBytes) <= s_SharedMemorySize ) )
    {
        //
        // Simply perform a memcpy of the requested size.
        //
        memcpy( ptr, s_pSharedMemory + address, numBytes );
    }
    else
    {
        printf( "ILink Server: Invalid read occurred\n");
    }
}

//=============================================================================
// Function:    WriteSharedMemory
//=============================================================================
// Description: This routine is a simple write accessor to the shared memory.
//
// Parameters:  address - where in shared memory to write to
//              numBytes - number of bytes to write.
//              ptr      - where to copy data from
//              
// Returns:     n/a
//
//------------------------------------------------------------------------------

void WriteSharedMemory( unsigned int address, unsigned int numBytes, unsigned char* ptr )
{
    //
    // Check if the parameters make sense.
    //
    if( (address < s_SharedMemorySize ) && ((address + numBytes) <= s_SharedMemorySize ) )
    {
        //
        // Simply perform a memcpy of the requested size.
        //
        memcpy( s_pSharedMemory + address, ptr, numBytes );
    }
    else
    {
        printf( "ILink Server: Invalid write occurred\n");
    }
}

//=============================================================================
// Function:    ILinkInitialize
//=============================================================================
// Description: This routine performs the Ilink initialation.
//
// Parameters:  size of shared memory region
//              
// Returns:     0 - failure
//              1 - success
//
//------------------------------------------------------------------------------

int ILinkInitialize( unsigned int sharedMemoryRegionSize )
{
    int EUI64[ 2 ] = { 0, 0 };
    int result;
    int readReplyPacket;
    int writeReplyPacket;

    //
    // Start the initialization sequence. Much of this was arrived at through experimentation.
    // as documentation sucked and there were no examples at time of writing.
    //
    sce1394Initialize( NULL );

    //
    // Get the GUID of this device
    //
    result = sce1394SbEui64( EUI64 );
    if( result == SCE1394ERR_OK )
    {
        printf( "ILink GUID = [0x%08x 0x%08x]\n",EUI64[ 0 ], EUI64[ 1 ] );
    }
    else
    {
        return( 0 );
    }

    //
    // This set is critical. Must set the asycn payloadsl.
    //
    sce1394ConfSet( SCE1394CF_TRSIZE_MASTER, PayLoadSizeMaster );
    sce1394ConfSet( SCE1394CF_TRSIZE_SLAVE,  PayLoadSize );

    //
    // Increasing the priority appears to improve performance. Disable this since 
    // changed data transfer mechanisms.
    //
    //    sce1394ConfSet( SCE1394CF_PRIORITY_HI, 9 );
    //    sce1394ConfSet( SCE1394CF_PRIORITY_LO, 10 );

    //
    // Now lets add our unit directory. The content is rather magical but this seems to work.
    // This must be done to cause WinNT to detect and load our NT device driver.
    //
    s_UnitDirectory.m_CRC = 0;                              // Calculated by the add call.
    s_UnitDirectory.m_NumberOfQuadlets = 2;                 // Number of 32 bit words in table
    s_UnitDirectory.m_UnitSpecIDCode = 0x12;                // Magic number
    s_UnitDirectory.m_UnitSpecID[ 0 ] = (unsigned char) (RadUnitSpecId & 0x0000ff); 
    s_UnitDirectory.m_UnitSpecID[ 1 ] = (unsigned char) ((RadUnitSpecId & 0x00ff00) >> 8);
    s_UnitDirectory.m_UnitSpecID[ 2 ] = (unsigned char) ((RadUnitSpecId & 0xff0000) >> 16);
    s_UnitDirectory.m_UnitSwVersionCode = 0x13;
    s_UnitDirectory.m_UnitSwVersion[ 0 ] = (unsigned char) (RadUnitSwVersion & 0x0000ff); 
    s_UnitDirectory.m_UnitSwVersion[ 1 ] = (unsigned char) ((RadUnitSwVersion & 0x00ff00) >> 8 ); 
    s_UnitDirectory.m_UnitSwVersion[ 2 ] = (unsigned char) ((RadUnitSwVersion & 0xff0000) >> 16); 

    //
    // Add the unit directory.
    //
    s_UnitId = sce1394UnitAdd( sizeof( s_UnitDirectory ) / 4, (unsigned int*) &s_UnitDirectory );    

    //
    // Let. Now lets add our asychronous data handler. We only respond to write requests. We
    // don't care about lock requests. We respond to address range 0:0 though 0:size of memory. 
    //
    s_DataIndId = sce1394TrDataInd( 0, 0, sharedMemoryRegionSize, NULL, ILinkWriteCallback, NULL, ILinkReadCallback, NULL, NULL );

    //
    // Lets allocate a transaction context. Used to perform our write requests. We don't know the address
    // of the master yet so just set the node id to zero.
    //
    s_TransactionContext = sce1394TrAlloc( 0xffff, 0 );

    if( s_TransactionContext < 0 )
    {
        return( 0 );
    }

    //
    // Finally lets enable the system.
    //
    sce1394SbEnable( );
    sce1394SbReset( 1 );

    return( 1 );
}

//=============================================================================
// Function:    ILinkTerminate
//=============================================================================
// Description: This routine performs the Ilink termination
//
// Parameters:  n/a
//              
// Returns:     n/a
//------------------------------------------------------------------------------

void ILinkTerminate( void )
{
    //
    // Just free up Ilink stuff
    //
    sce1394SbDisable( 1 );

    sce1394TrFree( s_TransactionContext );

    sce1394TrDataUnInd( s_DataIndId );

    sce1394UnitDelete( s_UnitId );
    
    sce1394Destroy( );
}

//=============================================================================
// Function:    ILinkReadCallback
//=============================================================================
// Description: This function is invoked by the ILink IRX when a read request
//              has arrived.
//
// Parameters:  offsetLow - memory address
//              size      - number of bytes
//              payload   - where to place data
//              pb        - packet block
//              
//              
// Returns:     status
//
// Notes:       Most of the code in here was arrived at through experimentation.
//              Note sure of all details but it appears to work.
//------------------------------------------------------------------------------

int ILinkReadCallback( unsigned int offsetLow, unsigned int size, unsigned char* payload, int pb)
{
    //
    // To keep reads atomic, disable interrupts if we write to addess zero. This address
    // is were are control block is and is the only thing that needs protection.
    //
    if( offsetLow == 0 )
    {
        CpuDisableIntr( );
    }

    ReadSharedMemory( offsetLow, size, payload );

    if( offsetLow == 0 )
    {
       CpuEnableIntr( );
    }

    return( 0 );
}

//=============================================================================
// Function:    ILinkWriteCallback
//=============================================================================
// Description: This function is invoked by the ILink IRX when a write request
//              has arrived.
//
// Parameters:  offsetLow - memory address
//              size      - number of bytes
//              payload   - where to get data
//              pb        - packet block
//              
//              
// Returns:     status
//
// Notes:       Most of the code in here was arrived at through experimentation.
//              Note sure of all details but it appears to work.
//------------------------------------------------------------------------------

int ILinkWriteCallback( unsigned int offsetLow, unsigned int size, unsigned char* payload, int pb)
{
    //
    // Save the node id the master.
    //
    s_MasterNodeId = sce1394PbGet(pb, SCE1394PB_SOURCE );         

    //
    // To keep writes atomic, disable interrupts if we write to addess zero. This address
    // is were are control block is and is the only thing that needs protection.
    //
    if( offsetLow == 0 )
    {
        CpuDisableIntr( );
    }

    WriteSharedMemory( offsetLow, size, payload );

    if( offsetLow == 0 )
    {
        CpuEnableIntr( );
    }

    return( 0 );
}

//=============================================================================
// Function:    WriteToMaster
//=============================================================================
// Description: This function is invoked to write data to the master. 
//
// Parameters:  offset    - offset into masters shared memory
//              size      - number of bytes
//              pData     - data to write
//              
// Returns:     n/a
//
// Notes:       If we have not received the master node id, then just disregard.
//              Higher level protocols will deal with error detection.
//------------------------------------------------------------------------------

void WriteToMaster( unsigned int offset, unsigned int size, unsigned char* pData )
{
    int          bytesSent;
    unsigned int currentRetry;

    //
    // Check if we have received the node id of the master. If not, we don't
    // know who we are talking to.
    //
    if( s_MasterNodeId == INVALID_NODEID )
    {
        return;
    }

    //
    // Set up the transaction context.
    //
    sce1394TrSetDest( s_TransactionContext, s_MasterNodeId );
    sce1394TrSetBlockSize( s_TransactionContext, PayLoadSizeMaster );
    sce1394TrSetGenNumber( s_TransactionContext, sce1394SbGenNumber( ) );
    sce1394TrSetSpeed( s_TransactionContext, SCE1394_SPEED_100M );

    //
    // Issue the write request. Sometimes, for some unknown reason, the operation fails.
    // By retrying, things work. Do it for a limited time.
    //
    currentRetry = 0;
    while( size > 0 )
    {
        //
        // Sorry about the goto.
        //  
        retry:
            bytesSent = sce1394TrWrite( s_TransactionContext, 1, offset, size, pData );
            if( bytesSent < 0 )
            {
                //
                // Here there is a retry. Fail if limit exceeded.
                //
                if( currentRetry < 50 )
                {
                    goto retry;
                }
                      
                //
                // Output message and indicate things did not work.
                //
                printf( "ILink transmit retry limit exceeded\n" );
                          
                break;   
            }
            else
            {
                //
                // Update size, pointer and offset with the amount sent.
                //
                size = size - bytesSent;
                pData = pData + bytesSent;
                offset = offset + bytesSent;
            }
    }
}

