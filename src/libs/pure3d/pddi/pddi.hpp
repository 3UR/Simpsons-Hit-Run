/*===========================================================================
    pddi.hpp

    Pure3D Device Driver Interface

    24-Apr-96 Created by NV
    28-Mar-97 release 2
    26-Aug-97 release 3
    08-Jan-98 release 4
    15-Apr-98 release 5
    03-Sep-99 release 6 for WIN32, Linux, PS2
    10-Jun-00 release 7
    01-Nov-00 release 8
    21-Jul-01 version bumped to 14.5
    22-Jan-02 version bumped to 16
    02-<ay-02 version bumbed to 17

    Copyright (c)1996-2002 Radical Entertainment Ltd
    All rights reserved.
===========================================================================*/
#ifndef _PDDI_HPP
#define _PDDI_HPP

// forward class declarations
class pddiBaseShader;
class pddiDevice;
class pddiRenderContext;
class pddiTexture;
class pddiShader;
class pddiPrimBuffer;
class pddiDisplay;
class pddiSurface;
class pddiBlendControl;
class pddiExtension;
struct pddiLockInfo;
struct SDL_Window;
struct SDL_WindowEvent;

#include <pddi/pdditype.hpp>
#include <pddi/pddienum.hpp>
#include <pddi/pddishade.hpp>

// version info
#define PDDI_VERSION_MAJOR    17
#define PDDI_VERSION_MINOR    2

// library IDs
const int PDDI_LIBID_STUB       = 0;
const int PDDI_LIBID_DIRECTDRAW = 1;
const int PDDI_LIBID_DIRECT3D   = 2;
const int PDDI_LIBID_OPENGL     = 3;
const int PDDI_LIBID_3DFXGLIDE  = 4;
const int PDDI_LIBID_WIN32GDI   = 5;
const int PDDI_LIBID_LINUX      = 6;
const int PDDI_LIBID_PS2        = 7;
const int PDDI_LIBID_MAC        = 8;
const int PDDI_LIBID_XBOX       = 10;
const int PDDI_LIBID_NEOGEO     = 11;
const int PDDI_LIBID_VECTREX    = 12;

// values returned by pddiObject::GetLastError()
// generic errors
#define PDDI_OK               1
#define PDDI_ERR              0

// initialization errors
#define PDDI_VERSION_ERROR    100

// texture errors
#define PDDI_TEX_NOT_POW_2    200
#define PDDI_TEX_TOO_BIG      201
#define PDDI_TEX_BADFORMAT    202

#if defined( RAD_WIN32 ) || defined ( RAD_UWP )
// code used for library initialisation on a PC

// video mode information
typedef struct
{
     int width;
     int height;
     int bpp;
} pddiModeInfo;

// display device information
class pddiDisplayInfo
{
public:
     int id;
     char description[256];
     unsigned pci;
     unsigned vendor;
     bool fullscreenOnly;
     unsigned caps;
     int nDisplayModes;
     pddiModeInfo* modeInfo;
};

#endif  // RAD_CONSOLE


struct pddiLibInfo
{
     int versionMajor;
     int versionMinor;
     int versionBuild;
     int libID;
     char description[256];
};


struct pddiVertexComponentWidth
{
    pddiVertexComponentWidth()
    {
        uv0 = uv1 = uv2 = uv3 = uv4 = uv5 = uv6 = uv7 = 2;
        position = 3;
    }

    unsigned char uv0;
    unsigned char uv1;
    unsigned char uv2;
    unsigned char uv3;
    unsigned char uv4;
    unsigned char uv5;
    unsigned char uv6;
    unsigned char uv7;
    unsigned char position;
    unsigned char pad[3];
};


class pddiPrimBufferDesc
{
public:
    pddiPrimBufferDesc(pddiPrimType type, unsigned format, unsigned vertexCount, unsigned indexCount = 0)
    {
        primType = type;
        vertexFormat = format;
        nVertex = vertexCount;
        nIndex = indexCount;
        memoryImaged = false;
        deformed = false;
        vertexProgram = NULL;
        matrixCount = 0;
    }
    
    void SetPrimType(pddiPrimType type)   { primType = type; }
    void SetVertexFormat(unsigned format) { vertexFormat = format; }
    void SetVertexCount(unsigned count)   { nVertex = count; }
    void SetIndexCount(unsigned count)    { nIndex = count; }
    void SetDeformed(bool d)              { deformed = d; }
    void SetMemoryImaged(bool mi)         { memoryImaged = mi; }
    void SetVertexProgram(char* program)  { vertexProgram = program; }
    void SetMatrixCount(unsigned count)   { matrixCount = count; }

    pddiPrimType GetPrimType(void)      { return primType; }
    unsigned GetVertexFormat(void)      { return vertexFormat; }
    unsigned GetVertexCount(void)       { return nVertex; }
    unsigned GetIndexCount(void)        { return nIndex; }
    bool GetDeformed(void)              { return deformed; }
    bool GetMemoryImaged(void)          { return memoryImaged; }
    const char* GetVertexProgram(void)  { return vertexProgram; }
    unsigned GetMatrixCount(void)       { return matrixCount; }

protected:
    pddiPrimType primType;
    unsigned vertexFormat;
    pddiVertexComponentWidth componentWidth;
    unsigned nVertex;
    unsigned nIndex;
    bool memoryImaged;
    bool deformed;
    char* vertexProgram;
    unsigned matrixCount;
};

class pddiTextureDesc
{
public:
    pddiTextureDesc()
    {
        xSize = ySize = zSize = 0;
        bpp = 0;
        alphaDepth = 0;
        nMip = 0;
        type = PDDI_TEXTYPE_RGB;
        usageHint = PDDI_USAGE_STATIC;
        volume = false;
        memImage = false;
    }

    void SetSizeX(unsigned s)  { xSize = s; }
    void SetSizeY(unsigned s)  { ySize = s; }
    void SetSizeZ(unsigned s)  { zSize = s; }
    void SetBitDepth(unsigned d) { bpp = d; }
    void SetAlphaDepth(unsigned d) { alphaDepth = d;}
    void SetMipMapCount(unsigned c) { nMip = c; }
    void SetType(pddiTextureType t) { type = t; }
    void SetUsage(pddiTextureUsageHint u) { usageHint = u;}
    void SetVolume(bool v) { volume = v;}
    void SetMemImage(bool m) { memImage = m;}

    unsigned GetSizeX(void)  { return xSize; }
    unsigned GetSizeY(void)  { return ySize; }
    unsigned GetSizeZ(void)  { return zSize; }
    unsigned GetBitDepth(void) { return bpp; }
    unsigned GetAlphaDepth(void) { return alphaDepth;}
    unsigned GetMipMapCount(void) { return nMip; }
    pddiTextureType GetType(void) { return type; }
    pddiTextureUsageHint GetUsage(void) { return usageHint;}
    bool GetVolume(void) { return volume;}
    bool GetMemImage(void) { return memImage; }

protected:
    int xSize;
    int ySize;
    int zSize;
    int bpp;
    int alphaDepth;
    int nMip;
    pddiTextureType type;
    pddiTextureUsageHint usageHint;
    bool volume;
    bool memImage;
};

//-------------------------------------------------------------------
// pddiDevice
//-------------------------------------------------------------------
class pddiDevice : public pddiObject
{
public:
#if defined( RAD_WIN32 ) || defined ( RAD_UWP )
    PDDI_INTERFACE int GetDisplayInfo(pddiDisplayInfo** info) PDDI_PURE;
#endif
    PDDI_INTERFACE void GetLibraryInfo(pddiLibInfo* info) PDDI_PURE;

    PDDI_INTERFACE void SetCurrentContext(pddiRenderContext* context) PDDI_PURE;
    PDDI_INTERFACE pddiRenderContext* GetCurrentContext() PDDI_PURE;

    PDDI_INTERFACE pddiDisplay* NewDisplay(int id) PDDI_PURE;
    PDDI_INTERFACE pddiRenderContext* NewRenderContext(pddiDisplay* display) PDDI_PURE;
    PDDI_INTERFACE pddiTexture* NewTexture(pddiTextureDesc*) PDDI_PURE;
    PDDI_INTERFACE pddiPrimBuffer* NewPrimBuffer(pddiPrimBufferDesc* desc) PDDI_PURE;
    PDDI_INTERFACE pddiShader* NewShader(const char* name, const char* aux = NULL) PDDI_PURE;

    PDDI_INTERFACE void AddCustomShader(const char* name, const char* aux) PDDI_PURE;

    class MessageCallback
    {
    public:
        virtual void Info(const char* message) = 0;
        virtual void Warn(const char* message) = 0;
        virtual bool Fatal(const char* message, const char* file, int line) = 0;
    };

    PDDI_INTERFACE void SetMessageCallback(MessageCallback*);

protected:
    virtual ~pddiDevice() {};
};

//-------------------------------------------------------------------
struct pddiLockInfo
{
     pddiLockInfo(): width(0), height(0), depth(0), volDepth(0), format(PDDI_PIXEL_UNKNOWN), native(0), pitch(0), slice(0), bits(0) 
                         { 
                             rgbaLShift[0] = rgbaLShift[1] = rgbaLShift[2] = rgbaLShift[3] = 0; 
                             rgbaRShift[0] = rgbaRShift[1] = rgbaRShift[2] = rgbaRShift[3] = 0; 
                             rgbaMask[0] = rgbaMask[1] = rgbaMask[2] = rgbaMask[3] = 0; 
                         }

    int width;
    int height;
    int depth;
    int volDepth;
    pddiPixelFormat format;
    bool native;   // pixel format is identical to pddiColour
    int rgbaLShift[4];
    int rgbaRShift[4];
    unsigned rgbaMask[4];
    int pitch;
    int slice;
    void* bits;
    void* palette;  // memory images only

    PDDI_U32 MakeColour(PDDI_U32 c)
    {
        return (((c&0x00ff0000)>>rgbaRShift[0])<<rgbaLShift[0]) |
                 (((c&0x0000ff00)>>rgbaRShift[1])<<rgbaLShift[1]) |
                 (((c&0x000000ff)>>rgbaRShift[2])<<rgbaLShift[2]) |
                 (((c&0xff000000)>>rgbaRShift[3])<<rgbaLShift[3]);
    }
};

//-------------------------------------------------------------------
// pddiDisplay
//-------------------------------------------------------------------
class pddiDisplay : public pddiObject
{
public:

#if defined( RAD_WIN32 ) || defined ( RAD_UWP )
   PDDI_INTERFACE long ProcessWindowMessage(SDL_Window* win, const SDL_WindowEvent* event) PDDI_PURE;
   PDDI_INTERFACE void SetWindow(SDL_Window* win) PDDI_PURE;
   PDDI_INTERFACE pddiDisplayInfo* GetDisplayInfo(void) PDDI_PURE;
#endif
   PDDI_INTERFACE bool InitDisplay(int x, int y, int bpp) PDDI_PURE;
   PDDI_INTERFACE bool InitDisplay(const pddiDisplayInit* initData) PDDI_PURE;

   PDDI_INTERFACE int GetHeight() PDDI_PURE;
   PDDI_INTERFACE int GetWidth() PDDI_PURE;
   PDDI_INTERFACE int GetDepth() PDDI_PURE;
   PDDI_INTERFACE pddiDisplayMode GetDisplayMode() PDDI_PURE;
   PDDI_INTERFACE int GetNumColourBuffer() PDDI_PURE;
   PDDI_INTERFACE unsigned GetBufferMask() PDDI_PURE;

   PDDI_INTERFACE unsigned GetFreeTextureMem() PDDI_PURE;

   PDDI_INTERFACE void SwapBuffers(void) PDDI_PURE;

   // amb nov29/2001 : return the current front buffer
   PDDI_INTERFACE unsigned Screenshot(pddiColour* buffer, int nBytes) PDDI_PURE;

   PDDI_INTERFACE bool IsWidescreen(void) {return GetWidth() * 3 > GetHeight() * 4; };

   PDDI_INTERFACE void SetForceVSync(bool IsForceVSync, bool only60 = false) {m_ForceVSync = IsForceVSync; m_only60 = only60;}
   PDDI_INTERFACE bool GetForceVSync(void) {return m_ForceVSync;}

protected:
   pddiDisplay() { m_ForceVSync = false; m_only60 = false; }
   virtual ~pddiDisplay() {};
   bool m_ForceVSync;
   bool m_only60;
};


//-------------------------------------------------------------------
// pddiPrimBuffer - retained mode rendering
//-------------------------------------------------------------------
class pddiPrimBufferStream
{
public:
    void Coord(float x, float y, float z) { Position(x, y, z); }
    PDDI_INTERFACE void Position(float x, float y, float z) PDDI_PURE;
    PDDI_INTERFACE void Normal(float x, float y, float z) PDDI_PURE;
    PDDI_INTERFACE void Binormal(float x, float y, float z) { };
    PDDI_INTERFACE void Tangent(float x, float y, float z) { };
    PDDI_INTERFACE void Colour(pddiColour colour, int channel = 0) PDDI_PURE;
    void UV(float u, float v, int channel = 0) { TexCoord2(u, v, channel); }
    PDDI_INTERFACE void TexCoord1(float s, int channel = 0) PDDI_PURE;
    PDDI_INTERFACE void TexCoord2(float s, float t, int channel = 0) PDDI_PURE;
    PDDI_INTERFACE void TexCoord3(float s, float t, float u, int channel = 0) PDDI_PURE;
    PDDI_INTERFACE void TexCoord4(float s, float t, float u, float v, int channel = 0) PDDI_PURE;

    PDDI_INTERFACE void Specular(pddiColour colour) PDDI_PURE;
    PDDI_INTERFACE void SkinIndices(unsigned, unsigned = 0, unsigned = 0, unsigned = 0) PDDI_PURE;
    PDDI_INTERFACE void SkinWeights(float, float = 0.0f, float = 0.0f) PDDI_PURE;

    PDDI_INTERFACE void Vertex(rmt::Vector* v, pddiColour c) PDDI_PURE;
    PDDI_INTERFACE void Vertex(rmt::Vector* v, rmt::Vector* n) PDDI_PURE;
    PDDI_INTERFACE void Vertex(rmt::Vector* v, rmt::Vector2* uv) PDDI_PURE;
    PDDI_INTERFACE void Vertex(rmt::Vector* v, pddiColour c, rmt::Vector2* uv) PDDI_PURE;
    PDDI_INTERFACE void Vertex(rmt::Vector* v, rmt::Vector* n, rmt::Vector2* uv) PDDI_PURE;

    PDDI_INTERFACE void Next() PDDI_PURE;
};

class pddiPrimBuffer : public pddiObject
{
public:
    PDDI_INTERFACE pddiPrimBufferStream* Lock() PDDI_PURE;
    PDDI_INTERFACE void Unlock(pddiPrimBufferStream* stream) PDDI_PURE;
    PDDI_INTERFACE void Finalize() { /**/ }

    PDDI_INTERFACE unsigned char* LockIndexBuffer() PDDI_PURE;
    PDDI_INTERFACE void UnlockIndexBuffer(int count) PDDI_PURE;

    PDDI_INTERFACE void SetIndices(unsigned short* indices, int count) PDDI_PURE;

    PDDI_INTERFACE bool CheckMemImageVersion(int version) PDDI_PURE;
    PDDI_INTERFACE void* LockMemImage(unsigned memImageLength) PDDI_PURE;
    PDDI_INTERFACE void UnlockMemImage() PDDI_PURE;
    PDDI_INTERFACE void SetMemImageParam(unsigned param, unsigned value) { /**/ }
                                          
    PDDI_INTERFACE void SetUsedSize(int size) {};

protected:
    virtual ~pddiPrimBuffer() {};
};

//-------------------------------------------------------------------
// pddiLightDesc - light description structure
//-------------------------------------------------------------------
class pddiLightDesc
{
public:
    inline void Clear()
    {
        enabled = false;
        type = PDDI_LIGHT_DIRECTIONAL;
        colour.Set(255,255,255);
        position.Set(0,0,0);
        direction.Set(0,0,1.0f);
        attenuationA = 1.0f;
        attenuationB = 0.0f;
        attenuationC = 0.0f;
        phi = 0.0f;
        theta = 0.0f;
        falloff = 0.0f;
    }
    
    pddiLightDesc() { Clear(); }
    pddiLightDesc(bool e) { Clear(); enabled = e; }

    inline void Enable(bool e)                { enabled = e; }
    inline void SetType(pddiLightType t)      { type = t; }
    inline void SetColour(pddiColour c)       { colour = c; }
    inline void SetPosition(rmt::Vector* pos)  { position = *pos; }
    inline void SetDirection(rmt::Vector* dir) { direction = *dir; }
    inline void SetAttenuation(float a, float b, float c) { attenuationA = a; attenuationB = b; attenuationC = c; }
    inline void SetCone(float p, float t, float f)     { phi = p; theta = t; falloff = f; }

    inline void SetDirectionalLight(pddiColour c, rmt::Vector* d)
    {
        type = PDDI_LIGHT_DIRECTIONAL;
        colour = c;
        direction = *d;
    }
    
    inline void SetPointLight(pddiColour c, rmt::Vector* p, float attenA = 1.0f, float attenB = 0.0f, float attenC = 0.0f)
    {
        type = PDDI_LIGHT_POINT;
        colour = c;
        position = *p;
        attenuationA = attenA;
        attenuationB = attenB;
        attenuationC = attenC;
    }
    
    inline void SetSpotLight(pddiColour c, rmt::Vector* pos, rmt::Vector* dir, float ph, float th, float fall, float attenA = 1.0f, float attenB = 0.0f, float attenC = 0.0f)
    {
        type = PDDI_LIGHT_SPOT;
        colour = c;
        position = *pos;
        direction = *dir;
        phi = ph;
        theta = th;
        falloff = fall;
        attenuationA = attenA;
        attenuationB = attenB;
        attenuationC = attenC;
    }
    
    inline const pddiLightType& GetType()   const { return type; }
    inline const pddiColour& GetColour()    const { return colour; }
    inline const rmt::Vector& GetPosition()  const { return position; }
    inline const rmt::Vector& GetDirection() const { return direction; }
    inline void GetAttenuation(float *a, float *b, float *c) const { *a = attenuationA; *b = attenuationB; *c = attenuationC; }
    inline void GetCone(float *p, float *t, float *f) const  { *p = phi; *t = theta; *f = falloff; }

private:
    friend class pddiBaseContext;

    bool enabled;
    pddiLightType type;
    pddiColour colour;
    rmt::Vector position;
    rmt::Vector direction;
    float attenuationA;
    float attenuationB;
    float attenuationC;
    float phi;
    float theta;
    float falloff;
};

//-------------------------------------------------------------------
// pddiRenderContext
//-------------------------------------------------------------------
class pddiRenderContext : public pddiObject
{
public:
    // frame synchronisation
    PDDI_INTERFACE void BeginFrame() PDDI_PURE;
    PDDI_INTERFACE void EndFrame() PDDI_PURE;

    // buffer clearing
    PDDI_INTERFACE void SetClearColour(pddiColour colour) PDDI_PURE;
    PDDI_INTERFACE pddiColour GetClearColour(void) PDDI_PURE;
    PDDI_INTERFACE void SetClearDepth(float depth) PDDI_PURE;
    PDDI_INTERFACE float GetClearDepth(void) PDDI_PURE;
    PDDI_INTERFACE void SetClearStencil(unsigned stencil) PDDI_PURE;
    PDDI_INTERFACE unsigned GetClearStencil(void) PDDI_PURE;
    PDDI_INTERFACE void Clear(unsigned bufferMask) PDDI_PURE;

    // matrix stack
    PDDI_INTERFACE void IdentityMatrix(pddiMatrixType id) PDDI_PURE;
    PDDI_INTERFACE void LoadMatrix(pddiMatrixType id, rmt::Matrix* matrix) PDDI_PURE;
    PDDI_INTERFACE void PushMatrix(pddiMatrixType id) PDDI_PURE;
    PDDI_INTERFACE void PopMatrix(pddiMatrixType id) PDDI_PURE;
    PDDI_INTERFACE void MultMatrix(pddiMatrixType id, rmt::Matrix* matrix) PDDI_PURE;
    PDDI_INTERFACE rmt::Matrix* GetMatrix(pddiMatrixType id) PDDI_PURE;

    // shortcuts:  push + operation to top-of-stack
    PDDI_INTERFACE void PushIdentityMatrix(pddiMatrixType id) PDDI_PURE;
    PDDI_INTERFACE void PushLoadMatrix(pddiMatrixType id, rmt::Matrix* matrix) PDDI_PURE;
    PDDI_INTERFACE void PushMultMatrix(pddiMatrixType id, rmt::Matrix* matrix) PDDI_PURE;

    // camera, view
    PDDI_INTERFACE void SetProjectionMode(pddiProjectionMode mode) PDDI_PURE;
    PDDI_INTERFACE pddiProjectionMode GetProjectionMode() PDDI_PURE;

    PDDI_INTERFACE void SetViewWindow(float left, float top, float right, float bottom) PDDI_PURE;
    PDDI_INTERFACE void GetViewWindow(float* left, float* top, float* right, float* bottom) PDDI_PURE;

    PDDI_INTERFACE void SetCamera(float nearPlane, float farPlane, float fov, float aspect) PDDI_PURE;
    PDDI_INTERFACE void GetCamera(float* nearPlane, float* farPlane, float* fov, float* aspect) PDDI_PURE;

    // viewport clipping
    PDDI_INTERFACE void SetScissor(pddiRect* rect) PDDI_PURE;
    PDDI_INTERFACE void GetScissor(pddiRect* rect) PDDI_PURE;

    // immediate mode prim rendering
    PDDI_INTERFACE pddiPrimStream* BeginPrims(pddiShader* material, pddiPrimType primType, unsigned vertexType, int vertexCount, unsigned pass = 0) PDDI_PURE;
    PDDI_INTERFACE void EndPrims(pddiPrimStream* stream) PDDI_PURE;

    // retained mode prim rendering
    PDDI_INTERFACE void DrawPrimBuffer(pddiShader* material, pddiPrimBuffer* buffer) PDDI_PURE;

    // debug text
    PDDI_INTERFACE void DrawString(const char* s, int x, int y, pddiColour colour = pddiColour(255,255,255)) PDDI_PURE;

    // lighting
    PDDI_INTERFACE int GetMaxLights() PDDI_PURE;

    PDDI_INTERFACE void SetAmbientLight(pddiColour col) PDDI_PURE;
    PDDI_INTERFACE pddiColour GetAmbientLight() PDDI_PURE;

    PDDI_INTERFACE void SetLight(int handle, pddiLightDesc* desc) PDDI_PURE;
    
    PDDI_INTERFACE void EnableLight(int handle, bool active) PDDI_PURE;
    PDDI_INTERFACE bool IsLightEnabled(int handle) PDDI_PURE;
    PDDI_INTERFACE void SetLightType(int handle, pddiLightType type) PDDI_PURE;
    PDDI_INTERFACE pddiLightType GetLightType(int handle) PDDI_PURE;

    PDDI_INTERFACE void SetLightColour(int handle, pddiColour colour) PDDI_PURE;
    PDDI_INTERFACE pddiColour GetLightColour(int handle) PDDI_PURE;

    PDDI_INTERFACE void SetLightPosition(int handle, rmt::Vector* dir) PDDI_PURE;
    PDDI_INTERFACE void GetLightPosition(int handle, rmt::Vector* dir) PDDI_PURE;

    PDDI_INTERFACE void SetLightDirection(int handle, rmt::Vector* dir) PDDI_PURE;
    PDDI_INTERFACE void GetLightDirection(int handle, rmt::Vector* dir) PDDI_PURE;

    PDDI_INTERFACE void SetLightRange(int handle, float range) PDDI_PURE;
    PDDI_INTERFACE float GetLightRange(int handle) PDDI_PURE;

    PDDI_INTERFACE void SetLightAttenuation(int handle, float a, float b, float c) PDDI_PURE;
    PDDI_INTERFACE void GetLightAttenuation(int handle, float* a, float* b, float* c) PDDI_PURE;

    PDDI_INTERFACE void SetLightCone(int handle, float phi, float theta, float falloff) PDDI_PURE;
    PDDI_INTERFACE void GetLightCone(int handle, float* phi, float* theta, float* falloff) PDDI_PURE;

    // backface culling 
    PDDI_INTERFACE void SetCullMode(pddiCullMode mode) PDDI_PURE;
    PDDI_INTERFACE pddiCullMode GetCullMode() PDDI_PURE;

    // colour buffer control
    PDDI_INTERFACE void SetColourWrite(bool red, bool green, bool blue, bool alpha) PDDI_PURE;
    PDDI_INTERFACE void GetColourWrite(bool* red, bool* green, bool* blue, bool* alpha) PDDI_PURE;

    // z-buffer control
    PDDI_INTERFACE void EnableZBuffer(bool enable) PDDI_PURE;
    PDDI_INTERFACE bool IsZBufferEnabled() PDDI_PURE;

    PDDI_INTERFACE void SetZCompare(pddiCompareMode compareMode) PDDI_PURE;
    PDDI_INTERFACE pddiCompareMode GetZCompare() PDDI_PURE;

    PDDI_INTERFACE void SetZWrite(bool) PDDI_PURE;
    PDDI_INTERFACE bool GetZWrite() PDDI_PURE;

    PDDI_INTERFACE void SetZBias(float bias) PDDI_PURE;
    PDDI_INTERFACE float GetZBias() PDDI_PURE;

    PDDI_INTERFACE void SetZRange(float n, float f) PDDI_PURE;
    PDDI_INTERFACE void GetZRange(float* n, float* f) PDDI_PURE;

    // stencil buffer control
    PDDI_INTERFACE void EnableStencilBuffer(bool enable) PDDI_PURE;
    PDDI_INTERFACE bool IsStencilBufferEnabled() PDDI_PURE;

    PDDI_INTERFACE void SetStencilCompare(pddiCompareMode compare) PDDI_PURE;
    PDDI_INTERFACE pddiCompareMode GetStencilCompare() PDDI_PURE;

    PDDI_INTERFACE void SetStencilRef(int ref) PDDI_PURE;
    PDDI_INTERFACE int GetStencilRef() PDDI_PURE;

    PDDI_INTERFACE void SetStencilMask(unsigned mask) PDDI_PURE;
    PDDI_INTERFACE unsigned GetStencilMask() PDDI_PURE;

    PDDI_INTERFACE void SetStencilWriteMask(unsigned mask) PDDI_PURE;
    PDDI_INTERFACE unsigned GetStencilWriteMask() PDDI_PURE;

    PDDI_INTERFACE void SetStencilOp(pddiStencilOp failOp, pddiStencilOp zFailOp, pddiStencilOp zPassOp) PDDI_PURE;
    PDDI_INTERFACE void GetStencilOp(pddiStencilOp* failOp, pddiStencilOp* zFailOp, pddiStencilOp* zPassOp) PDDI_PURE;

    // polygon fill
    PDDI_INTERFACE void SetFillMode(pddiFillMode mode) PDDI_PURE;
    PDDI_INTERFACE pddiFillMode GetFillMode() PDDI_PURE;

    // fog
    PDDI_INTERFACE void EnableFog(bool enable) PDDI_PURE;
    PDDI_INTERFACE bool IsFogEnabled() PDDI_PURE;

    PDDI_INTERFACE void SetFog(pddiColour colour, float start, float end) PDDI_PURE;
    PDDI_INTERFACE void GetFog(pddiColour* colour, float* start, float* end) PDDI_PURE;

    // utility
    PDDI_INTERFACE int GetIntStat(pddiStatType stat) PDDI_PURE;
    PDDI_INTERFACE float GetFloatStat(pddiStatType stat) PDDI_PURE;

    PDDI_INTERFACE int GetMaxTextureDimension(void) PDDI_PURE;
    PDDI_INTERFACE void EnableStatsOverlay(bool) PDDI_PURE;
    PDDI_INTERFACE bool IsStatsOverlayEnabled() PDDI_PURE;

    // state preservation, restoration
    PDDI_INTERFACE void PushState(pddiStateMask mask) PDDI_PURE;
    PDDI_INTERFACE void PopState(pddiStateMask mask) PDDI_PURE;

    // extensions
    PDDI_INTERFACE pddiExtension* GetExtension(unsigned extID) PDDI_PURE;
    PDDI_INTERFACE bool VerifyExtension(unsigned extID) PDDI_PURE;

    // forces drawing to complete
    PDDI_INTERFACE void DrawSync() PDDI_PURE;

    // amb may7/2001 render to texture
    PDDI_INTERFACE unsigned SetRenderTarget(pddiTexture* tex) PDDI_PURE;

protected:
    virtual ~pddiRenderContext() {};
};


//-------------------------------------------------------------------
// pddiTexture
//-------------------------------------------------------------------
class pddiTexture : public pddiObject
{
public:
    PDDI_INTERFACE pddiPixelFormat GetPixelFormat() PDDI_PURE;
    PDDI_INTERFACE int GetWidth() PDDI_PURE;
    PDDI_INTERFACE int GetHeight() PDDI_PURE;
    PDDI_INTERFACE int GetDepth() PDDI_PURE;
    PDDI_INTERFACE int GetAlphaDepth() PDDI_PURE;
    PDDI_INTERFACE int GetNumMipMaps() PDDI_PURE;

    // palette managment
    PDDI_INTERFACE int GetNumPaletteEntries(void) PDDI_PURE;
    PDDI_INTERFACE void SetPalette(int nEntries, pddiColour* palette) PDDI_PURE;
    PDDI_INTERFACE int GetPalette(pddiColour* palette) PDDI_PURE;

    // texture upload
    PDDI_INTERFACE pddiLockInfo* Lock(int mipLevel, pddiRect* rect = 0) PDDI_PURE;
    PDDI_INTERFACE void Unlock(int mipLevel) PDDI_PURE;
    PDDI_INTERFACE void SetCompressedData( int mipLevel, const char* const data, int len ){};

    // paging control
    PDDI_INTERFACE void Prefetch() PDDI_PURE;
    PDDI_INTERFACE void Discard() PDDI_PURE;
    PDDI_INTERFACE void SetPriority(int priority) PDDI_PURE;
    PDDI_INTERFACE int GetPriority() PDDI_PURE;

protected:
     virtual ~pddiTexture(){};
};


//-------------------------------------------------------------------
// pddiShader
//-------------------------------------------------------------------
class pddiShader : public pddiObject
{
public:
    PDDI_INTERFACE const char* GetType(void) PDDI_PURE;

    PDDI_INTERFACE bool SetTexture(unsigned paramName, pddiTexture* tex) PDDI_PURE;
    PDDI_INTERFACE bool SetInt(unsigned paramName, int) PDDI_PURE;
    PDDI_INTERFACE bool SetFloat(unsigned paramName, float) PDDI_PURE;
    PDDI_INTERFACE bool SetColour(unsigned paramName, pddiColour) PDDI_PURE;
    PDDI_INTERFACE bool SetVector(unsigned paramName, const rmt::Vector&) PDDI_PURE;
    PDDI_INTERFACE bool SetMatrix(unsigned paramName, const rmt::Matrix&) PDDI_PURE;
    virtual pddiBaseShader* CloneSimple(){return NULL;};

protected:
    virtual ~pddiShader() { /**/ }
};

#endif /* _PDDI_HPP */