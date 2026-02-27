//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef RADSOUNDUPDATABLEOBJECT_HPP
#define RADSOUNDUPDATABLEOBJECT_HPP

#include "radsoundobject.hpp"
#include <typeinfo>

struct radSoundUpdatableObject : public radSoundObject
{
    public:

        static inline void UpdateAll( unsigned int elapsed );

    protected:

        inline radSoundUpdatableObject( void );
        inline virtual ~radSoundUpdatableObject( void );

        inline void AddToUpdateList( void );
        inline void RemoveFromUpdateList( void );
        inline bool OnUpdateList( void );

    private:

        virtual void Update( unsigned int elapsed ) = 0;
    
        radSoundUpdatableObject * m_pRadSoundUpdateNext;
        radSoundUpdatableObject * m_pRadSoundUpdatePrev;

        static radSoundUpdatableObject * s_pRadSoundUpdateHead;
        static unsigned int              s_UpdatableCount;
        static unsigned int              s_UpdatingCount;
        static bool                      s_Updating;
        static bool                      s_ListWasChanged;
};

inline bool radSoundUpdatableObject::OnUpdateList( void )
{
    if ( m_pRadSoundUpdateNext != NULL || m_pRadSoundUpdatePrev != NULL ||
        s_pRadSoundUpdateHead == this )
    {
        return true;
    }

    return false;
}

inline radSoundUpdatableObject::radSoundUpdatableObject( void )
    :
    m_pRadSoundUpdateNext( NULL ),
    m_pRadSoundUpdatePrev( NULL )
{
    s_UpdatableCount++;
}

inline void radSoundUpdatableObject::AddToUpdateList( void )
{
    s_ListWasChanged = true;

    if( ! OnUpdateList( ) )
    {
        m_pRadSoundUpdateNext = s_pRadSoundUpdateHead;

        if ( m_pRadSoundUpdateNext != NULL )
        {
            m_pRadSoundUpdateNext->m_pRadSoundUpdatePrev = this;
        }
        m_pRadSoundUpdatePrev = NULL;
        s_pRadSoundUpdateHead = this;

        s_UpdatingCount++;
    }
}

inline void radSoundUpdatableObject::RemoveFromUpdateList( void )
{
    s_ListWasChanged = true;

    // Make sure we are actuall on the list

    if ( OnUpdateList( ) )
    {
        if ( m_pRadSoundUpdatePrev != NULL )
        {
            m_pRadSoundUpdatePrev->m_pRadSoundUpdateNext = m_pRadSoundUpdateNext;
        }
        else
        {
            s_pRadSoundUpdateHead = m_pRadSoundUpdateNext;
        }

        if ( m_pRadSoundUpdateNext != NULL )
        {
            m_pRadSoundUpdateNext->m_pRadSoundUpdatePrev = m_pRadSoundUpdatePrev;
        }

        m_pRadSoundUpdatePrev = NULL;
        m_pRadSoundUpdateNext = NULL;

        s_UpdatingCount--;
    }
}

inline radSoundUpdatableObject::~radSoundUpdatableObject( void )
{
    RemoveFromUpdateList( );

    s_UpdatableCount--;
}

/* static */ inline void radSoundUpdatableObject::UpdateAll( unsigned int elapsed )
{
    if ( s_Updating == false )
    {
        s_Updating = true;
        s_ListWasChanged = false;

        radSoundUpdatableObject * pRsuo = s_pRadSoundUpdateHead;

        while ( pRsuo != NULL )
        {
            //rDebugPrintf( "Updating: [%s]\n", typeid( *pRsuo ).name( ) );

            pRsuo->Update( elapsed );
        
            if ( s_ListWasChanged == true )
            {
                s_ListWasChanged = false;
                pRsuo = s_pRadSoundUpdateHead;
            
            }
            else
            {
                pRsuo = pRsuo->m_pRadSoundUpdateNext;
            }
        }
    
        s_Updating = false;
    }
    else
    {
        s_ListWasChanged = true; // start again if service requested
    }
}

#endif // RADSOUNDUPDATABLEOBJECT_HPP