//=============================================================================
//
// File:        rad1394qpnp.c
//
// Subsystem:	Radical Fire Wire Win32 Driver
//
// Description:	This file contains the qpnp routines for the driver. They were
//              borrowed directly from NTDDK examples.
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

#define INITGUID

#include <basetyps.h>
#include <wdm.h>
#include <1394.h>
#include "rad1394driver.h"
#include "rad1394qpnp.h"

//=============================================================================
// Local Definitions
//=============================================================================

NTSTATUS rad1394_PnpStartDevice( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );
NTSTATUS rad1394_PnpStopDevice( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );
NTSTATUS rad1394_PnpRemoveDevice( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );
NTSTATUS rad1394_PnpBusReset( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );

//=============================================================================
// Public Functions
//=============================================================================

NTSTATUS rad1394_PnpAddDevice
(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
)
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT          DeviceObject;
    PDEVICE_EXTENSION       deviceExtension;
    GUID                    rad1394GUID;
    PNODE_DEVICE_EXTENSION  pNodeExt;

    ntStatus = IoCreateDevice( DriverObject,
                               sizeof(DEVICE_EXTENSION),
                               NULL,
                               FILE_DEVICE_UNKNOWN,
                               0,
                               FALSE,
                               &DeviceObject
                               );

    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit_PnpAddDevice;
    }

    deviceExtension = DeviceObject->DeviceExtension;
    RtlZeroMemory(deviceExtension, sizeof(DEVICE_EXTENSION));

    rad1394GUID = rad1394DriverGUID;

    ntStatus = IoRegisterDeviceInterface( PhysicalDeviceObject,
                                          &rad1394GUID,
                                          NULL,
                                          &deviceExtension->SymbolicLinkName
                                          );

    if (!NT_SUCCESS(ntStatus))
    {

        IoDeleteDevice(DeviceObject);
        ntStatus = STATUS_NO_SUCH_DEVICE;
        goto Exit_PnpAddDevice;
    }

    deviceExtension->StackDeviceObject =
        IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);

    if (!deviceExtension->StackDeviceObject)
    {

        IoDeleteDevice(DeviceObject);
        ntStatus = STATUS_NO_SUCH_DEVICE;
        goto Exit_PnpAddDevice;
    }
	

    // get the port device object
    pNodeExt = deviceExtension->StackDeviceObject->DeviceExtension;
    deviceExtension->PortDeviceObject = pNodeExt->PortDeviceObject;


    // initialize the spinlock/list to store the bus reset irps...
    KeInitializeSpinLock(&deviceExtension->ResetSpinLock);
    InitializeListHead(&deviceExtension->BusResetIrps);

    KeInitializeSpinLock(&deviceExtension->AddressRangeSpinLock);
    deviceExtension->AddressRangeData = NULL;
    deviceExtension->OpenCount = 0;

    DeviceObject->Flags &= DO_POWER_PAGABLE;
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

Exit_PnpAddDevice:
    return(ntStatus);
}

NTSTATUS rad1394_Pnp
(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
)
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION      IrpSp;
    PDEVICE_EXTENSION       deviceExtension = DeviceObject->DeviceExtension;

    PDEVICE_CAPABILITIES    DeviceCapabilities;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->MinorFunction) 
    {
        case IRP_MN_START_DEVICE:
        {
            // pass down to layer below us first.
            ntStatus = rad1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, NULL);

            ntStatus = rad1394_PnpStartDevice(DeviceObject, Irp);

            Irp->IoStatus.Status = ntStatus;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        }

        case IRP_MN_QUERY_STOP_DEVICE:
        {
            // pass down to layer below us...
            ntStatus = rad1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, NULL);
            break;
        }
        case IRP_MN_STOP_DEVICE:
        {
            ntStatus = rad1394_PnpStopDevice(DeviceObject, Irp);

            // i'm not allowed to fail here. if i needed to fail, i should have done it
            // at the query_stop_device request.
            Irp->IoStatus.Status = STATUS_SUCCESS;

            // pass down to layer below us...
            ntStatus = rad1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, NULL);
            break;
        }
        case IRP_MN_CANCEL_STOP_DEVICE:
        {
            // pass down to layer below us first.
            ntStatus = rad1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, NULL);

            Irp->IoStatus.Status = ntStatus;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        }
        case IRP_MN_QUERY_REMOVE_DEVICE:
        {
            Irp->IoStatus.Status = STATUS_SUCCESS;

            // pass down to layer below us...
            ntStatus = rad1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, NULL);
            break;
        }       
        
        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_REMOVE_DEVICE:
        {
            ntStatus = rad1394_PnpRemoveDevice(DeviceObject, Irp);

            // pass down to layer below us...
            ntStatus = rad1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, NULL);

            // delete our device
            IoDetachDevice(deviceExtension->StackDeviceObject);
            IoDeleteDevice(DeviceObject);
            break;
        }

        case IRP_MN_CANCEL_REMOVE_DEVICE:
        {
            // pass down to layer below us first.
            ntStatus = rad1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, NULL);

            Irp->IoStatus.Status = ntStatus;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        }
        case IRP_MN_BUS_RESET:
        {
            ntStatus = rad1394_PnpBusReset(DeviceObject, Irp);

            Irp->IoStatus.Status = ntStatus;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        }
        case IRP_MN_QUERY_CAPABILITIES:
        {
            DeviceCapabilities = IrpSp->Parameters.DeviceCapabilities.Capabilities;

            DeviceCapabilities->SurpriseRemovalOK = TRUE;

            ntStatus = rad1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, NULL);

            Irp->IoStatus.Status = ntStatus;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        }
        default:
        {
            // pass down to layer below us...
            ntStatus = rad1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, NULL);

            Irp->IoStatus.Status = ntStatus;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        }
    }

    return(ntStatus);
} 

NTSTATUS rad1394_PnpStartDevice
( 
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
)
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION       deviceExtension = DeviceObject->DeviceExtension;

    deviceExtension->bShutdown = FALSE;

    ntStatus = rad1394_BusResetNotification( DeviceObject,
                                               Irp,
                                               REGISTER_NOTIFICATION_ROUTINE
                                               );

    // update the generation count...
    ntStatus = rad1394_GetGenerationCount(DeviceObject, Irp, &deviceExtension->GenerationCount);

    // activate the interface...
    ntStatus = IoSetDeviceInterfaceState(&deviceExtension->SymbolicLinkName, TRUE);

    return(ntStatus);
}

NTSTATUS rad1394_PnpStopDevice
(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
)
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;

    deviceExtension->bShutdown = TRUE;

    ntStatus = rad1394_BusResetNotification( DeviceObject,
                                               Irp,
                                               DEREGISTER_NOTIFICATION_ROUTINE
                                               );

    return(ntStatus);
}

NTSTATUS rad1394_PnpRemoveDevice
(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
)
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    KIRQL               Irql;

    if (!deviceExtension->bShutdown) {

        // haven't stopped yet, lets do so
        ntStatus = rad1394_PnpStopDevice(DeviceObject, Irp);
    }

    // deactivate the interface...
    ntStatus = IoSetDeviceInterfaceState(&deviceExtension->SymbolicLinkName, FALSE);

    // 
    // Free up the address range if it was allocated.
    //
    KeAcquireSpinLock(&deviceExtension->AddressRangeSpinLock, &Irql);

    if( deviceExtension->AddressRangeData != NULL )
    {
        PASYNC_ADDRESS_DATA     AsyncAddressData;

        AsyncAddressData = deviceExtension->AddressRangeData;

        // need to free up everything associated with this allocate...
        if (AsyncAddressData->pMdl)
            ExFreePool(AsyncAddressData->pMdl);

        if (AsyncAddressData->Buffer)
            ExFreePool(AsyncAddressData->Buffer);

        if (AsyncAddressData->AddressRange)
            ExFreePool(AsyncAddressData->AddressRange);

        if (AsyncAddressData)
            ExFreePool(AsyncAddressData);
        
        deviceExtension->AddressRangeData = NULL;
    }

    KeReleaseSpinLock(&deviceExtension->AddressRangeSpinLock, Irql);

    // get rid of any pending bus reset notify requests
    KeAcquireSpinLock(&deviceExtension->ResetSpinLock, &Irql);

    while (!IsListEmpty(&deviceExtension->BusResetIrps))
    {

        PBUS_RESET_IRP  BusResetIrp;
        PDRIVER_CANCEL  prevCancel = NULL;

        // get the irp off of the list
        BusResetIrp = (PBUS_RESET_IRP)RemoveHeadList(&deviceExtension->BusResetIrps);

        // make this irp non-cancelable...
        prevCancel = IoSetCancelRoutine(BusResetIrp->Irp, NULL);

        // and complete it...
        BusResetIrp->Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(BusResetIrp->Irp, IO_NO_INCREMENT);

        ExFreePool(BusResetIrp);
    }

    KeReleaseSpinLock(&deviceExtension->ResetSpinLock, Irql);

    // free up the symbolic link
    RtlFreeUnicodeString(&deviceExtension->SymbolicLinkName);

    return(ntStatus);
}

NTSTATUS rad1394_PnpBusReset
(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
)
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    KIRQL               Irql;
    PDRIVER_CANCEL      prevCancel = NULL;
    PBUS_RESET_IRP      BusResetIrp = NULL;

    if (deviceExtension->bShutdown)
    {
        return(ntStatus);
    }

    // update the generation count...
    ntStatus = rad1394_GetGenerationCount(DeviceObject, Irp, &deviceExtension->GenerationCount);

    // if we have any bus reset notify irps, then nows the time to complete them
    KeAcquireSpinLock(&deviceExtension->ResetSpinLock, &Irql);

    while (!IsListEmpty(&deviceExtension->BusResetIrps))
    {

        // get the irp off of the list
        BusResetIrp = (PBUS_RESET_IRP)RemoveHeadList(&deviceExtension->BusResetIrps);

        // make this irp non-cancelable...
        prevCancel = IoSetCancelRoutine(BusResetIrp->Irp, NULL);

        // and complete it...
        BusResetIrp->Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(BusResetIrp->Irp, IO_NO_INCREMENT);

        ExFreePool(BusResetIrp);
    }

    KeReleaseSpinLock(&deviceExtension->ResetSpinLock, Irql);

    return(ntStatus);
}


