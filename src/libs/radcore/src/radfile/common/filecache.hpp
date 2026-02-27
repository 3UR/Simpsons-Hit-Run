//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        filecache.hpp
//
// Subsystem:	Radical File System
//
// Description:	This file contains all definitions and classes relevant to
//              caching a file in memory.
//
// Revisions:	
//
//=============================================================================

#ifndef	FILECACHE_HPP
#define FILECACHE_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radmemory.hpp>
#include "drive.hpp"

//=============================================================================
// Class Declarations
//=============================================================================

class FileCache 
{   
public:

    //
    // Buffer/size should be aligned properly for optimal reads.
    //
    FileCache( );
    ~FileCache( );

    //
    // Set up the cache on the creation thread.
    //
    void InitCache( unsigned char* buffer, unsigned int size, radMemorySpace space );

    //
    // Return the memory space.
    //
    radMemorySpace GetCacheMemorySpace( void );
    
    //
    // Set up the cache on the drive thread.
    //
    void DoInit( unsigned int sectorSize, const char* fileName );

    //
    // Check if we should use the cache.
    //
    bool ShouldUseCache( unsigned int position, unsigned int size );

    //
    // Do a read from the cache. It will refresh itself as needed.
    //
    radDrive::CompletionStatus Read( radDrive* drive,
                                     radFileHandle handle, 
                                     unsigned int fileEndPosition,
                                     unsigned int fileSize,
                                     const char* fileName,
                                     IRadFile::BufferedReadState state,
                                     unsigned int position, 
                                     void* pData, 
                                     unsigned int bytesToRead, 
                                     unsigned int* bytesRead, 
                                     radMemorySpace pDataSpace );

    //
    // Invalidate the cache at some address.
    //
    void InvalidateCache( unsigned int position, unsigned int size );

private:
    enum
    {
        FileCacheInvalid,
        FileCacheValid
    }   m_CacheState;

    unsigned char*      m_CacheBuffer;

    unsigned int        m_CacheSize;
    unsigned int        m_CachePosition;

    radMemorySpace      m_CacheSpace;

    unsigned int        m_CacheSectorSize;
};

#endif 
