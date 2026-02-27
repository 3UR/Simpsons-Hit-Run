//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


/*
 * poser/priority.hpp
 */

#ifndef POSER_PRIORITY_HPP
#define POSER_PRIORITY_HPP


namespace poser
{


//---------------------------------------------------------------------------
// Global List of PoseDriver Priorities
//---------------------------------------------------------------------------

static const int PRIORITY_DEFAULT         = 0;

// sim library drivers priorities, 
// two drivers with same priority should not be used on the same joint.
static const int IKDriverPriority         = 5;
static const int TrackerJointPriority     = 5;
static const int FlexibleJointPriority    = 10;
static const int PhysicsJointPriority     = 10;
static const int PhysicsInertialJointPriority = 11;

} // namespace poser


#endif
