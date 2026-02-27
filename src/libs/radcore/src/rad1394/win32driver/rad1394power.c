//=============================================================================
//
// File:        rad1394power.c
//
// Subsystem:	Radical Fire Wire Win32 Driver
//
// Description:	This file contains the power routines for the driver. They were
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

#include <basetyps.h>
#include <wdm.h>
#include <1394.h>
#include "rad1394driver.h"
#include "rad1394power.h"

//=============================================================================
// Local Definitions
//=============================================================================

void rad1394_PowerRequestCompletion( IN PDEVICE_OBJECT DeviceObject, IN UCHAR MinorFunction,
                                     IN POWER_STATE PowerState, IN PIRP Irp,
                                     IN PIO_STATUS_BLOCK IoStatus );

//=============================================================================
// Public Functions
//=============================================================================

NTSTATUS rad1394_Power
(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
)
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION      IrpSp;
    PDEVICE_EXTENSION       deviceExtension;
    POWER_STATE             State;
    KIRQL                   Irql;

    deviceExtension = DeviceObject->DeviceExtension;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    State = IrpSp->Parameters.Power.State;

    switch (IrpSp->MinorFunction) 
    {
        case IRP_MN_SET_POWER:
        {
            switch (IrpSp->Parameters.Power.Type)
            {
                case SystemPowerState:
                {
                    if (State.SystemState == PowerSystemWorking)
                    {
                        State.DeviceState = PowerDeviceD0;
                    }
                    else
                    {
                        State.DeviceState = PowerDeviceD3;
                    }

                    if (State.DeviceState != deviceExtension->CurrentDevicePowerState.DeviceState)
                    {
                        ntStatus = PoRequestPowerIrp( deviceExtension->StackDeviceObject,
                                                      IRP_MN_SET_POWER,
                                                      State,
                                                      rad1394_PowerRequestCompletion,
                                                      Irp,
                                                      NULL );
                    }
                    else
                    {
                        IoCopyCurrentIrpStackLocationToNext(Irp);
                        PoStartNextPowerIrp(Irp);
                        ntStatus = PoCallDriver(deviceExtension->StackDeviceObject, Irp);
                    }

                    break; 
                }

                case DevicePowerState:
                {
                    deviceExtension->CurrentDevicePowerState = State;

                    IoCopyCurrentIrpStackLocationToNext(Irp);

                    PoStartNextPowerIrp(Irp);
                    ntStatus = PoCallDriver(deviceExtension->StackDeviceObject, Irp);

                    break; 
                }    

                default:
                {
                    break;
                }
            }
            break; 
        }

        case IRP_MN_QUERY_POWER:
        {
            IoCopyCurrentIrpStackLocationToNext(Irp);
            PoStartNextPowerIrp(Irp);
            ntStatus = PoCallDriver(deviceExtension->StackDeviceObject, Irp);

            break;
        }

        default:
        {
            IoCopyCurrentIrpStackLocationToNext(Irp);
            PoStartNextPowerIrp(Irp);
            ntStatus = PoCallDriver(deviceExtension->StackDeviceObject, Irp);

            break; 
        }
            
    } 

    return( ntStatus );
} 

void rad1394_PowerRequestCompletion
(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PIRP                 Irp,
    IN PIO_STATUS_BLOCK     IoStatus
)
{
    NTSTATUS            ntStatus;

    ntStatus = IoStatus->Status;

    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return;
} 


