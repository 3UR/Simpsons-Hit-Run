//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _ATENUM_HPP_
#define _ATENUM_HPP_

#define FOURCC(ch0, ch1, ch2, ch3) \
                     ((unsigned)(unsigned char)(ch0) | ((unsigned)(unsigned char)(ch1) << 8) |       \
                     ((unsigned)(unsigned char)(ch2) << 16) | ((unsigned)(unsigned char)(ch3) << 24 ))

namespace Pure3DAnimationTypes
{
    const unsigned UNDEFINED                        = FOURCC(0,0,0,0);
    const unsigned ANIMATED_OBJECT_AOBJ             = FOURCC('A','O','B','J');         
    const unsigned TEXTURE_TEX                      = FOURCC('T','E','X',0);
    const unsigned CAMERA_CAM                       = FOURCC('C','A','M',0);
    const unsigned LIGHT_LITE                       = FOURCC('L','I','T','E');
    const unsigned EXPRESSION_EXP                   = FOURCC('E','X','P',0);
    const unsigned POSE_TRANSFORM_PTRN              = FOURCC('P','T','R','N');
    const unsigned POSE_VISIBILITY_PVIS             = FOURCC('P','V','I','S');
    const unsigned SCENEGRAPH_TRANSFORM_STRN        = FOURCC('S','T','R','N');
    const unsigned SCENEGRAPH_VISIBILITY_SVIS       = FOURCC('S','V','I','S');
    const unsigned EVENT_EVT                        = FOURCC('E','V','T',0);
    const unsigned BILLBOARD_QUAD_GROUP_BQG         = FOURCC('B','Q','G',0);
    const unsigned EFFECT_EFX                       = FOURCC('E','F','X',0);
    const unsigned VERTEX_VRTX                      = FOURCC('V','R','T','X');
    const unsigned SHADER_SHAD                      = FOURCC('S','H','A','D');
}

namespace Pure3DAnimationChannels
{
    namespace ParticleSystem
    {
        namespace Particle 
        {
            const unsigned SPEED_OVER_LIFE_SPOL     = FOURCC('S','P','O','L');
            const unsigned WEIGHT_OVER_LIFE_WEOL    = FOURCC('W','E','O','L');
            const unsigned TEXTURE_OVER_LIFE_TEOL   = FOURCC('T','E','O','L');
            const unsigned COLOUR_OVER_LIFE_COOL    = FOURCC('C','O','O','L');
            const unsigned ALPHA_OVER_LIFE_ALOL     = FOURCC('A','L','O','L');
            const unsigned SIZE_OVER_LIFE_SZOL      = FOURCC('S','Z','O','L');
            const unsigned SPIN_OVER_LIFE_SNOL      = FOURCC('S','N','O','L');
        }

        namespace Emitter
        {
            const unsigned VISIBILITY_VIS       = FOURCC('V','I','S', 0);
            const unsigned TRANSLATION_TRA      = FOURCC('T','R','A',0);
            const unsigned ROTATION_ROT         = FOURCC('R','O','T',0);
            const unsigned EMISSION_RATE_RAT    = FOURCC('R','A','T',0);
            const unsigned NUMBER_PARTICLES_NUM = FOURCC('N','U','M',0);
            const unsigned LIFE_LIF             = FOURCC('L','I','F',0);
            const unsigned LIFE_VAR_LIFV        = FOURCC('L','I','F','V');
            const unsigned SPEED_SPD            = FOURCC('S','P','D',0);
            const unsigned SPEED_VAR_SPDV       = FOURCC('S','P','D','V');
            const unsigned WEIGHT_WGT           = FOURCC('W','G','T',0);
            const unsigned WEIGHT_VAR_WGTV      = FOURCC('W','G','T','V');
            const unsigned GRAVITY_GRA          = FOURCC('G','R','A',0);
            const unsigned DRAG_DRA             = FOURCC('D','R','A',0);
            const unsigned COLOUR_CLR           = FOURCC('C','L','R',0);
            const unsigned COLOUR_VAR_CLRV      = FOURCC('C','L','R','V');
            const unsigned ALPHA_ALP            = FOURCC('A','L','P',0);
            const unsigned ALPHA_VAR_ALPV       = FOURCC('A','L','P','V');
            const unsigned SIZE_SIZ             = FOURCC('S','I','Z',0);
            const unsigned SIZE_VAR_SIZV        = FOURCC('S','I','Z','V');
            const unsigned SPIN_SPI             = FOURCC('S','P','I',0);
            const unsigned SPIN_VAR_SPIV        = FOURCC('S','P','I','V');
        }

        namespace Generator
        {
            const unsigned HORIZ_SPREAD_HOSP    = FOURCC('H','O','S','P');
            const unsigned VERT_SPREAD_VESP     = FOURCC('V','E','S','P');
            const unsigned RADIAL_VAR_RDVA      = FOURCC('R','D','V','A');
            const unsigned WIDTH_WDT            = FOURCC('W','D','T',0);
            const unsigned HEIGHT_HGT           = FOURCC('H','G','T',0);
            const unsigned RADIUS_RAD           = FOURCC('R','A','D',0);
        }
    }

    namespace BillboardObjects
    {
        const unsigned VISIBILITY_VIS       = FOURCC('V','I','S', 0);
        const unsigned TRANSLATION_TRAN     = FOURCC('T','R','A','N');
        const unsigned ROTATION_ROT         = FOURCC('R','O','T',0);
        const unsigned WIDTH_WDT            = FOURCC('W','D','T',0);
        const unsigned HEIGHT_HGT           = FOURCC('H','G','T',0);
        const unsigned DISTANCE_DIST        = FOURCC('D','I','S','T');
        const unsigned COLOUR_CLR           = FOURCC('C','L','R',0);
        const unsigned UVOFFSET_OFF         = FOURCC('O','F','F',0);
        const unsigned UVOFFSET_RANGE_ORNG  = FOURCC('O','R','N','G');
        const unsigned SOURCE_RANGE_SRNG    = FOURCC('S','R','N','G');
        const unsigned EDGE_RANGE_ERNG      = FOURCC('E','R','N','G');
    }

    namespace Pose
    {
        const unsigned TRANSLATION_TRAN  = FOURCC('T','R','A','N');
        const unsigned ROTATION_ROT      = FOURCC('R','O','T', 0);
    }

    namespace SceneGraph
    {
        const unsigned TRANSLATION_TRAN  = FOURCC('T','R','A','N');
        const unsigned ROTATION_ROT      = FOURCC('R','O','T', 0);
    }

    namespace Camera
    {
        const unsigned TRANSLATION_TRAN         = FOURCC('T','R','A','N');
        const unsigned LOOKVECTOR_LOOK          = FOURCC('L','O','O','K');
        const unsigned UPVECTOR_UP              = FOURCC('U','P', 0 , 0);
        const unsigned FIELDOFVIEW_FOV          = FOURCC('F','O','V', 0);
        // Added by Bryan Ewert on 22 Feb 2002
        const unsigned NEARCLIP_NCLP            = FOURCC('N','C','L','P');
        const unsigned FARCLIP_FCLP             = FOURCC('F','C','L','P');
    }

    namespace Light
    {
        const unsigned COLOUR_CLR           = FOURCC('C','L','R', 0);
        const unsigned PARAMETER_PARM       = FOURCC('P','A','R','M');
        const unsigned ENABLE_EABL          = FOURCC('E','A','B','L');
        // DIR, CONE and ATTN Added by Bryan Ewert on 19 Dec 2001
        const unsigned DIRECTION_DIR        = FOURCC('D','I','R', 0);
        const unsigned CONEANGLE_CONE       = FOURCC('C','O','N','E');
        const unsigned ATTENUATION_ATTN     = FOURCC('A','T','T','N');
    }

    namespace Visibility
    {
        const unsigned VISIBILITY_VIS    = FOURCC('V','I','S', 0);
    }

    namespace Texture
    {
        const unsigned TEXTURE_TEX = FOURCC('T','E','X', 0);
    }

    namespace Expression
    {
        const unsigned STATE_STE    = FOURCC('S','T','E', 0);
    }

    namespace Event
    {
        const unsigned EVENT_EVT    = FOURCC('E','V','T', 0);
    }

    namespace Quaternion
    {
        const unsigned ONE_DOT_FIFTEEN  = FOURCC('1','.','1','5');
    }

    namespace Vertex
    {
        const unsigned VERTEX_VRTX = FOURCC('V','R','T','X'); 
    }
}

namespace Pure3DVertexAnim{
    namespace Offset{
        const unsigned Coord  = FOURCC('P','O','S',  0);
        const unsigned Normal = FOURCC('N','R','M','L');
        const unsigned Colour = FOURCC('C','L','R',  0);
        const unsigned UV0    = FOURCC('U','V','0',  0);
        const unsigned UV1    = FOURCC('U','V','1',  0);
        const unsigned UV2    = FOURCC('U','V','2',  0);
        const unsigned UV3    = FOURCC('U','V','3',  0);
    }
}

#endif
