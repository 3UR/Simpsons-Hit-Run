
#include "simcommon/dline2.hpp"
#include <p3d/shader.hpp>
#include <p3d/utility.hpp>
#include <simcommon/tarray.hpp>

using namespace RadicalMathLibrary;

namespace sim
{

TArray<LineElem>* g_pLineElemArray = 0;
pddiShader* g_pDefaultShader = 0;
int g_LineStreamCounter = 0;

void dStartStreamLine()
{
    g_LineStreamCounter++;
}

void dStreamLine(const Vector& p1, const Vector& p2, const tColour& c1, const tColour& c2)
{
    g_pLineElemArray->NewElem().Set(p1, p2, c1, c2);
}

void dEndStreamLine()
{
    g_LineStreamCounter--;
    if (g_LineStreamCounter <= 0 && g_pLineElemArray->GetSize() > 0)
    {
        pddiPrimStream* mystream = p3d::pddi->BeginPrims(g_pDefaultShader, PDDI_PRIM_LINES, PDDI_V_C, g_pLineElemArray->GetSize()*2);
        for (int i=0; i<g_pLineElemArray->GetSize(); i++)
        {
            LineElem* elem = &g_pLineElemArray->GetAt(i);
            mystream->Colour(elem->color1);
            mystream->Coord(elem->p1.x, elem->p1.y, elem->p1.z);
            mystream->Colour(elem->color2);
            mystream->Coord(elem->p2.x, elem->p2.y, elem->p2.z);
        }
        p3d::pddi->EndPrims(mystream);
        g_LineStreamCounter = 0;
        g_pLineElemArray->Clear();
    }
}

void dLine2(const Vector& p1, const Vector& p2, const tColour& c1, const tColour& c2)
{
    DrawLineToggler toggler;
    dStreamLine(p1, p2, c1, c2);
}

void dLineRel(const Vector& v1, const Vector& v2, const tColour& c)
{
    Vector v;
    v = v2;
    v.Add(v1);
    dLine2(v1, v, c);
}

void dQuad(const Vector& p, const Vector& a, const Vector& b, const tColour& colour)
{
    Vector p2;
    p2.Add(p, a);
    dLine2(p, p2, colour);
    /*
    pddiQuad q;
    
      q.vertex[0].x = p.x;
      q.vertex[0].y = p.y;
      q.vertex[0].z = p.z;
      
        q.vertex[1].x = p.x + b.x;
        q.vertex[1].y = p.y + b.y;
        q.vertex[1].z = p.z + b.z;
        
          q.vertex[2].x = q.vertex[1].x + a.x;
          q.vertex[2].y = q.vertex[1].y + a.y;
          q.vertex[2].z = q.vertex[1].z + a.z;
          
            q.vertex[3].x = p.x + a.x;
            q.vertex[3].y = p.y + a.y;
            q.vertex[3].z = p.z + a.z; 
            
              q.uv[0].u = 0.0f;
              q.uv[0].v = 0.0f;
              q.uv[1].u = 1.0f;
              q.uv[1].v = 0.0f;
              q.uv[2].u = 1.0f;
              q.uv[2].v = 1.0f;
              q.uv[3].u = 0.0f;
              q.uv[3].v = 1.0f;
              
                q.colour[0] = PDDICOLOUR(colour);
                q.colour[1] = PDDICOLOUR(colour);
                q.colour[2] = PDDICOLOUR(colour);
                q.colour[3] = PDDICOLOUR(colour);
                
                  if (inShader != NULL)
                  {
                  tPort::RenderContext->RenderQuad(inShader->GetShader(), &q);
                  }
                  else
                  {
                  tShader shader;
                  shader.SetColour(colour);
                  tPort::RenderContext->RenderQuad(mater.GetShader(), &q);
                  }
    */
}

void dBox3(const Vector& pos, const Vector& size, const tColour& colour)
{
    DrawLineToggler toggler;
    
    Vector p1, p2;
    Vector tmp;
    
    tmp.Scale(2.0f, size);
    p1.Add(pos, size);
    p2 = p1;
    p2.x -=  tmp.x;
    dStreamLine(p1, p2, colour, colour);
    p2.x =  p1.x;
    p2.y =  p1.y - tmp.y;
    dStreamLine(p1, p2, colour, colour);
    p2.y =  p1.y;
    p2.z =  p1.z - tmp.z;
    dStreamLine(p1, p2, colour, colour);
    
    p1.Sub(pos, size);
    p2 = p1;
    p2.x += tmp.x;
    dStreamLine(p1, p2, colour, colour);
    p2.x =  p1.x;
    p2.y =  p1.y + tmp.y;
    dStreamLine(p1, p2, colour, colour);
    p2.y =  p1.y;
    p2.z =  p1.z + tmp.z;
    dStreamLine(p1, p2, colour, colour);
}

void dAxes(const float scale, const Matrix& m)
{
	Vector axis;
	axis.Set(scale, 0.0f, 0.0f);
    axis.Transform(m);
	dLine2(m.Row(3), axis, tColour(255, 0, 0));
	axis.Set(0.0f, scale, 0.0f);
    axis.Transform(m);
	dLine2(m.Row(3), axis, tColour(0, 255, 0));
	axis.Set(0.0f, 0.0f, scale);
    axis.Transform(m);
	dLine2(m.Row(3), axis, tColour(0, 0, 255));
}

void dLine3(const Vector& p, float r, const tColour& colour)
{
    DrawLineToggler toggler;

    Vector p1, p2;
    
    p1 = p;
    p1.x += r;
    p2 = p;
    p2.x -= r;
    dStreamLine(p1, p2, colour, colour);
    p1 = p;
    p1.y += r;
    p2 = p;
    p2.y -= r;
    dStreamLine(p1, p2, colour, colour);
    p1 = p;
    p1.z += r;
    p2 = p;
    p2.z -= r;
    dStreamLine(p1, p2, colour, colour);
}

} // sim