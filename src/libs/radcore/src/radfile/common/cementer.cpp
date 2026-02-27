//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <string.h> 
#include <raddebug.hpp>
#include "file.hpp"
#include "drive.hpp"
#include "cementLibrary.hpp"
#include "cementer.hpp"

//=============================================================================
// Local enumerations
//=============================================================================

void KeyToStringKey32( char * pBuf, radKey32 key )
{
    sprintf( pBuf, "%d", key );
}

radKey32 StringKeyToKey32( char * pBuf )
{
    return atoi( pBuf );
}

//
// What are we to do?
//
enum EnumWhatToDo
{
    ReadFileInfo,
    ReadHeader,
    ReadHashEntries,
    Done
};

//=============================================================================
// Public Functions
//=============================================================================

//
// This is implemented as a state machine using callbacks. We could make things
// clearer by adding a request that does all the work on the thread. But since
// we don't yet have thread safe allocations, we can't do it. By using the 
// callback, we can guarantee that all of our allocations are done on the main
// thread of execution.
//

//=============================================================================
// Function:    radCementLibraryCreate
//=============================================================================
// Description: Creates a radical cement library and register it
//              with the appropriate drive
//
// Parameters:  pIRadCementLibrary -
//                  (out) the newly created cement library
//              pDrive - the drive associated with the cement library
//              cementLibraryFileName -
//                  the name of the cement library data file.
//              cacheSize -
//                  the size of the cache used by the cement libray data file
//              alloc - the memory allocator to use.
//
// Returns:
//------------------------------------------------------------------------------

void radCementLibraryCreate
(
    IRadCementLibrary** pIRadCementLibrary,
    radDrive* pDrive,
    const char* cementLibraryFileName,
    radCementLibraryPriority priority,
    unsigned int cacheSize,
    radMemoryAllocator alloc,
    radMemorySpace cacheSpace
)
{
    rAssert( pDrive != NULL );

    //
    // Open the data file for the cement library
    //
    radFile* pCementLibraryDataFile =
        new radFile
        (
            pDrive,
            cementLibraryFileName,
            false,
            OpenExisting,
            NormalPriority,
            cacheSize,
            alloc,
            cacheSpace
        );
    rAssert( pCementLibraryDataFile != NULL );

    //
    // Allocate the cement library.
    //
    radCementLibrary* pCementLibrary = NULL;
    pCementLibrary = new ( alloc ) radCementLibrary( pDrive, pCementLibraryDataFile, priority );
    rAssert( pCementLibrary != NULL );

    //
    // We no longer need the data file
    //
    pCementLibraryDataFile->Release( );

    //
    // Set up the library for returning to the user
    //
    *pIRadCementLibrary =
        static_cast< IRadCementLibrary* >( pCementLibrary );
}


//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// radCementLibrary implementation
//=============================================================================

//=============================================================================
// Function:    radCementLibrary::radCementLibrary
//=============================================================================
// Description: Constructor for the cement library.  This is done
//              asynchronously and assumes the data file is already pending
//              completion of a file open.
//
// Parameters:  pDrive      - a pointer to the drive object associated with
//                            this library.
//              pDataFile   - a pointer to the cement data file that this
//                            library will operate from
//------------------------------------------------------------------------------

radCementLibrary::radCementLibrary( radDrive* pDrive, radFile* pDataFile, radCementLibraryPriority priority )
    :
    radSafeRefCount( 1 ),
    m_pDrive( pDrive ),
    m_pDataFile( pDataFile ),
    m_CurrentStatus( CementLibraryPending ),
    m_Header( NULL ),
    m_pHashedFileEntries( NULL ),
    m_pNext( NULL ),
    m_Priority( priority ),
    m_pCallback( NULL ),
    m_pUserData( NULL ),
    m_TmpBuffer( NULL )
{
    //
    // Reference the drive object
    //
    rAssert( pDrive != NULL );
    m_pDrive->AddRef( );

    //
    // Reference the data file
    //
    rAssert( pDataFile != NULL );
    m_pDataFile->AddRef( );

    //
    // Set a callback for when the data file is done loading
    //
    m_pDataFile->AddCompletionCallback( this, (void*) ReadFileInfo );     
}

//=============================================================================
// Function:    radCementLibrary::~radCementLibrary
//=============================================================================
// Description: This routine is invoked to clean-up the cement library
//
// Parameters:
//
// Returns:
//------------------------------------------------------------------------------

radCementLibrary::~radCementLibrary( )
{
    //
    // If not fully registered, cancel now
    //
    rAssert( m_pDrive != NULL );
    if( m_CurrentStatus == radCementLibrary::CementLibraryPending )
    {
        m_pDataFile->CancelAsync( );
    }
    else if ( m_CurrentStatus == radCementLibrary::CementLibraryReady )
    {
        //
        // Remove ourself from the drive object
        //
        m_pDrive->UnregisterCementLibrary( this );
    }

    //
    // Release the data file
    //
    m_pDataFile->Release( );
    m_pDataFile = NULL;

    //
    // Release the drive object
    //
    m_pDrive->Release( );
    m_pDrive = NULL;

    //
    // Destroy allocated members it they are still around
    //
    if( m_TmpBuffer != NULL )
    {
        ::radMemoryFreeAligned( GetThisAllocator( ), m_TmpBuffer );
    }

    if( m_Header != NULL )
    {
        ::radMemoryFreeAligned( GetThisAllocator( ), m_Header );
    }
}

//=============================================================================
// Function:    radCementLibrary::Lock
//=============================================================================
// Description: Start a critical section
//
// Parameters:  
//              
// Returns:    
//
// Notes:       This library is a wrapper around a file, so just use the file's
//              mutex. 
//------------------------------------------------------------------------------

void radCementLibrary::Lock( void )
{
    m_pDataFile->Lock( );
}

//=============================================================================
// Function:    radCementLibrary::Unlock
//=============================================================================
// Description: End a critical section
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

void radCementLibrary::Unlock( void )
{
    m_pDataFile->Unlock( );
}

//=============================================================================
// Function:    radCementLibrary::GetStatus
//=============================================================================

IRadCementLibrary::CementLibraryStatus radCementLibrary::GetStatus( void )
{
    return m_CurrentStatus;
}

//=============================================================================
// Function:    radCementLibrary::WaitForCompletion
//=============================================================================
// Description: Spin until all pending activites are complete
//------------------------------------------------------------------------------

void radCementLibrary::WaitForCompletion( void )
{
    //
    // This service spins. Use with caution.
    //
    while( m_CurrentStatus == CementLibraryPending )
    {
        //
        // Service the file system.
        //    
        radFileService( );
        radThreadSleep(0);
    }
}

//=============================================================================
// Function:    radCementLibrary::IsOpen
//=============================================================================

bool radCementLibrary::IsOpen( void )
{
    return m_CurrentStatus == CementLibraryReady;
}

//=============================================================================
// Function:    radCementLibrary::SetCompletionCallback
//=============================================================================
// Description: Set the user's callback.
//
// Parameters:  
//
// Returns:
//------------------------------------------------------------------------------

void radCementLibrary::SetCompletionCallback( 
    IRadCementLibraryCompletionCallback* pCallback,
    void* pUserData 
)
{
    rAssertMsg( m_pCallback == NULL, "Only one callback can be registered." );

    m_pCallback = pCallback;
    m_pUserData = pUserData;
    m_pCallback->AddRef( );
}

//=============================================================================
// Function:    radCementLibrary::OnFileOperationsComplete
//=============================================================================
// Description: Used to control asynchronous access to the underlying cement
//              library data file.
//
// Parameters:  pUserData - an EnumWhatToDo explaining what we should do
//
// Returns:
//------------------------------------------------------------------------------

void radCementLibrary::OnFileOperationsComplete( void* pUserData )
{
    //
    // If we are not in a pending state, do not do anything!
    //
    if( m_CurrentStatus == CementLibraryReady )
    {
        rTuneString( "Cement library initialization terminated before becoming ready.\n" );
        rTunePrintf( "Cement file [%s] failed to load.\n", m_pDataFile->GetFilename( ) );
        CompleteRegister( CementLibraryError );
        return;
    }

    //
    // Get the action to perform
    //
    unsigned int whatToDo = ( uintptr_t ) pUserData;

    //
    // Check out what we should do
    //
    if( whatToDo == ReadFileInfo )
    {
        //
        // If the file is not open, the library is invalid
        //
        if( !m_pDataFile->IsOpen( ) )
        {
            rDebugPrintf( "Unable to open cement library data file [%s].\n", m_pDataFile->GetFilename( ) );
            CompleteRegister( CementLibraryError );
        }
        else
        {
            //
            // Allocate enough memory to read in either the file info or the header.
            //
            unsigned int readSize = ::radMemoryRoundUp( sizeof( radCFFileInfo ), m_pDataFile->GetOptimalSize( ) );
            unsigned int size = ::radMemoryRoundUp( sizeof( radCFHeader ), m_pDataFile->GetOptimalSize( ) );
            size = size > readSize ? size : readSize;
            m_TmpBuffer = (char*) ::radMemoryAllocAligned( RADMEMORY_ALLOC_TEMP, size, radFileOptimalMemoryAlignment );

            //
            // Seek to the file info, and read it.
            //
            m_pDataFile->SetPositionAsync( 0 );
            m_pDataFile->ReadAsync( m_TmpBuffer, readSize );

            m_pDataFile->AddCompletionCallback( this, (void*) ReadHeader );
        }
    }

    else if( whatToDo == ReadHeader )
    {
        if ( m_pDataFile->GetLastError( ) != Success )
        {
            rTunePrintf( "Cement file [%s] failed to load.\n", m_pDataFile->GetFilename( ) );
            return;
        }

        radCFFileInfo* pFileInfo = (radCFFileInfo*) m_TmpBuffer;

        //
        // First, make sure this is a valid cement library...
        //
        bool isLibraryOK = true;
        if(
            strcmp( pFileInfo->m_CementLibraryIdentification, RAD_CEMENT_LIBRARY_IDENTIFICATION ) != 0
          )
        {
            rTunePrintf( "Invalid library file [%s].\n", m_pDataFile->GetFilename( ) );
            isLibraryOK = false;
        }
        
        //
        // Check the library version, and its validity
        //
        if(
            pFileInfo->m_CementLibraryVersionMajor != RAD_CEMENT_LIBRARY_VERSION_MAJOR ||
            pFileInfo->m_CementLibraryVersionMinor != RAD_CEMENT_LIBRARY_VERSION_MINOR ||
            !pFileInfo->m_Valid
          )
        {
            rTunePrintf( "Unsupported library version in file [%s].\n", m_pDataFile->GetFilename( ) );
            isLibraryOK = false;
        }

        //
        // Check the endian
        //
        if ( pFileInfo->m_BigEndian )
        {
#       ifdef RAD_LITTLE_ENDIAN
            rTunePrintf( "Library [%s] is big endian, should be little endian.\n", m_pDataFile->GetFilename( ) );
            isLibraryOK = false;
#       endif
        }
        else
        {
#       ifdef RAD_BIG_ENDIAN
            rTunePrintf( "Library [%s] is little endian, should be big endian.\n", m_pDataFile->GetFilename( ) );
            isLibraryOK = false;
#       endif
        }

        //
        // If the library is OK, read the header. Use the tmp buffer since 
        // we don't need the file info.
        //
        if( isLibraryOK )
        {
            m_pDataFile->SetPositionAsync( pFileInfo->m_HeaderStartPos );
            m_pDataFile->ReadAsync( 
                (void*) m_TmpBuffer, 
                ::radMemoryRoundUp( sizeof( radCFHeader ), m_pDataFile->GetOptimalSize( ) ) );

            m_pDataFile->AddCompletionCallback( this, (void*) ReadHashEntries );
        }
        else
        {
            //
            // Set the status to error
            //
            CompleteRegister( CementLibraryError );
        }
    }

    else if( whatToDo == ReadHashEntries )
    {
        if( m_pDataFile->GetLastError( ) != Success )
        {
            rTunePrintf( "Cement file [%s] failed to load.\n", m_pDataFile->GetFilename( ) );
            CompleteRegister( CementLibraryError );
            return;
        }

        //
        // Allocate room for the hash table and the header. The hash table just follows the
        // header in memory.
        //
        unsigned int numFiles = ( ( radCFHeader* ) m_TmpBuffer )->m_NumFiles;
        unsigned int totalSize = 
            ::radMemoryRoundUp( sizeof( radCFHeader ) + sizeof( radCFHeader::HFE ) * numFiles, m_pDataFile->GetOptimalSize( ) );

        m_Header = ( radCFHeader* )
            ::radMemoryAllocAligned( GetThisAllocator( ), totalSize, radFileOptimalMemoryAlignment );
    
        //
        // Copy over the header info and beginning of the hash table we have read so far.
        //
        unsigned int bytesRead = 
            ::radMemoryRoundUp( sizeof( radCFHeader ), m_pDataFile->GetOptimalSize( ) );

        memcpy( m_Header, m_TmpBuffer, bytesRead );

        //
        // Set the internal pointer, and read in the rest.
        //
        char* headPtr = (char*) m_Header;
        m_pHashedFileEntries = ( radCFHeader::HFE* ) &headPtr[ sizeof( radCFHeader ) ];

        unsigned int bytesToRead = totalSize - bytesRead;
        if ( bytesToRead > 0 )
        {
            m_pDataFile->ReadAsync( &headPtr[ bytesRead ], bytesToRead );
        }

        //
        // One last callback to say we're done.
        //
        m_pDataFile->AddCompletionCallback( this, (void*) Done );

        //
        // We no longer need our temp buffer.
        //
        ::radMemoryFreeAligned( RADMEMORY_ALLOC_TEMP, m_TmpBuffer );
        m_TmpBuffer = NULL;
    
    }

    else if( whatToDo == Done )
    {
        //
        // Register this libary with the drive object.
        //
        m_pDrive->RegisterCementLibrary( this );

        //
        // We are done loading the library!
        //
        CompleteRegister( CementLibraryReady );
    }
    else
    {
        rAssertMsg( false, "Internal error" );
    }
}

//=============================================================================
// Function:    radCementLibrary::OpenFile
//=============================================================================
// Description: Open a file from the library. Should happen on a drive's thread.
//
// Parameters:
//
// Returns:     the file or NULL.
//------------------------------------------------------------------------------

radFile* radCementLibrary::OpenFile
(
    const char*     fileName,
    unsigned int*   pOffset,
    unsigned int*   pSize
)
{
    //
    // Verify that this library is ready for opening a file
    //
    if( m_CurrentStatus == radCementLibrary::CementLibraryReady )
    {
        //  
        // Find the given file in the hash table
        //
        radCFHeader::HFE* hashEntry =
            radCFHeader::FindFile( m_Header, m_pHashedFileEntries, radCFHeader::HashFunction( fileName ) );
            
        if ( hashEntry == NULL )
        {
            radKey32 key = StringKeyToKey32( (char*) fileName );
            
            hashEntry = radCFHeader::FindFile( m_Header, m_pHashedFileEntries, key );
        }

        if( hashEntry != NULL )
        {
            //
            // We found the file
            //
            *pOffset = hashEntry->m_FileOffset;
            *pSize   = hashEntry->m_FileSize;
            return m_pDataFile;
        }
        else
        {
            //
            // No file found
            //
            return NULL;
        }
    }
    else
    {
        rAssertMsg( false, "internal error" );
        return NULL;
    }
}

//=============================================================================
// Function:    radCementLibrary::CompleteRegister
//=============================================================================
// Description: Set the status and call the callback.
//
// Parameters:
//
// Returns:
//------------------------------------------------------------------------------

void radCementLibrary::CompleteRegister( IRadCementLibrary::CementLibraryStatus status )
{
    m_CurrentStatus = status;

    if ( m_pCallback != NULL )
    {
        m_pCallback->OnCementLibraryRegistered( m_pUserData );
        m_pCallback->Release( );
        m_pCallback = NULL;
        m_pUserData = NULL;
    }
}

//=============================================================================
// Function:    radCementLibrary::CompleteRegister
//=============================================================================

radCementLibraryPriority radCementLibrary::GetPriority( void ) const 
{
    return m_Priority;
}

//=============================================================================
// radCementLibraryList implementation
//=============================================================================

//=============================================================================
// Function:    radCementLibraryList::radCementLibraryList
//=============================================================================

radCementLibraryList::radCementLibraryList( void )
    : 
    m_pIter( NULL )
{
    for ( unsigned int i = 0; i < radCementLibraryNumPriorities; i++ )
    {
        m_pHead[ i ] = NULL;
        m_pTail[ i ] = NULL;
    }
}

//=============================================================================
// Function:    radCementLibraryList::~radCementLibraryList
//=============================================================================

radCementLibraryList::~radCementLibraryList( void )
{
    //
    // Remove everyone from the list, and yell at the user in debug mode.
    //
    for ( unsigned int i = 0; i < radCementLibraryNumPriorities; i++ )
    {
        if ( m_pHead[ i ] != NULL )
        {
            radCementLibrary* ptr = m_pHead[ i ];
            rDebugPrintf( "The following cement libraries have not been unregistered at priority [%u]: \n", i );
            while ( ptr != NULL )
            {
                rDebugPrintf( "\t%s\n", ptr->m_pDataFile->GetFilename( ) );
                radCementLibrary* dead = ptr;
                ptr = ptr->m_pNext;
                dead->Release( );
            }
            rAssert( false );
        }
    }
}

//=============================================================================
// Function:    radCementLibraryList::AddLibrary
//=============================================================================
// Description: Add a library to the back of the list.
//
// Parameters:
//
// Returns:
//------------------------------------------------------------------------------

void radCementLibraryList::AddLibrary( radCementLibrary* pCementLibrary )
{
    radCementLibraryPriority priority = pCementLibrary->GetPriority( );
    rAssert( priority < radCementLibraryNumPriorities );

    if ( m_pHead[ priority ] == NULL )
    {
        m_pHead[ priority ] = pCementLibrary;
    }
    else
    {
        m_pTail[ priority ]->m_pNext = pCementLibrary;
    }

    pCementLibrary->m_pNext = NULL;
    m_pTail[ priority ] = pCementLibrary;
}

//=============================================================================
// Function:    radCementLibraryList::RemoveLibrary
//=============================================================================
// Description: Remove the given library from the list.
//
// Parameters:
//
// Returns:
//------------------------------------------------------------------------------

void radCementLibraryList::RemoveLibrary( const radCementLibrary* pCementLibrary )
{
    radCementLibraryPriority priority = pCementLibrary->GetPriority( );
    rAssert( priority < radCementLibraryNumPriorities );

    radCementLibrary** ppLib = &m_pHead[ priority ];
    radCementLibrary* pEnd = NULL;

    while ( *ppLib != NULL )
    {
        if ( *ppLib == pCementLibrary )
        {
            //
            // Remove the element
            //
            radCementLibrary* pLib = *ppLib;
            *ppLib = pLib->m_pNext;
            break;
        }
        else
        {
            pEnd = *ppLib;
            ppLib = &( (*ppLib)->m_pNext );
        }
    }

    if ( *ppLib == NULL )
    {
        m_pTail[ priority ] = pEnd;
    }
}

//=============================================================================
// Function:    radCementLibraryList::BeginIteration
//=============================================================================
// Description: Start iterating from the front of the list.
//
// Parameters:
//
// Returns:
//------------------------------------------------------------------------------

void radCementLibraryList::BeginIteration( radCementLibraryPriority priority )
{
    rAssert( priority < radCementLibraryNumPriorities );

    m_pIter = m_pHead[ priority ];
}

//=============================================================================
// Function:    radCementLibraryList::GetNext
//=============================================================================
// Description: Get the next library off the list.
//
// Parameters:
//
// Returns:     NULL if there are no more libraries.
//------------------------------------------------------------------------------

radCementLibrary* radCementLibraryList::GetNext( void )
{
    radCementLibrary* ret = m_pIter;

    if ( m_pIter != NULL )
    {
        m_pIter = m_pIter->m_pNext;
    }

    return ret;
}

