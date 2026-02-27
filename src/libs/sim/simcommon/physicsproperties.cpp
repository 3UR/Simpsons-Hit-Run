

#include "simcommon/physicsproperties.hpp"
#include "simcommon/simmath.hpp"
#include "p3d/utility.hpp"

namespace sim
{


//
// class PhysicsProperties
//

PhysicsProperties::PhysicsProperties()
{
    mFrictCoeffCGS = 0;
    mDensityCGS = 1.0f;
    mRestCoeff = 1.5f;
    mTangRestCoeff = 0.0f;
    Update();
}

PhysicsProperties::~PhysicsProperties()
{
}

void PhysicsProperties::Update()
{
    mFrictCoeff = mFrictCoeffCGS;//mFrictCoeffCGS * sT;
    mDensity = mDensityCGS * sM;
    //mDensity = mDensity / Sqr(sL);
    //mDensity = mDensity / sL;
}

void PhysicsProperties::SetFrictCoeffCGS(float f)
{
    mFrictCoeffCGS = f;
    mFrictCoeff = mFrictCoeffCGS;//mFrictCoeffCGS * sT;
}

void PhysicsProperties::SetRestCoeffCGS(float f)
{
    mRestCoeff = f;
}

void PhysicsProperties::SetTangRestCoeffCGS(float f)
{
    mTangRestCoeff = f;
}

void PhysicsProperties::SetDensityCGS(float f)
{
    mDensityCGS = f;
    Update();
}

/*
    Here are the rule for creating physics properties:

    The parameters are:

    RestCoeff, restitution coefficient, indicates the bounciness of the material.
        1.0: means not bouncy at all, a value lower than 0 not be physically plausible
        2.0: very bouncy, bounce as high as the initial, a value larger than 2 would not be physically plausible

    FrictCoeff, the friction coefficient.
        0.0: indicates no friction at all.
        The higher the value, the more friction there is.

    TangRestCoeff, the tangential restitution of coefficient. 
    Indicates the collision response tangential to the collision normal.
        -1: no response, this cause the friction to be null.
            -0.5 will cause the final relative tangential velocity (rtv) to be 0.5 the initial one. etc.
        0.: the rtv at the contact points is absorbed. They stick together. There is no sliding. etc.
            0.5 will cause the final relative tangential velocity (rtv) to be 0.5 the initial one in the opposite direction.
        1.0:The rtv is reversed. 
            A rotating rubber ball bouncing will see its rotation speed to be reversed if there is enough friction.
    *This parameter will act as long as the combined friction is high enough.

    Density:
        1.0 is water, the density is larger than 0.

    Methods are used in order to combine the parameters during a collision. Those combinations can be very complex and 
    a more aproprite implementation could be added later.
*/

PhysicsProperties* PhysicsProperties::GetPhysicsProperties(tEntityStore* store, const char* inName, float inRest, float inFrict, float inTangRest, float inDensity)
{
    PhysicsProperties* tmpprop = NULL;

    if (store)
        tmpprop = p3d::find<PhysicsProperties>(store, inName);
    else
        tmpprop = p3d::find<PhysicsProperties>(inName);

    if (!tmpprop)
    {
        tmpprop = new PhysicsProperties;
        tmpprop->SetRestCoeffCGS(inRest);
        tmpprop->SetFrictCoeffCGS(inFrict);
        tmpprop->SetTangRestCoeffCGS(inTangRest);
        tmpprop->SetDensityCGS(inDensity);
        tmpprop->SetName(inName);
        if( store )
        {
            bool collision = store->TestCollision( tmpprop->GetUID(), tmpprop );
            if( !collision )
            {
                store->Store( tmpprop );
            }
            else
            {
            }
        }
        else
        {
            tInventory* inv = tPlatform::GetPlatform()->GetActiveContext()->GetInventory();
            bool collision = inv->TestCollision( tmpprop );
            if( !collision )
            {
                inv->Store( tmpprop );
            }
            else
            {
            }
        }

    }
    return tmpprop;
}

PhysicsProperties* PhysicsProperties::FindPhysicsProperty(const char* stringData, tEntityStore* store)
{
    return PhysicsProperties::FindPhysicsProperty(tEntity::MakeUID(stringData), store);
}

PhysicsProperties* PhysicsProperties::FindPhysicsProperty(tUID uid, tEntityStore* store)
{
    return NULL;
    if (uid == tEntity::MakeUID("NoData"))
    {
        return NULL;
    }

    PhysicsProperties* tmpprop;

    if (store)
        tmpprop = p3d::find<PhysicsProperties>(store, uid);
    else
        tmpprop = p3d::find<PhysicsProperties>(uid);

    if (!tmpprop)
    {
        if (uid == tEntity::MakeUID("hardwood"))
        {
            tmpprop = PhysicsProperties::HardWoodProperties(store);
        }
        else if (uid == tEntity::MakeUID("softwood"))
        {
            tmpprop = PhysicsProperties::SoftWoodProperties(store);
        }
        else if (uid == tEntity::MakeUID("hardrubber"))
        {
            tmpprop = PhysicsProperties::HardRubberProperties(store);
        }
        else if (uid == tEntity::MakeUID("softrubber"))
        {
            tmpprop = PhysicsProperties::SoftRubberProperties(store);
        }
        else if (uid == tEntity::MakeUID( "hardmetal"))
        {
            tmpprop = PhysicsProperties::HardMetalProperties(store);
        }
        else if (uid == tEntity::MakeUID("softmetal"))
        {
            tmpprop = PhysicsProperties::SoftMetalProperties(store);
        }
        else if (uid == tEntity::MakeUID("concrete"))
        {
            tmpprop = PhysicsProperties::ConcreteProperties(store);
        }
        else if (uid == tEntity::MakeUID("sticky"))
        {
            tmpprop = PhysicsProperties::StickyProperties(store);
        }
        else if (uid == tEntity::MakeUID("icy"))
        {
            tmpprop = PhysicsProperties::IcyProperties(store);
        }
    }
    return tmpprop;
}

PhysicsProperties* PhysicsProperties::DefaultPhysicsProperties(tEntityStore* store)
{
    return GetPhysicsProperties(store, "defaultPhysicsProperties", 1.5f, 1.0f, -0.75f);
}

PhysicsProperties* PhysicsProperties::DefaultArtPhysicsProperties(tEntityStore* store)
{
    return GetPhysicsProperties(store, "defaultArtPhysicsProperties", 1.05f, 10.0f, -0.25f);
}

PhysicsProperties* PhysicsProperties::HardWoodProperties(tEntityStore* store)
{
    return GetPhysicsProperties(store, "hardwood", 1.5f, 2.0f, -0.75f, 0.97f);
}

PhysicsProperties* PhysicsProperties::SoftWoodProperties(tEntityStore* store)
{
    return GetPhysicsProperties(store, "softwood", 1.2f, 2.0f, -0.75f, 0.5f);
}

PhysicsProperties* PhysicsProperties::HardRubberProperties(tEntityStore* store)
{
    return GetPhysicsProperties(store, "hardrubber", 1.9f, 100.0f, 0.5f, 1.3f);
}

PhysicsProperties* PhysicsProperties::SoftRubberProperties(tEntityStore* store)
{
    return GetPhysicsProperties(store, "softrubber", 1.1f, 100.0f, 0.5f, 1.3f);
}

PhysicsProperties* PhysicsProperties::HardMetalProperties(tEntityStore* store)
{
    return GetPhysicsProperties(store, "hardmetal", 1.4f, 100.0f, -0.85f, 10.0f);
}

PhysicsProperties* PhysicsProperties::SoftMetalProperties(tEntityStore* store)
{
    return GetPhysicsProperties(store, "softmetal", 1.1f, 100.0f, -0.85f, 2.7f);
}

PhysicsProperties* PhysicsProperties::ConcreteProperties(tEntityStore* store)
{
    return GetPhysicsProperties(store, "concrete", 1.05f, 1000.0f, -0.8f, 2.3f);
}

PhysicsProperties* PhysicsProperties::StickyProperties(tEntityStore* store)
{
    return GetPhysicsProperties(store, "sticky", 1.05f, 100000.0f, 0.0f);
}

PhysicsProperties* PhysicsProperties::IcyProperties(tEntityStore* store)
{
    return GetPhysicsProperties(store, "icy", 1.05f, 0.0f, -1.0f); // at -1.0 for tanrest, friction is ignored.
}

//

float PhysicsProperties::CombinedRestCoeff(const PhysicsProperties* inProperties1, const PhysicsProperties* inProperties2)
{
    return rmt::Min(inProperties1->RestCoeff(), inProperties2->RestCoeff());
}

float PhysicsProperties::CombinedTangRestCoeff(const PhysicsProperties* inProperties1, const PhysicsProperties* inProperties2)
{
    return Average(inProperties1->TangRestCoeff(), inProperties2->TangRestCoeff());
}

float PhysicsProperties::CombinedFrictCoeff(const PhysicsProperties* inProperties1, const PhysicsProperties* inProperties2)
{
    return Average(inProperties1->FrictCoeff(), inProperties2->FrictCoeff());
}

} // sim
