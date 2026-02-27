//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   FeResourceUser
//
// Description: 
//
// Authors:     Darwin Chau
//
// Revisions        Date            Author        Revision
//                  2000/07/12        DChau        Created
//
//===========================================================================

#ifndef FERESOURCEUSER_H
#define FERESOURCEUSER_H

//===========================================================================
// Nested Includes
//===========================================================================

//===========================================================================
// Forward References
//===========================================================================
class tEntityTable;

//===========================================================================
// Interface Definitions
//===========================================================================
class FeResourceUser
{
    public:

        FeResourceUser();
        virtual ~FeResourceUser();
        
        void AddImageResourceEntry( const char* alias, const char* filename );
        void AddTextStyleResourceEntry( const char* alias, const char* filename );
        void AddTextBibleResourceEntry( const char* alias, const char* filename );
        void AddMovieClipResourceEntry( const char* alias, const char* filename );
        void Add3dModelResourceEntry
        ( 
            const char* alias, 
            const char* filename, 
            const char* inventoryName, 
            const char* animationName,
            const char* cameraName
        );

        unsigned int FindImageResourceEntry( const char* alias ) const;
        unsigned int FindTextStyleResourceEntry( const char* alias ) const;
        unsigned int FindTextBibleResourceEntry( const char* alias ) const;
        unsigned int FindMovieClipResourceEntry( const char* alias ) const;
        unsigned int Find3dModelResourceEntry( const char* alias ) const;

    private:

        //---------------------------------------------------------------------
        // Private Functions
        //---------------------------------------------------------------------

        // No copying. Declare but don't define.
        //
        FeResourceUser( const FeResourceUser& );

        // No assignment. Declare but don't define.
        //
        FeResourceUser& operator= ( const FeResourceUser& );


        //---------------------------------------------------------------------
        // Private Data
        //---------------------------------------------------------------------
        tLinearTable* mpImageResources;
        tLinearTable* mpTextStyleResources;
        tLinearTable* mpTextBibleResources;
        tLinearTable* mpMovieClipResources;
        tLinearTable* mp3dModelResources;
};


#endif


