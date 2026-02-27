//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        externalmemoryobject.hpp
//
// Subsystem:	Foundation Technologies - ExternalMemory
//
// Description:	This file contains the implementation of the Foundation 
//              Technologies external memory component.
//
// Date:    	June 20, 2000
//
// Author:      Tim Hinds
//
//=============================================================================

#ifndef EXTERNALMEMORYOBJECT_HPP
#define EXTERNALMEMORYOBJECT_HPP

//============================================================================
// Class: IncludeFiles
//============================================================================

#include <string.h>
#include <radmemory.hpp>
#include <radstring.hpp>
#include <radmemorymonitor.hpp>

//============================================================================
// Forward Declarations
//============================================================================

class ExternalMemoryHeap;

//============================================================================
// Globals
//============================================================================

#define EMO_NAME_MAX_CHARS 64

//============================================================================
// Class: ExternalMemoryObject
//============================================================================

class ExternalMemoryObject
    :
    public IRadMemoryObject,
	public radBaseObject
{
    public:

		IMPLEMENT_BASEOBJECT( "ExternalMemoryObject" )

        ExternalMemoryObject( ) :
            radBaseObject( ),
            m_Address( 0 ),
            m_Size( 0 ),
            m_ClientSize( 0 ),
            m_ClientAddress( 0 ),
            m_pExternalMemoryHeap( NULL ),
            m_pEmo_Next( NULL ),
            m_pEmo_Previous( NULL )
        {
            radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "ExternalMemoryObject" );
        }

        // IRefCount

        virtual void AddRef( void );
        virtual void Release( void );

        // IRadMemoryObject

        virtual void* GetMemoryAddress( void );
        virtual unsigned int GetMemorySize( void );

        inline void Initialize( ExternalMemoryHeap * pExternalMemoryHeap,
                              unsigned int referenceCount,
                              uintptr_t address,
                              unsigned int size,
                              ExternalMemoryObject * pNext,
                              ExternalMemoryObject * pPrevious );

        inline static ExternalMemoryObject * Create( IRadMemoryPool * pIRadMemoryPool );
        inline static void Destroy( IRadMemoryPool * pIRadMemoryPool, ExternalMemoryObject * pEmo );

        void SetName( const char * pName );
        const char * GetName( void );

        unsigned int m_ReferenceCount;    
        uintptr_t m_Address;
        unsigned int m_Size;

		unsigned int m_ClientSize;
        uintptr_t m_ClientAddress;

        ExternalMemoryHeap   * m_pExternalMemoryHeap;
        ExternalMemoryObject * m_pEmo_Next;
        ExternalMemoryObject * m_pEmo_Previous;

        #ifdef RADMEMORY_EXTERNAL_HEAP_TRACK_NAMES
            char m_Name[ EMO_NAME_MAX_CHARS ];
        #endif

    private:
       
          
        inline void * operator new( size_t size, IRadMemoryPool * pIRadMemoryPool );
        inline void operator delete( void * pMemory ) { rAssert( false ); }
};

//============================================================================
// ExternalMemoryObject::operator new
//============================================================================

inline void * ExternalMemoryObject::operator new( size_t size, IRadMemoryPool * pIRadMemoryPool )
{
    return pIRadMemoryPool->GetMemory( size );
}

//============================================================================
// ExternalMemoryObject::Create
//============================================================================

inline ExternalMemoryObject * ExternalMemoryObject::Create( IRadMemoryPool * pIRadMemoryPool )
{
    return new( pIRadMemoryPool ) ExternalMemoryObject( );
}

//============================================================================
// ExternalMemoryObject::Destroy
//============================================================================

inline void ExternalMemoryObject::Destroy( IRadMemoryPool * pIRadMemoryPool, ExternalMemoryObject * pEmo )
{
    rAssert( pEmo != NULL );

    pEmo->~ExternalMemoryObject( );
    pIRadMemoryPool->FreeMemory( pEmo );
}

//============================================================================
// ExternalMemoryObject::Initialize
//============================================================================

void ExternalMemoryObject::Initialize
(
    ExternalMemoryHeap * pExternalMemoryHeap,
    unsigned int referenceCount,
    uintptr_t address,
    unsigned int size,
    ExternalMemoryObject * pEmo_Next,
    ExternalMemoryObject * pEmo_Previous
)
{
    m_ReferenceCount = referenceCount;
	m_Address = address;
    m_Size = size;
	m_ClientSize = 0;
	m_ClientAddress = 0;
    m_pExternalMemoryHeap = pExternalMemoryHeap;
    m_pEmo_Next = pEmo_Next;
    m_pEmo_Previous = pEmo_Previous;
           
    rAssert( m_pExternalMemoryHeap != NULL );

    #ifdef RADMEMORY_EXTERNAL_HEAP_TRACK_NAMES
        strcpy( m_Name, "Free" );
    #endif    

}

//============================================================================
// ExternalMemoryObject::SetName
//============================================================================

inline void ExternalMemoryObject::SetName( const char * pName )
{
    #ifdef RADMEMORY_EXTERNAL_HEAP_TRACK_NAMES

    if ( pName )
    {
        strncpy( m_Name, pName, EMO_NAME_MAX_CHARS );
        m_Name[ EMO_NAME_MAX_CHARS - 1 ] = 0;
    }
    else
    {
        sprintf( m_Name, "NO NAME" );
    }

    #endif 
    
}

//============================================================================
// ExternalMemoryObject::GetName
//============================================================================

inline const char * ExternalMemoryObject::GetName( void )
{
    #ifdef RADMEMORY_EXTERNAL_HEAP_TRACK_NAMES
        return m_Name;
    #else
        return "Names not enabled";
    #endif
}
#endif