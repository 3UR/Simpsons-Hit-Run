/*
 * choreo/rootblender.cpp
 */

#include <choreo/rootblender.hpp>
#include <choreo/rootblenddriver.hpp>
#include <choreo/utility.hpp>
#include <choreo/constants.hpp>

#include <poser/pose.hpp>
#include <poser/joint.hpp>

#include <p3d/error.hpp>

#include <string.h>


namespace choreo
{


//----------------------------------------------------------------------
// class RootBlender
//----------------------------------------------------------------------

RootBlender::RootBlender(int rootJointIndex,
                         int maxBlendedDrivers,
                         int maxBlendedPriorities):
    m_RootJointIndex(rootJointIndex),

    m_GroundPoint(0.0f, 0.0f, 0.0f),
    m_IsGroundPointSet(false),
    m_PrevGroundFixupWeight(0.0f),
    m_GroundFixupWeight(0.0f),

    m_DeltaTime(0.0f),

    m_Velocity(0.0f, 0.0f, 0.0f),
    m_TurningVelocity(0.0f),

    m_MaxRootDrivers(maxBlendedDrivers),
    m_RootDriverCount(0),
    m_MaxBlendedPriorities(maxBlendedPriorities)
{
    P3DASSERT(m_RootJointIndex >= 0);

    m_RootDrivers = new RootDriverStruct [m_MaxRootDrivers];
    m_TranslatePriorities = new BlendPriority<rmt::Vector> [m_MaxBlendedPriorities];
    m_RotatePriorities = new BlendPriority<rmt::Quaternion> [m_MaxBlendedPriorities];
    m_GroundFixupPriorities = new BlendPriority<float> [m_MaxBlendedPriorities];

    m_ParentTransform.Identity( );
}

RootBlender::~RootBlender()
{
    for (int i = m_RootDriverCount - 1; i >= 0; --i)
    {
        m_RootDrivers[i].rootDriver->Release();
    }
    
    delete[] m_GroundFixupPriorities;
    delete[] m_RotatePriorities;
    delete[] m_TranslatePriorities;
    delete[] m_RootDrivers;
}

void RootBlender::SetParentTransform(const poser::Transform& parentTransform)
{
    m_ParentTransform = parentTransform;
}

void RootBlender::SetRootTransform(const poser::Transform& rootTransform)
{
    m_RootTransform = rootTransform;
    m_Velocity.Clear();
    m_TurningVelocity = 0.0f;
}

void RootBlender::SetPosition(const rmt::Vector& position)
{
    m_RootTransform.SetTranslation(position);
    m_Velocity.Clear();
}

float RootBlender::GetOrientation() const
{
    rmt::Vector facing;
    m_RootTransform.GetMatrix().RotateVector(DEFAULT_FACING_VECTOR, &facing);
    return GetWorldAngle(facing.x, facing.z);
}

void RootBlender::SetOrientation(float radians)
{
    rmt::Quaternion q;
    q.BuildFromAngleY(radians);
    m_RootTransform.SetQuaternion(q);
    m_TurningVelocity = 0.0f;
}

float RootBlender::GetPrevOrientation() const
{
    rmt::Vector facing;
    m_PrevRootTransform.GetMatrix().RotateVector(DEFAULT_FACING_VECTOR, &facing);
    return GetWorldAngle(facing.x, facing.z);
}

rmt::Vector RootBlender::GetFacingVector() const
{
    rmt::Vector facing;
    m_RootTransform.GetMatrix().RotateVector(DEFAULT_FACING_VECTOR, &facing);
    return facing;
}

void RootBlender::SetFacingVector(const rmt::Vector& facingVector)
{
    static const rmt::Vector inv(0.0f, 0.0f, 1.0f);

    if (DEFAULT_FACING_VECTOR.Equals(facingVector))
    {
        rmt::Quaternion q;
        q.Identity();
        m_RootTransform.SetQuaternion(q);
    }
    else if(inv.Equals(facingVector))
    {
        rmt::Matrix m;
        m.Identity();
        m.FillRotateY(rmt::PI);
        m.Row(3) = m_RootTransform.GetTranslation();
        m_RootTransform.SetMatrix(m);
    }
    else
    {
        rmt::Matrix m;
        m.m[0][3] = 0.0f;
        m.m[1][3] = 0.0f;
        m.m[2][3] = 0.0f;
        m.m[3][3] = 1.0f;
        m.Row(3) = m_RootTransform.GetTranslation();
        MatrixFillRotateVectorToVector(m, DEFAULT_FACING_VECTOR, facingVector);
        m_RootTransform.SetMatrix(m);
    }
}

rmt::Vector RootBlender::GetPrevFacingVector() const
{
    rmt::Vector facing;
    m_PrevRootTransform.GetMatrix().RotateVector(DEFAULT_FACING_VECTOR, &facing);
    return facing;
}

void RootBlender::SetGroundPoint(const rmt::Vector& groundPoint)
{
    m_GroundPoint = groundPoint;
    m_IsGroundPointSet = true;
}

float RootBlender::GetGroundFixupWeight() const
{
    return m_GroundFixupWeight;
}

float RootBlender::GetPrevGroundFixupWeight() const
{
    return m_PrevGroundFixupWeight;
}

void RootBlender::WorldToRootSpace(const rmt::Vector& worldSpace, rmt::Vector& rootSpace)
{
    poser::Transform invXForm = m_RootTransform;
    invXForm.Invert();
    invXForm.GetMatrix().Transform(worldSpace, &rootSpace);
}

void RootBlender::RootToWorldSpace(const rmt::Vector& rootSpace, rmt::Vector& worldSpace)
{
    m_RootTransform.GetMatrix().Transform(rootSpace, &worldSpace);
}

bool RootBlender::Push(RootBlendDriver* rootDriver)
{
    // check to see if rootDriver is in current list
    // of previously pushed root drivers
    
    for (int i = 0; i < m_RootDriverCount; ++i)
    {
        RootDriverStruct* rDriver = (m_RootDrivers + i);
        
        if (rDriver->rootDriver == rootDriver)
        {
            // already in list, so set the "pushed" flag
            // for this frame and return
            rDriver->pushed = true;
            return true;
        }
    }
    
    // check to make sure we haven't maxed out the number of pushed drivers
    P3DASSERT(m_RootDriverCount < m_MaxRootDrivers);
    if (m_RootDriverCount >= m_MaxRootDrivers)
        return false;
    
    // append rootDriver to list of currently pushed drivers
    RootDriverStruct* rDriver = (m_RootDrivers + m_RootDriverCount);
    
    rDriver->rootDriver = rootDriver;
    rDriver->rootDriver->AddRef();
    
    rDriver->weight = 0.0f;
    rDriver->pushed = true;
    
    ++m_RootDriverCount;
    
    // inform root driver that it has been newly pushed
    rootDriver->Reset(this);
    
    return true;
}

void RootBlender::Advance(float deltaTime)
{
    int i = 0;
    
    while (i < m_RootDriverCount)
    {
        RootDriverStruct* rDriver = (m_RootDrivers + i);
        RootBlendDriver* rootDriver = rDriver->rootDriver;
        
        if (rDriver->pushed)
        {
            // driver is easing in, or is maxed out
            float blendIn = rootDriver->GetRootBlendInTime();
            
            if (blendIn <= 0.0f)
            {
                // blendIn == 0.0f means a pop
                rDriver->weight = 1.0f;
            }
            else
            {
                // ease in
                rDriver->weight += (deltaTime / blendIn);
                
                // clamp weight at 1.0f
                if (rDriver->weight > 1.0f)
                {
                    rDriver->weight = 1.0f;
                }
            }
            
            // clear pushed flag for next frame
            rDriver->pushed = false;
            
            ++i;
        }
        else
        {
            // driver is easing out
            float blendOut = rootDriver->GetRootBlendOutTime();
            
            if (blendOut <= 0.0f)
            {
                // blendOut == 0.0f means a pop
                rDriver->weight = 0.0f;
            }
            else
            {
                // ease out
                rDriver->weight -= (deltaTime / blendOut);
            }
            
            if (rDriver->weight <= 0.0f)
            {
                // driver weight has dropped to zero; dump it!
                rootDriver->Release();
                
                // dec driver count
                --m_RootDriverCount;
                
                // shift driver list over
                memmove(m_RootDrivers + i,
                        m_RootDrivers + i + 1,
                        (m_RootDriverCount - i) * sizeof(RootDriverStruct));
                
                // note:  don't inc i!
            }
            else
            {
                // driver weight still above zero, so
                // we're still ramping down...
                ++i;
            }
        }
    }
    
    m_DeltaTime += deltaTime;
}

void RootBlender::UpdateDriverTranslation(RootBlendDriver* rootDriver,
                                          float rootWeight,
                                          int rootPriority)
{
    BlendDriverWithContext
        <rmt::Vector, const poser::Transform&, RootBlendDriver>
        (m_RootTransform, rootDriver, &RootBlendDriver::GetRootTranslation, rootWeight, rootPriority,
         m_TranslatePriorities, m_MaxBlendedPriorities, m_TranslatePriorityCount);
}

void RootBlender::UpdateDriverRotation(RootBlendDriver* rootDriver,
                                       float rootWeight,
                                       int rootPriority)
{
    BlendDriverWithContext
        <rmt::Quaternion, const poser::Transform&, RootBlendDriver>
        (m_RootTransform, rootDriver, &RootBlendDriver::GetRootRotation, rootWeight, rootPriority,
         m_RotatePriorities, m_MaxBlendedPriorities, m_RotatePriorityCount);
}

void RootBlender::UpdateDriverGroundFixup(RootBlendDriver* rootDriver,
                                          float rootWeight,
                                          int rootPriority)
{
    BlendDriverNoContext
        <float, RootBlendDriver>
        (rootDriver, &RootBlendDriver::GetRootGroundFixup, rootWeight, rootPriority,
         m_GroundFixupPriorities, m_MaxBlendedPriorities, m_GroundFixupPriorityCount);
}

void RootBlender::UpdateTranslation()
{
    rmt::Vector translate;

    if (BlendPriorities<rmt::Vector>(m_TranslatePriorities,
                                     m_TranslatePriorityCount,
                                     translate))
    {
        // stuff translate value into root transform
        m_RootTransform.SetTranslation(translate);
    }
}

void RootBlender::UpdateRotation()
{
    rmt::Quaternion rotate;

    if (BlendPriorities<rmt::Quaternion>(m_RotatePriorities,
                                     m_RotatePriorityCount,
                                     rotate))
    {
        // stuff translate value into root transform
        m_RootTransform.SetQuaternion(rotate);
    }
}

void RootBlender::UpdateGroundFixup()
{
    m_PrevGroundFixupWeight = m_GroundFixupWeight;
    m_GroundFixupWeight = 0.0f;

    // stuff ground fixup value
    BlendPriorities<float>(m_GroundFixupPriorities,
                           m_GroundFixupPriorityCount,
                           m_GroundFixupWeight);
}

void RootBlender::Update(poser::Pose* pose)
{
    // make sure root index is valid
    P3DASSERT(m_RootJointIndex < pose->GetJointCount());
    if (m_RootJointIndex >= pose->GetJointCount())
        return;
    
    // store previous root position
    m_PrevRootTransform = m_RootTransform;
    
    // initialize priority slots
    m_TranslatePriorityCount = 0;
    m_RotatePriorityCount = 0;
    m_GroundFixupPriorityCount = 0;
    
    // iterate over each driver
    for (int i = 0; i < m_RootDriverCount; ++i)
    {
        RootDriverStruct* rDriver = (m_RootDrivers + i);
        RootBlendDriver* rootDriver = rDriver->rootDriver;

        bool hasTranslate = rootDriver->HasRootTranslation();
        bool hasRotate = rootDriver->HasRootRotation();
        bool hasGroundFixup = rootDriver->HasRootGroundFixup();

        if (hasTranslate || hasRotate || hasGroundFixup)
        {
            float rootWeight = (rDriver->weight * rootDriver->GetRootWeight());
        
            if (rootWeight > 0.0f)
            {
                int rootPriority = rootDriver->GetRootPriority();
            
                // fill in and blend within priority slots
                if (hasTranslate)
                {
                    UpdateDriverTranslation(rootDriver, rootWeight, rootPriority);
                }
                if (hasRotate)
                {
                    UpdateDriverRotation(rootDriver, rootWeight, rootPriority);
                }
                if (hasGroundFixup)
                {
                    UpdateDriverGroundFixup(rootDriver, rootWeight, rootPriority);
                }
            }
        }
    }
    
    // blend across priority slots
    UpdateTranslation();
    UpdateRotation();
    UpdateGroundFixup();
    
    // stuff final root transform into joint
    poser::Joint* joint = pose->GetJoint(m_RootJointIndex);
    poser::Transform theTransform = m_RootTransform;
    theTransform.Mult( m_ParentTransform );
    joint->SetObjectTransform(theTransform);
    joint->SetWorldTransform(theTransform);
    
    // calculate velocity and turning velocity
    if (!rmt::Epsilon(m_DeltaTime, 0.0f))
    {
        float invDeltaTime = 1.0f / m_DeltaTime;
        
        m_Velocity = GetPosition();
        m_Velocity.Sub(GetPrevPosition());
        m_Velocity.Scale(invDeltaTime);
        
        m_TurningVelocity = GetOrientation();
        m_TurningVelocity -= GetPrevOrientation();
        m_TurningVelocity *= invDeltaTime;
    }
    
    m_DeltaTime = 0.0f;
}

void RootBlender::ClearRootDrivers(void)
{
    for(int i = 0; i < m_RootDriverCount; i++)
    {
        tRefCounted::Release(m_RootDrivers[i].rootDriver);
    }
    m_RootDriverCount = 0;
}


//----------------------------------------------------------------------
// class RootFixupDriver
//----------------------------------------------------------------------

RootFixupDriver::RootFixupDriver(RootBlender* rootBlender):
    m_DeltaTime(0.0f)
{
    P3DASSERT(rootBlender != 0);
    m_RootBlender = rootBlender;
    m_RootBlender->AddRef();
}

RootFixupDriver::~RootFixupDriver()
{
    m_RootBlender->Release();
}

void RootFixupDriver::Advance(float dt)
{
    m_DeltaTime += dt;
}

void RootFixupDriver::Update(poser::Pose* pose)
{
    if (m_RootBlender->IsGroundPointSet())
    {
        if (m_RootBlender->GetGroundFixupWeight() > 0.0f)
        {
            // adjust root transform for ground
            poser::Transform theTransform = m_RootBlender->GetRootTransform();
            theTransform.Mult( m_RootBlender->GetParentTransform( ) );
            rmt::Vector rootPos = theTransform.GetTranslation();
            rmt::Vector groundPos = rootPos;
            groundPos.y = m_RootBlender->GetGroundPoint().y;
            rmt::Vector finalPos;
            BlendVector(rootPos, groundPos, m_RootBlender->GetGroundFixupWeight(), finalPos);
            
            rmt::Vector velocity = m_RootBlender->GetVelocity();
            rmt::Matrix invMat = m_RootBlender->GetParentTransform( ).GetMatrix( );
            invMat.InvertOrtho( );
            finalPos.Transform( invMat );
            m_RootBlender->SetPosition(finalPos);
            
            // recalculate velocity
            if (!rmt::Epsilon(m_DeltaTime, 0.0f))
            {
                velocity = m_RootBlender->GetPosition();
                velocity.Sub(m_RootBlender->GetPrevPosition());
                velocity.Scale(1.0f / m_DeltaTime);
            }
            m_RootBlender->SetVelocity(velocity);
            
            {
                // stuff fixed up root transform into joint
                poser::Joint* joint = pose->GetJoint(m_RootBlender->GetRootJointIndex());
                poser::Transform theTransform = m_RootBlender->GetRootTransform();
                theTransform.Mult( m_RootBlender->GetParentTransform( ) );
                joint->SetObjectTransform( theTransform );
                joint->SetWorldTransform( theTransform );
            }
        }
    }
    else
    {
        m_RootBlender->SetGroundPoint(m_RootBlender->GetPosition());
    }
    
    m_RootBlender->SetIsGroundPointSet(false);
    m_DeltaTime = 0.0f;
}


} // namespace choreo


// End of file.
