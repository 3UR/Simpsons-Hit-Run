//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Component
//
// Description: FeProject.h - this class controls the project and all 
//
// Authors:     Ian Gipson
//
// Revisions        Date            Author    Revision
//                  2000/03/20      WNG     Created
//
//===========================================================================

#ifndef __FEPROJECT_H_
#define __FEPROJECT_H_

//===========================================================================
// Nested Includes
//===========================================================================
#include "FeEntity.h"

#include "FeParent.h"
#include "Project.h"
#include "strings/pcstring.h"
#include "resourcemanager/FeResourceManager.h"


//===========================================================================
// Forward References
//===========================================================================
class FeScreen;
class FePage;

namespace Scrooby
{
    class Page;
    class Screen;
}

//===========================================================================
// Interface Definitions
//===========================================================================

const int MAX_CUR_SCREENS = 32;
// Number of P3D_UNICODE chars in the buffer 
const int SCROOBY_TEXT_BUFFER_SIZE = 512;

//===========================================================================
// FeProject
//
// 
//===========================================================================
class FeProject 
: 
    public FeEntity,
    public FeParent,
    public Scrooby::ResourceManagerCallback,
    virtual public Scrooby::Project
{
public:
    
    FeProject( const char* fullFilePath );

    //gets the current screen
    virtual Scrooby::Screen* GetCurrentScreen();

    // Gets a screen by name
    virtual Scrooby::Screen* GetScreen( const char* name );

    // Gets a screen from a hash value
    virtual Scrooby::Screen* GetScreen( const tUID hashValue );

    // Request screen change by name
    virtual void GotoScreen( const char* name, Scrooby::GotoScreenCallback* pCallback );
    
    // Request screen change by hash value
    virtual void GotoScreen( const tUID hashValue, Scrooby::GotoScreenCallback* pCallback );

    //Request screen change by screen pointer
    virtual void GotoScreen( Scrooby::Screen* screen, Scrooby::GotoScreenCallback* pCallback );

    // Get the number of screens in this project.
    virtual int GetScreenCount( void );

    // Request screen change by index.
    virtual void GotoScreenByIndex( int screenIndex, Scrooby::GotoScreenCallback* pCallback );

    virtual void LoadScreen( Scrooby::Screen* screen, Scrooby::GotoScreenCallback* pCallback );
    virtual void UnLoadScreen( Scrooby::Screen* screen );
    virtual bool IsScreenLoaded( Scrooby::Screen* screen );
    virtual bool IsLoadingScreens() const;

    FePage* AddPage( const char* name );
    FePage* AddPage( FePage* page );
    FeScreen* AddScreen( const char* name );

    const char* GetFileName();
    const char* GetProjectPath();
    
    void SetScreenPath( const char* p );
    const char* GetScreenPath();
    
    void SetPagePath( const char* p );
    const char* GetPagePath();
    
    void SetResPath( const char* p );
    const char* GetResPath();

    tLinearTable* GetScreenList() const { return( mChildren ); };

    void SetScreenRes( float x, float y );
    void GetScreenRes( float &x, float &y );
    float GetScreenWidth();
    float GetScreenHeight();

    void SetLastScreenToLoad( int screen ) { mLastScreenToLoad = screen; };
    int GetLastScreenToLoad() { return mLastScreenToLoad; };

    void SetInventorySection( const char* section );
    const char* GetInventorySection();

    bool IsLoaded();

    void LoadComplete();

    void OnResourceLoadComplete();

    virtual bool IsDrawable() { return false; }

protected:

    // No copying. Declare but don't define.
    FeProject( const FeProject& );

    // No assignment. Declare but don't define.
    FeProject& operator=( const FeProject& );

    virtual ~FeProject();

    //called when the resources for loading this project have been loaded
    //virtual void OnResourceLoadComplete();

    void ShowScreen( FeScreen* pScreen, Scrooby::GotoScreenCallback* pCallback );        

    PascalCString mFileName;
    PascalCString mPagePath;
    PascalCString mProjectPath;     //IAN IMPROVE: this never seems to be used - remove it if so
    PascalCString mResPath;
    PascalCString mScreenPath;
    PascalCString mInventorySection;

    FeScreen* mpCurScreen;
    FeScreen* mpScreenToChangeTo;

    Scrooby::GotoScreenCallback* mLoadScreenCallback;
    FeScreen* mpScreenLoadQueue[MAX_CUR_SCREENS];
    int mScreenLoadQueueHead;
    int mScreenLoadQueueTail;
    bool mIsLoadingScreen;
    FeScreen* mpScreensToUnLoad[MAX_CUR_SCREENS];
    int mUnLoadScreenCount;
    int mCurrentScreenIndex;
    int mLastScreenToLoad;
    bool mIsLoading;

private:
    void LoadNextScreen();
    void AddObjectsToScreen();
    float mScreenXRes;
    float mScreenYRes;
};

//===========================================================================
// Inline Functions
//===========================================================================

inline const char* FeProject::GetFileName()
{
    return mFileName;
}
  
inline const char* FeProject::GetProjectPath()
{
    return mProjectPath;
}

inline const char* FeProject::GetScreenPath()
{
    return mScreenPath;
}
    
inline const char* FeProject::GetPagePath()
{
    return mPagePath; 
}
    
inline const char* FeProject::GetResPath()
{
    return mResPath; 
}

inline bool FeProject::IsLoadingScreens() const
{
    return( mScreenLoadQueueHead != mScreenLoadQueueTail );
}

inline void FeProject::SetInventorySection( const char* section )
{
    mInventorySection = section;
}

inline const char* FeProject::GetInventorySection()
{
    return mInventorySection;
}

inline bool FeProject::IsLoaded()
{
    return !mIsLoading;
}

#endif
