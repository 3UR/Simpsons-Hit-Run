/*===========================================================================
    File:: chunks.h

    Chunk codes for Pure3D data types

    Copyright (c)1995 - 1998 Radical Entertainment, Inc.  All rights reserved.

    Mark removed the log from this file, since it was taking up too much space.
    
===========================================================================*/
 
//
// Chunks ranges:
//
// 0x0000 - 0x0FFF     unused
// 0x1000 - 0x1FFF     unused
// 0x2000 - 0x2FFF     Various data
// 0x3000 - 0x3EFF     Geo file
// 0x3E00 - 0x3FFF        H-spline
// 0x4000 - 0x4FFF     Animation Chunks 

// 0x5000 - 0x5FFF     PC specific chunks (future)
// 0x6000 - 0x6FFF     PSX specific chunks

// 0x7000 - 0x7FFF     Tool Specific data (unused by games, or for debugging)

// 0x8000 - 0x8FFF     reserved for game specific data chunks

// 0x9000 - 0x9FFF     Rendering Data Structures (BSP etc)

// 0xA000 - 0xAFFF     2D bitmap chunks

// 0xB000 - 0xBFFF     twaddler chunks

// 0xC000 - 0xCFFF     physic object chunks

// 0xFF00 - 0xFFFF     File types
//

#ifndef _CHUNKS_H
#define _CHUNKS_H
const unsigned P3D_IMAGE              = 0x3510;
const unsigned P3D_IMAGE_DATA         = 0x3511;
const unsigned P3D_IMAGE_FILENAME     = 0x3512;

// Skeleton Chunks
const unsigned P3D_SKELETON                  = 0x4500;
const unsigned P3D_SKELETON_JOINT            = 0x4501;
const unsigned P3D_SKELETON_JOINT_PHYSICS    = 0x4502;
const unsigned P3D_SKELETON_JOINT_MIRROR_MAP = 0x4503;
const unsigned P3D_SKELETON_JOINT_FIX_FLAG   = 0x4504;

const unsigned P3D_COMPOSITE_DRAWABLE           = 0x4512;
const unsigned P3D_COMPOSITE_DRAWABLE_SKIN_LIST = 0x4513;
const unsigned P3D_COMPOSITE_DRAWABLE_PROP_LIST = 0x4514;
const unsigned P3D_COMPOSITE_DRAWABLE_SKIN      = 0x4515;
const unsigned P3D_COMPOSITE_DRAWABLE_PROP      = 0x4516;
const unsigned P3D_COMPOSITE_DRAWABLE_EFFECT_LIST = 0x4517;
const unsigned P3D_COMPOSITE_DRAWABLE_EFFECT    = 0x4518;
const unsigned P3D_COMPOSITE_DRAWABLE_SORTORDER = 0x4519;

// Frame Controller chunks
const unsigned P3D_FRAME_CONTROLLER       = 0x4520;

const unsigned P3D_MULTI_CONTROLLER         = 0x48A0;
const unsigned P3D_MULTI_CONTROLLER_TRACKS  = 0x48A1;
const unsigned P3D_MULTI_CONTROLLER_TRACK   = 0x48A2;

const unsigned P3D_CAMERA             = 0x2200;

const unsigned P3D_LIGHT_GROUP         = 0x2380;

// 0x7000 - 0x7FFF  Tool Specific data (unused by games, or for debugging)
const unsigned P3D_HISTORY            = 0x7000;
const unsigned P3D_ALIGN              = 0x7001;

// Used by the maya exporter
const unsigned P3D_EXPORT_INFO         = 0x7030;
const unsigned P3D_EXPORT_NAMED_STRING = 0x7031;
const unsigned P3D_EXPORT_NAMED_INT    = 0x7032;

//********************************
// Chunks needed for conversions
//********************************
const unsigned P3D_EXPRESSION_PRESET         = 0x4A00;
const unsigned P3D_EXPRESSION_GROUP          = 0x4A01;
const unsigned P3D_EXPRESSION_ANIM           = 0x4A10;
const unsigned P3D_EXPRESSION_ANIM_CHANNEL   = 0x4A11;
const unsigned P3D_EXPRESSION_MIXER          = 0x4A20;

const unsigned P3D_VERTEXOFFSET              = 0x4A80;
const unsigned P3D_VERTEXOFFSET_ANIM         = 0x4A81;
const unsigned P3D_VERTEX_OFFSET_EXPRESSION  = 0x4A82;

const unsigned P3D_SG_TRANSFORM_ANIM        = 0x9150;

// Visibility animation
const unsigned P3D_VISIBILITY_ANIM           = 0x4290;
const unsigned P3D_VISIBILITY_ANIM_CHANNEL   = 0x4291;

// Texture animation
const unsigned P3D_TEXTURE_ANIM         = 0x3520;
const unsigned P3D_TEXTURE_ANIM_CHANNEL = 0x3521;

// V12 Animation chunks
const unsigned P3D_POSE_ANIM            = 0x4700;
const unsigned P3D_JOINT_LIST           = 0x4201;
const unsigned P3D_ANIM_CHANNEL         = 0x4702;
const unsigned P3D_POSE_ANIM_MIRRORED   = 0x4703;

const unsigned P3D_CHANNEL_1DOF          = 0x4800;
const unsigned P3D_CHANNEL_3DOF          = 0x4801;
const unsigned P3D_CHANNEL_1DOF_ANGLE    = 0x4802;
const unsigned P3D_CHANNEL_3DOF_ANGLE    = 0x4803;
const unsigned P3D_CHANNEL_STATIC        = 0x4804;
const unsigned P3D_CHANNEL_STATIC_ANGLE  = 0x4805;
const unsigned P3D_CHANNEL_QUATERNION    = 0x4806;
const unsigned P3D_CHANNEL_STATIC_QUATERNION = 0x4807;

const unsigned P3D_CAMERA_ANIM                      = 0x4900;
const unsigned P3D_CAMERA_ANIM_CHANNEL              = 0x4901;
const unsigned P3D_CAMERA_ANIM_POS_CHANNEL          = 0x4902;
const unsigned P3D_CAMERA_ANIM_LOOK_CHANNEL         = 0x4903;
const unsigned P3D_CAMERA_ANIM_UP_CHANNEL           = 0x4904;
const unsigned P3D_CAMERA_ANIM_FOV_CHANNEL          = 0x4905;

// V16 Camera Animation Chunks.
// Added by Bryan Ewert on 22 Feb 2002
const unsigned P3D_CAMERA_ANIM_NEARCLIP_CHANNEL     = 0x4906;
const unsigned P3D_CAMERA_ANIM_FARCLIP_CHANNEL      = 0x4907;

const unsigned P3D_LIGHT_ANIM                 = 0x4980;
const unsigned P3D_LIGHT_ANIM_CHANNEL         = 0x4981;
const unsigned P3D_LIGHT_ANIM_COLOUR_CHANNEL  = 0x4982;
const unsigned P3D_LIGHT_ANIM_PARAM_CHANNEL   = 0x4983;
const unsigned P3D_LIGHT_ANIM_ENABLE_CHANNEL  = 0x4985;

const unsigned P3D_ENTITY_ANIM_CHANNEL = 0x42A0;
const unsigned P3D_KEYLIST_COLOUR       = 0x4216;

// v17 Vertex Animaion Chunks
const unsigned P3D_VERTEX_ANIM      = 0x4a00;
const unsigned P3D_VERTEX_ANIM_CHANNEL      = 0x4a01;

//------------------------------------------------------------------------------------------
// UNUSED CHUNKS
//------------------------------------------------------------------------------------------

#if 0
// new:
const unsigned P3D_DATA_FILE               = 0xFF443350;       // New P3D wrapper ID
const unsigned P3D_DATA_FILE_SWAP          = 0x503344FF;       // New P3D wrapper ID on big endian
const unsigned P3D_16BIT_DATA_FILE         = 0xFF04; 


const unsigned ATTR_VERTEX            = 0x7010;
const unsigned ATTR_POLY              = 0x7011;

const unsigned P3D_EXPORTER_VERSION   = 0x7023;

// The following chunks represent Pure3D data.
const unsigned P3D_MATRIX             = 0x2000;
const unsigned P3D_POS_ROT            = 0x2001;
const unsigned P3D_COLOR_RGB          = 0x2002;
const unsigned P3D_COLOR_RGBA         = 0x2003;
const unsigned P3D_FOV                = 0x2004;
const unsigned P3D_DIRECTION          = 0x2005;
const unsigned P3D_POSITION           = 0x2006;
const unsigned P3D_ROTATION_AXIS      = 0x2007;
const unsigned P3D_BOX                = 0x2008;
const unsigned P3D_SPHERE             = 0x2009;
const unsigned P3D_PLANE              = 0x200A;
const unsigned P3D_PARAMETERS         = 0x200B;
const unsigned P3D_SPHERE_LIST        = 0x200C;

// this is the main wrapper chunk for .p3d files - ie. general chunk based p3d data files
// it is equivalent to P3D_YTR_FILE, P3D_GEO_FILE, P3D_PRO_FILE, and P3D_ANIM_FILE
// eventually it is the only one of these that should be used, so that P3D load 
// doesn't have to check for all these types of chunks.

// Particle system data
const unsigned P3D_PARTICLE_SYSTEM   = 0x2100;
const unsigned P3D_POINT_EMITTER     = 0x2101;
const unsigned P3D_SPRITE_EMITTER    = 0x2102;

const unsigned P3D_PARTICLE_LIFE_CHANNEL              = 0x2110;
const unsigned P3D_PARTICLE_SPEED_CHANNEL             = 0x2111;
const unsigned P3D_PARTICLE_WEIGHT_CHANNEL            = 0x2112;
const unsigned P3D_PARTICLE_LIFE_VAR_CHANNEL          = 0x2113;
const unsigned P3D_PARTICLE_SPEED_VAR_CHANNEL         = 0x2114;
const unsigned P3D_PARTICLE_WEIGHT_VAR_CHANNEL        = 0x2115;
const unsigned P3D_PARTICLE_LIFE_OL_CHANNEL           = 0x2116;
const unsigned P3D_PARTICLE_SPEED_OL_CHANNEL          = 0x2117;
const unsigned P3D_PARTICLE_WEIGHT_OL_CHANNEL         = 0x2118;
const unsigned P3D_PARTICLE_NUM_PARTICLES_CHANNEL     = 0x2119;
const unsigned P3D_PARTICLE_EMISSION_RATE_CHANNEL     = 0x211A;
const unsigned P3D_PARTICLE_SIZE_CHANNEL              = 0x211B;
const unsigned P3D_PARTICLE_SPIN_CHANNEL              = 0x211C;
const unsigned P3D_PARTICLE_TRANSPARENCY_CHANNEL      = 0x211D;
const unsigned P3D_PARTICLE_COLOUR_CHANNEL            = 0x211E;
const unsigned P3D_PARTICLE_SIZE_VAR_CHANNEL          = 0x211F;
const unsigned P3D_PARTICLE_SPIN_VAR_CHANNEL          = 0x2120;
const unsigned P3D_PARTICLE_TRANSPARENCY_VAR_CHANNEL  = 0x2121;
const unsigned P3D_PARTICLE_COLOUR_VAR_CHANNEL        = 0x2122;
const unsigned P3D_PARTICLE_SIZE_OL_CHANNEL           = 0x2123;
const unsigned P3D_PARTICLE_SPIN_OL_CHANNEL           = 0x2124;
const unsigned P3D_PARTICLE_TRANSPARENCY_OL_CHANNEL   = 0x2125;
const unsigned P3D_PARTICLE_COLOUR_OL_CHANNEL         = 0x2126;
const unsigned P3D_PARTICLE_CHANNEL                   = 0x2127;
const unsigned P3D_PARTICLE_POINT_GENERATOR           = 0x2128;
const unsigned P3D_PARTICLE_PLANE_GENERATOR           = 0x2129;
const unsigned P3D_PARTICLE_SPHERE_GENERATOR          = 0x212A;
const unsigned P3D_PARTICLE_GRAVITY_CHANNEL           = 0x212B;
const unsigned P3D_PARTICLE_GENERATOR_HORIZ_SPREAD    = 0x212E;
const unsigned P3D_PARTICLE_GENERATOR_VERT_SPREAD     = 0x212F;
const unsigned P3D_PARTICLE_POSITION_CHANNEL          = 0x2130;
const unsigned P3D_PARTICLE_ROTATION_CHANNEL          = 0x2131;


const unsigned P3D_FONT               = 0x3062;
const unsigned P3D_FONT_GLYPHS        = 0x3063;
const unsigned P3D_TEXTURE_FONT       = 0x3064;
const unsigned P3D_TEXTURE_GLYPH      = 0x3065;
const unsigned P3D_IMAGE_FONT         = 0x3066;
const unsigned P3D_IMAGE_GLYPH        = 0x3067;


// The following chunks are relevant to the geometry file format.

// V12 geometry format
const unsigned P3D_MESH              = 0x3100;
const unsigned P3D_VERTEX_LIST       = 0x3101;
const unsigned P3D_NORMAL_LIST       = 0x3102;
const unsigned P3D_UV_LIST           = 0x3103;
const unsigned P3D_COLOUR_LIST       = 0x3104;
const unsigned P3D_MATERIAL_LIST     = 0x3105;
const unsigned P3D_FACE_LIST         = 0x3106;
const unsigned P3D_PRIM_GROUP        = 0x3107;
const unsigned P3D_FACE_NORMAL_LIST  = 0x3108;

const unsigned P3D_EDGE_LIST         = 0x31a9;  // For experimental 'toon renderer

// V12 Skin format
const unsigned P3D_SKIN              = 0x3700;
const unsigned P3D_BONE_WEIGHTING    = 0x3701;
// TODO, remove this
const unsigned P3D_BONE_MAPPING      = 0x3701;


// V12 material format
const unsigned long P3D_MATERIAL          = 0x3120;
const unsigned long P3D_MATERIAL_PASS     = 0x3125;

// V14 shader format
const unsigned long P3D_SHADER               = 0x3130;
const unsigned long P3D_SHADER_DEFINITION    = 0x3131;
const unsigned long P3D_SHADER_TEXTURE_PARAM = 0x3132;
const unsigned long P3D_SHADER_INT_PARAM     = 0x3133;
const unsigned long P3D_SHADER_FLOAT_PARAM   = 0x3134;
const unsigned long P3D_SHADER_COLOUR_PARAM  = 0x3135;
const unsigned long P3D_SHADER_VECTOR_PARAM  = 0x3136;
const unsigned long P3D_SHADER_MATRIX_PARAM  = 0x3137;

// the 0x3000 MESH chunks are the old style mesh
const unsigned GEO_MESH              = 0x3000;
const unsigned GEO_VERTEX_LIST       = 0x3001;
const unsigned GEO_FACE_LIST_TEX     = 0x3005;
const unsigned GEO_UV_LIST           = 0x3006;
const unsigned GEO_NORMAL_LIST       = 0x3007;
const unsigned GEO_MATERIAL_GROUP    = 0x3008;
const unsigned GEO_HIT               = 0x3009;
const unsigned GEO_FLAGS             = 0x300A;
const unsigned GEO_ANIM_VERTEX_LIST  = 0x300B;
const unsigned GEO_ANIM_NORMAL_LIST  = 0x300C;

// chunks for colour by vertex information
const unsigned GEO_COLOUR_LIST        = 0x300D;
const unsigned GEO_VERTEX_COLOUR_LIST = 0x300E;

// chunks for binary properties file
const unsigned GEO_PRO_TEXTURE        = 0x3010;
const unsigned GEO_PRO_TEX_PAL        = 0x3011;
const unsigned GEO_PRO_TEX_PIXELS     = 0x3012;
const unsigned GEO_PRO_ALPHA_PIXELS   = 0x3013;


const unsigned GEO_PRO_MATERIAL       = 0x3020;
const unsigned GEO_PRO_MAT_COLOUR     = 0x3021;
const unsigned GEO_PRO_MAT_TEXTURE    = 0x3022;
const unsigned GEO_PRO_MAT_TRANSP     = 0x3023;
const unsigned GEO_PRO_MAT_BLENDMODE  = 0x3024;


const unsigned P3D_BMP_IMAGE          = 0x3040;
const unsigned P3D_SPRITE             = 0x3041;

// Tri Strips
const unsigned P3D_TRI_STRIP_MESH     = 0x3200;
const unsigned P3D_TRI_STRIP          = 0x3201;

// BACKGROUND (for things like background maps and colours from MAX)
const unsigned P3D_BACKGROUND         = 0x3300;

// Image references

const unsigned P3D_BMP_IMAGE_REF      = 0x3400;

// New texture format

const unsigned P3D_TEXTURE            = 0x3500;

// chunks for H-spline
const unsigned P3D_HSPLINE            = 0x3E00;

const unsigned P3D_HS_SB_LIST         = 0x3E10;
const unsigned P3D_HS_STORAGE_BLOCK   = 0x3E11;

const unsigned P3D_HS_GN_LIST         = 0x3E30;
const unsigned P3D_HS_GRAFTING_NODE   = 0x3E31;

const unsigned P3D_HS_CONTRIB_LIST    = 0x3E40;
const unsigned P3D_HS_CONTRIBUTOR     = 0x3E41;

const unsigned P3D_HS_EDGE_LIST       = 0x3E50;
const unsigned P3D_HS_EDGE            = 0x3E51;

const unsigned P3D_HS_OFFSET_LIST     = 0x3E60;
const unsigned P3D_HS_OFFSET          = 0x3E61;
const unsigned P3D_HS_OFFSET_ADD      = 0x3E62;
const unsigned P3D_HS_OFFSET_TANGENT  = 0x3E63;
const unsigned P3D_HS_OFFSET_JOINT    = 0x3E64;
const unsigned P3D_HS_OFFSET_PHANTOM  = 0x3E65;
const unsigned P3D_HS_OFFSET_FRAME    = 0x3E66;

const unsigned P3D_HS_COPY_LIST       = 0x3E70;
const unsigned P3D_HS_COPY_CN         = 0x3E71;

const unsigned P3D_HS_CONTROL_NODE    = 0x3E81;

const unsigned P3D_HS_CCPATCH_LIST    = 0x3E90;
const unsigned P3D_HS_CCPATCH         = 0x3E91;

const unsigned P3D_HS_REF_FRAME_LIST  = 0x3EA0;
const unsigned P3D_HS_REF_CN          = 0x3EA1;

const unsigned P3D_HSTREE               = 0x3EF0;
const unsigned P3D_HSTREE_JOINT         = 0x3EF1;
const unsigned P3D_HSTREE_MAPPED_HSTREE = 0x3EF2;
const unsigned P3D_HSTREE_MAPPING       = 0x3EF3;
const unsigned P3D_HSTREE_REST_POSE     = 0x3EF4;
const unsigned P3D_HSTREE_PARENT_INDEX  = 0x3EF5;

// H-Spline Stitcher Chunks

const unsigned P3D_HS_STITCHER          = 0x3F00;
const unsigned P3D_HS_STITCH            = 0x3F01;
const unsigned P3D_HS_STITCH_PATCH      = 0x3F02;
const unsigned P3D_HS_STITCH_PATCHLIST  = 0x3F03;
const unsigned P3D_HS_STITCH_TARGETLIST = 0x3F04;
const unsigned P3D_HS_STITCH_SKIN       = 0x3F05;

// H-Spline tessellation

const unsigned P3D_HS_TESSELLATION      = 0x3F10;
const unsigned P3D_HS_INDEX_MAPPING     = 0x3F11;

const unsigned P3D_HS_SKIN              = 0x3F20;
const unsigned P3D_HS_SKIN_OFFSET_GROUP = 0x3F21;
const unsigned P3D_HS_SKIN_CONNECT      = 0x3F22;
const unsigned P3D_HS_SKIN_VERT_CONNECT = 0x3F23;

const unsigned P3D_HS_POLYSKIN          = 0x3F30;

const unsigned P3D_HS_OFFSET_ANIM       = 0x3F40;
const unsigned P3D_HS_ANIM_CHANNEL      = 0x3F41;
const unsigned P3D_HS_CHANNEL_OFFSET_DYNAMIC  = 0x3F42;
const unsigned P3D_HS_CHANNEL_OFFSET_STATIC   = 0x3F43;

// animation
const unsigned GEO_ANIM               = 0x4001;
const unsigned GEO_ANIM_JOINT         = 0x4002;
const unsigned GEO_ANIM_TRANSL_LIST   = 0x4003;
const unsigned GEO_ANIM_ROTATE_LIST   = 0x4004;
const unsigned GEO_ANIM_QUAT_ROTATE_LIST = 0x4010;
const unsigned GEO_ANIM_SCALE_LIST    = 0x4005;
const unsigned GEO_ANIM_CLUT          = 0x4006;

const unsigned P3D_DEFORM_POLYSKIN           = 0x4A88;
const unsigned P3D_DEFORM_POLYSKIN_JOINT     = 0x4A89;
const unsigned P3D_DEFORM_POLYSKIN_STATE     = 0x4A8A;

// hybrid animation
const unsigned GEO_COMPOSITE_ANIM     = 0x4007;
const unsigned GEO_ANIM_TEX           = 0x4008;

const unsigned GEO_ANIM_ROOT_TRANS    = 0x4009;

// vertex animation
const unsigned GEO_ANIM_VERT          = 0x400A;
const unsigned GEO_ANIM_VERT_SPHERE   = 0x400B;
const unsigned GEO_ANIM_VERT_FRAMES   = 0x400C;

// compressed vertex animation
const unsigned GEO_ANIM_CVERT         = 0x400D;
const unsigned GEO_ANIM_CVERT_SPHERE  = 0x400E;
const unsigned GEO_ANIM_CVERT_FRAMES  = 0x400F;

// animation preferred hierarchy types
const unsigned GEO_ANIM_TREETYPE      = 0x4011;
const unsigned TREETYPE_UNDEFINED     = 0;
const unsigned TREETYPE_STREE         = 1;
const unsigned TREETYPE_ETREE         = 2;
const unsigned TREETYPE_MTREE         = 3;
const unsigned TREETYPE_HSTREE        = 4;

const unsigned ANIM_SEQ               = 0x4012;

const unsigned P3D_VIZ_ANIM           = 0x4020;
const unsigned P3D_VIZ_ANIM_DATA      = 0x4021;

// uv animation
const unsigned P3D_UV_ANIM            = 0x4030;
const unsigned P3D_UV_ANIM_FRAMES     = 0x4031;

// cbv animation
const unsigned P3D_CBV_ANIM              = 0x4040;
const unsigned P3D_CBV_ANIM_FRAMES       = 0x4041;
const unsigned P3D_CBV_PARAM_ANIM        = 0x4050;
const unsigned P3D_CBV_PARAM_ANIM_FRAMES = 0x4051;
// event animation
const unsigned P3D_EVENT_ANIM            = 0x4060;
const unsigned P3D_EVENT_ANIM_EVENT      = 0x4061;
const unsigned P3D_EVENT_ANIM_DATA       = 0x4062;


// Simple Scene Chunks
// const unsigned P3D_SIMPLE_SCENE           = 0x4510;
// const unsigned P3D_SIMPLE_SCENE_NODE      = 0x4511;
// SimpleScene renamed to CompoundMesh, June 13, 2000
// same hex id's
//const unsigned P3D_COMPOUND_MESH             = 0x4510;
//const unsigned P3D_COMPOUND_MESH_NODE        = 0x4511;
// New "UberSkin" called the tCompositeDrawable


// ALL these deformer chunks (0x4410 - 0x441C) are deprecated
// Skin Deformation Chunks (reserved 0x4410 to 0x4420)
const unsigned P3D_JOINT_STATE                 = 0x4410;
const unsigned P3D_STATE_VERTEX_MAP            = 0x4411;
const unsigned P3D_STATE_CHANNEL               = 0x4412;
const unsigned P3D_JOINT_DEFORMER              = 0x4413;
const unsigned P3D_DEFORMER_JOINT_STATE_MAP    = 0x4414;

// old unused chunk IDs (TODO: remove them)
const unsigned P3D_DEFORM_SKIN                 = 0x4416;
const unsigned P3D_DEFORMER                    = 0x4417;
const unsigned P3D_DEFORMER_CHANNEL            = 0x4418;
const unsigned P3D_DEFORMER_JOINT_DRIVER       = 0x4419;
const unsigned P3D_DEFORMER_JOINT_DRIVER_DATA  = 0x441A;
const unsigned P3D_DEFORMER_DRIVER_MAP         = 0x441B;
const unsigned P3D_DEFORMER_CHANNEL_GROUP      = 0x441C;

// Tree Chunks
const unsigned MTR_MTREE              = 0x4100;
const unsigned MTR_MTREE_JOINT        = 0x4101;

const unsigned MTR_BILLBOARD          = 0x4110;

const unsigned STR_STREE              = 0x4120;
const unsigned STR_STREE_JOINT        = 0x4121;
const unsigned STR_MAPPED_STREE       = 0x4122;
const unsigned STR_STREE_MAPPING      = 0x4123;
const unsigned STR_STREE_WEIGHTING    = 0x4124;
const unsigned STR_STREE_REST_POSE    = 0x4125;
const unsigned STR_STREE_PARENT_INDEX = 0x4126;

const unsigned ETR_ETREE              = 0x4140;
const unsigned ETR_ETREE_JOINT        = 0x4141;

// V11 Animation chunks
const unsigned P3D_TRAN_ANIM            = 0x4200;
const unsigned P3D_JOINT                = 0x4202;
const unsigned P3D_TIME_INDEX           = 0x4203;
const unsigned P3D_JOINT_NAMES          = 0x4204;
const unsigned P3D_JOINT_INFO           = 0x4205;

const unsigned P3D_KEYLIST_1DOF         = 0x4210;
const unsigned P3D_KEYLIST_2DOF         = 0x4211;
const unsigned P3D_KEYLIST_3DOF         = 0x4212;
const unsigned P3D_KEYLIST_1DOF_ANGLE   = 0x4213;
const unsigned P3D_KEYLIST_2DOF_ANGLE   = 0x4214;
const unsigned P3D_KEYLIST_3DOF_ANGLE   = 0x4215;
const unsigned P3D_KEYLIST_QUAT         = 0x4217;
const unsigned P3D_KEYLIST_ROT          = 0x4218;
const unsigned P3D_KEYLIST_SCALEMATRIX  = 0x4219;

const unsigned P3D_STATIC_ROT_KEYLIST   = 0x4220;
const unsigned P3D_STATIC_TRANS_KEYLIST = 0x4221;
const unsigned P3D_STATIC_SCALE_KEYLIST = 0x4222;
const unsigned P3D_STATIC_QUAT_KEYLIST  = 0x4223;
const unsigned P3D_STATIC_SCALEMATRIX   = 0x4224;
const unsigned P3D_STATIC_ROTATION      = 0x4225;
const unsigned P3D_STATIC_TRANSLATION   = 0x4226;

const unsigned P3D_KEYLIST_HS_OFF_3DOF  = 0x4230;

// generic parameter animation
const unsigned P3D_PARAM_ANIM           = 0x4300;
const unsigned P3D_PARAM_ANIM_PARAM     = 0x4301;
const unsigned P3D_HSPLINE_PARAM_ANIM   = 0x4400; // temporary for release 9

// parameter anim types             (not chunks!)
const unsigned P3D_USER_PARAM_ANIM       = 0x80000000; 
const unsigned P3D_PARAM_UNDEFINED_ANIM  = 0;
const unsigned P3D_PARAM_CAM_ANIM        = 0x001; 
const unsigned P3D_PARAM_TARGETCAM_ANIM  = 0x002; 
const unsigned P3D_PARAM_LIGHT_ANIM      = 0x101;
const unsigned P3D_PARAM_HS_ABS_OFF_ANIM = 0x201;
const unsigned P3D_PARAM_HS_REL_OFF_ANIM = 0x202;
const unsigned P3D_PARAM_CBV_ANIM        = 0x300;
// create your own param anims from 0x80000000 on

// Psx chunks for parameter animation
const unsigned PSX_KEYLIST_1DOF         = 0x4283;
const unsigned PSX_KEYLIST_3DOF         = 0x4285;

// chunks for progressive mesh information
const unsigned P3D_PM_MESH            = 0x5000;
const unsigned P3D_PM_SKIN            = 0x5001;
const unsigned P3D_PM_PRIM_GROUP      = 0x5002;
const unsigned P3D_PM_HISTORY         = 0x5005;
const unsigned P3D_PM_HISTORY_ELEMENT = 0x5006;

// chunks for view-dependant progressive meshes
const unsigned P3D_VDPM_GEO           = 0x5010;
const unsigned P3D_VDPM_STREE         = 0x5011;
const unsigned P3D_VDPM_HISTORY       = 0x5012;
const unsigned P3D_VDPM_JOINT_HISTORY = 0x5013;
const unsigned P3D_VDPM_HISTORY_LEVEL = 0x5014;

// param anim param types        (not chunks!)
const unsigned PARAM_UNDEFINED        = 0;

const unsigned PARAM_CAM_ROT          = 0x001;
const unsigned PARAM_CAM_TRANS        = 0x002;
const unsigned PARAM_CAM_FOV_X        = 0x011;
const unsigned PARAM_CAM_FOV_Y        = 0x012;
const unsigned PARAM_CAM_TARGET       = 0x021;
const unsigned PARAM_CAM_ROLL         = 0x022;

const unsigned PARAM_LIGHT_ROT        = 0x101;
const unsigned PARAM_LIGHT_TRANS      = 0x102;
const unsigned PARAM_LIGHT_COLOUR_RGB = 0x111;
const unsigned PARAM_LIGHT_COLOUR_HSV = 0x112;
const unsigned PARAM_LIGHT_MULTIPLIER = 0x120;


const unsigned PARAM_HS_OFFSET_TRANS  = 0x202;
// create your own param anim params from 0x8000000 on


// The following chunks are used to represent lights
const unsigned P3D_LIGHT               = 0x2300;
const unsigned P3D_LIGHT_EXCLUSION     = 0x2310;
const unsigned P3D_LIGHT_ATTENUATION   = 0x2320;
const unsigned P3D_LIGHT_SHADOW        = 0x2330;
const unsigned P3D_LIGHT_SHADOW_MAPPED = 0x2331;
const unsigned P3D_LIGHT_EXTRA         = 0x2340;

// Optic Effect chunks
const unsigned P3D_CORONA              = 0x2400;
const unsigned P3D_LENSFLARE           = 0x2401;
const unsigned P3D_LENSFLARE_FLARE     = 0x2402;

const unsigned SPACE_WORLD            = 1;
const unsigned SPACE_RELATIVE         = 0;

const unsigned ROT_X_AXIS             = 0;
const unsigned ROT_Y_AXIS             = 1;
const unsigned ROT_Z_AXIS             = 2;
const unsigned ROT_ARBITRARY          = 3;

const unsigned PSX_VERSION            = 0x6000;
const unsigned PSX_MATERIALS          = 0x6001;
const unsigned PSX_GEOMETRY           = 0x6002;
const unsigned PSX_COLLISION_GEOM     = 0x6003;
const unsigned PSX_VERT_ANIM          = 0x6004;
const unsigned PSX_NORM_ANIM          = 0x6005;
const unsigned PSX_CLUT_ANIM          = 0x6006;
const unsigned PSX_TEX_ANIM           = 0x6007;
const unsigned PSX_TEXTURE            = 0x6008;
const unsigned PSX_PRIMS              = 0x6009;
const unsigned PSX_TEX_ANIM_FRAMES    = 0x600A;
const unsigned PSX_TEX_ANIM_OFFSETS   = 0x600B;
const unsigned PSX_CLUT_ANIM_FRAMES   = 0x600C;
const unsigned PSX_CLUT_ANIM_OFFSETS  = 0x600D;

const unsigned PSX_UV_ANIM            = 0x600E;
const unsigned PSX_UV_ANIM_FRAMES     = 0x600F;
const unsigned PSX_UV_ANIM_OFFSETS    = 0x6010;

const unsigned PSX_CBV_ANIM           = 0x6011;
const unsigned PSX_CBV_ANIM_FRAMES    = 0x6012;
const unsigned PSX_CBV_ANIM_OFFSETS   = 0x6013;

const unsigned PSX_CBV_PARAM_ANIM         = 0x6021;
const unsigned PSX_CBV_PARAM_ANIM_FRAMES  = 0x6022;
const unsigned PSX_CBV_PARAM_ANIM_OFFSETS = 0x6023;

const unsigned PSX_SEQUENCE_ANIM      = 0x6040;

const unsigned PSX_MAIN_RAM_TEX_ANIM         = 0x6050;
const unsigned PSX_MAIN_RAM_TEX_ANIM_NAMES   = 0x6051;
const unsigned PSX_MAIN_RAM_TEX_ANIM_FRAMES  = 0x6052;

const unsigned PSX_STREE              = 0x6120;
const unsigned PSX_STREE_JOINT        = 0x6121;
const unsigned PSX_MAPPED_STREE       = 0x6122;
const unsigned PSX_STREE_WEIGHTING    = 0x6124;
const unsigned PSX_STREE_REST_POSE    = 0x6125;

const unsigned PSX_MTREE              = 0x6130;
const unsigned PSX_MTREE_JOINT        = 0x6131;

const unsigned PSX_ETREE              = 0x6140;
const unsigned PSX_ETREE_JOINT        = 0x6141;

const unsigned PSX_TRAN_ANIM          = 0x6400;

const unsigned PSX_TEXTURE_REF        = 0x6500;

const unsigned PSX_PRIM_OFFSETS       = 0x6600;

const unsigned PSX_MATRIX             = 0x6F00;

// the following are defined for flags bits in the binary properties file chunks
// GEO_PRO_MATERIAL flags
const unsigned MAT_USELIGHT =       0x00000001;
const unsigned MAT_GOURAUD =        0x00000002;
const unsigned MAT_COLOURBYVERTEX = 0x00000004;
const unsigned MAT_PERSP =          0x00000008;
const unsigned MAT_ENVMAP =         0x00000010;
const unsigned MAT_TILED =          0x00000020;
const unsigned MAT_STIPPLEALPHA =   0x00000040;
const unsigned MAT_MONOLIT =        0x00000080;
const unsigned MAT_ALPHA   =        0x00000100;
const unsigned MAT_WIREFRAME =      0x00000200;
const unsigned MAT_ALPHATEST =      0x00000400;

// GEO_PRO_MAT_BLENDMODE values
const unsigned MAT_BLEND_ZERO =                0x00000001;
const unsigned MAT_BLEND_ONE =                 0x00000002;
const unsigned MAT_BLEND_SRC =                 0x00000003;
const unsigned MAT_BLEND_ONE_MINUS_SRC =       0x00000004;
const unsigned MAT_BLEND_DEST =                0x00000005;
const unsigned MAT_BLEND_ONE_MINUS_DEST =      0x00000006;
const unsigned MAT_BLEND_SRCALPHA =            0x00000007;
const unsigned MAT_BLEND_ONE_MINUS_SRCALPHA =  0x00000008;
const unsigned MAT_BLEND_DESTALPHA =           0x00000009;
const unsigned MAT_BLEND_ONE_MINUS_DESTALPHA = 0x0000000a;
const unsigned MAT_BLEND_SRCALPHASATURATE =    0x0000000b;

// GEO_PRO_TEXTURE flags
const unsigned TEX_COLOURKEY =      0x00000001;
const unsigned TEX_ONEBITALPHA =    0x00000002;

// Bits for the GEO_HIT chunk
const unsigned HIT_RENDER            = 1;
const unsigned HIT_FACE_COLLISION    = 2;
const unsigned HIT_BOX_COLLISION     = 4;
const unsigned HIT_SPHERE_COLLISION  = 8;

// BSP tree chunks
const unsigned BSP_TREE       = 0x9000;
const unsigned BSP_NODE_SPLIT = 0x9001;
const unsigned BSP_NODE_LEAF  = 0x9002;
const unsigned BSP_NODE_NULL  = 0x9003;

// Scenegraph chunks
const unsigned P3D_SG_SCENEGRAPH   = 0x9100;
const unsigned P3D_SG_ROOT         = 0x9101;
const unsigned P3D_SG_BRANCH       = 0x9102;
const unsigned P3D_SG_TRANSFORM    = 0x9103;
const unsigned P3D_SG_DRAWABLE     = 0x9104;
const unsigned P3D_SG_CAMERA       = 0x9105;
const unsigned P3D_SG_LIGHTGROUP   = 0x9106;
const unsigned P3D_SG_ATTACHMENT   = 0x9107;
const unsigned P3D_SG_ATTACHMENTPOINT  = 0x9108;
const unsigned P3D_SG_VISIBILITY   = 0x9109; 

const unsigned P3D_SG_TRANSFORM_CONTROLLER  = 0x9151;

// chunks for physics
const unsigned PHY_OBJ_OLD                = 0xC000;
const unsigned PHY_OBJ                    = 0xC111;
const unsigned PHY_OBJ_IMAT               = 0xC001;
const unsigned PHY_OBJ_COLLEL             = 0xC002;
const unsigned PHY_OBJ_COLLEL_SPHERE      = 0xC003;
const unsigned PHY_OBJ_COLLEL_CYL         = 0xC004;
const unsigned PHY_OBJ_COLLEL_OBBOX       = 0xC005;
const unsigned PHY_OBJ_COLLEL_WALL        = 0xC006;
const unsigned PHY_OBJ_COLLEL_BBOX        = 0xC007;
const unsigned PHY_VECTOR                 = 0xC010;
const unsigned PHY_OBJ_JOINT              = 0xC011;
const unsigned PHY_OBJ_JOINT_DOF          = 0xC012;
const unsigned PHY_OBJ_SELFCOLL           = 0xC020;
const unsigned PHY_OBJ_SELFCOLL_ITEM      = 0xC021;

const unsigned PHY_FOOTSTEPS              = 0xC100;

const unsigned PHY_FLEX_GEOM              = 0xC200;
const unsigned PHY_FLEX_JOINT             = 0xC201;
const unsigned PHY_FLEX_PARAM             = 0xC210;
const unsigned PHY_FLEX_FIX_PARTICLE      = 0xC211;
const unsigned PHY_FLEX_MAP_VL            = 0xC212;
const unsigned PHY_FLEX_TRI_MAP           = 0xC213;
const unsigned PHY_FLEX_EDGE_MAP          = 0xC214;
const unsigned PHY_FLEX_EDGE_LEN          = 0xC215;
const unsigned PHY_FLEX_COLL_JOINT        = 0xC216;
const unsigned PHY_FLEX_JOINT_DEF         = 0xC220;

const unsigned PHY_LINK                   = 0xC320;
const unsigned PHY_LINK_IK                = 0xC321;
const unsigned PHY_LINK_REACH             = 0xC322;
const unsigned PHY_LINK_TRACKER           = 0xC323;
const unsigned PHY_LINK_TARGET            = 0xC330;
const unsigned PHY_TARGET_NODE            = 0xC331;
const unsigned PHY_TARGET_POSE            = 0xC332;

// TWADDLER CHUNKS

// the Twaddler world
const unsigned TW_WORLD    = 0xB000;

// Twaddler objects
const unsigned TW_OBJ_POINT   = 0xB001;
const unsigned TW_OBJ_LINE    = 0xB002;
const unsigned TW_OBJ_MESH    = 0xB003;
const unsigned TW_OBJ_ICON    = 0xB004;
const unsigned TW_OBJ_VOLUME  = 0xB005;
const unsigned TW_OBJ_SPHERE  = 0xB006;
const unsigned TW_OBJ_LIGHT   = 0xB007;

const unsigned TW_BLOCK_VOLUME   = 0xBB00;
#endif
#endif

