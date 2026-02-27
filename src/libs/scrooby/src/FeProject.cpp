//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   FeProject
//
// Description: implementation of the FeProject class. This class handles the
//              project and all associated screens
//
// Authors:     WNG     20 MAR 2000
//              IJG        10 JUL 2000
//
//===========================================================================


//===========================================================================
// Includes
//===========================================================================
#include "FeProject.h"
#include "FeApp.h"
#include "FePage.h"
#include "FeScreen.h"
#include "utility/debugmessages.h"
#include "ResourceManager/FeResourceManager.h"
#include "strings/pcstring.h"
#include "tLinearTable.h"

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================

//===========================================================================
// Member Functions
//===========================================================================

//===========================================================================
// FeProject::FeProject( const char* fullFilePath ) 
//===========================================================================
// Description: constructor
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
FeProject::FeProject( const char* fullFilePath ) 
:   mpCurScreen( NULL ),
    mpScreenToChangeTo( NULL ),
    mIsLoadingScreen( false ),
    mUnLoadScreenCount( 0 ),
    mLastScreenToLoad( 0 ),
    mIsLoading( true ),
    mScreenXRes( 0.0f ),
    mScreenYRes( 0.0f )
{
    radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
        PascalCString pathParser( fullFilePath );
        PascalCString fullFileName = pathParser.FullFilename();
    ::radMemorySetCurrentAllocator( old );
    mFileName = fullFileName;
    
    mProjectPath.Grow( pathParser.Length() + 3 );
    old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
        PascalCString justPath = pathParser.JustPath();
        PascalCString addMe = "\\";
    ::radMemorySetCurrentAllocator( old );
    mProjectPath = justPath;
    mProjectPath += addMe;

    FeEntity::SetName( fullFileName );
    mLoadScreenCallback = NULL;
}

FeProject::~FeProject()
{
    while( GetChildrenCount() )
    {
        FeEntity* child = GetChildIndex( GetChildrenCount()-1 );
        RemoveChild( child );
        FeApp::GetInstance()->GetFeResourceManager().RemoveResource( child );
    }

    mpCurScreen = NULL;
    mpScreenToChangeTo = NULL;
}



FeScreen* FeProject::AddScreen( const char* name )
{

    FeScreen* s = new FeScreen( name, this );
    AddChild( s );
    return( s );
}

FePage* FeProject::AddPage( const char* name )
{
    /*
    //
    // Remove the file extension before searching for the page name.
    //
    lPath pathParser( name );
    
    FePage* page = dynamic_cast<FePage*>( mpPages->Find( FeEntity::MakeUID( pathParser.FileName() ) ) );
    
    if( page == NULL ) 
    {
        page = new ( ScroobyPermPool ) FePage( name, this ) ;
        mpPages->Store( page );
    }
    else
    {
        //this clause means that we tried to Add a Page that had already been added
    }
    
    return( page );
    */
    return NULL;
}

FePage* FeProject::AddPage( FePage* page )
{
    /*rAssert( page != NULL );
    mpPages->Store( page );
    
    return( page );
    */
    return NULL;
}


//===========================================================================
// GetCurrentScreen()
//===========================================================================
// Description: Gets the current screen
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      Pointer to the current Scrooby::Screen.
//
//===========================================================================
//gets the current screen
Scrooby::Screen* FeProject::GetCurrentScreen()
{
    return mpCurScreen ? mpCurScreen : dynamic_cast<Scrooby::Screen*>( GetChildIndex( 0 ) );
}


int FeProject::GetScreenCount( void )
{
    return( GetChildrenCount() );
}

//===========================================================================
// GotoScreenByIndex
//===========================================================================
// Description: allows you to go to a screen by the number - 1,2,3...
//
// Constraints:    this screen must exist
//
// Parameters:    screenIndex - which screen should we go to
//              pCallback - the callback to trigger when successful
//
// Return:      None.
//
//===========================================================================
void FeProject::GotoScreenByIndex( int screenIndex, Scrooby::GotoScreenCallback* pCallback )
{
    // IAN IMPROVE: I put in this ugly hack because the PC controller code
    // doesn't co-operate with the Foundation dispatcher and can make bad
    // things happen.  In this case, the Viewer app is in the middle of
    // drawing a frame, when the user pushes a controller button which
    // forces a screen change while we're still in the middle of drawing.
    //
    // The proper fix is to prevent the controller from interrupting.
    //
    /*if( FeApp::GetInstance()->IsDrawingFrame() )
    {
        Scrooby::Log::Message( Scrooby::LVL_WARNING, "Ignored screen change request" );
        if( pCallback != NULL )
        {
            pCallback->OnGotoScreenComplete();
        }
        return;
    }*/
    
    FeScreen* screen = dynamic_cast<FeScreen*>( GetChildIndex( screenIndex ) );

    if( screen )
    {
        GotoScreen( screen, pCallback );
      }
}

//===========================================================================
// GotoScreen
//===========================================================================
// Description: allows you to go to a screen by it's name
//
// Constraints:    this screen must exist
//
// Parameters:    name - string representing the screen to go to
//              pCallback - the callback to trigger when successful
//
// Return:      None.
//
//===========================================================================
void FeProject::GotoScreen( const char* name, Scrooby::GotoScreenCallback* pCallback )
{
    //Scrooby::Log::Message( Scrooby::LVL_INFO, "FeProject::GotoScreen - %s", name );
    
    PascalCString newName = name;
    if( newName.JustExtension() == "" )
    {
        newName += ".scr";
    }
    tUID uid = FeEntity::MakeUID( newName );
    GotoScreen( uid, pCallback );
}

//===========================================================================
// GotoScreen
//===========================================================================
// Description: allows you to go to a screen by it's hashed name
//
// Constraints:    this screen must exist
//
// Parameters:    hashValue - the hashed name of the screen
//              pCallback - the callback to trigger when successful
//
// Return:      None.
//
//===========================================================================
void FeProject::GotoScreen( const tUID hashValue, Scrooby::GotoScreenCallback* pCallback )
{
    FeScreen* screen = dynamic_cast<FeScreen*>( mChildren->Find( hashValue ) );
    
    if( screen )
    {
        GotoScreen( screen, pCallback );
    }
}

void FeProject::GotoScreen( Scrooby::Screen* screen, Scrooby::GotoScreenCallback* pCallback )
{
    FeScreen* fescreen = dynamic_cast<FeScreen*>( screen );

    if( fescreen )
    {
        mpCurScreen = fescreen;

        if( pCallback )
        {
            if( IsLoaded() )
            {
                pCallback->OnGotoScreenComplete();
                pCallback = NULL;
            }
            else
            {
                // TC: I'm assuming the OnGotoScreenComplete() callback always gets
                //     called immediately within the same call to GotoScreen().
                //     If not, I'm gonna have to change a few things, just to be safe.
                //
                rReleaseAssertMsg( 0, "*** WARNING: Tell Tony if you encounter this assert!" );

                mLoadScreenCallback = pCallback;
            }
        }
    }
}

void FeProject::SetScreenPath( const char* p)
{
    mScreenPath = p;
}

void FeProject::SetPagePath( const char* p)
{
    mPagePath = p;
}

void FeProject::SetResPath( const char* p)
{
    mResPath = p;
}

//===========================================================================
// ShowScreen
//===========================================================================
// Description: this function actually accomplishes the swapping of two 
//              screens
//
// Constraints:    NONE
//
// Parameters:    screen - the new screen that should be shown
//
// Return:      NONE
//
//===========================================================================
void FeProject::ShowScreen( FeScreen* screen, Scrooby::GotoScreenCallback* pCallback )
{
    mpScreenToChangeTo = screen;

    // if there is a current screen, but it is not in the preloaded section, remove it
    // 
    if( mpCurScreen )
    {
        bool bIsPreLoaded = false;

        if( !bIsPreLoaded )
        {
            //this->UnLoadScreen( mpCurScreen );
        }
    }

    if( this->IsScreenLoaded( screen ) )
    {
        this->AddObjectsToScreen();
    }
    else
    {
        this->LoadScreen( screen, pCallback );
    }
}

//===========================================================================
// LoadScreen
//===========================================================================
// Description: 
//              
//
// Constraints:    NONE
//
// Parameters:    
//
// Return:      NONE
//
//===========================================================================
void FeProject::LoadScreen( Scrooby::Screen* screen, Scrooby::GotoScreenCallback* pCallback )
{
    /*int i;
    FeApp::GetInstance()->MarkUnsafeToRender();

    // check if already loaded (either in preloaded screens or current screen)
    //
    for( i = 0; i < mPreLoadedScreenCount; i++ )
    {
        if( mpPreLoadedScreen[i] == screen )
        {
            //screen = NULL;
        }
    }
    if( mpCurScreen == screen )
    {
        screen = NULL;
    }
    // check if already in load queue
    // 
    for( i = mScreenLoadQueueHead; i != mScreenLoadQueueTail; )
    {
        if( mpScreenLoadQueue[i] == screen )
        {
            screen = NULL;
        }
        if( ++i >= MAX_CUR_SCREENS )
        {
            i = 0;
        }
    }
    // make sure it's not in remove queue
    //
    for( i = 0; i < mUnLoadScreenCount; i++ )
    {
        if( mpScreensToUnLoad[i] == screen )
        {
            // Remove from unload queue
            // 
            mpScreensToUnLoad[i] = mpScreensToUnLoad[--mUnLoadScreenCount];
            break;
        }
    }

    // if screen is not already loaded or in queue
    //
    if( screen != NULL )
    {
        // set the load callback
        //
        this->mLoadScreenCallback[mScreenLoadQueueTail] = pCallback;
        if( pCallback )
        {
            pCallback->SetScreen( screen );
        }

        // add to queue
        // 
        mpScreenLoadQueue[mScreenLoadQueueTail] = dynamic_cast<FeScreen*>( screen );
        rAssert( mpScreenLoadQueue[mScreenLoadQueueTail] );

        // if this is not the screen to change to, then it is for preloading
        // 
        if( mpScreenToChangeTo != screen )
        {
            mLoadForPreLoad[mScreenLoadQueueTail] = true;
        }
        else
        {
            rAssert( !mLoadForPreLoad[mScreenLoadQueueTail] );
        }

        if( ++mScreenLoadQueueTail >= MAX_CUR_SCREENS )
        {
            mScreenLoadQueueTail = 0;
        }
        rAssert( mScreenLoadQueueTail != mScreenLoadQueueHead );

        this->LoadNextScreen();
    }*/
}

//===========================================================================
// UnLoadScreen
//===========================================================================
// Description: 
//              
//
// Constraints:    NONE
//
// Parameters:    screen - the new screen that should be unloaded
//
// Return:      NONE
//
//===========================================================================
void FeProject::UnLoadScreen( Scrooby::Screen* screen )
{
    /*bool bScreenFound = ( screen == mpCurScreen ) ? true : false;
    int i;

    if( !bScreenFound )
    {
         // find the screen in the list and make sure it's there
        //
        for( i = 0; i < mPreLoadedScreenCount; i++ )
        {
            if( mpPreLoadedScreen[i] == screen )
            {
                bScreenFound = true;
                break;
            }
        }
    }

    if( bScreenFound )
    {
        // check if it's already in remove queue
        //
        for( i = 0; i < mUnLoadScreenCount; i++ )
        {
            if( mpScreensToUnLoad[i] == screen )
            {
                screen = NULL;
                break;
            }
        }
            
        // if screen not already in remove queue, add it
        //
        if( screen )
        {
            rAssert( mUnLoadScreenCount < MAX_CUR_SCREENS );
            mpScreensToUnLoad[mUnLoadScreenCount] = dynamic_cast<FeScreen*>( screen );
            rAssert( mpScreensToUnLoad[mUnLoadScreenCount] );
            mUnLoadScreenCount++;
        }
    }*/
}

//===========================================================================
// IsScreenLoaded
//===========================================================================
// Description: 
//              
//
// Constraints:    NONE
//
// Parameters:    
//
// Return:      NONE
//
//===========================================================================
bool FeProject::IsScreenLoaded( Scrooby::Screen* screen )
{
    /*if( screen == mpCurScreen )
    {
        return true;
    }*/

    // check if already loaded
    //
/*    for( int i = 0; i < mPreLoadedScreenCount; i++ )
    {
        if( mpPreLoadedScreen[i] == screen )
        {
            return true;
        }
    }*/
    return false;

}

//===========================================================================
// Scrooby::Screen* FeProject::GetScreen( const char* name ) 
//===========================================================================
// Description: gets a screen from within a project by name
//
// Constraints:    
//
// Parameters:    char* name - the name of the screen
//
// Return:      a pointer to the page or null if nothing was found
//
//===========================================================================
Scrooby::Screen* FeProject::GetScreen( const char* name ) 
{
    radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
        PascalCString newName = name;
        if( !newName.JustExtension().EqualsInsensitive( "scr" ) )
        {
            newName += ".scr";
        }
    ::radMemorySetCurrentAllocator( old );
    FeEntity* theEntity = GetChild( newName ) ;
    FeScreen* theScreen = dynamic_cast< FeScreen* >( theEntity ) ;
    return( theScreen );
}

//===========================================================================
// Scrooby::Screen* FeProject::GetScreen( const tUID hashValue )
//===========================================================================
// Description: gets a screen from within a project by name
//
// Constraints:    
//
// Parameters:    char* name - the name of the screen
//
// Return:      a pointer to the screen or null if nothing was found
//
//===========================================================================
Scrooby::Screen* FeProject::GetScreen( const tUID hashValue )
{
    FeEntity* theEntity = GetChild( hashValue ) ;
    FeScreen* theScreen = dynamic_cast< FeScreen* >( theEntity ) ;
    rAssert( theScreen != NULL ) ;
    return theScreen ;
}

//===========================================================================
// OnResourceLoadComplete()
//===========================================================================
// Description: called when all the resources for this screen are loaded
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
/*void FeProject::OnResourceLoadComplete()
{
    Scrooby::GotoScreenCallback* pCallback = mLoadScreenCallback[mScreenLoadQueueHead];
    mLoadScreenCallback[mScreenLoadQueueHead] = NULL;

    // if this screen is to be put into preload queue
    // 
    if( mLoadForPreLoad[mScreenLoadQueueHead] )
    {
        mLoadForPreLoad[mScreenLoadQueueHead] = false;

        //
        // walk through the list of preloaded screens, to see if it's already there
        //
        bool isItAlreadyThere = false;
        int i;
        int size = MAX_CUR_SCREENS;
        for( i = 0; i < size; i++ )
        {
            if( mpPreLoadedScreen[ i ] == mpScreenLoadQueue[mScreenLoadQueueHead] )
            {
                isItAlreadyThere = true;
            }
        }

        //
        // move loaded screen from queue to our list of current screens
        // 
        if( !isItAlreadyThere )
        {
            rAssert( mPreLoadedScreenCount < MAX_CUR_SCREENS );
            mpPreLoadedScreen[mPreLoadedScreenCount] = mpScreenLoadQueue[mScreenLoadQueueHead];
            mPreLoadedScreenCount++;
        }
    }
    //  if this is the screen to change to
    //
    if( mpScreenToChangeTo == mpScreenLoadQueue[mScreenLoadQueueHead] )
    {
        this->AddObjectsToScreen();
    }

    // update queue and process next
    //
    if( ++mScreenLoadQueueHead >= MAX_CUR_SCREENS )
    {
        mScreenLoadQueueHead = 0;
    }
    mIsLoadingScreen = false;

    mCurrentScreenIndex = 0;
    mpCurScreen = dynamic_cast<FeScreen*>( this->GetChildIndex( 0 ) );

    //these callback functions are now 
    if( pCallback != NULL )
    {
        pCallback->OnGotoScreenComplete();
    }
}*/

void FeProject::LoadNextScreen()
{
    // if screen is not currently loading
    // 
    if( ++mCurrentScreenIndex > GetChildrenCount() )
    {
        mCurrentScreenIndex = 0;
    }
    mpCurScreen = dynamic_cast<FeScreen*>( GetChildIndex( mCurrentScreenIndex ) );
    if( !mIsLoadingScreen )
    {
        // if there's something in the queue
        // 
        if( mScreenLoadQueueHead != mScreenLoadQueueTail )
        {
            mIsLoadingScreen = true;
            //tell the resource manager to load the resources for this screen
            //IAN IMPROVE: this should use the callback provided
            //FeApp::GetInstance()->GetFeResourceManager().LoadChildren( mpScreenLoadQueue[mScreenLoadQueueHead], this ) ;
            FeApp::GetInstance()->GetFeResourceManager().ContinueLoading();
        }
    }
}

void FeProject::AddObjectsToScreen()
{
    /*//
    // Wipeout the scene element list
    //
    //Fe2DCore::GetInstance()->ClearObjects();
        
    //
    // Unload any screens in unload list
    //
    while( mUnLoadScreenCount )
    {
        mUnLoadScreenCount--;
        //FeApp::GetInstance()->GetFeResourceManager().UnloadChildren( mpScreensToUnLoad[mUnLoadScreenCount] );

        int i;

        //
        // Remove it from preloaded screen list
        //
        for( i = 0; i < mPreLoadedScreenCount; i++ )
        {
            if( mpPreLoadedScreen[i] == mpScreensToUnLoad[mUnLoadScreenCount] )
            {
                mpPreLoadedScreen[i] = mpPreLoadedScreen[--mPreLoadedScreenCount];
                break;
            }
        }
        //
        // remove from current screen if there
        //
        if( mpCurScreen == mpScreensToUnLoad[mUnLoadScreenCount] )
        {
            mpCurScreen = NULL;
        }
    }

    mpCurScreen = mpScreenToChangeTo;
    mpCurScreen->Show();*/
}

void FeProject::SetScreenRes( float x, float y )
{
    mScreenXRes = x;
    mScreenYRes = y;
}

void FeProject::GetScreenRes( float &x, float &y )
{
    x = mScreenXRes;
    y = mScreenYRes;
}

float FeProject::GetScreenWidth()
{
    return mScreenXRes;
}

float FeProject::GetScreenHeight()
{
    return mScreenYRes;
}

void FeProject::LoadComplete()
{
}

void FeProject::OnResourceLoadComplete()
{
    mIsLoading = false;
    if( mLoadScreenCallback )
    {
        mLoadScreenCallback->OnGotoScreenComplete();
        mLoadScreenCallback = NULL;
    }    
}


