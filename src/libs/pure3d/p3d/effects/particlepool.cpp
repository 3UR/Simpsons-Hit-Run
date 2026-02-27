/*===========================================================================
    particlepool.cpp
    15-April-2002 Created by Kevin Voon

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#include <p3d/error.hpp>
#include <p3d/effects/particletype.hpp>
#include <p3d/effects/particlepool.hpp>

const int MAX_NUM_PARTICLES = 1000;

// Michael Riegger
// Modifications that create a static particle array instead of allocating a tParticle
// for each possible instance.

// Templated List class implemented on an fixed length array to avoid fragmentation
// From Intro to Algorithms.

template < typename T >
class Node
{
public:
	int next;
	T	element;
};

template < typename T >
class List
{
public:
	List( size_t numElems);
	~List();
	
	short insert( T );
    short nextFree( ); // Equiv to insert, but doesn't copy anything into the list
	void remove( short index );
	T& operator[]( short i );
	size_t size() const { return mNumElements; }

private:
	Node< T >*	mData;
	short mFree;
	size_t mNumElements;
};


template < typename T >
List< T >::List( size_t numElems )
{
	mFree = 0;
	mNumElements = numElems;
	mData = new Node< T >[ numElems ];
	for ( size_t i = 0 ; i < numElems - 1 ; i++ )
	{
		mData[ i ].next = i + 1;
	}
	mData[ numElems - 1 ].next = -1;
}



template < typename T >
List< T >::~List()
{
	delete [] mData;
}

template < typename T >
short List< T >::insert( T newElement )
{

	short index;
	if ( mFree != -1 )
	{
		index = mFree;
		mData[ index ].element = newElement;
		mFree = mData[ index ].next;

	}
	else
	{
		index = -1;
	}
	return index;
}

template < typename T >
short List< T >::nextFree( )
{
    short index;
    if ( mFree != -1 )
    {
        index = mFree;
        mFree = mData[ index ].next;
    }
    else
    {
        index = -1;
    }
    return index;
}


template < typename T >
void List< T >::remove( short index )
{
	assert( index >=0 && index < static_cast<short>(mNumElements) );

    mData[ index ].next = mFree;
    mFree = index;
}
template < typename T >
T& List< T >::operator[]( short index )
{
	assert( index >=0 && index < static_cast<short>(mNumElements) );
	return mData[ index ].element;
}

// Create an array of tSpriteParticles

static List< tSpriteParticle > s_SpriteParticleList( MAX_NUM_PARTICLES );

//*****************************************************************************
//
// Class tParticlePool
//
//*****************************************************************************
tParticlePool::tParticlePool() :
    tRefCounted(),
    particleType(p3dParticleSystemConstants::UNDEFINED),
    numSubPools(0),
    numAvailableSubPools(0),
    numParticlesPerSubPool(0)
{
}

tParticlePool::tParticlePool(unsigned type, unsigned int nParticles, unsigned int nSubPools) :
    tRefCounted(),
    particleType(p3dParticleSystemConstants::UNDEFINED),
    numSubPools(0),
    numAvailableSubPools(0),
    numParticlesPerSubPool(0)
{
    P3DASSERT((nParticles>0)&&(nSubPools>0));
    switch (type)
    {
        case p3dParticleSystemConstants::ParticleType::SPRITE:
        {
            particleType = type;
            numSubPools = nSubPools;
            numAvailableSubPools = numSubPools;
            numParticlesPerSubPool = nParticles;
            
            break;
        }
        default:
        {
            P3DASSERT(particleType == p3dParticleSystemConstants::UNDEFINED);
            break;
        }
    }
}

tParticlePool::~tParticlePool()
{

    switch (particleType)
    {
        case p3dParticleSystemConstants::ParticleType::SPRITE:
        {

            break;
        }
        default:
        {
            break;
        }
    }
}
  
bool tParticlePool::RequestParticle( tBaseParticle** newParticle, short* newParticleIndex )
{
    P3DASSERT( newParticleIndex );
    P3DASSERT( newParticle );
   
    short index = s_SpriteParticleList.nextFree( );

    bool success;
    if ( index >= 0 )
    {
        *newParticleIndex = index;
        *newParticle = &s_SpriteParticleList[ index ];
        success = true;
    }
    else
    {
        success = false;
    }
    return success;

}

void tParticlePool::ReleaseParticle( short particleIndex )
{
    s_SpriteParticleList.remove( particleIndex );
}

