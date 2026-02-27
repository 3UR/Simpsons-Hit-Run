//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


/*
 * poser/posedriver.hpp
 */

#ifndef POSER_POSEDRIVER_HPP
#define POSER_POSEDRIVER_HPP


#include <p3d/entity.hpp>
#include <poser/priority.hpp>


namespace poser
{


class Pose;


class PoseDriver: public tEntity
{
public:

    PoseDriver();
    
    virtual int GetMinimumJointIndex() const
        { return 0; }
    virtual int GetPriority() const
        { return PRIORITY_DEFAULT; }
    
    bool IsEnabled() const
        { return m_IsEnabled; }
    virtual void SetIsEnabled(bool isEnabled)
        { m_IsEnabled = isEnabled; }
    
    // deltaTime in seconds
    virtual void Advance(float deltaTime) = 0;
    virtual void Update(Pose* pose) = 0;
    
protected:
    
    virtual ~PoseDriver();
    
private:
    
    bool m_IsEnabled;
};


} // namespace poser


#endif
