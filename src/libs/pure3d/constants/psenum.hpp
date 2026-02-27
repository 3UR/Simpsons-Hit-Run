//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _PSENUM_HPP_
#define _PSENUM_HPP_

#define FOURCC(ch0, ch1, ch2, ch3) \
                     ((unsigned)(unsigned char)(ch0) | ((unsigned)(unsigned char)(ch1) << 8) |       \
                     ((unsigned)(unsigned char)(ch2) << 16) | ((unsigned)(unsigned char)(ch3) << 24 ))

namespace p3dBillboardConstants
{
    const unsigned UNDEFINED = FOURCC(0,0,0,0);

    namespace BillboardMode
    {
        const unsigned ALL_AXIS     = FOURCC('A','A','X',0);
        const unsigned X_AXIS       = FOURCC('X','A','X',0);
        const unsigned Y_AXIS       = FOURCC('Y','A','X',0);
        const unsigned LOCAL_X_AXIS = FOURCC('L','X','A','X');
        const unsigned LOCAL_Y_AXIS = FOURCC('L','Y','A','X');
        const unsigned NO_AXIS      = FOURCC('N','O','A','X');
    }

    namespace CutOffMode
    {
        const unsigned NONE             = FOURCC('N','0','N','E');
        const unsigned SINGLE_SIDED     = FOURCC('S','N','G',0);
        const unsigned DOUBLE_SIDED     = FOURCC('D','B','L',0);
        const unsigned CAMERA           = FOURCC('C','A','M',0);
    }
}

namespace p3dParticleSystemConstants
{
    const unsigned UNDEFINED = FOURCC(0,0,0,0);

    namespace ParticleType
    {
        const unsigned SPRITE    = FOURCC('S','P','R','T');
        const unsigned DRAWABLE  = FOURCC('D','R','A','W');
    }

    namespace ParticleAngleMode
    {
        const unsigned SPECIFIED        = FOURCC('S','P','E','C');
        const unsigned RANDOM           = FOURCC('R','A','N','D');
        const unsigned MOTION_ALIGNED   = FOURCC('M','O','A','L');
    }

    namespace GeneratorType
    {
        const unsigned POINT    = FOURCC('P','N','T',0);
        const unsigned PLANE    = FOURCC('P','L','N',0);
        const unsigned SPHERE   = FOURCC('S','P','H','R');
    }

    namespace ColourAnimMode
    {
        const unsigned NONE             = FOURCC('N','O','N','E');
        const unsigned BOTH             = FOURCC('B','O','T','H');
        const unsigned COLOUR           = FOURCC('C','O','L','R');
        const unsigned ALPHA            = FOURCC('A','L','P','H');
    }

    namespace TextureAnimMode
    {
        const unsigned NONE             = FOURCC('N','O','N','E');
        const unsigned LINEAR           = FOURCC('L','I','N',0);
        const unsigned SPECIFIED        = FOURCC('S','P','E','C');
        const unsigned RANDOM           = FOURCC('R','A','N','D');
    }

    namespace EmitterBias
    {
        const unsigned NUM_PARTICLES    = FOURCC('N','P','R','T');
        const unsigned EMISSION         = FOURCC('E','M','I','T');
        const unsigned LIFE             = FOURCC('L','I','F','E');
        const unsigned SPEED            = FOURCC('S','P','D',0);
        const unsigned WEIGHT           = FOURCC('W','G','T',0);
        const unsigned GRAVITY          = FOURCC('G','R','A','V');
        const unsigned DRAG             = FOURCC('D','R','A','G');
        const unsigned SIZE             = FOURCC('S','I','Z','E');
        const unsigned SPIN             = FOURCC('S','P','I','N');
    }

    namespace GeneratorBias
    {
        const unsigned HORIZ_SPREAD = FOURCC('H','O','S','P');
        const unsigned VERT_SPREAD  = FOURCC('V','E','S','P');
        const unsigned RADIAL_VAR   = FOURCC('R','A','V','A');
        const unsigned WIDTH        = FOURCC('W','I','D',0);
        const unsigned HEIGHT       = FOURCC('H','G','T',0);
        const unsigned RADIUS       = FOURCC('R','A','D',0);
    }
}

#endif

