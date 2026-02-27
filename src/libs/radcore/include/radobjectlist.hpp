//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radobjectlist.hpp
//
// Subsystem:	Foundation Technologies - Object Array Interface Definition
//
// Description:	This file contains all definitions and interfaces required to
//              interact with the Object Array component.
//
// Revisions:   June 16, 2000     Creation     TEH
//
//=============================================================================

#ifndef RADOBJECTLIST_HPP
#define RADOBJECTLIST_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radobject.hpp>
#include <radmemory.hpp>

//=============================================================================
// Forward Declarations
//=============================================================================

struct IRadObjectList;
struct IRadWeakInterfaceWrapper;
struct IRadWeakCallbackWrapper;

//=============================================================================
// Factories and Functions
//=============================================================================

void radObjectListTerminate( void );

inline void radObjectListCreate( IRadObjectList** ppIObjectList, radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );
inline IRadObjectList * radObjectListCreate( IRadMemoryAllocator * pIRadMemoryAllocator );
void radObjectDynamicArrayCreate( IRadObjectList ** ppIObjectList, radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );

void radWeakInterfaceWrapperCreate( IRadWeakInterfaceWrapper** ppWI , radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );
void radWeakCallbackWrapperCreate( IRadWeakCallbackWrapper** ppIWcr, radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );

void radWeakCallbackRegister
(
    IRadObjectList* pCallbackList,
    radMemoryAllocator allocator,
    void* pCallback,
    void* pUserData
);
void radWeakCallbackUnregister
(
    IRadObjectList* pCallbackList,
    void* pCallback,
    void* pUserData
);

//=============================================================================
// Interface: IRadObjectList
//=============================================================================

#include <radmemory.hpp>
#include <radmemorymonitor.hpp>
#include <radobject.hpp>
#include <radobjectlist.hpp>
#include <raddebug.hpp>

//============================================================================
// Interface: iIObjectListNode
//============================================================================

struct radObjectListNode
    :
    public radBaseObject
{
	private:

		IMPLEMENT_BASEOBJECT( "radObjectListNode" )

		inline void * operator new( size_t size, void * pPlacement )
		{
			return pPlacement;
		}

		inline void * operator new ( size_t size, radMemoryAllocator allocator )
		{
			return radMemoryAlloc( allocator, size );
		}

	public:

        inline radObjectListNode( )
        {
            radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radObjectListNode" );
        }

		inline static radObjectListNode * Construct( IRadMemoryPool * pIRadMemoryPool )
		{
			return new ( pIRadMemoryPool->GetMemory( ) ) radObjectListNode( );
		}

		inline static void Destroy( radObjectListNode * pNode, IRadMemoryPool * pIRadMemoryPool )
		{
			pNode->~radObjectListNode( );

			pIRadMemoryPool->FreeMemory( pNode );
		}

		inline static radObjectListNode * Construct ( radMemoryAllocator allocator )
		{
			return new ( allocator ) radObjectListNode( );
		}

		inline static void Destroy( radObjectListNode * pNode, radMemoryAllocator allocator )
		{
			pNode->~radObjectListNode( );

			::radMemoryFree( allocator, pNode );
		}

		radRef< IRefCount > m_xIObject;
		radObjectListNode * m_pNext;
};

//============================================================================
// Component IRadObjectList
//============================================================================

struct IRadObjectList
	:
    public IRefCount,
    public radRefCount
{

//============================================================================
// Data Members
//============================================================================

    unsigned int m_Size;
    unsigned int m_SearchIndex;

    radObjectListNode * m_pHead;
    radObjectListNode * m_pTail;
    radObjectListNode * m_pSearch;

	radRef< IRadMemoryPool > m_xIRadMemoryPool;
	
    IMPLEMENT_REFCOUNTED( "IRadObjectList" )

    //========================================================================
	// IRadObjectList::AddObject
	//========================================================================

	inline void AddObject( IRefCount * pIObject )
	{
		rAssert( pIObject != NULL );

		radObjectListNode * pNew;

		if ( m_xIRadMemoryPool != NULL )
		{
			pNew = radObjectListNode::Construct( m_xIRadMemoryPool );
		}
		else
		{
			pNew = radObjectListNode::Construct( GetThisAllocator( ) );
		}

        pNew->m_xIObject = pIObject;
        pNew->m_pNext = NULL;

        //
        // We want to add it to the end of the list, or the client might
        // freak out
        //

        if ( m_pTail != NULL )
        {
            m_pTail->m_pNext = pNew;
        }
        else
        {
            // If the tail pointer is NULL, so is the head.

            m_pHead = pNew;
        }

        m_pTail = pNew;
      
        m_Size++;
        
        m_pSearch = m_pHead;
        m_SearchIndex = 0;            
	}
	
    inline void AddHead( IRefCount * pIObject )
    {
		rAssert( pIObject != NULL );

		radObjectListNode * pNew;

		if ( m_xIRadMemoryPool != NULL )
		{
			pNew = radObjectListNode::Construct( m_xIRadMemoryPool );
		}
		else
		{
			pNew = radObjectListNode::Construct( GetThisAllocator( ) );
		}

        pNew->m_xIObject = pIObject;
        pNew->m_pNext    = m_pHead;
        
        m_pHead = pNew;
        
        if ( m_pTail == NULL )
        {
            m_pTail = pNew;
        }
              
        m_Size++;
        
        m_pSearch = m_pHead;
        m_SearchIndex = 0;            
    
    }
   	

	//========================================================================
	// IRadObjectList::RemoveObject
	//========================================================================

    inline void RemoveObject( IRefCount * pIObject )
    {
        radObjectListNode * pPrev   = NULL;
        radObjectListNode * pSearch = m_pHead;

        while ( pSearch != NULL )
        {
            if ( pSearch->m_xIObject == pIObject )
            {
                if ( pSearch->m_pNext == NULL )
                {
                    m_pTail = pPrev;
                }

                if ( pPrev == NULL )
                {
                    m_pHead = pSearch->m_pNext;
                }
                else
                {
                    pPrev->m_pNext = pSearch->m_pNext;
                }

				if ( m_xIRadMemoryPool != NULL )
				{
					radObjectListNode::Destroy( pSearch, m_xIRadMemoryPool );
				}
				else
				{
					radObjectListNode::Destroy( pSearch, GetThisAllocator( ) );
				}					

                m_Size--;
                
                m_pSearch = m_pHead;
                m_SearchIndex = 0;

                return;

            }

            pPrev = pSearch;
            pSearch = pSearch->m_pNext;
        }

        rAssert( 0 ); // Not found
    }

    //========================================================================
	// IRadObjectList::IRadObjectList
	//========================================================================

    inline void Reset( void )
    {
        m_pSearch = m_pHead;
        m_SearchIndex = 0;
    }

    //========================================================================
	// IRadObjectList::GetNext
	//========================================================================

    inline IRefCount * GetNext( void )
    {
        if ( m_pSearch != NULL )
        {
            IRefCount * pIRefCount = m_pSearch->m_xIObject;

            m_pSearch = m_pSearch->m_pNext;
            m_SearchIndex++;

            return pIRefCount;
        }

        return NULL;            
    }

    //========================================================================
    // IRadObjectList::GetAt
    //========================================================================

    inline IRefCount * GetAt( unsigned int index )
    {
        if ( ! ( m_SearchIndex < m_Size && m_SearchIndex <= index ) )
        {           
            m_SearchIndex = 0;
            m_pSearch = m_pHead;
        }

        while( m_pSearch != NULL )
        {
            if ( m_SearchIndex >= index )
            {
                return m_pSearch->m_xIObject;
            }

            m_pSearch = m_pSearch->m_pNext;
            m_SearchIndex++;
        }

       // Check that we are out of range, otherwise we have big problems

        rAssert( index >= m_Size );

        return NULL;
    }

    //========================================================================
    // IRadObjectList::RemoveAll
    //========================================================================

    inline void RemoveAll( void )
    {
        radObjectListNode * pSearch = m_pHead;
        radObjectListNode * pTemp   = NULL;

        while ( pSearch != NULL )
        {
            pTemp = pSearch;
            pSearch = pSearch->m_pNext;

			if ( m_xIRadMemoryPool != NULL )
			{
				radObjectListNode::Destroy( pTemp, m_xIRadMemoryPool );
			}
			else
			{
				radObjectListNode::Destroy( pTemp, GetThisAllocator( ) );
			}
        }

        m_pHead = m_pTail = m_pSearch = NULL;
        m_SearchIndex = 0;
		m_Size = 0;
    }

    //========================================================================
    // IRadObjectList::GetSize
    //========================================================================

    inline unsigned int GetSize ( void )
    {
        return m_Size;
    }

    //========================================================================
	// IRadObjectList::IRadObjectList
	//========================================================================

	inline void Initialize( unsigned int poolPageSize )
	{
		rAssert( m_pHead == NULL );
		rAssert( m_xIRadMemoryPool == NULL );

		::radMemoryCreatePool(
			& m_xIRadMemoryPool, sizeof( radObjectListNode ), poolPageSize,
			poolPageSize,false, None, GetThisAllocator( ), "IRadObjectList" );
	}

    //========================================================================
	// IRadObjectList::IRadObjectList
	//========================================================================

    inline IRadObjectList( unsigned int allocator )
        :
        m_Size( 0 ),
        m_SearchIndex( 0 ),
        m_pHead( NULL ),
        m_pTail( NULL ),
        m_pSearch( NULL )   
    {
        radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "IRadObjectList" );
    }

    //========================================================================
	// IRadObjectList::~IRadObjectList
	//========================================================================

    inline ~IRadObjectList( void )
    {
    
        //
        // Could just set head to null, but this would cause a stack
        // explosion for huge lists.
        //

        RemoveAll( );
             
    }

};


inline void radObjectListCreate( IRadObjectList** ppIObjectList, radMemoryAllocator allocator )
{
    *ppIObjectList = new ( allocator ) IRadObjectList( allocator );
}

//=============================================================================
// Interface: 
//
// Description: 
//      Use an object of this type to store objects that are not derived from
//      IRefCount safely
//      
//=============================================================================

struct IRadWeakInterfaceWrapper : public IRefCount
{
    virtual void SetWeakInterface( void* pWeakInterface ) = 0;
    virtual void *GetWeakInterface( void ) = 0;
};

//=============================================================================
// Interface: 
//
// Description: 
//      Use an object of this type to store callback interfaces with associated 
//      user callback data that are not derived from bIObject.
//      
//=============================================================================

struct IRadWeakCallbackWrapper
    :
    public IRefCount
{
    virtual void SetWeakInterface( void * pWeakInterface ) = 0;
    virtual void * GetWeakInterface( void ) = 0;

    virtual void SetUserData( void * pserData ) = 0;
    virtual void * GetUserData( void ) = 0;
};

#endif