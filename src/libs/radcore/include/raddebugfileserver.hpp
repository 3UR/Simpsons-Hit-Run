//===========================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Debug file server
//
// Description: This file contains the public definitions related to the
//              Radical debugging file server.
//
// Authors:     Soleil Lapierre
//
//===========================================================================

// Recompilation protection flag.
#ifndef __RADDEBUGFILESERVER_H
#define __RADDEBUGFILESERVER_H

//=============================================================================
// Build Configuration Check
//=============================================================================

#if !defined(RAD_UWP) && !defined(RAD_WIN32)
    #error 'FTech requires definition of RAD_UWP or RAD_WIN32'
#endif

#ifdef RAD_WIN32                            // server only exists on Windows

//===========================================================================
// Nested Includes
//===========================================================================

#include <radobject.hpp>
#include <radmemory.hpp>
#include <radfile.hpp>
#include "..\src\radprotocols\fileprotocol.hpp"
#include <raddebugcommunication.hpp>


//===========================================================================
// Forward References
//===========================================================================

struct IRadDebugFileServer;
struct IRadDebugFileServerLogCallback;
struct IRadDebugFileServerFileCallback;


//=============================================================================
// Functions
//=============================================================================

// Initialize the file server system. Generally this function should 
// only be called once at program startup.
//
void radDebugFileServerInit( void );

//
// Create a file server object. The caller can specify the number of exceptions 
// (alternate files to be served) that the server can handle
//
void radDebugFileServerCreate( IRadDebugFileServer** pIRadDebugFileServer, int pNumExceptions );

//
// Shut down the file server system. The caller should still release it's references to 
// file servers.
// 
void radDebugFileServerTerminate( void );


//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description:	The file server interface provides the ability to listen for
//              debug network connections from targets (game systems) and to
//              serve files to those targets. The file server can act as a
//              shadow disk - a simulated game disk created from files 
//              on the hard disk of the host computer.
//
//              The typical sequence of events is:
//              1) The file server is started and configured.
//              2) The file server starts listening for a specific target to
//                 connect.
//              3) The target game console boots and connects to the server.
//              4) The game runs and performs normal file operations.
//              5) The debug comms layer redirects the file operations over
//                 the network to the file server.
//              6) The file server either performs the operations on its 
//                 local disk or directs the game to use the files on its
//                 own disk instead (this is configurable).
//              7) When the game terminates, the connection is closed.
//              8) The file server returns to listening or is shut down.
//
//              The file server interface also provides the ability to log
//              file requests and actions as they take place.
//
// Constraints: File serving is only available from Windows hosts, but any 
//              program that uses the debug communication layer can be a 
//              target (client).
//
//              The file server cannot use a target that is in use by
//              another file server on the same host computer.
//
//              There is a hack in place that redirects requests for
//              "bootload.cfg" to return user-specified data. 
//              This is to support test program selection on the PS/2.
//
//===========================================================================

struct IRadDebugFileServer : public IRefCount
{
    //
    // File system setup methods.
    //

    //
    // Set root path of shadow disk.
    // This method configures the location of the base directory containing
    // the files to be served to targets.
    //
    virtual void SetBasePath( char* pPath ) = 0;

    //
    // Set the default location of files.
    // If called with true, files will be served by the host file server
    // unless they match one of the exception strings. If set to false,
    // files will be loaded from the target's own disk unless they match
    // an exception.
    //
    virtual void SetDefaultHost( bool pLocal ) = 0;

    //
    // Set the content to be returned when the target requests "bootload.cfg".
    // The system contains a hack that allows the file "bootload.cfg" to
    // contain the name of an executable that should be loaded and run by
    // the target system. The file need not actually exist; the file server
    // fakes its contents using the string provided to this method.
    //
    virtual void SetBootData( unsigned char* pData, int pDataLength ) = 0;

    //
    // Add an exception string to the server's internal list.
    // An exception string is a path/file name or wildcard that specifies
    // files that are NOT served by the default host. The default host
    // is either the file server or the target, as set by SetDefaultHost.
    // The new exception will not be added if the internal list (table)
    // is already full. The table size is set by radDebugFileServerInit.
    //
    virtual void AddException( char* pString ) = 0;

    //
    // Remove an exception string from the server's internal list.
    // This deletes a previously added file exception string, and makes
    // room for a new one to be added. If the specified string is not
    // present, no action is taken. 
    //
    virtual void RemoveException( char *pString ) = 0;

    //
    // Clear all existing exception strings.
    // Use this when new server configuration data is loaded.
    //
    virtual void ClearExceptions( void ) = 0;

    //
    // Enable or disable absolute path overrides.
    // If this is called with a true argument, the file server will allow
    // targets to specify files by their path on the host file system
    // (eg, c:\temp). The host will try to access the file relative to the
    // base path first, and if that fails it will try the absolute path.
    // WARNING: When this enabled, the target could damage important files
    // on the host computer.
    //
    virtual void SetPathOverride( bool pEnable ) = 0;


    //
    // Event callback setup methods.
    // These are mainly intended for allowing the file server to notify the 
    // user program of state changes, for example so a debug GUI can update
    // its display for the user.
    //

    //
    // Set callback for log messages.
    // The file server can pass debugging messages to the application via
    // this logger interface. It is up to the callback what to do with the
    // log strings - display them, save them to a file etc.
    // If NULL is passed in, logging is disabled.
    // 
    virtual void RegisterLogCallback( IRadDebugFileServerLogCallback* pCallback ) = 0;

    //
    // Set callback for notifying the user program of file operations
    // performed by the target. The callback will be invoked when the target
    // makes a request, just after the file server has serviced the request.
    // This feedback is purely intended for informational purposes, and not
    // to allow the user program to override the file server's behavior.
    // An example task for the callback would be to call EnumerateOpenFiles 
    // and refresh a displayed list of open files whenever an open or close
    // operation occurs.
    //
    virtual void RegisterFileCallback( IRadDebugFileServerFileCallback* pCallback ) = 0;

    //
    // Set callback to be invoked when the target connection status changes.
    // The callback will be called when the target connection changes from
    // connected to disconnected or from disconnected to connected.
    //
    virtual void RegisterStatusCallback( IRadDbgComChannelStatusCallback* pCallback ) = 0;


    //
    // Public networking methods.
    //
    
    //
    // Attach to target.
    // Use this method to begin listening for a target connection. The file
    // path, exceptions and callbacks should be configured before calling.
    // False and an error message are returned on error.
    //
    virtual bool Attach( radDbgComTargetName pName, char* pErrorMessage ) = 0;

    //
    // Detach from target.
    // Use this method to close a target connection. It is safe to do so
    // even if there is no open connection.
    //
    virtual void Detach( void ) = 0;


    //
    // Accessors for getting info about the file server state.
    //

    //
    // List files currently known to be opened by the target system.
    // This will call the provided callback once per open file with a 
    // user-readable string containing info about the file.
    // 
    virtual void EnumerateOpenFiles( void (*pEnumerationCallback)( char* pName, char* pMessage ) ) const = 0;

    //
    // Accessor to get at the host target manipulation interface.
    // The user can use this interface to add and delete targets, get
    // the default target, and enumerate targets. The user must call
    // Release on the object when finished with it.
    //
    virtual void GetTargetInterface( IRadDbgComTargetTable** pTargetTable ) const = 0;

    //
    // Determine whether or not we are currently attaching to a target.
    // (Note this only reflects the attached/ing states, not whether or not
    //  a real target is communicating with us.)
    //
    virtual bool IsConnected( void ) const = 0;
};



// Callback interface definitions

//
// Implement this interface to receive log messages from the file server.
// Log messages are passed as user-readable strings.
//
struct IRadDebugFileServerLogCallback
{
    virtual void OnFileServerLogMessage( const char* pMessage ) = 0;
};


//
// Implement this interface to receive notification that the target
// system has performed a file operation. The callback function
// is passed the name of the relevant file, the operation
// performed and in the case of reads and writes, the file position and 
// data quantity affected. The callback does not process the request - 
// its purpose is only to inform the app that an action has occurred.
//
struct IRadDebugFileServerFileCallback
{
    virtual void OnTargetFileAction( char* pName, HfpCommand pCommand,
                                     int pPosition, int pLength ) = 0;
};



#endif                                  // RAD_WIN32 only

#endif                                  // End conditional inclusion

