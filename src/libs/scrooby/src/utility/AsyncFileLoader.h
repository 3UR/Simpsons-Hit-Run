//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Asychronous File Loader
//
// Description: This class asychronously loads the specified file into a
//              memory buffer.
//
// Authors:     Darwin Chau
//
//===========================================================================

// Recompilation protection flag.
#ifndef ASYNCFILELOADER_H
#define ASYNCFILELOADER_H


//===========================================================================
// Nested Includes
//===========================================================================
#include <radfile.hpp>
#include "memory.h"
#include <stdio.h>  // for NULL

//===========================================================================
// Forward References
//===========================================================================
class AsyncFileLoaderCallback;

//===========================================================================
//
// Description: Use this to asynchronously load a file that will be put in
//              a memory buffer.  This class will allocate a buffer if one
//              is not provided.  In either case, the client is responsible
//              for deleting the buffer.
//
// Constraints: None
//
//===========================================================================
class AsyncFileLoader :  public IRadFileCompletionCallback
{
    public:
        
        AsyncFileLoader();
        virtual ~AsyncFileLoader();

        //fixes the forward/back slashes in the filename
        static void FixSlashes( char* filename );

        //passing true to this function will turn all the slashes into backslashes
        //passing false to this funciton will turn all the slashes into forward slashes
        static void ForceBackSlashes( bool useBackSlashes );

        //===================================================================
        // Description: Specify the file you want to load and a pointer to
        //              your callback object that will be invoked once the
        //              file has finished loading.
        //
        //              Optionally, you can pass in a buffer to receive the
        //              data.  If the buffer is not provided, it will be
        //              allocated for you.
        //
        // Constraints: VERY IMPORTANT!  The client is responsible for
        //              deleting the buffer in either case.
        //===================================================================
        void LoadFile( const char* filename, 
                       AsyncFileLoaderCallback* pCallback,
                       char* buffer = NULL,
                       unsigned int bufferSize = 0,
                       unsigned int allocator = p3d::ALLOC_LOADED );

        // Overrides for callbacks required by the Foundation file system.
        // Clients can ignore these.
        //
        virtual void OnFileOperationsComplete( void* pUserData );

        // Overrides required for COM.  Clients can igonre these.
        //
        virtual void AddRef( void );
        virtual void Release( void );

        //Registers a cement file to be used in conjunciton with the file system
        void RegisterCementFile( const char* cementFileName );


    protected:

        static bool g_UseBackSlashes;   //'\' - back slash
                                        //'/' - forward slash

        static bool ValidFilename( const char* filename );

        unsigned int mRefCount;

        unsigned int mFileSize;
        char* mBuffer;
        unsigned int mBufferSize;
        IRadFile*   m_pIRadFile;
//        static IRadDrive* m_pDrive;

        AsyncFileLoaderCallback* mCallback;

        bool mLoadPending;
};


//===========================================================================
//
// Description: This class encapsulates the callback function that gets
//              invoked when the asynchronous call to AsyncFileLoader::LoadFile()
//              finally completes.
//
//              The client must derive a concrete class from this abstract
//              base class and implement OnFileLoadComplete().
//
// Constraints: None
//
//===========================================================================
class AsyncFileLoaderCallback
{
    public:
        
        AsyncFileLoaderCallback()
        {
        }

        //===================================================================
        // Description: This member is invoked when the file has completed
        //              loading from disk.  The client is responsible for
        //              implementing this member function.
        //
        // Constraints: VERY IMPORTANT!  The client is responsible for
        //              deleting the fileDataBuffer when it's finsihed with
        //              it!
        //===================================================================
        virtual void OnFileLoadComplete( const unsigned int fileSize,
                                         char* fileDataBuffer ) = 0;
};


#endif
