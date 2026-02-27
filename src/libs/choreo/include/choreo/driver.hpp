/*
 * choreo/driver.hpp
 */

#ifndef CHOREO_DRIVER_HPP
#define CHOREO_DRIVER_HPP


#include <p3d/entity.hpp>


namespace choreo
{


class Engine;
class Puppet;


//----------------------------------------------------------------------
// Engine driver interface
//----------------------------------------------------------------------

class Driver: public tEntity
{
public:

    virtual void Apply(Engine* engine) = 0;
};


//----------------------------------------------------------------------
// Puppet driver interface
//----------------------------------------------------------------------

class PuppetDriver: public Driver
{
public:

    virtual PuppetDriver* Clone() const = 0;
    
    virtual void Begin(Puppet* puppet) = 0;
    virtual void Advance(float deltaTime) = 0;
    virtual bool IsFinished() const = 0;
    virtual void End(Puppet* puppet) = 0;
};


//----------------------------------------------------------------------
// PuppetDriverFactory interface
//----------------------------------------------------------------------

class PuppetDriverFactory: public tEntity
{
public:

    virtual PuppetDriver* NewDriver() = 0;
};


} // namespace choreo


#endif
