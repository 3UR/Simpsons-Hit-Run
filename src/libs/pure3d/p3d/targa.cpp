//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


// TARGA (TGA) image loader
// 26-June-98, Neall Verheyde
// 21-March-99 classified
// 15-Jun-99 factoried
// 21-Feb-00 uses padded targa header

#include <p3d/targa.hpp>
#include <p3d/image.hpp>
#include <pddi/pdditype.hpp>
#include <p3d/error.hpp>
#include <p3d/file.hpp>
#include <p3d/utility.hpp>

#include <string.h>
#include <stdio.h>

enum targaImageType 
{
    TGA_NONE = 0,
    TGA_UNCOMPRESSED_CMAP = 1,
    TGA_UNCOMPRESSED_TRUECOLOUR = 2,
    TGA_UNCOMPRESSED_MONO = 3,
    TGA_RLE_CMAP = 9,
    TGA_RLE_TRUECOLOUR = 10,
    TGA_RLE_MONO = 11
};

const unsigned HEADER_SIZE = 18;  // size on disk, not in memory (due to padding bytes);

typedef struct
{
    unsigned char idLength;
    unsigned char cmapType;
    unsigned char imageType;
    unsigned short cmapStart;
    unsigned short cmapLength;
    unsigned char cmapFormat;
    unsigned short xOrigin;
    unsigned short yOrigin;
    unsigned short width;
    unsigned short height;
    unsigned char bpp;
    unsigned char imageDecr;
} TargaHeader;

static void GetTGAHeader(tFile* f, TargaHeader* header)
{
    header->idLength = f->GetByte();
    header->cmapType = f->GetByte();
    header->imageType = f->GetByte();
    header->cmapStart = f->GetWord();
    header->cmapLength = f->GetWord();
    header->cmapFormat = f->GetByte();
    header->xOrigin = f->GetWord();
    header->yOrigin = f->GetWord();
    header->width = f->GetWord();
    header->height = f->GetWord();
    header->bpp = f->GetByte();
    header->imageDecr = f->GetByte();
}

static void WriteTGAHeader(FILE* file, int width, int height, int depth)
{
    TargaHeader header;
    header.idLength = 0;
    header.cmapType = (depth==8) ? 1 : 0;
    header.imageType = (depth==8) ? TGA_UNCOMPRESSED_CMAP : TGA_UNCOMPRESSED_TRUECOLOUR;
    header.cmapStart = 0;
    header.cmapLength = (depth==8) ? 256 : 0;
    header.cmapFormat = (depth==8) ? 24 : 0;
    header.xOrigin = 0;
    header.yOrigin = 0;
    header.width = width;
    header.height = height;
    header.bpp = depth;
    header.imageDecr = 8;

    fwrite(&header.idLength,   sizeof(header.idLength), 1, file);   
    fwrite(&header.cmapType,   sizeof(header.cmapType), 1, file);
    fwrite(&header.imageType,  sizeof(header.imageType), 1, file);
    fwrite(&header.cmapStart,  sizeof(header.cmapStart), 1, file);
    fwrite(&header.cmapLength, sizeof(header.cmapLength), 1, file);
    fwrite(&header.cmapFormat, sizeof(header.cmapFormat), 1, file);
    fwrite(&header.xOrigin,    sizeof(header.xOrigin), 1, file);
    fwrite(&header.yOrigin,    sizeof(header.yOrigin), 1, file);
    fwrite(&header.width,      sizeof(header.width), 1, file);
    fwrite(&header.height,     sizeof(header.height), 1, file);
    fwrite(&header.bpp,        sizeof(header.bpp), 1, file);
    fwrite(&header.imageDecr,  sizeof(header.imageDecr), 1, file);
}

//-------------------------------------------------------------------
static void LoadTargaCMAP(TargaHeader& header, tFile* file, tImageHandler::Builder* builder)
{
    P3DASSERT(header.cmapLength <= 256);
    P3DASSERT((header.cmapFormat==24) || (header.cmapFormat==32));

    int nColours = header.cmapLength;
    pddiColour palette[256];
    if(nColours < 256)
    {
        memset(&palette[nColours], 0, (256-nColours)*4);
    }

    // get palette
    if(header.cmapFormat == 24)
    {
        for(int i=0; i < nColours; i++)
        {
            unsigned char b = file->GetByte();
            unsigned char g = file->GetByte();
            unsigned char r = file->GetByte();
            palette[i].Set(r, g, b);
        }
    }
    else
    {
        // 32 bit colour format
        file->GetData(palette, nColours * 4, tFile::BYTE);
        //file->GetData(palette, nColours, tFile::DWORD);
    }

    builder->BeginImage(header.width, header.height, 8, tImageHandler::Builder::BOTTOM, palette);
    unsigned char* scanline = (unsigned char*)p3d::MallocTemp(header.width);

    for(int i=0; i < header.height; i++)
    {
        file->GetData(scanline, header.width, tFile::BYTE);
        builder->ProcessScanline8(scanline);
    }
    
    p3d::FreeTemp(scanline);
    builder->EndImage();
}

static void LoadTargaTRUECOLOUR(TargaHeader& header, tFile* file, tImageHandler::Builder* builder)
{
    builder->BeginImage(header.width, header.height, header.bpp, tImageHandler::Builder::BOTTOM, NULL);
    P3D_U32* scanline = (P3D_U32*)p3d::MallocTemp(header.width * sizeof(P3D_U32));

    if(header.bpp == 32)
    {
        for(int i=0; i < header.height; i++)
        {
            file->GetData(scanline, header.width, tFile::DWORD);
            builder->ProcessScanline32((unsigned int*)scanline);
        }
    }
    else
    {
        // 24 bits per pixel (ugly)
        for(int i=0; i < header.height; i++)
        {
            int count = header.width;
            unsigned char* scan = (unsigned char*)scanline;
            do
            {
                scan[0] = file->GetByte();
                scan[1] = file->GetByte();
                scan[2] = file->GetByte();
                scan[3] = 0xff;
                scan += 4;
            } while(--count);
            builder->ProcessScanline32((unsigned int*)scanline);
        }
    }

    p3d::FreeTemp(scanline);
    builder->EndImage();
}

//-------------------------------------------------------------------
bool tTargaHandler::CheckFormat(Format f)
{
    return f == IMG_TGA;
}

void tTargaHandler::CreateImage(tFile* file, tImageHandler::Builder* builder)
{
    TargaHeader header;
    GetTGAHeader(file, &header);

    if(header.imageType != TGA_UNCOMPRESSED_CMAP &&
        header.imageType != TGA_UNCOMPRESSED_TRUECOLOUR)
    {
        char buf[256];
        sprintf(buf, "Attempted to load an unsuported TARGA format from file %s\n (Ignore will continue safely)", file->GetFilename());
        P3DASSERTMSG(0, buf, "tTargaHandler");
        return;
    }

    if(header.cmapType != 0 &&
        header.cmapType != 1)
    {
        char buf[256];
        sprintf(buf, "Attempted to load an unsuported TARGA format from file %s\n (Ignore will continue safely)", file->GetFilename());
        P3DASSERTMSG(0, buf, "tTargaHandler");
        return;
    }

    if(header.bpp != 32 &&
        header.bpp != 24 &&
        header.bpp != 8)
    {
        char buf[256];
        sprintf(buf, "Attempted to load an unsuported TARGA format from file %s\n (Ignore will continue safely)", file->GetFilename());
        P3DASSERTMSG(0, buf, "tTargaHandler");
        return;
    }

    if(header.width > 4096 ||
        header.height > 4096)
    {
        char buf[256];
        sprintf(buf, "Attempted to load an unsuported TARGA format from file %s\n (Ignore will continue safely)", file->GetFilename());
        P3DASSERTMSG(0, buf, "tTargaHandler");
        return;
    }

    // skip over image id field
    file->Advance(header.idLength);

    if(header.imageType == TGA_UNCOMPRESSED_CMAP)
    {
        LoadTargaCMAP(header, file, builder);
    }
    else
    if(header.imageType == TGA_UNCOMPRESSED_TRUECOLOUR)
    {
        LoadTargaTRUECOLOUR(header, file, builder);
    }
}

bool tTargaHandler::SaveImage(tImage* image, char* filename)
{
    FILE* file = fopen(filename, "wb");
    if(!file)
    {
        return false;
    }

    int width = image->GetWidth();
    int height = image->GetHeight();
    int depth = image->GetDepth();
 
    int nBytes = width*height*(depth / 8);

    WriteTGAHeader(file, width, height, depth);
    if(depth == 8)
    {
        unsigned char p24[256*3];
        unsigned char* p = p24;
        for(int i=0; i < 256; i++)
        {         
            tColour pe32(((tImage8*)image)->GetPaletteEntry(i));
            p[0] = pe32.Red();
            p[1] = pe32.Green();
            p[2] = pe32.Blue();
            p += 3;
        }
        // write palette
        fwrite(p24, 256*3, 1, file);
    }

    image->Invert();
    unsigned char* bits = image->GetColourChannel();
    int nWritten = fwrite(bits, nBytes, 1, file);
    fclose(file);
    image->Invert();
        
    return nWritten == 1;
}