//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _PDDIEXT_HPP
#define _PDDIEXT_HPP

#include <pddi/pddi.hpp>

enum pddiBuffer
{
    PDDI_BUFFER_FRONT,
    PDDI_BUFFER_BACK
};


// base class for extensions, empty 
class pddiExtension
{
    protected:
        virtual ~pddiExtension() { }
};

//--------------------------------------------------------------------
// 0x00000000 - 0x000100000
// Common to all (or most) PDDI libraries
//--------------------------------------------------------------------

// extension for tracking memory allocated outside the ordinary process (e.g., by Direct3D or memalign)
const unsigned PDDI_EXT_MEM_REGISTRATION = 0x00000100;
class pddiExtMemRegistration : public pddiExtension
{
public:
    class CallBack
    {
        public:
            virtual void Alloc(void* mem, int estSize) = 0;
            virtual void Free(void* mem) = 0;
    };

    virtual void Register(CallBack*) = 0;

protected:
    virtual ~pddiExtMemRegistration() {};
};

// control the gamma correction curve of a the display (DX8/OpenGL/XBox)
const unsigned PDDI_EXT_GAMMACONTROL = 0x00000101;

class pddiExtGammaControl : public pddiExtension
{
public:
    virtual void SetGamma(float r, float g, float b) = 0;
    virtual void GetGamma(float *r, float *g, float *b) = 0;

protected:
    virtual ~pddiExtGammaControl() {};
};

const unsigned PDDI_EXT_HARDWARE_SKINNING = 0x00000102;

class pddiExtHardwareSkinning : public pddiExtension
{
public:
    virtual int MaxMatrixPaletteSize(unsigned weightcount) = 0;

    virtual void Begin(void) = 0;
    virtual void End(void) = 0;

    virtual void SetMatrixCount(unsigned count) = 0;
    virtual void SetMatrix(unsigned index, pddiMatrix* m) = 0;
    virtual void DrawSkin(pddiShader* shader, pddiPrimBuffer* skin) = 0;

protected:
    virtual ~pddiExtHardwareSkinning() {};
};



const unsigned PDDI_EXT_FRAMEBUFFER_EFFECTS = 0x00000103;

class pddiExtFramebufferEffects : public pddiExtension
{
public:
    virtual void EnableMotionBlur(bool enable, float alpha, float zoom, bool rgb ) = 0;
    virtual void RenderMotionBlur(void) = 0;
    

    enum Quality {Fastest, Smallest, Best};    

    virtual void SetQuality(Quality q) = 0;

protected:
    virtual ~pddiExtFramebufferEffects() {};
    
};


const unsigned PDDI_EXT_FRAMEBUFFER_TEXTURE = 0x00000104;

class pddiExtFrameBufferTexture:  public pddiExtension
{
public:
    virtual pddiTexture * CreateTexture( pddiBuffer source ) = 0;
    virtual void ReleaseTexture(  ) = 0;

protected:
    virtual ~pddiExtFrameBufferTexture( ) { };
};

const unsigned PDDI_EXT_FOCUSBLUR_EFFECT = 0x00000105;

class pddiExtFocusBlurEffect: public pddiExtension
{
public:
	enum FilterMode{
		FM_BOX,
		FM_Y4,
		FM_X4
	};

    enum EffectMode{
        EM_NONE,
        EM_FOCUS,
        EM_BLUR
    };
    virtual void BeginFocusBlur( ) = 0;
    virtual void EndFocusBlur( ) = 0;
    virtual void DrawEffect( FilterMode m = FM_BOX ) = 0;
    virtual void SetFocusRange( float n, float f ) = 0;
    virtual void SetEffectMode( EffectMode m ) = 0;

protected:
    virtual ~pddiExtFocusBlurEffect( ){ };
};

//--------------------------------------------------------------------
// 0x00200000 - 0x000300000
// Playstation 2
//--------------------------------------------------------------------
const unsigned PDDI_EXT_PS2_CONTROL = 0x00200100;

enum ps2DestAlphaTestMode
{
    PS2_DESTALPHA_NONE,
    PS2_DESTALPHA_PASS_0,
    PS2_DESTALPHA_PASS_1
};

enum ps2VramView
{
    PS2_FRONT_BUFFER = 0,
    PS2_DRAW_BUFFER,
    PS2_ZBUFFER ,
    PS2_TEXTURE_MEMORY
};

class pddiExtPS2Control : public pddiExtension
{
public:
    // Reloads PDDI VU1 microcode program
    virtual void LoadMicrocode() = 0;

    // returns the next free microcode address in the VU1 after
    virtual unsigned GetFreeMicrocodeAddress() = 0;

    // tells PDDI to finish up whatever it's working on so the scratchpad
    // can be used
    virtual void SyncScratchPad() = 0;

    // turn near plane clipping on/off
    virtual void EnableClipper(bool enable) = 0;

    virtual void SetDestAlphaTest(ps2DestAlphaTestMode mode) = 0;

    // this function clears ALL 4mb of vram to [rgba]
    virtual void ClearVRAM(unsigned char r, unsigned char g, unsigned char b, unsigned char a) = 0;

    //
    virtual void EnableFSAA(int flags) = 0;

    // new stencil shadow code   
    virtual int BeginStencilPass(int flags) = 0;
    virtual int EndStencilPass(int flags) = 0;

    // nov7/2001 amb - call this function after rendering all your opaque objects
    // in your scene. NOTE - this will flush most of your textures
    virtual int HardwareZbufferFog( int r, int g, int b, int a, float znear, float zfar ) = 0;

    // nov19/2001 amb - changes the frame display pointer to texture ram or z buffer
    virtual int DisplayVram(ps2VramView what) = 0;

    // nov21/2001 amb - forces mfifo to sync every time a packet is submitted
    // warning : this is slooooww!
    virtual int ForceMFIFOSync( bool force ) = 0;

    // feb14/2002 amb : not implemented yet
    virtual int SetShadowColour( int r, int g, int b, int a ) = 0;

    // feb14/2002 amb
    virtual int SwirlyEffect(float t, float du, float dv, int flags) = 0;

    // mar13/2002 amb
    virtual int DisableTexCache( bool disable ) = 0;

    // apr17/2002 amb
    virtual int InvalidateTexCache() = 0;

    // access to the vu1 code manager
    // use these to install user programs
    virtual int AddVU1Program(const char* name, unsigned addr, int codePath, bool replicate) = 0;
    virtual unsigned SetVU1CodePath(int path) = 0;
    virtual unsigned GetVU1ProgramAddress(int handle) = 0;
    virtual unsigned GetVU1ProgramAddress(const char* name) = 0;
    
    // get pointer to backbuffer in vram so we can ram data, primarily movies, into it
    virtual unsigned GetBackBufferVRAM(void) = 0;

    // jul31/2002 amb - this disables all texture uploads
    virtual int EnableTextures(bool enable) = 0;

    // SIMPSONS 2: used to shut down MFIFO for error display
    virtual void MFIFOEnable( bool enableFlag ) = 0;

    // may1/2003 amb - used for occlusion flares
    virtual int TransformToScreen( const pddiVector* in, pddiVector* out ) = 0;

    // may1/2003 amb
    virtual int ClearDestAlpha(const pddiRect* rc) = 0;
    virtual int SampleDestAlphaAndFill(const pddiRect* rc) = 0; // in screen coordinates

    // jan30/2003 amb
    /// adjust display brightness
    virtual int SetBrightness(pddiColour) = 0;

protected:
    virtual ~pddiExtPS2Control() {};
};

//--------------------------------------------------------------------
// 0x00300000 - 0x000400000
// OpenGL specific
//--------------------------------------------------------------------
const unsigned PDDI_EXT_GL_CONTEXT = 0x00300100;

struct IRadThreadMutex;
class pddiExtGLContext : public pddiExtension
{
public:
    virtual void BeginContext() = 0;
    virtual void EndContext() = 0;

protected:
    virtual ~pddiExtGLContext() {};
};


//--------------------------------------------------------------------
// 0x00400000 - 0x000500000
// DX8 specific
//--------------------------------------------------------------------


//--------------------------------------------------------------------
// 0x00500000 - 0x000600000
// XBox specific
//--------------------------------------------------------------------

const unsigned PDDI_EXT_ANTIALIAS_CONTROL = 0x00500001;

class pddiExtAntialiasControl : public pddiExtension
{
public:
    virtual void EnableMultiSampling(bool) = 0;
    virtual void EnablePolyClear(bool) = 0;
    virtual void SetWriteMask(unsigned) = 0;
    virtual int GetAntiAliasMode(void) = 0;

protected:
    virtual ~pddiExtAntialiasControl() {};
};


const unsigned PDDI_EXT_VISIBILITY_TEST = 0x00500002;

class pddiExtVisibilityTest: public pddiExtension
{
public:
    virtual unsigned MaxIndex(void) = 0;

    virtual void Begin(void) = 0;
    virtual void End(unsigned) = 0;
    virtual unsigned Result(unsigned) = 0;
    virtual bool CurrentlyTestingVisiblity(void) = 0;

protected:
    virtual ~pddiExtVisibilityTest() {};
};

const unsigned PDDI_EXT_VERTEX_PROGRAM = 0x00500003;

class pddiExtVertexProgram: public pddiExtension
{
public:
    enum VertexFlags
    {
        COMPRESSED = 0x1 << 0
    };

    struct ShaderLightingInfo
    {
        pddiColour diffuse;
        pddiColour specular;
        pddiColour ambient;
        pddiColour emissive;

        float shininess;
    };

    virtual void     AddVertexProgram(const char* name, void* tokens) = 0;
    virtual unsigned GetVertexProgram(const char* name, pddiPrimType primType, unsigned mask, unsigned aux) = 0;
    virtual void     SetBufferProgram(pddiPrimBuffer* buf, unsigned program) = 0;
    virtual void     SetStreamProgram(unsigned program) = 0;

    virtual void     SetConstant(int index, float* data, int n = 1) = 0;
    virtual void     GetShaderLightingInfo(pddiShader*, ShaderLightingInfo*) = 0;

protected:
    virtual ~pddiExtVertexProgram() {};
};

// global projected textures for CSPR shader
const unsigned PDDI_EXT_GLOBAL_PROJECTED_TEXTURE = 0x00500004;
const unsigned PDDI_MAX_GLOBAL_PROJECTED_TEXTURE = 16;

class pddiExtGlobalProjectedTexture: public pddiExtension
{
public:
    virtual void SetProjection(unsigned id, const pddiMatrix*, pddiTexture*) = 0;
    virtual bool GetProjection(unsigned id, pddiMatrix*, pddiTexture**) = 0;
};


// read pixels from the frame buffer
const unsigned PDDI_EXT_READ_PIXELS = 0x00500005;

class pddiExtReadPixels : public pddiExtension
{
    public:
        virtual void ReadPixels( pddiBuffer source, const pddiRect& rect, pddiColour* buffer ) const = 0;

    protected:
        virtual ~pddiExtReadPixels() {}

    private:

};

class pddiExtBufferCopy : public pddiExtension
{
public:
    virtual pddiTexture *CreateZTexture(int xres, int yres, int bpp) = 0;
    virtual pddiTexture *CreateBackTexture(int xres, int yres, int bpp, bool alpha) = 0;
    virtual pddiTexture *CreateAlphaTexture(int xres, int yres, int bpp) = 0;
    virtual bool CopyZBuf(pddiTexture *tex, bool clear) = 0; 
    virtual bool CopyBackBuf(pddiTexture *tex, bool clear, bool halfsize) = 0;
    virtual bool CopyAlphaBuf(pddiTexture *tex, bool clear, int bpp, int alphadepth) = 0;

protected:
    virtual ~pddiExtBufferCopy() {};
};

#endif /* _PDDIEXT_HPP */