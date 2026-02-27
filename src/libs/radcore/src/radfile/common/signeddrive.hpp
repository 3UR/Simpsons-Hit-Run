//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef	SIGNEDDRIVE_HPP
#define SIGNEDDRIVE_HPP

//=============================================================================
// Include Files
//=============================================================================

#include "drive.hpp"

//=============================================================================
// Class Declarations
//=============================================================================

class radSignedReader
{
public:

    radDrive::CompletionStatus SignedReadFile( 
                                       radDrive* pDrive,
                                       radFileHandle handle, 
                                       const char* fileName,
                                       unsigned int position, 
                                       void* pData, 
                                       unsigned int bytesToRead, 
                                       unsigned int* bytesRead, 
                                       radMemorySpace pDataSpace );

    virtual radDrive::CompletionStatus ReadSignedBlock( radFileHandle handle,
                                                       const char* fileName,
                                                       unsigned int block,
                                                       unsigned int position,
                                                       unsigned int numBytes,
                                                       void* pData, 
                                                       radMemorySpace pDataSpace ) = 0;

    virtual unsigned int GetReadBlockSize( void ) = 0;
    virtual unsigned int GetReadHeaderSize( radFileHandle handle ) = 0;
};

#define IMPLEMENT_SIGNED_READ \
    radDrive::CompletionStatus ReadFile( radFileHandle handle, \
                                           const char* fileName, \
                                           IRadFile::BufferedReadState buffState, \
                                           unsigned int position, \
                                           void* pData, \
                                           unsigned int bytesToRead, \
                                           unsigned int* bytesRead, \
                                           radMemorySpace pDataSpace ) \
{ return SignedReadFile( this, handle, fileName, position, pData, bytesToRead, bytesRead, pDataSpace ); }


class radSignedWriter
{
public:

    radDrive::CompletionStatus SignedWriteFile( 
                                       radDrive* pDrive,
                                       radFileHandle handle, 
                                       const char* fileName,
                                       unsigned int position, 
                                       const void* pData, 
                                       unsigned int bytesToWrite, 
                                       unsigned int* bytesWritten,
                                       unsigned int* size,
                                       radMemorySpace pDataSpace );

    virtual radDrive::CompletionStatus WriteSignedBlock( radFileHandle handle,
                                                       const char* fileName,                                               
                                                       unsigned int block,
                                                       unsigned int position,
                                                       unsigned int numBytes,
                                                       const void* pData,
                                                       unsigned int* size,
                                                       radMemorySpace pDataSpace ) = 0;

    virtual unsigned int GetWriteBlockSize( void ) = 0;
    virtual unsigned int GetWriteHeaderSize( radFileHandle handle ) = 0;
};


#define IMPLEMENT_SIGNED_WRITE \
    radDrive::CompletionStatus WriteFile( radFileHandle handle, \
                                        const char* fileName, \
                                        IRadFile::BufferedReadState buffState, \
                                        unsigned int position, \
                                        const void* pData, \
                                        unsigned int bytesToWrite, \
                                        unsigned int* bytesWritten, \
                                        unsigned int* size, \
                                        radMemorySpace pDataSpace ) \
{ return SignedWriteFile( this, handle, fileName, position, pData, bytesToWrite, bytesWritten, size, pDataSpace ); }

#endif
