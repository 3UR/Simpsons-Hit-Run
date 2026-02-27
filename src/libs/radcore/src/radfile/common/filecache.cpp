//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        filecache.cpp
//
// Subsystem:	Radical File System
//
// Description:	This file contains the implementation of
//              the radFile object.
//
// Revisions:	
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include "filecache.hpp"

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    FileCache::FileCache
//=============================================================================
// Description: Construct a file cache object. No cache!
//
// Parameters: 
//
// Returns:     
//------------------------------------------------------------------------------

FileCache::FileCache( )
    :
    m_CacheState( FileCacheInvalid ),
    m_CacheBuffer( NULL ),
    m_CacheSize( 0 ),
    m_CacheSpace( radMemorySpace_Null ),
    m_CacheSectorSize( 0 )
{
}

//=============================================================================
// Function:    FileCache::~FileCache
//=============================================================================
// Description: Destroy a file cache object. We do nothing since we are given the memory.
//
// Parameters: 
//
// Returns:     
//------------------------------------------------------------------------------

FileCache::~FileCache( )
{
}

//=============================================================================
// Function:    FileCache::InitCache
//=============================================================================
// Description: Initialize the cache. Give the cache memory to work with, which should
//              be aligned, and a multiple of a sector.
//
// Parameters: 
//
// Returns:     
//------------------------------------------------------------------------------

void FileCache::InitCache( unsigned char* buffer, unsigned int size, radMemorySpace space )
{
    m_CacheState = FileCacheInvalid;
    m_CacheBuffer = buffer;
    m_CacheSize = size;
    m_CacheSpace = space;
}

//=============================================================================
// Function:    FileCache::DoInit
//=============================================================================
// Description: The second half of the initialize, this is called off the drive thread.
//              Sets the sector size of the drive, and checks if sizes make sense.
//
// Parameters: 
//
// Returns:     
//------------------------------------------------------------------------------

void FileCache::DoInit( unsigned int sectorSize, const char* fileName )
{
    if ( m_CacheBuffer == NULL )
    {
        return;
    }

    //
    // If the buffer is smaller than a sector, there no point in caching.
    //
    if ( m_CacheSize < sectorSize )
    {
        rDebugPrintf( "radFile: cache buffer for file [%s] must be at least size [%u].\n", fileName, sectorSize );
        rAssert( false );

        m_CacheBuffer = NULL;
        m_CacheSize = 0;
        m_CacheSpace = radMemorySpace_Null;
        return;
    }

    m_CacheSectorSize = sectorSize;

    //
    // Make the buffer a multiple of a sector.
    //
    unsigned int alignSize = ::radMemoryRoundDown( m_CacheSize, sectorSize );
    if ( m_CacheSize != alignSize )
    {
        rDebugPrintf( "radFile: WASTING MEMORY!\n" );
        rDebugPrintf( "File cache on file [%s] wastes [%u] bytes.\n", fileName, m_CacheSize - alignSize );
        rDebugPrintf( "Make cache buffers a multiple of the sector size [%u].\n", sectorSize );

        m_CacheSize = alignSize;
    }
}

//=============================================================================
// Function:    FileCache::GetCacheMemorySpace
//=============================================================================
// Description: Simply return the cache space.
//
// Parameters: 
//
// Returns:     
//------------------------------------------------------------------------------

radMemorySpace FileCache::GetCacheMemorySpace( void )
{
    return m_CacheSpace;
}

//=============================================================================
// Function:    FileCache::ShouldUseCache
//=============================================================================
// Description: Returns whether the cache should be used.
//
// Parameters:  position and size of the read.
//
// Returns:     true if it makes sense to use the cache.
//------------------------------------------------------------------------------

bool FileCache::ShouldUseCache( unsigned int position, unsigned int size )
{
    //
    // Can't use it if it's not there!
    //
    if ( m_CacheBuffer == NULL )
    {
        return false;
    }

    //
    // If the read is too big, then there's really no point to using the cache.
    //
    if ( position - ::radMemoryRoundDown( position, m_CacheSectorSize ) + size > m_CacheSize )
    {
        return false;
    }
    else
    {
        return true;
    }
}

//=============================================================================
// Function:    FileCache::Read
//=============================================================================
// Description: Do a read from the drive, possibly refreshing the cache if needed.
//
// Parameters:  same as drive::read except:
//              fileEndPosition -- the position of the first byte after the file
//              fileSize -- the size of the file
//
// Returns:     
//------------------------------------------------------------------------------

radDrive::CompletionStatus FileCache::Read
( 
    radDrive* pDrive,
    radFileHandle handle,
    unsigned int fileEndPosition,
    unsigned int fileSize,
    const char* fileName,
    IRadFile::BufferedReadState buffState,
    unsigned int position, 
    void* pData, 
    unsigned int bytesToRead, 
    unsigned int* bytesRead, 
    radMemorySpace pDataSpace
)
{
    rAssert( m_CacheBuffer != NULL );
    rAssert( m_CacheSpace != radMemorySpace_Null );
    rAssert( m_CacheSize >= m_CacheSectorSize );
    rAssert( position - ::radMemoryRoundDown( position, m_CacheSectorSize ) + bytesToRead <= m_CacheSize );
   
    *bytesRead = 0;
    unsigned char* pUserData = (unsigned char*) pData;

    //
    // If our cache is valid, check to see if we already have the data loaded.
    //
    if ( m_CacheState == FileCacheValid && position >= m_CachePosition )
    {
        if ( position + bytesToRead <= m_CachePosition + m_CacheSize )
        {
            //
            // The read is in our cache! Copy it.
            //
            IRadMemorySpaceCopyRequest* pMemCpyRequest = 
                radMemorySpaceCopyAsync( pUserData, 
                                         pDataSpace, 
                                         &m_CacheBuffer[ position - m_CachePosition ],
                                         m_CacheSpace,
                                         bytesToRead );

            rAssert( pMemCpyRequest != NULL );
            pMemCpyRequest->AddRef( );

            //
            // Run the request
            //
            while ( !pMemCpyRequest->IsDone( ) )
            {
                ::radThreadSleep( 0 );
            }

            pMemCpyRequest->Release( );
            pMemCpyRequest = NULL;

            *bytesRead += bytesToRead;

            //
            // DONE!!
            //
            return radDrive::Complete;
        }
        else if ( position < m_CachePosition + m_CacheSize )
        {
            //
            // Overlap the right of the buffer. Copy the first bytes.
            //
            unsigned int readSize = m_CachePosition + m_CacheSize - position;
            IRadMemorySpaceCopyRequest* pMemCpyRequest = 
                radMemorySpaceCopyAsync( pUserData, 
                                         pDataSpace, 
                                         &m_CacheBuffer[ position - m_CachePosition ],
                                         m_CacheSpace,
                                         readSize );

            rAssert( pMemCpyRequest != NULL );
            pMemCpyRequest->AddRef( );

            //
            // Run the request
            //
            while ( !pMemCpyRequest->IsDone( ) )
            {
                ::radThreadSleep( 0 );
            }

            pMemCpyRequest->Release( );
            pMemCpyRequest = NULL;

            *bytesRead += readSize;

            //
            // Update the variables, so that when we refresh the cache, it will finish off
            // this read.
            //
            bytesToRead -= readSize;
            position += readSize;
            pUserData += readSize;
        }
    }

    //
    // If the request was satisfied, then we returned. So if we get here, we need to
    // refresh our cache and copy the data over to the user.
    //
    m_CacheState = FileCacheInvalid;

    //
    // If the file is smaller than the cache, just put the whole thing in.
    // We must be certain not to read past the last sector if we have a big cache.
    //
    unsigned int readSize = m_CacheSize;
    if ( fileSize <= m_CacheSize )
    {
        m_CachePosition = 0;
        readSize = ::radMemoryRoundUp( fileSize, m_CacheSectorSize );
    }
    else
    {
        //
        // Start our cache at the first sector boundary before position, and align it to be
        // sector sized.
        //
        m_CachePosition = ::radMemoryRoundDown( position, m_CacheSectorSize );

        //
        // If we are at the end of the file, move the position back so that we cache up to
        // the end of the last sector.
        //
        if ( m_CachePosition + m_CacheSize > fileEndPosition )
        {
            m_CachePosition = ::radMemoryRoundUp( fileEndPosition, m_CacheSectorSize ) - m_CacheSize;
        }
    }

    //
    // Load in the data from the drive.
    //
    unsigned int cacheBytesRead;
    radDrive::CompletionStatus status =
            pDrive->ReadFile
            ( 
                handle,
                fileName,
                buffState,
                m_CachePosition,
                m_CacheBuffer,
                readSize,
                &cacheBytesRead,
                m_CacheSpace
            );

    if ( status != radDrive::Complete )
    {
        return status;
    }

    m_CacheState = FileCacheValid;

    //
    // Finally, copy the data over to the user.
    //
    IRadMemorySpaceCopyRequest* pMemCpyRequest = 
        radMemorySpaceCopyAsync( pUserData, 
                                 pDataSpace, 
                                 &m_CacheBuffer[ position - m_CachePosition ],
                                 m_CacheSpace,
                                 bytesToRead );

    rAssert( pMemCpyRequest != NULL );
    pMemCpyRequest->AddRef( );

    //
    // Run the request
    //
    while ( !pMemCpyRequest->IsDone( ) )
    {
        ::radThreadSleep( 0 );
    }

    pMemCpyRequest->Release( );
    pMemCpyRequest = NULL;

    *bytesRead += bytesToRead;

    return radDrive::Complete;
}

//=============================================================================
// Function:    FileCache::InvalidateCache
//=============================================================================
// Description: Tell the cache it is invalid for size bytes after position.
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void FileCache::InvalidateCache( unsigned int position, unsigned int size )
{
    if ( m_CacheBuffer == NULL )
    {
        return;
    }

    //
    // If this range overlaps with our cache, then invalidate it.
    //
    if ( 
        ( position >= m_CachePosition && position < m_CachePosition + m_CacheSize ) ||
        ( position + size > m_CachePosition && position + size <= m_CachePosition + m_CacheSize ) ||
        ( position < m_CachePosition && position + size > m_CachePosition + m_CacheSize )
       )
    {
        m_CacheState = FileCacheInvalid;
    }
}
