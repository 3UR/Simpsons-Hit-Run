/*===========================================================================
    particleconstants.hpp
    28-Feb-2001 Created by Kevin Voon

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _PARTICLE_CONSTANTS_HPP
#define _PARTICLE_CONSTANTS_HPP

#include <radmath/radmath.hpp>
#include <p3d/p3dtypes.hpp>
#include <constants/psenum.hpp>

#ifndef FOURCC
#define FOURCC(ch0, ch1, ch2, ch3) \
             ((unsigned)(unsigned char)(ch0) | ((unsigned)(unsigned char)(ch1) << 8) |       \
             ((unsigned)(unsigned char)(ch2) << 16) | ((unsigned)(unsigned char)(ch3) << 24 ))
#endif

const unsigned PARTICLE_SYSTEM_FACTORY_VERSION = 0;
const unsigned PARTICLE_SYSTEM_VERSION = 0;
const unsigned BASE_PARTICLE_ARRAY_VERSION = 0;
const unsigned SPRITE_PARTICLE_ARRAY_VERSION = 0;
const unsigned DRAWABLE_PARTICLE_ARRAY_VERSION = 0;
const unsigned BASE_PARTICLE_EMITTER_FACTORY_VERSION = 0;
const unsigned SPRITE_PARTICLE_EMITTER_FACTORY_VERSION = 0;
const unsigned DRAWABLE_PARTICLE_EMITTER_FACTORY_VERSION = 0;
const unsigned PARTICLE_ANIMATION_VERSION = 0;
const unsigned EMITTER_ANIMATION_VERSION = 0;
const unsigned GENERATOR_ANIMATION_VERSION = 0;

const rmt::Vector DEFAULT_POSITION                  = rmt::Vector(0.0f,0.0f,0.0f);
const float       DEFAULT_LIFE                      = 30.0f;
const float       DEFAULT_LIFE_VARIATION            = 0.0f;
const float       DEFAULT_SPEED                     = 1.0f;
const float       DEFAULT_SPEED_VARIATION           = 0.0f;
const float       DEFAULT_WEIGHT                    = 1.0f;
const float       DEFAULT_WEIGHT_VARIATION          = 0;
const tColour     DEFAULT_COLOUR                    = tColour(255,255,255,255);
const char        DEFAULT_COLOUR_VARIATION          = char(0);
const char        DEFAULT_ALPHA                     = char(255);
const char        DEFAULT_ALPHA_VARIATION           = char(0);
const float       DEFAULT_SIZE                      = 1.0f;
const float       DEFAULT_SIZE_VARIATION            = 0.0f;
const float       DEFAULT_SPIN                      = 0.0f;
const float       DEFAULT_SPIN_VARIATION            = 0.0f;
const float       DEFAULT_EMISSION_RATE             = 10.0f;
const int         DEFAULT_MAX_NUM_PARTICLES         = 100;
const rmt::Vector DEFAULT_VELOCITY                  = rmt::Vector(0.0f,1.0f,0.0f);
const float       DEFAULT_GRAVITY                   = -0.0f;
const float       DEFAULT_DRAG                      = 0.0f;
const int         DEFAULT_NUM_TEX_FRAME             = 1;
const int         DEFAULT_TEX_FRAME_RATE            = 1;  //default number of frames to wait between texture swapping
const float       DEFAULT_PARTICLE_ANGLE_OFFSET     = 0.0f;
const int         DEFAULT_LOOK_UP_SIZE              = 20;

#endif

