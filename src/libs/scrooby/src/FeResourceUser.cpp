//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   FeResourceUser
//
// Description: Implementation of the FeResourceUser class.
//
// Authors:     Darwin Chau
//
// Revisions        Date            Author        Revision
//                  2000/07/13        DChau        Created
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================
#include <raddebug.hpp>
#include "tLinearTable.h"  // From Pure3D for tLinearTable

#include "FeResourceUser.h"
#include "FeResourceEntry.h"

#include "FeApp.h"
#include "ResourceManager/FeResourceManager.h"

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================


//===========================================================================
// Public Member Functions
//===========================================================================

//===========================================================================
// FeResourceUser::FeResourceUser
//===========================================================================
// Description: Constructor.
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      N/A.
//
//===========================================================================
FeResourceUser::FeResourceUser()
:
    mpImageResources( NULL ),
    mpTextStyleResources( NULL ),
    mpTextBibleResources( NULL ),
    mpMovieClipResources( NULL ),
    mp3dModelResources( NULL )
{
    //
    // Nothing to see here.
    //
}


//===========================================================================
// FeResourceUser::~FeResourceUser
//===========================================================================
// Description: Destructor.
//
// Constraints:    None.
//
// Parameters:    None.
//
// Return:      N/A.
//
//===========================================================================
FeResourceUser::~FeResourceUser()
{
    if( NULL != mpImageResources )
    {
        mpImageResources->Release();
    }

    if( NULL != mpTextBibleResources )
    {
        mpTextBibleResources->Release();
    }

    if( NULL != mpTextStyleResources )
    {
        mpTextStyleResources->Release();
    }

    if( NULL != mpMovieClipResources )
    {
        mpMovieClipResources->Release();
    }

    if( NULL != mp3dModelResources )
    {
        mp3dModelResources->Release();
    }

}


//===========================================================================
// FeResourceUser::AddImageResourceEntry
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
void FeResourceUser::AddImageResourceEntry
( 
    const char* alias,
    const char* filename
)
{
    rAssert( NULL != alias );
    rAssert( NULL != filename );

    //
    // Create a table the first time this is called.  
    //
    if( NULL == mpImageResources )
    {
        mpImageResources = new( ScroobyPermPool )tLinearTable;
        rValid( mpImageResources );
        
        mpImageResources->AddRef();
    }
      
    unsigned int handle;
    handle = FeApp::GetInstance()->GetFeResourceManager().AddFilenameSprite( filename );

    FeResourceEntry* pImageEntry = new ( ScroobyPermPool ) FeResourceEntry( alias, handle );
    
    mpImageResources->Store( pImageEntry );
}


//===========================================================================
// FeResourceUser::AddTextBibleResourceEntry
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
void FeResourceUser::AddTextBibleResourceEntry
( 
    const char* alias,
    const char* filename
)
{
    rAssert( NULL != alias );
    rAssert( NULL != filename );

    //
    // Create a table the first time this is called.  
    //
    if( NULL == mpTextBibleResources )
    {
        mpTextBibleResources = new( ScroobyPermPool ) tLinearTable;
        rValid( mpTextBibleResources );
        
        mpTextBibleResources->AddRef();
    }
      
    unsigned int handle = 0;
    handle = FeApp::GetInstance()->GetFeResourceManager().AddFilenameTextBible( filename );

    FeResourceEntry* pTextBibleEntry = new( ScroobyPermPool ) FeResourceEntry( alias, handle );
    
    mpTextBibleResources->Store( pTextBibleEntry );
}

//===========================================================================
// FeResourceUser::AddTextStyleResourceEntry
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
void FeResourceUser::AddTextStyleResourceEntry
( 
    const char* alias,
    const char* filename
)
{
    rAssert( NULL != alias );
    rAssert( NULL != filename );

    //
    // Create a table the first time this is called.  
    //
    if( NULL == mpTextStyleResources )
    {
        mpTextStyleResources = new( ScroobyPermPool ) tLinearTable;
        rValid( mpTextStyleResources );
        
        mpTextStyleResources->AddRef();
    }
      
    unsigned int handle;
    handle = FeApp::GetInstance()->GetFeResourceManager().AddFilenameTextStyle( filename );

    FeResourceEntry* pTextStyleEntry = new ( ScroobyPermPool ) FeResourceEntry( alias, handle );
    
    mpTextStyleResources->Store( pTextStyleEntry );
}


//===========================================================================
// FeResourceUser::AddMovieClipResourceEntry
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
void FeResourceUser::AddMovieClipResourceEntry
( 
    const char* alias,
    const char* filename
)
{
    rAssert( NULL != alias );
    rAssert( NULL != filename );

    //
    // Create a table the first time this is called.  
    //
    if( NULL == mpMovieClipResources )
    {
        mpMovieClipResources = new( ScroobyPermPool ) tLinearTable;
        rValid( mpMovieClipResources );
        
        mpMovieClipResources->AddRef();
    }
      
    unsigned int handle;
    handle = FeApp::GetInstance()->GetFeResourceManager().AddFilenameMovieClip( filename );

    FeResourceEntry* pMovieClipEntry = new( ScroobyPermPool ) FeResourceEntry( alias, handle );
    
    mpMovieClipResources->Store( pMovieClipEntry );
}


//===========================================================================
// FeResourceUser::Add3dModelResourceEntry
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
void FeResourceUser::Add3dModelResourceEntry
( 
    const char* alias,
    const char* filename,
    const char* inventoryName,
    const char* animationName,      //IAN IMPROVE: i'm casting to and from PascalCString all over the place here
    const char* cameraName
)
{
    rAssert( NULL != alias );
    rAssert( NULL != filename );
    rAssert( NULL != inventoryName );

    //
    // Create a table the first time this is called.  
    //
    if( NULL == mp3dModelResources )
    {
        mp3dModelResources = new( ScroobyPermPool ) tLinearTable;
        rValid( mp3dModelResources );
        
        mp3dModelResources->AddRef();
    }
      
    unsigned int handle;
    handle = FeApp::GetInstance()->GetFeResourceManager().AddFilename3dModel
    ( 
        filename, 
        alias, 
        inventoryName,
        animationName,
        cameraName
    );  //IAN IMPROVE: this should say alias, filename

    FeResourceEntry* p3dModelEntry = new( ScroobyPermPool ) FeResourceEntry( alias, handle );
    
    mp3dModelResources->Store( p3dModelEntry );
}


//===========================================================================
// FeResourceUser::FindImageResourceEntry
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
unsigned int FeResourceUser::FindImageResourceEntry
( 
    const char* alias
)
const
{
    tLinearTable::RawIterator iter( mpImageResources );
    
    tUID id = FeEntity::MakeUID( alias );

    FeEntity* pEntity = iter.First( id );

    FeResourceEntry* pResourceEntry = dynamic_cast<FeResourceEntry*>( pEntity );
    rValid( pResourceEntry );

    return( pResourceEntry->GetResourceHandle() );
}


//===========================================================================
// FeResourceUser::FindTextStyleResourceEntry
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
unsigned int FeResourceUser::FindTextStyleResourceEntry
( 
    const char* alias
)
const
{
    tLinearTable::RawIterator iter( mpTextStyleResources );
    
    tUID id = FeEntity::MakeUID( alias );

    FeEntity* pEntity = iter.First( id );

    FeResourceEntry* pResourceEntry = dynamic_cast<FeResourceEntry*>( pEntity );
    rValid( pResourceEntry );

    return( pResourceEntry->GetResourceHandle() );
}


//===========================================================================
// FeResourceUser::FindTextBibleResourceEntry
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
unsigned int FeResourceUser::FindTextBibleResourceEntry
( 
    const char* alias
)
const
{
    tLinearTable::RawIterator iter( mpTextBibleResources );
    
    tUID id = FeEntity::MakeUID( alias );

    FeEntity* pEntity = iter.First( id );

    FeResourceEntry* pResourceEntry = dynamic_cast<FeResourceEntry*>( pEntity );
    rValid( pResourceEntry );

    return( pResourceEntry->GetResourceHandle() );
}


//===========================================================================
// FeResourceUser::FindMovieClipResourceEntry
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
unsigned int FeResourceUser::FindMovieClipResourceEntry
( 
    const char* alias
)
const
{
    tLinearTable::RawIterator iter( mpMovieClipResources );
    
    tUID id = FeEntity::MakeUID( alias );

    FeEntity* pEntity = iter.First( id );

    FeResourceEntry* pResourceEntry = dynamic_cast<FeResourceEntry*>( pEntity );
    rValid( pResourceEntry );

    return( pResourceEntry->GetResourceHandle() );
}


//===========================================================================
// FeResourceUser::Find3dModelResourceEntry
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
unsigned int FeResourceUser::Find3dModelResourceEntry
( 
    const char* alias
)
const
{
    tLinearTable::RawIterator iter( mp3dModelResources );
    
    tUID id = FeEntity::MakeUID( alias );
    
    FeEntity* pEntity = iter.First( id );
    rAssert( pEntity != NULL );

    FeResourceEntry* pResourceEntry = dynamic_cast<FeResourceEntry*>( pEntity );
    rValid( pResourceEntry );

    return( pResourceEntry->GetResourceHandle() );
}
