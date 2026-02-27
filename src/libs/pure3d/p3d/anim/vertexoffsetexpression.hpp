/*=============================================================================
  vertexoffsetexpression.hpp
  29-August-2000
  Created by: Bryan Brandt

  Description: Pure3D vertex offset expression classes for parameterized
                    vertex offset expressions
  Copyright (c) 1997-2000 Radical Entertainment, Inc.
  All Rights Reserved
=============================================================================*/

#ifndef _VERTEXOFFSETEXPRESSION_HPP
#define _VERTEXOFFSETEXPRESSION_HPP

#include <p3d/primgroup.hpp>
#include <p3d/drawable.hpp>
#include <p3d/anim/expression.hpp>
#include <p3d/anim/expressionoffsets.hpp>

const float P3D_KEY_EPSILON = 0.00001f;
const unsigned int P3D_VERTEXOFFSET_NUM_ACCUMULATE_STAGES = 3;

class tVertexOffsetExpressionMixer : public tExpressionMixer
{
public:
    tVertexOffsetExpressionMixer();

    virtual bool ValidateExpressionGroup(tExpressionGroup* eg);
    
    // Target must be tPolySkin and must have tOffsetLists.
    virtual void SetTarget(tEntity* t);
    virtual tEntity* GetTarget() const { return m_pDrawable; }

    // to be used later when PDDI refactored to add offsets in GPU
    // precondition: primgroup is deformable. If it isn't returns NULL
    virtual const rmt::Vector* GetVtxOffsets(int primgroupIdx);
    virtual void Update();

    // Enable detection of data inconsistencies
    void EnableDiagnosisMode() { m_diagnose = true; }
    void DisableDiagnosisMode() { m_diagnose = false; }

    virtual void SetExpressionGroup(tExpressionGroup* eg);
    
    // data structure that stores of a vertex and corresponding offset data separately
    // allows hardware to add offsets to platform-specific vertex representations
    class vertexData
    {
    public:
        vertexData(): position(0,0,0), offset(0,0,0){}
        rmt::Vector position; //eventually we don't need this
        rmt::Vector offset;
    }; // class vertexData

    // structures for preprocessing and quick deindexing of vertex offset animations
    class Offset
    {
    public:
        Offset() { vertex = NULL; expressionIndex =-1; keyVal = -2.0f; }
        Offset& operator=( const Offset& o )
        {
            vertex = o.vertex;
            expressionIndex = o.expressionIndex;
            keyVal = o.keyVal;
            offset.x = o.offset.x;
            offset.y = o.offset.y;
            offset.z = o.offset.z;
            return *this;
        }

        vertexData* vertex;
        int expressionIndex;
        float keyVal;
        rmt::Vector offset;
    }; // class Offset

protected:

    virtual ~tVertexOffsetExpressionMixer();
    virtual void ResetAccumulators();
    virtual bool ValidateTarget(tEntity* e);

    void InitOffsetArray();
    void CompactBlendStages();

    int m_nPrimGroups;          // number of primgroups in drawable
    int    m_nExprPrimGroups;      // number of primgroups with vertex offsets
    unsigned* m_nVertices;      // number of vertices in each primgroup
    vertexData** m_pVtxOffsets; // store original position of vtxs and calculated offsets 

    tDrawable* m_pDrawable;     // pointer tPolySkin - for reference counting purposes
    Offset* m_pOffsetArray;     // data table compiled from Expression group, and offset lists
    int m_nMaxNumStages;
    unsigned int m_uTotalNumOffsets;    // size of m_pOffsetArray

    rmt::Vector**  m_pResultOffsets;    // result of mixer as  vertex offsets lists - one per group
    bool m_diagnose;

    class ExpressionCalcCash
    {
    public:
       ExpressionCalcCash() 
                :
                absExprWeight(0.0f), 
                exprWeight(0.0f), 
                low_frac(0.0f), 
                high_frac(0.0f), 
                mirror(0.0f){}

       float absExprWeight; 
       float exprWeight; 
       float low_frac; 
       float high_frac;
       float mirror;
    };

    int m_ExpressionCalcCashSize;
    ExpressionCalcCash* m_ExpressionCalcCash;
};

class tPolySkin;

int compareOffsets(const void*, const void*);

inline rmt::Vector VecLerp(const rmt::Vector& v1, const rmt::Vector& v2, float t)
{
    rmt::Vector ret;
    ret.x = v1.x*(1-t) + v2.x*t;
    ret.y = v1.y*(1-t) + v2.y*t;
    ret.z = v1.z*(1-t) + v2.z*t;
    return ret;
}

inline rmt::Vector VecWghtAv(const rmt::Vector& v1, const rmt::Vector& v2, float v1_frac, float v2_frac, float mirror )
{
    rmt::Vector ret;
    ret.x = mirror * ( v1.x*v1_frac + v2.x * v2_frac );
    ret.y = mirror * ( v1.y*v1_frac + v2.y * v2_frac );
    ret.z = mirror * ( v1.z*v1_frac + v2.z * v2_frac );
    return ret;
}

inline rmt::Vector VecWght(const rmt::Vector& v1, float v1_frac, float mirror)
{
    rmt::Vector ret;
    ret.x = mirror * ( v1.x*v1_frac );
    ret.y = mirror * ( v1.y*v1_frac );
    ret.z = mirror * ( v1.z*v1_frac );
    return ret;
}

inline bool keyEquals(float key1, float key2)
{
    if (fabsf(key1-key2) < P3D_KEY_EPSILON)
        return true;
    else return false;

}

#endif
