//===========================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Radical debug file server
//
// Description: This file contains classes and definitions relevant to the
//              file server, other than those public definitions contained 
//              in raddebugfileserver.hpp.
//
// Authors:     Soleil Lapierre
//
//===========================================================================

// Recompilation protection flag.
#ifndef __FILESERVER_H
#define __FILESERVER_H

#if defined (RAD_WIN32) // server only exists on Windows


//===========================================================================
// Nested Includes
//===========================================================================

#include <stdio.h>
#include <radmemory.hpp>
#include "radfile.hpp"
#include "raddebugcommunication.hpp"
#include "raddebugfileserver.hpp"


//===========================================================================
// Private Class Definitions
//===========================================================================

//===========================================================================
//
// Description: This class implements the Radical debug file server interface
//              and provides the necessary behind-the-scenes support code
//              to make a functional server.
//
//===========================================================================
class rDebugFileServer : public IRadDebugFileServer,
                         public IRadDbgComChannelStatusCallback,
                         public IRadDbgComChannelSendCallback,
                         public IRadDbgComChannelReceiveCallback
{
    public:

    //
    // Constructor. 
    //
    rDebugFileServer( int pNumExceptions );
    
    //
    // Methods required by the IRefCount inreface spec.
    //
    virtual void AddRef( void );
    virtual void Release( void );

    //
    // Methods required by the IRadDbgComFileServer interface spec.
    //
    virtual void SetBasePath( char* pPath );
    virtual void SetDefaultHost( bool pLocal );
    virtual void SetBootData( unsigned char* pData, int pDataLength );
    virtual void AddException( char* pString );
    virtual void RemoveException( char *pString );
    virtual void ClearExceptions( void );
    virtual void SetPathOverride( bool pEnable );
    virtual void RegisterLogCallback( IRadDebugFileServerLogCallback* pCallback );
    virtual void RegisterFileCallback( IRadDebugFileServerFileCallback* pCallback );
    virtual void RegisterStatusCallback( IRadDbgComChannelStatusCallback* pCallback );
    virtual bool Attach( radDbgComTargetName pName, char* pErrorMessage );
    virtual void Detach( void );
    virtual void EnumerateOpenFiles( void (*pEnumerationCallback)( char* pName, char* pMessage ) ) const;
    virtual void GetTargetInterface( IRadDbgComTargetTable** pTargetTable ) const;
    virtual bool IsConnected( void ) const;
    
    //
    // Methods required by the IRadDbgComChannelStatusCallback interface.
    //
    virtual void OnStatusChange( IRadDbgComChannel::ConnectionState connectionState,
                                 const char* Message );

    //
    // Methods required by the IRadDbgComChannelSendCallback interface.
    //
    virtual void OnSendComplete( bool Successful );

    //
    // Methods required by the IRadDbgComChannelReceiveCallback interface.
    //
    virtual void OnReceiveComplete( bool Successful, unsigned int bytesReceived );


    private:

    //
    // Destructor. Public freeing is done via Release.
    //
    ~rDebugFileServer( void );

    //
    // Converts a target's file name to a full path and file name on the host.
    //
    void QualifyFilename( char* pFilename, char* pFullpath );
    
    //
    // Determines whether or nor a file should be served up by the host.
    //
    bool CheckIfFileOnHost( char* filename );

    //
    // Handle a request from the target.
    //
    void ProcessFileRequest( char* request, int requestSize, char* reply, int* replySize );

    //
    // Wildcard matcher for exception rules.
    //
    bool StringMatch( char* MatchName, char* Name);

    //
    // Close all open files. Used when connection is terminated.
    //
    void CloseFiles( );

    //
    // Pass a debug message to the logger callback.
    //
    void LogMessage( const char* pString );


    //
    // Private data members.
    //
    struct TargetFileList                 // tracks file opened by target
    {                                     // this is a basic linked list
        FILE* m_LocalHandle;              // local open file handle
        void* m_RemoteHandle;      // target's file handle
        char m_FileName[ 512 + 1 ];       // name the file was opened with
        bool m_WasOverride;               // true if path override was needed
        char m_Flags[ 16 ];               // file open permissions
        TargetFileList* m_Next;           // next file in list
    };
    TargetFileList* m_OpenFiles;          // list of open files

    struct TargetSearchList               // Active search handles of target
    {                                     // this is a basic linked list
        HANDLE m_Handle;                  // local search handle (not given to target)
        WIN32_FIND_DATA m_FindData;       // Windows search struct
        char m_SearchSpec[ 256 + 1 ];     // Wildcard search path
        TargetSearchList* m_Next;         // next file in list
    };
    TargetSearchList* m_OpenSearches;     // list of open files

    typedef char ExceptionRule[ 256 ];    // file match exception rule string

    int  m_ReferenceCount;                // reference counter
    radDbgComTargetName m_TargetName;     // currently selected target
    char m_Path[ 256+1 ];                 // the base path of the shadow disk
    unsigned char* m_BootData;            // data for fake bootload.cfg file
    int  m_BootDataSize;                  // length of the boot data
    bool m_DefaultHost;                   // serve files from host by default?
    int  m_ExceptionCount;                // # exceptions allowed
    ExceptionRule* m_Exceptions;          // exception buffer
    bool m_Override;                      // allow absolute paths?
    bool m_Attached;                      // attach attempt active?
    struct IRadDbgComTargetTable* m_Host; // debug comms target interface
    struct IRadDbgComChannel* m_Channel;  // debug comms channel object

    char m_ReceiveBuffer[ sizeof( HfpReadRpy ) ]; // buffer for incoming data
    char m_SendBuffer[ sizeof( HfpReadRpy ) ];    // buffer for outgoing data

    bool   m_TxOutstanding;               // more data to send?
    unsigned int m_TxPendingBytes;        // # bytes to be sent

    IRadDebugFileServerLogCallback* m_LogCallback;     // logging handler
    IRadDebugFileServerFileCallback* m_FileCallback;   // file op info handler
    IRadDbgComChannelStatusCallback* m_StatusCallback; // state change handler

};



#endif

#endif                                  // End conditional inclusion

