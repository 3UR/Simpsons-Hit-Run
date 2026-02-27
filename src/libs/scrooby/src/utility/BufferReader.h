//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Stream Reader 
//
// Description: This class does buffered reads of the specified file and
//              returns a single char to the client on request.
//             
//              It uses the file services from Foundation Tech to perform
//              asynchronous file access even though the interface appears
//              synchronous to its clients.
//
// Authors:     Darwin Chau
//              Eric Tong
//
//===========================================================================

// Recompilation protection flag.
#ifndef STREAMREADER_H
#define STREAMREADER_H

class StreamReader
{
    public:
        
        // Specify the file you want to read and the number of bytes
        // to be read at a time.  Obviously a smaller buffer uses less
        // memory but will require more file accesses.
        //
        StreamReader( const char* buffer, 
                      const unsigned int bufferSize = 0 );
        
        // Get the next character from the file.  Returns '\0' at
        // the end of the file.
        // 
        char GetNextChar( void );

    private:

        const char* mBuffer;
        unsigned int mBufferSize;
        
        // Number of times client has called GetNextChar().
        //
        unsigned int mBytesStreamed;
};        

/*
//===========================================================================
// Nested Includes
//===========================================================================
#include <bStore.hpp>

//===========================================================================
// Forward References
//===========================================================================
struct bIDispatcherHack;
struct bIReadAsync;


//===========================================================================
//
// Description: Use this to open a file and retrieve one char at a time.
//
// Constraints: None
//
//===========================================================================
class StreamReader : public bIObjectAvailableCallback,
                     public bIReadAsyncCallback
{
    public:
        
        // Specify the file you want to read and the number of bytes
        // to be read at a time.  Obviously a smaller buffer uses less
        // memory but will require more file accesses.
        //
        StreamReader( const char* filename, 
                      const unsigned int readBufferSize = 32768 );
        
        virtual ~StreamReader( );

        // Get the next character from the file.  Returns '\0' at
        // the end of the file.
        // 
        char GetNextChar( void );
        
        unsigned int GetFileSize( void ) { return( mFileSize ); };


        // Overrides for callbacks required by the Foundation file system.
        // Clients can ignore these.
        //
        void OnObjectAvailable( bIObject* pObject );
        void OnReadAsyncComplete( void );

        // Overrides required for COM.  Clients can igonre these.
        //
        void AddRef( void );
        void Release( void );
        void GetInterface( bInterfaceId interfaceId, void** ppInterface );


    private:
        
        // Used to read a chunk of the file into memory.
        //
        void GetNextBufferFromFile( void );
        
      
        // COM reference count for this object.
        //
        unsigned int mRefCount;

        // We run the dispatcher while waiting for the file to load.
        //
        bIDispatcherHack* mpDispatcherHack;      
        
        // Interface to read a file.
        //
        bIReadAsync* mpIReadAsync;

        // Flags to tell when the asynchronous file accesses are complete.
        //
        bool mFileReady;
        bool mReadComplete;

        const unsigned int mBufferSize;

        unsigned int mFileSize;
        
        // Number of times client has called GetNextChar().
        //
        unsigned int mBytesStreamed;
        
        unsigned int mIndex;
        char* mBuffer;
};
*/
#endif