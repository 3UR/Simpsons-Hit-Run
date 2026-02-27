//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/vertexlist.hpp>
#include <p3d/utility.hpp>
#include <p3d/chunkfile.hpp>
#include <p3d/file.hpp>
#include <constants/chunks.h>
#include <p3d/anim/vertexanimobject.hpp>
#include <p3d/anim/vertexanimkey.hpp>

#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define STREAM_COLOUR_WITH_OFFSET(  i, color, offset )  \
                    int r, g, b, a; \
                    r = color[ i ].Red( ) + offset[ i ].red;  \
                    g = color[ i ].Green( ) + offset[ i ].green;   \
                    b = color[ i ].Blue( ) + offset[ i ].blue;    \
                    a = color[ i ].Alpha( ) + offset[ i ].alpha;  \
                    colour.Set( r, g, b, a );   \
                    stream->Colour( colour );

#define STREAM_POSITION_WITH_OFFSET( i, position, offset )  \
                    stream->Coord( position[ i ].x + offset[ i ].x, position[ i ].y + offset[ i ].y, position[ i ].z + offset[ i ].z );

#define STREAM_NORMAL_WITH_OFFSET( i, normal, offset )  \
                    stream->Normal( normal[ i ].x + offset[ i ].x, normal[ i ].y + offset[ i ].y, normal[ i ].z + offset[ i ].z ); 
                    
#define STREAM_UV_WITH_OFFSET( i, uv, offset, uvIdx )  \
                    stream->UV( uv[ i ].u + offset[ i ].u, uv[ i ].v + offset[ i ].v, uvIdx );


using namespace RadicalMathLibrary;

tVertexList::tVertexList(unsigned num, unsigned type)
:offset( NULL )
{
    Allocate(num, type);
}

tVertexList::tVertexList(tVertexList& vl)
:offset( NULL )
{
    Allocate(vl.GetNumVertex(), vl.GetFormat());

    memcpy(coord, vl.GetPositions(), sizeof(Vector)*nVertex);

    if(format & PDDI_V_NORMAL)
    {
        memcpy(normal, vl.GetNormals(), sizeof(Vector)*nVertex);
    }

    if(format & PDDI_V_BINORMAL)
    {
        memcpy(binormal, vl.GetBinormals(), sizeof(Vector)*nVertex);
    }

    if(format & PDDI_V_TANGENT)
    {
        memcpy(tangent, vl.GetTangents(), sizeof(Vector)*nVertex);
    }

    if(format & PDDI_V_COLOUR)
    {
        memcpy(colour[0], vl.GetColours(0), sizeof(tColour)*nVertex);
    }

    int i;
    if(format & PDDI_V_COLOUR2)
    {
        int numColourSets = PddiNumColourSets(format);
        for(i = 0; i < numColourSets; i++)
        {
            memcpy(colour[i], vl.GetColours(i), sizeof(tColour)*nVertex);
        }
    }

    for(i=0; i < MAX_NUM_UV_SETS; i++)
    {
        if(i < (int)(format & 0xf))
        {
            memcpy(uv[i], vl.GetUVs(i), sizeof(Vector2)*nVertex);
        }
    }

    if(format & PDDI_V_INDICES)
    {
        memcpy(matIndices, vl.GetMatrixIndices(), sizeof(MatrixIndices)*nVertex);
    }

    if(format & PDDI_V_WEIGHTS)
    {
        memcpy(matWeights, vl.GetMatrixWeights(), sizeof(MatrixWeights)*nVertex);
    }
}

tVertexList::~tVertexList()
{
    delete[] coord;
    delete[] normal;
    delete[] binormal;
    delete[] tangent;
    int i;
    for(i = 0; i < MAX_NUM_COLOUR_SETS; i++)
    {
        delete[] colour[i];
    }
    for(i = 0; i < MAX_NUM_UV_SETS; i++)
    {
        delete[] uv[i];
    }
    delete[] matIndices;
    delete[] matWeights;

    if( offset )
        offset->Release( );
}

void tVertexList::Allocate(unsigned num, unsigned type)
{
    nVertex = num;
    format = type;

    normal     = NULL;
    binormal   = NULL;
    tangent    = NULL;
    matIndices = NULL;
    matWeights = NULL;

    coord = new Vector[nVertex];

    if(format & PDDI_V_NORMAL)
    {
        normal = new Vector[nVertex];
    }

    if(format & PDDI_V_BINORMAL)
    {
        binormal = new Vector[nVertex];
    }

    if(format & PDDI_V_TANGENT)
    {
        tangent = new Vector[nVertex];
    }

    int i;
    for(i = 0; i < MAX_NUM_COLOUR_SETS; i++)
    {
        colour[i] = NULL;
    }
    if(format & PDDI_V_COLOUR)
    {
        colour[0] = new tColour[nVertex];
    }
    if(format & PDDI_V_COLOUR2)
    {
        int numColourSets = PddiNumColourSets(format);
        for(i = 0; i < numColourSets;  i++)
        {
            colour[i] = new tColour[nVertex];
        }
    }

    for(i=0; i < MAX_NUM_UV_SETS; i++)
    {
        uv[i] = NULL;
        if(i < (int)(format & 0xf))
        {
            uv[i] = new Vector2[nVertex];
        }
    }

    if(format & PDDI_V_INDICES)
    {
        matIndices = new MatrixIndices[nVertex];
    }

    if(format & PDDI_V_WEIGHTS)
    {
        P3DASSERT(format & PDDI_V_INDICES);
        matWeights = new MatrixWeights[nVertex];
    }
}

//-------------------------------------------------------------------
template<class T> static inline void StreamInternal(tVertexList* list, T* stream, int start, int count)
{
    int i;
    pddiVector* p = (pddiVector*)list->GetPositions();
    pddiColour* c[tVertexList::MAX_NUM_COLOUR_SETS];
    for(i = 0; i <tVertexList:: MAX_NUM_COLOUR_SETS; i++)
    {
        c[i] = (pddiColour*)list->GetColours(i);
    }
    pddiColour* c0 = c[0];
    pddiVector* n = (pddiVector*)list->GetNormals();
    pddiVector* b = (pddiVector*)list->GetBinormals();
    pddiVector* t = (pddiVector*)list->GetTangents();
    pddiVector2* t0 = (pddiVector2*)list->GetUVs(0);
    pddiVector2* t1 = (pddiVector2*)list->GetUVs(1);
    pddiVector2* t2 = (pddiVector2*)list->GetUVs(2);
    pddiVector2* t3 = (pddiVector2*)list->GetUVs(3);
    tVertexList::MatrixIndices* mi = list->GetMatrixIndices();
    tVertexList::MatrixWeights* mw = list->GetMatrixWeights();

    unsigned format = list->GetFormat() & ~PDDI_V_COLOUR_MASK;

    switch(format)
    {
        case PDDI_V_C:
            for(i=start; i < start+count; i++)
                stream->Vertex(&p[i], c[0][i]);
        break;

        case PDDI_V_N:
            for(i=start; i < start+count; i++)
                stream->Vertex(&p[i], &n[i]);
        break;

        case PDDI_V_T:
            for(i=start; i < start+count; i++)
                stream->Vertex(&p[i], &t0[i]);
        break;

        case PDDI_V_CT:
            for(i=start; i < start+count; i++)
                stream->Vertex(&p[i], c[0][i], &t0[i]);
        break;

        case PDDI_V_NT:
            for(i=start; i < start+count; i++)
                stream->Vertex(&p[i], &n[i], &t0[i]);
        break;

        default:
            for(i=start; i < start+count; i++)
            {
                int j;
                if(n)  stream->Normal(n[i].x, n[i].y, n[i].z);
                if(b)  stream->Binormal(b[i].x, b[i].y, b[i].z);
                if(t)  stream->Tangent(t[i].x, t[i].y, t[i].z);
                for(j = 0; j < tVertexList::MAX_NUM_COLOUR_SETS; j++)
                {
                    if(c[j]) stream->Colour(c[j][i]);
                }
                if(t0) stream->UV(t0[i].u, t0[i].v, 0);
                if(t1) stream->UV(t1[i].u, t1[i].v, 1);
                if(t2) stream->UV(t2[i].u, t2[i].v, 2);
                if(t3) stream->UV(t3[i].u, t3[i].v, 3);
                stream->Coord(p[i].x, p[i].y, p[i].z);
            }
        break;
    }
}

///
///     render a vertex list with offset list
///
template<class T> static inline void StreamWithOffsetInternal(tVertexList* list, T* stream, int start, int count)
{
    int i;
    pddiVector* p = (pddiVector*)list->GetPositions();
    pddiColour* c[tVertexList::MAX_NUM_COLOUR_SETS];
    for(i = 0; i < tVertexList::MAX_NUM_COLOUR_SETS; i++)
    {
        c[i] = (pddiColour*)list->GetColours(i);
    }
    pddiColour* c0 = c[0]; // Only colour 0 is animatable
    pddiVector* n = (pddiVector*)list->GetNormals();
    pddiVector* b = (pddiVector*)list->GetBinormals();
    pddiVector* t = (pddiVector*)list->GetTangents();
    pddiVector2* t0 = (pddiVector2*)list->GetUVs(0);
    pddiVector2* t1 = (pddiVector2*)list->GetUVs(1);
    pddiVector2* t2 = (pddiVector2*)list->GetUVs(2);
    pddiVector2* t3 = (pddiVector2*)list->GetUVs(3);

    tVertexOffsetList *offset = list->GetOffsetList( );

    assert( offset );           //offset must not be NULL
  
    pddiColour colour;

    pddiVector *op = (pddiVector*) ( offset->mMask & tVertexOffsetList::eOffsetCoord ? offset->mCoord : NULL);
    tColourOffset *oc = (tColourOffset*) ( offset->mMask & tVertexOffsetList::eOffsetColour ? offset->mColour : NULL );
    pddiVector *on = (pddiVector*) ( offset->mMask & tVertexOffsetList::eOffsetNormal ? offset->mNormal : NULL );
    pddiVector2 *ot0 = (pddiVector2*) ( offset->mMask & tVertexOffsetList::eOffsetUV0 ? offset->mUV[ 0 ] : NULL ); 
    pddiVector2 *ot1 = (pddiVector2*) ( offset->mMask & tVertexOffsetList::eOffsetUV1 ? offset->mUV[ 1 ] : NULL ); 
    pddiVector2 *ot2 = (pddiVector2*) ( offset->mMask & tVertexOffsetList::eOffsetUV2 ? offset->mUV[ 2 ] : NULL ); 
    pddiVector2 *ot3 = (pddiVector2*) ( offset->mMask & tVertexOffsetList::eOffsetUV3 ? offset->mUV[ 3 ] : NULL ); 

    unsigned format = list->GetFormat() & ~PDDI_V_COLOUR_MASK;
    switch(format)
    {
        case PDDI_V_C:
            if( oc && op ){         //with color and position offset
                for( i = start; i < start + count; i++ ){
                    STREAM_COLOUR_WITH_OFFSET( i, c0, oc );
                    STREAM_POSITION_WITH_OFFSET( i, p, op );
                }
            }
            else if( op ){      //position offset
                for( i = start; i < start + count; i++ ){
                    stream->Colour( c0[ i ] );                    
                    STREAM_POSITION_WITH_OFFSET( i, p, op );
                }
                
            }
            else if( oc ){  //colour offset only
                for( i = start; i < start + count; i++ ){                    
                    STREAM_COLOUR_WITH_OFFSET( i, c0, oc );
                    stream->Coord( p[ i ].x, p[ i ].y, p[ i ].z );
                }
            }
            else{       //no offset
                for( i = start; i < start + count; i++ ){                    
                    stream->Vertex(&p[i], c0[i]);
                }
            }
        break;

        case PDDI_V_N:
            if( on && op){         //with normal and position offset
                for( i = start; i < start + count; i++ ){
                    STREAM_NORMAL_WITH_OFFSET( i, n, on );
                    STREAM_POSITION_WITH_OFFSET( i, p, op );
                }
            }
            else if( op ){      //with position offset
                for( i = start; i < start + count; i++ ){
                    stream->Normal( n[ i ].x, n[ i ].y, n[ i ].z );
                    STREAM_POSITION_WITH_OFFSET( i, p, op ); 
                }
                
            }
            if( on ){    //with normal offset
                for( i = start; i < start + count; i++ ){                    
                    STREAM_NORMAL_WITH_OFFSET( i, n, on );
                    stream->Coord( p[ i ].x, p[ i ].y, p[ i ].z );
                }
            }
            else{   //no offset
                for( i = start; i < start + count; i++ ){                    
                    stream->Vertex( &p[i], &n[i] );
                }
            }
        break;

        case PDDI_V_T:
            if( ot0  && op ){         //with UV and position offset
                for( i = start; i < start + count; i++ )
                    STREAM_UV_WITH_OFFSET( i, t0, ot0, 0 );
                    STREAM_POSITION_WITH_OFFSET( i, p, op ); 

            }
            else if( op ){      //position offset only
                for( i = start; i < start + count; i++ ){
                    stream->UV( t0[ i ].u, t0[ i ].v, 0 );
                    STREAM_POSITION_WITH_OFFSET( i, p, op );
                }
            }
            else if( ot0 ){    //with uv offset only
                for( i = start; i < start + count; i++ ){                    
                    STREAM_UV_WITH_OFFSET( i, t0, ot0, 0 );
                    stream->Coord( p[ i ].x, p[ i ].y, p[ i ].z );
                }
            }
            else{   //no offset
                for( i = start; i < start + count; i++ ){                    
                    stream->Vertex(&p[i], &t0[i]);
                }
            }

        break;

        case PDDI_V_CT:
            if( oc && ot0 && op){         //with color, uv and position offset
                for( i = start; i < start + count; i++ ){
                    STREAM_COLOUR_WITH_OFFSET( i, c0, oc );
                    STREAM_UV_WITH_OFFSET( i, t0, ot0, 0 );
                    STREAM_POSITION_WITH_OFFSET( i, p, op );
                }
            }
            else if( oc && ot0 ){      //color and uv offset
                for( i = start; i < start + count; i++ ){
                    STREAM_COLOUR_WITH_OFFSET( i, c0, oc );
                    STREAM_UV_WITH_OFFSET( i, t0, ot0, 0 );
                    stream->Coord( p[ i ].x, p[ i ].y, p[ i ].z );
                }                
            }
            else if( ot0 && op ){       //uv and position offset
                for( i = start; i < start + count; i++ ){
                    stream->Colour( c0[ i ] ); 
                    STREAM_UV_WITH_OFFSET( i, t0, ot0, 0 );
                    STREAM_POSITION_WITH_OFFSET( i, p, op );
                }
            }
            else if( oc && op ){    //colour and position offset
                for( i = start; i < start + count; i++ ){
                    STREAM_COLOUR_WITH_OFFSET( i, c0, oc );
                    stream->UV( t0[ i ].u, t0[ i ].v, 0 );
                    STREAM_POSITION_WITH_OFFSET( i, p, op );
                }
            }
            else if( oc ){      //colour offset only
                for( i = start; i < start + count; i++ ){
                    STREAM_COLOUR_WITH_OFFSET( i, c0, oc );
                    stream->UV( t0[ i ].u, t0[ i ].v, 0 );
                    stream->Coord( p[ i ].x, p[ i ].y, p[ i ].z );
                }  
            }
            else if( ot0 ){     //uv offset only
                for( i = start; i < start + count; i++ ){
                    stream->Colour( c0[ i ] );
                    STREAM_UV_WITH_OFFSET( i, t0, ot0, 0 );
                    stream->Coord( p[ i ].x, p[ i ].y, p[ i ].z );
                }
            }
            else if( op ){      //position offset only
                for( i = start; i < start + count; i++ ){
                    stream->Colour( c0[ i ] );
                    stream->UV( t0[ i ].u, t0[ i ].v, 0 );
                    STREAM_POSITION_WITH_OFFSET( i, p, op );
                }
            }
            else{ //no offset
                for( i = start; i < start + count; i++ ){
                    stream->Vertex(&p[i], c0[i], &t0[i]);
                }
            }
            
        break;

        case PDDI_V_NT:

            if( ot0 && on && op){         //with uv, normal and position offset
                for( i = start; i < start + count; i++ ){
                    STREAM_UV_WITH_OFFSET( i, t0, ot0, 0 );
                    STREAM_NORMAL_WITH_OFFSET( i, n, on );
                    STREAM_POSITION_WITH_OFFSET( i, p, op );
                }            
            }
            else if( ot0 && on ){   //with uv and normal offset
                for( i = start; i < start + count; i++ ){
                    STREAM_UV_WITH_OFFSET( i, t0, ot0, 0 );
                    STREAM_NORMAL_WITH_OFFSET( i, n, on );
                    stream->Coord( p[ i ].x, p[ i ].y, p[ i ].z );
                }   
            }
            else if( ot0 && op ){   //with uv and position offset
                for( i = start; i < start + count; i++ ){
                    STREAM_UV_WITH_OFFSET( i, t0, ot0, 0 );
                    stream->Normal( n[ i ].x, n[ i ].y, n[ i ].z );
                    STREAM_POSITION_WITH_OFFSET( i, p, op );
                }      
            }
            else if( on && op ){    //with normal and position offset
               for( i = start; i < start + count; i++ ){
                    stream->UV( t0[ i ].u, t0[ i ].v, 0 );
                    STREAM_NORMAL_WITH_OFFSET( i, n, on );
                    STREAM_POSITION_WITH_OFFSET( i, p, op );
               }    
            }
            else if( ot0 ){     //uv offset only
               for( i = start; i < start + count; i++ ){
                    STREAM_UV_WITH_OFFSET( i, t0, ot0, 0 );
                    stream->Normal( n[ i ].x, n[ i ].y, n[ i ].z );
                    stream->Coord( p[ i ].x, p[ i ].y, p[ i ].z );
               }   
            }
            else if( on ){      //normal offset only
                for( i = start; i < start + count; i++ ){
                    stream->UV( t0[ i ].u, t0[ i ].v, 0 );
                    STREAM_NORMAL_WITH_OFFSET( i, n, on );
                    stream->Coord( p[ i ].x, p[ i ].y, p[ i ].z );
                }    
            }
            else if( op ){  //position offset only
                for( i = start; i < start + count; i++ ){
                    stream->UV( t0[ i ].u, t0[ i ].v, 0 );
                    stream->Normal( n[ i ].x, n[ i ].y, n[ i ].z );
                    STREAM_POSITION_WITH_OFFSET( i, p, op );
                }   
            }
            else{       //no offset 
                for( i = start; i < start + count; i++ ){
                    stream->Vertex(&p[i], &n[i], &t0[i]);
                }   

            }
        break;

        default:
            for(i=start; i < start+count; i++)
            {
                if(n){
                    if( on )
                        stream->Normal( n[ i ].x + on[ i ].x, n[ i ].y + on[ i ].y, n[ i ].z + on[ i ].z ); 
                    else
                        stream->Normal(n[i].x, n[i].y, n[i].z);
                }

                if(b)  stream->Binormal(b[i].x, b[i].y, b[i].z);
                if(t)  stream->Tangent(t[i].x, t[i].y, t[i].z);

                if(c0){  
                    if( oc ){
                        int r, g, b, a;
                        r = c0[ i ].Red( ) + oc[ i ].red;
                        g = c0[ i ].Green( ) + oc[ i ].green;
                        b = c0[ i ].Blue( ) + oc[ i ].blue;
                        a = c0[ i ].Alpha( ) + oc[ i ].alpha;
                        colour.Set( r, g, b, a );
                        stream->Colour( colour );
                    }
                    else 
                        stream->Colour( c0[ i ] );
                }

                int j;
                for(j = 1; j < tVertexList::MAX_NUM_COLOUR_SETS; j++)
                {
                    if(c[j]) stream->Colour( c[j][i] );
                }

                if(t0){ 
                    if( ot0 )
                        stream->UV( t0[ i ].u + ot0[ i ].u, t0[ i ].v + ot0[ i ].v, 0 );
                    else
                        stream->UV( t0[ i ].u, t0[ i ].v, 0 );
                }

                if(t1){ 
                    if( ot1 )
                        stream->UV( t1[ i ].u + ot1[ i ].u, t1[ i ].v + ot1[ i ].v, 1 );
                    else
                        stream->UV( t1[ i ].u, t1[ i ].v, 1 );
                }

                if(t2){ 
                    if( ot2 )
                        stream->UV( t2[ i ].u + ot2[ i ].u, t2[ i ].v + ot2[ i ].v, 2 );
                    else
                        stream->UV( t2[ i ].u, t2[ i ].v, 2 );
                }               
                
                if(t3){ 
                    if( ot3 )
                        stream->UV( t3[ i ].u + ot3[ i ].u, t3[ i ].v + ot3[ i ].v, 3 );
                    else
                        stream->UV( t3[ i ].u, t3[ i ].v, 3 );
                }

                if( op )
                    stream->Coord( p[ i ].x + op[ i ].x, p[ i ].y + op[ i ].y, p[ i ].z + op[ i ].z );                
                else
                    stream->Coord( p[ i ].x, p[ i ].y, p[ i ].z );

            }
        break;
    }
}

void tVertexList::Stream(pddiPrimStream* stream, int start, int count)
{
    if( offset )
    {
        //UNUSED CODE ???
        StreamWithOffsetInternal( this, stream, start, count );
    }
    else
    {
        StreamInternal(this, stream, start, count);
    }
}
