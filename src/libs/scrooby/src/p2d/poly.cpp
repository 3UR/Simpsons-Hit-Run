#include "../FeDrawable.h"
#include "poly.hpp"
#include <raddebug.hpp>
#include <shader.hpp>
#include <utility.hpp>
#include "utility/debugMessages.h"

namespace Pure2D
{

Poly::Poly( const int n, const rmt::Vector* v, const tColour* c)
:
    m_Vectors( NULL ),
    m_Colours( NULL ),
    m_Shader( NULL ),
    m_Color( 0xff, 0xff, 0xff, 0xff )
{
    nVer = ( n - 2 ) * 3;
    m_Shader = NULL;
    m_Shader = new tShader;                 //P3D NEW
    m_Shader->AddRef();
 
    m_Vectors = NULL;
    m_Vectors = new rmt::Vector[ nVer ];    //P3D NEW
    m_Colours = new tColour[ nVer ];        //P3D NEW
    m_Shader->SetInt( PDDI_SP_BLENDMODE, PDDI_BLEND_ALPHA );
    m_Shader->SetInt( PDDI_SP_SHADEMODE, PDDI_SHADE_GOURAUD );
#ifdef P3D_PS2
    m_Shader->SetInt( PDDI_SP_ALPHATEST, 1 );   //turn on alpha testing     (only required for PS2 16 bit framebuffers )
    //IAN IMPROVE: check the framebuffer depth also
#endif
 
    // TODO, should use normalized Z
    int count = 0;
    int i;
    for( i = 1; i < n - 1; i++ )
    {
        m_Vectors[ count ].Set( v[ 0 ].x, v[ 0 ].y, v[ 0 ].z );
        m_Colours[ count ] = c[ 0 ];
        count++;
        m_Vectors[ count ].Set( v[ i ].x, v[ i ].y, v[ i ].z );
        m_Colours[ count ] = c[ i ];
        count++;
        m_Vectors[ count ].Set( v[ i + 1 ].x, v[ i + 1 ].y, v[ i + 1 ].z );
        m_Colours[ count ] = c[ i + 1 ];
        count++;
    }
}

//===========================================================================
// Poly::~Poly
//===========================================================================
// Description: Destructor
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
Poly::~Poly()
{
    delete[] m_Vectors;
    delete[] m_Colours;
    m_Shader->Release();
}

//===========================================================================
// Display
//===========================================================================
// Description: displays the polygon usign P3D v14 Streaming Drawprims
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
void Poly::Display( void )
{
    pddiShader* shader = m_Shader->GetShader();
    pddiPrimStream* stream = p3d::pddi->BeginPrims( shader, PDDI_PRIM_TRIANGLES, PDDI_V_C );   
    int i;
    for( i = 0; i < this->nVer; i++ )
    {
        tColour modulated = m_Colours[ i ];
        FeDrawable::ModulateColour( modulated, this->m_Color );
        stream->Colour( modulated );
        stream->Coord( m_Vectors[ i ].x, m_Vectors[ i ].y, m_Vectors[ i ].z );
    }
    p3d::pddi->EndPrims(stream);
}

//===========================================================================
// SetColour
//===========================================================================
// Description: sets the colour of the polygon by multiplying each vertex's 
//              color by this color modifier
//
// Constraints:    NONE
//
// Parameters:    c - the colour to modulate with
//
// Return:      NONE
//
//===========================================================================
void Poly::SetColour( tColour c )
{
    this->m_Color = c;
}

}
