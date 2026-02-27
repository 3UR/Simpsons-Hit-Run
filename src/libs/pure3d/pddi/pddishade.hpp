//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _PDDISHADE_HPP_
#define _PDDISHADE_HPP_

#include <pddi/pdditype.hpp>

#define PDDI_FOURCC(ch0, ch1, ch2, ch3) \
                     ((PDDI_U32)(PDDI_U8)(ch0) | ((PDDI_U32)(PDDI_U8)(ch1) << 8) |       \
                     ((PDDI_U32)(PDDI_U8)(ch2) << 16) | ((PDDI_U32)(PDDI_U8)(ch3) << 24 ))

const unsigned PDDI_SP_NULL         = 0;

const unsigned PDDI_SP_BASETEX       = PDDI_FOURCC('T','E','X', 0 );
const unsigned PDDI_SP_TOPTEX        = PDDI_FOURCC('T','T','E','X');  // used by layered shader; specify the top texture
const unsigned PDDI_SP_TWOLAYERCBV   = PDDI_FOURCC('C','L','Y','R');  // apply cbv to both layers of layered shader
const unsigned PDDI_SP_UVMODE        = PDDI_FOURCC('U','V','M','D');
const unsigned PDDI_SP_FILTER        = PDDI_FOURCC('F','I','M','D');
const unsigned PDDI_SP_SHADEMODE     = PDDI_FOURCC('S','H','M','D');
const unsigned PDDI_SP_ISFOGGED      = PDDI_FOURCC('F','O','G', 0 );
const unsigned PDDI_SP_ISLIT         = PDDI_FOURCC('L','I','T', 0 );
const unsigned PDDI_SP_ISLIGHTMAP    = PDDI_FOURCC('I','S','L','M');
const unsigned PDDI_SP_AMBIENT       = PDDI_FOURCC('A','M','B','I');
const unsigned PDDI_SP_DIFFUSE       = PDDI_FOURCC('D','I','F','F');
const unsigned PDDI_SP_EMISSIVE      = PDDI_FOURCC('E','M','I','S');
const unsigned PDDI_SP_EMISSIVEALPHA = PDDI_FOURCC('E','M','S','A');  // alpha component of emissive colour (int)
const unsigned PDDI_SP_SPECULAR      = PDDI_FOURCC('S','P','E','C');
const unsigned PDDI_SP_SHININESS     = PDDI_FOURCC('S','H','I','N');
const unsigned PDDI_SP_BLENDMODE     = PDDI_FOURCC('B','L','M','D');
const unsigned PDDI_SP_TEXBLENDMODE  = PDDI_FOURCC('T','B','L','M');
const unsigned PDDI_SP_ALPHATEST     = PDDI_FOURCC('A','T','S','T');
const unsigned PDDI_SP_ALPHACOMPARE  = PDDI_FOURCC('A','C','M','P');
const unsigned PDDI_SP_LIGHTMAP      = PDDI_FOURCC('L','M','A','P');
const unsigned PDDI_SP_BUMPMAP       = PDDI_FOURCC('B','U','M','P');
const unsigned PDDI_SP_LIGHTV        = PDDI_FOURCC('L','I','T','V');
const unsigned PDDI_SP_CAMERAV       = PDDI_FOURCC('C','A','M','V');
const unsigned PDDI_SP_BMSPECULAR    = PDDI_FOURCC('B','M','S','P');  // [en|dis]able specular bump mapping
const unsigned PDDI_SP_BMLIGHTDEP    = PDDI_FOURCC('B','M','L','D');  // do or don't use a constant light vector for DP3 bumpmapping
const unsigned PDDI_SP_REFLMAP       = PDDI_FOURCC('R','E','F','L');
const unsigned PDDI_SP_CUBEIDX       = PDDI_FOURCC('C','U','B','I');
const unsigned PDDI_SP_CUBECREATE    = PDDI_FOURCC('C','U','B','C');
const unsigned PDDI_SP_CUBE_XPOS     = PDDI_FOURCC('C','B','X','P');
const unsigned PDDI_SP_CUBE_XNEG     = PDDI_FOURCC('C','B','X','N');
const unsigned PDDI_SP_CUBE_YPOS     = PDDI_FOURCC('C','B','Y','P');
const unsigned PDDI_SP_CUBE_YNEG     = PDDI_FOURCC('C','B','Y','N');
const unsigned PDDI_SP_CUBE_ZPOS     = PDDI_FOURCC('C','B','Z','P');
const unsigned PDDI_SP_CUBE_ZNEG     = PDDI_FOURCC('C','B','Z','N');
const unsigned PDDI_SP_TOONSTYLE     = PDDI_FOURCC('T','N','S','T');  // 0 = default, 1 = silhouette, 2 = depth/metal
const unsigned PDDI_SP_ROTVEC        = PDDI_FOURCC('R','O','T','V');  // rotation vector for sphere map
												   

// jul5/2001 amb - mipmap control
const unsigned PDDI_SP_MIPMAP_SHARPNESS = PDDI_FOURCC('M','S','H','P');  // [0-1], 0.5 default
const unsigned PDDI_SP_MIPMAP_MIN       = PDDI_FOURCC('M','M','I','N');  // 0-7, value is 2^(x+3)
const unsigned PDDI_SP_MIPMAP_MAX       = PDDI_FOURCC('M','M','A','X');  // 0-7, value is 2^(x+3)

const unsigned PDDI_SP_PROJTEX    = PDDI_FOURCC('P','R','J','T');  // map for texture projection shader
const unsigned PDDI_SP_PROJTEX0   = PDDI_FOURCC('P','R','J', 0 );  // map for texture projection shader
const unsigned PDDI_SP_PROJTEX1   = PDDI_FOURCC('P','R','J','1');  // map for texture projection shader
const unsigned PDDI_SP_SPECMAP    = PDDI_FOURCC('S','P','M','P');
const unsigned PDDI_SP_PROJMATRIX = PDDI_FOURCC('P','R','M','T');  // matrix for texture projection shader
const unsigned PDDI_SP_PROJID     = PDDI_FOURCC('P','R','J','N');  // Identify which class of projected textures to use

const unsigned PDDI_SP_TWOSIDED   = PDDI_FOURCC('2','S','I','D');   

const unsigned PDDI_SP_SHADOWTEXTURE = PDDI_FOURCC('S','H','T','X');  // Depth map from light view for shadows
const unsigned PDDI_SP_SHADOWCOLOUR  = PDDI_FOURCC('S','H','C','L');  // modulate colour for shadowed area 

// "particle" shader properties
const unsigned PDDI_SP_POINTSPRITESCALEENABLE = PDDI_FOURCC('P','S','C','E');  // int 
const unsigned PDDI_SP_POINTSPRITESIZE        = PDDI_FOURCC('P','S','S','Z');  // float  
const unsigned PDDI_SP_POINTSPRITESIZEMIN     = PDDI_FOURCC('P','S','S','A');  // float  
const unsigned PDDI_SP_POINTSPRITESIZEMAX     = PDDI_FOURCC('P','S','S','B');  // float  
const unsigned PDDI_SP_POINTSPRITESCALEA      = PDDI_FOURCC('P','S','C','A');  // float  
const unsigned PDDI_SP_POINTSPRITESCALEB      = PDDI_FOURCC('P','S','C','B');  // float  
const unsigned PDDI_SP_POINTSPRITESCALEC      = PDDI_FOURCC('P','S','C','C');  // float  
const unsigned PDDI_SP_POINTSPRITESCALEOFFSET = PDDI_FOURCC('P','S','C','S');  // float  

// "environment" shader properties
const unsigned PDDI_SP_ENVBLEND  = PDDI_FOURCC('E','N','V','B');

// "refract" shader properties
const unsigned PDDI_SP_REFRACTINDEX  = PDDI_FOURCC('R','E','F','I');
const unsigned PDDI_SP_REFRACTCOLOUR = PDDI_FOURCC('R','E','F','C');
const unsigned PDDI_SP_REFRACTBLEND =  PDDI_FOURCC('R','E','F','B');

// ----------------------------
// xbox ubershader
// specify how many texture stages a uber shader has
const unsigned PDDI_SP_STAGENUM     = PDDI_FOURCC('S','T','G','N');

//texture parameters: maximum number of texture stage: 8
const unsigned PDDI_SP_TEX1 = PDDI_FOURCC('T','E','X','1');
const unsigned PDDI_SP_TEX2 = PDDI_FOURCC('T','E','X','2');
const unsigned PDDI_SP_TEX3 = PDDI_FOURCC('T','E','X','3');
const unsigned PDDI_SP_TEX4 = PDDI_FOURCC('T','E','X','4');
const unsigned PDDI_SP_TEX5 = PDDI_FOURCC('T','E','X','5');
const unsigned PDDI_SP_TEX6 = PDDI_FOURCC('T','E','X','6');
const unsigned PDDI_SP_TEX7 = PDDI_FOURCC('T','E','X','7');
const unsigned PDDI_SP_TEX8 = PDDI_FOURCC('T','E','X','8');

// colour blending mode parameters: indicate the blending mode to take for every texture stage
const unsigned PDDI_SP_COLOUROP1 = PDDI_FOURCC('C','O','P','1');
const unsigned PDDI_SP_COLOUROP2 = PDDI_FOURCC('C','O','P','2');
const unsigned PDDI_SP_COLOUROP3 = PDDI_FOURCC('C','O','P','3');
const unsigned PDDI_SP_COLOUROP4 = PDDI_FOURCC('C','O','P','4');
const unsigned PDDI_SP_COLOUROP5 = PDDI_FOURCC('C','O','P','5');
const unsigned PDDI_SP_COLOUROP6 = PDDI_FOURCC('C','O','P','6');
const unsigned PDDI_SP_COLOUROP7 = PDDI_FOURCC('C','O','P','7');
const unsigned PDDI_SP_COLOUROP8 = PDDI_FOURCC('C','O','P','8');

// alpha blending mode parameters: indicate the alpha blending mode to take for every texture stage
const unsigned PDDI_SP_ALPHAOP1 = PDDI_FOURCC('A','O','P','1');
const unsigned PDDI_SP_ALPHAOP2 = PDDI_FOURCC('A','O','P','2');
const unsigned PDDI_SP_ALPHAOP3 = PDDI_FOURCC('A','O','P','3');
const unsigned PDDI_SP_ALPHAOP4 = PDDI_FOURCC('A','O','P','4');
const unsigned PDDI_SP_ALPHAOP5 = PDDI_FOURCC('A','O','P','5');
const unsigned PDDI_SP_ALPHAOP6 = PDDI_FOURCC('A','O','P','6');
const unsigned PDDI_SP_ALPHAOP7 = PDDI_FOURCC('A','O','P','7');
const unsigned PDDI_SP_ALPHAOP8 = PDDI_FOURCC('A','O','P','8');

// texture blending mode parameters. Texture Op = colour op + alpha op.
const unsigned PDDI_SP_TEXTUREOP1 = PDDI_FOURCC('T','O','P','1');
const unsigned PDDI_SP_TEXTUREOP2 = PDDI_FOURCC('T','O','P','2');
const unsigned PDDI_SP_TEXTUREOP3 = PDDI_FOURCC('T','O','P','3');
const unsigned PDDI_SP_TEXTUREOP4 = PDDI_FOURCC('T','O','P','4');
const unsigned PDDI_SP_TEXTUREOP5 = PDDI_FOURCC('T','O','P','5');
const unsigned PDDI_SP_TEXTUREOP6 = PDDI_FOURCC('T','O','P','6');
const unsigned PDDI_SP_TEXTUREOP7 = PDDI_FOURCC('T','O','P','7');
const unsigned PDDI_SP_TEXTUREOP8 = PDDI_FOURCC('T','O','P','8');

// constant parameters: the constants used for texture stage, maximum 4
const unsigned PDDI_SP_CONSTANT1 = PDDI_FOURCC('C','S','T','1');
const unsigned PDDI_SP_CONSTANT2 = PDDI_FOURCC('C','S','T','2');
const unsigned PDDI_SP_CONSTANT3 = PDDI_FOURCC('C','S','T','3');
const unsigned PDDI_SP_CONSTANT4 = PDDI_FOURCC('C','S','T','4');
const unsigned PDDI_SP_CONSTANT5 = PDDI_FOURCC('C','S','T','5');
const unsigned PDDI_SP_CONSTANT6 = PDDI_FOURCC('C','S','T','6');
const unsigned PDDI_SP_CONSTANT7 = PDDI_FOURCC('C','S','T','7');
const unsigned PDDI_SP_CONSTANT8 = PDDI_FOURCC('C','S','T','8');

// UV channel parameters: specify the UV channel to  which for a texture
const unsigned PDDI_SP_UVMAP1 = PDDI_FOURCC(0,'U','V','1' );
const unsigned PDDI_SP_UVMAP2 = PDDI_FOURCC(0,'U','V','2' );
const unsigned PDDI_SP_UVMAP3 = PDDI_FOURCC(0,'U','V','3' );
const unsigned PDDI_SP_UVMAP4 = PDDI_FOURCC(0,'U','V','4' );
const unsigned PDDI_SP_UVMAP5 = PDDI_FOURCC(0,'U','V','5' );
const unsigned PDDI_SP_UVMAP6 = PDDI_FOURCC(0,'U','V','6' );
const unsigned PDDI_SP_UVMAP7 = PDDI_FOURCC(0,'U','V','7' );
const unsigned PDDI_SP_UVMAP8 = PDDI_FOURCC(0,'U','V','8' );

// UV channel parameters: specify the UV addressing mode applied to each texture stage
const unsigned PDDI_SP_UVMODE1 = PDDI_FOURCC('U','V','M','1' );
const unsigned PDDI_SP_UVMODE2 = PDDI_FOURCC('U','V','M','2' );
const unsigned PDDI_SP_UVMODE3 = PDDI_FOURCC('U','V','M','3' );
const unsigned PDDI_SP_UVMODE4 = PDDI_FOURCC('U','V','M','4' );
const unsigned PDDI_SP_UVMODE5 = PDDI_FOURCC('U','V','M','5' );
const unsigned PDDI_SP_UVMODE6 = PDDI_FOURCC('U','V','M','6' );
const unsigned PDDI_SP_UVMODE7 = PDDI_FOURCC('U','V','M','7' );
const unsigned PDDI_SP_UVMODE8 = PDDI_FOURCC('U','V','M','8' );

// Matrix parameters: UV transform matrices
const unsigned PDDI_SP_UVMATRIX1 = PDDI_FOURCC('M','T','X','1');
const unsigned PDDI_SP_UVMATRIX2 = PDDI_FOURCC('M','T','X','2');
const unsigned PDDI_SP_UVMATRIX3 = PDDI_FOURCC('M','T','X','3');
const unsigned PDDI_SP_UVMATRIX4 = PDDI_FOURCC('M','T','X','4');
const unsigned PDDI_SP_UVMATRIX5 = PDDI_FOURCC('M','T','X','5');
const unsigned PDDI_SP_UVMATRIX6 = PDDI_FOURCC('M','T','X','6');
const unsigned PDDI_SP_UVMATRIX7 = PDDI_FOURCC('M','T','X','7');
const unsigned PDDI_SP_UVMATRIX8 = PDDI_FOURCC('M','T','X','8');

// Filter parameters: filter mode applied to each texture stage
const unsigned PDDI_SP_FILTER1 = PDDI_FOURCC('F','L','T','1');
const unsigned PDDI_SP_FILTER2 = PDDI_FOURCC('F','L','T','2');
const unsigned PDDI_SP_FILTER3 = PDDI_FOURCC('F','L','T','3');
const unsigned PDDI_SP_FILTER4 = PDDI_FOURCC('F','L','T','4');
const unsigned PDDI_SP_FILTER5 = PDDI_FOURCC('F','L','T','5');
const unsigned PDDI_SP_FILTER6 = PDDI_FOURCC('F','L','T','6');
const unsigned PDDI_SP_FILTER7 = PDDI_FOURCC('F','L','T','7');
const unsigned PDDI_SP_FILTER8 = PDDI_FOURCC('F','L','T','8');

//constant for toon shader
const unsigned PDDI_SP_SILHOUETTE    = PDDI_FOURCC('S','L','H','T');
const unsigned PDDI_SP_TOONLEVEL    = PDDI_FOURCC('T','L','V','L');
const unsigned PDDI_SP_TOONLINEWIDTH    = PDDI_FOURCC('T','W','T','H');

const unsigned PDDI_SP_ZWRITE = PDDI_FOURCC('Z','W','R','T');
const unsigned PDDI_SP_ZTEST  = PDDI_FOURCC('Z','T','S','T');
const unsigned PDDI_SP_ALPHACOMPARE_THRESHOLD = PDDI_FOURCC('A','C','T','H');

// Multiple vertex colour values.
const unsigned PDDI_SP_MULTI_CBV        = PDDI_FOURCC('M','C','B','V');
const unsigned PDDI_SP_CBV_BLEND_MODE   = PDDI_FOURCC('C','B','V','M');
const unsigned PDDI_SP_CBV_BLEND_VALUE  = PDDI_FOURCC('C','B','V','V');
const unsigned PDDI_SP_CBV_BLEND_SET_A  = PDDI_FOURCC('C','B','V','A');
const unsigned PDDI_SP_CBV_BLEND_SET_B  = PDDI_FOURCC('C','B','V','B');
const unsigned PDDI_SP_CBV_BLEND_COLOUR = PDDI_FOURCC('C','B','V','C');

// For hulk toon texture
const unsigned PDDI_SP_HCTOON           = PDDI_FOURCC('H','C','T','N');

#endif

