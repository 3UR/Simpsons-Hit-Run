/*===========================================================================
    bmp.cpp
    15-Feb-00 Created by Neall

    Copyright (c) 2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#include <p3d/image.hpp>
#include <p3d/bmp.hpp>
#include <p3d/file.hpp>
#include <p3d/error.hpp>
#include <p3d/utility.hpp>
#include <pddi/pdditype.hpp>

#include <string.h>
#include <stdio.h>

struct BMPHeader
{
    P3D_S32 size;
    P3D_U8  reserved1[2];
    P3D_U8  reserved2[2];
    P3D_S32 offset;
    P3D_S32 headerSize;
    P3D_S32 width;
    P3D_S32 height;
    P3D_S16 nPlanes;
    P3D_S16 bpp;
    P3D_S32 compression;
    P3D_S32 byteSize;
    P3D_S32 xPixelsPerMeter;
    P3D_S32 yPixelsPerMeter;
    P3D_S32 nColour;
    P3D_S32 nImportantColour;
};


static void LoadBMP4(tFile*, BMPHeader&, tImageHandler::Builder*);
static void LoadBMP8(tFile*, BMPHeader&, tImageHandler::Builder*);
static void LoadBMP24(tFile*, BMPHeader&, tImageHandler::Builder*);
static void FillHeader(tFile*, BMPHeader*);

bool tBMPHandler::CheckFormat(Format format)
{
    return format == IMG_BMP;
}

void tBMPHandler::CreateImage(tFile* file, tImageHandler::Builder* builder)
{
    char buf[256];
    char id[2];

    file->GetData(id, 2, tFile::BYTE);


    if(id[0] == 'B' && id[1] == 'M')
    {

    }
    else
    {
        sprintf(buf, "Attempted to load an unsuported BMP format from file %s\n (Ignore will continue safely)", file->GetFilename());
        P3DASSERTMSG(0, buf, "tBMPHandler");
        return;
    }

    BMPHeader header;
    FillHeader(file, &header);

    if(!( ((header.bpp == 1) || (header.bpp == 4) || (header.bpp == 8) || (header.bpp == 24)) && (header.compression == 0) ))
    {
        sprintf(buf, "Attempted to load an unsuported BMP format from file %s\n (Ignore will continue safely)", file->GetFilename());
        P3DASSERTMSG(0, buf, "tBMPHandler");
        return;
    }

    tImage* image = NULL;
    if(header.bpp == 4)
    {
        LoadBMP4(file, header, builder);
    }
    else
    if(header.bpp == 8)
    {
        LoadBMP8(file, header, builder);
    }
    else
    if(header.bpp == 24)
    {
        LoadBMP24(file, header, builder);
    }
}

bool tBMPHandler::SaveImage(tImage*, char*)
{
    return false;
}

void LoadBMP4(tFile* file, BMPHeader& header, tImageHandler::Builder* builder)
{
    // get palette
    pddiColour pal[16];
    memset(pal, 0, 16*sizeof(pddiColour));
    int nColour = header.nColour;
    if(nColour == 0) nColour = 16;
    P3DASSERT(nColour <= 16);   
    file->GetData(pal, nColour, tFile::DWORD);
    for(int i=0; i < nColour; i++)
    {
        // alpha is zero in palette, make it 255
        int* oldpal = (int*)pal;
        oldpal[i] = oldpal[i] | 0xff000000;
        // convert from platform independant to (possibly) platform dependant colour, does nothing on PC
        pal[i] = pddiColour(oldpal[i]);
    }
    tImage* image = NULL;

    builder->BeginImage(header.width, header.height, 4, tImageHandler::Builder::BOTTOM, (pddiColour*)pal);

    file->Advance(header.offset - file->GetPosition());

    int width = header.width;
    int pad = 0;
    if((header.width/2) & 3)
    {
        pad = 4 - ((header.width/2) & 3);
    }

    unsigned char* scanline_in  = (unsigned char*)p3d::MallocTemp(width/2);
    unsigned char* scanline_out = (unsigned char*)p3d::MallocTemp(width);;

    for(int y=0; y < header.height; y++)
    {
        file->GetData(scanline_in, width/2, tFile::BYTE);
      

        // promote 4-bit palette indexes to 8-bit palette indexes
        for (int i=0; i<width/2; i++)
        {
            scanline_out[(i*2)+0] = scanline_in[i] >> 4;
            scanline_out[(i*2)+1] = scanline_in[i] & 0x0f;
        }

        
        builder->ProcessScanline8(scanline_out);
        file->Advance(pad);
    }

    p3d::FreeTemp(scanline_in);
    p3d::FreeTemp(scanline_out);
    builder->EndImage();
}


void LoadBMP8(tFile* file, BMPHeader& header, tImageHandler::Builder* builder)
{
    // get palette
    pddiColour pal[256];
    memset(pal, 0, 256*sizeof(pddiColour));
    int nColour = header.nColour;
    if(nColour == 0) nColour = 256;
    P3DASSERT(nColour <= 256);   
    file->GetData(pal, nColour, tFile::DWORD);
    for(int i=0; i < nColour; i++)
    {
        // alpha is zero in palette, make it 255
        int* oldpal = (int*)pal;
        oldpal[i] = oldpal[i] | 0xff000000;
        // convert from platform independant to (possibly) platform dependant colour, does nothing on PC
        pal[i] = pddiColour(oldpal[i]);
    }
    tImage* image = NULL;

    builder->BeginImage(header.width, header.height, 8, tImageHandler::Builder::BOTTOM, (pddiColour*)pal);

    file->Advance(header.offset - file->GetPosition());

    int width = header.width;
    int pad = 0;
    if(header.width & 3)
    {
        pad = 4 - (header.width & 3);
    }

    unsigned char* scanline = (unsigned char*)p3d::MallocTemp(width);

    for(int y=0; y < header.height; y++)
    {
        file->GetData(scanline, width, tFile::BYTE);
        builder->ProcessScanline8(scanline);
        file->Advance(pad);
    }

    p3d::FreeTemp(scanline);
    builder->EndImage();
}

void LoadBMP24(tFile* file, BMPHeader& header, tImageHandler::Builder* builder)
{
    builder->BeginImage(header.width, header.height, 24, tImageHandler::Builder::BOTTOM, NULL);
    int width = header.width;
    int pad = 0;
    if((header.width*3) & 3)
    {
        pad = 4 - ((header.width*3) & 3);
    }

    P3D_U32* scanline = (P3D_U32*)p3d::MallocTemp(width * 4);
    
    for(int y=0; y < header.height; y++)
    {
        unsigned char* c = (unsigned char*)scanline;
        for(int x=0; x < width; x++)
        {
            c[0] = file->GetByte();
            c[1] = file->GetByte();
            c[2] = file->GetByte();
            c[3] = 0xff;
            c += 4;
        }
        file->Advance(pad);
        builder->ProcessScanline32((unsigned int*)scanline);
    }

    p3d::FreeTemp(scanline);
    builder->EndImage();
}

static void FillHeader(tFile* f, BMPHeader* h)
{
//   f->GetData(h, sizeof(BMPHeader), tFile::BYTE);
    h->size = f->GetDWord();
    h->reserved1[0] = f->GetByte();
    h->reserved1[1] = f->GetByte();
    h->reserved2[0] = f->GetByte();
    h->reserved2[1] = f->GetByte();
    h->offset = f->GetDWord();
    h->headerSize = f->GetDWord();
    h->width = f->GetDWord();
    h->height = f->GetDWord();
    h->nPlanes  = f->GetWord();
    h->bpp = f->GetWord();
    h->compression = f->GetDWord();
    h->byteSize = f->GetDWord();
    h->xPixelsPerMeter = f->GetDWord();
    h->yPixelsPerMeter = f->GetDWord();
    h->nColour = f->GetDWord();
    h->nImportantColour = f->GetDWord();
}

