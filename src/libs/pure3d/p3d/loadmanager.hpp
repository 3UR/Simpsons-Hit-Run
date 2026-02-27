//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _LOADMANAGER_HPP_
#define _LOADMANAGER_HPP_

#include <p3d/refcounted.hpp>
#include <p3d/entity.hpp>
#include <radload/loader.hpp>
#include <p3d/buildconfig.hpp>

class tFile;
class tEntity;
class tEntityStore;
class tChunkFile;

struct IRadFileCompletionCallback;

#ifndef WORLD_BUILDER
enum tLoadStatus { LOAD_OK, LOAD_ERROR };
#endif

class tFileHandler : public radLoadFileLoader
{
public :
    tFileHandler() { }

    virtual tLoadStatus Load(tFile* file, tEntityStore* store) = 0;
    virtual void LoadFile(radLoadOptions* options, radLoadUpdatableRequest* request);
    
    virtual bool CheckExtension(char*) = 0;

protected:
    ~tFileHandler() { }
};

class tChunkHandler : public radLoadDataLoader
{
public:
    tChunkHandler() {}

    virtual tLoadStatus Load(tChunkFile* file, tEntityStore* store) = 0;
    virtual IRefCount* LoadData(radLoadStream* stream,
                            radMemoryAllocator alloc,
                            radLoadInventory* resolver = NULL,
                            IRefCount* originalObject = NULL );
    virtual bool CheckChunkID(unsigned) = 0;
    virtual unsigned int GetChunkID() = 0;
    virtual void SetNameOverride( const char* Name ) {} // Reset to null after Load.

#ifdef P3D_TRACK_LOAD_STATS
    static const char *ChunkTypeName(unsigned id);
#endif
protected:
    ~tChunkHandler() {}
};

class tSimpleChunkHandler : public tChunkHandler
{
public:
    tSimpleChunkHandler(unsigned id) : _id( id ), m_NameOverride( 0 ) {}

    tLoadStatus Load(tChunkFile* file, tEntityStore* store);
    bool CheckChunkID(unsigned);
    unsigned GetChunkID() { return _id; }
    void SetNameOverride( const char* Name ) { m_NameOverride = Name; } // Reset to null after Load.

protected:
    ~tSimpleChunkHandler() {} 
    virtual void HandleCollision(tEntity*);
    virtual tEntity* LoadObject(tChunkFile* file, tEntityStore* store) = 0;
    unsigned _id;
    tLoadStatus status;
    const char* m_NameOverride;  // This name will be used for the loaded entity
};


class tP3DFileHandler : public tFileHandler
{
public:
    tP3DFileHandler(int nLoad = 64);
    
    bool CheckExtension(char*);
    tLoadStatus Load(tFile* file, tEntityStore* store);

    tChunkHandler* AddHandler(tChunkHandler*);
    tChunkHandler* AddHandler(tChunkHandler*, unsigned chunkID);
    tChunkHandler* GetHandler(unsigned chunkID);
    void RemoveHandler(unsigned chunkID);
    void RemoveAllHandlers(void);

    void AddAlternateExtension(char*);

protected:
    ~tP3DFileHandler();

    int nExtensions;
    char extensions[8][8];

#ifdef P3D_TRACK_LOAD_STATS
    unsigned mTotalLoadTime;
    unsigned mTotalTaskedOutTime;
#endif
};

// a single requested load
class tLoadRequest : public tRefCountedTemp
{
public:
    // callback that is triggered when load is complete
    class Callback : public radLoadCallback
    {
    public:
        Callback() : request(NULL) {}

        virtual void AddRef() = 0; 
        virtual void Release() = 0; 
        virtual void Done(tLoadStatus status, tLoadRequest *load) = 0;
        void Done()
        {
            Done( ((request==NULL) || (request->GetState() == COMPLETE)) ? LOAD_OK : LOAD_ERROR, request );
            tRefCounted::Release(request);
        }
    protected:
        tLoadRequest* request;
        friend class tLoadRequest;

    };

    class InternalCallback : public radLoadCallback
    {
    public:
        InternalCallback(tLoadRequest* r) : request(NULL) { tRefCounted::Assign(request, r); }
        void Done();
    protected:
        tLoadRequest* request;
    };

    // constuction interface, shouldn't be called after tLoadManager::Load is called
    tLoadRequest( const char* filename);
    tLoadRequest(tFile* file);

    const char* GetFilename(void);
    float GetPortionLoaded(void);


    void SetCallback(Callback*);            // assign a calback to be triggered when the load is complete

    void SetAsync(bool);                    // request that the load be completed asynchronously

    void SetMemoryAllocator(int);           // set a memory allocator to be used for data created byt this load

    void SetInventorySection(const char*);  // set an inventory section that the loaded data should be put in
    void SetInventorySection(tUID);         //      note : these functions won't have any effect if SetStore has been called    
                                            //      also note : loads go into the main inventory "default" section 
                                            //          (not the current section) unless this is set


    void SetStore(tEntityStore*);           // set an entity store to place loaded data in rather than the main inventory

    void SetPriority(int);                  // set the priority of the request (larger numbers are loaded first);

    void SetUserData(void*);                // set a arbitrary piece of data
    void* GetUserData(void) { return userData;}

    void SetDummy(bool);                    // if true request is not an actual load, but just used in queue for synchronisation

    // These functions can be called once loading has started
    
    radLoadState GetState(void);
    bool  IsDummy(void);  // true if reuest is not intended to represent an actual load (for syncronisation only)
    void  Complete(void); // block until this load request is completed
    void  Cancel(void);   

protected:
    ~tLoadRequest();
    friend class tLoadManager;
    friend class InternalCallback;

protected:
    radLoadOptions* options;
    radLoadRequest* request;
    tFile* file;

    Callback* callback;

    tUID section;
    tEntityStore* store;
    int priority;
    void* userData;
    bool dummy;
};

class tLoadManager : public tRefCounted
{
public:
    tLoadManager(int nHandlers = 64);

    // Request a load
    tLoadStatus Load(tLoadRequest* request);

    void CancelAll(void);
    void CompleteAll(void);

    void SwitchTask(void);
    void TriggerCallbacks(void);

    tFileHandler* AddHandler(tFileHandler*, const char* ext);
    tFileHandler* GetHandler(const char* ext);
    void RemoveHandler(char* ext);
    void RemoveAllHandlers(void);

    tP3DFileHandler* GetP3DHandler();

    tLoadRequest* GetCurrentRequest(void) {return NULL;}

#ifdef P3D_TRACK_LOAD_STATS
    // Returns number of MicroSeconds!
    static void ResetTaskedOutStats(void) { mTaskedOutTime = 0; mTaskedOutMemoryDelta = 0; mTaskedOutAllocationsDelta = 0; }

    static unsigned TaskedOutTime(void)  { return mTaskedOutTime; }
    static int TaskedOutMemoryDelta(void)      { return mTaskedOutMemoryDelta; }
    static int TaskedOutAllocationsDelta(void) { return mTaskedOutAllocationsDelta; }
    static void SetLoadActive(bool active)     { mLoadActive = active; }

#endif
protected:
    friend class tContext;

#ifdef P3D_TRACK_LOAD_STATS
    static unsigned mTaskedOutTime;
    static int      mTaskedOutMemoryDelta;
    static int      mTaskedOutAllocationsDelta;
    static void     GetMemoryStats(int *freeMemory, int *allocations);
    static bool     mLoadActive;
#endif
    ~tLoadManager();
};

#endif

