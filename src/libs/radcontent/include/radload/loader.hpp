/** \file loader.hpp
  *
  * Interfaces for building file and data loaders.
  *
  * \author Mike Perzel
  * \attention Copyright (C) 2002 Radical Entertainment Ltd.  All Rights Reserved
  */

#ifndef RADLOAD_LOADER_HPP
#define RADLOAD_LOADER_HPP

#include "radload/utility/object.hpp"
#include "radload/radload.hpp"

// Forward Declarations
class radLoadStream;
class radLoadUpdatableRequest;

///@{ platform and type independent data formats.  A word is two bytes.
typedef unsigned char radLoadByte;
typedef unsigned short radLoadWord;
typedef unsigned int radLoadDWord;
#if defined(RAD_WIN32) || defined(RAD_UWP)
typedef uint64_t radLoadQWord;
#else
typedef unsigned long long radLoadQWord;
#endif //@}

// Glboal functions
/// Creates a stream from the parameters passed to a radLoadFileLoader.
void radLoadCreateLoaderStream( radLoadStream** stream, radLoadOptions* options, radLoadUpdatableRequest* request = NULL );
/// Create an radLoadStream object from a file on disk
void radLoadCreateFileStream( radLoadStream** stream, const char* filename, radLoadUpdatableRequest* request = NULL );
/// Create an radLoadStream object from a chunk of memory
void radLoadCreateDataStream( radLoadStream** stream, const unsigned char* buffer, unsigned int size, radLoadUpdatableRequest* reqeust = NULL );

///@{ Retrieve specific data sizes from a stream
radLoadByte radLoadStreamGetByte( radLoadStream* stream );
radLoadWord radLoadStreamGetWord( radLoadStream* stream );
radLoadDWord radLoadStreamGetDWord( radLoadStream* stream );
radLoadQWord radLoadStreamGetQWord( radLoadStream* stream ); //@}

/**
  * A radLoadRequest object which gives the rights to update it's contents. 
  * This is so the radLoadFileLoader can update the request.
  */
class radLoadUpdatableRequest : public radLoadRequest
{
public:
    radLoadUpdatableRequest();

    /// Set the inventory object for the request.  This should be called at the
    /// end of the load with a fully populated inventory
    void SetInventory( radLoadInventory* inventory );
    /// Allows the hookup of a stream to the request to allow interaction
    /// between them to update items like the percentage done.
    void SetStream( radLoadStream* stream );
    /// Allows the setting of the state.  This will normally be done by the
    /// loadmanager.
    void SetState( radLoadState state );
    /// Allow retrieval of the current stream
    radLoadStream* GetStream();
    /// Overload to work with the included stream
    float GetPercentDone();

protected:
    ~radLoadUpdatableRequest();
    
    radLoadStream* m_pStream;
};

/**
  * Base class for all file loaders.  A file loader is a loader that handles all
  * files of a particular extension.  Usually, these file loaders will then pass
  * control on to a data loader.  File loaders must be registered with the load
  * manager in order to be useful.
  */
class radLoadFileLoader : public radLoadObject
{
public:
    /// Load the file, using the information in the load options.  The request
    /// object should also be updated as necessary during the load.
    /// \attention This call should only ever be made by the load manager
    virtual void LoadFile( radLoadOptions* options, radLoadUpdatableRequest* request ) = 0;
};

/**
  * Data loader interface.  These are for loading a particular object type,
  * which is specified by a ClassID.  Data loaders must be registered with the
  * load manager in order to be useful.
  */
class radLoadDataLoader : public radLoadObject
{
public:
    /// Creates a new object in a particular memory pool from a data
    /// stream.  Any references the object needs to resolve will be contained
    /// in the resolving inventory.
    /// 
    /// \note The original object reference is used for object reloading.  When an
    /// object attempts to reload itself, it will pass in it's current self.  If
    /// an object is capable of reloading, it should do so.  If it is not, then
    /// it should just return the original object
    virtual IRefCount* LoadData( radLoadStream* stream, 
            radMemoryAllocator alloc, 
            radLoadInventory* resolver = NULL, 
            IRefCount* originalObject = NULL ) = 0;
};


/**
  * Generic data stream.  These will be used to pass data to the object 
  * factories (data loaders).
  */
class radLoadStream : public radLoadObject
{
public:
    /// Data size definitions.  These can be used to pass nicer looking
    /// parameters to the Read call.
    enum
    {
        BYTE = 1,
        WORD = 2,
        DWORD = 4,
        QWORD = 8
    };

    radLoadStream() : m_endianSwap( false ) {}
    
    /// Grabs data from the stream.
    /// Reads the next [count] objects of size [dataSize] from the stream
    /// into the buffer [data]. The total data read will be count*dataSize.
    /// If data is NULL, then the stream will advance, but will not read the data.
    /// \returns true if it successfully read the data, false otherwise
    virtual bool Read( void* data, unsigned int count, unsigned int dataSize = BYTE ) = 0;
    /// \returns the size of the stream
    virtual unsigned int GetSize() = 0;
    /// \returns the current offset of the data pointer from the beginning of the stream
    virtual unsigned int GetPosition() = 0;

    virtual bool GetEndianSwap(void) { return m_endianSwap; }
    // returns prevoius state of the endian swap bool
    virtual bool SetEndianSwap(bool swap) { bool orig = m_endianSwap; m_endianSwap = swap; return orig; }
    
protected:
    void EndianSwap( void* data, unsigned int count, unsigned int dataSize );

    bool m_endianSwap;

};

// end conditional define
#endif

