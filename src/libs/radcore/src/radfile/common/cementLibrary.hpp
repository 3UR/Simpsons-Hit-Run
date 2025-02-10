//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        cementLibrary.hpp
//
// Subsystem:   Radical Cement File Tool - Cement Library Data Structures
//
// Description: This file contains prototypes and structures for creating and
//              modifying cement files.
//
// Note:        The convention for radcore cement file structures is that
//              they are prefixed with "radCF"
//
// Revisions:   V1.00   May 29, 2001        Creation
//
//=============================================================================

#ifndef  CEMENTLIBRARY_HPP
#define  CEMENTLIBRARY_HPP

//=============================================================================
// Include files
//=============================================================================

#include <string.h>
#include <radkey.hpp>

//=============================================================================
// Library versioning information
//=============================================================================

//
// Indicates this is a library (DO NOT MODIFY)
//
#define RAD_CEMENT_LIBRARY_IDENTIFICATION_LENGTH  32
#define RAD_CEMENT_LIBRARY_IDENTIFICATION  "RADCORE CEMENT LIBRARY"

//
// Major and minor version numbers
//
#define RAD_CEMENT_LIBRARY_VERSION_MAJOR 1
#define RAD_CEMENT_LIBRARY_VERSION_MINOR 2


//=============================================================================
// Enums and typedefs
//=============================================================================

//
// Format for storing file modification times
//
typedef long FileTime;


//=============================================================================
// File writing conventions
//=============================================================================

//
// The convention for writing library files is as follows...
//
//  1) Write the file info structure
//          Since there is no dynamic data, this is easy
//  2) Write the header structure at the position specified in the file info
//          First, the structure is stored, then each piece of dynamic
//          data (the hashed file entries) are stored in the order the appear
//          in the structure.
//  3) Write the detailed file information in the position specified by the
//     header
//          The dynamic detailed file info is also written in the order that
//          it is refered to.  For each record, we write the individual pieces
//          of data (filename length, filename, mod time) seperately to disk.
//  4) Write each of the files at the position that they specified at in
//     the header.
//          


//=============================================================================
// Structures
//=============================================================================

//=============================================================================
// Structure:   radCFFileInfo
//=============================================================================
// Purpose:     This structures stores all the general info needed for modifying
//              a cement file.
// Important Data:
//   A tag indicating this is a library file.
//   A major.minor version so that compatibility can be deduced
//   Alignment value
//   Padding value and start of real data
//
//=============================================================================

struct radCFFileInfo
{
    //
    // Store the cement library name
    //
    char m_CementLibraryIdentification[ RAD_CEMENT_LIBRARY_IDENTIFICATION_LENGTH ];

    //
    // Store the versions
    //
    unsigned char m_CementLibraryVersionMajor;
    unsigned char m_CementLibraryVersionMinor;

    //
    // Are we using big endian or not?
    //
    unsigned char m_BigEndian;

    //
    // Is this library valid?  This is not set
    // to true until the entire library has 
    // been written to disk
    //
    unsigned char m_Valid;

    //
    // Alignment value
    //
    unsigned int m_Alignment;

    //
    // Padding net size
    //
    unsigned int m_PadNetSize;

    //
    // Start position of the header
    //
    unsigned int m_HeaderStartPos;
};


//=============================================================================
// Structure:   radCFHeader
//=============================================================================
// Purpose:     This structure stores all information needed to find and access
//              a cement file within the library.
//
//=============================================================================

struct radCFHeader
{
    //
    // How many files are there in total
    //
    unsigned int m_NumFiles;

    //
    // Where is the detailed file info?
    //
    unsigned int m_DetailedFileInfoStartPos;

    //
    // Where does the first file start?
    //
    unsigned int m_FirstFileStartPos;

    //
    // Define a structure for a hashed file entry
    //
    struct HashedFileEntry
    {
        //
        // What is the file name's hashed value
        //
        unsigned int m_HashValue;

        //
        // What is the offset in the library of the file
        //
        unsigned int m_FileOffset;

        //
        // What is the size of the file
        //
        unsigned int m_FileSize;
    };

    //
    // Store an alias for a hashed file entry
    //
    typedef HashedFileEntry HFE;

    //
    // Previously m_pHashedFileEntries pointer
    //
    unsigned int m_Reserved;


    //=========================================================================
    // Function:    radCFHeader::HashFunction
    //=========================================================================
    // Description: Hash a given string to an unsigned int.
    //
    // Parameters:  string - string to hash
    //
    // Returns:     hash value
    //
    //-------------------------------------------------------------------------
    static inline unsigned int HashFunction( const char* string )
    {
        return radMakeCaseInsensitiveKey32( string );
    }

    //=========================================================================
    // Function:    radCFHeader::FindFile
    //=========================================================================
    // Description: Finds the given file name in the hashed file entry list.
    //
    // Parameters:  pHeader - the header to check
    //              hashValue - the hash value of the file name (Using
    //                          radCFHeader::HashFunction)
    //
    // Returns:     Returns a pointer to the hashed file entry for the
    //              desired file, or NULL if none found.
    //
    // Notes:       Currenly, we perform a binary search, and assume that
    //              the hashed file entries are sorted in assending order
    //              by the hash value.
    //-------------------------------------------------------------------------
    static HashedFileEntry* FindFile
    (
        const radCFHeader* pHeader,
        radCFHeader::HashedFileEntry* pHashedFileEntries,
        unsigned int hashValue
    )
    {
        unsigned int low = 0;
        unsigned int mid;
        unsigned int high = pHeader->m_NumFiles;

        //
        // Bail out with not-found result if the cement library is empty.
        //
        if( pHeader->m_NumFiles < 1 )
        {
            return NULL;
        }

        //
        // Search for an entry with the same hash using a binary search
        //
        while( low <= high )
        {
            //
            // Calculate the mid point.  The actual function is:
            //      mid = ( low + high ) / 2;
            // We spice it up a bit to avoid overflow problems and
            // to confuse anybody who may be reading it ;)
            //
            mid = ( low / 2 ) + ( high / 2 );
            if( low&0x01 && high&0x01 )
            {
                mid++;
            }

            //
            // Do they match?
            //
            if( pHashedFileEntries[ mid ].m_HashValue == hashValue )
            {
                return pHashedFileEntries + mid;
            }

            //
            // Update the search
            //
            if( pHashedFileEntries[ mid ].m_HashValue < hashValue )
            {
                low = mid + 1;
            }
            else
            {
                //
                // We don't want to "underflow" unsigned int
                //
                if( mid == 0 )
                {
                    break;
                }
                high = mid - 1;
            }
        }

        //
        // None found, return NULL
        //
        return NULL;
    }

};


//=============================================================================
// Structure:   radCFDetailedFileInformation
//=============================================================================
// Purpose:     This structures stores stores the actual strings (not hashed
//              values) for all file names in the library.  It also stores
//              any extra information such as the last modification time of a file.
//
// IMPORTANT:   The file information blocks must be placed in decreasing
//              order.  That is. for example, the 256-character block
//              goes first, then the 21-character block, then the 9-character
//              block goes last.  This specification is used by search algorithms
//              iterating the file information.
//
// How it works:
//              We store the detailed file information in blocks of varying string
//              size.  This way we may have file paths of a very long size, but
//              they will not take up an unnecessary amount of storage room in
//              the file.
//
//=============================================================================

struct radCFDetailedFileInformation
{
    //
    // How many files are here?
    //
    unsigned int m_NumFiles;

    //
    // Define a structure for an actual file information record
    //
    struct DetailedFileInfoRecord
    {
        //
        // How long is the file name
        // (Maximum length including NULL terminator)
        //
        unsigned int m_FileNameLength;
        
        //
        // The file name (allocated to m_FileNameLength characters)
        //
        char* m_FileName;

        //
        // The file's last modification time (for update)
        //
        FileTime m_FileModificationTime;
    };

    //
    // Store an alias for a detailed file info record
    //
    typedef DetailedFileInfoRecord DFIR;

    //
    // Store a group of m_NumFilesInBlock file entries
    //
    DetailedFileInfoRecord* m_pFileInfoRecords;

    //=========================================================================
    // Function:    radCFDetailedFileInformation::FindFile
    //=========================================================================
    // Description: Finds the given file name in the detailed file info.
    //
    // Parameters:  pDetailedFileInfo - the detailed file information to check
    //              filename - the name of the file to find
    //
    // Returns:     Returns a pointer to the detailed info record corresponding
    //              to the given name.
    //
    //-------------------------------------------------------------------------
    static DFIR* FindFile
    (
        const radCFDetailedFileInformation* pDetailedFileInfo,
        const char* fileName
    )
    {
        //
        // Store the string length
        //
        unsigned int stringlength = strlen( fileName );

        //
        // Search for the entry
        //
        unsigned int i;
        for( i = 0; i < pDetailedFileInfo->m_NumFiles; i++ )
        {
            //
            // Do they match?
            //
            if( stringlength == pDetailedFileInfo->m_pFileInfoRecords[ i ].m_FileNameLength )
            {
                if( strcmp( pDetailedFileInfo->m_pFileInfoRecords[ i ].m_FileName, fileName ) == 0 )
                {
                    return &pDetailedFileInfo->m_pFileInfoRecords[ i ];
                }
            }
        }

        //
        // None found, return NULL
        //
        return NULL;
    }
};


#endif //CEMENTLIBRARY_HPP

