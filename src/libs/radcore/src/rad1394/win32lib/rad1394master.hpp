//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        rad1394Master.hpp
//
// Subsystem:	Radical 1394 Firewire Master
//
// Description:	This file contains all definitions and classes relevant to
//              the radical 1394 master. This module is the Win32 side of the
//              PS2 Firewire implementation. It provides very simple services
//              that can be used to read and write memory on the PS2IOP over
//              Firewire.
//
//              This is a module is the bridge between the application and the
//              Win2k device driver.
//
// Revisions:	27-June-2001 Creation
//
// Notes:       
//
//=============================================================================

#ifndef	RAD1394MASTER_HPP
#define RAD1394MASTER_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <rad1394.hpp>
#include <radobject.hpp>
#include <radmemory.hpp>
#include <windows.h>

//=============================================================================
// Forward Class Declarations
//=============================================================================

//=============================================================================
// Defintions
//=============================================================================

//=============================================================================
// Class Declarations
//=============================================================================

//
// This class represents the object used to communicated with WIN2k device driver.
//
class rad1394Master : public IRad1394Master
					
{
    public:

    //
    // Constructor/Destructor.
    //
    rad1394Master( unsigned int sharedMemorySize );
    virtual ~rad1394Master( void );
  
	//
	// IRad1394Master Interfaces
	//
    virtual bool ReadMemory( unsigned int busNumber, unsigned int nodeNumber, unsigned int address,
                             unsigned int size, void* dest, bool localRead );

    virtual bool WriteMemory( unsigned int busNumber, unsigned int nodeNumber, unsigned int address,
                              unsigned int size, void* source );

    virtual void AddRef( void );
    virtual void Release( void );

    private:

    //
    // Static function run as a thread to monitor device changes and its window
    // procedure.
    //
    static unsigned int DeviceMontiorThread( rad1394Master* pMaster );
    static LRESULT CALLBACK DeviceMonitorWindowProcedure( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

    //
    // Member to update the current device info.
    //
    void UpdateDeviceInfo( void );

    //
    // Helper function to find a device that matches the bus and node number.
    //
    HANDLE FindDevice( unsigned int busNumber, unsigned int nodeNumber );

    //
    // This structure is used to manage the list of active devices on the 1394 bus. When
    // ever we get notifcation about a change in device status we update entries in this table.
    // When communicating, we use this to make sure the dervice is present.
    //
    struct DeviceInfo
    {
        bool            m_IsValid;              // True if record is in use and contains valid data
        char            m_DeviceName[ 256 ];    // Name of the device.
        HANDLE          m_DeviceHandle;         // Handle of device object to send requests to.
        unsigned int    m_busNumber;            // Bus number of where dervice can be found.
        unsigned int    m_nodeNumber;           // Node number of the device.
    };

    //
    // Here is the table. It is maitained by a by a background thread monitoring for device
    // changes.
    //
    DeviceInfo           m_DeviceInfo[ 4 ];     

    //
    // These are used by the background thread used to monitor device changes.
    //
    CRITICAL_SECTION    m_ExclusionObject;
    HANDLE              m_ThreadHandle;
    HWND                m_DeviceMonitorWindow;

    //
    // This member maintains the reference count of this object.
    //
    unsigned int m_ReferenceCount;    

    //
    // This is used to slow done the rate a which we perform writes.
    //
    unsigned int m_LastWriteTime;

    //
    // Size of local shared memory.
    //
    unsigned int m_SharedMemorySize;
    
};

#endif


