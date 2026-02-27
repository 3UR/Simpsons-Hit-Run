/*===========================================================================
    debugdraw.cpp
    25-Jun-01 Created by Liberty 
    Some basic wireframe drawing functions

    Copyright (c)2001 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#ifndef RAD_RELEASE

#include <p3d/debugdraw.hpp>
#include <p3d/utility.hpp>
#include <p3d/matrixstack.hpp>

using namespace RadicalMathLibrary;

//---------------------------------------------------------------------------
// Support Math Functions that should go into Radmath
//---------------------------------------------------------------------------


Matrix MatrixAlongXAxis(Vector origin, Vector target)
{
    Matrix basisMatrix;

    basisMatrix.Identity();
    basisMatrix.Row(3) = origin;

    Vector& xAxis = basisMatrix.Row(0);
    Vector& yAxis = basisMatrix.Row(1);
    Vector& zAxis = basisMatrix.Row(2);

    Vector yUp(0.0f,1.0f,0.0f);
    Vector zUp(0.0f,0.0f,1.0f);

    xAxis.Sub(target, origin);
    
    xAxis.Normalize();

    const float colinearTest = 0.95f;
    if((float)fabs(xAxis.y)>colinearTest)
    {
        //
        // Use Z up for the building the matrix
        //
        yAxis.CrossProduct(zUp, xAxis);
        yAxis.Normalize();
        zAxis.CrossProduct(xAxis, yAxis);
    }
    else
    {
        //
        // Use Y up for the building the matrix
        //
        zAxis.CrossProduct(xAxis, yUp);
        zAxis.Normalize();
        yAxis.CrossProduct(zAxis, xAxis);
    }

    return basisMatrix;
}

Matrix MatrixAlongYAxis(Vector origin, Vector target)
{
    Matrix basisMatrix;

    basisMatrix.Identity();
    basisMatrix.Row(3) = origin;

    Vector& xAxis = basisMatrix.Row(0);
    Vector& yAxis = basisMatrix.Row(1);
    Vector& zAxis = basisMatrix.Row(2);

    Vector xUp(1.0f,0.0f,0.0f);
    Vector zUp(0.0f,0.0f,1.0f);

    yAxis.Sub(target, origin);
    
    yAxis.Normalize();

    const float colinearTest = 0.95f;
    if((float)fabs(yAxis.x)>colinearTest)
    {
        //
        // Use Z up for the building the matrix
        //
        xAxis.CrossProduct(yAxis, zUp);
        xAxis.Normalize();
        zAxis.CrossProduct(xAxis, yAxis);
    }
    else
    {
        //
        // Use X up for the building the matrix
        //
        zAxis.CrossProduct(xUp, yAxis);
        zAxis.Normalize();
        xAxis.CrossProduct(yAxis, zAxis);
    }

    return basisMatrix;
}

Matrix MatrixAlongZAxis(Vector origin, Vector target)
{
    Matrix basisMatrix;

    basisMatrix.Identity();
    basisMatrix.Row(3) = origin;

    Vector& xAxis = basisMatrix.Row(0);
    Vector& yAxis = basisMatrix.Row(1);
    Vector& zAxis = basisMatrix.Row(2);

    Vector xUp(1.0f,0.0f,0.0f);
    
    Vector yUp(0.0f,1.0f,0.0f);

    zAxis.Sub(target, origin);
    
    zAxis.Normalize();

    const float colinearTest = 0.95f;
    if((float)fabs(zAxis.y)>colinearTest)
    {
        //
        // Use X up for the building the matrix
        //
        yAxis.CrossProduct(zAxis, xUp);
        yAxis.Normalize();
        xAxis.CrossProduct(yAxis, zAxis);
    }
    else
    {
        //
        // Use Y up for the building the matrix
        //
        xAxis.CrossProduct(yUp, zAxis);
        xAxis.Normalize();
        yAxis.CrossProduct(zAxis, xAxis);
    }

    return basisMatrix;
}

//---------------------------------------------------------------------------
// Debug drawing functions
//---------------------------------------------------------------------------
void P3DDrawOrientedBox
(
    const Vector minCorner,
    const Vector maxCorner,
    tShader& shader,
    tColour lineColor, // default tColour(255,0,0),
    bool drawDiagonals // default true 
)
{

    //
    // X-axis Lines
    //  
    Vector startPoint = minCorner;
    Vector endPoint   = minCorner;

    int count = 24;
    if (drawDiagonals) count += 12;

    pddiPrimStream* stream = p3d::pddi->BeginPrims(shader.GetShader(), PDDI_PRIM_LINES, PDDI_V_C, count);

    endPoint.x = maxCorner.x;
    stream->Vertex(((pddiVector*)(&(startPoint))),  lineColor);
    stream->Vertex(((pddiVector*)(&(endPoint))),    lineColor);

    startPoint.z = maxCorner.z;
    endPoint.z   = maxCorner.z;
    stream->Vertex(((pddiVector*)(&(startPoint))),  lineColor);
    stream->Vertex(((pddiVector*)(&(endPoint))),    lineColor);
    
    startPoint.y = maxCorner.y;
    endPoint.y   = maxCorner.y;
    stream->Vertex(((pddiVector*)(&(startPoint))),  lineColor);
    stream->Vertex(((pddiVector*)(&(endPoint))),    lineColor);
    
    startPoint.z = minCorner.z;
    endPoint.z   = minCorner.z;
    stream->Vertex(((pddiVector*)(&(startPoint))),  lineColor);
    stream->Vertex(((pddiVector*)(&(endPoint))),    lineColor);

    //
    // Z-axis Lines
    //
    startPoint = minCorner;
    endPoint   = minCorner;
    endPoint.z = maxCorner.z;
    stream->Vertex(((pddiVector*)(&(startPoint))),  lineColor);
    stream->Vertex(((pddiVector*)(&(endPoint))),    lineColor);

    startPoint.x = maxCorner.x;
    endPoint.x   = maxCorner.x;
    stream->Vertex(((pddiVector*)(&(startPoint))),  lineColor);
    stream->Vertex(((pddiVector*)(&(endPoint))),    lineColor);
    
    startPoint.y = maxCorner.y;
    endPoint.y   = maxCorner.y;
    stream->Vertex(((pddiVector*)(&(startPoint))),  lineColor);
    stream->Vertex(((pddiVector*)(&(endPoint))),    lineColor);

    startPoint.x = minCorner.x;
    endPoint.x   = minCorner.x;
    stream->Vertex(((pddiVector*)(&(startPoint))),  lineColor);
    stream->Vertex(((pddiVector*)(&(endPoint))),    lineColor);

    //
    // Y-axis Lines
    //
    startPoint = minCorner;
    endPoint   = minCorner;
    endPoint.y = maxCorner.y;
    stream->Vertex(((pddiVector*)(&(startPoint))),  lineColor);
    stream->Vertex(((pddiVector*)(&(endPoint))),    lineColor);

    startPoint.x = maxCorner.x;
    endPoint.x   = maxCorner.x;
    stream->Vertex(((pddiVector*)(&(startPoint))),  lineColor);
    stream->Vertex(((pddiVector*)(&(endPoint))),    lineColor);
    
    startPoint.z = maxCorner.z;
    endPoint.z   = maxCorner.z;
    stream->Vertex(((pddiVector*)(&(startPoint))),  lineColor);
    stream->Vertex(((pddiVector*)(&(endPoint))),    lineColor);

    startPoint.x = minCorner.x;
    endPoint.x   = minCorner.x;
    stream->Vertex(((pddiVector*)(&(startPoint))),  lineColor);
    stream->Vertex(((pddiVector*)(&(endPoint))),    lineColor);

    if(drawDiagonals)
    {
        //
        // This follows a zig-zag pattern
        //
        startPoint = minCorner;
        endPoint   = maxCorner;

        endPoint.z = minCorner.z;
        stream->Vertex(((pddiVector*)(&(startPoint))),  lineColor);
        stream->Vertex(((pddiVector*)(&(endPoint))),    lineColor);

        startPoint = endPoint;
        endPoint = maxCorner;
        endPoint.y = minCorner.y; 
        stream->Vertex(((pddiVector*)(&(startPoint))),  lineColor);
        stream->Vertex(((pddiVector*)(&(endPoint))),    lineColor);

        startPoint = endPoint;
        endPoint = maxCorner;
        endPoint.x = minCorner.x; 
        stream->Vertex(((pddiVector*)(&(startPoint))),  lineColor);
        stream->Vertex(((pddiVector*)(&(endPoint))),    lineColor);

        startPoint = endPoint;
        endPoint = minCorner;
        stream->Vertex(((pddiVector*)(&(startPoint))),  lineColor);
        stream->Vertex(((pddiVector*)(&(endPoint))),    lineColor);

        startPoint = endPoint;
        endPoint   = maxCorner;
        endPoint.y = minCorner.y;
        stream->Vertex(((pddiVector*)(&(startPoint))),  lineColor);
        stream->Vertex(((pddiVector*)(&(endPoint))),    lineColor);

        // on top to finish
        startPoint = maxCorner;
        endPoint   = maxCorner;
        startPoint.z = minCorner.z;
        endPoint.x   = minCorner.x;

        stream->Vertex(((pddiVector*)(&(startPoint))),  lineColor);
        stream->Vertex(((pddiVector*)(&(endPoint))),    lineColor);
    }
    p3d::pddi->EndPrims(stream);
}

void P3DDrawSphere
(
    float radius,
    const Vector position,
    tShader& shader,
    tColour lineColor,   // default = tColour(255,0,0),
    float wireResolution,// default = 4, 
    float wireContours   // default = 4
)
{
    float angletheta;
    float anglephi;
    Vector cursurTail;
    Vector cursur;

    Matrix positionMatrix;
    positionMatrix.Identity();
    positionMatrix.FillTranslate(position);
    p3d::stack->PushMultiply(positionMatrix);

    int longcount = rmt::FtoL((wireContours * 2 + 1) * (wireResolution + 1) * 2 * 2);

    pddiPrimStream* stream = p3d::pddi->BeginPrims(shader.GetShader(), PDDI_PRIM_LINES, PDDI_V_C, longcount);

    //
    // Draw Longitudinal axes
    //
    anglephi = 0;
    angletheta = 0;
    SphericalToCartesian(radius, angletheta, anglephi,   
                              &(cursur.x), &(cursur.y), &(cursur.z) );
    for( angletheta = 0; angletheta <= 2*PI; angletheta += PI/wireContours )
    {
        for( anglephi = 0; anglephi <= PI; anglephi += PI/wireResolution )   
        {
            cursurTail = cursur;
            SphericalToCartesian(radius, angletheta, anglephi,   
                                        &(cursur.x), &(cursur.y), &(cursur.z) );
     
            stream->Vertex(((pddiVector*)(&(cursurTail))),  lineColor);
            stream->Vertex(((pddiVector*)(&(cursur))),      lineColor);
        }
    }

    //
    // Draw Latitudinal axes
    //
    anglephi = 0;
    angletheta = 0;
    SphericalToCartesian(radius, angletheta, anglephi,   
                              &(cursur.x), &(cursur.y), &(cursur.z) );

    for( anglephi = 0; anglephi <= PI; anglephi += PI/wireContours )   
    {
        for( angletheta = 0; angletheta <= 2*PI; angletheta += PI/wireResolution )
        {
            cursurTail = cursur;
            SphericalToCartesian(radius, angletheta, anglephi,   
                                        &(cursur.x), &(cursur.y), &(cursur.z) );
     
            stream->Vertex(((pddiVector*)(&(cursurTail))), lineColor);
            stream->Vertex(((pddiVector*)(&(cursur))),     lineColor);
        }
    }
 
    p3d::pddi->EndPrims(stream);

    p3d::stack->Pop();
}

void P3DDrawCylinder
(
    float radius,
    Vector position,
    Vector position2,
    tShader& shader,
    tColour lineColor,   // default = tColour(255,0,0),
    float wireResolution,// default = 4, 
    float wireContours   // default = 4
)
{
    P3DDrawCone(
        radius,
        position,
        radius,
        position2,
        shader,
        lineColor,   
        wireResolution,
        wireContours
    );
}

void P3DDrawCone
(
    float radius,
    Vector position,
    float radius2,
    Vector position2,
    tShader& shader,
    tColour lineColor,   // default = tColour(255,0,0),
    float wireResolution,// default = 4, 
    float wireContours   // default = 4
)
{

    float anglephi = PI/2;
    float angletheta = 0;
    Vector cursur;
    Vector cursurTail;

    Vector Length;
    Length.Sub(position2, position);
    float halfLength = Length.Magnitude()/2;
    Length.Normalize();
    Length.Scale(halfLength);
    Length.Add(position);

    Matrix positionMatrix = MatrixAlongXAxis(Length, position2);

    p3d::stack->PushMultiply(positionMatrix);

    int mult = 2 + ((radius != 0.0f) ? 2 : 0) +  ((radius2 != 0.0f) ? 2 : 0);
    int count = (int) (mult * (((int)wireResolution * 2) + 1));

    pddiPrimStream* stream = p3d::pddi->BeginPrims(shader.GetShader(), PDDI_PRIM_LINES, PDDI_V_C, count);

    //
    // Circle at 'position' with 'radius'
    //
    if(radius != 0.0f)
    {
        cursur.x = -halfLength;
        cursur.y = radius * Cos(angletheta);
        cursur.z = radius * Sin(angletheta);
        for( angletheta = 0; angletheta <= PI*2; angletheta += PI/wireResolution )   
        {
            cursurTail = cursur;

            cursur.y = radius * Cos(angletheta);
            cursur.z = radius * Sin(angletheta);

            stream->Vertex(((pddiVector*)(&(cursurTail))), lineColor);
            stream->Vertex(((pddiVector*)(&(cursur))), lineColor);

        }
    }

    //
    // Circle at 'position2' with 'radius2'
    //
    if(radius2 != 0.0f)
    {
        cursur.x = halfLength;
        cursur.y = radius2 * Cos(angletheta);
        cursur.z = radius2 * Sin(angletheta);
        for( angletheta = 0; angletheta <= PI*2; angletheta += PI/wireResolution )   
        {
            cursurTail = cursur;

            cursur.y = radius2 * Cos(angletheta);
            cursur.z = radius2 * Sin(angletheta);

            stream->Vertex(((pddiVector*)(&(cursurTail))), lineColor);
            stream->Vertex(((pddiVector*)(&(cursur))), lineColor);

        }
    }

    //
    // Ribs
    //
    for( angletheta = 0; angletheta <= PI*2; angletheta += PI/wireResolution )   
    {
        cursur.x = -halfLength;
        cursur.y = radius * Cos(angletheta);
        cursur.z = radius * Sin(angletheta);
        stream->Vertex(((pddiVector*)(&(cursur))), lineColor);

        cursur.x = halfLength;
        cursur.y = radius2 * Cos(angletheta);
        cursur.z = radius2 * Sin(angletheta);
        stream->Vertex(((pddiVector*)(&(cursur))), lineColor);
    }

    p3d::pddi->EndPrims(stream);
    p3d::stack->Pop();
}


void P3DDrawCapsule
(
    float radius,
    Vector position,
    Vector position2,
    tShader& shader,
    tColour lineColor,   // default = tColour(255,0,0),
    float wireResolution,// default = 4, 
    float wireContours   // default = 4
)
{
    //
    // Draw 2 spheres 
    //

    Vector Length;
    Length.Sub(position2, position);
    float halfLength = Length.Magnitude()/2;
    Length.Normalize();
    Length.Scale(halfLength);
    Length.Add(position);

    Matrix positionMatrix = MatrixAlongZAxis(Length, position2);

    p3d::stack->PushMultiply(positionMatrix);

    Length.Clear();

    Length.z = halfLength;
    P3DDrawSphere(radius, Length, shader, lineColor, wireResolution, wireContours);

    Length.z = -halfLength;
    P3DDrawSphere(radius, Length, shader, lineColor, wireResolution, wireContours);

    p3d::stack->Pop();

    P3DDrawCylinder(radius, position, position2, shader, lineColor, wireResolution, wireContours);
}

#endif