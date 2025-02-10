//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

#ifdef WIN32

#ifndef _PLAT_FILEMAP_HPP
#define _PLAT_FILEMAP_HPP

#include <p3d/file.hpp>

class tWin32FileMap : public tFileMem
{
public:
    tWin32FileMap(const char* filename);

    bool IsOpen(void) { return GetMemory() != NULL; }

protected:
    virtual ~tWin32FileMap();

    void Open(const char* filename);
    void Close(void);

    void* hMap;
    void* hFile;
};

const int FILE_BUFFER_SIZE = 65536;

class tWin32FileAsync : public tFile
{
public:
    tWin32FileAsync(const char* filename);
    ~tWin32FileAsync();

    bool     EndOfFile(void);
    unsigned GetSize(void);
    unsigned GetPosition(void);
    void     Advance(unsigned);
    void     SetPosition(int offset);

    bool GetData(void* buf, unsigned count, DataType type = BYTE);
    bool PeekData(void* buf, unsigned count, DataType type = BYTE);

    bool IsOpen(void);
protected:
    void FillBuffer(unsigned start);

    bool     first;

    unsigned bufferStart;
    unsigned bufferPos;
    char     buffer[FILE_BUFFER_SIZE];


    unsigned fileSize;
    unsigned filePosition;

    void* hEvent;
    void* hFile;
};

#endif /*_FILEMAP_HPP*/
#endif // WIN32