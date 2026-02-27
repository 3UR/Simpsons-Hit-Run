#ifndef _DLINE_HPP_
#define _DLINE_HPP_


#include <p3d/p3dtypes.hpp>
#include <radmath/radmath.hpp>
#include <simcommon/tarray.hpp>

class pddiShader;

namespace sim
{


    
class LineElem
{
public:
    void Set(rmt::Vector inp1, rmt::Vector inp2, tColour incolor1, tColour incolor2)
    {
        p1 = inp1;
        p2 = inp2;
        color1 = incolor1;
        color2 = incolor2;
    }
    rmt::Vector p1;
    rmt::Vector p2;
    tColour color1;
    tColour color2;
};


extern TArray<LineElem>* g_pLineElemArray;
extern pddiShader* g_pDefaultShader;
extern int g_LineStreamCounter;


void dLine2(const rmt::Vector& p1, const rmt::Vector& p2, const tColour& c1, const tColour& c2);
inline void dLine2(const rmt::Vector& p1, const rmt::Vector& p2, const tColour& c) { dLine2(p1, p2, c, c); }

void dLineRel(const rmt::Vector& p1, const rmt::Vector& p2, const tColour& c1);

void dBox3(const rmt::Vector& pos, const rmt::Vector& size, const tColour& colour);
void dQuad(const rmt::Vector& p, const rmt::Vector& a, const rmt::Vector& b, const tColour& colour);
void dLine3(const rmt::Vector& p, float r, const tColour& colour);
void dAxes(const float scale, const rmt::Matrix& m);

inline void DrawLine(const float p1[], const float p2[], const tColour &c1)
{
    dLine2(*((rmt::Vector*)p1), *((rmt::Vector*)p2), c1);
}

inline void DrawLine(const float p1[], const float p2[], const tColour &c1, const tColour &c2)
{
    dLine2(*((rmt::Vector*)p1), *((rmt::Vector*)p2), c1, c2);
}

// used together
void dStartStreamLine();
void dStreamLine(const rmt::Vector& p1, const rmt::Vector& p2, const tColour& c1, const tColour& c2);
//void dStreamLine(const rmt::Vector& p1, const rmt::Vector& p2, const tColour& c1) { dStreamLine(p1, p2, c1, c1); }
void dEndStreamLine();

class DrawLineToggler
{
public:
    DrawLineToggler() { dStartStreamLine(); }
    ~DrawLineToggler() { dEndStreamLine(); }
};

} // sim

#endif
