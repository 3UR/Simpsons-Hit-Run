
#ifndef _SYMMATRIX_HPP_
#define _SYMMATRIX_HPP_

#include "radmath/radmath.hpp"

namespace sim
{

class SymMatrix
{
public:
    SymMatrix();
    SymMatrix(float r1, float r2, float r3, float r4, float r5, float r6);
    virtual ~SymMatrix();
    
    float xx;
    float xy;
    float xz;
    float yy;
    float yz;
    float zz;
    
    // set the SymMat to m using m's upper part, no check is done to verify that m is symetric
    void Set(rmt::Matrix& m);
    
    // other basic operation
    void Add(const SymMatrix& m);
    void Sum(const SymMatrix& ma, const SymMatrix& mb);
    void Scale(const float s);
    void Scale(const SymMatrix& a, const float s);
    void Mult(const SymMatrix& ma, const SymMatrix& mb);
    
    // fill m with the SymMat value
    void FillMatrix(rmt::Matrix& m) const;
    
    // transform the SymMat as m*mat*transpose(m)
    //void Transform(const rmt::Matrix& m);
    // transform the SymMat as transpose(m)*mat*m
    void TransformSelf(const rmt::Matrix& m);
    
    // transform the SymMat according to skew(v)*mat*skew(v)
    void TransformSelf(const rmt::Vector& v);
    
    // transform the vector r = v*m
    void TransformVector(const rmt::Vector& v, rmt::Vector& r) const;
    
    // invert the SymMat
    void Invert(const SymMatrix& a);
    void Invert() { SymMatrix tmp; tmp.Invert(*this); *this = tmp; }
    
    // set the SymMat to identity
    void Identity();
    
    // set the SymMat to 0
    void Zero();
    
};

} // sim

#endif // _SymMatrix_HPP_