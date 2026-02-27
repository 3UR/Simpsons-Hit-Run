//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _GLDEV_HPP_
#define _GLDEV_HPP_
                
#include <pddi/pddi.hpp>

class pglWrapper;

class pglDevice : public pddiDevice
{
public:
    pglDevice();
    ~pglDevice();

    void        GetLibraryInfo(pddiLibInfo* info);
    const char* GetDeviceDescription();
    int         GetDisplayInfo(pddiDisplayInfo** info);
    unsigned    GetCaps();

    void SetCurrentContext(pddiRenderContext* context);
    pddiRenderContext* GetCurrentContext();

    pddiDisplay* NewDisplay(int id);
    pddiRenderContext* NewRenderContext(pddiDisplay* display);
    pddiTexture* NewTexture(pddiTextureDesc* desc);
    pddiPrimBuffer* NewPrimBuffer(pddiPrimBufferDesc* desc);
    pddiShader* NewShader(const char* name, const char* aux = NULL);
    
    void AddCustomShader(const char* name, const char* aux = NULL);

    void Release(void);

protected:
    bool initialized;
    pddiRenderContext* context;

    int nDisplays;
    pddiDisplayInfo* displayInfo;
};
#endif

