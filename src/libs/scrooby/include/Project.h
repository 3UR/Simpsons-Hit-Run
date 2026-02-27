//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This contains the Scrooby::Project class.
//
// Authors:     Ian Gipson
//
//===========================================================================

// Recompilation protection flag.
#ifndef SCROOBY_PROJECT_H
#define SCROOBY_PROJECT_H

//===========================================================================
// Nested Includes
//===========================================================================
#include <p3d/p3dtypes.hpp> // for tUID


namespace Scrooby
{
//===========================================================================
// Forward References
//===========================================================================
class Screen;

//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: This class encapsulates the callback function that gets
//              invoked when the asynchronous call to:
//              Scrooby::Project->GotoScreen() finally completes.
//
//              The client must derive a concrete class from this abstract
//              base class and implement OnGotoScreenComplete().
//
// Constraints: None
//
//===========================================================================
class GotoScreenCallback
{
    public:

        //IAN IMPROVE: inline functions are bad - remove them
        GotoScreenCallback() 
        { 
            mScreen = NULL; 
        };

        //
        // This member will be invoked when the screen has completed loading
        // from disk.  
        //
        // The client is responsible for implementing this member function.
        //
        virtual void OnGotoScreenComplete() = 0;

        void SetScreen( Screen* pScreen ) 
        { 
            mScreen = pScreen; 
        };
        Screen* GetScreen() 
        { 
            return mScreen; 
        };

    private:
        Screen* mScreen;
};


//===========================================================================
//
// Description: This class is used to represent a front-end project 
//
// Constraints: None
//
//===========================================================================

class Project
{
    public:

        // Gets the current screen
        //
        virtual Screen* GetCurrentScreen() = 0;

        // Gets a screen from within a project by hashed name.
        //
        virtual Screen* GetScreen( const tUID hashName ) = 0;

        // Change to the specified screen by hashed name.
        //
        virtual void GotoScreen
        ( 
            const tUID hashName, 
            GotoScreenCallback* pCallback 
        ) = 0; 

        virtual void GotoScreen( Screen* screen, 
                                 GotoScreenCallback* pCallback ) = 0; 
        
           // Gets a screen from within a project by name.
        // Note: This version should only be used for debugging.
        //
        virtual Screen* GetScreen( const char* name ) = 0;

        // Change to the specified screen by name.
        // Note: This version should only be used for debugging.
        //
        virtual void GotoScreen( const char* name, 
                                 GotoScreenCallback* pCallback ) = 0; 

        // Retrieve the number of screens in this project.
        //
        virtual int GetScreenCount( void ) = 0;

        // Change to the specified screen by index. (Only the test viewer
        // actually needs this.
        //
        virtual void GotoScreenByIndex( int screenIndex, 
                                        GotoScreenCallback* pCallback ) = 0; 

        virtual void LoadScreen( Screen* screen, GotoScreenCallback* pCallback ) = 0;

        virtual void UnLoadScreen( Screen* screen ) = 0;

        virtual bool IsScreenLoaded( Screen* screen ) = 0;

        virtual bool IsLoadingScreens() const = 0;

    protected:
    private:
};


} // End namespace

#endif // End conditional inclusion
