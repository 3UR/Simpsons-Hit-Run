//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef MEMORYSPACEOBJECT_HPP
#define MEMORYSPACEOBJECT_HPP

#include <radstring.hpp>
#include <radsoundobject.hpp>
#include <radsound.hpp>

struct radSoundMemorySpaceObject
    :
    public IRadSoundMemorySpaceObject,
    public radSoundObject
{
    IMPLEMENT_REFCOUNTED( "radSoundMemorySpaceObject" )

    radSoundMemorySpaceObject( void );
    virtual ~radSoundMemorySpaceObject( void );

    virtual void Initialize(
        radMemorySpace space,
        IRadMemoryAllocator * pIRadMemoryAllocator,
        unsigned int numberOfBytes,
        unsigned int alignment,
        const char * pIdentifier );
    
    virtual void *                GetAddress( void );
    virtual IRadMemoryAllocator * GetAllocator( void );
    virtual radMemorySpace        GetMemorySpace( void );
    virtual unsigned int          GetSizeInBytes( void );
    virtual const char *          GetIdentifier( void );

    private:

        void *             m_pMemory;
        radMemorySpace     m_RadMemorySpace;
        unsigned int       m_NumberOfBytes;
        radRef< IRadMemoryAllocator > m_xIRadMemoryAllocator;
        radRef< IRadString >          m_xIRadString_Identifier;
};

#endif