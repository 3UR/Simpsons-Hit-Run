//===========================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Debug file server.
//
// Description: This file contains the implementation of the debug file
//              server for Windows. 
//
// Authors:     Peter Mielcarski, Soleil Lapierre
//
//===========================================================================

#include "pch.hpp"
#ifdef RAD_WIN32			// server only exists on Windows

//---------------------------------------------------------------------------
// Includes
//===========================================================================

#include <stdio.h>
#include <io.h>
#include <raddebugfileserver.hpp> // interface specification
#include <radmemorymonitor.hpp>
#include "fileserver.hpp"         // class specification


//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================


//===========================================================================
// Static Data Definitions
//===========================================================================

//
// Indicates whether radDebugFileServerInit has been called.
//
static bool s_Initialized = false;


//===========================================================================
// Global Functions
//===========================================================================


//===========================================================================
// radDebugFileServerInit
//===========================================================================
// Description: Initialize the file server system. Currently does nothing 
//              and is provided only for completeness, and for future
//              expansion. Generally this function should only be called 
//              once at program startup.
//
// Constraints:	None.
//
// Parameters:	pNumExceptions - the max # of exception strings to handle.
//
//===========================================================================

void radDebugFileServerInit( void )
{
    s_Initialized = true;
}


//===========================================================================
// radDebugFileServerCreate
//===========================================================================
// Description: Obtains a pointer to the file server singleton.
//
// Constraints:	The file server should be initialized by calling
//              radDebugFileServerInit before calling this function.
//
// Parameters:	pIRadDebugFileServer - the reference is returned via
//                                      this parameter.
//
//              pNumExceptions - the max # of exception strings to handle.
//
//===========================================================================

void radDebugFileServerCreate( IRadDebugFileServer** pIRadDebugFileServer, int pNumExceptions)
{
    rAssert( s_Initialized );
    rAssert( pIRadDebugFileServer != NULL );

    *pIRadDebugFileServer = new rDebugFileServer( pNumExceptions );
}


//===========================================================================
// radDebugFileServerTerminate
//===========================================================================
// Description: Shut down the file server system. Currently does nothing 
//              and is provided only for completeness, and for future
//              expansion.
//
//===========================================================================

void radDebugFileServerTerminate( void )
{
    s_Initialized = false;
}


//===========================================================================
// Member Functions
//===========================================================================

//===========================================================================
// rDebugFileServer::rDebugFileServer
//===========================================================================
// Description: Constructor. Initializes internam variables and allocates
//              buffers for exception strings.
//
// Parameters:	pNumExceptions - maximum number of exception strings the
//                  server must be able to handle. Since the file server is 
//                  not intended to run on console systems, this can be set 
//                  as large as needed, but typically not many exceptions
//                  are needed.
//
//===========================================================================

rDebugFileServer::rDebugFileServer( int pNumExceptions = 100 )
    :
    m_ReferenceCount( 1 ),
    m_OpenFiles( NULL ),
    m_OpenSearches( NULL ),
    m_BootData( NULL ),
    m_BootDataSize( 0 ),
    m_DefaultHost( true ),
    m_ExceptionCount( 0 ),
    m_Exceptions( NULL ),
    m_Override( false ),
    m_Attached( false ),
    m_Host( NULL ),
    m_Channel( NULL ),
    m_TxOutstanding( false ),
    m_TxPendingBytes( 0 ),
    m_LogCallback( NULL ),
    m_FileCallback( NULL ),
    m_StatusCallback( NULL )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "rDebugFileServer" );
    strcpy( m_TargetName, "" );
    strcpy( m_Path, "" );

    //
    // Allocate one exception even if none requested; simplifies logic.
    //
    m_ExceptionCount = pNumExceptions;
    if( m_ExceptionCount < 1 )
    {
        m_ExceptionCount = 1;
    }

    m_Exceptions = new ExceptionRule[ m_ExceptionCount ];
    rAssert( m_Exceptions != NULL );

    ClearExceptions( );

    // 
    // Working directory will serve as the initial base path.
    //
    GetCurrentDirectory( 256, m_Path );

    //
    // Get target table interface.
    //
    radDbgComHostGetTargetTable( &m_Host );
    rAssert( m_Host != NULL );
}



//===========================================================================
// rDebugFileServer::~rDebugFileServer
//===========================================================================
// Description: This is the private destructor for this file server
//              implementation. It cleans up memory allocated within the
//              class.
//
//              Users should call Release instead in order to preserve 
//              references.
//
//===========================================================================

rDebugFileServer::~rDebugFileServer( void )
{
    Detach( );

    if( m_Host != NULL )
    {
        radRelease( m_Host, this );
    }

    if( m_Channel != NULL )
    {
        radRelease( m_Channel, this );
    }

    if( m_BootData != NULL )
    {
        delete [ ] m_BootData;
    }

    if( m_Exceptions != NULL )
    { 
        delete [ ] m_Exceptions;
    }
}


//===========================================================================
// rDebugFileServer::SetBasePath
//===========================================================================
// Description: Sets the path to the local directory that will serve as the
//              root directory for files requested by targets. 
//
// Constraints:	The base path should always be set before attaching the
//              server to a target. The initial default is the host program's
//              working directory.
//
//              It is the user's responsibility to make sure the base path
//              is a valid directory name.
//
// Parameters:	pPath - the path to use as the root directory.
//
//===========================================================================

void rDebugFileServer::SetBasePath( char* pPath )
{
    rAssert( pPath != NULL );
    strcpy( m_Path, pPath );
  
    //
    // Log this change.
    //
    char buf[ 512 ];
    wsprintf( buf, "Base path set to: %s", pPath );
    LogMessage( buf );
}


//===========================================================================
// rDebugFileServer::SetDefaultHost
//===========================================================================
// Description: Sets a flag that controls whether files are served from 
//              the host or target by default. If set to true, files will
//              be served from the host unless there is an applicable
//              exception rule. If set to false, files will be served from
//              the target's local disk unless there is an applicable
//              exception rule. The default is true.
//
// Parameters:	pLocal - true if host should serve files by default
//
//===========================================================================

void rDebugFileServer::SetDefaultHost( bool pLocal )
{
    m_DefaultHost = pLocal;

    //
    // Log this change.
    //
    if( pLocal )
    {
        LogMessage( "Host is default file source." );
    } 
    else 
    {
        LogMessage( "Target is default file source." );
    }
}


//===========================================================================
// rDebugFileServer::SetBootData
//===========================================================================
// Description: Set the data that should be returned to the target when
//              an attempt is made to read "bootload.cfg". 
//
//              This capability is a hack put in place to support test 
//              programs that can load multiple executables on the console
//              target systems. At startup, these programs request a file
//              called "bootload.cfg" which typically contains the name of
//              the executable that should be loaded by the target. 
//              
//              The file "bootload.cfg" does not really exist on the host
//              system because we do not want to pollute the shadow disk
//              space with non-game files. This method is provided to allow 
//              the contents of the virtual file to be specified. The caller
//              can provide an arbitrary data buffer that will be treated
//              as the (read-only) contents of bootload.cfg.
//
//              The data is copied into a private buffer by the file server,
//              so the caller does not need to preserve it after the call.
//
// Constraints:	Although the boot data will typically by a simple file name
//              string, the capability is provided to handle arbitrary data.
//              Therefore the buffer is not assumed to be a null-terminated
//              string. To be safe, when passing in a string the caller should
//              add 1 to the length parameter to ensure the terminating 
//              null character is included in the buffer.
//
// Parameters:	pData - pointer to the file content data
//              pDataSize - the length of the buffer
//
//===========================================================================

void rDebugFileServer::SetBootData( unsigned char* pData, int pDataLength )
{
    //
    // Free any previously existing data.
    //
    if( m_BootData != NULL )
    {
        delete [ ] m_BootData;
    }

    //
    // Allocate and fill the new buffer if desired.
    //
    if( pDataLength > 0 && pData != NULL )
    {
        m_BootData = new unsigned char[ pDataLength ];
        rAssert( m_BootData != NULL );
        m_BootDataSize = pDataLength;
        memcpy( m_BootData, pData, pDataLength );

        //
        // Log this change.
        //
        char buf[ 512 ];
        wsprintf( buf, "%d bytes of boot data loaded.", pDataLength );
        LogMessage( buf );
    }
    else
    {
        m_BootData = NULL;
        m_BootDataSize = 0;
    }
}


//===========================================================================
// rDebugFileServer::AddException
//===========================================================================
// Description: Add an exception rule for determining where files should
//              be served from.
//
//              An exception rule is a string that contains a single file 
//              name, possibly with a path and wildcard characters. It 
//              identifies a file or set of files that should be treated
//              in a manner opposite to that specified by the default-host
//              flag - that is, if files are served by the host by default,
//              then files that match an exception rule are not served by
//              the host.
//
// Constraints:	The maximum number of active exception strings is fixed at 
//              the time the file server object is created. 
//
//              No precendence order is enforced on the rules; they are
//              tested in an arbitrary order.
//
// Parameters:	pString - a new exception rule to add
//
//===========================================================================

void rDebugFileServer::AddException( char* pString )
{
    rAssert( m_ExceptionCount > 0 );

    if( pString != NULL )
    {
        if( strlen( pString ) > 0 )
        {
            //
            // Search for an empty slot for the rule.
            //
            int index = 0;
            bool found = false;
            while( !found && index < m_ExceptionCount )
            {
                if( strlen( m_Exceptions[ index ] ) == 0 )
                {
                    found = true;
                }
                else
                {
                    index++;
                }
            }

            //
            // Insert the new rule.
            //
            if( found )
            {
                strcpy( m_Exceptions[ index ], pString );

                //
                // Log this addition.
                //
                char buf[ 512 ];
                wsprintf( buf, "Exception rule added: %s", pString );
                LogMessage( buf );
            }
        }
    }
}


//===========================================================================
// rDebugFileServer::RemoveException
//===========================================================================
// Description: Remove an exception rule. This method deletes the first
//              exception rule string that is identical to the one provided.
//              If there is no match, no action is taken. If there are 
//              multiple identical rules, only the first match is deleted.
//
// Parameters:	pString - the rule to delete
//
//===========================================================================

void rDebugFileServer::RemoveException( char *pString )
{
    rAssert( m_ExceptionCount > 0 );

    if( pString != NULL )
    {
        if( strlen( pString ) > 0 )
        {
            //
            // Search for the first match and delete it.
            //
            bool found = false;
            int index = 0;
            while( !found && index < m_ExceptionCount )
            {
                if( _stricmp( m_Exceptions[ index ], pString ) == 0 )
                {
                    found = true;
                    m_Exceptions[ index ][ 0 ] = '\0';

                    //
                    // Log the deletion.
                    //
                    char buf[ 512 ];
                    wsprintf( buf, "Exception rule removed: %s", pString );
                    LogMessage( buf );
                }
                index++;
            }
        }
    }
}


//===========================================================================
// rDebugFileServer::ClearExceptions
//===========================================================================
// Description: Remove all exception rules. This method deletes all
//              existing exception rule strings without deallocating the
//              exception table. Use this method when the server program
//              wants to reset the exception list without restarting the
//              whole file server - ie, when a new config is loaded.
//
//===========================================================================

void rDebugFileServer::ClearExceptions( void )
{
    for( int index = 0; index < m_ExceptionCount; index++ )
    {
        m_Exceptions[ index ][ 0 ] = '\0';
    }
}



//===========================================================================
// rDebugFileServer::SetPathOverride
//===========================================================================
// Description: Enable or disable file path overriding by targets.
//
//              If true is passed to this method, targets will be able
//              to access files by their absolute path names on the host
//              computer. If a file is to be opened on the host end, and
//              if the file is not found by using its name relative to
//              the base path, then the host will try to find it using
//              its name without the base path prepended.
//
// Constraints:	There is no protection against targets trying to write to
//              existing files on the host, so take care not to overwrite
//              any critical files in your target programs.
//
//              There is limited delete protection - targets cannot delete
//              files outside of the base path and its subdirectory tree.
//
// Parameters:	pEnable - true to enable absolute path trial
//
//===========================================================================

void rDebugFileServer::SetPathOverride( bool pEnable )
{
    m_Override = pEnable;

    //
    // Log this config change.
    //
    if( pEnable )
    {
        LogMessage( "Path override enabled." );
    }
    else
    {
        LogMessage( "Path override disabled." );
    }
}


//===========================================================================
// rDebugFileServer::RegisterLogCallback
//===========================================================================
// Description: Register a function to be called by the file server when it
//              has a log message for the user to read.
//          
//              The callback function will be called with a server-generated,
//              user-readable string. It is up to that function to determine 
//              what to do with the string - ie, display it on the screen or
//              write it to a log file.
//
//              Pass in NULL to disable logging.
//
// Constraints: By registering a long-term non-reference-counted callback,
//              the caller is guaranteeing that class and function will 
//              be persistent. If the class instance owning the callback 
//              function is going to be deleted, the callback MUST first
//              be unregistered by reregistering it as NULL.
//
// Parameters:	pCallback - a log interface instance
//
//===========================================================================

void rDebugFileServer::RegisterLogCallback( IRadDebugFileServerLogCallback* pCallback )
{
    m_LogCallback = pCallback;
}


//===========================================================================
// rDebugFileServer::RegisterFileCallback
//===========================================================================
// Description: Register a function to be called whenever the target performs
//              a file operation.
//
//              The callback function will be called whenever a file 
//              operation request comes in from the target. The callback is 
//              provided with the relevant filename, the type of operation
//              requested, and in the case of reads and writes, the offset
//              and length of data read or written. 
//
//              Note that the file server answers these requests internally.
//              The purpose of this callback is informational only - so that
//              the server application can notify the user of what is going
//              on. If it is desired to modify the server's behavior in
//              response to target requests, new functionality will need to
//              be added.
//
// Constraints: By registering a long-term non-reference-counted callback,
//              the caller is guaranteeing that class and function will 
//              be persistent. If the class instance owning the callback 
//              function is going to be deleted, the callback MUST first
//              be unregistered by reregistering it as NULL.
//
// Parameters:	pCallback - a file notification interface instance
//
//===========================================================================

void rDebugFileServer::RegisterFileCallback( IRadDebugFileServerFileCallback* pCallback )
{
    m_FileCallback = pCallback;
}


//===========================================================================
// rDebugFileServer::RegisterStatusCallback
//===========================================================================
// Description: Register a function to be called when the target connection
//              state changes.
//
//              The callback function will be called with the new state value
//              when it changes. This action is for informational purposes,
//              for example so that the application program can diable 
//              GUI controls that are not relevant to the new state.
//
// Constraints: By registering a long-term non-reference-counted callback,
//              the caller is guaranteeing that class and function will 
//              be persistent. If the class instance owning the callback 
//              function is going to be deleted, the callback MUST first
//              be unregistered by reregistering it as NULL.
//
// Parameters:	pCallback - a channel status notification interface instance
//
//===========================================================================

void rDebugFileServer::RegisterStatusCallback( IRadDbgComChannelStatusCallback* pCallback )
{
    m_StatusCallback = pCallback;
}


//===========================================================================
// rDebugFileServer::Attach
//===========================================================================
// Description: Begin listening for the target.
//
//              This method starts the file server's main functionality.
//              It begins listening for the specified target to connect,
//              and when that occurs it begins handling file requests from
//              that target.
//
// Constraints: All important server state information should be set up before
//              calling this method. That includes specifying the base path,
//              the default host flag, the path override flag, any desired
//              exception rules, and any desired bootload.cfg content.
//
// Parameters:	pName - the target to listen for
//              pMessage - a place to store a user-readable error message if
//                         some problem occurs
//
// Return:      Returns true on success, false on error.
//
//===========================================================================

bool rDebugFileServer::Attach( radDbgComTargetName pName, char* pErrorMessage )
{
    rAssert( m_Host != NULL );

    if( !m_Attached )
    {
        strcpy( m_TargetName, pName );

        //
        // We are now attaching.
        // Check configuration data for validity.
        //
        bool configOK = true;
        configOK = configOK && ( strlen( m_TargetName ) > 0 );
        configOK = configOK && ( strlen( m_Path ) > 0 );

        if( !configOK )
        {
            if( pErrorMessage != NULL )
            {
                strcpy( pErrorMessage, "Cannot attach without target name and base path." );
            }
            LogMessage( "Attach attempt failed: Insufficient data." );
            return( false );
        }
  
        //
        // Get a channel.
        //
        if( m_Channel != NULL )
        {
            radRelease( m_Channel, this );
            m_Channel = NULL;
        }

        radDbgComHostCreateChannel( m_TargetName, HOST_FILE_PROTOCOL ,&m_Channel );
      
        if( m_Channel == NULL )
        {
            if( pErrorMessage != NULL )
            {
                strcpy( pErrorMessage, "Target may already be in use by a file server" );
            }
            LogMessage( "Attach attempt failed: Target in use." );
            return( false );
        }
 
        //
        // Initiate the connection.
        //
        m_Channel->RegisterStatusCallback( this );
        m_Channel->Attach( );

        //
        // Log the attachment.
        //
        LogMessage( "Attach attempt succeeded." );
        LogMessage( "Waiting for target connections." );
        m_Attached = true;      
    }
    else
    {
        if( pErrorMessage != NULL )
        {
            strcpy( pErrorMessage, "Already attached!" );
        }
        return( false );
    }

    return( true );
}


//===========================================================================
// rDebugFileServer::Detach
//===========================================================================
// Description: Stop listening for target connections, close any open 
//              target connections, and close all open files. Use this to
//              forcibly close the target connection without shutting
//              down the server.
//
//===========================================================================

void rDebugFileServer::Detach( void )
{
    if( m_Attached )
    {
        m_Attached = false;
        
        m_Channel->Detach( );
        radRelease( m_Channel, this );
        m_Channel = NULL;

        CloseFiles( );    

        if( m_StatusCallback != NULL )
        {
            m_StatusCallback->OnStatusChange( IRadDbgComChannel::Detached, "Disconnected" );
        }
        LogMessage( "Detached." );
    }
}


//===========================================================================
// rDebugFileServer::EnumerateOpenFiles
//===========================================================================
// Description: List all files currently open by the target. The listing
//              action is performed by calling the user-supplied callback
//              function once for each open file.
//
//              The callback function receives the name of each open
//              file, with an additional string that the server may use
//              to provide user-readable details about the file.
//
//              This functionality is provided as a polling alternative
//              to the event-driven file tracking provided by registering
//              a file event callback.
//
// Constraints: The file server currently only tracks files opened on the
//              host end. Tracking file operations at the target end would
//              require modifying the radFile implementation.
//
// Parameters:	pCallback - function to call with each file's info
//
//===========================================================================

void rDebugFileServer::EnumerateOpenFiles( void (*pEnumerationCallback)( char* pName, char* pMessage ) ) const
{
    rAssert( pEnumerationCallback != NULL );

    //
    // Loop over all open files.
    //
    TargetFileList* listItem = m_OpenFiles;
    while( listItem != NULL )
    {
        //
        // Generate an informational message about the file
        //
        char message[ 512 ];
        strcpy( message, listItem->m_Flags );

        if( listItem->m_WasOverride )
        {
            strcat( message, " (override)" );
        }

        //
        // Pass the file info back to the caller.
        //
        (*pEnumerationCallback)( listItem->m_FileName, message );
        listItem = listItem->m_Next;
    }
}


//===========================================================================
// rDebugFileServer::GetTargetInterface
//===========================================================================
// Description: Obtain a reference to the server's target table. This
//              interface can be used to enumerate available targets,
//              obtain the user's default target name, and to add and delete
//              target definitions.
//
// Constraints: The interface is reference counted. The AddRef is done here,
//              and the caller must call Release on the interface when
//              finished with it.
//
// Parameters:	pTargetTable - store the interface handle here.
//
//===========================================================================

void rDebugFileServer::GetTargetInterface( IRadDbgComTargetTable** pTargetTable ) const
{
    rAssert( m_Host != NULL );
    rAssert( pTargetTable != NULL );
    *pTargetTable = m_Host;
    radAddRef( m_Host, const_cast< rDebugFileServer * >( this ) );
}


//===========================================================================
// rDebugFileServer::IsConnected
//===========================================================================
// Description: This accessor returns true if the host file server it
//              currently connected to a target. 
//
// Constraints: It is difficult to accurately track whether or not a
//              genuine target conversation is occurring, so for now this
//              accessor just returns true if the file server has its port
//              open waiting for a target to connect, or if the target is
//              connected. Essentially the return is true between a call to
//              Attach and the subsequent call to Detach.
//
// Return:  	true if a connection is active
//
//===========================================================================

bool rDebugFileServer::IsConnected( void ) const
{
    return( m_Attached );
}


//===========================================================================
// rDebugFileServer::OnStatusChange
//===========================================================================
// Description: This is a private method of the file server that is used
//              to receive notification of connection state changes.
//
// Parameters:	connectionState - new connection status
//              Message - error message reported by comms layer
//
//===========================================================================
void rDebugFileServer::OnStatusChange( IRadDbgComChannel::ConnectionState connectionState,
                                  const char* Message )
{
    //
    // Pass message on to logger.
    //
    if( Message != NULL )
    {
        LogMessage( "Connection state changed." );
        LogMessage( (char *)Message );
    }
    
    //
    // Decide what to do about it.
    //
    switch( connectionState )
    {
        case IRadDbgComChannel::Attaching :
        {
            if( m_StatusCallback != NULL )
            {
                m_StatusCallback->OnStatusChange( IRadDbgComChannel::Attaching, "Attaching" );
            }
            CloseFiles( );
            break;
        }

        case IRadDbgComChannel::Detaching :
        {
            if( m_StatusCallback != NULL )
            {
                m_StatusCallback->OnStatusChange( IRadDbgComChannel::Detaching, "Detaching" );
            }
            CloseFiles( );
            break;
        }
                
        case IRadDbgComChannel::Attached :
        {
            if( m_StatusCallback != NULL )
            {
                m_StatusCallback->OnStatusChange( IRadDbgComChannel::Attached, "Connected" );
            }

            //
            // Issue a receive to get things rolling.
            //
            m_Channel->ReceiveAsync( m_ReceiveBuffer, sizeof( m_ReceiveBuffer), this );

            break;
        }

        case IRadDbgComChannel::Detached :
        {
            if( m_Attached )
            {
                m_Attached = false;

                CloseFiles( );    

                if( m_StatusCallback != NULL )
                {
                    m_StatusCallback->OnStatusChange( IRadDbgComChannel::Detached, "Disconnected" );
                }
            }

            break;
        }
    }
}


//===========================================================================
// rDebugFileServer::OnSendComplete
//===========================================================================
// Description: This is a private method of the file server used to receive
//              notification of the completion of send operations.
//
// Parameters:	Successful - true if the last send finished OK.
//
//===========================================================================
void rDebugFileServer::OnSendComplete( bool Successful )
{
    m_TxOutstanding = false;

    if( Successful )
    {
        //
        // Check if pending bytes to send. Initiate if so.
        //
        if( m_TxPendingBytes != 0 )
        {
            m_TxOutstanding = true;
            m_Channel->SendAsync( m_SendBuffer, m_TxPendingBytes, this );
            m_TxPendingBytes = 0;
        }
    }
	else
	{
  	    LogMessage( "WARNING: Send failed." );
	}
}


//===========================================================================
// rDebugFileServer::OnReceiveComplete
//===========================================================================
// Description: This is a private method of the file server used to obtain
//              notification of the completion of a network receive operation.
//
// Parameters:	Successful - false if there was a networking error
//              bytesReceived - the number of bytes received
//
//===========================================================================
void rDebugFileServer::OnReceiveComplete( bool Successful, unsigned int bytesReceived )
{
    if( Successful )
    {
        int bytesToSend;

        //
        // Handle the target's request. This is where the actual server work
        // transpires.
        //
        ProcessFileRequest( m_ReceiveBuffer, bytesReceived, m_SendBuffer, &bytesToSend );
        
        if( bytesToSend != 0 )
        {
            //
            // Here we have data to send. Make sure the previous send has completed. If not
            // set pending bytes to send.
            //
            if( !m_TxOutstanding )
            {
                m_TxPendingBytes = 0;
                m_TxOutstanding = true;
                m_Channel->SendAsync( m_SendBuffer, bytesToSend, this );
            }
            else
            {
                m_TxPendingBytes = bytesToSend;
            }
        }

        //
        // Set up receiver for next target request.
        //
        m_Channel->ReceiveAsync( m_ReceiveBuffer, sizeof( m_ReceiveBuffer), this );

    } 
}


//===========================================================================
// rDebugFileServer::QualifyFilename
//===========================================================================
// Description: This private function converts a target's requested file name
//              to a fully qualified file name on the host machine. It does
//              this by concatenating the target's file name on to the end
//              of the host's base path. Note that path overrides are handled
//              elsewhere.
//
// Parameters:	pFilename - the target's file name
//              pFullpath - buffer to store host file name in
//
//===========================================================================

void rDebugFileServer::QualifyFilename( char* pFilename, char* pFullpath )
{
    rAssert( pFilename != NULL );
    rAssert( pFullpath != NULL );
    rAssert( strlen( m_Path ) > 0 );

    int len = strlen( m_Path );
    strcpy( pFullpath, m_Path);

    if( pFullpath[ len - 1 ] != '\\' )
    {
        pFullpath[ len ] = '\\';
        pFullpath[ len + 1] = '\0';
    }

    strcat( pFullpath, pFilename );
}



//===========================================================================
// rDebugFileServer::CheckIfFileOnHost
//===========================================================================
// Description: This private function determines whether a target's requested
//              file should be opened on the host file server or on the
//              target's local disk. This is where exception rules are
//              processed.
//
// Constraints: The exception matching is fairly primitive - it does not
//              understand full regular expressions or complex wildcard
//              strings.
//
//              This function does not check to see if the file actually 
//              exists; that is a task for when the file is actually opened,
//              since the file SHOULD exist where the rules say.
//
// Parameters:	filename - target's requested file name (not fully qualified)
//
// Return:      true if the host should serve up the file
//              false if the file should be opened on the target's disk.
//
//===========================================================================

bool rDebugFileServer::CheckIfFileOnHost( char* filename )
{
    //
    // HACK: Say bootload.cfg is always on host.
    //
    if( _stricmp( filename, "bootload.cfg") == 0 )
    {
        return( true );
    }

    //
    // First check if we have any exceptions.
    //
    if( m_ExceptionCount == 0 )
    {
        //
        // No exceptions. Return default answer.
        //
        return( m_DefaultHost );
    }
    else
    {
        for( int i = 0 ; i < m_ExceptionCount; i++ )
        {
            //
            // Check for a match in the exception list.
            //
            if( strlen( m_Exceptions[ i ] ) > 0 ) 
            {
                if( StringMatch( m_Exceptions[ i ], filename ) )
                {  
                    //
                    // Here we have a match. Return opposite of default.
                    //
                    return( !m_DefaultHost );
                }
            }
        }

        //
        // No match. Just return default answer,
        //
        return( m_DefaultHost );
    }            
}


//===========================================================================
// rDebugFileServer::StringMatch
//===========================================================================
// Description: This private function performs matching of exception rules
//              against target file names. This is what determines whether
//              or not a target request matches an exception string.
//
// Constraints: This is not a regular expression parser. Only simple
//              wildcard matching is done.
//
// Parameters:	MatchName - the rule to test against.
//              Name - the file name to test.
//
// Return:      true on match.
//
//===========================================================================

bool rDebugFileServer::StringMatch( char* MatchName, char* Name)
{
    long x1=0, x2=0;

    //
    // Loop over filename string.
    //
    while( Name[ x2 ] != '\0' )
    {
        if( Name[ x2 ] == '.' )
        {
            if( Name[ x2+1 ] == '.' )
            {
                if( MatchName[ x1 ] != '*' )
                {
                    if( ( MatchName[ x1 ] != '.' ) || ( MatchName[ x1+1 ] != '.' ) ) 
                    {
                        return false;
                    }
                    x1 = x1 + 2;
                }
                x2++;
            }
            else
            {
                if( MatchName[ x1 ] == '*' ) 
                {
                    x1++;
                }
                if( MatchName[ x1 ] != '.' ) 
                {
                    return false;
                }
                x1++;
            }       
        }       
        else if( MatchName[ x1 ] != '*' )
        {
            if( toupper( Name[ x2 ] ) != toupper( MatchName[ x1 ] ) ) 
            {
                return false;
            }
            x1++;
        }
        x2++;
    }

    if( MatchName[ x1 ] == '\0' ) 
    {
        return true;
    }

    if( ( MatchName[ x1 ] == '*' ) && ( MatchName[ x1+1 ] == '\0' ) )
    {
        return true;
    }

    return false;
}


//===========================================================================
// rDebugFileServer::ProcessFileRequest
//===========================================================================
// Description: This private function is the main dispatch handler for 
//              incoming file operation requests from the target system.
//              This is where the work actually gets done.
//
// Parameters:	request - the raw packet received from the target
//              requestSize - the length of the packet
//              reply - store reply packet here
//              replySize - length of the reply packet
//
//===========================================================================

void rDebugFileServer::ProcessFileRequest( char* request, int requestSize, char* reply, int* replySize )
{
    char filespec[ 512 ];  // Buffer for full path name.
    char buf[ 512 ];       // Buffer for constructing messages.
    (void) requestSize;

    switch( *((HfpCommand*) request) )
    {
        //
        // Request: Open a file.
        //
        case HfsOpen :
        {
            HfpOpenCmd* pCmd = (HfpOpenCmd*) request;    
            HfpOpenRpy* pRpy = (HfpOpenRpy*) reply;    

            //
            // Log the event.
            //
            wsprintf( buf, "REQ: Open %s", pCmd->m_FileName );
	        LogMessage( buf );

            //
            // Check if we should open on the host.
            //
            if( CheckIfFileOnHost( pCmd->m_FileName ))
            {
	            LogMessage ("File to be opened on host.");

                //
                // Here we are opening the file on the host.
                // Determine the file permissions to use.
                //
                char flags[ 16 ];

                if( pCmd->m_WriteAccess )
                {
                    if( pCmd->m_Flags == CreateAlways )
                    {
                        strcpy( flags, "w+b" );
                    }
                    else
                    {
                        strcpy( flags, "r+b" );
                    }
                }
                else 
                {
                    strcpy( flags, "rb" );
                }
            

                //
                // HACK for bootload.cfg: "open" virtual file, and embeding argc and argv
                // for that executable.
                //
                if( _stricmp( pCmd->m_FileName, "bootload.cfg" ) == 0 ) 
                {
	                LogMessage( "Virtual file opened OK." );
                    LogMessage( "File handle is DEADBEEF." );

                    //
                    // Note: The file handle 0xDEADBEEF has no special meaning.
                    // In order for things to work, we just need a file handle 
                    // that is not one of the flag values 0 or -1 and does not 
                    // duplicate an existing handle. Since DEADBEEF is odd
                    // and most memory alloc routines return a word boundary 
                    // aligned address, this value should present no problems.
                    //
                    pRpy->m_Handle = (void*)0xDEADBEEF;  
                    pRpy->m_Size = m_BootDataSize;

                    //
                    // Save internal info about this open file.
                    //
                    TargetFileList *fileInfo = new TargetFileList;
                    rAssert( fileInfo != NULL );
                    fileInfo->m_LocalHandle = NULL; // Important - acts as a flag.
                    fileInfo->m_RemoteHandle = pRpy->m_Handle;
                    strcpy( fileInfo->m_FileName, pCmd->m_FileName );
                    fileInfo->m_WasOverride = false;
                    strcpy( fileInfo->m_Flags, flags );
                    fileInfo->m_Next = m_OpenFiles;
                    m_OpenFiles = fileInfo;

                    //
                    // Notify application that file was opened.
                    //
                    if( m_FileCallback != NULL )
                    {
                        m_FileCallback->OnTargetFileAction( pCmd->m_FileName, HfsOpen, 0, 0 );
                    }

                    pRpy->m_Command = pCmd->m_Command;
                    *replySize = sizeof( HfpOpenRpy );
            
                    break;
                }
                
                //
                // Not a virtual file - generate fully qualified local filename.
                //
                QualifyFilename( pCmd->m_FileName, filespec );

                //
                // Try to open the file under the base path.
                //
                FILE* pFile = NULL;
                pFile = fopen( filespec, flags );                

                if( pFile != NULL )
                {
                    //
                    // Success - log the event.
                    //
	                LogMessage( "File opened OK." );
                    wsprintf( buf, "File handle is %p.", pFile );
                    LogMessage( buf );

                    //
                    // Return file object pointer and add file to open list.
                    //
                    pRpy->m_Handle = pFile;
                    pRpy->m_Size = _filelength( _fileno( pFile ) );                    

                    TargetFileList *fileInfo = new TargetFileList;
                    rAssert( fileInfo != NULL );
                    fileInfo->m_LocalHandle = pFile;
                    fileInfo->m_RemoteHandle = pRpy->m_Handle;
                    strcpy( fileInfo->m_FileName, pCmd->m_FileName );
                    fileInfo->m_WasOverride = false;
                    strcpy( fileInfo->m_Flags, flags );
                    fileInfo->m_Next = m_OpenFiles;
                    m_OpenFiles = fileInfo;

                    //
                    // Notify application that file was opened.
                    //
                    if( m_FileCallback != NULL )
                    {
                        m_FileCallback->OnTargetFileAction( pCmd->m_FileName, HfsOpen, 0, 0 );
                    }
                }
                else
                {
                    //
                    // Failed to open. Check if we allow override of the path.
                    //
                    if( m_Override )
                    {
                        //
                        // Log this action.
                        //
    	                LogMessage( "File not opened." );
                        LogMessage( "Trying path override." );

                        //
                        // Try the filename directly.
                        //
                        pFile = fopen( pCmd->m_FileName, flags );

                        if( pFile != NULL )
                        {
                            //
                            // Override succeeded - log it.
                            //
	                        LogMessage( "Override succeeded." );
                            wsprintf( buf, "File handle is %p.", pFile );
                            LogMessage( buf );

                            //
                            // Return file object pointer and add file to open list.
                            //
                            pRpy->m_Handle = pFile;
                            pRpy->m_Size = _filelength( _fileno( pFile ) );
                        
                            TargetFileList *fileInfo = new TargetFileList;
                            rAssert( fileInfo != NULL );
                            fileInfo->m_LocalHandle = pFile;
                            fileInfo->m_RemoteHandle = pRpy->m_Handle;
                            strcpy( fileInfo->m_FileName, pCmd->m_FileName );
                            fileInfo->m_WasOverride = true;
                            strcpy( fileInfo->m_Flags, flags );
                            fileInfo->m_Next = m_OpenFiles;
                            m_OpenFiles = fileInfo;

                            //
                            // Notify application of file open.
                            //
                            if( m_FileCallback != NULL )
                            {
                                m_FileCallback->OnTargetFileAction( pCmd->m_FileName, HfsOpen, 0, 0 );
                            }                
                        } 
                        else // Override failed.
                        {
	                        LogMessage( "Override failed." );
                            LogMessage( "ERROR: Failed to open file on host." );
                            pRpy->m_Handle = (void*)-1;
                        }
                    }
                    else // Override disabled.
                    {
	                    LogMessage( "ERROR: Failed to open file on host." );
                        pRpy->m_Handle = (void*)-1;
                    }

                }    
            }
            else // Open on target instead.
            {
	            LogMessage( "File to be opened on target." );

                //
                // Here we return a flag saying that the file will be opened on 
                // the remote (target) drive.
                //
                pRpy->m_Handle = 0;
            }

            pRpy->m_Command = pCmd->m_Command;
            *replySize = sizeof( HfpOpenRpy );
            
            break;
        }    


        //
        // Request: Read from an open file.
        //
        case HfsRead :
        {
            FILE* pFile = NULL;
            HfpReadCmd* pCmd = (HfpReadCmd*) request;    
            HfpReadRpy* pRpy = (HfpReadRpy*) reply;    
            TargetFileList *listItem = m_OpenFiles;

            //
            // Find the open file handle.
            //
            while( listItem != NULL )
            {
                if( pCmd->m_Handle == listItem->m_RemoteHandle )
                {
                    pFile = listItem->m_LocalHandle;
                    break;
                }
                else
                {
                    listItem = listItem->m_Next;
                }
            }

            //
            // HACK for bootload.cfg reads - simulate from memory.
            //
            if( listItem != NULL )
            {
                //
                // Log the request.
                //
                wsprintf( buf, "REQ: Read %u bytes @ %u from %s", 
                            pCmd->m_NumBytes, pCmd->m_Position, listItem->m_FileName );
	            LogMessage( buf );

                if( _stricmp( listItem->m_FileName, "bootload.cfg" ) == 0 )
                {
                    LogMessage( "Reading from virtual file." );

                    //
                    // Calculate section of memory buffer to be read from.
                    //
                    int numBytes = pCmd->m_NumBytes;
                    if( m_BootDataSize - (signed int)pCmd->m_Position < numBytes )
                    {
                        numBytes = m_BootDataSize - pCmd->m_Position;
                    }

                    //
                    // Fill in reply data fields.
                    //
                    pRpy->m_NumBytes = pCmd->m_NumBytes;
                    memset( pRpy->m_Data, 0, pCmd->m_NumBytes );
                    if( numBytes > 0 )
                    {
                        memcpy( pRpy->m_Data, m_BootData, numBytes );
                    }
                    
                    pRpy->m_Command = pCmd->m_Command;
                    *replySize = sizeof( HfpReadRpy ) - HFP_MAX_READWRITE + pCmd->m_NumBytes;

                    //
                    // Notify application of the read operation.
                    //
                    if( m_FileCallback != NULL )
                    {
                        m_FileCallback->OnTargetFileAction( listItem->m_FileName, HfsRead, 
                                                    pCmd->m_Position, pCmd->m_NumBytes );
                    }                
                    break;    
                }
            }

            //
            // File not open.
            //
            if( pFile == NULL )
            {
                //
                // Log the request.
                //
                wsprintf( buf, "REQ: Read %u bytes @ %u from fh %X", 
                            pCmd->m_NumBytes, pCmd->m_Position, pCmd->m_Handle );
	            LogMessage( buf );
                LogMessage( "ERROR: Handle not found." );
                *replySize = 0;
                return;
            }

            //
            // We have a read. The second word contains the number of bytes read. Always return
            // number rquested, even in error. Seek to position first. This operation should be
            // failsafe from the target's point of view, though incorrect data could result
            // from a failure. Error conditions are passed to the application's logger for the
            // user to see.
            //
            if( fseek( pFile, (long)(pCmd->m_Position), SEEK_SET ) != 0 )
            {
                LogMessage( "ERROR: Seek failure." );
            }

            pRpy->m_NumBytes = (unsigned int)fread( &pRpy->m_Data[ 0 ], 1, (size_t)(pCmd->m_NumBytes), pFile );
            if( pRpy->m_NumBytes != pCmd->m_NumBytes )
            {
                wsprintf( buf, "ERROR: Read %u should be %u bytes.", pRpy->m_NumBytes, pCmd->m_NumBytes);
                LogMessage( buf );
                if( feof( pFile ) != 0 ) 
                {
                    LogMessage( "WARNING: Read past EOF." );
                }
                pRpy->m_NumBytes = pCmd->m_NumBytes;
            }
                    
            pRpy->m_Command = pCmd->m_Command;
            *replySize = sizeof( HfpReadRpy ) - HFP_MAX_READWRITE + pCmd->m_NumBytes;

            //
            // Log event and notify application of read operation.
            //
            wsprintf( buf, "Read %d bytes from %s.", pCmd->m_NumBytes, listItem->m_FileName );
            LogMessage( buf );

            if( m_FileCallback != NULL )
            {
                m_FileCallback->OnTargetFileAction( listItem->m_FileName, HfsRead, 
                                                    pCmd->m_Position, pCmd->m_NumBytes );
            }                
            break;    
        }


        //
        // Request: Close an open file.
        //
        case  HfsClose :
        {
            FILE* pFile = NULL;
            HfpCloseCmd* pCmd = (HfpCloseCmd*) request;    
            HfpCloseRpy* pRpy = (HfpCloseRpy*) reply;    
            TargetFileList *listItem = m_OpenFiles;
            TargetFileList *prevItem = NULL;

            //
            // Search for the file handle.
            //
            while( listItem != NULL )
            {
                if( pCmd->m_Handle == listItem->m_RemoteHandle )
                {
                    pFile = listItem->m_LocalHandle;
                    break;
                }
                else
                {
                    prevItem = listItem;
                    listItem = listItem->m_Next;
                }
            }

            //
            // File not open.
            //
            if( listItem == NULL )
            {
                //
                // Log the request.
                //
                wsprintf( buf, "REQ: Close file %X.", pCmd->m_Handle );
	            LogMessage( buf );

	            LogMessage ("ERROR: Handle not found.");
                *replySize = 0;
                return;
            }

            // 
            // Log this event.
            //
            wsprintf( buf, "REQ: Close file %s", listItem->m_FileName );
            LogMessage( buf );
              
            //
            // Set up reply to target.
            //
            pRpy->m_Command = pCmd->m_Command;
            *replySize = sizeof( HfpCloseRpy );

            //
            // Notify user application of action.
            //
            if( m_FileCallback != NULL )
            {
                m_FileCallback->OnTargetFileAction( listItem->m_FileName, HfsClose, 0, 0 );
            }                

            //
            // Close the file.
            // Note: The hack for bootload.cfg implicit in the logic here. fclose is the
            // only operation in this block that doesn't apply to the virtual file.
            //
            if( pFile != NULL ) 
            {
                fclose( pFile );
            }

            //
            // Unlink and delete the file info struct from the private list 
            // of open files.
            //
            if( prevItem == NULL )
            {
                m_OpenFiles = listItem->m_Next;
            }
            else
            {
                prevItem->m_Next = listItem->m_Next;
            }
            delete listItem;
            listItem = NULL;
               
            break;    
        }


        //
        // Request: Write data to an open file.
        //
        case HfsWrite :
        {
            FILE* pFile = NULL;
            HfpWriteCmd* pCmd = (HfpWriteCmd*) request;    
            HfpWriteRpy* pRpy = (HfpWriteRpy*) reply;    
            TargetFileList *listItem = m_OpenFiles;

            // 
            // Locate the file handle.
            //
            while( listItem != NULL )
            {
                if( pCmd->m_Handle == listItem->m_RemoteHandle )
                {
                    pFile = listItem->m_LocalHandle;
                    break;
                }
                else
                {
                    listItem = listItem->m_Next;
                }
            }

            //
            // File not open.
            //
            if( listItem == NULL )
            {
                //
                // Log this request.
                //
                wsprintf( buf, "REQ: Write %u bytes @ %u in fh %X.", 
                            pCmd->m_NumBytes, pCmd->m_Position, pCmd->m_Handle );
	            LogMessage( buf );

	            LogMessage ("ERROR: Handle not found.");
                *replySize = 0;
                return;
            }

            //
            // Log this event.
            //
            wsprintf( buf, "REQ: Write %u bytes @ %u in %s", 
                        pCmd->m_NumBytes, pCmd->m_Position, listItem->m_FileName );
            LogMessage( buf );

            //
            // We have a write. Errors are simply logged.
            // Need to track possible growth of file here.
            //
            unsigned int newLength = 0;

            //
            // HACK for bootload.cfg: writes not allowed.
            // The write is only actually performed if this is a real file.
            //
            if( pFile != NULL )
            {
                if( fseek( pFile, pCmd->m_Position, SEEK_SET ) != 0 )
                {
                    LogMessage( "ERROR: Seek failure." );
                }

                unsigned int bytesWritten = fwrite( &pCmd->m_Data[ 0 ], 1, pCmd->m_NumBytes, pFile );
                if( bytesWritten != pCmd->m_NumBytes )
                {
                    wsprintf( buf, "ERROR: Write %u should be %u bytes.", bytesWritten, pCmd->m_NumBytes);
                    LogMessage( buf );
                }
                fflush( pFile );
                newLength = _filelength( _fileno( pFile ) );
                wsprintf( buf, "New file length is %u.", newLength );
                LogMessage( buf );
            }
            else
            {
                pRpy->m_NewSize = m_BootDataSize;
            }

            //
            // Return the new file length that the target wants to see, even
            // if there was a write failure.
            //
            if( pCmd->m_Position + pCmd->m_NumBytes > newLength ) 
            {
                newLength = pCmd->m_Position + pCmd->m_NumBytes;
                wsprintf( buf, "Corrected file length is %u.", newLength );
                LogMessage( buf );
            }
            pRpy->m_NewSize = newLength;
            pRpy->m_NumBytes = pCmd->m_NumBytes;
            pRpy->m_Command = pCmd->m_Command;
            *replySize = sizeof( HfpWriteRpy ); 

            //
            // Notify user application of action.
            //
            if( m_FileCallback != NULL )
            {
                m_FileCallback->OnTargetFileAction( listItem->m_FileName, HfsWrite, 
                                                    pCmd->m_Position, pCmd->m_NumBytes );
            }                
            break;    
        }


        //
        // Request: Delete a file on the host.
        //
        case HfsDestroy :
        {
            HfpDestroyCmd* pCmd = (HfpDestroyCmd*) request;    
            HfpDestroyRpy* pRpy = (HfpDestroyRpy*) reply;    

            //
            // Log this request.
            //
            wsprintf( buf, "REQ: Delete file %s.", pCmd->m_FileName );
	        LogMessage( buf );

            char filespec[ 256 ];
            QualifyFilename( pCmd->m_FileName, filespec );
            //
            // NOTE that targets can't delete their local files, so we don't check
            // to see where the file is - we just assume it is on the host.
            //

            //
            // HACK for bootload.cfg: Just fake it.
            //
            bool result = false;
            if( _stricmp( pCmd->m_FileName, "bootload.cfg" ) == 0 )
            {
                result = true; 
            }
            else
            {
                result = ( ::DeleteFile( filespec ) != 0 );
            }

            if( result )
            {
                wsprintf( buf, "Destroyed file %s on host.", pCmd->m_FileName );
                LogMessage( buf );

                pRpy->m_Result = 0;
            }
            else
            {
                //
                // We don't allow deletion of overridden paths, so just fail
                // if the delete operation failed.
                wsprintf( buf, "ERROR (%X): Failed to Destroy %s on host", GetLastError( ), pCmd->m_FileName );
                LogMessage( buf );
                pRpy->m_Result = 1;
            }

            //
            // NOTE: We don't check to see if the destroyed file was on the list of open
            // files because the target should close a file before destroying it.
            // There isn't much point in checking for the error because normal games
            // won't be deleting files.
            //

            pRpy->m_Command = pCmd->m_Command;
            *replySize = sizeof( HfpDestroyRpy );

            //
            // Notify user application of action.
            //
            if( m_FileCallback != NULL )
            {
                m_FileCallback->OnTargetFileAction( pCmd->m_FileName, HfsDestroy, 0, 0 );
            }                
            break;
        }    
       
        //
        // Request: Start a search.
        //
        case HfsFindFirst :
        {
            HfpFindFirstCmd* pCmd = (HfpFindFirstCmd*) request;    
            HfpFindFirstRpy* pRpy = (HfpFindFirstRpy*) reply;    

            //
            // Log the event.
            //
            wsprintf( buf, "REQ: Begin search %s", pCmd->m_SearchSpec );
	        LogMessage( buf );

            //
            // Convert search spec to local path setup.
            //
            QualifyFilename( pCmd->m_SearchSpec, filespec );

            //
            // Create a local search record.
            //
            TargetSearchList* search = new TargetSearchList;
            rAssert( search != NULL );
            search->m_Handle = INVALID_HANDLE_VALUE;
            search->m_SearchSpec[ 0 ] = '\0';
            search->m_Next = NULL;

            //
            // Initiate a local FindFirst
            //
            bool failed = false;
            bool noMatch = false;
            DWORD lastErrorCode = ERROR_SUCCESS;
            SetLastError( ERROR_SUCCESS );
            search->m_Handle = ::FindFirstFile( filespec, &( search->m_FindData ) );

            //
            // Determine if a failure was because of no matches or a real error.
            //
            if( search->m_Handle == INVALID_HANDLE_VALUE )
            {
                lastErrorCode = ::GetLastError( );
                if( lastErrorCode == ERROR_NO_MORE_FILES || 
                    lastErrorCode == ERROR_FILE_NOT_FOUND ||
                    lastErrorCode == ERROR_PATH_NOT_FOUND )
                {
                    noMatch = true;
                }
                else
                {
                    failed = true;
                }
            }

            //
            // If there was no error, formulate a response packet and save the search info.
            //
            if( !failed )
            {
                //
                // Success - log the event.
                //
                wsprintf( buf, "Search ID is %X.", (unsigned int)search );
                LogMessage( buf );

                //
                // Return search result.
                //
                strcpy( search->m_SearchSpec, pCmd->m_SearchSpec );
                pRpy->m_Handle = (unsigned int) search;
                if( noMatch )
                {
                    LogMessage( "No match found." );
                    strcpy( pRpy->m_Name, "" );
                    pRpy->m_Type = ( unsigned int ) IRadDrive::DirectoryInfo::DirectoryEntryType::IsDone;
                }
                else
                {
                    wsprintf( buf, "Found: %s", search->m_FindData.cFileName );
                    LogMessage( buf );

                    strcpy( pRpy->m_Name, search->m_FindData.cFileName );
                    if( ( search->m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
                    {
                        pRpy->m_Type = 
                            ( unsigned int ) IRadDrive::DirectoryInfo::DirectoryEntryType::IsDirectory;
                    } 
                    else
                    {
                        pRpy->m_Type = 
                            ( unsigned int ) IRadDrive::DirectoryInfo::DirectoryEntryType::IsFile;
                    }
                }

                //
                // Add search info struct to our local list.
                //
                search->m_Next = m_OpenSearches;
                m_OpenSearches = search;
            }
            else // There was a real failure.
            {
                //
                // Log the event.
                //
                LogMessage( "ERROR: FindFirst failed." );
                wsprintf( buf, "System error code was %d.", lastErrorCode );
                LogMessage( buf );

                //
                // Set up error packet to return to target.
                //
                pRpy->m_Handle = 0xffffffff;
                strcpy( pRpy->m_Name, "" );
                pRpy->m_Type = ( unsigned int ) IRadDrive::DirectoryInfo::DirectoryEntryType::IsDone;

                //
                // Delete the local search data; we won't save it.
                //
                delete search;
            }

            pRpy->m_Command = pCmd->m_Command;
            *replySize = sizeof( HfpFindFirstRpy );
            
            break;
        }    

        //
        // Request: Continue a search.
        //
        case HfsFindNext :
        {
            HfpFindNextCmd* pCmd = (HfpFindNextCmd*) request;    
            HfpFindNextRpy* pRpy = (HfpFindNextRpy*) reply;    

            // 
            // Locate the search handle.
            //
            TargetSearchList* search = m_OpenSearches;
            TargetSearchList* searchPrev = NULL;
            while( search != NULL )
            {
                if( pCmd->m_Handle == ( unsigned int ) search )
                {
                    break; // found
                }
                else
                {
                    searchPrev = search;
                    search = search->m_Next;
                }
            }

            //
            // Report error if search handle not found.
            //
            if( search == NULL )
            {
                //
                // Log the event.
                //
                wsprintf( buf, "REQ: Continue search ID %X", pCmd->m_Handle );
	            LogMessage( buf );

	            LogMessage ("ERROR: Search handle not found.");
                *replySize = 0;
                return;
            }

            //
            // Log the event.
            //
            wsprintf( buf, "REQ: Continue search %s", search->m_SearchSpec );
	        LogMessage( buf );

            //
            // Initiate a local FindNext
            //
            bool failed = false;
            bool noMatch = false;
            DWORD lastErrorCode = ERROR_SUCCESS;
            SetLastError( ERROR_SUCCESS );
            failed = ( 0 == ::FindNextFile( search->m_Handle, &( search->m_FindData ) ) );

            //
            // Determine if a failure was because of no matches or a real error.
            //
            if( failed )
            {
                lastErrorCode = ::GetLastError( );
                if( lastErrorCode == ERROR_NO_MORE_FILES )
                {
                    noMatch = true;
                    failed = false;
                }
            }

            //
            // If there was no error, formulate a response packet and save the search info.
            //
            if( !failed )
            {
                //
                // Return search result.
                //
                pRpy->m_Result = (unsigned int) search;
                if( noMatch )
                {
                    LogMessage( "No more matches found." );
                    strcpy( pRpy->m_Name, "" );
                    pRpy->m_Type = ( unsigned int ) IRadDrive::DirectoryInfo::DirectoryEntryType::IsDone;
                }
                else
                {
                    wsprintf( buf, "Found: %s", search->m_FindData.cFileName );
                    LogMessage( buf );

                    strcpy( pRpy->m_Name, search->m_FindData.cFileName );
                    if( ( search->m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
                    {
                        pRpy->m_Type = 
                            ( unsigned int ) IRadDrive::DirectoryInfo::DirectoryEntryType::IsDirectory;
                    } 
                    else
                    {
                        pRpy->m_Type = 
                            ( unsigned int ) IRadDrive::DirectoryInfo::DirectoryEntryType::IsFile;
                    }
                }
            }
            else // There was a real failure.
            {
                //
                // Log the event.
                //
                LogMessage( "ERROR: FindNext failed." );
                wsprintf( buf, "System error code was %d.", lastErrorCode );
                LogMessage( buf );

                //
                // Set up error packet to return to target.
                //
                pRpy->m_Result = 0xffffffff;
                strcpy( pRpy->m_Name, "" );
                pRpy->m_Type = ( unsigned int ) IRadDrive::DirectoryInfo::DirectoryEntryType::IsDone;

                //
                // Tell Windows we are done with this handle.
                //
                ::FindClose( search->m_Handle );

                //
                // Unlink and delete the local search data; we won't save it.
                //
                if( searchPrev == NULL )
                {
                    m_OpenSearches = search->m_Next;
                }
                else
                {
                    searchPrev->m_Next = search->m_Next;
                }
                delete search;
            }

            pRpy->m_Command = pCmd->m_Command;
            *replySize = sizeof( HfpFindNextRpy );
                
            break;
        }


        //
        // Request: Terminate a search.
        //
        case HfsFindClose :
        {
            HfpFindCloseCmd* pCmd = (HfpFindCloseCmd*) request;    
            HfpFindCloseRpy* pRpy = (HfpFindCloseRpy*) reply;    

            // 
            // Locate the search handle.
            //
            TargetSearchList* search = m_OpenSearches;
            TargetSearchList* searchPrev = NULL;
            while( search != NULL )
            {
                if( pCmd->m_Handle == ( unsigned int ) search )
                {
                    break; // found
                }
                else
                {
                    searchPrev = search;
                    search = search->m_Next;
                }
            }

            //
            // Report error if search handle not found.
            //
            if( search == NULL )
            {
                //
                // Log the event.
                //
                wsprintf( buf, "REQ: Terminate search ID %X", pCmd->m_Handle );
	            LogMessage( buf );

	            LogMessage ("ERROR: Search handle not found.");
                *replySize = 0;
                return;
            }

            //
            // Log the event.
            //
            wsprintf( buf, "REQ: Terminate search %s", search->m_SearchSpec );
	        LogMessage( buf );

            //
            // Initiate a local FindClose
            //
            bool failed = false;
            SetLastError( ERROR_SUCCESS );
            failed = ( 0 == ::FindClose( search->m_Handle ) );

            //
            // If there was no error, formulate a response packet and save the search info.
            //
            if( !failed )
            {
                //
                // Success - log the event.
                //
                LogMessage( "Search ended OK." );  

                //
                // Return OK result.
                //
                pRpy->m_Result = (unsigned int) search;
            }
            else // There was a failure.
            {
                //
                // Log the event.
                //
                LogMessage( "ERROR: FindClose failed." );
                wsprintf( buf, "System error code was %d.", ::GetLastError );
                LogMessage( buf );

                //
                // Set up error packet to return to target.
                //
                pRpy->m_Result = 0xffffffff;

                //
                // Unlink and delete the local search data; we won't save it.
                //
                if( searchPrev == NULL )
                {
                    m_OpenSearches = search->m_Next;
                }
                else
                {
                    searchPrev->m_Next = search->m_Next;
                }
                delete search;
            }

            pRpy->m_Command = pCmd->m_Command;
            *replySize = sizeof( HfpFindCloseRpy );
                
            break;
        }

        default:
        {
            //
            // Should never happen - protocol error - unknown request type.
            //
            rAssert( false );
            break;
        }
    }
}


//===========================================================================
// rDebugFileServer::CloseFiles
//===========================================================================
// Description: This private member closes all files and searches currently 
//              held open on the host by the target, and deletes their file 
//              info structures.
//
//              This method should be called whenever a target connection
//              is closed, and when the server is shutting down.
//              It does not activate any informational callbacks.
//
// Constraints:	The caller must not be holding any file info struct
//              pointers or substructures when this method is called.
//
//===========================================================================

void rDebugFileServer::CloseFiles( )
{
    TargetFileList* listItem = m_OpenFiles;

    while( listItem != NULL ) 
    {
        TargetFileList* temp = listItem;
        if( listItem->m_LocalHandle != NULL )
        {
            fclose( listItem->m_LocalHandle );
        }
        listItem = listItem->m_Next;
        delete temp;
    }

    TargetSearchList* searchListItem = m_OpenSearches;

    while( searchListItem != NULL ) 
    {
        TargetSearchList* temp = searchListItem;
        if( searchListItem->m_Handle != INVALID_HANDLE_VALUE )
        {
            FindClose( searchListItem->m_Handle );
        }
        searchListItem = searchListItem->m_Next;
        delete temp;
    }

    m_OpenFiles = NULL;
    m_OpenSearches = NULL;
}


//===========================================================================
// rDebugFileServer::LogMessage
//===========================================================================
// Description: This private method passes log strings to the user program's
//              logging callback, if the callback is registered. If there is
//              no callback registered, no action is taken.
//
// Parameters:	pString - the message to log
//
//===========================================================================

void rDebugFileServer::LogMessage( const char* pString )
{
    if( m_LogCallback != NULL && pString != NULL )
    {
        m_LogCallback->OnFileServerLogMessage( pString );
    }
}


//===========================================================================
// rDebugFileServer::AddRef
//===========================================================================
// Description: Reference counting support. Adds 1 to the internal ref count.
//
//===========================================================================

void rDebugFileServer::AddRef( void )
{
    m_ReferenceCount++;
}


//===========================================================================
// rDebugFileServer::Release
//===========================================================================
// Description: Reference counting support. Subtracts 1 from the internal
//              reference count, and deletes the object when the count
//              reaches zero.
//
//===========================================================================

void rDebugFileServer::Release( void )
{
    m_ReferenceCount--;

    if( m_ReferenceCount == 0 )
    {
        delete this;
    }
}


#endif