//=============================================================================
//
// File:        rad1394qpnp.c
//
// Subsystem:	Radical Fire Wire Win32 Driver
//
// Description:	This file contains the most of the routines for the driver. They were
//              borrowed directly from NTDDK examples. Sorry for lack of documentation,
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	July 12, 2001
//
// Notes:       
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include <basetyps.h>
#include <wdm.h>
#include <1394.h>
#include "rad1394driver.h"
#include "rad1394qpnp.h"
#include "rad1394power.h"

//=============================================================================
// Local Definitions
//=============================================================================

NTSTATUS rad1394_Create( IN PDEVICE_OBJECT DriverObject, IN PIRP  Irp );
NTSTATUS rad1394_Close( IN PDEVICE_OBJECT DriverObject, IN PIRP Irp);
NTSTATUS rad1394_SynchCompletionRoutine( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PKEVENT Event );
NTSTATUS rad1394_IoControl( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );
NTSTATUS rad1394_Get1394AddressFromDeviceObject( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
                                                 IN ULONG fulFlags, OUT PNODE_ADDRESS pNodeAddress );
NTSTATUS rad1394_AsyncRead( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN ULONG bRawMode, IN ULONG bGetGeneration,
                            IN IO_ADDRESS DestinationAddress, IN ULONG nNumberOfBytesToRead, IN ULONG nBlockSize,
                            IN ULONG fulFlags, IN ULONG ulGeneration, IN ULONG localRead, IN OUT PULONG Data );
NTSTATUS rad1394_AsyncWrite( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN ULONG bRawMode, IN ULONG bGetGeneration,
                             IN IO_ADDRESS DestinationAddress, IN ULONG nNumberOfBytesToWrite, IN ULONG nBlockSize,
                             IN ULONG fulFlags, IN ULONG ulGeneration, IN OUT PULONG Data );
NTSTATUS rad1394_SetAddressRangeSize( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
                                      IN ULONG fulFlags, IN ULONG size, IN OUT PULONG Data );
NTSTATUS rad1394_FreeAddressRange( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );

//=============================================================================
// Public Functions
//=============================================================================

NTSTATUS DriverEntry
(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
)
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;

    DriverObject->MajorFunction[IRP_MJ_CREATE]          = rad1394_Create;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = rad1394_Close;
    DriverObject->MajorFunction[IRP_MJ_PNP]             = rad1394_Pnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]           = rad1394_Power;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = rad1394_IoControl;
    DriverObject->DriverExtension->AddDevice            = rad1394_PnpAddDevice;

    return(ntStatus);
}

NTSTATUS rad1394_Create
(
    IN PDEVICE_OBJECT   DriverObject,
    IN PIRP             Irp
)
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;

    PDEVICE_EXTENSION   deviceExtension = DriverObject->DeviceExtension;

    //
    // Count number of opens so we care free shared memory.
    //
    deviceExtension->OpenCount++;

    return(ntStatus);
}

NTSTATUS rad1394_Close
(
    IN PDEVICE_OBJECT   DriverObject,
    IN PIRP             Irp
)
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;

    PDEVICE_EXTENSION   deviceExtension = DriverObject->DeviceExtension;

    deviceExtension->OpenCount--;

    if( deviceExtension->OpenCount == 0 )
    {
        ntStatus = rad1394_FreeAddressRange( DriverObject, Irp );
    }

    return(ntStatus);
}

NTSTATUS rad1394_SubmitIrpSynch
(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIRB                 Irb
)
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    KEVENT              Event;
    PIO_STACK_LOCATION  NextIrpStack;

    if (Irb)
    {

        NextIrpStack = IoGetNextIrpStackLocation(Irp);
        NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
        NextIrpStack->Parameters.Others.Argument1 = Irb;
    }
    else 
    {

        IoCopyCurrentIrpStackLocationToNext(Irp);
    }

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    IoSetCompletionRoutine( Irp,
                            rad1394_SynchCompletionRoutine,
                            &Event,
                            TRUE,
                            TRUE,
                            TRUE
                            );

    ntStatus = IoCallDriver(DeviceObject, Irp);

    if (ntStatus == STATUS_PENDING)
    {

        KeWaitForSingleObject( &Event,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL
                               );

    }

    ntStatus = Irp->IoStatus.Status;

    return(ntStatus);
} 

NTSTATUS rad1394_SynchCompletionRoutine
(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PKEVENT          Event
)
{
    NTSTATUS        ntStatus = STATUS_SUCCESS;

    if (Event)
    {
        KeSetEvent(Event, 0, FALSE);
    }
    return(STATUS_MORE_PROCESSING_REQUIRED);
} 

NTSTATUS rad1394_SubmitIrpAsync
(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIRB                 Irb
)
{
    PIO_STACK_LOCATION  NextIrpStack;
    NTSTATUS            ntStatus = STATUS_SUCCESS;

    if( Irb )
    {

        NextIrpStack = IoGetNextIrpStackLocation(Irp);
        NextIrpStack->Parameters.Others.Argument1 = Irb;
    }
    else
    {

        IoCopyCurrentIrpStackLocationToNext(Irp);
    }

    ntStatus = IoCallDriver (DeviceObject, Irp);
    return ntStatus;
}

NTSTATUS rad1394_IoControl
( 
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
)
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION      IrpSp;
    PDEVICE_EXTENSION       deviceExtension;
    PVOID                   ioBuffer;
    ULONG                   inputBufferLength;
    ULONG                   outputBufferLength;
    ULONG                   ioControlCode;
    

    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    // Get a pointer to the device extension
    deviceExtension = DeviceObject->DeviceExtension;

    // Get the pointer to the input/output buffer and it's length
    ioBuffer		   = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    // make sure our device isn't in shutdown mode...
    if (deviceExtension->bShutdown)
    {

        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(ntStatus);
    }

    switch (IrpSp->MajorFunction)
    {
        case IRP_MJ_DEVICE_CONTROL:
        {
            ioControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;

            switch (ioControlCode)
            {
                case RAD1394_IOCTL_READ:
                {
                    ASYNC_READ*     AsyncRead;

                    if( inputBufferLength < (sizeof(ASYNC_READ) - RAD1394_MAX_READ_SIZE) ) 
                    {
                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                    }
                    else
                    {
                        AsyncRead = (ASYNC_READ*)ioBuffer;

                        if ((outputBufferLength < (sizeof(ASYNC_READ) - RAD1394_MAX_READ_SIZE)) || 
                            (outputBufferLength-(sizeof(ASYNC_READ) - RAD1394_MAX_READ_SIZE) < AsyncRead->nNumberOfBytesToRead))
                        {
                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else
                        {
                            ntStatus = rad1394_AsyncRead( DeviceObject,
                                                            Irp,
                                                            AsyncRead->bRawMode,
                                                            AsyncRead->bGetGeneration,
                                                            AsyncRead->DestinationAddress,
                                                            AsyncRead->nNumberOfBytesToRead,
                                                            AsyncRead->nBlockSize,
                                                            AsyncRead->fulFlags,
                                                            AsyncRead->ulGeneration,
                                                            AsyncRead->bLocalRead,
                                                            (PULONG)&AsyncRead->Data
                                                            );

                            if( NT_SUCCESS(ntStatus) )
                            {    
                                Irp->IoStatus.Information = outputBufferLength;
                            }
                        }
                    }
                    break; 
                }
    
                case RAD1394_IOCTL_WRITE:
                {
                    ASYNC_WRITE*    AsyncWrite;

                    if (inputBufferLength < (sizeof(ASYNC_WRITE) - RAD1394_MAX_WRITE_SIZE) )
                    {
                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                    }
                    else
                    {
                        AsyncWrite = (ASYNC_WRITE*)ioBuffer;

                        if (inputBufferLength-(sizeof(ASYNC_WRITE)-RAD1394_MAX_WRITE_SIZE) < AsyncWrite->nNumberOfBytesToWrite)
                        {
                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else 
                        {
                            ntStatus = rad1394_AsyncWrite( DeviceObject,
                                                             Irp,
                                                             AsyncWrite->bRawMode,
                                                             AsyncWrite->bGetGeneration,
                                                             AsyncWrite->DestinationAddress,
                                                             AsyncWrite->nNumberOfBytesToWrite,
                                                             AsyncWrite->nBlockSize,
                                                             AsyncWrite->fulFlags,
                                                             AsyncWrite->ulGeneration,
                                                             (PULONG)&AsyncWrite->Data
                                                             );
                        }
                    }
                    break;
                }

                case RAD1394_IOCTL_GET_ADDRESS_FROM_DEVICE:
                {
                    GET_1394_ADDRESS*   Get1394Address;

                    if ((inputBufferLength < sizeof(GET_1394_ADDRESS)) ||
                        (outputBufferLength < sizeof(GET_1394_ADDRESS)))
                    {
                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                    }
                    else
                    {
                        Get1394Address = (GET_1394_ADDRESS*)ioBuffer;

                        ntStatus = rad1394_Get1394AddressFromDeviceObject( DeviceObject,
                                                                             Irp,
                                                                             Get1394Address->fulFlags,
                                                                             &Get1394Address->NodeAddress
                                                                             );

                        if (NT_SUCCESS(ntStatus))
                        {
                            Irp->IoStatus.Information = outputBufferLength;
                        }
                    }
                    break;
                }

                case RAD1394_IOCTL_SET_ADDRESS_RANGE_SIZE:
                {
                    SET_1394_ADDRESS_RANGE_SIZE*   Set1394AddressRangeSize;

                    if ((inputBufferLength < sizeof(SET_1394_ADDRESS_RANGE_SIZE)) ||
                        (outputBufferLength < sizeof(SET_1394_ADDRESS_RANGE_SIZE)))
                    {
                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                    }
                    else
                    {
                        Set1394AddressRangeSize = (SET_1394_ADDRESS_RANGE_SIZE*)ioBuffer;

                        ntStatus = rad1394_SetAddressRangeSize( DeviceObject,
                                                                Irp,
                                                                Set1394AddressRangeSize->fulFlags,
                                                                Set1394AddressRangeSize->Size,
                                                                (PULONG) &Set1394AddressRangeSize->Data );

                        if (NT_SUCCESS(ntStatus))
                        {
                            Irp->IoStatus.Information = outputBufferLength;
                        }
                    }
                    break;
                }

                default:
                {
                    ntStatus = STATUS_INVALID_PARAMETER;
                    break; 
                }

            } 

            break;
        }

        default:
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            break;
        }
    } 

    // only complete if the device is there
    if(ntStatus != STATUS_NO_SUCH_DEVICE)
    {
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return(ntStatus);
} 


NTSTATUS rad1394_AsyncRead
(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            bRawMode,
    IN ULONG            bGetGeneration,
    IN IO_ADDRESS       DestinationAddress,
    IN ULONG            nNumberOfBytesToRead,
    IN ULONG            nBlockSize,
    IN ULONG            fulFlags,
    IN ULONG            ulGeneration,
    IN ULONG            bLocalRead,
    IN OUT PULONG       Data
)
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIRB                pIrb;
    PMDL                pMdl;
    PDEVICE_OBJECT      NextDeviceObject;
    PIRP                newIrp;
    BOOLEAN             allocNewIrp = FALSE;
    KEVENT              Event;
    IO_STATUS_BLOCK     ioStatus;

    if( (deviceExtension->AddressRangeData != NULL) && bLocalRead )
    {
        void* srcAddress = (unsigned char*)(deviceExtension->AddressRangeData->Buffer) + DestinationAddress.IA_Destination_Offset.Off_Low;
        RtlCopyMemory( Data, srcAddress, nNumberOfBytesToRead ) ;
        return( ntStatus );
    }

    //
    // get the location of the next device object in the stack
    //
    if (bRawMode) {
        NextDeviceObject = deviceExtension->PortDeviceObject;
    }
    else {
        NextDeviceObject = deviceExtension->StackDeviceObject;
    }

    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, NextDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp)
        {

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_AsyncRead;            
        }
        allocNewIrp = TRUE;
    }

    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb)
    {

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_AsyncRead;
    }

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_ASYNC_READ;
    pIrb->Flags = 0;
    pIrb->u.AsyncRead.DestinationAddress = DestinationAddress;
    pIrb->u.AsyncRead.nNumberOfBytesToRead = nNumberOfBytesToRead;
    pIrb->u.AsyncRead.nBlockSize = nBlockSize;
    pIrb->u.AsyncRead.fulFlags = fulFlags;

    if (bGetGeneration)
    {
        pIrb->u.AsyncRead.ulGeneration = deviceExtension->GenerationCount;
    }
    else 
    {
    
        pIrb->u.AsyncRead.ulGeneration = ulGeneration;
    }

    pMdl = IoAllocateMdl (Data, 
                          nNumberOfBytesToRead,
                          FALSE,
                          FALSE,
                          NULL);
    
    MmBuildMdlForNonPagedPool(pMdl);
    
    pIrb->u.AsyncRead.Mdl = pMdl;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) 
    {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = rad1394_SubmitIrpAsync (NextDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
    
        ntStatus = rad1394_SubmitIrpSynch(NextDeviceObject, Irp, pIrb);
        
    }

    if (pMdl)
        ExFreePool(pMdl);

    if (pIrb)
        ExFreePool(pIrb);

Exit_AsyncRead:

    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;
    return(ntStatus);
}


NTSTATUS rad1394_AsyncWrite
(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            bRawMode,
    IN ULONG            bGetGeneration,
    IN IO_ADDRESS       DestinationAddress,
    IN ULONG            nNumberOfBytesToWrite,
    IN ULONG            nBlockSize,
    IN ULONG            fulFlags,
    IN ULONG            ulGeneration,
    IN OUT PULONG       Data
)
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIRB                pIrb;
    PMDL                pMdl;

    PDEVICE_OBJECT      NextDeviceObject;
    PIRP                newIrp;
    BOOLEAN             allocNewIrp = FALSE;
    KEVENT              Event;
    IO_STATUS_BLOCK     ioStatus;

    if (nNumberOfBytesToWrite == 0)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto Exit_AsyncWrite;
    }

    //
    // get the location of the next device object in the stack
    //
    if (bRawMode)
    {
        NextDeviceObject = deviceExtension->PortDeviceObject;
    }
    else
    {
        NextDeviceObject = deviceExtension->StackDeviceObject;
    }

    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode)
    {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, NextDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_AsyncWrite;            
        }
        allocNewIrp = TRUE;
    }

    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_AsyncWrite;
    } // if

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_ASYNC_WRITE;
    pIrb->Flags = 0;
    pIrb->u.AsyncWrite.DestinationAddress = DestinationAddress;
    pIrb->u.AsyncWrite.nNumberOfBytesToWrite = nNumberOfBytesToWrite;
    pIrb->u.AsyncWrite.nBlockSize = nBlockSize;
    pIrb->u.AsyncWrite.fulFlags = fulFlags;

    //
    // Write the data to our local memory if it exists.
    //
    if( deviceExtension->AddressRangeData != NULL )
    {
        void* destAddress = (unsigned char*)(deviceExtension->AddressRangeData->Buffer) + DestinationAddress.IA_Destination_Offset.Off_Low;
        RtlCopyMemory( destAddress, Data, nNumberOfBytesToWrite);
    }

    if (bGetGeneration) {
    
        pIrb->u.AsyncRead.ulGeneration = deviceExtension->GenerationCount;
    }
    else
    {
        pIrb->u.AsyncRead.ulGeneration = ulGeneration;
    }
    
    pMdl = IoAllocateMdl (Data,
                          nNumberOfBytesToWrite,
                          FALSE,
                          FALSE,
                          NULL);
    
    MmBuildMdlForNonPagedPool(pMdl);

    pIrb->u.AsyncWrite.Mdl = pMdl;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = rad1394_SubmitIrpAsync (NextDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
    
        ntStatus = rad1394_SubmitIrpSynch(NextDeviceObject, Irp, pIrb);
    }
    
    if (pMdl)
        ExFreePool(pMdl);

    if (pIrb)
        ExFreePool(pIrb);

Exit_AsyncWrite:

    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;

    return(ntStatus);
}


NTSTATUS rad1394_Get1394AddressFromDeviceObject
(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            fulFlags,
    OUT PNODE_ADDRESS   pNodeAddress
)
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIRB                pIrb;

    PIRP                newIrp;
    BOOLEAN             allocNewIrp = FALSE;
    KEVENT              Event;
    IO_STATUS_BLOCK     ioStatus;
    
    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, deviceExtension->StackDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {
      
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_Get1394AddressFromDeviceObject;            
        }
        allocNewIrp = TRUE;
    }
    
    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_Get1394AddressFromDeviceObject;
    } // if

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_GET_ADDR_FROM_DEVICE_OBJECT;
    pIrb->Flags = 0;
    pIrb->u.Get1394AddressFromDeviceObject.fulFlags = fulFlags;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = rad1394_SubmitIrpAsync (deviceExtension->StackDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
        ntStatus = rad1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, pIrb);
    }
    
    if (NT_SUCCESS(ntStatus)) {

        *pNodeAddress = pIrb->u.Get1394AddressFromDeviceObject.NodeAddress;
    }

    ExFreePool(pIrb);

Exit_Get1394AddressFromDeviceObject:

    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;
        
    return(ntStatus);
} 


NTSTATUS rad1394_SetAddressRangeSize
( 
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            fulFlags,
    IN ULONG            size,
    IN OUT PULONG       Data
)
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION       deviceExtension = DeviceObject->DeviceExtension;
    PIRB                    pIrb;
    PASYNC_ADDRESS_DATA     pAsyncAddressData;
    KIRQL                   Irql;
    ULONG                   nPages;
    BOOLEAN                 freeRange = FALSE;

    PIRP                    newIrp;
    BOOLEAN                 allocNewIrp = FALSE;
    KEVENT                  Event;
    IO_STATUS_BLOCK         ioStatus;
 

    // 
    // Free up the address range if it was allocated already and size different than 
    // requested size.
    //
    KeAcquireSpinLock(&deviceExtension->AddressRangeSpinLock, &Irql);

    if( deviceExtension->AddressRangeData != NULL )
    {
        pAsyncAddressData = deviceExtension->AddressRangeData;

        if( pAsyncAddressData->nLength != size )
        {
            freeRange = TRUE;
        }
        else 
        {
            //
            // We have some already allocated and it is the same size. Just return.
            //
            KeReleaseSpinLock(&deviceExtension->AddressRangeSpinLock, Irql);

            ntStatus = STATUS_SUCCESS;
            return(ntStatus);
        }
    }

    KeReleaseSpinLock(&deviceExtension->AddressRangeSpinLock, Irql);

    //
    // Check if in fact we need to free the memory outside of the protection
    // of the spin lock.
    //
    if( freeRange )
    {
        rad1394_FreeAddressRange( DeviceObject, Irp );
    }

    if( size == 0) 
    {
        //
        // Nothing to do. Return success.
        //
        ntStatus = STATUS_SUCCESS;
        return(ntStatus);
    }
    
    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, deviceExtension->StackDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {
      
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_AllocateAddressRange;            
        }
        allocNewIrp = TRUE;
    }

    //
    // Allocate memory for the IEEE 1394 Request Block
    //
    pIrb = ExAllocatePool( NonPagedPool, sizeof(IRB));

    if (!pIrb)
    {
        //
        // Not enough memory. Exit.
        //
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_AllocateAddressRange;
    } 

    //
    // Allocate memory for the address data.
    //
    pAsyncAddressData = ExAllocatePool(NonPagedPool, sizeof(ASYNC_ADDRESS_DATA));

    if (!pAsyncAddressData)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_AllocateAddressRange;
    }

    //
    // Allocate memory for the actuall address range to appear on the BUS.
    //
    pAsyncAddressData->Buffer = ExAllocatePool(NonPagedPool, size);

    if (!pAsyncAddressData->Buffer) 
    {
        ExFreePool(pAsyncAddressData);
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_AllocateAddressRange;
    }

    //
    // we need to know how big our address range buffer will be...
    // The sample I got this from used an address in the virtual space for this calculation. 
    // I don't understand why but I just passed an address from the outside world as well.
    //
    nPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(Data, size);

    pAsyncAddressData->AddressRange = ExAllocatePool(NonPagedPool, sizeof(ADDRESS_RANGE)*nPages);

    if (!pAsyncAddressData->AddressRange) 
    {
        ExFreePool(pAsyncAddressData->Buffer);
        ExFreePool(pAsyncAddressData);

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_AllocateAddressRange;
    }

    pAsyncAddressData->pMdl = MmCreateMdl(NULL, pAsyncAddressData->Buffer, size);

    if (!pAsyncAddressData->pMdl)
    {
        ExFreePool(pAsyncAddressData->AddressRange);
        ExFreePool(pAsyncAddressData->Buffer);
        ExFreePool(pAsyncAddressData);

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_AllocateAddressRange;
    }

    MmBuildMdlForNonPagedPool(pAsyncAddressData->pMdl);

    pAsyncAddressData->nLength = size;

    pIrb->FunctionNumber = REQUEST_ALLOCATE_ADDRESS_RANGE;
    pIrb->Flags = 0;
    pIrb->u.AllocateAddressRange.Mdl = pAsyncAddressData->pMdl;
    pIrb->u.AllocateAddressRange.fulFlags = fulFlags;
    pIrb->u.AllocateAddressRange.nLength = size;
    pIrb->u.AllocateAddressRange.MaxSegmentSize = 0;
    pIrb->u.AllocateAddressRange.fulAccessType = ACCESS_FLAGS_TYPE_WRITE | ACCESS_FLAGS_TYPE_READ;
    pIrb->u.AllocateAddressRange.fulNotificationOptions = NOTIFY_FLAGS_NEVER;
    pIrb->u.AllocateAddressRange.Callback = NULL;
    pIrb->u.AllocateAddressRange.Context = NULL;
    pIrb->u.AllocateAddressRange.FifoSListHead = NULL;
    pIrb->u.AllocateAddressRange.FifoSpinLock = NULL;
    pIrb->u.AllocateAddressRange.AddressesReturned = 0;
    pIrb->u.AllocateAddressRange.p1394AddressRange = pAsyncAddressData->AddressRange;
    pIrb->u.AllocateAddressRange.DeviceExtension = deviceExtension;

    pIrb->u.AllocateAddressRange.Required1394Offset.Off_High = 1;
    pIrb->u.AllocateAddressRange.Required1394Offset.Off_Low = 0;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = rad1394_SubmitIrpAsync (deviceExtension->StackDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
    
        ntStatus = rad1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, pIrb);
    }

    if( NT_SUCCESS(ntStatus))
    {
        // save off info into our struct...
        pAsyncAddressData->DeviceExtension = deviceExtension;
        pAsyncAddressData->nAddressesReturned = pIrb->u.AllocateAddressRange.AddressesReturned;
        pAsyncAddressData->hAddressRange = pIrb->u.AllocateAddressRange.hAddressRange;

        // add our struct to our device extenion
        KeAcquireSpinLock(&deviceExtension->AddressRangeSpinLock, &Irql);
        deviceExtension->AddressRangeData = pAsyncAddressData ;
        KeReleaseSpinLock(&deviceExtension->AddressRangeSpinLock, Irql);
    }
    else 
    {
        ExFreePool(pAsyncAddressData->pMdl);
        ExFreePool(pAsyncAddressData->Buffer);
        ExFreePool(pAsyncAddressData->AddressRange);
        ExFreePool(pAsyncAddressData);
    }


Exit_AllocateAddressRange:

    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;

    if (pIrb)
    {
        ExFreePool(pIrb);
    }
    return(ntStatus);

}



NTSTATUS rad1394_FreeAddressRange
( 
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION       deviceExtension = DeviceObject->DeviceExtension;
    PIRB                    pIrb;
    KIRQL                   Irql;
    PASYNC_ADDRESS_DATA     AsyncAddressData;

    PIRP                    newIrp;
    BOOLEAN                 allocNewIrp = FALSE;
    KEVENT                  Event;
    IO_STATUS_BLOCK         ioStatus;
 
    KeAcquireSpinLock(&deviceExtension->AddressRangeSpinLock, &Irql);

    AsyncAddressData = deviceExtension->AddressRangeData;

    deviceExtension->AddressRangeData = NULL;

    KeReleaseSpinLock(&deviceExtension->AddressRangeSpinLock, Irql);

    // never found an entry...
    if( !AsyncAddressData )
    {
        //
        // Consider it successful in our case.
        //
        goto Exit_FreeAddressRange;
    }

    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, deviceExtension->StackDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {
       
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_FreeAddressRange;            
        }
        allocNewIrp = TRUE;
    }
 

    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if( !pIrb ) 
    {
        //
        // we need to add this back into our list since we were
        // unable to free it...
        KeAcquireSpinLock(&deviceExtension->AddressRangeSpinLock, &Irql);
        deviceExtension->AddressRangeData = AsyncAddressData;
        KeReleaseSpinLock(&deviceExtension->AddressRangeSpinLock, Irql);

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_FreeAddressRange;
    } 

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_FREE_ADDRESS_RANGE;
    pIrb->Flags = 0;
    pIrb->u.FreeAddressRange.nAddressesToFree = AsyncAddressData->nAddressesReturned;
    pIrb->u.FreeAddressRange.p1394AddressRange = AsyncAddressData->AddressRange;
    pIrb->u.FreeAddressRange.pAddressRange = &AsyncAddressData->hAddressRange;
    pIrb->u.FreeAddressRange.DeviceExtension = (PVOID)deviceExtension;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = rad1394_SubmitIrpAsync (deviceExtension->StackDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
    
        ntStatus = rad1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, pIrb);
    }

    if (pIrb)
    {
        ExFreePool(pIrb);
    }
    // need to free up everything associated with this allocate...
    if (AsyncAddressData->pMdl)
    {
        ExFreePool(AsyncAddressData->pMdl);
    }
    if (AsyncAddressData->Buffer)
    {
        ExFreePool(AsyncAddressData->Buffer);
    }
    if (AsyncAddressData->AddressRange)
    {
        ExFreePool(AsyncAddressData->AddressRange);
    }
    if (AsyncAddressData)
    {
        ExFreePool(AsyncAddressData);
    }

Exit_FreeAddressRange:
    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;

    return(ntStatus);
}

void rad1394_BusResetRoutine
(
    IN PVOID    Context
)
{
    PDEVICE_EXTENSION   deviceExtension = Context;

}

NTSTATUS rad1394_BusResetNotification
(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            fulFlags
)
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIRB                pIrb;

    PIRP                newIrp;
    BOOLEAN             allocNewIrp = FALSE;
    KEVENT              Event;
    IO_STATUS_BLOCK     ioStatus;
    
    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, deviceExtension->StackDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {
       
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_BusResetNotification;            
        }
        allocNewIrp = TRUE;
    }
    
    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_BusResetNotification;
    } // if

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_BUS_RESET_NOTIFICATION;
    pIrb->Flags = 0;
    pIrb->u.BusResetNotification.fulFlags = fulFlags;
    pIrb->u.BusResetNotification.ResetRoutine = rad1394_BusResetRoutine;
    pIrb->u.BusResetNotification.ResetContext = DeviceObject;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = rad1394_SubmitIrpAsync (deviceExtension->StackDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
        ntStatus = rad1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, pIrb);
    }
    
    if (pIrb)
        ExFreePool(pIrb);

Exit_BusResetNotification:

    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;
        
    return(ntStatus);
}

NTSTATUS rad1394_GetGenerationCount
(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN OUT PULONG       GenerationCount
)
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIRB                pIrb;

    PIRP                newIrp;
    BOOLEAN             allocNewIrp = FALSE;
    KEVENT              Event;
    IO_STATUS_BLOCK     ioStatus;

    //
    // If we were not passed an Irp or this is a UserMode request create a newIrp 
    // so that the request will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, deviceExtension->StackDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {
       
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_GetGenerationCount;            
        }
        allocNewIrp = TRUE;
    }

    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_GetGenerationCount;
    } // if

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_GET_GENERATION_COUNT;
    pIrb->Flags = 0;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = rad1394_SubmitIrpAsync (deviceExtension->StackDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
        ntStatus = rad1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, pIrb);
    }
    
    if (NT_SUCCESS(ntStatus)) {

        *GenerationCount = pIrb->u.GetGenerationCount.GenerationCount;
    }

    ExFreePool(pIrb);

Exit_GetGenerationCount:

    if (Irp && allocNewIrp) 
        Irp->IoStatus = ioStatus;
        
    return(ntStatus);
}

