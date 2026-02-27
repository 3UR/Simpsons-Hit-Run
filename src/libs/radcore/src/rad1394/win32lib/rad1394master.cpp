//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        rad1394master.cpp
//
// Subsystem:	Radical Fire Wire Win32 Master
//
// Description:	This file contains the implementation the master interface to
//              the Firewire. It provides the bridge to the device driver.
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	June 27, 2001
//
// Notes:       
//
//=============================================================================

//
// This stuff only works on Windows 2K.
//
#define WINVER 0x0500

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <stdlib.h>
#include <windows.h>
#include <dbt.h>
#include <INITGUID.H>
#include <Setupapi.h>
#include "rad1394master.hpp"
#include <radplatform.hpp>
#include <radtime.hpp>
#include "..\win32driver\rad1394driver.h"

//=============================================================================
// Local Definitions
//=============================================================================

#define WINDOW_CLASSNAME "rad1394MasterWindowClass"

//=============================================================================
// Statics
//=============================================================================

static rad1394Master*  s_the1394Master = NULL;

//=============================================================================
// Public Functions
//=============================================================================

//=============================================================================
// Function:    rad1394Initialize
//=============================================================================
// Description: This routine must be invoke prior to accessing any other 1394
//              functions. 
//
// Parameters:  unsigned int sharedMemorySize.
//              
// Returns:     n/a
//
// Notes:       
//------------------------------------------------------------------------------

void rad1394Initialize
( 
    unsigned int sharedMemorySize
)
{
    //
    // See if this subsystem has not allready been initialized.
    //
    if( s_the1394Master == NULL )
    {
        //
        // Construct the object.
        //
        s_the1394Master = new rad1394Master( sharedMemorySize );
    }
    else
    {
        //
        // Initailize and terminate must be balanced. Add ref the existing on.
        //
        s_the1394Master->AddRef( );
    }
}

//=============================================================================
// Function:    rad1394Terminate
//=============================================================================
// Description: This routine will shut down things.
//
// Parameters:  n/a
//              
// Returns:     n/a
//
// Notes:       
//------------------------------------------------------------------------------

void rad1394Terminate( void )
{
    //
    // Make sure has been initailzed. Release reference.
    //
    rAssertMsg( s_the1394Master != NULL, "rad1394 master not initialized\n");
    
    s_the1394Master->Release( );
}

//=============================================================================
// Function:    rad1394Get
//=============================================================================
// Description: This routine obtains an interface pointer to the singleton 
//              master object.
//
// Parameters:  
//              
// Returns:     IRad1394Master
//
// Notes:       Client is reponsible for adding a reference if he will hold the
//              pointer.
//------------------------------------------------------------------------------

IRad1394Master* rad1394Get( void )
{
    rAssertMsg( s_the1394Master != NULL, "rad1394 master not initialized\n");
    return( s_the1394Master );
};

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    rad1394Master::rad1394Master
//=============================================================================
// Description: Constructor for the master object.
//              
// Paramenters: size of local shared memory
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

rad1394Master::rad1394Master( unsigned int sharedSize )
    :
    m_DeviceMonitorWindow( NULL ),
    m_ReferenceCount( 1 ),
    m_SharedMemorySize( sharedSize )
{
    radTimeInitialize( );

    //
    // Clear the device info table to all invalid.
    //
    for( unsigned int i = 0 ; i < sizeof( m_DeviceInfo ) / sizeof( DeviceInfo ) ; i++ )
    {
        m_DeviceInfo[ i ].m_IsValid = false;
    }

    m_LastWriteTime = radTimeGetMicroseconds( );

    //
    // Lets create a critical section to serialize access to the proceeding table.
    //
    InitializeCriticalSection( &m_ExclusionObject );

    //
    // Update the status of the device info.
    //
    UpdateDeviceInfo( );

    //
    // Create a thread to monitor device changes.
    //
    m_ThreadHandle = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) DeviceMontiorThread, 
                                   this, 0, NULL ); 
        
}

//=============================================================================
// Function:    rad1394Master::~rad1394Master
//=============================================================================
// Description: Destructor for the client object.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

rad1394Master::~rad1394Master( void )
{
    //
    // Here we are shutting down. First kill the device monitor thread by 
    // posting it a quit message. There is a window where the thread may not have 
    // created its window and we are shutdown immediately. Deal with this.
    //
    while( m_DeviceMonitorWindow == NULL )
    {
        Sleep( 0 );
    }

    //
    // Post the quit message.
    //
    PostMessage( m_DeviceMonitorWindow, WM_QUIT, 0, 0 );

    //
    // Now wait for the thread to terminate.
    //
    WaitForSingleObject( m_ThreadHandle, INFINITE );
    
    //
    // Close thread handle and the critical section.       
    //
    CloseHandle( m_ThreadHandle );

    DeleteCriticalSection( &m_ExclusionObject );

    radTimeTerminate( );

    s_the1394Master = NULL;
}

//=============================================================================
// Function:    rad1394Master::DeviceMonitorThread
//=============================================================================
// Description: This thread is used for monitoring device changes.
//
// Parameters:  pointer to this.
//              
// Returns:     0
//
// Notes:
//------------------------------------------------------------------------------

unsigned int rad1394Master::DeviceMontiorThread( rad1394Master* pMaster )
{
    //
    // First lets create a window to register with the device notifcaiton function.
    // Start by registering window class, which requires the module handle of
    // this process.
    //
    char Filename[ 256 ];
    GetModuleFileName( NULL, Filename, sizeof(Filename) );
    HINSTANCE hInstance = GetModuleHandle( Filename );

    WNDCLASSEX WindowClass;
    
    WindowClass.cbSize = sizeof( WNDCLASSEX );
    WindowClass.style = 0;
    WindowClass.lpfnWndProc = DeviceMonitorWindowProcedure;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 4;
    WindowClass.hInstance = hInstance;
    WindowClass.hIcon = NULL;
    WindowClass.hCursor = NULL;
    WindowClass.hbrBackground = NULL;
    WindowClass.lpszMenuName = NULL;
    WindowClass.lpszClassName = WINDOW_CLASSNAME;
    WindowClass.hIconSm = NULL;

    RegisterClassEx( &WindowClass );    
 
    //
    // Create a window 
    //
    pMaster->m_DeviceMonitorWindow = CreateWindowEx( WS_EX_NOPARENTNOTIFY, WINDOW_CLASSNAME, "", WS_POPUP, 0, 0, 0, 0, 
                                                     NULL, NULL, hInstance, (void*) pMaster );

    //
    // Register the device notification routine.
    //
    DEV_BROADCAST_DEVICEINTERFACE notificationFilter;

    notificationFilter.dbcc_size = sizeof( DEV_BROADCAST_DEVICEINTERFACE );
    notificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    notificationFilter.dbcc_name[ 0 ] = 0;
    notificationFilter.dbcc_classguid = rad1394DriverGUID;

    HDEVNOTIFY hDeviceNotifyHandle = RegisterDeviceNotification( pMaster->m_DeviceMonitorWindow,
                                                                 &notificationFilter,   
                                                                 DEVICE_NOTIFY_WINDOW_HANDLE );

    //
    // Enter the message loop. Will fall out on receipt of a quit message.
    //
    MSG msg;

    while( GetMessage( &msg, pMaster->m_DeviceMonitorWindow, 0, 0 ) )
    {
        DispatchMessage( &msg );
    }
    
    //
    // Unregister our notification routine.
    //
    UnregisterDeviceNotification( hDeviceNotifyHandle );
    
    //
    // We are done. Destroy window and unregisted class
    //
    DestroyWindow( pMaster->m_DeviceMonitorWindow );

    UnregisterClass( WINDOW_CLASSNAME, hInstance );

    //
    // Finally lets close all devices that are still open. Grab exc
    //
    EnterCriticalSection( &pMaster->m_ExclusionObject );

    for( unsigned int i = 0 ; i < sizeof( pMaster->m_DeviceInfo ) / sizeof( DeviceInfo ) ; i++ )
    {
        if( pMaster->m_DeviceInfo[ i ].m_IsValid )
        {
            pMaster->m_DeviceInfo[ i ].m_IsValid = false;
            CloseHandle( pMaster->m_DeviceInfo[ i ].m_DeviceHandle );
        }
    }
    
    LeaveCriticalSection( &pMaster->m_ExclusionObject );

    return( 0 );
}

//=============================================================================
// Function:    rad1394Master::DeviceMonitorWindowProcedure
//=============================================================================
// Description: This window procedure fields messages generated when device
//              status change occurs.
//
// Parameters:  see Windows
//              
// Returns:     0
//
// Notes:
//------------------------------------------------------------------------------

LRESULT CALLBACK rad1394Master::DeviceMonitorWindowProcedure
( 
    HWND    hwnd, 
    UINT    uMsg, 
    WPARAM  wParam,
    LPARAM  lParam
)
{
    //
    // Check if create messaage. If so, save the "this: pointer.
    //
    if( uMsg == WM_NCCREATE )
    {
        //
        // Set the windows instance data.
        //
        SetWindowLong( hwnd, 0, (long) (((CREATESTRUCT*) lParam)->lpCreateParams) );

    }
    else if( uMsg == WM_DEVICECHANGE )
    {
        if( (wParam == DBT_DEVICEARRIVAL) || (wParam == DBT_DEVICEREMOVECOMPLETE) )
        {
            //
            // Get the this pointer 
            //
            rad1394Master* pMaster = (rad1394Master*) GetWindowLong( hwnd, 0 );

            //
            // Update our device list.
            //
            pMaster->UpdateDeviceInfo( );           
        }
    }

    return( DefWindowProc(hwnd, uMsg, wParam, lParam) );
}

//=============================================================================
// Function:    rad1394Master::UpdateDeriveInfo
//=============================================================================
// Description: This function queries for devices matching our GUID and updates
//              things.
//
// Parameters:  n/a
//              
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rad1394Master::UpdateDeviceInfo( void )
{
    //
    // Protect modification of this table.
    //
    EnterCriticalSection( &m_ExclusionObject );

    //
    // First lets close any open devices. This is probably not the most
    // efficient method but device changes normally will be quite rare.
    //
    for( unsigned int i = 0 ; i < sizeof( m_DeviceInfo ) / sizeof( DeviceInfo ) ; i++ )
    {
        if( m_DeviceInfo[ i ].m_IsValid )
        {
            CloseHandle( m_DeviceInfo[ i ].m_DeviceHandle );
            m_DeviceInfo[ i ].m_IsValid = false;        
        }
    }

    //
    // Now lets interogate what devices are present.
    //
    GUID    deviceClass = rad1394DriverGUID;
    HDEVINFO hDevInfo = SetupDiGetClassDevs( &deviceClass, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE );
    
    if( hDevInfo != NULL )
    {
        bool moreDevices = true;           
        unsigned int memberIndex = 0;

        while( moreDevices )
        {
            SP_DEVICE_INTERFACE_DATA deviceInterfaceData;

            deviceInterfaceData.cbSize = sizeof( SP_DEVICE_INTERFACE_DATA );

            if( SetupDiEnumDeviceInterfaces( hDevInfo, 0, &deviceClass, memberIndex, &deviceInterfaceData) )
            {
                unsigned long requiredSize;
                SP_DEVICE_INTERFACE_DETAIL_DATA*  pDeviceInterfaceDetailData;
                // 
                // Find out how much space we need to allocate to obtain driver interface datail.
                //
                SetupDiGetDeviceInterfaceDetail( hDevInfo, &deviceInterfaceData, NULL, 0, &requiredSize, NULL );
        
                pDeviceInterfaceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA*) new unsigned char[ requiredSize ];
                pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

                SetupDiGetDeviceInterfaceDetail( hDevInfo, &deviceInterfaceData,  pDeviceInterfaceDetailData,
                                                 requiredSize, NULL, NULL );
                //
                // Here we have the name of the device. Copy it into our local structure for prosperity.
                // and free the device interface datail memory.
                //
                strcpy( m_DeviceInfo[ memberIndex ].m_DeviceName, pDeviceInterfaceDetailData->DevicePath );

                delete[ ] (unsigned char*) pDeviceInterfaceDetailData;

                //
                // Lets try and open the device. If successful, save handle and get the bus and node
                // number for this device.
                //
                m_DeviceInfo[ memberIndex ].m_DeviceHandle = CreateFile(  m_DeviceInfo[ memberIndex ].m_DeviceName,
                                                                          GENERIC_WRITE | GENERIC_READ,
                                                                          FILE_SHARE_WRITE | FILE_SHARE_READ,
                                                                          NULL,
                                                                          OPEN_EXISTING,
                                                                          0,
                                                                          NULL );

                if( m_DeviceInfo[ memberIndex ].m_DeviceHandle != INVALID_HANDLE_VALUE )
                {
                    //
                    // Successfully openned the device. Lets get the bus and node numbers.
                    //
                    GET_1394_ADDRESS Address;
                    unsigned long bytesReturned;
                  
                    if( DeviceIoControl( m_DeviceInfo[ memberIndex ].m_DeviceHandle,
                                         RAD1394_IOCTL_GET_ADDRESS_FROM_DEVICE,
                                         &Address,
                                         sizeof(GET_1394_ADDRESS),
                                         &Address,
                                         sizeof(GET_1394_ADDRESS),
                                         &bytesReturned,
                                         NULL ) )
                    {
                        //
                        // Life is good. Save the address and set as valid.   
                        //
                        m_DeviceInfo[ memberIndex ].m_busNumber = Address.NodeAddress.NA_Bus_Number;
                        m_DeviceInfo[ memberIndex ].m_nodeNumber = Address.NodeAddress.NA_Node_Number;
                        m_DeviceInfo[ memberIndex ].m_IsValid = true;
    
                        //
                        // If we have shared memory, set size now.
                        //
                        if( m_SharedMemorySize != 0 )
                        {
                            SET_1394_ADDRESS_RANGE_SIZE* pAddressRangeSize;
                            pAddressRangeSize = (SET_1394_ADDRESS_RANGE_SIZE*) LocalAlloc(LPTR, sizeof( SET_1394_ADDRESS_RANGE_SIZE ) );
                            pAddressRangeSize->Size = m_SharedMemorySize;
                            pAddressRangeSize->fulFlags = 0;
                 
                            DeviceIoControl( m_DeviceInfo[ memberIndex ].m_DeviceHandle,
                                             RAD1394_IOCTL_SET_ADDRESS_RANGE_SIZE,
                                             pAddressRangeSize,
                                             sizeof(SET_1394_ADDRESS_RANGE_SIZE),
                                             pAddressRangeSize,
                                             sizeof(SET_1394_ADDRESS_RANGE_SIZE),
                                             &bytesReturned,
                                             NULL );
    
                            LocalFree( pAddressRangeSize );
                        }
                    }
                }
            }
            else 
            {
                moreDevices = false;
            }
                        
            memberIndex++;
        } 
    
        //
        // Free up the device info list
        //
        SetupDiDestroyDeviceInfoList( hDevInfo );
    }

    LeaveCriticalSection( &m_ExclusionObject );
}

//=============================================================================
// Function:    rad1394Master::ReadMemory
//=============================================================================
// Description: This member is invoked to read from the 1394 memory space.
//
// Parameters:  busNumber - bus number of device
//              nodeNumber - node number of device
//              addresss - 0 based offest of were to read. 
//              size     - number of bytes to read. 
//              dest     - where to copy
//              localRead - read local or remove slave
//
// Returns:     bool - true success
//                   - false - error
//
// Notes:
//------------------------------------------------------------------------------

bool rad1394Master::ReadMemory
( 
    unsigned int busNumber,
    unsigned int nodeNumber,
    unsigned int address,
    unsigned int size,
    void*        dest,
    bool         localRead
)
{
    //
    // First we must see if we have a valid device that matches the bus and node numbers.
    //
    HANDLE  devHandle = FindDevice( busNumber, nodeNumber );
       
    if( devHandle == NULL )
    {
        //
        // This was added to update devices as sometimes windows would not
        // inform us correctly.
        //
        UpdateDeviceInfo( );

        Sleep( 0 );

        return( false );
    }

    //
    // Through experimentation I found partioning the read into a bunch of small reads
    // worked substancially faster than allowing the 1394 driver to do it. Not sure why
    // but it works.
    //
    ASYNC_READ  AsyncRead;
    unsigned long bytesReturned;
    
    AsyncRead.bRawMode = false;
    AsyncRead.bGetGeneration = true;
    AsyncRead.DestinationAddress.IA_Destination_ID.NA_Bus_Number = busNumber;
    AsyncRead.DestinationAddress.IA_Destination_ID.NA_Node_Number = nodeNumber;
    AsyncRead.DestinationAddress.IA_Destination_Offset.Off_High = 0;
    AsyncRead.DestinationAddress.IA_Destination_Offset.Off_Low = address;
    AsyncRead.bLocalRead = localRead ? 1 : 0;
    AsyncRead.fulFlags = 0;
    AsyncRead.nBlockSize = 70;
    AsyncRead.ulGeneration = 0;

    //
    // Loop, breaking up the read into the partition. 
    //
    while( size )
    {
        if( size > RAD1394_MAX_READ_SIZE )
        {
           AsyncRead.nNumberOfBytesToRead = RAD1394_MAX_READ_SIZE;
        }
        else
        {
           AsyncRead.nNumberOfBytesToRead = size;
        }

        //
        // Issue the write request.
        //
        unsigned int result = DeviceIoControl( devHandle,
                                               RAD1394_IOCTL_READ,
                                               &AsyncRead,
                                               sizeof( ASYNC_READ ) + AsyncRead.nNumberOfBytesToRead - RAD1394_MAX_READ_SIZE,
                                               &AsyncRead,
                                               sizeof( ASYNC_READ ) + AsyncRead.nNumberOfBytesToRead - RAD1394_MAX_READ_SIZE,
                                               &bytesReturned,
                                               NULL );
        if( result == 0 )
        {
            return( false );
        }
        
        //
        // Copy the data
        //
        memcpy( dest, AsyncRead.Data, AsyncRead.nNumberOfBytesToRead );

        //
        // Update things and see if more to do.
        //
        size -= AsyncRead.nNumberOfBytesToRead;
        AsyncRead.DestinationAddress.IA_Destination_Offset.Off_Low += AsyncRead.nNumberOfBytesToRead;
        dest = (unsigned char*) dest + AsyncRead.nNumberOfBytesToRead;
    }
  
    return( true ); 

}

//=============================================================================
// Function:    rad1394Master::WriteMemory
//=============================================================================
// Description: This member is invoked to write to the 1394 memory space.
//
// Parameters:  busNumber - bus number of device
//              nodeNumber - node number of device
//              addresss - 0 based offest of were to read. 
//              size     - number of bytes to read. 
//              source     - where to copy
//
// Returns:     bool - true success
//                   - false - error
//
// Notes:
//------------------------------------------------------------------------------


bool rad1394Master::WriteMemory
( 
    unsigned int busNumber,
    unsigned int nodeNumber,
    unsigned int address,
    unsigned int size,
    void*        source
)
{
    //
    // First we must see if we have a valid device that matches the bus and node numbers.
    //
    HANDLE  devHandle = FindDevice( busNumber, nodeNumber );
       
    if( devHandle == NULL )
    {
        //
        // This was added to update devices as sometimes windows would not
        // inform us correctly.
        //
        UpdateDeviceInfo( );

        Sleep( 0 );

        return( false );
    }

    //
    // Through experimentation I found partioning the write into a bunch of small writes
    // worked substancially faster than allowing the 1394 driver to do it. Not sure why
    // but it works.
    //
    ASYNC_WRITE  AsyncWrite;
    unsigned long bytesReturned;
    
    AsyncWrite.bRawMode = false;
    AsyncWrite.bGetGeneration = true;
    AsyncWrite.DestinationAddress.IA_Destination_ID.NA_Bus_Number = busNumber;
    AsyncWrite.DestinationAddress.IA_Destination_ID.NA_Node_Number = nodeNumber;
    AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_High = 0;
    AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low = address;
    AsyncWrite.fulFlags = 0;
    AsyncWrite.nBlockSize = 0x0;
    AsyncWrite.ulGeneration = 0;

    //
    // Loop, breaking up the write into the partition.
    //
    while( size )
    {
        if( size > RAD1394_MAX_WRITE_SIZE )
        {
           AsyncWrite.nNumberOfBytesToWrite = RAD1394_MAX_WRITE_SIZE;
        }
        else
        {
           AsyncWrite.nNumberOfBytesToWrite = size;
        }

        memcpy( AsyncWrite.Data, source, AsyncWrite.nNumberOfBytesToWrite );

        //
        // Make sure we are not issueing writes too frequently. I found that I 
        // had to ensure 900 mircoseconds elasped between sends for optimal performance.
        // No idea as to why.
        //
        while( (radTimeGetMicroseconds( ) - m_LastWriteTime) < 900 )
        {
        }
        
        //
        // Issue the write request.
        //
        unsigned int result = DeviceIoControl( devHandle,
                                               RAD1394_IOCTL_WRITE,
                                               &AsyncWrite,
                                               sizeof( ASYNC_WRITE ) + AsyncWrite.nNumberOfBytesToWrite - RAD1394_MAX_WRITE_SIZE ,
                                               &AsyncWrite,
                                               sizeof( ASYNC_WRITE ) + AsyncWrite.nNumberOfBytesToWrite - RAD1394_MAX_WRITE_SIZE ,
                                               &bytesReturned,
                                               NULL );

        m_LastWriteTime = radTimeGetMicroseconds( );

        if( result == 0 )
        {
            return( false );
        }

        //
        // Update things and see if more to do.
        //
        size -= AsyncWrite.nNumberOfBytesToWrite;
        AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low += AsyncWrite.nNumberOfBytesToWrite;
        source = (unsigned char*) source + AsyncWrite.nNumberOfBytesToWrite;
    }
    return( true ); 
}

//=============================================================================
// Function:    rad1394Master::FindDevice
//=============================================================================
// Description: This member is invoked find a device that is valid and matches
//              the node and bus number.
//
// Parameters:  busNumber - bus number of device
//              nodeNumber - node number of device
//
// Returns:     NULL - device not found
//              handle - device found
//
// Notes:
//------------------------------------------------------------------------------

HANDLE rad1394Master::FindDevice
( 
    unsigned int busNumber, 
    unsigned int nodeNumber
)
{
    for( unsigned int i = 0 ; i < sizeof( m_DeviceInfo ) / sizeof( DeviceInfo ) ; i++ )
    {
        if( m_DeviceInfo[ i ].m_IsValid )
        {
            if( (m_DeviceInfo[ i ].m_busNumber == busNumber) && (m_DeviceInfo[ i ].m_nodeNumber == nodeNumber) )
            {
                //
                // We have a match. Return device handle.
                //
                return( m_DeviceInfo[ i ].m_DeviceHandle );
            }
        }
    }

    //
    // Here we failed. Return NULL.
    //
    return( NULL );
}

//=============================================================================
// Function:    rad1394Master::AddRef
//=============================================================================
// Description: Reference Management.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rad1394Master::AddRef
(
	void
)
{
	m_ReferenceCount++;
}

//=============================================================================
// Function:    rad1394Master::Release
//=============================================================================
// Description: Reference Management.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rad1394Master::Release
(
	void
)
{
	m_ReferenceCount--;

	if ( m_ReferenceCount == 0 )
	{
		delete this;
	}
}

