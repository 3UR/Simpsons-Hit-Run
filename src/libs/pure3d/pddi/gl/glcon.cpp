//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

#include <pddi/gl/gl.hpp>
#include <pddi/gl/glcon.hpp>
#include <pddi/gl/gldev.hpp>
#include <pddi/gl/gldisplay.hpp>
#include <pddi/gl/gltex.hpp>
#include <pddi/gl/glmat.hpp>

#include <pddi/base/debug.hpp>
#include <math.h>
#include <string.h>
#include <SDL2/SDL.h>

#include <microprofile.h>

// vertex arrays rendering
GLenum primTypeTable[5] =
{
    GL_TRIANGLES, //PDDI_PRIM_TRIANGLES
    GL_TRIANGLE_STRIP, //PDDI_PRIM_TRISTRIP
    GL_LINES, //PDDI_PRIM_LINES
    GL_LINE_STRIP, // PDDI_PRIM_LINESTRIP
    GL_POINTS, //PDDI_PRIM_POINTS
};

static inline void FillGLColour(pddiColour c, float* f)
{
    f[0] = float(c.Red()) / 255;
    f[1] = float(c.Green()) / 255;
    f[2] = float(c.Blue()) / 255;
    f[3] = float(c.Alpha()) / 255;
}

// extensions
class pglExtContext : public pddiExtGLContext 
{
public:
    pglExtContext(pglDisplay* d) : display(d) {}

    void BeginContext()
    {
        display->BeginContext();
    }

    void EndContext()
    {
        display->EndContext();
    }

private:
    pglDisplay* display;
};

class pglExtGamma : public pddiExtGammaControl
{
public:
    pglExtGamma(pglDisplay* d) { display = d;}

    void SetGamma(float r, float g, float b)     {display->SetGamma(r,g,b);}
    void GetGamma(float *r, float *g, float *b)  {display->GetGamma(r,g,b);}

protected:
    pglDisplay* display;
};

pglContext::pglContext(pglDevice* dev, pglDisplay* disp) : pddiBaseContext((pddiDisplay*)disp,(pddiDevice*)dev)
{
    device = dev;
    display = disp;

    device->AddRef();
    display->AddRef();
    disp->SetContext(this);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
    DefaultState();
    contextID = 0;

    extContext = new pglExtContext(display);
    extGamma = new pglExtGamma(display);

    defaultShader = new pglMat(this);
    defaultShader->AddRef();
}

pglContext::~pglContext()
{
    defaultShader->Release();

    delete extContext;
    delete extGamma;

    display->SetContext(NULL);
    display->Release();
    device->Release();
}

// frame synchronisation
void pglContext::BeginFrame()
{
    pddiBaseContext::BeginFrame();

    SDL_GL_SetSwapInterval(display->GetForceVSync() ? 1 : 0);

    if(display->HasReset())
    {
        contextID++;

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glColor4f(1,1,1,1);

        glEnable(GL_DITHER);

        if(display->CheckExtension("GL_EXT_separate_specular_color"))
        {
            glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
        }

        SyncState(0xffffffff);
    }

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    glMatrixMode( GL_MODELVIEW );
}

void pglContext::EndFrame()
{
    pddiBaseContext::EndFrame();
}

// buffer clearing
void pglContext::Clear(unsigned bufferMask)
{
    pddiBaseContext::Clear(bufferMask);

    int myClearMask = 0;
    myClearMask |= (bufferMask & PDDI_BUFFER_COLOUR) ? GL_COLOR_BUFFER_BIT : 0;
    myClearMask |= (bufferMask & PDDI_BUFFER_DEPTH) ? GL_DEPTH_BUFFER_BIT : 0;
    myClearMask |= (bufferMask & PDDI_BUFFER_STENCIL) ? GL_STENCIL_BUFFER_BIT : 0;

    glClearDepth(state.viewState->clearDepth);
    glClearColor(float(state.viewState->clearColour.Red())/255.0f, 
                     float(state.viewState->clearColour.Green())/255.0f, 
                     float(state.viewState->clearColour.Blue())/255.0f,
                     float(state.viewState->clearColour.Alpha())/255.0f);
    glClearStencil(state.viewState->clearStencil);
    glClear(myClearMask);
}

void pglContext::SetupHardwareProjection(void)
{
    switch(state.viewState->projectionMode)
    {
        case PDDI_PROJECTION_DEVICE :
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, display->GetWidth(),
                      display->GetHeight(), 0,
                      (state.viewState->camera.nearPlane),(state.viewState->camera.farPlane));
            glViewport(0, 0, display->GetWidth(), display->GetHeight());
            glMatrixMode( GL_MODELVIEW );
            break;

        case PDDI_PROJECTION_ORTHOGRAPHIC :
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(-0.5,  0.5,
                      -((1/state.viewState->camera.aspect)/2),  ((1/state.viewState->camera.aspect)/2),
                      (state.viewState->camera.nearPlane),(state.viewState->camera.farPlane));
            glViewport(int(state.viewState->viewWindow.left * display->GetWidth()), 
                              int((1.0f - state.viewState->viewWindow.bottom) * display->GetHeight() ),
                              int((state.viewState->viewWindow.right - state.viewState->viewWindow.left) * display->GetWidth()), 
                              int((state.viewState->viewWindow.bottom - state.viewState->viewWindow.top) * display->GetHeight()));
            glMatrixMode( GL_MODELVIEW );
            break;

        case PDDI_PROJECTION_PERSPECTIVE :
            {
                float halfX = (float)tan(double(state.viewState->camera.fov * 0.5)) * state.viewState->camera.nearPlane;
                float halfY = halfX / state.viewState->camera.aspect;

                glMatrixMode( GL_PROJECTION );
                glLoadIdentity();
                glFrustum(-halfX,halfX,-halfY,halfY,state.viewState->camera.nearPlane,state.viewState->camera.farPlane);
                glViewport(int(state.viewState->viewWindow.left * display->GetWidth()), 
                              int((1.0f - state.viewState->viewWindow.bottom) * display->GetHeight() ),
                              int((state.viewState->viewWindow.right - state.viewState->viewWindow.left) * display->GetWidth()), 
                              int((state.viewState->viewWindow.bottom - state.viewState->viewWindow.top) * display->GetHeight()));
                glMatrixMode( GL_MODELVIEW );
            }
            break;
        default:
            PDDIASSERTMSG(0, "Bad projection mode","");
            break;
    }

}

void pglContext::LoadHardwareMatrix(pddiMatrixType id)
{
    switch(id)
    {
        case PDDI_MATRIX_MODELVIEW :
        {
            
            glMatrixMode( GL_MODELVIEW );
            pddiMatrix tmp = *state.matrixStack[id]->Top();
            tmp.m[0][2] = -tmp.m[0][2];
            tmp.m[1][2] = -tmp.m[1][2];
            tmp.m[2][2] = -tmp.m[2][2];
            tmp.m[3][2] = -tmp.m[3][2];
            glLoadMatrixf((float*)&tmp);
        }
        break;
        default :
            PDDIASSERTMSG(0, "Invalid matrix load","");
            break;
    }
}

// viewport clipping
void pglContext::SetScissor(pddiRect* rect)
{
    pddiBaseContext::SetScissor(rect);
    if(!rect)
    {
        glDisable(GL_SCISSOR_TEST);
    }
    else
    {
        glScissor(rect->left, display->GetHeight() - rect->bottom, rect->right - rect->left, rect->bottom - rect->top);
        glEnable(GL_SCISSOR_TEST);
    }
}

class pglPrimStream : public pddiPrimStream
{
public:
    void Coord(float x, float y, float z)  
    { 
        glVertex3f(x,y,z); 
    }

    void Normal(float x, float y, float z) 
    { 
        glNormal3f(x,y,z); 
    }

    void Colour(pddiColour colour, int channel = 0)
    {        
        // HBW: Multiple CBVs not yet implemented.  For now just ignore channel.
        glColor4ub(colour.Red(), colour.Green(), colour.Blue(), colour.Alpha()); 
    }

    void UV(float u, float v, int channel = 0) 
    { 
        if(channel == 0)
        {
            glTexCoord2f(u,v); 
        }
    }

    void Specular(pddiColour colour) 
    {
        //
    }

    void Vertex(pddiVector* v, pddiColour c) 
    {
        glColor4ub(c.Red(), c.Green(), c.Blue(), c.Alpha());
        glVertex3f(v->x,v->y,v->z);
    }

    void Vertex(pddiVector* v, pddiVector* n)
    {
        glNormal3f(n->x,n->y,n->z);
        glVertex3f(v->x,v->y,v->z);
    }

    void Vertex(pddiVector* v, pddiVector2* uv)
    {
        glTexCoord2f(uv->u, uv->v);
        glVertex3f(v->x,v->y,v->z);
    }

    void Vertex(pddiVector* v, pddiColour c, pddiVector2* uv)
    {
        glColor4ub(c.Red(), c.Green(), c.Blue(), c.Alpha());
        glTexCoord2f(uv->u, uv->v);
        glVertex3f(v->x,v->y,v->z);
    }

    void Vertex(pddiVector* v, pddiVector* n, pddiVector2* uv)
    {
        glNormal3f(n->x,n->y,n->z);
        glTexCoord2f(uv->u, uv->v);
        glVertex3f(v->x,v->y,v->z);
    }

} thePrimStream;

pddiPrimStream* pglContext::BeginPrims(pddiShader* mat, pddiPrimType primType, unsigned vertexType, int vertexCount, unsigned pass)
{
    if(!mat)
        mat = defaultShader;

    pddiBaseContext::BeginPrims(mat, primType, vertexType, vertexCount);
    pddiBaseShader* material = (pddiBaseShader*)mat;
    ADD_STAT(PDDI_STAT_MATERIAL_OPS, !material->IsCurrent());
    material->SetMaterial();
    glBegin(primTypeTable[primType]);
    return &thePrimStream;
}

void pglContext::EndPrims(pddiPrimStream* stream)
{
    pddiBaseContext::EndPrims(stream);
    glEnd();
}

class pglPrimBufferStream : public pddiPrimBufferStream
{
public:
    pglPrimBuffer* buffer;

    pglPrimBufferStream(pglPrimBuffer* b)
    {
        buffer = b;
    }

    void Next(void)  
    {
        if(buffer->coord)
            buffer->coord += 3;

        if(buffer->normal)
            buffer->normal += 3;

        if(buffer->uv)
            buffer->uv += 2;

        if(buffer->colour)
            buffer->colour += 4;

        buffer->total++;
        PDDIASSERT(buffer->total <= buffer->allocated);
    }

    void Position(float x, float y, float z)  
    { 
        buffer->coord[0] = x;
        buffer->coord[1] = y;
        buffer->coord[2] = z;
        Next();
    }

    void Normal(float x, float y, float z) 
    { 
        buffer->normal[0] = x;
        buffer->normal[1] = y;
        buffer->normal[2] = z;
    }

    void Colour(pddiColour colour, int channel = 0)         
    {
        // HBW: Multiple CBVs not yet implemented.  For now just ignore channel.
        buffer->colour[0] = colour.Red();
        buffer->colour[1] = colour.Green();
        buffer->colour[2] = colour.Blue();
        buffer->colour[3] = colour.Alpha();
    }

    void TexCoord1(float u, int channel = 0) {}

    void TexCoord2(float u, float v, int channel = 0) 
    { 
        if(channel == 0)
        {
            buffer->uv[0] = u;
            buffer->uv[1] = v;
        }
    }

    void TexCoord3(float u, float v, float s, int channel = 0) {}
    void TexCoord4(float u, float v, float s, float t, int channel = 0) {}

    void Specular(pddiColour colour) 
    {
        //
    }

    void SkinIndices(unsigned, unsigned, unsigned, unsigned)
    {
    }

    void SkinWeights(float, float, float)
    {
    }

    void Vertex(pddiVector* v, pddiColour c) 
    {
        buffer->colour[0] = c.Red();
        buffer->colour[1] = c.Green();
        buffer->colour[2] = c.Blue();
        buffer->colour[3] = c.Alpha();
        buffer->coord[0] = v->x;
        buffer->coord[1] = v->y;
        buffer->coord[2] = v->z;
        Next();
    }

    void Vertex(pddiVector* v, pddiVector* n)
    {
        buffer->normal[0] = n->x;
        buffer->normal[1] = n->y;
        buffer->normal[2] = n->z;
        buffer->coord[0] = v->x;
        buffer->coord[1] = v->y;
        buffer->coord[2] = v->z;
        Next();
    }

    void Vertex(pddiVector* v, pddiVector2* uv)
    {
        buffer->uv[0] = uv->u;
        buffer->uv[1] = uv->v;
        buffer->coord[0] = v->x;
        buffer->coord[1] = v->y;
        buffer->coord[2] = v->z;
        Next();
    }

    void Vertex(pddiVector* v, pddiColour c, pddiVector2* uv)
    {
        buffer->colour[0] = c.Red();
        buffer->colour[1] = c.Green();
        buffer->colour[2] = c.Blue();
        buffer->colour[3] = c.Alpha();
        buffer->uv[0] = uv->u;
        buffer->uv[1] = uv->v;
        buffer->coord[0] = v->x;
        buffer->coord[1] = v->y;
        buffer->coord[2] = v->z;
        Next();
    }

    void Vertex(pddiVector* v, pddiVector* n, pddiVector2* uv)
    {
        buffer->normal[0] = n->x;
        buffer->normal[1] = n->y;
        buffer->normal[2] = n->z;
        buffer->uv[0] = uv->u;
        buffer->uv[1] = uv->v;
        buffer->coord[0] = v->x;
        buffer->coord[1] = v->y;
        buffer->coord[2] = v->z;
        Next();
    }

    bool CheckMemImageVersion(int version) { return false; }
    void* GetMemImagePtr()                 { return NULL; }
    unsigned GetMemImageLength()           { return 0; }

};

pglPrimBuffer::pglPrimBuffer(pglContext* c, pddiPrimType type, unsigned vertexFormat, int nVertex, int nIndex) : context(c)
{
    stream = new pglPrimBufferStream(this);

    total = allocated = nStrips = 0;
    coord = normal = uv = NULL;
    colour = NULL;
    strips = NULL;
    indices = NULL;

    valid = false;
    vertexBuffer = indexBuffer = 0;

    primType = type;
    vertexType = vertexFormat;

    allocated = nVertex;
    
    coord = new float[3 * nVertex];
    mem = 12;

    if(vertexFormat & PDDI_V_NORMAL) 
    {
        normal = new float[3 * nVertex];
        mem += 12;
    }

    if(vertexFormat & 0xf) 
    {
        uv = new float[2 * nVertex];
        mem += 8;
    }

    if(vertexFormat & PDDI_V_COLOUR) 
    {
        colour = new unsigned char[4 * nVertex];
        mem += 4;
    }

    mem *= nVertex;

    indexCount = nIndex;
    if(indexCount) 
        indices = new unsigned short[indexCount];

    nStrips = 0;
    strips = NULL;

    context->ADD_STAT(PDDI_STAT_BUFFERED_COUNT, 1);
    context->ADD_STAT(PDDI_STAT_BUFFERED_ALLOC, mem / 1024.0f);
}

pglPrimBuffer::~pglPrimBuffer()
{
    delete stream;

    delete [] strips;
    delete [] indices;
    delete [] coord;
    delete [] normal;
    delete [] uv;
    delete [] colour;

    context->ADD_STAT(PDDI_STAT_BUFFERED_COUNT, -1);
    context->ADD_STAT(PDDI_STAT_BUFFERED_ALLOC, -mem / 1024.0f);
}

pddiPrimBufferStream* pglPrimBuffer::Lock()
{
    total = 0;
    return stream;
}

void pglPrimBuffer::Unlock(pddiPrimBufferStream* stream)
{
    if(coord)
        coord -= total * 3;

    if(normal)
        normal -= total * 3;

    if(uv)
        uv -= total * 2;

    if(colour)
        colour -= total * 4;

    valid = false;
}

unsigned char* pglPrimBuffer::LockIndexBuffer()
{
    PDDIASSERT(0);
    return NULL;
}

void pglPrimBuffer::UnlockIndexBuffer(int count)
{
    PDDIASSERT(0);
}

void pglPrimBuffer::SetIndices(unsigned short* i, int count)
{
    PDDIASSERT(count <= (int)indexCount);
    memcpy(indices, i, count * sizeof(unsigned short));
    valid = false;
}

void pglPrimBuffer::Display(void)
{
    MICROPROFILE_SCOPEI("PDDI", "pglPrimBuffer::Display", MP_RED);

    if(!vertexBuffer)
    {
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, mem, NULL, GL_DYNAMIC_DRAW);
    }
    else
    {
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    }
    
    GLintptr offset = 0;
    if( !valid )
        glBufferSubData( GL_ARRAY_BUFFER, offset, allocated*12, coord );
    glVertexPointer(3,GL_FLOAT,0,(void*)offset);
    offset += allocated*12;

    if(vertexType & PDDI_V_NORMAL)
    {
        if(!valid)
            glBufferSubData(GL_ARRAY_BUFFER,offset,allocated*12,normal);
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT,0,(void*)offset);
        offset += allocated*12;
    }
    else
    {
        glDisableClientState(GL_NORMAL_ARRAY);
    }

    if(vertexType & PDDI_V_COLOUR)
    {
        if(!valid)
            glBufferSubData(GL_ARRAY_BUFFER,offset,allocated*4,colour);
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4,GL_UNSIGNED_BYTE,0,(void*)offset);
        offset += allocated*4;
    }
    else
    {
        glDisableClientState(GL_COLOR_ARRAY);
    }

    if(vertexType & 0xf)
    {
        if(!valid)
            glBufferSubData(GL_ARRAY_BUFFER,offset,allocated*8,uv);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2,GL_FLOAT,0,(void*)offset);
        offset += allocated*8;
    }
    else
    {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    if(indexCount && indices)
    {
        if(!indexBuffer)
        {
            glGenBuffers(1, &indexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,indexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,indexCount*sizeof(unsigned short),NULL,GL_DYNAMIC_DRAW);
        }
        else
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,indexBuffer);
        }

        if(!valid)
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,0,indexCount*sizeof(unsigned short),indices);
        glDrawElements(primTypeTable[primType],indexCount,GL_UNSIGNED_SHORT,0);
    }
    else
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
        glDrawArrays(primTypeTable[primType], 0, total);
    }

    valid = true;
}

/*
protected:
    float* coord;
    float* normal;
    float* uv;
    unsigned char* colour;

    unsigned allocated;
    unsigned total;

};
*/

void pglContext::DrawPrimBuffer(pddiShader* mat, pddiPrimBuffer* buffer)
{
    if(!mat)
        mat = defaultShader;

    pddiBaseShader* material = (pddiBaseShader*)mat;
    ADD_STAT(PDDI_STAT_MATERIAL_OPS, !material->IsCurrent());
    material->SetMaterial();
    ((pglPrimBuffer*)buffer)->Display();
}

// lighting

int pglContext::GetMaxLights(void)
{
    int tmp;
    glGetIntegerv(GL_MAX_LIGHTS, &tmp);
    return tmp;
}

void pglContext::SetupHardwareLight(int handle)
{
    GLenum h = GLenum(GL_LIGHT0 + handle);
    
    if(state.lightingState->light[handle].enabled)
        glEnable(h);
    else
        glDisable(h);

    glLightfv(h, GL_CONSTANT_ATTENUATION, &state.lightingState->light[handle].attenA);
    glLightfv(h, GL_LINEAR_ATTENUATION, &state.lightingState->light[handle].attenB);
    glLightfv(h, GL_QUADRATIC_ATTENUATION, &state.lightingState->light[handle].attenC);
    

    float c[4];
    FillGLColour(state.lightingState->light[handle].colour, c);
    glLightfv(h, GL_DIFFUSE, c);
    glLightfv(h, GL_SPECULAR, c);
    
    float dir[4];
    switch(state.lightingState->light[handle].type)
    {
        case PDDI_LIGHT_DIRECTIONAL :
            dir[0] = -state.lightingState->light[handle].worldDirection.x;
            dir[1] = -state.lightingState->light[handle].worldDirection.y;
            dir[2] = -state.lightingState->light[handle].worldDirection.z;
            dir[3] = 0.0f;
            break;

        case PDDI_LIGHT_POINT :
            dir[0] = state.lightingState->light[handle].worldPosition.x;
            dir[1] = state.lightingState->light[handle].worldPosition.y;
            dir[2] = state.lightingState->light[handle].worldPosition.z;
            dir[3] = 1.0f;
            break;

        case PDDI_LIGHT_SPOT :
            PDDIASSERT(0);
            break;
    }
    glLightfv(h, GL_POSITION, dir);
}

void pglContext::SetAmbientLight(pddiColour col)
{
    pddiBaseContext::SetAmbientLight(col);
    float ambient[4];
    FillGLColour(col,ambient);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
}


// backface culling
GLenum cullModeTable[3] =
{
    GL_FRONT, // PDDI_CULL_NONE (disabled using glDisable())
    GL_FRONT, // PDDI_CULL_NORMAL
    GL_BACK   // PDDI_CULL_INVERTED
};
    
void pglContext::SetCullMode(pddiCullMode mode)
{
    pddiBaseContext::SetCullMode(mode);

    if(mode == PDDI_CULL_NONE)
    {
        glDisable(GL_CULL_FACE);
    }
    else
    {
        glEnable(GL_CULL_FACE);
        glCullFace(cullModeTable[mode]);
    }
}

// z-buffer control
GLenum compTable[8] = {
    GL_NEVER,
    GL_ALWAYS,  
    GL_LESS,
    GL_LEQUAL,
    GL_GREATER,    
    GL_GEQUAL,  
    GL_EQUAL,
    GL_NOTEQUAL,
};

void pglContext::SetColourWrite( bool red, bool green, bool blue, bool alpha )
{
    pddiBaseContext::SetColourWrite(red, green, blue, alpha);
    glColorMask(red, green, blue, alpha);
}

void pglContext::EnableZBuffer(bool enable)
{
    pddiBaseContext::EnableZBuffer(enable);
    if(enable)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
}


void pglContext::SetZCompare(pddiCompareMode compareMode)
{
    pddiBaseContext::SetZCompare(compareMode);
    glDepthFunc(compTable[compareMode]);
}

void pglContext::SetZWrite(bool b)
{
    pddiBaseContext::SetZWrite(b);
    glDepthMask(b);
}

void pglContext::SetZBias(float bias)
{
    pddiBaseContext::SetZBias(bias);
//TODO : Figure out how ro do this
}

void pglContext::SetZRange(float n, float f)
{
    pddiBaseContext::SetZRange(n,f);
    glDepthRange(n,f);
}

// stencil buffer control
GLenum stencilTable[6] = {
    GL_KEEP,
    GL_ZERO,
    GL_REPLACE,
    GL_INCR,
    GL_DECR,
    GL_INVERT
};

void pglContext::EnableStencilBuffer(bool enable)
{
    pddiBaseContext::EnableStencilBuffer(enable);
    if(enable)
        glEnable(GL_STENCIL_TEST);
    else
        glDisable(GL_STENCIL_TEST);
}
        
void pglContext::SetStencilCompare(pddiCompareMode compare)
{
    pddiBaseContext::SetStencilCompare(compare);
    glStencilFunc(compTable[compare], state.stencilState->ref, state.stencilState->mask);
}

void pglContext::SetStencilRef(int ref)
{
    pddiBaseContext::SetStencilRef(ref);
    glStencilFunc(compTable[state.stencilState->compare], ref, state.stencilState->mask);
}

void pglContext::SetStencilMask(unsigned mask)
{
    pddiBaseContext::SetStencilMask(mask);
    glStencilFunc(compTable[state.stencilState->compare], state.stencilState->ref, mask);
}

void pglContext::SetStencilWriteMask(unsigned mask)
{
    pddiBaseContext::SetStencilWriteMask(mask);
    glStencilMask(mask);
}

void pglContext::SetStencilOp(pddiStencilOp failOp, pddiStencilOp zFailOp, pddiStencilOp zPassOp)
{
    pddiBaseContext::SetStencilOp(failOp, zFailOp, zPassOp);
    glStencilOp(stencilTable[failOp],stencilTable[zFailOp],stencilTable[zPassOp]);
}

// polygon fill
GLenum fillTable[3] =
{
    GL_FILL,
    GL_LINE,
    GL_POINT
};

void pglContext::SetFillMode(pddiFillMode mode)
{
    pddiBaseContext::SetFillMode(mode);
    glPolygonMode(GL_FRONT_AND_BACK, fillTable[mode]);
}

// fog
void pglContext::EnableFog(bool enable)
{
    pddiBaseContext::EnableFog(enable);
    if(enable)
        glEnable(GL_FOG);
    else
        glDisable(GL_FOG);
}

// pddiFogMode
static GLenum fogTable[] =
{
    GL_LINEAR,
    GL_EXP,
    GL_EXP2
};

void pglContext::SetFog(pddiColour colour, float start, float end)
{
    pddiBaseContext::SetFog(colour,start,end);

    float fog[4];
    fog[0] = float(colour.Red()) / 255;
    fog[1] = float(colour.Green()) / 255;
    fog[2] = float(colour.Blue()) / 255;
    fog[3] = float(colour.Alpha()) / 255;

    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogfv(GL_FOG_COLOR, fog);
    glFogf(GL_FOG_START, start);
    glFogf(GL_FOG_END, end);
}

int pglContext::GetMaxTextureDimension(void)
{
    return maxTexSize;
}

pddiExtension* pglContext::GetExtension(unsigned extID)
{ 
    switch(extID)
    {
        case PDDI_EXT_GL_CONTEXT :
            return extContext;
        case PDDI_EXT_GAMMACONTROL :
            return extGamma;
    }

    return pddiBaseContext::GetExtension(extID);
}

bool pglContext::VerifyExtension(unsigned extID)
{ 
    switch(extID)
    {
        case PDDI_EXT_GL_CONTEXT :
        case PDDI_EXT_GAMMACONTROL :
            return true;
    }

    return pddiBaseContext::VerifyExtension(extID);
}

void  pglContext::BeginTiming(void)
{
    display->BeginTiming();
}

float pglContext::EndTiming(void)
{
    return display->EndTiming();
}



