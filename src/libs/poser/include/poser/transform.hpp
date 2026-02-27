/*
 * poser/transform.hpp
 */

#ifndef POSER_TRANSFORM_HPP
#define POSER_TRANSFORM_HPP


#include <radmath/radmath.hpp>
#include <raddebug.hpp>


namespace poser
{


class Transform
{
public:

    Transform();
    Transform(const rmt::Matrix& m);
    Transform(const rmt::Quaternion& q,
              const rmt::Vector& v = rmt::Vector(0.0f, 0.0f, 0.0f));
    Transform(const rmt::Vector& v);
    
    void Identity();
    
    const rmt::Matrix& GetMatrix() const
    {
        rAssert(m_Dirty != DIRTY_MATRIX);
        return m_Matrix;
    }

    void SetMatrix(const rmt::Matrix& m);
    // only sets rotation part of matrix
    void SetMatrixRotation(const rmt::Matrix& m);
    
    const rmt::Vector& GetTranslation() const
        { return const_cast<Transform*>(this)->m_Matrix.Row(3); }
    void SetTranslation(const rmt::Vector& v)
        { m_Matrix.FillTranslate(v); }
    
    const rmt::Quaternion& GetQuaternion() const;
    void SetQuaternion(const rmt::Quaternion& q);
    
    void Blend(float alpha, const Transform& xForm);
    
    void Mult(const Transform& xForm) 
    {
        m_Matrix.Mult(xForm.GetMatrix());
        m_Dirty = DIRTY_QUATERNION;
    }

    void Invert();
    
private:
    
    rmt::Matrix m_Matrix;
    rmt::Quaternion m_Quaternion;
    
    enum Dirty
    {
        DIRTY_NONE,
        DIRTY_MATRIX,
        DIRTY_QUATERNION
    };
    
    Dirty m_Dirty;
};


} // namespace poser


#endif
