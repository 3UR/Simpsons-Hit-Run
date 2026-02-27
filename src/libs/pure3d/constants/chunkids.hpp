/*===========================================================================
    File:: chunksids.hpp

    Chunk codes for Pure3D data types
    
    A complete description of the allocation of ids is at the end of this
    file.

    Copyright (c)2001, 2002 Radical Entertainment, Ltd.
    All rights reserved.
===========================================================================*/

#ifndef _CHUNKIDS_HPP
#define _CHUNKIDS_HPP

/*===========================================================================
 Project Ranges

  0x00000000 - 0x00ffffff  Pure3D
  0x01000000 - 0x01ffffff  Foundation
  0x02000000 - 0x02ffffff  Hairclub
  0x03000000 - 0x03ffffff  Simpsons
  0x04000000 - 0x04ffffff  M2
  0x05000000 - 0x05ffffff  Squidney
  0x06000000 - 0x06ffffff  Penthouse
  0x07000000 - 0x07ffffff  Simulation
  0x08000000 - 0x08ffffff  SmartProp

  0xff000000 - 0xffffffff  Reserved
===========================================================================*/
namespace Pure3D
{
    // The top level wrapper chunk IDs
    const unsigned DATA_FILE                 = 0xFF443350;
    const unsigned DATA_FILE_SWAP            = 0x503344FF;       // P3D wrapper ID on big endian
    const unsigned DATA_FILE_COMPRESSED      = 0x5A443350;
    const unsigned DATA_FILE_COMPRESSED_SWAP = 0x5033445A;

    // Mesh System 0x00010000 - 0x00010fff
    namespace Mesh
    {
        const unsigned MESH              = 0x00010000;
        const unsigned SKIN              = 0x00010001;
        const unsigned PRIMGROUP         = 0x00010002;
        const unsigned BOX               = 0x00010003;
        const unsigned SPHERE            = 0x00010004;
        const unsigned POSITIONLIST      = 0x00010005;
        const unsigned NORMALLIST        = 0x00010006;
        const unsigned UVLIST            = 0x00010007;
        const unsigned COLOURLIST        = 0x00010008;
        const unsigned STRIPLIST         = 0x00010009;
        const unsigned INDEXLIST         = 0x0001000A;
        const unsigned MATRIXLIST        = 0x0001000B;
        const unsigned WEIGHTLIST        = 0x0001000C;
        const unsigned MATRIXPALETTE     = 0x0001000D;
        const unsigned OFFSETLIST        = 0x0001000E;
        const unsigned INSTANCEINFO      = 0x0001000F;
        const unsigned PACKEDNORMALLIST  = 0x00010010; 
        const unsigned VERTEXSHADER      = 0x00010011;
        const unsigned MEMORYIMAGEVERTEXLIST = 0x00010012;
        const unsigned MEMORYIMAGEINDEXLIST  = 0x00010013;
        const unsigned MEMORYIMAGEVERTEXDESCRIPTIONLIST = 0x00010014;
        const unsigned TANGENTLIST       = 0x00010015;
        const unsigned BINORMALLIST      = 0x00010016;
        const unsigned RENDERSTATUS      = 0x00010017;
        const unsigned EXPRESSIONOFFSETS = 0x00010018;
        const unsigned SHADOWSKIN        = 0x00010019;
        const unsigned SHADOWMESH        = 0x0001001A;
        const unsigned TOPOLOGY          = 0x0001001B;
        const unsigned MULTICOLOURLIST   = 0x0001001C;
    }
    // End Mesh System 
    
    // Shader System 0x00011000 - 0x00011fff
    namespace Shader
    {
        const unsigned SHADER            = 0x00011000;
        const unsigned SHADER_DEFINITION = 0x00011001;
        const unsigned TEXTURE_PARAM     = 0x00011002;
        const unsigned INT_PARAM         = 0x00011003;
        const unsigned FLOAT_PARAM       = 0x00011004;
        const unsigned COLOUR_PARAM      = 0x00011005;
        const unsigned VECTOR_PARAM      = 0x00011006;
        const unsigned MATRIX_PARAM      = 0x00011007;
    }
    
    // GameAddr 0x00012000 - 0x00012fff
    namespace GameAttr
    {
        const unsigned GAME_ATTR    = 0x00012000;
        const unsigned INT_PARAM    = 0x00012001;
        const unsigned FLOAT_PARAM  = 0x00012002;
        const unsigned COLOUR_PARAM = 0x00012003;
        const unsigned VECTOR_PARAM = 0x00012004;
        const unsigned MATRIX_PARAM = 0x00012005;
    }

    // Light 0x00013000-0x00013fff
    namespace Light
    {
        const unsigned LIGHT       = 0x00013000;
        const unsigned DIRECTION   = 0x00013001;
        const unsigned POSITION    = 0x00013002;
        const unsigned CONE_PARAM  = 0x00013003;
        const unsigned SHADOW      = 0x00013004;
        const unsigned PHOTON_MAP  = 0x00013005;

        // Decay range
        const unsigned DECAY_RANGE             = 0x00013006;
        const unsigned DECAY_RANGE_ROTATION_Y  = 0x00013007;

        const unsigned ILLUMINATION_TYPE = 0x00013008;
    }
    // End Light System
    
    // Locator 0x00014000-0x00014fff
    namespace Locator
    {
        const unsigned LOCATOR = 0x00014000;
    }
    
    // Particle System 0x00015000-0x00015fff
    namespace ParticleSystem
    {
        //0x00015000-0x000157ff v14 particle system specific chunks
        //!!! DON'T USE !!!

        //0x00015800-0x000158ff particle system specific chunks
        const unsigned SYSTEM_FACTORY           = 0x00015800;
        const unsigned SYSTEM                   = 0x00015801;

        const unsigned BASE_PARTICLE_ARRAY      = 0x00015802;
        const unsigned SPRITE_PARTICLE_ARRAY    = 0x00015803;
        const unsigned DRAWABLE_PARTICLE_ARRAY  = 0x00015804;

        const unsigned BASE_EMITTER_FACTORY     = 0x00015805;
        const unsigned SPRITE_EMITTER_FACTORY   = 0x00015806;
        const unsigned DRAWABLE_EMITTER_FACTORY = 0x00015807;

        const unsigned PARTICLE_ANIMATION       = 0x00015808;
        const unsigned EMITTER_ANIMATION        = 0x00015809;
        const unsigned GENERATOR_ANIMATION      = 0x0001580a;

        const unsigned INSTANCING_INFO          = 0x0001580b;
    }

    // Optic Effect 0x00016000-0x00016fff
    namespace OpticEffect
    {
        const unsigned CORONA_V14               = 0x00016000;
        const unsigned LENS_FLARE_PARENT_V14    = 0x00016001;
        const unsigned LENS_FLARE_V14           = 0x00016002;
        const unsigned VECTOR_V14               = 0x00016f00;

        const unsigned LENS_FLARE_GROUP         = 0x00016006;
        const unsigned LENS_FLARE               = 0x00016007;
    }

    // Billboard 0x00017000-0x00017fff
    namespace BillboardObject
    {
        const unsigned QUAD_V14         = 0x00017000;

        const unsigned QUAD             = 0x00017001;
        const unsigned QUAD_GROUP       = 0x00017002;
        const unsigned DISPLAY_INFO     = 0x00017003;
        const unsigned PERSPECTIVE_INFO = 0x00017004;
    }

    // Scrooby (Front End Subsystem) 0x00018000-0x00018fff
    namespace Scrooby
    {
        const unsigned PROJECT     = 0x00018000;
        const unsigned SCREEN      = 0x00018001;
        const unsigned PAGE        = 0x00018002;
        const unsigned LAYER       = 0x00018003;

        //objects
        const unsigned GROUP       = 0x00018004;      
        const unsigned MOVIE       = 0x00018005;      
        const unsigned MULTISPRITE = 0x00018006;      
        const unsigned MULTITEXT   = 0x00018007;      
        const unsigned P3DOBJECT   = 0x00018008;      
        const unsigned POLYGON     = 0x00018009;      
        const unsigned SPRITE      = 0x0001800A;

        //subobjects
        const unsigned STRINGTEXTBIBLE = 0x0001800B;
        const unsigned STRINGHARDCODED = 0x0001800C;

        //text bible objects
        const unsigned TEXTBIBLE = 0x0001800D;
        const unsigned LANGUAGE  = 0x0001800E;

        //resources
        const unsigned RESOURCEIMAGE        = 0x00018100;
        const unsigned RESOURCEPURE3D       = 0x00018101;
        const unsigned OLDRESOURCETEXTSTYLE = 0x00018102;
        const unsigned OLDRESOURCETEXTBIBLE = 0x00018103;
        const unsigned RESOURCETEXTSTYLE    = 0x00018104;
        const unsigned RESOURCETEXTBIBLE    = 0x00018105;
    }

    // Texture 0x00019000-0x00019fff
    namespace Texture
    {
        const unsigned TEXTURE        = 0x00019000;
        const unsigned IMAGE          = 0x00019001;
        const unsigned IMAGE_DATA     = 0x00019002;
        const unsigned IMAGE_FILENAME = 0x00019003;
        const unsigned VOLUME_IMAGE   = 0x00019004;
        const unsigned SPRITE         = 0x00019005;
    }

    // Animated Object 0x00020000-0x00020fff
    namespace AnimatedObject
    {
        const unsigned FACTORY   = 0x00020000;
        const unsigned OBJECT    = 0x00020001;
        const unsigned ANIMATION = 0x00020002;
    }

    // Animated Object 0x00021000-0x00021fff
    namespace Expression
    {
        const unsigned VERTEX_EXPRESSION       = 0x00021000;
        const unsigned VERTEX_EXPRESSION_GROUP = 0x00021001;
        const unsigned VERTEX_EXPRESSION_MIXER = 0x00021002;
    }

    // Font 0x00022000-0x00022fff
    namespace Font
    {
        const unsigned TEXTURE_FONT       = 0x00022000;
        const unsigned TEXTURE_GLYPH_LIST = 0x00022001;
        const unsigned IMAGE_FONT         = 0x00022002;
        const unsigned IMAGE_GLYPH_LIST   = 0x00022003;
    }

    // next free 0x00023000-0x00023fff

    // SceneGraph 0x00120100-0x001201ff
    namespace SceneGraph
    {
        const unsigned SCENEGRAPH      = 0x00120100;
        const unsigned ROOT            = 0x00120101;
        const unsigned BRANCH          = 0x00120102;
        const unsigned TRANSFORM       = 0x00120103;
        const unsigned VISIBILITY      = 0x00120104;
        const unsigned ATTACHMENT      = 0x00120105;
        const unsigned ATTACHMENTPOINT = 0x00120106;
        const unsigned DRAWABLE        = 0x00120107;
        const unsigned CAMERA          = 0x00120108;
        const unsigned LIGHTGROUP      = 0x00120109;
        const unsigned SORTORDER       = 0x0012010A;
    }

    // Animation 0x000121000-0x000121fff
    namespace Animation
    {
        // Animation Data 0x00121000-0x001210ff
        namespace AnimationData
        {
            const unsigned ANIMATION  = 0x00121000;
            const unsigned GROUP      = 0x00121001;
            const unsigned GROUP_LIST = 0x00121002;
            const unsigned SIZE       = 0x00121004;
        }

        // Channel Data 0x000121100-0x0001211ff
        namespace ChannelData
        {
            const unsigned FLOAT_1                  = 0x00121100;
            const unsigned FLOAT_2                  = 0x00121101;
            const unsigned VECTOR_1DOF              = 0x00121102;
            const unsigned VECTOR_2DOF              = 0x00121103;
            const unsigned VECTOR_3DOF              = 0x00121104;
            const unsigned QUATERNION               = 0x00121105;
            const unsigned STRING                   = 0x00121106;
            const unsigned ENTITY                   = 0x00121107;
            const unsigned BOOL                     = 0x00121108;
            const unsigned COLOUR                   = 0x00121109;
            const unsigned EVENT                    = 0x0012110A;
            const unsigned EVENT_OBJECT             = 0x0012110B;
            const unsigned EVENT_OBJECT_DATA        = 0x0012110C;
            const unsigned EVENT_OBJECT_DATA_IMAGE  = 0x0012110D;
            const unsigned INT                      = 0x0012110E;
            const unsigned QUATERNION_FORMAT        = 0x0012110F;
            const unsigned INTERPOLATION_MODE       = 0x00121110;
            const unsigned COMPRESSED_QUATERNION    = 0x00121111;
        }

        // Channel Data 0x000121200-0x0001212ff
        namespace FrameControllerData
        {
            const unsigned FRAME_CONTROLLER = 0x000121200;
        }

        // Chunk id from 0x000121300-0x0001213ff
        namespace VertexAnim
        {
            namespace OffsetList
            {
                const unsigned COLOUR               = 0x000121300;
                const unsigned VECTOR               = 0x000121301;
                const unsigned VECTOR2              = 0x000121302;
                const unsigned INDEX                = 0x000121303;
            }
            const unsigned KEY                      = 0x000121304;
 
        }
        namespace VertexSplineAnim
        {
            namespace List
            {
                const unsigned VECTOR               = 0x000121400;
                const unsigned VECTOR2              = 0x000121401;
            }
            const unsigned KEY                      = 0x000121402;
        }

    }
}  // Pure3D


// Simulation System 0x07000000 - 0x07ffffff
namespace Simulation
{
    // Collision System 0x07010000 - 0x07010fff
    namespace Collision
    {
        const unsigned OBJECT          = 0x07010000;
        const unsigned VOLUME          = 0x07010001;
        const unsigned SPHERE          = 0x07010002;
        const unsigned CYLINDER        = 0x07010003;
        const unsigned OBBOX           = 0x07010004;
        const unsigned WALL            = 0x07010005;
        const unsigned BBOX            = 0x07010006;
        const unsigned VECTOR          = 0x07010007;
        const unsigned SELFCOLLISION   = 0x07010020;
        const unsigned OWNER           = 0x07010021;
        const unsigned OWNERNAME       = 0x07010022;
        const unsigned ATTRIBUTE       = 0x07010023;
    }

    // Physics System 0x07011000 - 0x07011fff
    namespace Physics
    {
        const unsigned OBJECT          = 0x07011000;
        const unsigned IMAT            = 0x07011001;
        const unsigned VECTOR          = 0x07011002;

        const unsigned JOINT           = 0x07011020;
        const unsigned JOINT_DOF       = 0x07011021;
    }

    // Flexible System 0x07012000 - 0x07012fff
    namespace Flexible
    {
        const unsigned OBJECT            = 0x07012000;
        const unsigned OBJECT_PARAMETERS = 0x07012001;
        const unsigned FIX_PARTICLE      = 0x07012002;
        const unsigned MAP_VL            = 0x07012003;
        const unsigned TRI_MAP           = 0x07012004;
        const unsigned EDGE_MAP          = 0x07012005;
        const unsigned EDGE_LEN          = 0x07012006;
        const unsigned OBJECT_LAMBDA     = 0x07012007;
        const unsigned JOINT             = 0x07012020;
        const unsigned JOINT_PARAMETERS  = 0x07012021;
        const unsigned JOINT_DEFINITION  = 0x07012022;
        const unsigned JOINT_LAMBDA      = 0x07012023;
    }

    // IK System 0x07013000 - 0x07013fff
    namespace IK
    {
    }

    // Link System 0x07014000 - 0x07014fff
    namespace Link
    {
        const unsigned IK              = 0x07014000;
        const unsigned REACH           = 0x07014001;
        const unsigned TRACKER         = 0x07014002;
        const unsigned TARGET          = 0x07014003;
        const unsigned TARGET_NODE     = 0x07014004;
        const unsigned TARGET_POSE     = 0x07014005;
    }

    // Param System 0x07015000 - 0x07015fff
    namespace Parameters
    {
        const unsigned ENVIRONMENT     = 0x07015000;
        const unsigned PHYSICS         = 0x07015001;
    }
} // Simulation


// SmartProp System 0x08000000 - 0x08ffffff
namespace SmartProp
{
    // Collision System 0x08010000 - 0x08010fff
    const unsigned SMARTPROP                    = 0x08010000;
    const unsigned SMARTPROPSTATEDATA           = 0x08010001;
    const unsigned SMARTPROPVISIBILITYDATA      = 0x08010002;
    const unsigned SMARTPROPFRAMECONTROLLERDATA = 0x08010003;
    const unsigned SMARTPROPEVENTDATA           = 0x08010004;
    const unsigned SMARTPROPCALLBACKDATA        = 0x08010005;
    const unsigned SMARTPROPAPPLIEDFORCE        = 0x08010006;
    const unsigned SMARTPROPBREAKABLE           = 0x08010007;
    const unsigned SMARTPROPEXTRAATTRIBUTE      = 0x08010008;
} // SmartProp
// StateProp System 0x08000000 - 0x08ffffff
namespace StateProp
{
    const unsigned STATEPROP                    = 0x08020000;
    const unsigned STATEPROPSTATEDATA           = 0x08020001;
    const unsigned STATEPROPVISIBILITYDATA      = 0x08020002;
    const unsigned STATEPROPFRAMECONTROLLERDATA = 0x08020003;
    const unsigned STATEPROPEVENTDATA           = 0x08020004;
    const unsigned STATEPROPCALLBACKDATA        = 0x08020005;
} // StateProp


namespace MemorySection
{
    const unsigned MEMORYSECTION = 0xFFFF0000;
}

#endif

/*===========================================================================

 Global allocation
 ~~~~~~~~~~~~~~~~~

 A chunk ID is 32 bits. Each project will be allocated a 24 bit block of
 IDs, giving them roughly one million unique IDs for their project
 specific chunks. The top level chunk ID list is the only piece of
 of information that has to be managed across teams. When a new project
 comes along, they simply request the next block on the list. There is
 room to accomodate 256 "project blocks" using this scheme. The Pure3D
 team will be responsible for maintaining this list. 
 The final block is reserved for file format control information.
 
 Management of Project Blocks
 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 
 While it is up to the individual teams to manage their chunk IDs in any
 manner they choose, it is recommended that a controlled, centralised
 scheme is used to manage the ID space. The system should allow
 sub-systems to grow to accomodate new chunk types. Usually, it is
 desirable for related chunks to have contiguous chunk IDs. One approach
 is to use an allocation scheme similar to how IP addresses are
 allocated. When a programmer is designing a new sub-system, they "carve
 off" a section of the chunk space, reserving a block of IDs for that
 system. The designer may also further sub-divide that space into
 additional regions for related sub-systems.

 Example reserve sizes:

 Huge sub-system - 4096 (0x000 - 0xfff)
 Major sub-system - 256 (0x000 - 0x0ff)
 Minor sub-system - 64 (0x000 - 0x03f)

 Example chunk ID file:

 namespace Pure3D
 {
 //-------- Animation System 0x00100000-0x00100fff

     //-------- Particle Systems 0x00100000-0x001000ff
     const unsigned PARTICLE_SYSTEM                  = 0x00100000;
     const unsigned EMITTER                          = 0x00100001;
     const unsigned SIMPLE_EMITTER                   = 0x00100002;
     const unsigned EMITTER_LIFE_CHANNEL             = 0x00100003;
     const unsigned EMITTER_SPEED_CHANNEL            = 0x00100004;
     const unsigned EMITTER_WEIGHT_CHANNEL           = 0x00100005;
     const unsigned EMITTER_LIFE_VAR_CHANNEL         = 0x00100006;
     const unsigned EMITTER_SPEED_VAR_CHANNEL        = 0x00100007;
     const unsigned EMITTER_EMISSION_RATE_CHANNEL    = 0x00100008;
     const unsigned S_EMITTER_SIZE_CHANNEL           = 0x00100009;
     const unsigned S_EMITTER_SPIN_CHANNEL           = 0x0010000A;
     const unsigned S_EMITTER_TRANSPARENCY_CHANNEL   = 0x0010000B;
     const unsigned S_EMITTER_COLOUR_CHANNEL         = 0x0010000C;
     const unsigned S_EMITTER_SIZE_VAR_CHANNEL       = 0x0010000D;
     const unsigned S_EMITTER_SPIN_VAR_CHANNEL       = 0x0010000E;
 
     // next free 0x00100100
 
 // next free 0x00101000
 };

 In the above example, the animation system has 4096 IDs allocated to it.
 Within this space, the particles module has 256 IDs allocated to it. The
 comments in the file document the size of each reserved block, and the
 location of the next free block. 


 Notes on the Pure3D Block
 ~~~~~~~~~~~~~~~~~~~~~~~~~
 
 0x00000000 - 0x00001fff  unused
 0x00002000 - 0x0000ff04  Pure3D legacy
 0x0000ff05 - 0x0000ffff  reserved

 The first 65536 entires of the Pure3D team block contains the old file
 format chunks, and will not be used in the new system. 

==========================================================================*/
