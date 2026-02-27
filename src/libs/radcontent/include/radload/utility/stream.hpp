/** \file stream.hpp
  *
  * Implementations of streams for loading
  *
  * \author Mike Perzel
  * \attention Copyright (C) 2002 Radical Entertainment Ltd.  All Rights Reserved.
  */

#ifndef RADLOADSTREAM_HPP
#define RADLOADSTREAM_HPP

#include <radload/loader.hpp>

struct IRadFile;
struct IRadThreadSemaphore;

/// Implements the size and position calls on top of the base stream
class radLoadBaseStream : public radLoadStream
{
public:
    /// \param size The length of the stream (in bytes)
    /// \param request A request that can be updated by the stream 
    radLoadBaseStream( unsigned int size, radLoadUpdatableRequest* request );
    virtual bool Read( void* data, unsigned int count, unsigned int dataSize ) = 0;
    virtual unsigned int GetSize() { return m_size; }
    virtual unsigned int GetPosition() { return m_position; }

protected:

    unsigned int m_size;
    unsigned int m_position;
};

/// A stream constructed from a block of memory instead of a file.
class radLoadDataStream : public radLoadBaseStream
{
public:
    /// \param buffer Memory address which will be treated as a stream
    /// \param size Length of the memory buffer
    /// \param del Will delete the buffer at the streams destruction if true
    /// \param request to report the state of the stream
    radLoadDataStream( unsigned char* buffer, unsigned int size, bool del = false, radLoadUpdatableRequest* request = NULL );

    virtual bool Read( void* data, unsigned int count, unsigned int dataSize );
    /// \returns the address of the memory buffer being used as a stream
    unsigned char* GetMemory();

protected:
    virtual ~radLoadDataStream();

    unsigned char* m_pBuffer;
    bool m_bDelete;
};

/// Standard file stream.
class radLoadFileStream : public radLoadBaseStream
{
public:
    /// \param filename constructs a stream from this file
    /// \param request to report the state of the stream
    radLoadFileStream( const char* filename, radLoadUpdatableRequest* request = NULL );
    
    virtual bool Read( void* data, unsigned int count, unsigned int dataSize );

    //@{
    /// Controls the synchronous state.  If true, then the file will
    /// load synchronously.  Otherwise, it will load asynchronously
    void SetSyncLoading( bool sync ) { m_bSync = sync; }
    bool IsSyncLoading() { return m_bSync; } 
    //@}

    void SetYieldMillis( float yieldMillis ) { m_yieldMillis = yieldMillis; }
    float GetYieldMillis() { return m_yieldMillis; }
    void SetYieldSize( unsigned int yieldSize ) { m_yieldSize = yieldSize; }
    unsigned int GetYieldSize() { return m_yieldSize; }

    //@{
    /// \overload
    virtual unsigned int GetSize();
    virtual unsigned int GetPosition(); 
    //@}

    class Callback;
    
    friend class Callback;

protected:
    virtual ~radLoadFileStream();
    
    void WaitForCompletion();

    bool m_bDone;
    bool m_bSync;
    float m_yieldMillis;
    unsigned int m_yieldSize;

    IRadFile* m_pFile;
    Callback* m_pCallback;
};

/// Similar to a file stream, but uses the options object for it's load parameters 
class radLoadLoaderStream : public radLoadFileStream
{
public:
    /// \param options Load the stream using the supplied options
    /// \param request to report the state of the stream
    radLoadLoaderStream( radLoadOptions* options, radLoadUpdatableRequest* request );

protected:
    virtual ~radLoadLoaderStream();
};

#endif

