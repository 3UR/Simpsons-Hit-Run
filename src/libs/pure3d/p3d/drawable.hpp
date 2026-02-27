//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _DRAWABLE_HPP
#define _DRAWABLE_HPP

#include <p3d/entity.hpp>
#include <p3d/error.hpp>
#include <radmath/radmath.hpp>

class tTexture;
class tShader;

class tDrawable : public tEntity
{
public:
    class ShaderCallback
    {
    public:
        virtual tShader* Process(tShader*) = 0;
    };

    virtual void Display() = 0;    

    virtual void ProcessShaders(ShaderCallback&) { P3DASSERTMSG(0, "No process shader funciton defined for object",""); }

#ifndef RAD_RELEASE
    virtual void DisplayBoundingBox(tColour colour = tColour(0,255,0));
    virtual void DisplayBoundingSphere(tColour colour = tColour(0,255,0));
#endif

    virtual void GetBoundingBox(rmt::Box3D* box);
    virtual void GetBoundingSphere(rmt::Sphere* sphere);

protected:
    virtual ~tDrawable() {};
};

class tShaderIntBroadcast : public tDrawable::ShaderCallback
{
    public:
        tShaderIntBroadcast(unsigned p, int v) : param(p), val(v) {};
        virtual tShader* Process(tShader*);

    protected:
        unsigned param;
        int val;
};

class tShaderFloatBroadcast : public tDrawable::ShaderCallback
{
    public:
        tShaderFloatBroadcast(unsigned p, float v) : param(p), val(v) {};
        virtual tShader* Process(tShader*);

    protected:
        unsigned param;
        float val;
};

class tShaderColourBroadcast : public tDrawable::ShaderCallback
{
    public:
        tShaderColourBroadcast(unsigned p, tColour v) : param(p), val(v) {};
        virtual tShader* Process(tShader*);

    protected:
        unsigned param;
        tColour val;
};


class tShaderTextureBroadcast : public tDrawable::ShaderCallback
{
    public:
        tShaderTextureBroadcast(unsigned p, tTexture* v) : param(p), val(v) {};
        virtual tShader* Process(tShader*);

    protected:
        unsigned param;
        tTexture* val;
};


class tShaderVectorBroadcast : public tDrawable::ShaderCallback
{
    public:
        tShaderVectorBroadcast(unsigned p, const rmt::Vector& v) : param(p), val(v) {};
        virtual tShader* Process(tShader*);

    protected:
        unsigned param;
        rmt::Vector val;
};


class tShaderMatrixBroadcast : public tDrawable::ShaderCallback
{
    public:
        tShaderMatrixBroadcast(unsigned p, const rmt::Matrix& v) : param(p), val(v) {};
        virtual tShader* Process(tShader*);

    protected:
        unsigned param;
        rmt::Matrix val;
};
#endif // _DRAWABLE_HPP

