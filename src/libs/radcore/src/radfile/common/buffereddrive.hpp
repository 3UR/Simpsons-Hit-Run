//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef	BUFFEREDDRIVE_HPP
#define BUFFEREDDRIVE_HPP

//=============================================================================
// Include Files
//=============================================================================

#include "drive.hpp"

//=============================================================================
// Class Declarations
//=============================================================================

class radBufferedReader
{
public:

    radDrive::CompletionStatus BufferedReadFile( 
                                       radDrive* pDrive,
                                       radFileHandle handle,
                                       const char* fileName,
                                       IRadFile::BufferedReadState buffState,
                                       unsigned int position, 
                                       void* pData, 
                                       unsigned int bytesToRead, 
                                       unsigned int* bytesRead, 
                                       radMemorySpace pDataSpace );

    //
    // Read optimally. Read numSectors starting at sector into pData.
    // Assumes that pData is properly aligned.
    //
    virtual radDrive::CompletionStatus ReadAligned( radFileHandle handle,
                                                    const char* fileName,
                                                    unsigned int sector,
                                                    unsigned int numSectors,
                                                    void* pData, 
                                                    radMemorySpace pDataSpace ) = 0;
 
    //
    // Read non-optimally. Read numSectors starting at sector, and transfer
    // numBytes of that into pData starting at position within the first read sector.
    // numSectors is guaranteed to not exceed GetReadBufferSize( )
    //
    virtual radDrive::CompletionStatus ReadBuffered( radFileHandle handle,
                                                     const char* fileName,
                                                     unsigned int sector,
                                                     unsigned int numSectors,
                                                     unsigned int position,
                                                     unsigned int numBytes,
                                                     void* pData, 
                                                     radMemorySpace pDataSpace ) = 0;
    virtual unsigned int GetReadBufferSectors( void ) = 0;
};

#define IMPLEMENT_BUFFERED_READ \
    radDrive::CompletionStatus ReadFile( radFileHandle handle, \
                                           const char* fileName, \
                                           IRadFile::BufferedReadState buffState, \
                                           unsigned int position, \
                                           void* pData, \
                                           unsigned int bytesToRead, \
                                           unsigned int* bytesRead, \
                                           radMemorySpace pDataSpace ) \
{ return BufferedReadFile( this, handle, fileName, buffState, position, pData, bytesToRead, bytesRead, pDataSpace ); }


/*
//
// This is a subclass of radDrive which implements the write function and
// replaces it a new contract for subclasses used for non-optimal reading.
//
class radBufferedWriteDrive : public radDrive
{
public:
    //
    // Give the number of sectors which are buffered in non-optimal reads
    //
    radBufferedWriteDrive( const char* pDriveName, unsigned int numSectors );
    virtual ~radBufferedWriteDrive( );

    CompletionStatus WriteFile( radFileHandle handle, 
                                unsigned int position, 
                                const void* pData, 
                                unsigned int bytesToWrite, 
                                unsigned int* bytesWritten, 
                                unsigned int* size, 
                                radMemorySpace pDataSpace );

    virtual CompletionStatus WriteAligned( radFileHandle handle, 
                                           unsigned int sector,
                                           unsigned int numSectors,
                                           void* pData, 
                                           radMemorySpace pDataSpace ) = 0;
 
    virtual CompletionStatus WriteBuffered( radFileHandle handle,
                                            unsigned int sector,
                                            unsigned int numSectors,
                                            unsigned int position,
                                            unsigned int numBytes,
                                            void* pData, 
                                            radMemorySpace pDataSpace ) = 0;
};
*/
#endif
