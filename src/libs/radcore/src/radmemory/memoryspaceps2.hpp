//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        memoryspaceps2.hpp
//
// Subsystem:	Foundation Technologies - Memory Operation Services
//
// Description:	This file contains the ps2 implementation of the 
//              remote memory space operations
//
// Revisions:   13-Oct-2001     Creation     Rob
//
//=============================================================================

#ifndef MEMORYSPACEPS2_HPP
#define MEMORYSPACEPS2_HPP

//============================================================================
// Include Files
//============================================================================

#include <radmemory.hpp>
#include <radtime.hpp>
#include <sifrpc.h>

//============================================================================
// Forward Declarations
//============================================================================

// Internal init done by radMemoryInitialize( )

void radMemorySpaceInitialize( unsigned int maxRequests );
void radMemorySpaceTerminate( void );

struct MemorySpaceAsyncRequest_CopyEeToEe;
struct MemorySpaceAsyncRequest_CopyEeToIop;
struct MemorySpaceAsyncRequest_CopyIopToEe;

//============================================================================
// struct MemorySpaceAsyncRequest_CopyEeToEe
//============================================================================

struct MemorySpaceAsyncRequest_CopyEeToEe
	:
	public IRadMemorySpaceCopyRequest
{
	public: 

        MemorySpaceAsyncRequest_CopyEeToEe( void );
		~MemorySpaceAsyncRequest_CopyEeToEe( void );
        virtual void AddRef( void );
        virtual void Release( void );
		virtual bool IsDone( void );

    private:

        unsigned int m_RefCount;
};

//============================================================================
// struct MemorySpaceAsyncRequest
//============================================================================

struct MemorySpaceAsyncRequest : public radObject, public IRadMemorySpaceCopyRequest
{
    MemorySpaceAsyncRequest( void );

    virtual ~MemorySpaceAsyncRequest( void );

    void Implement_AddRef( void ) { m_RefCount++; }
    void Implement_Release( void ) { rAssert( m_RefCount > 0 ); m_RefCount--; if ( m_RefCount == 0 ) { delete this; } }

    void * operator new( size_t size, const char * pIdentifier );
    void operator delete( void * pMemory );

    static void ServiceHead( void );
    static void QueueRequest( MemorySpaceAsyncRequest * pRequest );

    protected:

        virtual void DoRequest( void ) = 0;
        virtual bool Service( void ) = 0; // true - finished, false - more to do             

    private:

        MemorySpaceAsyncRequest * m_pRequest_Next;
        
        static MemorySpaceAsyncRequest * s_pRequest_Head;

        unsigned int m_RefCount;
};  

//============================================================================
// struct MemorySpaceAsyncRequest_CopyEeToIop
//============================================================================

struct MemorySpaceAsyncRequest_CopyEeToIop
	:
    public MemorySpaceAsyncRequest
{
	public: 

		IMPLEMENT_BASEOBJECT( "MemorySpaceAsyncRequest_CopyEeToIop" )

		MemorySpaceAsyncRequest_CopyEeToIop( void * pDest, const void * pSrc, unsigned int bytes );
		~MemorySpaceAsyncRequest_CopyEeToIop( void );

        virtual void AddRef( void )  { Implement_AddRef( ); }
        virtual void Release( void ) { Implement_Release( ); }
		virtual bool IsDone( void );

    protected:
             
        virtual void DoRequest( );
	
	private:

        bool Service( void );

		unsigned int m_TransactionId;
        void * m_pDest;
        const void * m_pSource;
        unsigned int m_Bytes;

        enum State { Queued, Waiting, Started, Done } m_State;
};

//============================================================================
// struct MemorySpaceAsyncRequest_CopyIopToEe
//============================================================================

struct MemorySpaceAsyncRequest_CopyIopToEe
	:
    public MemorySpaceAsyncRequest
{
	public: 

		IMPLEMENT_BASEOBJECT( "MemorySpaceAsyncRequest_CopyIopToEe" )

		MemorySpaceAsyncRequest_CopyIopToEe( void * pDest, const void * pSrc, unsigned int bytes );
		~MemorySpaceAsyncRequest_CopyIopToEe( void );

        virtual void AddRef( void )  { Implement_AddRef( ); }
        virtual void Release( void ) { Implement_Release( ); }
		virtual bool IsDone( void );

    protected:

        virtual void DoRequest( );   
	
	private:

        bool Service( void );

		sceSifReceiveData m_ReceiveData;

        void * m_pDest;
        const void * m_pSource;
        unsigned int m_Bytes;

        enum State { Queued, Waiting, Started, Done } m_State;
};

//============================================================================
// struct MemorySpaceAsyncRequest_CopyIopToIop
//============================================================================

struct MemorySpaceAsyncRequest_CopyIopToIop
	:
    public MemorySpaceAsyncRequest
{
	public: 

        static void Initialize( unsigned int transferBufferSize );
        static void Terminate( void );

        // IRadMemorySpaceCopyRequest

        virtual void AddRef( void )  { Implement_AddRef( ); }
        virtual void Release( void ) { Implement_Release( ); }
		virtual bool IsDone( void );

		IMPLEMENT_BASEOBJECT( "MemorySpaceAsyncRequest_CopyIopToIop" )

		MemorySpaceAsyncRequest_CopyIopToIop( void * pDest, const void * pSrc, unsigned int bytes );
		~MemorySpaceAsyncRequest_CopyIopToIop( void );
    
        virtual void DoRequest( );

        bool Service( void );
        	
	private:
		
        unsigned int m_SourceAddress;       // Src in Iop Memory
		unsigned int m_DestinationAddress;  // Destination in Iop memory
		unsigned int m_BytesToCopy;
        unsigned int m_BytesCopied;
        unsigned int m_BytesCopying;

		enum State { Queued, Waiting, Started, IopToEe, IopToEeDone, EeToIop, EeToIopDone, Done } m_State;

		sceSifReceiveData m_ReceiveData; // Sony struct for IOP -> EE transfer
		int m_TransactionId;			 // Sony handle for EE -> IOP transfer

        static char * s_pTransferBuffer;
        static unsigned int s_TransferBufferSize;
};

#endif // MEMORYSPACEPS2_HPP