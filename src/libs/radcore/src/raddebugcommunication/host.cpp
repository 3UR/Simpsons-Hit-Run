//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        host.cpp
//
// Subsystem:   Radical Debug Communication Host
//
// Description:	This file contains the implementation of the Debug Communication
//              host. 
//
// Author:		PGM
//
// Revisions:	Nov 6, 2000    Creation
//
// Notes:       This implementation is for Windows only. This system is thread
//              safe accross process boundaries. However, within a signle process,
//              this system is not thread safe and caller must insure access is
//              serialized.
//
//=============================================================================

#include "pch.hpp"
#ifdef RAD_WIN32

//=============================================================================
// Include Files
//=============================================================================

#include <windows.h>
#include <raddebug.hpp>
#include <raddebugcommunication.hpp>
#include <radmemorymonitor.hpp>
#include "protocol.hpp"
#include "host.hpp"              
#include "hostchannel.hpp"   

//=============================================================================
// Local Defintions
//=============================================================================

//
// This definines the name of the mutex we use for serializing access to the 
// Target information table.
//
#define MUTEX_NAME      "rDbgComHostMutex"

//
// Defines the name of the shared memory file used to hold target information
// table shared between processes.
//
#define MEMORYFILE_NAME "rDbgComHostMemoryFile"

//
// This constant defines the base of the registry key used to store target 
// configurations.
//
#define REGISTRY_KEY    "Software\\Radical Entertainment\\rDbgCom\\Targets"

//
// This is the name of our registered window class.
//
#define WINDOW_CLASSNAME "rDbgComWindowClass"

//
// This constant governs the maximum number of target definitions that can
// exist at any one time.
//
#define MAX_TARGETS     64

//
// This structure is used to define a target definition. These are maintained
// in shared memory accessable to any process accessing this DLL.
//
struct TargetInformation
{
    bool                    m_RecordIsValid;        // Record is valid defintion
    radDbgComTargetName     m_TargetName;           // Name
    radDbgComIPAddress      m_IpAddress;            // Ip Address
    unsigned int            m_Port;                 // Port;   
    unsigned int            m_IsDefault;            // True if default target
    unsigned short          m_Protocols[ MaxProtocols ]; // Zero is invalid protocol.
};

//=============================================================================
// Static Data Defintions
//=============================================================================

//
// This static maintains a pointer to the signleton debug com host.
//
static rDbgComHost*  s_theHost = NULL;

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    rDbgComHost::rDbgComHost
//=============================================================================
// Description: Constructor for the host object. Initialize things and gain
//              access to the shared memory object. This object accesses
//              memory which is shared between processes.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

rDbgComHost::rDbgComHost( void )
    :
    m_ReferenceCount( 1 ),
    m_MutexHandle( NULL ),
    m_MemoryFileHandle( NULL ),
    m_TargetTable( NULL ),
    m_hWnd( NULL )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "rDbgComHost" );

    //
    // First obtain access to the mutex we use to protect manipulation of 
    // shared memory.
    //
    m_MutexHandle = CreateMutex( NULL, FALSE, MUTEX_NAME );
    rAssert( m_MutexHandle != NULL );

    //
    // Now gain ownership of this mutex. Wait forever.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );

    //
    // Gain access to the shared memory containing our target information 
    // table. If it does not exist yet, we are responsible for initializing it.
    //
    m_MemoryFileHandle = CreateFileMapping( (HANDLE) 0xFFFFFFFF, NULL, PAGE_READWRITE, 0, 
                                            sizeof( TargetInformation ) * MAX_TARGETS,
                                            MEMORYFILE_NAME );
    rAssert( m_MemoryFileHandle  != NULL );

    //
    // Get the last error to see if the file already exists. If not, we are responsible
    // for initializing it.
    //
    bool AlreadyInitialized = (ERROR_ALREADY_EXISTS == GetLastError( ) );

    //
    // Map the view of the file into our process space.
    //
    m_TargetTable = (TargetInformation*) MapViewOfFile( m_MemoryFileHandle, FILE_MAP_WRITE, 0, 0, 0 );
    rAssert( m_TargetTable != NULL );

    if( !AlreadyInitialized )
    {
        //
        // Here we are responsible for initializing the shared memory. Do so now.
        // Lets set all records inintially not in use and set protocols to not in use.
        //
        for( unsigned int i = 0 ; i < MAX_TARGETS ; i++ )
        {
            m_TargetTable[ i ].m_RecordIsValid = false;    
            for( unsigned int j = 0 ; j < MaxProtocols ; j++ )
            {
                m_TargetTable[ i ].m_Protocols[ j ] = 0;
            }
        }

        //
        // Lets read up the taget settings from the registry.
        //
        HKEY RegKey;
        DWORD Disposition;
        LONG result = RegCreateKeyEx( HKEY_CURRENT_USER, 
                                      REGISTRY_KEY,
                                      0,
                                      NULL,
                                      REG_OPTION_NON_VOLATILE,
                                      KEY_ALL_ACCESS,
                                      NULL,  
                                      &RegKey,
                                      &Disposition );

        rAssert( result == ERROR_SUCCESS );
    
        //
        // Now eumrate the entries under this key.
        //
        DWORD Index = 0;
        do
        {
            DWORD       SubKeyLength;
            FILETIME    FileTime;
            
            SubKeyLength = sizeof( radDbgComTargetName );
            
            result = RegEnumKeyEx( RegKey, 
                                   Index, 
                                   m_TargetTable[ Index ].m_TargetName,
                                   &SubKeyLength,
                                   0,
                                   NULL,
                                   NULL,
                                   &FileTime );
      
            if( result == ERROR_SUCCESS )
            {
                //
                // We have found a defintions. Open the key to this target name.
                //
                HKEY TargetKey;
                DWORD ValueSize;
                
                RegOpenKeyEx( RegKey, 
                              m_TargetTable[ Index ].m_TargetName, 
                              0, KEY_ALL_ACCESS, &TargetKey );
                
                //
                // Read the three values we stored here.
                //
                ValueSize = sizeof( radDbgComIPAddress );
                RegQueryValueEx( TargetKey, "IPAddress", 0, NULL, 
                                 (unsigned char*) m_TargetTable[ Index ].m_IpAddress,   
                                 &ValueSize );

                ValueSize = sizeof( unsigned int );
                RegQueryValueEx( TargetKey, "Port", 0, NULL, 
                                 (unsigned char*) &m_TargetTable[ Index ].m_Port,   
                                 &ValueSize );

                ValueSize = sizeof( unsigned int );
                RegQueryValueEx( TargetKey, "Default", 0, NULL, 
                                 (unsigned char*) &m_TargetTable[ Index ].m_IsDefault,   
                                 &ValueSize );

                //
                // Close the key.
                //
                RegCloseKey( TargetKey );
                                
                m_TargetTable[ Index ].m_RecordIsValid = true;
            }

            Index++;
        }
        while( result == ERROR_SUCCESS );

        //
        // Close the key 
        //
        RegCloseKey( RegKey );

    }

    //
    // Since we have the shared memory visble in this process space and it has been
    // initialized, we can release the mutex.
    //
    ReleaseMutex( m_MutexHandle );

    //
    // Register a window class used to field windows messages directed for sockets.
    // First get the parent module instance.
    //
    char Filename[ 256 ];
    GetModuleFileName( NULL, Filename, sizeof(Filename) );
    m_hInstance = GetModuleHandle( Filename );

    WNDCLASSEX WindowClass;
    
    WindowClass.cbSize = sizeof( WNDCLASSEX );
    WindowClass.style = 0;
    WindowClass.lpfnWndProc = WindowProcedure;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 4;
    WindowClass.hInstance = m_hInstance;
    WindowClass.hIcon = NULL;
    WindowClass.hCursor = NULL;
    WindowClass.hbrBackground = NULL;
    WindowClass.lpszMenuName = NULL;
    WindowClass.lpszClassName = WINDOW_CLASSNAME;
    WindowClass.hIconSm = NULL;

    RegisterClassEx( &WindowClass );    
   
    //
    // Lets initialize sockets on for the calling processes.We want a socket implementation
    // of version 2.0 or greater. If not we will still use it.
    //
    WORD    wVersionRequested;
    WSADATA wsaData;
    int     err;
 
    wVersionRequested = MAKEWORD( 2, 0 );
    err = WSAStartup( wVersionRequested, &wsaData );

    rAssert( err == 0);

    s_theHost = this;
}

//=============================================================================
// Function:    rDbgComHost::~rDbgComHost
//=============================================================================
// Description: Destructor for the host object. Simply unintialize things for
//              the process.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

rDbgComHost::~rDbgComHost( void )
{
    //
    // Terminate our access to sockets for this process.
    //
    WSACleanup( );

    //
    // Remove our window class.
    //
    UnregisterClass( WINDOW_CLASSNAME, m_hInstance );

    //
    // Gain ownership of this mutex. Wait forever.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );

    //
    // Unmap view of the file and close it.
    //
    UnmapViewOfFile( m_TargetTable );

    CloseHandle( m_MemoryFileHandle );

    //
    // Release our mutex.
    //
    ReleaseMutex( m_MutexHandle );
            
    CloseHandle( m_MutexHandle );

    //
    // Finally clear the global indicating this system can be unloaded.
    //
    s_theHost = NULL;

}            

//=============================================================================
// Function:    rDbgComHost::AddTargetDefinition
//=============================================================================
// Description: Use this member to add a new target definition. If target defintion
//              already exists or is in use, then an error is returned.
//
//  Parameters: pName       - Target Name
//              pIpAddress  - Ip address to use.
//              port        - TCPIP port
//           
// Returns:     true - success, false otherwise.
//
// Notes:
//------------------------------------------------------------------------------

bool rDbgComHost::AddTargetDefinition
( 
    const radDbgComTargetName   pName,
    const radDbgComIPAddress    pIpAddress,
    unsigned short              port
)
{
    rAssert( strlen( pName ) != 0 );
    rAssert( strlen( pIpAddress ) != 0 );    

    //
    // Now gain ownership of this mutex. Wait forever.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );
    
    //
    // Verify that the target definition does not exist.
    //
    unsigned int freeEntry = MAX_TARGETS;
    for( unsigned int i = 0 ; i < MAX_TARGETS ; i++ )
    {   
        if( m_TargetTable[ i ].m_RecordIsValid )
        {
            if( 0 == stricmp( pName, m_TargetTable[ i ].m_TargetName ) )
            {
                //
                // Here we have the same name already defined. Return
                // false.
                //
                ReleaseMutex( m_MutexHandle );

                return( false );
            }
        }
        else
        {
            if( freeEntry == MAX_TARGETS )
            {
                freeEntry = i;
            }
        }
    }

    //
    // Here we did not find a duplicate name. Check if have a free entry.
    //
    if( freeEntry == MAX_TARGETS )
    {
        //
        // No room in the inn. Fail request.
        //
        ReleaseMutex( m_MutexHandle );

        return( false );
    }

    //
    // Save the callers info into the shared memory image and mark record in use.
    //
    strcpy( m_TargetTable[ freeEntry ].m_TargetName, pName );
    strcpy( m_TargetTable[ freeEntry ].m_IpAddress, pIpAddress );
    m_TargetTable[ freeEntry ].m_Port = port;
    m_TargetTable[ freeEntry ].m_IsDefault = 0;
    m_TargetTable[ freeEntry ].m_RecordIsValid = true;

    //
    // Write this record to the registry.
    //
    char RegistryString[ 256 ];
    strcpy( RegistryString, REGISTRY_KEY );
    strcat( RegistryString, "\\");
    strcat( RegistryString, pName );

    
    HKEY RegKey;
    DWORD Disposition;
    LONG result = RegCreateKeyEx( HKEY_CURRENT_USER, 
                                  RegistryString,
                                  0,
                                  NULL,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_ALL_ACCESS,
                                  NULL,  
                                  &RegKey,
                                  &Disposition );

    rAssert( result == ERROR_SUCCESS );

    RegSetValueEx( RegKey, "IPAddress", 0, REG_SZ, (const unsigned char*) pIpAddress, strlen( pIpAddress ) + 1 );
    RegSetValueEx( RegKey, "Port", 0, REG_DWORD, (const unsigned char*) &m_TargetTable[ freeEntry ].m_Port, sizeof( m_TargetTable[ freeEntry ].m_Port ) );
    RegSetValueEx( RegKey, "Default", 0, REG_DWORD, (const unsigned char*) &m_TargetTable[ freeEntry ].m_IsDefault, sizeof( m_TargetTable[ freeEntry ].m_IsDefault ) );

    RegCloseKey( RegKey );

    ReleaseMutex( m_MutexHandle );

    return( true );
}

//=============================================================================
// Function:    rDbgComHost::DeleteTargetDefinition
//=============================================================================
// Description: Use this member to delete a target definition. If must exist
//              and not be in use.
//
//  Parameters: pName       - Target Name
//           
// Returns:     true - success, false otherwise.
//
// Notes:
//------------------------------------------------------------------------------

bool rDbgComHost::DeleteTargetDefinition
( 
    const radDbgComTargetName pName 
)
{
    rAssert( strlen( pName ) != 0 );

    //
    // Now gain ownership of this mutex. Wait forever.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );
    
    //
    // Verify that the target definition does exist.
    //
    for( unsigned int i = 0 ; i < MAX_TARGETS ; i++ )
    {   
        if( m_TargetTable[ i ].m_RecordIsValid )
        {
            if( 0 == stricmp( pName, m_TargetTable[ i ].m_TargetName ) )
            {
                //
                // Here we have the record. Check that all protocols are
                // zero, indicating that the record is not in use.
                //
                for( unsigned int j = 0 ; j < MaxProtocols ; j++ )
                {
                    if( m_TargetTable[ i ].m_Protocols[ j ] != 0 )
                    {
                        //  
                        // The record is in use. Return false.
                        //
                        ReleaseMutex( m_MutexHandle );

                        return( false );
                    }
                }

                //
                // Here we delete the record. Set it to no longer in use and
                // remove it from the registry.
                //
                m_TargetTable[ i ].m_RecordIsValid = false;
        
                HKEY RegKey;
                DWORD Disposition;
                LONG result = RegCreateKeyEx( HKEY_CURRENT_USER, 
                                              REGISTRY_KEY,
                                              0,
                                              NULL,
                                              REG_OPTION_NON_VOLATILE,
                                              KEY_ALL_ACCESS,
                                              NULL,  
                                              &RegKey,
                                              &Disposition );

                rAssert( result == ERROR_SUCCESS );

                RegDeleteKey( RegKey, pName );
                      
                RegCloseKey( RegKey );

                //
                // Life is good. Return success
                //
                ReleaseMutex( m_MutexHandle );

                return( true );
            }
        }
    }

    //
    // If we get here, the record did not exist. Return false.
    //
    ReleaseMutex( m_MutexHandle );

    return( false );
}    

//=============================================================================
// Function:    rDbgComHost::EnumerateTargets
//=============================================================================
// Description: Use this member to enumerate target definitions
//
// Parameters:  void* user context.
//              pEnumerationCallback - function to invoke with target information,
//           
// Returns:     n/a
//
// Notes:       This routine locks teh mutex for the life of the call. This is 
//              not such a great idea as calling out while still holding a mutex
//              can cause deadlocks. Do not think this will occur however.
//------------------------------------------------------------------------------

void rDbgComHost::EnumerateTargets
( 
    void* context,
    void (*pEnumerationCallback)( void* context, const radDbgComTargetName pName, const radDbgComIPAddress pIpAddress, unsigned short port, bool InUse ) 
)
{
    //
    // Now gain ownership of this mutex. Wait forever.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );
    
    //
    // Traverse table looking for valid records.
    //
    for( unsigned int i = 0 ; i < MAX_TARGETS ; i++ )
    {
        if( m_TargetTable[ i ].m_RecordIsValid )    
        {
            //
            // Determine if record is currently in use.
            //
            bool InUse = false;
            for( unsigned int j = 0 ; j < MaxProtocols ; j++ )
            {
                if( m_TargetTable[ i ].m_Protocols[ j ] != 0 )
                {
                    InUse = true;
                    break;
                }
            }

            //
            // Invoke the callback with this record,
            //
            (pEnumerationCallback)( context,
                                    m_TargetTable[ i ].m_TargetName,
                                    m_TargetTable[ i ].m_IpAddress,
                                    m_TargetTable[ i ].m_Port,
                                    InUse );
        }               
    }
     
    ReleaseMutex( m_MutexHandle );

}

//=============================================================================
// Function:    rDbgComHost::GetDefaultTarget
//=============================================================================
// Description: Use this member to find the default target
//
// Parameters:  where to copy default
//           
// Returns:     true if found, false otherwise
//
// Notes:       
//------------------------------------------------------------------------------

bool rDbgComHost::GetDefaultTarget
(
    radDbgComTargetName pName
)
{
    //
    // Now gain ownership of this mutex. Wait forever.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );
    
    //
    // Traverse table looking for valid records.
    //
    for( unsigned int i = 0 ; i < MAX_TARGETS ; i++ )
    {
        if( m_TargetTable[ i ].m_RecordIsValid )    
        {
            if( m_TargetTable[ i ].m_IsDefault )
            {
                strcpy( pName, m_TargetTable[ i ].m_TargetName );

                ReleaseMutex( m_MutexHandle );

                return( true );
            }
        }
    }

    ReleaseMutex( m_MutexHandle );

    return( false );
}

//=============================================================================
// Function:    rDbgComHost::SetDefaultTarget
//=============================================================================
// Description: Use this member to set the default targer
//
// Parameters:  pName
//           
// Returns:     true if name found, false otherwise.
//
// Notes:       
//------------------------------------------------------------------------------

bool rDbgComHost::SetDefaultTarget
(
    const radDbgComTargetName pName
)
{

    //
    // First check if the specified target name exists. Do so under
    // mutex protection.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );
    
    unsigned int FoundTargetIndex = MAX_TARGETS;
    unsigned int OldDefaultIndex = MAX_TARGETS;

    for( unsigned int i = 0 ; i < MAX_TARGETS ; i++ )
    {   
        if( m_TargetTable[ i ].m_RecordIsValid )
        {
            if( 0 == stricmp( pName, m_TargetTable[ i ].m_TargetName ) )
            {
                //
                // Here have a match. Save index and we are done.
                //
                FoundTargetIndex = i;
            }
            
            if( m_TargetTable[ i ].m_IsDefault != 0 )
            {
                OldDefaultIndex = i;
            }    
        }
    }

    if( FoundTargetIndex == MAX_TARGETS )
    {
        //
        // Did not find name. Return false.
        //
        ReleaseMutex( m_MutexHandle );
    
        return( false );
    }

    //
    // Check if we need to clear an exist default target.
    //
    if( OldDefaultIndex != MAX_TARGETS )
    {
        m_TargetTable[ OldDefaultIndex ].m_IsDefault = 0;
        
        char RegistryString[ 256 ];
        strcpy( RegistryString, REGISTRY_KEY );
        strcat( RegistryString, "\\");
        strcat( RegistryString, m_TargetTable[ OldDefaultIndex ].m_TargetName );

    
        HKEY RegKey;
        DWORD Disposition;
        LONG result = RegCreateKeyEx( HKEY_CURRENT_USER, 
                                      RegistryString,
                                      0,
                                      NULL,
                                      REG_OPTION_NON_VOLATILE,
                                      KEY_ALL_ACCESS,
                                      NULL,  
                                      &RegKey,
                                      &Disposition );

        rAssert( result == ERROR_SUCCESS );

        RegSetValueEx( RegKey, "Default", 0, REG_DWORD, (const unsigned char*) &m_TargetTable[ OldDefaultIndex ].m_IsDefault, sizeof( m_TargetTable[ OldDefaultIndex ].m_IsDefault ) );

        RegCloseKey( RegKey );
    }

    //
    // Write the new dafault
    //
    m_TargetTable[ FoundTargetIndex ].m_IsDefault = 1;

    char RegistryString[ 256 ];
    strcpy( RegistryString, REGISTRY_KEY );
    strcat( RegistryString, "\\");
    strcat( RegistryString, m_TargetTable[ FoundTargetIndex ].m_TargetName );

    HKEY RegKey;
    DWORD Disposition;
    LONG result = RegCreateKeyEx( HKEY_CURRENT_USER, 
                                  RegistryString,
                                  0,
                                  NULL,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_ALL_ACCESS,
                                  NULL,  
                                  &RegKey,
                                  &Disposition );

    rAssert( result == ERROR_SUCCESS );

    RegSetValueEx( RegKey, "Default", 0, REG_DWORD, (const unsigned char*) &m_TargetTable[ FoundTargetIndex  ].m_IsDefault, sizeof( m_TargetTable[ FoundTargetIndex ].m_IsDefault ) );

    RegCloseKey( RegKey );

    ReleaseMutex( m_MutexHandle );

    return( true );
}


//=============================================================================
// Function:    rDbgComHost::CreateChannel
//=============================================================================
// Description: Use this member to create a host channel for communication, Must
//              specify a non-zero protocol and it must not be in use.
//
// Parameters:  TargetName      - which target to attach to.
//              protocol        - what protocol to use.
//              ppHostChannel   - returns a channel object to use for communcation.
//           
// Returns:     n/a
//
// Notes:       
//------------------------------------------------------------------------------

void rDbgComHost::CreateChannel
( 
    const radDbgComTargetName    pName,
    unsigned short               protocol, 
    IRadDbgComChannel**          ppHostChannel
)
{
    //
    // Validdate protocol
    //
    rAssert( (protocol >= 0xE000) && (protocol < 0xF000) );

    //
    // First check if the specified target name exists. Do so under
    // mutex protection.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );
    
    unsigned int FoundTargetIndex = MAX_TARGETS;

    for( unsigned int i = 0 ; i < MAX_TARGETS ; i++ )
    {   
        if( m_TargetTable[ i ].m_RecordIsValid )
        {
            if( 0 == stricmp( pName, m_TargetTable[ i ].m_TargetName ) )
            {
                //
                // Here have a match. Save index and we are done.
                //
                FoundTargetIndex = i;
                break;
            }
        }
    }

    if( FoundTargetIndex == MAX_TARGETS )
    {
        //
        // Target name not found. Return null 
        //
        *ppHostChannel = NULL;
    
        ReleaseMutex( m_MutexHandle );

        return;
    }

    //
    // Verify we have a free protocol slot and that the specified protocol
    // is not in use.
    //
    unsigned int FreeProtocolIndex = MaxProtocols;
    for( unsigned int j = 0 ; j < MaxProtocols ; j++ )
    {
        if( m_TargetTable[ FoundTargetIndex ].m_Protocols[ j ] == protocol )
        {
            //  
            // The record is in use. Return null
            //
            *ppHostChannel = NULL;

            ReleaseMutex( m_MutexHandle );
            return;
        }
        else if( m_TargetTable[ FoundTargetIndex ].m_Protocols[ j ] == 0 )
        {
            FreeProtocolIndex = j;
        }
    }
    
    if( FreeProtocolIndex == MaxProtocols )
    {
        //
        // All protocols in use.Fail
        //
        *ppHostChannel = NULL;
    
        ReleaseMutex( m_MutexHandle );

        return;
    }

    //
    // Set the protocol entry in the table and construct the channel object.
    //
    m_TargetTable[ FoundTargetIndex ].m_Protocols[ FreeProtocolIndex ] = protocol;

    //
    // Mutex can be release now since we have updated shared resources.
    //
    ReleaseMutex( m_MutexHandle );
    
    *ppHostChannel = new rDbgComHostChannel( this, FoundTargetIndex, protocol );
}


//=============================================================================
// Function:    rDbgComHost::AddRef
//=============================================================================
// Description: This member is used to update the reference count of the object.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
    
void rDbgComHost::AddRef( void )
{
    m_ReferenceCount++;
}

//=============================================================================
// Function:    rDbgComHost::Release
//=============================================================================
// Description: This member is used to update the reference count of the object.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
    
void rDbgComHost::Release( void )
{
    m_ReferenceCount--;
    if( m_ReferenceCount == 0 )
    {
        delete this;
    }   
}

//=============================================================================
// Function:    rDbgComHost::CreateWindowHelper
//=============================================================================
// Description: This member will create a window on behalf of the cleint channel
//
// Parameters:  pChannel  
//
// Returns:     HWND
//
// Notes:
//------------------------------------------------------------------------------

HWND rDbgComHost::CreateWindowHelper
( 
    rDbgComHostChannel* pChannel 
)
{
    m_hWnd = CreateWindowEx( WS_EX_NOPARENTNOTIFY, WINDOW_CLASSNAME, "", WS_POPUP, 0, 0, 0, 0, 
                    NULL, NULL, m_hInstance, (void*) pChannel );

    return m_hWnd;
}

//=============================================================================
// Function:    rDbgComHost::DestroyWindowHelper
//=============================================================================
// Description: This member will destroy a window. Exists for symetry
//
// Parameters:  hWnd  
//
// Returns:     
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComHost::DestroyWindowHelper( HWND hWnd )
{
    DestroyWindow( hWnd );
}

//=============================================================================
// Function:    rDbgComHost::WindowProcedure
//=============================================================================
// Description: This static member is responsible for directing windows messages
//              to the appropiate handler.
//
// Parameters:  See Windows
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

LRESULT CALLBACK rDbgComHost::WindowProcedure
( 
    HWND    hwnd, 
    UINT    uMsg, 
    WPARAM  wParam,
    LPARAM  lParam
)
{
    //
    // Check if create window. If so, lParam contains the this pointer of 
    // the channel we should direct the window message to.
    //
    if( uMsg == WM_NCCREATE )
    {
        //
        // Set the windows instance data.
        //
        SetWindowLong( hwnd, 0, (long) (((CREATESTRUCT*) lParam)->lpCreateParams) );

        return( DefWindowProc(hwnd, uMsg, wParam, lParam) );
    }

    //
    // Get the this pointer to the channel associated with this window.
    //
    rDbgComHostChannel* pChannel = (rDbgComHostChannel*) GetWindowLong( hwnd, 0 );

    return( pChannel->OnWindowMessage( hwnd, uMsg, wParam, lParam ) );
}

//=============================================================================
// Function:    rDbgComHost::GetIpAddress
//=============================================================================
// Description:
//
// Parameters: 
//
// Returns:     
//
// Notes:
//------------------------------------------------------------------------------

const char* rDbgComHost::GetIpAddress
( 
    unsigned int targetIndex
)
{
    return( m_TargetTable[ targetIndex ].m_IpAddress );
}

//=============================================================================
// Function:    rDbgComHost::GetPort
//=============================================================================
// Description:
//
// Parameters: 
//
// Returns:     
//
// Notes:
//------------------------------------------------------------------------------

unsigned short rDbgComHost::GetPort
( 
    unsigned int targetIndex
)
{
    return( m_TargetTable[ targetIndex ].m_Port );
}

//=============================================================================
// Function:    rDbgComHost::FreeProtocol
//=============================================================================
// Description: Frees as protooc.
//
// Parameters:  target index and protocol
//           
// Returns:     n/a
//
// Notes:       
//------------------------------------------------------------------------------

void rDbgComHost::FreeProtocol
( 
    unsigned int    targetIndex,
    unsigned short  protocol
)
{
    //
    // Now gain ownership of this mutex. Wait forever.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );
    
    //
    // Traverse table looking for the protocol.
    //
    for( unsigned int j = 0 ; j < MaxProtocols ; j++ )
    {
        if( m_TargetTable[ targetIndex ].m_Protocols[ j ] == protocol )
        {
            m_TargetTable[ targetIndex ].m_Protocols[ j ] = 0;
                   
            ReleaseMutex( m_MutexHandle );
            return;
        }
    }

    //
    // If we get here things are fucked up.
    //
    rAssert( false );
}

//=============================================================================
// Function:    rDbgComHost::Service
//=============================================================================
// Description: Service debug com host by explicitly service window message.
//
// Parameters:  
//           
// Returns:     n/a
//
// Notes:       
//------------------------------------------------------------------------------

void rDbgComHost::Service( void )
{

    if ( m_hWnd == NULL  )
    {
        return;
    }

    MSG msg;

    if ( PeekMessage( &msg, m_hWnd, 0, 0, PM_REMOVE ) ) 
    {
        TranslateMessage( &msg );

        //
        // We have a message. Normally, we would check for a quit message. We
        // use our own termination system instead.
        //
        DispatchMessage( &msg );
    }
}

//=============================================================================
// Function:    radDbgComHostGetTargetTable
//=============================================================================
// Description: Get the interface to the target defintion table.
//
// Parameters:  
//           
// Returns:     n/a
//
// Notes:       
//------------------------------------------------------------------------------

void radDbgComHostGetTargetTable( IRadDbgComTargetTable** pIRadDbgComTargetTable )
{
    if( s_theHost != NULL )
    {
        radAddRef( s_theHost, NULL );
    }
    else
    {
        s_theHost = new rDbgComHost( );
    }       

    *pIRadDbgComTargetTable = s_theHost;
}

//=============================================================================
// Function:    radDbgComHostCreateChannel
//=============================================================================
// Description: Create a host side communication channel.
//
// Parameters:  
//           
// Returns:     n/a
//
// Notes:       
//------------------------------------------------------------------------------

void radDbgComHostCreateChannel
( 
    const radDbgComTargetName pName,
    unsigned short protocol, 
    IRadDbgComChannel** ppChannel
)
{
    rDbgComHost* pTargetTable;
    
    radDbgComHostGetTargetTable( (IRadDbgComTargetTable**) &pTargetTable );

    pTargetTable->CreateChannel( pName, protocol, ppChannel );

    radRelease( pTargetTable, NULL );
}

void radDbgComHostService( void )
{
    if ( s_theHost != NULL )
    {
        s_theHost->Service( );
    }
}


#endif