#ifndef LOCATOR_TYPES_H
#define LOCATOR_TYPES_H

//HEY, when you add a new type, make sure to add a string for it's name too!!!

namespace LocatorType
{
    enum Type
    {
        EVENT,
        SCRIPT,
        GENERIC,
        CAR_START,
        SPLINE,
        DYNAMIC_ZONE,
        OCCLUSION,
        INTERIOR_ENTRANCE,
        DIRECTIONAL,
        ACTION,
        FOV,
        BREAKABLE_CAMERA,
        STATIC_CAMERA,
        PED_GROUP,
        COIN,
        SPAWN_POINT,
        
        NUM_TYPES
    };

    extern const char* const Name[NUM_TYPES];
}

#endif
