//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        saferefobject.hpp
//
// Subsystem:	Radical Drive System
//
// Description:	This file contains definitions for thread safe ref counted objects.
//
// Revisions:	
//
//=============================================================================

#ifndef SAFEREFOBJECT_HPP
#define SAFEREFOBJECT_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <raddebug.hpp>
#include <radobject.hpp>

//============================================================================
// Macros
//============================================================================

//
// The following macro implements a reference counted object
//
#define IMPLEMENT_SAFEREFCOUNTED \
    virtual void AddRef( void ) { radSafeRefCount::Implement_AddRef(); } \
	virtual void Release( void ) { radSafeRefCount::Implement_Release(); } \

//============================================================================
// Interface: IRadMutable
//
// Represents classes that can be locked and unlocked for thread safety
//============================================================================

struct IRadMutable
{
    virtual void Lock( void ) = 0;
    virtual void Unlock( void ) = 0;
};

//============================================================================
// Class: radSafeRefCount
//
// Implements thread safe ref counting; does not implement IRadMutable functions
//============================================================================

class radSafeRefCount : public IRadMutable
{
public:

    inline radSafeRefCount( unsigned int refCount = 1 );
    inline virtual ~radSafeRefCount( void );
    inline void Implement_AddRef( void );
    inline void Implement_Release( void );
	inline int GetRefCount ( void );
    
private:
    
    int m_RefCount;    
};

inline radSafeRefCount::radSafeRefCount( unsigned int refCount )
    :
    m_RefCount( (int) refCount )
{
}

inline radSafeRefCount::~radSafeRefCount( void )
{
}

inline void radSafeRefCount::Implement_AddRef( void )
{
    Lock( );
    rAssert( m_RefCount < MAX_REFCOUNT );
    m_RefCount++;
    Unlock( );
}

inline void radSafeRefCount::Implement_Release( void )
{
    Lock( );
    rAssert( m_RefCount > 0 && m_RefCount <  MAX_REFCOUNT );
    m_RefCount--;
    if (m_RefCount == 0 )
    {
		// Must avoid recursive destruction, set refcount to some high
		// value.

		m_RefCount = MAX_REFCOUNT / 2;

        Unlock( );
        delete this;
    }
    else
    {
        Unlock( );
    }
}

inline int radSafeRefCount::GetRefCount ( void ) 
{ 
    return m_RefCount; 
}

#endif