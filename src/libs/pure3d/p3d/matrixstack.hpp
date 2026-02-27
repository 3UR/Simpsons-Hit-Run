//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _MATRIXSTACK_HPP_
#define _MATRIXSTACK_HPP_

#include <pddi/pddienum.hpp>
#include <p3d/p3dtypes.hpp>
#include <p3d/context.hpp>

class pddiRenderContext;

class tMatrixStack 
{
public:
    tMatrixStack(pddiRenderContext* context);

    inline void Load(const rmt::Matrix& m);
    inline void LoadIdentity(void);

    inline void Push(void);
    inline void Pop(void);
    inline void Multiply(const rmt::Matrix& matrix);

    // shortcuts: push + operation to top of stack
    inline void PushIdentity();
    inline void PushLoad(const rmt::Matrix& matrix);
    inline void PushMultiply(const rmt::Matrix& matrix);

    void RotateX(float x);
    void RotateY(float y);
    void RotateZ(float z);
    void RotateXYZ(float x, float y, float z);
    void RotateYZX(float x, float y, float z);

    void Translate(const rmt::Vector& trans);
    void Translate(float x, float y, float z);

    void Scale(float x, float y, float z);
    inline void Scale(const rmt::Vector& scale);
    inline void Scale(float scale);

    inline rmt::Matrix* GetMatrix(void);
    inline void TransformVector(rmt::Vector&,rmt::Vector*);

private:
    pddiRenderContext* pddi_context;

    static rmt::Matrix tmp;
    static rmt::Vector tmpv;
};


inline void tMatrixStack::Load(const rmt::Matrix& m)
{
    pddi_context->LoadMatrix(PDDI_MATRIX_MODELVIEW, (pddiMatrix*)(&m));
}

inline void tMatrixStack::LoadIdentity(void)
{
    pddi_context->IdentityMatrix(PDDI_MATRIX_MODELVIEW);
}

inline void tMatrixStack::Push(void)
{
    pddi_context->PushMatrix(PDDI_MATRIX_MODELVIEW);
}

inline void tMatrixStack::Pop(void)
{
    pddi_context->PopMatrix(PDDI_MATRIX_MODELVIEW);
}

inline void tMatrixStack::Multiply(const rmt::Matrix& matrix)
{
    pddi_context->MultMatrix(PDDI_MATRIX_MODELVIEW, (pddiMatrix*)(&matrix));
}

inline void tMatrixStack::PushIdentity()
{
    pddi_context->PushIdentityMatrix(PDDI_MATRIX_MODELVIEW);
}

inline void tMatrixStack::PushLoad(const rmt::Matrix& matrix)
{
    pddi_context->PushLoadMatrix(PDDI_MATRIX_MODELVIEW, (pddiMatrix*)(&matrix));
}

inline void tMatrixStack::PushMultiply(const rmt::Matrix& matrix)
{
    pddi_context->PushMultMatrix(PDDI_MATRIX_MODELVIEW, (pddiMatrix*)(&matrix));
}

inline void tMatrixStack::Scale(const rmt::Vector& scale)
{
    Scale(scale.x, scale.y, scale.z);
}

inline void tMatrixStack::Scale(float scale)
{
    Scale(scale, scale, scale);
}

inline void tMatrixStack::TransformVector(rmt::Vector& v, rmt::Vector* r)
{
    (*((rmt::Matrix*)pddi_context->GetMatrix(PDDI_MATRIX_MODELVIEW))).Transform(v, r);
}

inline rmt::Matrix* tMatrixStack::GetMatrix(void)
{
    return (rmt::Matrix*)pddi_context->GetMatrix(PDDI_MATRIX_MODELVIEW);
}

#endif

