//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

#ifdef WIN32

#include <p3d/platform/win32/plat_filemap.hpp>
#include <p3d/utility.hpp>

#include <io.h>
#include <fcntl.h>

#ifndef _WINDOWS_
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <winbase.h>
#endif

tWin32FileMap::tWin32FileMap(const char* filename)
{
    hFile = INVALID_HANDLE_VALUE;
    hMap = 0;
    length = 0;
    Open(filename);
    SetFilename((char*)filename);
}

tWin32FileMap::~tWin32FileMap()
{
    Close();
}

void tWin32FileMap::Open(const char* filename)
{
    int fh = _open(filename, _O_RDONLY);
    if(fh == -1)
        return;   

    length = _filelength(fh);
    _close(fh);

    hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
        return;

    hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if(!hMap)
    {
        CloseHandle(hFile);
        return;
    }

    // get a usable pointer to mapped file
    unsigned char* memory = (unsigned char*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if(!memory)
    {
        CloseHandle(hFile);
        CloseHandle(hMap);
        return;
    }
    dataStream->Release();
    dataStream = new radLoadDataStream( memory, length, del );

}

void tWin32FileMap::Close()
{
    if(hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }
    if(hMap)
    {
        CloseHandle(hMap);
        hMap = 0;
    }

    if(GetMemory())
    {
        UnmapViewOfFile(GetMemory());
        dataStream->Release();
    }

    length = 0;
}


tWin32FileAsync::tWin32FileAsync(const char* filename)
{
    hFile = INVALID_HANDLE_VALUE;
    hEvent = INVALID_HANDLE_VALUE;

    int fh = _open(filename, _O_RDONLY);
    if(fh == -1)
        return;   

    fileSize = _filelength(fh);
    _close(fh);

    hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    SetFilename((char*)filename);
    first = true;
    filePosition = 0;
}

tWin32FileAsync::~tWin32FileAsync()
{
    if(hFile != INVALID_HANDLE_VALUE)
        CloseHandle((HANDLE)hFile);

    if(hEvent != INVALID_HANDLE_VALUE)
        CloseHandle((HANDLE)hEvent);
}

bool tWin32FileAsync::IsOpen(void)
{
    return (hFile != INVALID_HANDLE_VALUE);
}

bool tWin32FileAsync::EndOfFile(void)
{
    return filePosition <= fileSize;
}

unsigned tWin32FileAsync::GetSize(void)
{
    return fileSize;
}

unsigned tWin32FileAsync::GetPosition(void)
{
    return filePosition;
}

void     tWin32FileAsync::Advance(unsigned offset)
{
    filePosition += offset;
    if(offset < (FILE_BUFFER_SIZE - bufferPos))
    {
        bufferPos += offset;
    }
    else
    {
        FillBuffer(filePosition);
    }
}

void     tWin32FileAsync::SetPosition(int offset)
{
    filePosition = offset;
    if(((unsigned)offset >= bufferStart ) & ((unsigned)offset < (bufferStart  + FILE_BUFFER_SIZE)))
    {
        bufferPos = offset - bufferStart;
    }
    else
    {
        FillBuffer(filePosition);
    }
}

bool tWin32FileAsync::GetData(void* buf, unsigned count, DataType type)
{
    if(first)
    {
        first = false;
        FillBuffer(filePosition);
    }

    filePosition += count * type;

    // TODO : endian
    if((bufferPos + (count * type)) < FILE_BUFFER_SIZE)
    {
        memcpy(buf, &buffer[bufferPos], count * type);
        bufferPos += count * type;
        return true;
    }

    char* b = (char*)buf;
    unsigned byteCount = count * type;

    while((bufferPos + byteCount) >= FILE_BUFFER_SIZE)
    {
        memcpy(b, &buffer[bufferPos], FILE_BUFFER_SIZE - bufferPos);
        b += FILE_BUFFER_SIZE - bufferPos;
        byteCount -= FILE_BUFFER_SIZE - bufferPos;
        FillBuffer(bufferStart + FILE_BUFFER_SIZE);
    }

    if(byteCount)
    {
        memcpy(b, &buffer[bufferPos], byteCount);
        bufferPos += byteCount;
    }

    return true;
}

bool tWin32FileAsync::PeekData(void* buf, unsigned count, DataType type)
{
    if(first)
    {
        first = false;
        FillBuffer(filePosition);
    }

    if((bufferPos + count) < FILE_BUFFER_SIZE)
    {
        memcpy(buf, &buffer[bufferPos], count * type);
        return true;
    }

    int f = GetPosition();
    GetData(buf, count, type);
    SetPosition(f);
    return true;
}

void tWin32FileAsync::FillBuffer(unsigned start)
{
    bufferStart = start;
    bufferPos = 0;
    OVERLAPPED overlapped;
    overlapped.Offset = start;
    overlapped.OffsetHigh = 0;
    overlapped.hEvent = hEvent; 

    int size = ((fileSize - start) < FILE_BUFFER_SIZE) ? (fileSize - start) : FILE_BUFFER_SIZE;

    int rc = ReadFile(hFile, buffer, size, NULL, &overlapped);

    if(rc != 0)
    {
        do
        {
            p3d::loadManager->SwitchTask();
        }
        while(WaitForSingleObject(hEvent, 1) == 0x00000102);
    }
}

/*
    unsigned bufferStart[2];
    bool     bufferFilled[2];
    char     buffer[2][FILE_BUFFER_SIZE];

    unsigned curBuffer;
    unsigned curBufferPos;

    unsigned fileSize;
    unsigned filePosition;
    */

#endif // WIN32