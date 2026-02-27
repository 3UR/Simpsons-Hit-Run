/*===========================================================================
    pddienum.hpp
    03-Sep-99 Created by Neall

    Copyright (c)1999 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _PDDIENUM_HPP
#define _PDDIENUM_HPP

//----------------------------------------------------
// pddiDisplay enums
enum pddiDisplayMode
{
    PDDI_DISPLAY_FULLSCREEN,
    PDDI_DISPLAY_WINDOW,
    PDDI_DISPLAY_FULLSCREEN_PAL
};

enum pddiLockType
{ 
    PDDI_LOCK_READONLY,
    PDDI_LOCK_WRITEONLY,
    PDDI_LOCK_READWRITE
};

//-------------------------------------------------------------------
// pddiRenderContext enums

// matrix stack types
enum pddiMatrixType
{
    PDDI_MATRIX_MODELVIEW,
    PDDI_MATRIX_TEXTURE0,
    PDDI_MATRIX_TEXTURE1,
    PDDI_MATRIX_TEXTURE2,
    PDDI_MATRIX_TEXTURE3
};

// projection
enum pddiProjectionMode
{
    PDDI_PROJECTION_PERSPECTIVE,
    PDDI_PROJECTION_ORTHOGRAPHIC,
    PDDI_PROJECTION_DEVICE
};

// buffer clear flags
enum pddiBufferMask
{
    PDDI_BUFFER_COLOUR  = 1,
    PDDI_BUFFER_DEPTH   = 2,
    PDDI_BUFFER_STENCIL = 4,
    PDDI_BUFFER_ALL     = 0xffffffff
};

// z buffer comparison flags
enum pddiCompareMode
{
    PDDI_COMPARE_NONE,
    PDDI_COMPARE_ALWAYS,
    PDDI_COMPARE_LESS,
    PDDI_COMPARE_LESSEQUAL,
    PDDI_COMPARE_GREATER,
    PDDI_COMPARE_GREATEREQUAL,
    PDDI_COMPARE_EQUAL,
    PDDI_COMPARE_NOTEQUAL
};

// stencil modes
enum pddiStencilOp
{
     PDDI_STENCIL_KEEP,
     PDDI_STENCIL_ZERO,
     PDDI_STENCIL_REPLACE,
     PDDI_STENCIL_INCR,
     PDDI_STENCIL_DECR,
     PDDI_STENCIL_INVERT
};

// fill modes
enum pddiFillMode
{
    PDDI_FILL_SOLID,
    PDDI_FILL_WIRE,
    PDDI_FILL_POINT
};

// backface culling modes
enum pddiCullMode
{
    PDDI_CULL_NONE,
    PDDI_CULL_NORMAL,
    PDDI_CULL_INVERTED,
    // june26/2001 amb : temp hack, these are in fact
    // equivalent to normal/inverted mode
    PDDI_CULL_SHADOW_BACKFACE,
    PDDI_CULL_SHADOW_FRONTFACE
};

// texture-cbv combine modes
enum pddiTextureBlendMode
{
    PDDI_TEXBLEND_DECAL,
    PDDI_TEXBLEND_DECALALPHA,
    PDDI_TEXBLEND_MODULATE,
    PDDI_TEXBLEND_MODULATEALPHA,
    PDDI_TEXBLEND_ADD,
    PDDI_TEXBLEND_MODULATEINTENSITY
};

// texture coordinate wrapping modes
enum pddiUVMode
{
    PDDI_UV_TILE,
    PDDI_UV_CLAMP
};

// texture filtering modes
enum pddiFilterMode
{
    PDDI_FILTER_NONE,
    PDDI_FILTER_BILINEAR,
    PDDI_FILTER_MIPMAP,
    PDDI_FILTER_MIPMAP_BILINEAR,
    PDDI_FILTER_MIPMAP_TRILINEAR
};

// light types
enum pddiLightType
{
    PDDI_LIGHT_DIRECTIONAL, // infinite-directional light
    PDDI_LIGHT_POINT,        // pseudo-point light source
    PDDI_LIGHT_SPOT
};

// vertex components
enum pddiVertexMask
{
    // first four bits of mask specify number of
    // texture co-ordinates
    PDDI_V_UVCOUNT0    = 0,
    PDDI_V_UVCOUNT1    = 1,
    PDDI_V_UVCOUNT2    = 2,
    PDDI_V_UVCOUNT3    = 3,
    PDDI_V_UVCOUNT4    = 4,
    PDDI_V_UVCOUNT5    = 5,
    PDDI_V_UVCOUNT6    = 6,
    PDDI_V_UVCOUNT7    = 7,
    PDDI_V_UVCOUNT8    = 8,
    PDDI_V_UVMASK      = 15,
    // remaining bits are flags to indicate the 
    // presence/absence of particular components
    PDDI_V_NORMAL       = 1<<4,
    // PDDI_V_COLOUR and PDDI_V_COLOUR2 are mutually exclusive.
    PDDI_V_COLOUR       = 1<<5, // Use one colour per vertex. (Pure3D::Mesh::COLOURLIST)
    PDDI_V_SPECULAR     = 1<<6, 
    PDDI_V_INDICES      = 1<<7,
    PDDI_V_WEIGHTS      = 1<<8,
    PDDI_V_SIZE         = 1<<9,
    PDDI_V_W            = 1<<10,
    PDDI_V_BINORMAL     = 1<<11,
    PDDI_V_TANGENT      = 1<<12,
    PDDI_V_POSITION     = 1<<13,
    PDDI_V_COLOUR2      = 1<<14, // Use multiple colours per vertex. (Pure3D::Mesh::MULTICOLOURLIST)
    // The next three bits of mask specify the number of vertex colours.
    PDDI_V_COLOUR_COUNT0 = 0<<15,
    PDDI_V_COLOUR_COUNT1 = 1<<15,
    PDDI_V_COLOUR_COUNT2 = 2<<15,
    PDDI_V_COLOUR_COUNT3 = 3<<15,
    PDDI_V_COLOUR_COUNT4 = 4<<15,
    PDDI_V_COLOUR_COUNT5 = 5<<15,
    PDDI_V_COLOUR_COUNT6 = 6<<15,
    PDDI_V_COLOUR_COUNT7 = 7<<15,
    PDDI_V_COLOUR_MASK   = 7<<15,
    PDDI_V_COLOUR_MASK_OFFSET = 15
};
    
// Number of colour sets (CBVs per vertex).  Used with PDDI_V_COLOUR2.
inline unsigned PddiNumColourSets(unsigned vertexFormat)
{
    return (vertexFormat & PDDI_V_COLOUR_MASK) >> PDDI_V_COLOUR_MASK_OFFSET;
}

// vertex types
const unsigned PDDI_V_C    = PDDI_V_POSITION | PDDI_V_COLOUR;
const unsigned PDDI_V_C2   = PDDI_V_POSITION | PDDI_V_COLOUR2;
const unsigned PDDI_V_N    = PDDI_V_POSITION | PDDI_V_NORMAL;
const unsigned PDDI_V_T    = PDDI_V_POSITION | PDDI_V_UVCOUNT1;
const unsigned PDDI_V_T2   = PDDI_V_POSITION | PDDI_V_UVCOUNT2;
const unsigned PDDI_V_CT   = PDDI_V_POSITION | PDDI_V_COLOUR  | PDDI_V_UVCOUNT1;
const unsigned PDDI_V_C2T  = PDDI_V_POSITION | PDDI_V_COLOUR2 | PDDI_V_UVCOUNT1;
const unsigned PDDI_V_CT2  = PDDI_V_POSITION | PDDI_V_COLOUR  | PDDI_V_UVCOUNT2;
const unsigned PDDI_V_C2T2 = PDDI_V_POSITION | PDDI_V_COLOUR2 | PDDI_V_UVCOUNT2;
const unsigned PDDI_V_NT   = PDDI_V_POSITION | PDDI_V_NORMAL  | PDDI_V_UVCOUNT1;
const unsigned PDDI_V_NT2  = PDDI_V_POSITION | PDDI_V_NORMAL  | PDDI_V_UVCOUNT2;
// skinning types
const unsigned PDDI_V_CKT   = PDDI_V_POSITION | PDDI_V_COLOUR  | PDDI_V_UVCOUNT1 | PDDI_V_INDICES | PDDI_V_WEIGHTS;
const unsigned PDDI_V_C2KT  = PDDI_V_POSITION | PDDI_V_COLOUR2 | PDDI_V_UVCOUNT1 | PDDI_V_INDICES | PDDI_V_WEIGHTS;
const unsigned PDDI_V_NKT   = PDDI_V_POSITION | PDDI_V_NORMAL  | PDDI_V_UVCOUNT1 | PDDI_V_INDICES | PDDI_V_WEIGHTS;
const unsigned PDDI_V_CKT2  = PDDI_V_POSITION | PDDI_V_COLOUR  | PDDI_V_UVCOUNT2 | PDDI_V_INDICES | PDDI_V_WEIGHTS;
const unsigned PDDI_V_C2KT2 = PDDI_V_POSITION | PDDI_V_COLOUR2 | PDDI_V_UVCOUNT2 | PDDI_V_INDICES | PDDI_V_WEIGHTS;
const unsigned PDDI_V_NKT2  = PDDI_V_POSITION | PDDI_V_NORMAL  | PDDI_V_UVCOUNT2 | PDDI_V_INDICES | PDDI_V_WEIGHTS;
// xbox only
const unsigned PDDI_V_CS  = PDDI_V_POSITION | PDDI_V_COLOUR | PDDI_V_SIZE;
const unsigned PDDI_V_CSW = PDDI_V_POSITION | PDDI_V_COLOUR | PDDI_V_SIZE | PDDI_V_W;


enum pddiPrimType
{
    PDDI_PRIM_TRIANGLES,
    PDDI_PRIM_TRISTRIP,
    PDDI_PRIM_LINES,
    PDDI_PRIM_LINESTRIP,
    PDDI_PRIM_POINTS
};

enum pddiStateMask
{
    PDDI_STATE_RENDER =     (1<<0),
    PDDI_STATE_VIEW =       (1<<1),
    PDDI_STATE_LIGHTING =   (1<<2),
    PDDI_STATE_FOG =        (1<<3),
    PDDI_STATE_STENCIL =    (1<<4),
    PDDI_STATE_ALL =        (0xffffffff)
};

enum pddiStatType
{
    PDDI_STAT_CURRENT_FRAME,
    PDDI_STAT_FRAME_TIME,
    PDDI_STAT_BUFFERED_PRIM,
    PDDI_STAT_BUFFERED_PRIM_VERT,
    PDDI_STAT_BUFFERED_INDEXED_PRIM,
    PDDI_STAT_BUFFERED_INDEXED_PRIM_VERT,
    PDDI_STAT_BUFFERED_PRIM_CALLS,
    PDDI_STAT_BUFFERED_PRIM_AVG,
    PDDI_STAT_BUFFERED_COUNT,
    PDDI_STAT_BUFFERED_ALLOC,
    PDDI_STAT_STREAMED_PRIM,
    PDDI_STAT_STREAMED_PRIM_VERT,
    PDDI_STAT_STREAMED_PRIM_CALLS,
    PDDI_STAT_STREAMED_PRIM_AVG,
    PDDI_STAT_SKINNED_BONES,
    PDDI_STAT_SKINNED_XFORM_VERT,
    PDDI_STAT_SKINNED_XFORM_MS,
    PDDI_STAT_SKINNED_WAIT_MS,
    PDDI_STAT_MATRIX_OPS,
    PDDI_STAT_LIGHT_OPS,
    PDDI_STAT_MATERIAL_OPS,
    PDDI_STAT_TEXTURE_HITS,
    PDDI_STAT_TEXTURE_MISSES,
    PDDI_STAT_TEXTURE_UPLOADED,
    PDDI_STAT_TEXTURE_ALLOCATED,
    PDDI_STAT_TEXTURE_SLOP,
    PDDI_STAT_TEXTURE_COUNT_4BIT,
    PDDI_STAT_TEXTURE_ALLOC_4BIT,
    PDDI_STAT_TEXTURE_COUNT_8BIT,
    PDDI_STAT_TEXTURE_ALLOC_8BIT,
    PDDI_STAT_TEXTURE_COUNT_16BIT,
    PDDI_STAT_TEXTURE_ALLOC_16BIT,
    PDDI_STAT_TEXTURE_COUNT_32BIT,
    PDDI_STAT_TEXTURE_ALLOC_32BIT,
    PDDI_STAT_TEXTURE_COUNT_DXTN,
    PDDI_STAT_TEXTURE_ALLOC_DXTN
};

//-------------------------------------------------------------------
// pddiTexture enums

// orientation
enum pddiTextureOrigin
{
    PDDI_ORIGIN_TOP,
    PDDI_ORIGIN_BOTTOM
};

// texture types
enum pddiTextureType
{
    PDDI_TEXTYPE_RGB,
    PDDI_TEXTYPE_PALETTIZED,
    PDDI_TEXTYPE_LUMINANCE,
    PDDI_TEXTYPE_BUMPMAP,
    PDDI_TEXTYPE_DXT1,
    PDDI_TEXTYPE_DXT2,
    PDDI_TEXTYPE_DXT3,
    PDDI_TEXTYPE_DXT4,
    PDDI_TEXTYPE_DXT5,
    PDDI_TEXTYPE_IPU,
    PDDI_TEXTYPE_Z,
    PDDI_TEXTYPE_LINEAR,
    PDDI_TEXTYPE_RENDERTARGET,
    PDDI_TEXTYPE_PS2_4BIT,
    PDDI_TEXTYPE_PS2_8BIT,
    PDDI_TEXTYPE_PS2_16BIT,
    PDDI_TEXTYPE_PS2_32BIT,
    PDDI_TEXTYPE_GC_4BIT,
    PDDI_TEXTYPE_GC_8BIT,
    PDDI_TEXTYPE_GC_16BIT,
    PDDI_TEXTYPE_GC_32BIT,
    PDDI_TEXTYPE_GC_DXT1,
    PDDI_TEXTYPE_YUV
};

// pixel formats
enum pddiPixelFormat
{
    PDDI_PIXEL_UNKNOWN,
    PDDI_PIXEL_RGB565,
    PDDI_PIXEL_ARGB1555,
    PDDI_PIXEL_RGB555,
    PDDI_PIXEL_ARGB4444,
    PDDI_PIXEL_RGB888,
    PDDI_PIXEL_ARGB8888,
    PDDI_PIXEL_PAL8,
    PDDI_PIXEL_PAL4,
    PDDI_PIXEL_LUM8,
    PDDI_PIXEL_DUDV88,
    PDDI_PIXEL_DXT1,
    PDDI_PIXEL_DXT2,
    PDDI_PIXEL_DXT3,
    PDDI_PIXEL_DXT4,
    PDDI_PIXEL_DXT5,
    PDDI_PIXEL_Z32,
    PDDI_PIXEL_Z24,
    PDDI_PIXEL_Z16,
    PDDI_PIXEL_Z8,
    PDDI_PIXEL_PS2_4BIT,
    PDDI_PIXEL_PS2_8BIT,
    PDDI_PIXEL_PS2_16BIT,
    PDDI_PIXEL_PS2_32BIT,
    PDDI_PIXEL_GC_4BIT,
    PDDI_PIXEL_GC_8BIT,
    PDDI_PIXEL_GC_16BIT,
    PDDI_PIXEL_GC_32BIT,
    PDDI_PIXEL_GC_DXT1,
    PDDI_PIXEL_YUV
};

enum pddiTextureUsageHint
{
    PDDI_USAGE_STATIC,
    PDDI_USAGE_DYNAMIC,
    PDDI_USAGE_NOCACHE
};

//-------------------------------------------------------------------
// pddiMaterial enums
enum pddiBlendMode
{
    PDDI_BLEND_NONE,
    PDDI_BLEND_ALPHA,
    PDDI_BLEND_ADD,
    PDDI_BLEND_SUBTRACT,
    PDDI_BLEND_MODULATE,
    PDDI_BLEND_MODULATE2,
    PDDI_BLEND_ADDMODULATEALPHA,
    PDDI_BLEND_SUBMODULATEALPHA,
    PDDI_BLEND_DESTALPHA
};

enum pddiShadeMode
{
    PDDI_SHADE_FLAT,
    PDDI_SHADE_GOURAUD
};

enum pddiTextureGen
{
    PDDI_TEXGEN_NONE,
    PDDI_TEXGEN_ENVMAP,    // environment mapping
    PDDI_TEXGEN_SPHEREMAP  // spherical mapping
};

//-------------------------------------------------------------------
// Blend mode for multiple CBVs. Cobmine a colour from CBV SetA and
// SetB using the float blend value f.  SetA and SetB selected from
// pddiMultiCBVBlendOperand below.
enum pddiMultiCBVBlendMode
{
    PDDI_MULTI_CBV_BLEND_NONE,       // SetA
    PDDI_MULTI_CBV_BLEND_ADD,        // SetA + f * SetB
    PDDI_MULTI_CBV_BLEND_SUBTRACT,   // SetA - f * SetB
    PDDI_MULTI_CBV_BLEND_MODULATE,   // SetA * ( ( 1 - f) + f * setB )
    PDDI_MULTI_CBV_BLEND_INTERPOLATE // ( 1 - f) * SetA + f * setB
};

//-------------------------------------------------------------------
// Target operands for multiple CBVs (use with PDDI_V_COLOUR2).
enum pddiMultiCBVBlendOperand
{
    PDDI_MULTI_CBV_BLEND_COLOUR,
    PDDI_MULTI_CBV_SET1,
    PDDI_MULTI_CBV_SET2,
    PDDI_MULTI_CBV_SET3,
    PDDI_MULTI_CBV_SET4,
    PDDI_MULTI_CBV_SET5,
    PDDI_MULTI_CBV_SET6,
    PDDI_MULTI_CBV_SET7
};

#endif /* _PDDIENUM_HPP */