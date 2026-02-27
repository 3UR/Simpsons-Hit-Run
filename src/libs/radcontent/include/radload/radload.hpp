/** \file radload.hpp
  *
  * Interfaces for loading files, and retrieving the information in them.
  *
  * \author Mike Perzel
  * \attention Copyright (C) 2002 Radical Entertainment Ltd.  All Rights Reserved.
  */

#ifndef RADLOAD_HPP
#define RADLOAD_HPP

#include <radkey.hpp>
#include "radload/utility/object.hpp"

// Typedefs
enum radLoadState{ CREATED, QUEUED, LOADING, COMPLETE, CANCELED };
/// A unique identifier for each class type.  Used to associate a class with
/// a particular radLoadDataLoader
typedef unsigned int radLoadClassID;

// Forward References
struct radLoadInit;
class ILoadManager;
class radLoadManagerWrapper;
class radLoadRequest;
class radLoadCallback;
class radLoadInventory;
class radLoadStore;
struct radLoadOptions;
class radLoadStream;

class radLoadFileLoader;
class radLoadDataLoader;

/// Initialize radload.  This should be called once only per application, and
/// relies on all of the libraries it uses being alreadly initialized.
/// Currently, that is radfile, radmemory, radthread, radtime, and optionally
/// raddebugcommunications, radmemorymonitor, and raddebugwatcher.
/// \param init passes in user selectable data.  If NULL, it uses it's defaults
void radLoadInitialize( radLoadInit* init = NULL );
/// Cleans up all of the radload resources, and deletes the radload manager.
/// This must be called before radfile, radmemory, radthread, and radtime are
/// terminated.
void radLoadTerminate();
/// Called at regular intervals to allow updating
void radLoadService();
/// Retrieve an instance of the load manager.  For example:
/// \code
///     radLoadInstance()->Load( options, request );
/// \endcode
/// \sa radLoadManagerWrapper
ILoadManager* radLoadInstance();

///@{ Find an object from an inventory, filtering by type base
template <class base> base* radLoadFind( radLoadInventory* inventory, const radKey id );
template <class base> base* radLoadFind( radLoadInventory* inventory, const char* name ); //@}

/**
  * Central load manager.
  * A Singleton instance which all load requests are put through.  Loaders must
  * be registered with the ILoadManager instance before they can be used
  */
class ILoadManager
{
public:
    /// Loads using the information from the options parameter.
    /// The request object will be immediately constructed, and can be used to
    /// track the progress of the load.
    /// \sa radLoadOptions \sa radLoadRequest 
    virtual void Load( radLoadOptions* options, radLoadRequest** request ) = 0;
    /// Loads the filename specified.
    /// A radLoadOptions object will be constructed using all of the default
    /// values, and then loaded using that.
    /// The request object will be immediately constructed, and can be used to
    /// track the progress of the load.
    /// \sa radLoadOptions \sa radLoadRequest \sa #Load(radLoadOptions*,radLoadRequest**) 
    virtual void Load( const char* filename, radLoadRequest** request ) = 0;

    /// Sets the loadmanager into synchronous mode.  All load requests will be
    /// handled synchronously while this flag is true.  This flag should only
    /// be used for debugging purposes, since synchronous loading has many side
    /// effects when used improperly.  Individual requests can still be handled 
    /// synchronously using the syncLoad option in the radLoadOptions Object. 
    /// \sa radLoadOptions 
    virtual void SetSyncLoading( bool sync ) = 0;
    /// \returns true if the global sync loading flag is set.  false otherwise 
    virtual bool IsSyncLoading() = 0;

    /// Registers a radLoadFileLoader to handle a specific file extension.  A file loader
    /// is allowed to handle more than one extension, but there can only be one file loader
    /// per file extension 
    virtual void AddFileLoader( radLoadFileLoader* fileLoader, const char* extension ) = 0;
    /// Registers a radLoadDataLoader to handle a specific radLoadClassID.  Data loaders and
    /// Class ID's should have a one-to-one correspondance 
    virtual void AddDataLoader( radLoadDataLoader* dataLoader, radLoadClassID id ) = 0;

    /// @{Retrieve the loader for a particular identifier.  This should normally be left
    /// up to the Load call to automatically decide.
    /// \returns the loader if it is found, NULL otherwise 
    virtual radLoadFileLoader* GetFileLoader( const char* extension ) = 0;
    virtual radLoadDataLoader* GetDataLoader( radLoadClassID id ) = 0; //@}

    ///@{ Remove all loaders assigned to handle the specified identifier (if any) 
    virtual void RemoveFileLoader( const char* extention ) = 0;
    virtual void RemoveDataLoader( radLoadClassID id ) = 0; //@}
    ///@{ Remove the specified loader, if found. 
    virtual void RemoveFileLoader( radLoadFileLoader* loader ) = 0;
    virtual void RemoveDataLoader( radLoadDataLoader* loader ) = 0;//@}

    /// \returns false if the load queue is empty, true otherwise 
    virtual bool IsLoadPending() = 0;
    /// \returns the percentage of the currenly queued requests that are completed. 
    virtual float PercentDone() = 0;

    /// Allows updating of the load status. \sa ::radLoadService(). 
    virtual void Service() = 0;

    /// Cancels all pending load requests, then cleans up and deletes itself.
    /// \sa ::radLoadTerminate() 
    virtual void Terminate() = 0;

    /// Adds a callback to the load queue.  Callbacks are triggered when all
    /// of the load requests prior to the callback have been completed. 
    virtual void AddCallback( radLoadCallback* callback ) = 0;

    /// Cancel all pending load requests.  Callbacks will not be issued. 
    virtual void Cancel() = 0;

    /// Prints out the stats gathered during runtime for debugging and diagnostics.
    /// Requires RADLOAD_GATHER_STATS to be defined. \sa radloadconfig.hpp 
    virtual void PrintStats() = 0;

    /// Switches between the main thread and the loading thread.  This should
    /// never be triggered directly.  Leave it up to radLoad and the Service call.
    virtual void SwitchTasks() = 0;

    // Allow certian classes access to the base instance pointer
    friend void radLoadInitialize( radLoadInit* init );
    friend void radLoadTerminate();
    friend ILoadManager* radLoadInstance();
    friend class radLoadManagerWrapper;

protected:
    static ILoadManager* s_instance;
private:
};

/**
  * Retrieves an instance of the LoadManager.
  * For example :
  * \code
  *     radLoad->Load( options, request ); 
  * \endcode
  * \sa ::radLoadInstance()
  */
class radLoadManagerWrapper
{
public:
    operator ILoadManager*() { return ILoadManager::s_instance; }
    ILoadManager* operator->() { return ILoadManager::s_instance; }
};

extern radLoadManagerWrapper radLoad;

/**
  * Initialization structure.  Contains the parameters that are passed to 
  * the Initialize function of the ILoadManager.
  */
struct radLoadInit
{
    // Constructor to fill in the default variables
    radLoadInit() : fileLoaderListSize( 64 ), dataLoaderListSize( 256 ), loadQueueSize( 32 ), loadThreadStackSize( 65536 ) {};

    ///@{ Sets the loader list size limit.
    /// \attention Must be a power of two.
    int fileLoaderListSize;
    int dataLoaderListSize; //@}
    /// Max number of pending load requests.
    int loadQueueSize;
    /// Sets the stack size that the load thread will have.
    int loadThreadStackSize;
};

/** 
  * Specifies the options for a particular load call.  Either the filename
  * or the stream must be specified.
  */
struct radLoadOptions
{
    // Constructor, which will assign default values
    radLoadOptions(): filename(NULL), stream(NULL), resolverInventory(NULL), allocator(RADLOAD_ALLOC_DEFAULT), syncLoad(false), yieldMillis(0.0f), yieldSize(0) {} 
    /// The file name to be loaded. This string will not be deleted. 
    char* filename;
    /// load the file from a pre-existing stream instead of a filename.
    /// \attention This option exists for compatibility with the old pure3d
    /// system only, and should not be used in any new implementations. 
    radLoadStream* stream;
    /// The inventory which should be used for resolving external references. 
    /// Specifies the inventory that will be used when looking up dependencies 
    /// to other objects at load time.  Dependencies will first be checked
    /// against objects in the same file.  If this is NULL, then dependencies
    /// will be checked in the current file only 
    radLoadInventory* resolverInventory;
    /// The allocator for any allocations associated with this request. 
    radMemoryAllocator allocator;
    /// Issues this load synchronously.  Any loads queued previous to this one
    /// will also be loaded synchronously immediately.
    /// \attention This can be a dangerous option, and should only be used when
    /// sure that it will not interfere with other systems.
    /// \attention Any systems that need to be serviced regularly may will have
    /// unpredictable results due to synchronous loads 
    bool syncLoad;
    /// Currently not being used, since the loading system is pre-emptive 
    float yieldMillis;
    /// Currently not being used, since the loading system is pre-emptive 
    unsigned int yieldSize;
};

/**
  * Used for tracking the progress of a load.
  * This object can also be used as a handle to the objects loaded from a 
  * file, and then used to Release all of the objects from the inventory
  * by releasing the request.
  */
class radLoadRequest : public radLoadObject
{
public:
    radLoadRequest();

    /// \returns the percentage of the file which has been loaded 
    virtual float GetPercentDone();
    /// \returns true if the load has finished, false otherwise 
    bool IsComplete();
    /// Try and stop the request from loading, and then continue to the next request 
    void Cancel();
    /// \retunrs the current state of the load. \sa radLoadState 
    radLoadState GetState();
    /// \returns the inventory with the loaded objects if the load is complete, NULL otherwise 
    radLoadInventory* GetInventory();
    /// The total amount of time the request took from being asked to
    /// begin loading, to it's completion (in Milliseconds) 
    unsigned int GetTotalLoadTime();
    /// The time from when the request was queued to when it began loading
    /// (in Milliseconds) 
    unsigned int GetTotalQueuedTime();
    
protected:
    ~radLoadRequest();

    unsigned int m_tQueued;
    unsigned int m_tLoadStart;
    unsigned int m_tLoadFinish;

    radLoadState m_state;
    radLoadInventory* m_pInventory;
};

/**
  * Base class for Callbacks in the loading system.  A callback is
  * triggered when all of the loads which were queued before it have been
  * completed.
  * \sa ILoadManager::AddCallbac(radLoadCallback*)
  */
class radLoadCallback : public radLoadObject
{
public:
    /// Called when the callback is triggered.  Needs to be implemented in
    /// the derrived callback
    virtual void Done() = 0; 
};

/**
  * A collection of objects which can be searched.  The inventory contains
  * objects, as well as keys to locate those objects.  The find methods
  * allow for searching by key, as well as filtering by class type.
  */
class radLoadInventory : public radLoadObject
{
public:
    /**
      * necessary base class for templatized SafeCast
      */
    class SafeCastBase
    {
    public:
        virtual IRefCount* safe_cast( IRefCount* obj ) = 0;
    };
    /**
      * Implements a templatized dynamic cast
      */
    template <class T> class SafeCast : public SafeCastBase
    {
    public:
        /// Does a cast to the class type that the object was created with
        virtual IRefCount* safe_cast( IRefCount* obj )
        {
            T* object = dynamic_cast<T*>(obj);
            if( !object )
            {
                return NULL;
            }
            return static_cast<IRefCount*>(object);
        }
    };
  
    /// There isn't any point to calling this function directly, since the global
    /// functions are much nicer to use.
    virtual IRefCount* Find( SafeCastBase& caster, const radKey id ) = 0;    
};

// Have to implement these global functions here, because they're templatized
template <class base> base* radLoadFind( radLoadInventory* inventory, const radKey id )
{
    radLoadInventory::SafeCast<base> sc;
    return static_cast<base*>(inventory->Find( sc, id ));
}
template <class base> base* radLoadFind( radLoadInventory* inventory, const char* name )
{
    return radLoadFind<base>( inventory, radMakeKey( name ) );
}



#endif

