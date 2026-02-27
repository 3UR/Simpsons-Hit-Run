//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        cementer.hpp
//
// Subsystem:   Radical Cement Library
//
// Description: This file contains classes relevant to cement files and cement
//              file libraries.  These classes are used by the file system
//              to interpret cement files as if they were normal files.
//
// Revisions:   07-June-2001    Creation
//
//=============================================================================

//
// Cement files (extent into the file system):
//      * library and routines in radDrive
//      * members in radFile
//      * code sections in FileOpenRequest, FileCloseRequest, FileReadRequest 
//      * FileWriteRequest (assert).
//

#ifndef	CEMENTER_HPP
#define CEMENTER_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radobject.hpp> 
#include <radfile.hpp>
#include "saferefobject.hpp"
#include "cementLibrary.hpp"

//=============================================================================
// Forward references
//=============================================================================

class radFile;
class radDrive;

//=============================================================================
// Public Function Declarations
//=============================================================================

//
// Create a cement library and register it with the appropriate
// drive object.
//
void radCementLibraryCreate
(
    IRadCementLibrary** pIRadCementLibrary,
    radDrive* pDrive,
    const char* cementLibraryFileName,
    radCementLibraryPriority priority,
    unsigned int cacheSize,
    radMemoryAllocator alloc,
    radMemorySpace cacheSpace
);


//=============================================================================
// Class Declarations
//=============================================================================

//
// This class is responsible for managing a cement library.
//
class radCementLibrary : public IRadCementLibrary,
                         public IRadFileCompletionCallback,
                         public radObject,
                         public radSafeRefCount
{
public:
    //
    // Constructor and destructor
    //
    radCementLibrary( radDrive* pDrive, radFile* pDataFile, radCementLibraryPriority priority );
    virtual ~radCementLibrary( );

    IMPLEMENT_SAFEREFCOUNTED;

    //
    // IRadMutable
    //
    void Lock( void );
    void Unlock( void );

    //
    // IRadCementLibrary
    CementLibraryStatus GetStatus( void );
    void WaitForCompletion( void );

    //
    // IRadFileCompletionCallback
    //
    void OnFileOperationsComplete( void* pUserData );

    //
    // Open a file. Return NULL if it doesn't exist.
    //
    radFile* OpenFile( const char* fileName,
                       unsigned int* pOffset,
                       unsigned int* pSize );

    //
    // Put in the callback
    //
    void SetCompletionCallback( IRadCementLibraryCompletionCallback* pCallback,
                                void* pUserData );

    //
    // Used for backward compatibility
    //
    bool IsOpen( void );

    //
    // Return the priority of this library.
    //
    radCementLibraryPriority GetPriority( void ) const;

private:
    friend class radCementLibraryList;

    // 
    // A pointer to the drive object with which this library is registered
    //
    radDrive*   m_pDrive;

    //
    // A pointer to the data file
    //
    radFile*    m_pDataFile;

    //
    // Use this to remember the libary's state
    //
    CementLibraryStatus m_CurrentStatus;

    //
    // Store the header info and hash table.
    //
    radCFHeader*        m_Header;

    //
    // Store the hashed file information
    // Note: There is m_NumFiles worth of data here
    // Important: The hashed file entries are sorted
    //            in increasing order by there hash
    //            value.
    //
    radCFHeader::HashedFileEntry* m_pHashedFileEntries;

    //
    // Pointer to the next library. Used by the library list.
    //
    radCementLibrary*   m_pNext;

    //
    // Remeber our priority
    //
    radCementLibraryPriority m_Priority;

    //
    // User's callback.
    //
    IRadCementLibraryCompletionCallback* m_pCallback;
    void*                                m_pUserData;

    //
    // Temp buffer
    //
    char*   m_TmpBuffer;

    //
    // Finish the registration with the given status.
    //
    void CompleteRegister( IRadCementLibrary::CementLibraryStatus status );
};


//
// This class is used to control a linked list (FIFO) of cement libraries.
// This is an internal object and is not reference counted.
//
class radCementLibraryList
{
public:
    //
    // Constructor and destructor
    //
    radCementLibraryList( void );
    ~radCementLibraryList( void );

    //
    // Add an element to cement a library list
    //
    void AddLibrary( radCementLibrary* pCementLibrary );

    //
    // Remove a library from a library list
    //
    void RemoveLibrary( const radCementLibrary* pCementLibrary );

    //
    // Iterations.
    //
    void BeginIteration( radCementLibraryPriority priority );
    radCementLibrary* GetNext( void );

private:
    radCementLibrary*   m_pHead[ radCementLibraryNumPriorities ];
    radCementLibrary*   m_pTail[ radCementLibraryNumPriorities ];

    radCementLibrary*   m_pIter;
};


#endif //CEMENTER_HPP

