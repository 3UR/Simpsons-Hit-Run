//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radobject.hpp
//
// Subsystem:	Foundation Technologies - Reference Count Inteface
//                                      - RadObject
//                                      - radRef (smart pointer template ).
//
// Description:	This file contains interface definitions associated with 
//              reference counting. It also provided a bass class definition
//              with all Foundation Tech components derive from. This base
//              class provides tracking mechanism to indentify all objects in
//              the system. Finally it contains the smart pointer template.
//
// Revisions:   Mar 3, 2001         Creation
//
//=============================================================================

#ifndef	RADOBJECT_HPP
#define RADOBJECT_HPP

#include <cstdint>
#include <memory/classsizetracker.h>

//=============================================================================
// Build Configuration Check
//=============================================================================

#if !defined(RAD_UWP) && !defined(RAD_WIN32)
    #error 'FTech requires definition of RAD_UWP or RAD_WIN32'
#endif
#if !defined(RAD_DEBUG) && !defined(RAD_TUNE) && !defined(RAD_RELEASE) 
    #error 'FTech requires definition of RAD_DEBUG, RAD_TUNE, or RAD_RELEASE'
#endif

#if defined (RAD_WIN32) || defined (RAD_UWP)
#pragma warning ( disable : 4291 )      // Get rid of delete warning
#endif

//=============================================================================
// Include Files
//=============================================================================
#include <radoptions.hpp>
#include <stdio.h>
#include <raddebug.hpp>

//=============================================================================
// Forward Declarations
//=============================================================================
struct IRefCount;

//
// global name used to name all FTech objects.
//
extern const char * const g_nameFTech;

typedef int radMemoryAllocator;
extern radMemoryAllocator radMemoryGetCurrentAllocator( void );
extern void* radMemoryAlloc( radMemoryAllocator allocator, unsigned int numberOfBytes );
extern void  radMemoryFree( radMemoryAllocator allocator, void* pMemory );
extern void  radMemoryFree( void* pMemory );

#if defined RADMEMORYMONITOR
extern void radMemoryMonitorIdentifyAllocationAdaptor( void * address, const char * group, const char * name, unsigned int* pReferenceCount );
extern void radMemoryMonitorReportAddRefAdaptor( void* pReference, void* pObject );
extern void radMemoryMonitorReportReleaseAdaptor( void* pReference, void* pObject );
#endif

void radAddRef( IRefCount * pRefObject, void * pParentObject );
void radRelease( IRefCount * pRefObject, void * pParentObject );

//=============================================================================
// Intefaces
//=============================================================================

//
// This interface is defines the reference counting system. Implemented by 
// most Foundation Tech Objects.
//
struct IRefCount 
{
    friend void radAddRef( IRefCount * pRefObject, void * pParentObject );
    friend void radRelease( IRefCount * pRefObject, void * pParentObject );
    //
    // Invoke this member anytime another object is given a pointer to this object.
    // Simply updates the reference count.
    //
    virtual void AddRef( void ) = 0;

    //
    // This member should be used instead of delete. It will ensure correct management
    // of the objects lifetime.
    //
    virtual void Release( void ) = 0;   
};

inline void radAddRef( IRefCount * pRefObject, void * pParentObject )
{
#if defined RADMEMORYMONITOR
    radMemoryMonitorReportAddRefAdaptor( pParentObject, pRefObject );
#endif
    (void)pParentObject;
    pRefObject->AddRef( );
}

inline void radRelease( IRefCount * pRefObject, void * pParentObject )
{
#if defined RADMEMORYMONITOR
    radMemoryMonitorReportReleaseAdaptor( const_cast< void *>( pParentObject ), pRefObject );
#endif
    (void)pParentObject;
    pRefObject->Release( );
}

//============================================================================
// Macros
//============================================================================

//
// The following macro definitions and base classes provide advanced object tracking,
// allocators and an implementation of the reference count. Should be used by
// all Foundation Tech components,
//
#define MAX_REFCOUNT 5000

#ifdef RAD_DEBUG

#define IMPLEMENT_REFCOUNTED_NOSIZE( pClassName ) \
	virtual void AddRef( void ) { radRefCount::Implement_AddRef(); } \
	virtual void Release( void ) { radRefCount::Implement_Release(); } \
	virtual void Dump( char * pStringBuffer, unsigned int bufferSize ) { \
					  sprintf( pStringBuffer, "Object: [%s] At Memory Location:[%p]\n", pClassName, this ); \
					  }


#define IMPLEMENT_REFCOUNTED( pClassName ) \
	virtual void AddRef( void ) { radRefCount::Implement_AddRef(); } \
	virtual void Release( void ) { radRefCount::Implement_Release(); } \
	virtual size_t GetObjectSize( void ) { return sizeof( *this); } \
	virtual void Dump( char * pStringBuffer, unsigned int bufferSize ) { \
					  sprintf( pStringBuffer, "Object: [%s] At Memory Location:[%p]\n", pClassName, this ); \
					  }



#define IMPLEMENT_BASEOBJECT( pClassName ) virtual void Dump( char * pStringBuffer, unsigned int bufferSize ) { \
                                                              sprintf( pStringBuffer, "Object: [%s] At Memory Location:[%p]\n", pClassName, this ); \
                                                              } \
										   virtual size_t GetObjectSize( void ) { return sizeof( *this); }
#else
#define IMPLEMENT_REFCOUNTED_NOSIZE( pClassName ) \
	virtual void AddRef( void ) { radRefCount::Implement_AddRef(); } \
	virtual void Release( void ) { radRefCount::Implement_Release(); }

#define IMPLEMENT_REFCOUNTED( pClassName ) \
	virtual void AddRef( void ) { radRefCount::Implement_AddRef(); } \
	virtual void Release( void ) { radRefCount::Implement_Release(); }

#define IMPLEMENT_BASEOBJECT( pClassName )
#endif

//============================================================================
// Class: radBaseObject
//============================================================================

class radBaseObject
{
	public:

		inline radBaseObject( void );
		inline virtual ~radBaseObject( void );

		#ifdef RAD_DEBUG
			virtual size_t GetObjectSize( void );
			virtual void Dump( char* pStringBuffer, unsigned int bufferSize );
			static  void DumpObjects( void );
		#endif
		
		#ifdef RAD_DEBUG
			radBaseObject * m_pRadBaseObjectNext;
			radBaseObject * m_pRadBaseObjectPrev;
			static radBaseObject * s_pRadBaseObjectHead;
		#endif
		
	private:
			
		void AddToList( void );
		void RemoveFromList( void );
};

inline radBaseObject::radBaseObject( void )
{
    CLASSTRACKER_CREATE( radBaseObject );
    #ifdef RAD_DEBUG
		AddToList( );
   
	#endif	
};

inline radBaseObject::~radBaseObject( void )
{
    CLASSTRACKER_DESTROY( radBaseObject );
    #ifdef RAD_DEBUG
		RemoveFromList( );
    #endif
}

//============================================================================
// Class: radObject
//============================================================================

class radObject // This should be called radHeapObject
	:
	public radBaseObject
{
	public:

		inline radObject( void );

        inline virtual ~radObject ( void );

        inline void* operator new( size_t size );
        inline void* operator new( size_t size, void* p );
      	inline void* operator new( size_t size, radMemoryAllocator allocator );
        
        inline void* operator new[]( size_t size );
        inline void* operator new[]( size_t size, void* p );	
      	inline void* operator new[]( size_t size, radMemoryAllocator allocator );
		inline void operator delete( void * pMemory );
		inline void operator delete[]( void * pMemory );

        inline radMemoryAllocator GetAllocator( void );

    protected:

        inline radMemoryAllocator GetThisAllocator( void );

    private:


        radMemoryAllocator m_ThisAllocator;

        static radMemoryAllocator s_Allocator;
};

inline void* radObject::operator new( size_t size )
{
    s_Allocator = radMemoryGetCurrentAllocator( );
	return radMemoryAlloc( s_Allocator, size );
}

inline void* radObject::operator new[ ]( size_t size )
{
    s_Allocator = radMemoryGetCurrentAllocator( );
	return radMemoryAlloc( s_Allocator, size );
}

inline void* radObject::operator new( size_t size, void* p )
{
    (void) size;
    return( p );
}    	

inline void* radObject::operator new[ ]( size_t size, void* p )
{
    (void) size;
    return( p );
}    	

inline void* radObject::operator new( size_t size, radMemoryAllocator allocator )
{
    s_Allocator = allocator;
	return radMemoryAlloc( allocator, size );
}

inline void* radObject::operator new[ ]( size_t size, radMemoryAllocator allocator )
{
    s_Allocator = allocator;
	return radMemoryAlloc( allocator, size );
}

inline void radObject::operator delete( void * pMemory )
{
	radMemoryFree( pMemory );
}

inline void radObject::operator delete[ ]( void * pMemory )
{
	radMemoryFree( pMemory );
}

inline radMemoryAllocator radObject::GetThisAllocator( void )
{
    return m_ThisAllocator;
}

inline radMemoryAllocator radObject::GetAllocator( void )
{
    return GetThisAllocator( );
}

inline radObject::radObject( void ) :
    m_ThisAllocator( s_Allocator )
{
    CLASSTRACKER_CREATE( radObject );
#if defined RADMEMORYMONITOR
    radMemoryMonitorIdentifyAllocationAdaptor( this, g_nameFTech, "radObject", NULL );
#endif
}

inline radObject::~radObject ( void )
{
    CLASSTRACKER_DESTROY( radObject );
    s_Allocator = m_ThisAllocator;
}

//============================================================================
// class: radRefCount
//============================================================================

class radRefCount : public radObject
{
    friend class radObject;
    public:

    radRefCount( unsigned int refCount = 1 )
        :
        radObject( ),
        m_RefCount( (int) refCount )
    {
        CLASSTRACKER_CREATE( radRefCount );
#if defined RADMEMORYMONITOR
        radMemoryMonitorIdentifyAllocationAdaptor( this, g_nameFTech, "radRefCount", (unsigned int*)(& m_RefCount) );
#endif
    }

    virtual ~radRefCount( void )
    {
        CLASSTRACKER_DESTROY( radRefCount );
    }

    void Implement_AddRef( void )
    {
        rAssert( m_RefCount < MAX_REFCOUNT );
        m_RefCount++;
    }

    void Implement_Release( void )
    {
        rAssert( m_RefCount > 0 && m_RefCount <  MAX_REFCOUNT );
        m_RefCount--;
        if (m_RefCount == 0 )
        {
			// Must avoid recursive destruction, set refcount to some high
			// value.

			m_RefCount = MAX_REFCOUNT / 2;

            delete this;
        }
    }

	int GetRefCount ( void ) { return m_RefCount; }
    
    private:
    
        int m_RefCount;    
};

//============================================================================
// Template: radRef 
//============================================================================

template < class T > class radRef
{
	public:

        void * operator new[]( size_t size )
        {
            return ::radMemoryAlloc( radMemoryGetCurrentAllocator(), size);
        }

        void * operator new[]( size_t size, radMemoryAllocator allocator )
        {
            return ::radMemoryAlloc( allocator, size );
        }

        void operator delete[]( void * pMemory )
        {
            ::radMemoryFree( pMemory );
        }

        void * operator new( size_t size )
        {
            return ::radMemoryAlloc( radMemoryGetCurrentAllocator(), size);
        }
        
        void * operator new( size_t size, radMemoryAllocator allocator )
        {
            return ::radMemoryAlloc( allocator, size );
        }

        void operator delete( void * pMemory )
        {
            ::radMemoryFree( pMemory );
        }

        radRef( )
		{
			m_pInterface = NULL;
		}

        radRef( T * pInterface )
		{
			m_pInterface = pInterface;

			if ( m_pInterface != NULL )
			{
				m_pInterface->AddRef( );
			}
		}

        radRef( const radRef< T > & copy )
        { 
            m_pInterface = copy.m_pInterface;

            if ( m_pInterface != NULL )
            {
                m_pInterface->AddRef( );
            }
        }

		~radRef( )
		{
			if ( m_pInterface != NULL )
			{
				T * pIOld = m_pInterface;
				m_pInterface = NULL;
				pIOld->Release( );
			}
		}


		operator T*( ) { return m_pInterface; }

		T &  operator *( )
        {
            rAssert( m_pInterface != NULL );
            return *m_pInterface;
        }

		T ** operator &( )
        {
            rAssert( m_pInterface == NULL );
            return &m_pInterface;
        }

		T *  operator->( ) const
        {
            rAssert( m_pInterface != NULL );
            return m_pInterface;
        }

        radRef< T > & operator = ( const radRef< T > & copy )
        {
            if ( copy != *this )
            {
                if ( m_pInterface != NULL )
                {
                    m_pInterface->Release( );
                }
                
                m_pInterface = copy.m_pInterface;
                
                if ( m_pInterface != NULL )
                {
                    m_pInterface->AddRef( );
                }
            }
            
            return *this;
        }
                     
		T* operator=( T * pInterface )
		{
			if ( m_pInterface != pInterface )
			{
                T * pOld = m_pInterface;
                m_pInterface = pInterface;

                if ( m_pInterface != NULL )
                {
                    m_pInterface->AddRef( );
                }

				if ( pOld != NULL )
				{
					pOld->Release( );
				}
			}

			return m_pInterface;
		}

        bool operator != ( T * pInterface ) const
        {
            return ( m_pInterface != pInterface );
        }

        bool operator == ( T * pInterface ) const
        {
            return ( m_pInterface == pInterface );
        }

		T * m_pInterface;
};

#if defined RADMEMORYMONITOR
//============================================================================
// Template specialization on IRefCount
//============================================================================
template<>
class radRef< IRefCount >
{
	public:

        radRef( )
		{
			m_pInterface = NULL;
		}

        radRef( IRefCount * pInterface )
		{
			m_pInterface = pInterface;

			if ( m_pInterface != NULL )
			{
		        radAddRef( m_pInterface, this );
			}
		}

        radRef( const radRef< IRefCount > & copy )
        { 
            m_pInterface = copy.m_pInterface;

            if ( m_pInterface != NULL )
            {
        		radAddRef( m_pInterface, this );
            }
        }

		~radRef( )
		{
			if ( m_pInterface != NULL )
			{
				IRefCount * pIOld = m_pInterface;
				m_pInterface = NULL;
                radRelease( pIOld, this );
			}
		}


		operator IRefCount*( ) { return m_pInterface; }

		IRefCount &  operator *( )
        {
            rAssert( m_pInterface != NULL );
            return *m_pInterface;
        }

		IRefCount ** operator &( )
        {
            rAssert( m_pInterface == NULL );
            return &m_pInterface;
        }

		IRefCount *  operator->( ) const
        {
            rAssert( m_pInterface != NULL );
            return m_pInterface;
        }

        radRef< IRefCount > & operator = ( const radRef< IRefCount > & copy )
        {
            if ( m_pInterface != NULL )
            {
                radRelease( m_pInterface, this );
            }
            
            m_pInterface = copy.m_pInterface;
            
            if ( m_pInterface != NULL )
            {
        		radAddRef( m_pInterface, this );
            }
            
            return *this;
        }
                     
		IRefCount* operator=( IRefCount * pInterface )
		{
            IRefCount * pOld = m_pInterface;
            m_pInterface = pInterface;

            if ( m_pInterface != NULL )
            {
        		radAddRef( m_pInterface, this );
            }

			if ( pOld != NULL )
			{
                radRelease( pOld, this );
			}

			return m_pInterface;
		}

        bool operator != ( IRefCount * pInterface ) const
        {
            return ( m_pInterface != pInterface );
        }

        bool operator == ( IRefCount * pInterface ) const
        {
            return ( m_pInterface == pInterface );
        }

        void * operator new[]( size_t size )
        {
            return ::radMemoryAlloc( radMemoryGetCurrentAllocator(), size);
        }

        void * operator new[]( size_t size, radMemoryAllocator allocator )
        {
            return ::radMemoryAlloc( allocator, size );
        }

        void operator delete[]( void * pMemory )
        {
            ::radMemoryFree( pMemory );
        }

        void * operator new( size_t size )
        {
            return ::radMemoryAlloc( radMemoryGetCurrentAllocator(), size);
        }
        
        void * operator new( size_t size, radMemoryAllocator allocator )
        {
            return ::radMemoryAlloc( allocator, size );
        }

        void operator delete( void * pMemory )
        {
            ::radMemoryFree( pMemory );
        }

		IRefCount * m_pInterface;
};
#endif // RADMEMORYMONITOR

#endif // RADOBJECT_HPP