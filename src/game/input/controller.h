#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

namespace Input
{
    // Platform specific controller topology.
    #if defined( RAD_UWP )
    const static unsigned int MaxPorts = 4;
    const static unsigned int MaxSlots = 1;
    #endif
    #if defined( RAD_WIN32 )
    const static unsigned int MaxPorts = 4;
    const static unsigned int MaxSlots = 1;
    #endif

    // Platform specific max controllers.
    const static unsigned int MaxControllers = (MaxPorts * MaxSlots);

    #ifdef RAD_WIN32
        const static unsigned int NumExtraButtonsForSuperSprint = 7;
    #endif

    // Maximum number of physical buttons (in a UserController)
    #ifdef RAD_WIN32
    const static unsigned int MaxPhysicalButtons = 42 + NumExtraButtonsForSuperSprint;
    #else
    const static unsigned int MaxPhysicalButtons = 40;
    #endif

    // Maximum number of logical buttons (in a UserController)
    #ifdef RAD_WIN32
    const static unsigned int MaxLogicalButtons = 42 + NumExtraButtonsForSuperSprint;
    #else
    const static unsigned int MaxLogicalButtons = 40;
    #endif

    // Maximum number of logical controllers for a physical device
    const static unsigned int MaxMappables = 16;

    // Maximum number of control mappings for a logical controller
    const static unsigned int MaxMappings = 2;

    // Maximum number of physical keys that can be assigned to a virtual key
    // for a controller.
    #ifdef RAD_WIN32
    const static unsigned int MaxVirtualMappings = 2;
    #endif

    // Maximum number of rumble motors
    const static unsigned int MaxOutputMotor = 2;

    // Control system state (for keeping input roped in in certain gameplay states)
    enum ActiveState
    {
        ACTIVE_NONE         = 0,
        ACTIVE_GAMEPLAY     = 1 << 0,
        ACTIVE_FRONTEND     = 1 << 1,
        ACTIVE_SS_GAME      = 1 << 2,
        ACTIVE_FIRST_PERSON = 1 << 3,
        ACTIVE_ANIM_CAM     = 1 << 4,
        DEACTIVE_ANIM_CAM   = 0xfffffffe,
        ACTIVE_ALL      = 0xffffffff
    };

    // handy constant for a bunk controller / input
    enum
    {
        INVALID_CONTROLLERID = -1
    };
};



#endif
