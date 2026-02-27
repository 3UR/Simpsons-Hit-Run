//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef RADLOADMANAGER_HPP
#define RADLOADMANAGER_HPP

#include <radload/radload.hpp>
#include <radload/loader.hpp>
#include <radthread.hpp>

template <class T> class RefHashTable;
template <class T> class RefQueue;

class radLoadManager : public ILoadManager
{
public:
    radLoadManager( radLoadInit& init );
    
    void Load( radLoadOptions* options, radLoadRequest** request );
    void Load( const char* filename, radLoadRequest** request );

    void SetSyncLoading( bool sync );
    bool IsSyncLoading();

    void AddFileLoader( radLoadFileLoader* fileLoader, const char* extension );
    void AddDataLoader( radLoadDataLoader* dataLoader, radLoadClassID id );

    radLoadFileLoader* GetFileLoader( const char* extension );
    radLoadDataLoader* GetDataLoader( radLoadClassID id );

    void RemoveFileLoader( const char* extension );
    void RemoveDataLoader( radLoadClassID id );
    void RemoveFileLoader( radLoadFileLoader* loader );
    void RemoveDataLoader( radLoadDataLoader* loader );

    bool IsLoadPending();
    float PercentDone();
    
    void Service();
    void Terminate();

    void AddCallback( radLoadCallback* callback );
    
    void Cancel();

    void PrintStats();

    void SwitchTasks();

    class QueueItem : public radLoadUpdatableRequest
    {
    public:
        QueueItem( radLoadOptions& options ) : m_options( options ) {}
        radLoadOptions* GetOptions() { return &m_options; }
    protected:
        radLoadOptions m_options;
    };

    static unsigned int LoadThreadEntry( void* data );
    
protected:
    ~radLoadManager();

    void InternalService();

    bool m_bSyncLoading;
    bool m_bDone;

#ifdef RADLOAD_GATHER_STATS
    unsigned int m_totalLoads;
    unsigned int m_completedLoads;

    unsigned int m_pendingLoads;
    unsigned int m_maxPendingLoads;
    
    unsigned int m_minLoadTime;
    unsigned int m_maxLoadTime;
    unsigned int m_avgLoadTime;

    unsigned int m_minQueuedTime;
    unsigned int m_maxQueuedTime;
    unsigned int m_avgQueuedTime;
#endif

    QueueItem* m_pCurrent;

    RefHashTable<radLoadFileLoader>* m_pFileLoaders;
    RefHashTable<radLoadDataLoader>* m_pDataLoaders;

    RefQueue<radLoadObject>* m_pLoadQueue;
    RefQueue<radLoadCallback>* m_pCallbacks;
    
    IRadThread* m_pThread;
    IRadThreadSemaphore* m_pSemaphore;
    IRadThreadMutex* m_pMutex;
};

#endif

