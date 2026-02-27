//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef RADSOUNDOBJECT_HPP
#define RADSOUNDOBJECT_HPP

#include <radsound.hpp>
#include <radmemorymonitor.hpp>
#include <typeinfo>
#undef new

//============================================================================
// Class: radSoundObject
//============================================================================

struct radSoundObject : public radRefCount
{
	inline radSoundObject( void );
    inline virtual ~radSoundObject( void );

    inline void * operator new ( size_t size, const char * pName, radMemoryAllocator allocator );
    
    unsigned int GetRefCount( void ) { return radRefCount::GetRefCount( ); }

    static const char * s_SoundObjectName;
    static unsigned int s_SoundObjectCount;
};

inline radSoundObject::radSoundObject( void )
    :
    radRefCount( 0 )
{
    ::radMemoryMonitorIdentifyAllocation(
        this,
        radSoundDebugChannel,
        s_SoundObjectName,
        NULL,
        radMemorySpace_Local );

    s_SoundObjectCount++;
}

inline radSoundObject::~radSoundObject( void )
{
    s_SoundObjectCount--;
}

inline void * radSoundObject::operator new ( size_t size, const char * pName, radMemoryAllocator allocator )
{
    s_SoundObjectName = pName;

    return radObject::operator new( size, allocator );
}

//============================================================================
// Class: radSoundPoolObject
//============================================================================

template < class T > class radSoundPoolObject
	:
	public radSoundObject
{
	public:

		static void Initialize( radMemoryAllocator allocator,
			unsigned int maxObjects, unsigned int growBy )
		{
			rAssert( s_pIRadMemoryPool == NULL );

			::radMemoryCreatePool(
				& s_pIRadMemoryPool,
				sizeof( T ), 
                maxObjects,
				growBy,
                false, 
                None,
                allocator );
		}
	
		static void Terminate( )
		{
			s_pIRadMemoryPool->Release( );
		}

		void * operator new ( size_t size, char * pName )
		{
            s_SoundObjectName = pName;

		    rAssertMsg( s_pIRadMemoryPool != NULL, "Class memory pool not initialized." );
			return s_pIRadMemoryPool->GetMemory( );
		}

		void operator delete ( void * pMemory )
		{
			s_pIRadMemoryPool->FreeMemory( pMemory );
		}

	private:

		static IRadMemoryPool * s_pIRadMemoryPool;
};

#endif // RADSOUNDOBJECT_HPP