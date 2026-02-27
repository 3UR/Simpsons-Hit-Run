//=============================================================================
//
// File:        rad1394driver.h
//
// Subsystem:	Radical 1394 Firewire Driver
//
// Description:	This file contains all definitions required to use the rad1394
//              FireWire Driver.
//
// Revisions:	27-June-2001 Creation
//
// Author:      Peter Mielcarski
//
//=============================================================================

#ifndef	RAD1394DRIVER_H
#define RAD1394DRIVER_H

//=============================================================================
// Include Files
//=============================================================================

//=============================================================================
// Defintions
//=============================================================================

//
// This is the GUID of our Win2K device driver. {24E72B3A-0D7F-4931-8475-F46E187B0955}
//
DEFINE_GUID( rad1394DriverGUID, 0x24e72b3a, 0xd7f, 0x4931, 0x84, 0x75, 0xf4, 0x6e, 0x18, 0x7b, 0x9, 0x55);

//
// The following defintions are found in NT DDK file "devioctl.h". To prevent
// a hastle for people, the definitions will defined here.
//
#ifndef _DEVIOCTL_
#ifndef DRIVER

#define FILE_DEVICE_UNKNOWN             0x00000022
#define METHOD_BUFFERED                 0
#define FILE_ANY_ACCESS                 0
#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
                  ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) )
#endif
#endif

//
// These are our definitions used to perform device I/O control functions in
// our device driver.
//
#define RAD1394_IOCTL_INDEX     0x0800

#define RAD1394_IOCTL_READ                              CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        RAD1394_IOCTL_INDEX + 0,        \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define RAD1394_IOCTL_WRITE                             CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        RAD1394_IOCTL_INDEX + 1,        \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define RAD1394_IOCTL_GET_ADDRESS_FROM_DEVICE           CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        RAD1394_IOCTL_INDEX + 2,        \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define RAD1394_IOCTL_SET_ADDRESS_RANGE_SIZE            CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        RAD1394_IOCTL_INDEX + 3,        \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

//
// Structures used to pass information into the device driver.
//
#ifndef DRIVER
typedef struct
{
    USHORT              NA_Node_Number:6;       // Bits 10-15
    USHORT              NA_Bus_Number:10;       // Bits 0-9
} NODE_ADDRESS;

typedef struct
{
    USHORT              Off_High;
    ULONG               Off_Low;
} ADDRESS_OFFSET;

typedef struct
{
    NODE_ADDRESS        IA_Destination_ID;
    ADDRESS_OFFSET      IA_Destination_Offset;
}
IO_ADDRESS;

#endif

//
// These constants were choosen through experimentation to give maximum
// thoughput without error. No idea why?
//
#define     RAD1394_MAX_WRITE_SIZE  512 * 4
//#define     RAD1394_MAX_WRITE_SIZE  512
#define     RAD1394_MAX_READ_SIZE   512 * 4

typedef struct 
{
    ULONG           bRawMode;
    ULONG           bGetGeneration;
    IO_ADDRESS      DestinationAddress;
    ULONG           nNumberOfBytesToRead;
    ULONG           bLocalRead;
    ULONG           nBlockSize;
    ULONG           fulFlags;
    ULONG           ulGeneration;
    UCHAR           Data[RAD1394_MAX_READ_SIZE];
    UCHAR           Reserve[ 1 ];
} ASYNC_READ;

typedef struct
{ 
    ULONG           bRawMode;
    ULONG           bGetGeneration;
    IO_ADDRESS      DestinationAddress;
    ULONG           nNumberOfBytesToWrite;
    ULONG           nBlockSize;
    ULONG           fulFlags;
    ULONG           ulGeneration;
    UCHAR           Data[RAD1394_MAX_WRITE_SIZE];
    UCHAR           Reserve[ 1 ];
} ASYNC_WRITE;

typedef struct
{
    ULONG           fulFlags;
    NODE_ADDRESS    NodeAddress;
} GET_1394_ADDRESS;

typedef struct
{
    ULONG           fulFlags;
    ULONG           Size;
    UCHAR           Data[ 1 ];
} SET_1394_ADDRESS_RANGE_SIZE;


//
// These are internal definitions used only by the driver.
//
#ifdef DRIVER



//
// This is used to store data for each async address range. Currently we only allocate
// a single address range
//

typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT          StackDeviceObject;
    PDEVICE_OBJECT          PortDeviceObject;
    UNICODE_STRING          SymbolicLinkName;
    KSPIN_LOCK              ResetSpinLock;
    BOOLEAN                 bShutdown;
    POWER_STATE             CurrentDevicePowerState;
    ULONG                   GenerationCount;
    LIST_ENTRY              BusResetIrps;

    KSPIN_LOCK                     AddressRangeSpinLock;
    struct _ASYNC_ADDRESS_DATA*     AddressRangeData;

    ULONG                   OpenCount;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _ASYNC_ADDRESS_DATA 
{
    PDEVICE_EXTENSION       DeviceExtension;
    PVOID                   Buffer;
    ULONG                   nLength;
    ULONG                   nAddressesReturned;
    PADDRESS_RANGE          AddressRange;
    HANDLE                  hAddressRange;
    PMDL                    pMdl;
} ASYNC_ADDRESS_DATA, *PASYNC_ADDRESS_DATA;

typedef struct _BUS_RESET_IRP {
    LIST_ENTRY      BusResetIrpList;
    PIRP            Irp;
} BUS_RESET_IRP, *PBUS_RESET_IRP;


NTSTATUS rad1394_GetGenerationCount
(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN OUT PULONG       GenerationCount
);

NTSTATUS rad1394_BusResetNotification
(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            fulFlags
);

NTSTATUS rad1394_SubmitIrpSynch
(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIRB                 Irb
);

NTSTATUS rad1394_SubmitIrpAsync
(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIRB                 Irb
);





#endif

#endif