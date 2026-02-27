//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/buildconfig.hpp>

#ifndef _FILEFTT_HPP_
#define _FILEFTT_HPP_

#include <p3d/file.hpp>
#include <raddebug.hpp>
#include <radfile.hpp>

class tFileFTT : public tFile, public IRadFileCompletionCallback
{
public:
    tFileFTT(const char* filename, bool synchronous = true);

    bool     EndOfFile();
    unsigned GetSize();
    unsigned GetPosition();
    void     Advance(unsigned offset);
    void     SetPosition(int offset);

    bool   GetData(void* buf, unsigned nbytes, DataType);

    void   SetCompressed(bool b);
    void   SetUncompressedSize(int size);

    void   OnFileOperationsComplete(void*);
    void   WaitForCompletion( void );

    void   AddRef() { tFile::AddRef(); }         
    void   Release() { tFile::Release(); }

    static void SetYieldTime(unsigned ms);

protected:

    IRadFile*    m_pIRadFile;
    int fileSize;
    int compressedFileSize;

    int position;
    int filePosition;

    int currentCache;

    char* cache[2];
    char* uncompressedcache;

    char* currentBuffer;
    int currentPos;
    int currentSize;
    int cycle_size;

    char* compressedBuffer;
    int compressedPos;
    int compressedSize;
    
    int nextSize;

    int bufferSize;
    
    bool done; 

    static unsigned mTaskedOutTime;

    void OpenFile( void );
    void ReadBuffer(char* buf);
    void FillBuffer();
    void AdvanceCycle(int numBytes);
    void Decompress (const unsigned char* input, unsigned int inputsize,
                            unsigned char* output, unsigned int outputsize);

    virtual ~tFileFTT();

};

#endif

