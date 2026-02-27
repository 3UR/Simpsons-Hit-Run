//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

#include <pddi/gl/gl.hpp>
#include <pddi/gl/gldev.hpp>
#include <pddi/gl/gldisplay.hpp>
#include <pddi/gl/glcon.hpp>
#include <pddi/gl/gltex.hpp>
#include <pddi/gl/glmat.hpp>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// #include <io.h>
#include <pddi/base/debug.hpp>
#include <SDL2/SDL.h>

#define PDDI_GL_BUILD 36

static pglDevice gblDevice;

char libName [] = "OpenGL";

int pddiCreate(int versionMajor, int versionMinor, pddiDevice** device)
{
    if((versionMajor != PDDI_VERSION_MAJOR) || (versionMinor != PDDI_VERSION_MINOR))
    {
        *device = NULL;
        return PDDI_VERSION_ERROR;
    }

    *device = &gblDevice;
    return PDDI_OK;
}

//--------------------------------------------------------------
pglDevice::pglDevice() 
{
    nDisplays = 0;
    displayInfo = NULL;
}

//--------------------------------------------------------------
pglDevice::~pglDevice()
{
    for( int i = 0; i < nDisplays; i++ )
    {
        delete[] displayInfo[i].modeInfo;
    }
    delete[] displayInfo;
    displayInfo = NULL;
}

//--------------------------------------------------------------
void pglDevice::GetLibraryInfo(pddiLibInfo* info)
{
    info->versionMajor = PDDI_VERSION_MAJOR;
    info->versionMinor = PDDI_VERSION_MINOR;
    info->versionBuild = PDDI_GL_BUILD;
    info->libID = PDDI_LIBID_OPENGL;
    strcpy( info->description, libName );
}

unsigned pglDevice::GetCaps()
{
    return 0;
}

int pglDevice::GetDisplayInfo(pddiDisplayInfo** info)
{
    *info = displayInfo;

    if (displayInfo)
    {
        return nDisplays;
    }

    int totalDisplay = SDL_GetNumVideoDisplays();
    displayInfo = new pddiDisplayInfo[totalDisplay];

    nDisplays = 0;
    for(int i = 0; i < totalDisplay; i++)
    {
        const char* displayName = SDL_GetDisplayName(i);
        int totalModes = SDL_GetNumDisplayModes(i);
        if (!displayName || totalModes <= 0)
            continue;

        displayInfo[nDisplays].id = i;
        strcpy(displayInfo[0].description,SDL_GetDisplayName(i));
        displayInfo[nDisplays].pci = 0;
        displayInfo[nDisplays].vendor = 0;
        displayInfo[nDisplays].fullscreenOnly = false;
        displayInfo[nDisplays].caps = 0;

        displayInfo[nDisplays].modeInfo = new pddiModeInfo[totalModes];
        displayInfo[nDisplays].nDisplayModes = pglDisplay::FillDisplayModes(i, displayInfo[nDisplays].modeInfo);
        displayInfo[nDisplays].modeInfo = displayInfo[nDisplays].modeInfo;
        nDisplays++;
    }

    return nDisplays;
}

const char* pglDevice::GetDeviceDescription()
{
    return libName;
}

void pglDevice::SetCurrentContext(pddiRenderContext* c)
{
    context = c;
}

pddiRenderContext* pglDevice::GetCurrentContext(void)
{
    return context;
}

pddiDisplay *pglDevice::NewDisplay(int id)
{
    pddiDisplayInfo* dummy;
    GetDisplayInfo(&dummy);

    PDDIASSERT(id < nDisplays);
    pglDisplay* display = new pglDisplay(&displayInfo[id]);

    if(display->GetLastError() != PDDI_OK)
    {
        delete display;
        return NULL;
    }

    return (pddiDisplay *)display;
}
//--------------------------------------------------------------
pddiRenderContext *pglDevice::NewRenderContext(pddiDisplay* display)
{
    pglContext* context = new pglContext(this, (pglDisplay*)display);

    if(context->GetLastError() != PDDI_OK)
    {
        delete context;
        return NULL;
    }
    return context;
}

//--------------------------------------------------------------
pddiTexture* pglDevice::NewTexture(pddiTextureDesc* desc)
{
    pglTexture* tex = new pglTexture((pglContext*)context);
    if(!tex->Create(desc->GetSizeX(), desc->GetSizeY(), desc->GetBitDepth(), 
                     desc->GetAlphaDepth(), desc->GetMipMapCount(),desc->GetType(),desc->GetUsage()))
    {
        lastError = tex->GetLastError();
        delete tex;
        return NULL;
    }
    return tex;
}
//--------------------------------------------------------------
pddiShader *pglDevice::NewShader(const char* name, const char*) 
{ 
    pglMat* mat= new pglMat((pglContext*)context);
    if(mat->GetLastError() != PDDI_OK) {
        delete mat;
        return NULL;
    }

    return mat;
}

pddiPrimBuffer *pglDevice::NewPrimBuffer(pddiPrimBufferDesc* desc) 
{ 
    return new pglPrimBuffer((pglContext*)context, desc->GetPrimType(), desc->GetVertexFormat(), desc->GetVertexCount(), desc->GetIndexCount());;
}

void pglDevice::AddCustomShader(const char* name, const char* aux)
{
}

void pglDevice::Release(void)
{
}

