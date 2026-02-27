//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <radmemory.hpp>
#include "buffereddrive.hpp"

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    radBufferedReader::ReadFile
//=============================================================================
// Description: This member reads a file, fixing up alignment problems if needed.
// Properly handle bytesRead sometime ...
//-----------------------------------------------------------------------------

radDrive::CompletionStatus radBufferedReader::BufferedReadFile
(
    radDrive* pDrive,
    radFileHandle handle, 
    const char* fileName,
    IRadFile::BufferedReadState buffState,
    unsigned int position, 
    void* pData, 
    unsigned int bytesToRead, 
    unsigned int* bytesRead, 
    radMemorySpace pDataSpace 
)
{
    unsigned int sectorSize = pDrive->GetMediaInfo( )->m_SectorSize;

#if defined RAD_DEBUG && defined RADFILE_DEBUG_WARNINGS
    //
    // Print warning about non-optimal reads. Asserts if buffered read is off.
    //
    if ( buffState != IRadFile::BufferedReadOn )
    {
        if( !radMemoryIsAligned( ( unsigned int ) pData, radFileOptimalMemoryAlignment ) )
        {
            rDebugPrintf( "Warning: File reads on file [%s%s] are occuring non-optimally.\n"
                "Warning: Memory at [%X] should be aligned to radFileOptimalMemoryAlignment (%u).\n", 
                pDrive->GetDriveName( ), fileName, pData, radFileOptimalMemoryAlignment );

            rAssertMsg( buffState != IRadFile::BufferedReadOff, "Buffered reading is off" );
        }

        if( position % sectorSize != 0 )
        {
            rDebugPrintf( "Warning: File reads on drive [%s%s] are occuring non-optimally.\n"
                "Warning: File offset [%X] should be aligned to the sector size (%u).\n", 
                pDrive->GetDriveName( ), fileName, position, sectorSize );

            rAssertMsg( buffState != IRadFile::BufferedReadOff, "Buffered reading is off" );
        }

        if( bytesToRead % sectorSize != 0 )
        {
            rDebugPrintf( "Warning: File reads on drive [%s%s] are occuring non-optimally.\n"
                "Warning: Read size [%u] should be aligned to the sector size (%u).\n", 
                pDrive->GetDriveName( ), fileName, bytesToRead, sectorSize );

            rAssertMsg( buffState != IRadFile::BufferedReadOff, "Buffered reading is off" );
        }
    }
#endif // RADFILE_DEBUG_WARNINGS

    //
    // Initialize variables for reading
    //
    radDrive::CompletionStatus result = radDrive::Complete;
    unsigned int lsn = position / sectorSize;
    unsigned int size = bytesToRead;
    char* pBuffer = (char*) pData;

    //
    // If the disk or memory are not aligned, we might need to buffer everything.
    //
    if( 
        !radMemoryIsAligned( ( unsigned int ) pData, radFileOptimalMemoryAlignment ) ||
        position % sectorSize != 0
      )
    {
        //
        // If the memory alignment and position alignment are off by the same amount
        // then things aren't so bad, and we can just go to the next case. But if this
        // isn't the case, we must buffer everything.
        //
        if ( (unsigned int) pData % radFileOptimalMemoryAlignment != position % sectorSize )
        {
            //
            // Find how much extra we are reading before position
            //
            unsigned int extraBytes = position - ::radMemoryRoundDown( position, sectorSize );

            //
            // Pick the smaller of the actual disc aligned read size, and our buffer size
            //
            unsigned int userSectors = GetReadBufferSectors( );
            unsigned int numSectors = ( size + extraBytes + sectorSize - 1 ) / sectorSize;
            numSectors = numSectors < userSectors ? numSectors : userSectors;

            //
            // Number of bytes actually copied into the user's buffer. This is the smaller
            // of the user's size and amount read minus extraBytes.
            //
            unsigned int userBytes = numSectors * sectorSize - extraBytes;
            userBytes = userBytes < size ? userBytes : size;

            //
            // Do the buffered read.
            //
            result = ReadBuffered( handle, fileName, lsn, numSectors, extraBytes, userBytes, pBuffer, pDataSpace );

            //
            // Check if we succeeded.
            //
            if ( result != radDrive::Complete )
            {
                return result;
            }

            lsn += numSectors;
            size -= userBytes;
            pBuffer += userBytes;

            //
            // Read the disc aligned data until only the tail is left
            //
            userBytes = userSectors * sectorSize;
            while ( size >= userBytes )
            {
                result = ReadBuffered( handle, fileName, lsn, userSectors, 0, userBytes, pBuffer, pDataSpace );

                //
                // Check if we succeeded.
                //
                if ( result != radDrive::Complete )
                {
                    return result;
                }

                lsn += userSectors;
                size -= userBytes;
                pBuffer += userBytes;
            }

            //
            // Finally the tail is left
            //
            if ( size > 0 )
            {
                numSectors = ( size + sectorSize - 1 ) / sectorSize;
                result = ReadBuffered( handle, fileName, lsn, numSectors, 0, size, pBuffer, pDataSpace );

                //
                // Check if we succeeded.
                //
                if ( result != radDrive::Complete )
                {
                    return result;
                }
            }

            //
            // Finished
            //
            *bytesRead = bytesToRead;
            return radDrive::Complete;
        }
        else
        {
            // position is not aligned, so go to that case.
        }
    }

    //
    // If the read is non-optimal, and small enough to fit into our transfer buffer, 
    // then do so.
    //
    if ( radMemoryRoundUp( size, sectorSize ) <= ( GetReadBufferSectors( ) * sectorSize ) && 
        ( position % sectorSize != 0 || size % sectorSize != 0 )
       )
    {
        //
        // Find how much extra we are reading before position
        //
        unsigned int extraBytes = position - ::radMemoryRoundDown( position, sectorSize );

        //
        // Pick the smaller of the actual disc aligned read size, and our buffer size
        //
        unsigned int numSectors = ( size + extraBytes + sectorSize - 1 ) / sectorSize;
        rAssert( numSectors < GetReadBufferSectors( ) );

        //
        // Do the buffered read.
        //
        result = ReadBuffered( handle, fileName, lsn, numSectors, extraBytes, size, pBuffer, pDataSpace );

        //
        // Check if we succeeded.
        //
        if ( result != radDrive::Complete )
        {
            return result;
        }
        else
        {
            *bytesRead = bytesToRead;
            return radDrive::Complete;
        }
    }

    //
    // Here we either have an optimal read, or only need to buffer the beginning.
    // Read the initial non-aligned part.
    //
    if ( position % sectorSize != 0 )
    {
        //
        // Find how much extra we are reading before position
        //
        unsigned int extraBytes = 
            position - ::radMemoryRoundDown( position, sectorSize );

        //
        // Number of sectors to read, which is 1
        //
        unsigned int numSectors = 1;

        //
        // Number of bytes actually copied into the user's buffer. This is the smaller
        // of the user's size and amount read minus extraBytes.
        //
        unsigned int userBytes = numSectors * sectorSize - extraBytes;
        userBytes = userBytes < size ? userBytes : size;

        //
        // Do the buffered read.
        //
        result = ReadBuffered( handle, fileName, lsn, numSectors, extraBytes, userBytes, pBuffer, pDataSpace );

        //
        // Check if we succeeded.
        //
        if ( result != radDrive::Complete )
        {
            return result;
        }

        lsn += numSectors;
        size -= userBytes;
        pBuffer += userBytes;
    }

    //
    // Read the aligned part if there's at least one sector to read.
    //
    if ( size >= sectorSize )
    {
        unsigned int numSectors = size / sectorSize;
        result = ReadAligned( handle, fileName, lsn, numSectors, pBuffer, pDataSpace );
    
        if ( result != radDrive::Complete )
        {
            return result;
        }

        lsn += numSectors;
        size -= numSectors * sectorSize;
        pBuffer += numSectors * sectorSize;
    }

    //
    // Read trailing non-aligned part
    //
    if ( size > 0 )
    {
        rAssert( size % sectorSize != 0 );
        rAssert( size < sectorSize );

        //
        // Should have only part of a sector left to read
        //
        result = ReadBuffered( handle, fileName, lsn, 1, 0, size, pBuffer, pDataSpace );

        if ( result != radDrive::Complete )
        {
            return result;
        }
    }
    
    *bytesRead = bytesToRead;
    return radDrive::Complete;
}

