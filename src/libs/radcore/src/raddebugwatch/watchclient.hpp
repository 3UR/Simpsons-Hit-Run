//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        watchclient.hpp
//
// Subsystem:   Radical Debug Watch
//
// Description: This file contains all definitions and classes relevant to
//              the debug watch client
//
// Revisions:   18-Jan-2001 Creation
//
// Notes:       
//
//=============================================================================

#ifndef WATCHCLIENT_HPP
#define WATCHCLIENT_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <raddebugcommunication.hpp>
#include <raddebugwatch.hpp>
#include <radobject.hpp>
#include <radmemory.hpp>
#include "../radprotocols/debugwatchprotocol.hpp"

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
// This class represents to object used to communicated with the host.
// It is a singleton.
//
class DebugWatchClient : public IRadDebugWatch,
                         public IRadDbgComChannelStatusCallback,
                         public IRadDbgComChannelReceiveCallback,
                         public IRadDbgComChannelSendCallback,
                         public radObject
{
    public:

    //
    // Constructor. Nothing too interesting.
    //
    DebugWatchClient( radMemoryAllocator alloc );
    virtual ~DebugWatchClient( void );
  
    //
    // DebugWatchInterfaces
    //
    virtual void Initialize( const char* pNameSpace, unsigned int internalBufferSize );
    virtual void Terminate( void );
    virtual void AddUnsignedInt( unsigned int* pValue, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, unsigned int min,  unsigned int max, bool readonly );
    virtual void AddInt( int* pValue, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, int min, int max, bool readonly );
    virtual void AddUnsignedShort( unsigned short* pValue, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, unsigned short min, unsigned short max, bool readonly );
    virtual void AddShort( short* pValue, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, short min, short max, bool readonly );
    virtual void AddUnsignedChar( unsigned char* pValue, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, unsigned char min, unsigned char max, bool readonly );
    virtual void AddChar( char* pValue, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, char min, char max, bool readonly );
    virtual void AddBoolean( bool* pValue, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, bool readonly );
    virtual void AddFloat( float* pValue, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, float min, float max, bool readonly );
    virtual void AddString( char* pValue, unsigned int stringLength, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, bool readonly );
    virtual void AddEnumeration( int* pValue, const EnumerationElement* pEnumerationInfo, unsigned int numberOfElements, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, bool readonly );
    virtual void AddVector( float* pValue, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData, float min, float max, bool readonly );
    virtual void Delete( void* pValue );
    virtual void AddFunction( const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData );
    virtual void IssueHostRequest( unsigned int Id, void* pData, unsigned int length );
    virtual void AssociateFile( const char* pNameSpace, const char* pFilename, FileType fileType ); 

    //
    // Callbacks from the communication system
    //
    virtual void OnStatusChange( IRadDbgComChannel::ConnectionState connectionState,
                                 const char* Message );
    virtual void OnReceiveComplete( bool successful, unsigned int bytesReceived );
    virtual void OnSendComplete( bool successful );

    virtual void AddRef( void );
    virtual void Release( void );

    #ifdef RAD_DEBUG
    virtual void Dump( char* pStringBuffer, unsigned int bufferSize );
    #endif

    private:

    //
    // Helpers to control transmission.
    //
    bool        IsThereTxRoom( unsigned int size );
    bool        IsCompletePacket(unsigned int bytesRemaining, unsigned char *pRxBuffer);
    void        InitiateTransmission( );
    void        QueueNameSpace( const char* pNameSpace );
    WCCreate*   DoCreatePacketPreamble( void* pValue, const char* pName, const char* pNameSpace, RADDEBUGWATCH_CALLBACK callback, void* userData );
    
    //
    // This member maintains the reference count of this object.
    //
    unsigned int m_ReferenceCount;    

    //
    // Buffer used to receive messages from host. Fixed size. 
    //
    unsigned char m_RxBuffer[ WP_MAX_PACKET ];

    //
    // Buffer used for transmitting data. Double buffered so we can
    // queue information to send. Size is set by client application.
    //
    unsigned char* m_TxBuffer1;
    unsigned char* m_TxBuffer2;
    unsigned int   m_TxBufferSize;

    //
    // Used to control transmit state machine.    
    //
    bool            m_TxOutstanding;
    unsigned int    m_TxBytesQueued;
    unsigned char*  m_CurrentTxAddress;
    bool            m_Buffer1Active;

    //
    // Stops us from spewing tons of error messages.
    //
    bool            m_OverflowReported;

    //
    // Contains our debug communication channel.
    //
    IRadDbgComChannel*     m_Channel;

    //
    // Boolean indicating that we are attached.
    //
    bool                m_Attached;

    //
    // USed to peform memory allocations.
    //
    radMemoryAllocator  m_Allocator;

};

#endif


