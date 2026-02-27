//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef RADSOUNDMEMORYREGION_HPP
#define RADSOUNDMEMORYREGION_HPP

#include <radsoundobject.hpp>

struct radSoundHalMemoryRegion
	:
	public IRadSoundHalMemoryRegion,
	public radSoundObject
{
	IMPLEMENT_REFCOUNTED( "radSoundHalMemoryRegion" )

	//
	// Static Functions
	//

	static void Initialize( void * pStartOfMemory,
		unsigned int rootSize,
        unsigned int maxRootAllocations,
        unsigned int alignment,
        radMemorySpace memorySpace,
		radMemoryAllocator allocator );

	static radSoundHalMemoryRegion * GetRootRegion( void );

	static unsigned int GetTotalFreeMemory(
		IRadSoundHalMemoryRegion * IRadSoundHalMemoryRegion );

	static void Terminate( void );

	//
	// Class functions
	//

	radSoundHalMemoryRegion( void );
	virtual ~radSoundHalMemoryRegion( void );

	virtual IRadSoundHalMemoryRegion * CreateChildRegion(
		unsigned int sizeInBytes,
        unsigned int maxAllocations,
        const char * pIdentifier );

	void CreateMemoryObject( IRadMemoryObject ** ppIRadMemoryObject,
		unsigned int size,
        const char * pIdentifier );

	virtual IRadSoundHalMemoryRegion * GetParent( void );
	virtual IRadSoundHalMemoryRegion * GetFirstChild( void );
	
	virtual IRadSoundHalMemoryRegion * GetNextChild(
		IRadSoundHalMemoryRegion * pIRSoundMemoryRegion_LastChild );

	virtual unsigned int GetSize( void );

	virtual void GetStats( unsigned int * pFreeMemory,
		unsigned * pNumObjects, unsigned int * pLargestBlock,
		bool recurseChildren );

	private:

		static void GetStats_Recurse(
			radSoundHalMemoryRegion * pRadSoundMemoryRegion,
			unsigned int * pFreeMemory, unsigned int * pNumObjects,
			unsigned int * pLargestBlock,
			bool recurse
		);

		void Create( radSoundHalMemoryRegion * pRadSoundMemoryRegion_Parent,
			IRadMemoryObject * pIRadMemoryObject,
            unsigned int maxAllocations );

		void CreateRoot
		(
			void * pStartOfMemory,
			unsigned int sizeInBytes,
            unsigned int maxAllocations
		);

		radSoundHalMemoryRegion * m_pMemoryRegion_Next;
		radSoundHalMemoryRegion * m_pMemoryRegion_Prev;
		radSoundHalMemoryRegion * m_pMemoryRegion_ChildHead;

		radRef< radSoundHalMemoryRegion > m_xRadSoundMemoryRegion_Parent;
		
		radRef< IRadMemoryObject > m_xIRadMemoryObject_HeapMemory;
		radRef< IRadMemoryHeap >   m_xIRadMemoryHeap;

		static radSoundHalMemoryRegion * s_pRadSoundMemoryRegion_Root;
		static unsigned int s_Alignment;
        static radMemorySpace s_MemorySpace;

};

#endif // RADSOUNDMEMORY_REGION