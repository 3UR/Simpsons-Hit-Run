/*
 * choreo/constants.hpp
 */

#ifndef CHOREO_CONSTANTS_HPP
#define CHOREO_CONSTANTS_HPP


#include <radmath/radmath.hpp>


namespace choreo
{


// choreo script chunk id
static const unsigned SCRIPT_CHUNK_ID = 0x02f00f00;

// default epsilon value used for compares
static const float DEFAULT_EPSILON = 0.000001f;

// default world facing vector
extern const rmt::Vector DEFAULT_FACING_VECTOR;

// ground fixup flags
enum GroundFixupEnum
{
    GROUND_FIXUP_NEVER = 0,
    GROUND_FIXUP_ANIMATION_START,
    GROUND_FIXUP_ANIMATION_RUN,
    GROUND_FIXUP_ALWAYS
};


} // namespace choreo


#endif
