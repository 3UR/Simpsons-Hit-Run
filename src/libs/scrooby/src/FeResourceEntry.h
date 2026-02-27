//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   FeResourceEntry
//
// Description: 
//
// Authors:     Darwin Chau
//
// Revisions        Date            Author        Revision
//                  2000/07/17        DChau        Created
//
//===========================================================================

#ifndef FERESOURCEENTRY_H
#define FERESOURCEENTRY_H

//===========================================================================
// Nested Includes
//===========================================================================
#include "FeEntity.h"

//===========================================================================
// Forward References
//===========================================================================

//===========================================================================
// Interface Definitions
//===========================================================================

class FeResourceEntry : public FeEntity
{
    public:

        FeResourceEntry( const char* alias, unsigned int resourceHandle );
        virtual ~FeResourceEntry();

        const char* GetAlias( void ) const;
        unsigned int GetResourceHandle( void ) const;

    private:

        //---------------------------------------------------------------------
        // Private Functions
        //---------------------------------------------------------------------

        // No copying. Declare but don't define.
        //
        FeResourceEntry( const FeResourceEntry& );

        // No assignment. Declare but don't define.
        //
        FeResourceEntry& operator= ( const FeResourceEntry& );

        //---------------------------------------------------------------------
        // Private Data
        //---------------------------------------------------------------------
        char* mAlias;
        unsigned int mResourceHandle;
};

inline const char* FeResourceEntry::GetAlias( void ) const
{
    return( mAlias );
}

inline unsigned int FeResourceEntry::GetResourceHandle( void ) const
{
    return( mResourceHandle );
}

#endif
