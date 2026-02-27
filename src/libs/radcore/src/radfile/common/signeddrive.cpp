//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <radmemory.hpp>
#include "signeddrive.hpp"

//=============================================================================
// Public Member Functions
//=============================================================================

radDrive::CompletionStatus radSignedReader::SignedReadFile
(
    radDrive* pDrive,
    radFileHandle handle, 
    const char* fileName,
    unsigned int position, 
    void* pData, 
    unsigned int bytesToRead, 
    unsigned int* bytesRead, 
    radMemorySpace pDataSpace 
)
{
    unsigned int bufferSize = GetReadBlockSize( );

    //
    // First translate the position to skip any header.
    //
    position += GetReadHeaderSize( handle );

    //
    // To do a read we need the block's address, the size and the buffer.
    //
    radDrive::CompletionStatus result = radDrive::Complete;
    unsigned int blockAddress = position / bufferSize;
    unsigned int size = bytesToRead;
    char* pBuffer = (char*) pData;

    //
    // First we need to read the head part.
    //
    unsigned int extraBytes = position - ::radMemoryRoundDown( position, bufferSize );

    //
    // Number of bytes we need from this read.
    //
    unsigned int userBytes = bufferSize - extraBytes;
    userBytes = userBytes < size ? userBytes : size;

    //
    // Do the read.
    //
    result = ReadSignedBlock( handle, fileName, blockAddress, extraBytes, userBytes, pBuffer, pDataSpace );

    //
    // Check if we succeeded.
    //
    if ( result != radDrive::Complete )
    {
        return result;
    }

    blockAddress++;
    size -= userBytes;
    pBuffer += userBytes;

    //
    // Read up until the tail.
    //
    userBytes = bufferSize;
    while ( size >= userBytes )
    {
        result = ReadSignedBlock( handle, fileName, blockAddress, 0, userBytes, pBuffer, pDataSpace );

        //
        // Check if we succeeded.
        //
        if ( result != radDrive::Complete )
        {
            return result;
        }

        blockAddress++;
        size -= userBytes;
        pBuffer += userBytes;
    }

    //
    // Finally the tail is left
    //
    if ( size > 0 )
    {
        result = ReadSignedBlock( handle, fileName, blockAddress, 0, size, pBuffer, pDataSpace );

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
};

radDrive::CompletionStatus radSignedWriter::SignedWriteFile
(
    radDrive* pDrive,
    radFileHandle handle, 
    const char* fileName,
    unsigned int position, 
    const void* pData, 
    unsigned int bytesToWrite, 
    unsigned int* bytesWritten,
    unsigned int* pSize,
    radMemorySpace pDataSpace
)
{
    unsigned int bufferSize = GetWriteBlockSize( );

    //
    // First translate the position to skip any header.
    //
    position += GetWriteHeaderSize( handle );

    //
    // To do a write we need the blocks address, the size and the buffer.
    //
    radDrive::CompletionStatus result = radDrive::Complete;
    unsigned int blockAddress = position / bufferSize;
    unsigned int size = bytesToWrite;
    char* pBuffer = (char*) pData;

    //
    // First we need to write the head part.
    //
    unsigned int extraBytes = position - ::radMemoryRoundDown( position, bufferSize );

    //
    // Number of bytes we need from this write.
    //
    unsigned int userBytes = bufferSize - extraBytes;
    userBytes = userBytes < size ? userBytes : size;

    //
    // Do the write.
    //
    result = WriteSignedBlock( handle, fileName, blockAddress, extraBytes, userBytes, pBuffer, pSize, pDataSpace );

    //
    // Check if we succeeded.
    //
    if ( result != radDrive::Complete )
    {
        return result;
    }

    blockAddress++;
    size -= userBytes;
    pBuffer += userBytes;

    //
    // write up until the tail.
    //
    userBytes = bufferSize;
    while ( size >= userBytes )
    {
        result = WriteSignedBlock( handle, fileName, blockAddress, 0, userBytes, pBuffer, pSize, pDataSpace );

        //
        // Check if we succeeded.
        //
        if ( result != radDrive::Complete )
        {
            return result;
        }

        blockAddress++;
        size -= userBytes;
        pBuffer += userBytes;
    }

    //
    // Finally the tail is left
    //
    if ( size > 0 )
    {
        result = WriteSignedBlock( handle, fileName, blockAddress, 0, size, pBuffer, pSize, pDataSpace );

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
    *bytesWritten = bytesToWrite;
    return radDrive::Complete;
};