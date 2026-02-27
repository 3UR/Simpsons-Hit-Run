//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/matrixstack.hpp>
#include <radmath/radmath.hpp>

using namespace RadicalMathLibrary;

rmt::Matrix tMatrixStack::tmp;
rmt::Vector tMatrixStack::tmpv;

tMatrixStack::tMatrixStack(pddiRenderContext* context)
{
    pddi_context = context;
    LoadIdentity();
}

void tMatrixStack::RotateX(float x)
{
    tmp.Identity();
    tmp.FillRotateX(x);
    Multiply(tmp);
}

void tMatrixStack::RotateY(float y)
{
    tmp.Identity();
    tmp.FillRotateY(y);
    Multiply(tmp);
}

void tMatrixStack::RotateZ(float z)
{
    tmp.Identity();
    tmp.FillRotateZ(z);
    Multiply(tmp);
}

void tMatrixStack::RotateXYZ(float x, float y, float z)
{
    tmp.Identity();
    tmp.FillRotateXYZ(x,y,z);
    Multiply(tmp);
}

void tMatrixStack::RotateYZX(float x, float y, float z)
{
    tmp.Identity();
    tmp.FillRotateYZX(x,y,z);
    Multiply(tmp);
}

void tMatrixStack::Translate(const rmt::Vector& trans)
{
    tmp.Identity();
    tmp.Row(3) = trans;
    Multiply(tmp);
}

void tMatrixStack::Translate(float x, float y, float z)
{
    tmpv.x = x;
    tmpv.y = y;
    tmpv.z = z;
    Translate(tmpv);
}

void tMatrixStack::Scale(float x, float y, float z)
{
    tmp.Identity();
    tmp.FillScale(x,y,z);
    Multiply(tmp);
}

