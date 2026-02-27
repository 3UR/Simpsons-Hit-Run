//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================



#include <p3d/rawimage.hpp>
#include <p3d/image.hpp>
#include <pddi/pdditype.hpp>
#include <p3d/error.hpp>
#include <p3d/file.hpp>
#include <p3d/utility.hpp>

#include <string.h>
#include <stdio.h>

struct RawHeader
{
    unsigned id;            // must be "P3DI"
    unsigned pixelFormat;   // from pddienum.hpp
    unsigned width;
    unsigned height;
    unsigned bpp;
    unsigned paletteSize;   // in bytes
    unsigned imageSize;     // in bytes
};

const unsigned HEADER_FOURCC = PDDI_FOURCC('P', '3', 'D', 'I');

//-------------------------------------------------------------------
bool tRawImageHandler::CheckFormat(Format format)
{
    switch(format)
    {
        case IMG_PS2_4BIT:
        case IMG_PS2_8BIT:
        case IMG_PS2_16BIT:
        case IMG_PS2_32BIT:
            return true;
        default:
            return false;
    }

    return false;
}

static void GetHeader(tFile* file, RawHeader* header)
{
    file->GetData(header, sizeof(RawHeader)/4, tFile::DWORD);
}

void tRawImageHandler::CreateImage(tFile* file, tImageHandler::Builder* builder)
{
    RawHeader header;
    GetHeader(file, &header);

    if(header.id != HEADER_FOURCC)
    {
        p3d::printf("Warning: unsuported Pure3D memory imaged format from file %s\n", file->GetFilename());
        return;
    }

    // we don't specify a palette since it will be loaded through the memory image interface
    builder->BeginImage(header.width, header.height, header.bpp, tImageHandler::Builder::TOP, NULL);

    if(header.paletteSize > 0)
    {
        void* palettePtr = builder->GetPaletteMemoryImagePtr();
        if(palettePtr)
        {
          file->GetData(palettePtr, header.paletteSize, tFile::BYTE); // read palette memory image
        }
    }

    void* ptr = builder->GetMemoryImagePtr();
    if(ptr)
    {
        file->GetData(ptr, header.imageSize, tFile::BYTE); // read pixels memory image
    }

    builder->EndImage();
}

