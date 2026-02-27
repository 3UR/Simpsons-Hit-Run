//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _CHUNKFILE_HPP_
#define _CHUNKFILE_HPP_

#include <p3d/file.hpp>

const int CHUNK_STACK_SIZE = 32;

class tChunkFile : public radLoadStream
{
public:

    tChunkFile(tFile*);
    ~tChunkFile();

    bool     ChunksRemaining(void);
    unsigned BeginChunk(void);
    void     EndChunk(void);

    unsigned GetCurrentID(void);
    unsigned GetCurrentDataLength(void);
    unsigned GetCurrentOffset(void);

    tFile* BeginInset(void);
    void   EndInset(tFile*);

    unsigned       GetUInt(void)           { return realFile->GetDWord();}
    int            GetInt(void)            { return (int)realFile->GetDWord();}
    unsigned short GetUShort(void)         { return realFile->GetWord();}
    short          GetShort(void)          { return (short)realFile->GetWord();}
    unsigned char  GetUChar(void)          { return realFile->GetByte();}
    char           GetChar(void)           { return (char)realFile->GetByte();}
    float          GetFloat(void)          { FloatInt tmp; tmp.un = realFile->GetDWord(); return tmp.fn;}
    char*          GetString(char* string) { unsigned len = (unsigned)realFile->GetByte(); realFile->GetData(string, len); string[len] = 0; return string;}

    bool GetData(void* buf, unsigned count, tFile::DataType type = tFile::BYTE)  { return realFile->GetData(buf,count,type);}
    virtual bool Read(void* data, unsigned count, unsigned size) { return GetData(data, count, static_cast<tFile::DataType>(size));}

    virtual unsigned GetSize() { return realFile->GetSize(); }
    virtual unsigned GetPosition() { return realFile->GetPosition(); }

    const char* GetFullFilename(void) { return realFile->GetFullFilename();}
    const char* GetFilename(void) { return realFile->GetFilename();}

    // legacy functions
    unsigned short GetUWord() {return GetUShort();}
    short          GetWord()  {return GetShort();}
    int            GetLong()  {return GetInt();}
    char*          GetPString(char* string)               { return GetString(string);}    

protected:
    unsigned BeginChunk(unsigned);

    struct Chunk
    {
        unsigned id, dataLength, chunkLength, startPosition;
    } chunkStack[CHUNK_STACK_SIZE];

    union FloatInt
    {
        unsigned un;
        float    fn;
    };

    int stackTop;

    tFile* realFile;
};

#endif

