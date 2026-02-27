/*
 * poser/transform.cpp
 */

#include <poser/transform.hpp>

namespace poser
{


Transform::Transform()
{
    Identity();
}

Transform::Transform(const rmt::Matrix& m)
{
    m_Matrix = m;
    m_Quaternion.Identity();
    m_Dirty = DIRTY_QUATERNION;
}

Transform::Transform(const rmt::Quaternion& q,
                     const rmt::Vector& v)
{
    m_Matrix.Identity();
    m_Matrix.FillTranslate(v);
    SetQuaternion(q);
}

Transform::Transform(const rmt::Vector& v)
{
    Identity();
    m_Matrix.FillTranslate(v);
}

void Transform::Identity()
{
    m_Matrix.Identity();
    m_Quaternion.Identity();
    m_Dirty = DIRTY_NONE;
}

void Transform::SetMatrix(const rmt::Matrix& m)
{
    m_Matrix = m;
    m_Dirty = DIRTY_QUATERNION;
}

void Transform::SetMatrixRotation(const rmt::Matrix& m)
{
    m_Matrix.Row(0) = m.Row(0);
    m_Matrix.Row(1) = m.Row(1);
    m_Matrix.Row(2) = m.Row(2);
    m_Dirty = DIRTY_QUATERNION;
}

const rmt::Quaternion& Transform::GetQuaternion() const
{
    if (m_Dirty == DIRTY_QUATERNION)
    {
        Transform* cthis = const_cast<Transform*>(this);
        cthis->m_Quaternion.BuildFromMatrix(cthis->m_Matrix);
        cthis->m_Dirty = DIRTY_NONE;
    }
    
    return m_Quaternion;
}

void Transform::SetQuaternion(const rmt::Quaternion& q)
{
    m_Quaternion = q;
    m_Quaternion.ConvertToMatrix(&m_Matrix);
    m_Dirty = DIRTY_NONE;
}

void Transform::Blend(float alpha, const Transform& xForm)
{
    // blend translation
    rmt::Vector blendVector = GetTranslation();
    blendVector.Scale(1.0f - alpha);
    
    rmt::Vector otherVector = xForm.GetTranslation();
    otherVector.Scale(alpha);
    
    blendVector.Add(otherVector);
    SetTranslation(blendVector);
    
    // blend rotation
    rmt::Quaternion blendQuat;
    blendQuat.Slerp(GetQuaternion(), xForm.GetQuaternion(), alpha);
    SetQuaternion(blendQuat);
}

void Transform::Invert()
{
    m_Matrix.InvertOrtho();
    m_Dirty = DIRTY_QUATERNION;
}


} // namespace poser


// End of file.
