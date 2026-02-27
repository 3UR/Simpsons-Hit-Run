/*=============================================================================
  vertexoffsetexpression.cpp
  30-Aug-2000
  Created by: Bryan Brandt

  Description: Pure3D vertex offset expression classes for parameterized
                    vertex animation
  Copyright (c) 2000 Radical Entertainment, Inc.
  All Rights Reserved
=============================================================================*/

#include <string.h>
#include <stdlib.h>

#include <p3d/anim/polyskin.hpp>
#include <p3d/anim/vertexoffsetexpression.hpp>

#include <new>

#ifdef _PERF_WIN

#include <windows.h>
#include <winbase.h>

/////////////////////////////////////////////////////////////////
//implement a timer for performance testing

class winTimer
{
public:
    winTimer():startTime(0), stopTime(0), frequency(0){};
    ~winTimer(){};
    void startTimer();
    void resetTimer();
    void stopTimer();
    char elapsedTime[64];
private:
    DWORD startTime;
    DWORD stopTime;
    DWORD frequency;
};

winTimer updateTimer;
winTimer setVerticesTimer;
#endif


//-------------------------------------------------------------------
tVertexOffsetExpressionMixer::tVertexOffsetExpressionMixer() :
    tExpressionMixer(),
    m_nPrimGroups(0),
    m_nVertices(NULL),
    m_pVtxOffsets(NULL),
    m_pDrawable(NULL),
    m_pOffsetArray(NULL),
    m_nMaxNumStages(0),
    m_uTotalNumOffsets(0),
    m_pResultOffsets(NULL),
    m_diagnose(true),
    m_ExpressionCalcCashSize(0),
    m_ExpressionCalcCash(NULL)
{
    //
}

tVertexOffsetExpressionMixer::~tVertexOffsetExpressionMixer()
{

    int i;
    for(i = 0; i < m_nPrimGroups; i++)
    {
        if(m_pVtxOffsets[i]) delete[] m_pVtxOffsets[i];
    }

    delete[] m_pVtxOffsets;
    m_pVtxOffsets = NULL;

    for(i = 0; i < m_nPrimGroups; i++)
    {
        if(m_pResultOffsets[i]) delete[] m_pResultOffsets[i];
    }

    delete[] m_pResultOffsets;
    m_pResultOffsets = NULL;

    delete[] m_nVertices;
    m_nVertices = NULL;

    delete[] m_pOffsetArray;
    m_pOffsetArray = NULL;

    tRefCounted::Release(m_pDrawable);
    delete[] m_ExpressionCalcCash;
}


bool tVertexOffsetExpressionMixer::ValidateExpressionGroup(tExpressionGroup* eg)
{
    int nExpr = eg->GetNumExpression();

    // just check to make sure all expressions are in fact tExpressions
    for (int i = 0; i < nExpr; i++)
    {
        tExpression* expr = eg->GetExpression(i);

        if(expr == NULL)
        {
            return false;
        }
    }

    return true;
}

void tVertexOffsetExpressionMixer::SetExpressionGroup(tExpressionGroup* eg)
{
    
    tExpressionMixer::SetExpressionGroup(eg);

    delete[] m_ExpressionCalcCash;
    m_ExpressionCalcCashSize = GetNumExpression();
    m_ExpressionCalcCash = new ExpressionCalcCash[m_ExpressionCalcCashSize];
}

bool tVertexOffsetExpressionMixer::ValidateTarget(tEntity* e)
{
    tDrawable* target = dynamic_cast<tDrawable*>(e);
    if (target == NULL)
    {
        return false;
    }

    P3DASSERT(expressionGroup != NULL);

    /*
    // Removed by njv.  This doesn't work with sections and asynchrnous loading since
    // the object we are searching for might be in a different store.
    tUID uid = expressionGroup->GetTarget();
    tDrawable* egTarget = p3d::find<tDrawable>(uid);
    if (egTarget == target)
    {
        return true;
    }

    return false;
    */
    return true;
}



//---------------------------------------------------------------------------
//  Description: Use to set a new mesh target for this mixer.
//  Performs necessary clean-up and reallocation of memory when    
//  a new tPolySkin target is specified
//
//  Parameters: a tPolySkin
//
//  Warning: Ensure that this target has the required tOffsetLists for the
//  current expression group assigned to this Mixer.
//
//---------------------------------------------------------------------------
void tVertexOffsetExpressionMixer::SetTarget(tEntity* t)
{
    P3DASSERT(ValidateTarget(t));

    // guaranteed we have a tGeometry or tPolySkin from ValidateTarget
    tDrawable* dw_tmp = (tDrawable*)(t);
    tRefCounted::Assign(m_pDrawable, dw_tmp);

// 1. Check that the target is a tPolySkin: expression animations only apply to skins

    tPolySkin* pSkin = dynamic_cast<tPolySkin*>(m_pDrawable);
    P3DASSERT(pSkin && "Drawable must be a skin");

    //Check that this skin has offsets
    tExpressionOffsets* expressionOffsets = pSkin->GetExpressionOffsets();
    P3DASSERTMSG(expressionOffsets,"Invalid target.", "tVertexOffsetExpressionMixer::SetTarget");
    P3DASSERTMSG(expressionOffsets->GetNumOffsetLists(), "Invalid target. Target has no offset lists.", "tVertexOffsetExpressionMixer::SetTarget");

    expressionOffsets->AddRef();

// 2. Check if target previously set if so delete existing data strucs
    if(m_pVtxOffsets)
    {
        int i;
        for(i = 0; i < m_nPrimGroups; i++)
        {
            delete[] m_pVtxOffsets[i];
        }
        delete[] m_pVtxOffsets; 
        m_pVtxOffsets = NULL;
        delete[] m_nVertices;
        m_nVertices = NULL;
    }
    if(m_pResultOffsets)
    {
        int i;
        for(i = 0; i < m_nPrimGroups; i++)
        {
            delete[] m_pResultOffsets[i];
        }        
        m_pResultOffsets = NULL;
    }

// 3. Allocate memory
    m_nPrimGroups = pSkin->GetNumPrimGroups();

    m_nVertices = new unsigned[m_nPrimGroups];
    P3DASSERT(m_nVertices); 

    m_pVtxOffsets = new vertexData*[m_nPrimGroups];
    P3DASSERT(m_pVtxOffsets);

    m_pResultOffsets = new rmt::Vector*[m_nPrimGroups];

    int i;
    for(i = 0; i < m_nPrimGroups; i++)
    {
        tPrimGroup* pPrimGroupSkinned = pSkin->GetPrimGroupSkinned(i);
        m_nVertices[i] = pPrimGroupSkinned->GetVertexCount();

        if(expressionOffsets->hasExpression(i) )
        {
            m_pVtxOffsets[i] = new vertexData[m_nVertices[i]];
            rmt::Vector* tempVtxs = new rmt::Vector[m_nVertices[i]];
            m_pResultOffsets[i] = new rmt::Vector[m_nVertices[i]];

            // NOTE: Modify this when offsets are added to original vtx pos in hardware
            pPrimGroupSkinned->GetVertices(0, m_nVertices[i],tempVtxs);
            for(unsigned j = 0; j <  m_nVertices[i]; j++)
            {
                m_pVtxOffsets[i][j].position = tempVtxs[j];
                m_pResultOffsets[i][j].x = 0;
                m_pResultOffsets[i][j].y = 0;
                m_pResultOffsets[i][j].z = 0;
            }
            delete[] tempVtxs;
        }
        else 
        {
            m_pVtxOffsets[i] = NULL;
            m_pResultOffsets[i] = NULL;
        }
    }

    CompactBlendStages();
    mixerReady = false;
    expressionOffsets->Release();
}


void tVertexOffsetExpressionMixer::CompactBlendStages()
{
    unsigned int numExp = expressionGroup->GetNumExpression();
    unsigned int allPossibleStages[P3D_VERTEXOFFSET_NUM_ACCUMULATE_STAGES];
    memset(allPossibleStages, 0, P3D_VERTEXOFFSET_NUM_ACCUMULATE_STAGES * sizeof(unsigned int));
    p3dExpressionStage* actualStages = new p3dExpressionStage[numExp];
    memset(actualStages, 0, numExp * sizeof(p3dExpressionStage));
    
    // Here we want to find out if all the blend stages are actually being used
    // so that in the Update call we can use a m_nMaxNumStages variable, which might be
    // less than the NUM_ACCUMULATE_STAGES.   So what we do is create a bit array of all
    // the possible stages, e.g. 0 1 2.  Then we fill it in if necessary: e.g. 1 0 1 would
    // correspond to blend stages 0 and 2 being used.  Then we need to compact the blend
    // stages, so if there aren't any 1's, we change all the 2's to 1's.  Now we have a
    // m_nMaxNumStages of 2 instead of 3, and we set all the blendStages to 1 instead of 2.
    unsigned int expIter, stageIter;
    p3dExpressionStage lastEmpty;
    for(expIter = 0; expIter < numExp; expIter++)
    {
        p3dExpressionStage expStage = expressionGroup->GetExpressionStage(expIter);
        actualStages[expIter] = expStage;
        allPossibleStages[expStage] = 1;
    }


    for (stageIter = 0; stageIter < P3D_VERTEXOFFSET_NUM_ACCUMULATE_STAGES; stageIter++)
    {
        if (allPossibleStages[stageIter] == 0)
        {
            lastEmpty = p3dExpressionStage(stageIter);
            for (expIter = 0; expIter < numExp; expIter++)
            {
                if (actualStages[expIter] == p3dExpressionStage(stageIter + 1))
                {
                    actualStages[expIter] = lastEmpty;
                    allPossibleStages[stageIter] = 1;
                    allPossibleStages[stageIter+1] = 0;
                }
            }
        }
    }

    for (stageIter = 0;stageIter < P3D_VERTEXOFFSET_NUM_ACCUMULATE_STAGES; stageIter++)
    {
        if (allPossibleStages[stageIter] == 1)
        {
            m_nMaxNumStages++;
        }
    }

    for (expIter = 0; expIter < numExp; expIter++)
    {
        expressionGroup->SetExpressionStage(expIter, actualStages[expIter]);
    }

    delete[] actualStages;

    InitOffsetArray();
}


void tVertexOffsetExpressionMixer::ResetAccumulators()
{
    // Reinitialize intermediate values (m_pVtxOffsets[i][j].offset & m_pResultOffsets to zero)
    for(int k = 0; k < m_nPrimGroups; k++)
    {
        if(m_pVtxOffsets[k] && m_pResultOffsets[k])
        {
            for(unsigned l = 0; l < m_nVertices[k]; l++)
            {
                m_pVtxOffsets[k][l].offset.x = 0;
                m_pVtxOffsets[k][l].offset.y = 0;
                m_pVtxOffsets[k][l].offset.z = 0;
                m_pResultOffsets[k][l].x = 0;
                m_pResultOffsets[k][l].y = 0;
                m_pResultOffsets[k][l].z = 0;
            }
        }
    }
}


//----------------------------------------------------------------------------
//  Description: Invoked during each tBlendframeController update call. tAnimController
//  calculaes the the weight of each expression . This method does the following
//  for each vertex in m_pOffsetArray (i) calculate the offset contribution for
//  expression as the weighted average of offsets defined by two bounding keys
//  (ii) take the weighted average the offset contributions of expressions in the
//  same blendstage to determine the offset contribution of each blendstage
//  (iii) sum the offset contributions of all blend stages.  
//
//  Parameters: none
//
//  Returns: calculates offset vectors (currently also sums the original vtx position
//  and offset vector and applies it to each skin primgroup .. this step should be removed
//  and the addition should be done in PDDI )
//
//----------------------------------------------------------------------------/
#if(1)
void tVertexOffsetExpressionMixer::Update()
{
    if (mixerReady)
    {
      //  return;
    }

    ResetAccumulators();

#ifdef _PERF_WIN
    updateTimer.startTimer();
    unsigned int expCount = 0;
#endif
    
    int exprIndex;
    int i;
    rmt::Vector zeroVector(0.0f,0.0f,0.0f);

    //
    // Initialize arrays to store intermediate and final results
    //
    rmt::Vector vertexOffsets[P3D_VERTEXOFFSET_NUM_ACCUMULATE_STAGES];    
    float vertexWeights[P3D_VERTEXOFFSET_NUM_ACCUMULATE_STAGES] = {0.0f}; 
    memset(vertexOffsets, 0, sizeof(rmt::Vector)*P3D_VERTEXOFFSET_NUM_ACCUMULATE_STAGES);

    Offset* curOffset, *endOffset, *lowOffset, *highOffset;
    curOffset = m_pOffsetArray;
    endOffset = m_pOffsetArray + m_uTotalNumOffsets;

    //
    // Expression stuff
    //
    float recWeight;
    float curKey;
    float upperKey = 0;
    float lowerKey = 0;
    tExpression* expr = NULL;
    bool negKeysExists = false, posKeysExists = false;
    p3dExpressionStage exprStage = P3D_EXPRESSION_STAGE_1;

    //
    // Create this frame's weight mask
    //
    unsigned int curExpMask = 0;
    int numExpression = expressionGroup->GetNumExpression();
    for (exprIndex=0; exprIndex < numExpression; exprIndex++)
    {
        if(state[exprIndex] != 0.0f)
        {

#ifdef _PERF_WIN
    expCount++;
#endif
            curExpMask |= (1 << exprIndex);

//
// PreCompute blending info
//
            float absExprWeight, exprWeight, low_frac = 0.0f, high_frac= 0.0f, mirror = 1.0f;                
            expr = expressionGroup->GetExpression(exprIndex);
            negKeysExists = expr->NegKeysExist();
            posKeysExists = expr->PosKeysExist();
            exprWeight = state[exprIndex]; // this array is filled by tExpressionFrameController::Update()

            exprStage  = expressionGroup->GetExpressionStage(exprIndex);
            absExprWeight = rmt::Fabs(exprWeight);

            //
            // Determine the upper and lower bounding key given current 
            // weight of this expression
            // Note: if the current weight is equals of a key, 
            // or if current weight is less all keys in
            // expression or greater than all keys in expression, 
            // then upperKey will equal lowerKey
            //
            if(exprWeight < 0 && !negKeysExists)
            {
                upperKey = expr->GetUpperBoundKey(absExprWeight);
                lowerKey = expr->GetLowerBoundKey(absExprWeight);
                mirror = -1.0f;
            }
            else if(exprWeight > 0 && !posKeysExists)
            {
                upperKey = expr->GetUpperBoundKey(-1 * exprWeight);
                lowerKey = expr->GetLowerBoundKey(-1 * exprWeight);
                mirror = -1.0f;
            }
            else
            {
                upperKey = expr->GetUpperBoundKey(exprWeight);
                lowerKey = expr->GetLowerBoundKey(exprWeight);
                mirror = 1.0f;
            }
            
            //
            // calculate the weighted average factor for this offset vector
            //
            if(upperKey == lowerKey) 
            {
                high_frac = 1.0f;
                low_frac = 0.0f;
            }
            else if ( keyEquals(upperKey, 0) ) 
            {
                high_frac = 0.0f;
                low_frac = rmt::Fabs(exprWeight / lowerKey);
            }
            else if ( keyEquals(lowerKey, 0) )
            {
                low_frac = 0.0f;
                high_frac = rmt::Fabs(exprWeight / upperKey);
            }
            else
            {
                high_frac = rmt::Fabs( (exprWeight - lowerKey) / (upperKey - lowerKey) );
                low_frac = 1 - high_frac;
            }

            P3DASSERT(upperKey != tExpression::INVALID_EXPRESSION_KEYVAL || 
                      lowerKey != tExpression::INVALID_EXPRESSION_KEYVAL);

            //
            // Cache the results
            //
            m_ExpressionCalcCash[exprIndex].absExprWeight = absExprWeight;
            m_ExpressionCalcCash[exprIndex].exprWeight  = exprWeight; 
            m_ExpressionCalcCash[exprIndex].low_frac    = low_frac;
            m_ExpressionCalcCash[exprIndex].high_frac   = high_frac;
            m_ExpressionCalcCash[exprIndex].mirror      = mirror;
        }
        else
        {
            // Zero?
        }
    }
/*
    if (curExpMask == 0)
    {
        mixerReady = true;
        return;
    }
*/
    vertexData* vertex;
    rmt::Vector interpVtx;
//
// looping stats
//
int offestLoop = 0;
int offsetVertLoop = 0;
int expressionLoop = 0;


    //
    // For all offset data in offset_array
    //
    while (curOffset < endOffset) 
    {
        vertex = curOffset->vertex;
offestLoop++;
        //
        // Per-Vertex Loop!
        // Calculate the offsets to be applied to this vertex
        // 
        do 
        {
offsetVertLoop++;
            //
            // Compute & accumulate the offset for this vertex: 
            // each expression contributes a certain amount
            // to this vertex's offset. 
            //
            exprIndex  = curOffset->expressionIndex;
            //
            // quick test to see if this expression participates
            //
            if (curExpMask & (1 << exprIndex))  
            {
                float absExprWeight = m_ExpressionCalcCash[exprIndex].absExprWeight;
                float exprWeight    = m_ExpressionCalcCash[exprIndex].exprWeight; 
                float low_frac      = m_ExpressionCalcCash[exprIndex].low_frac;
                float high_frac     = m_ExpressionCalcCash[exprIndex].high_frac;
                float mirror        = m_ExpressionCalcCash[exprIndex].mirror;         
                
                highOffset = lowOffset = NULL;
                bool foundKeys = false;

                //
                // Accumulate this expression's contribution to the 
                // final offset for this vertex
                //
                do 
                {
expressionLoop++;
                    //
                    // Iterate through each offset in expr until you find 
                    // upperKey/lowerKey in m_poffsetArray
                    // then get their offsets to calculate the weighted average 
                    //
                    curKey = curOffset->keyVal;

                    if(curKey == lowerKey)
                    {
                        lowOffset = curOffset;
                    }

                    if(curKey == upperKey)
                    {
                        highOffset = curOffset;
                    }

                    //
                    // Found offset values for bounding keys - 
                    // calculate the offset and accumulate it
                    //
                    if( keyEquals(high_frac, 0) && lowOffset != NULL)
                    {
                        interpVtx = VecWght(lowOffset->offset, low_frac, mirror);
                        vertexOffsets[exprStage].ScaleAdd(absExprWeight, interpVtx);
                        vertexWeights[exprStage]+=absExprWeight;
                        foundKeys = true;
                        break;
                    }
                    else if( keyEquals(low_frac, 0) && highOffset != NULL)
                    {
                        interpVtx = VecWght(highOffset->offset, high_frac, mirror);
                        vertexOffsets[exprStage].ScaleAdd(absExprWeight, interpVtx);
                        vertexWeights[exprStage]+=absExprWeight;
                        foundKeys = true;
                        break;
                    }
                    else if(lowOffset != NULL && highOffset != NULL)
                    {
                        if(highOffset == lowOffset)
                        {
                            vertexOffsets[exprStage].ScaleAdd(1, highOffset->offset);
                            vertexWeights[exprStage]+=absExprWeight;
                        }
                        else
                        {
                            interpVtx = VecWghtAv(lowOffset->offset, highOffset->offset, low_frac, high_frac, mirror);
                            vertexOffsets[exprStage].ScaleAdd(absExprWeight, interpVtx);
                            vertexWeights[exprStage]+=absExprWeight;
                        }
                        foundKeys = true;
                        break;
                    }
                    else 
                    {
                        //
                        // Advance to the next element in m_pOffsetArray
                        //
                        curOffset++;     
                    }

                } while ( (exprIndex == curOffset->expressionIndex) && (curOffset->vertex == vertex) );


                if(m_diagnose)
                {
                    //
                    // Expression group assigned to this mixer and the mixer's 
                    // target are mismatched
                    // Currently disabled

           //         P3DASSERTMSG(foundKeys, "Detected expression group and mixer target mismatch", "tVertexExpressionMixer::Update");
                }

                //
                // Found high and low bounding keys so advance to the next expression
                //
                while( (exprIndex == curOffset->expressionIndex) && (curOffset->vertex == vertex) )
                {
                    curOffset++;
                }
            } //end if
            else 
            {
                curOffset++;
            }

        } while(curOffset->vertex == vertex);

        //
        // Average the offsets from expressions to get the contribution of a blendstage
        // sum contributions of all blendstage
        //
        for (int s = 0; s < m_nMaxNumStages; s++)
        {
            if (vertexWeights[s] > 0.0f)
            {
                recWeight = 1.0f / vertexWeights[s];
                vertex->offset.ScaleAdd(recWeight, vertexOffsets[s]);
                vertexOffsets[s] = zeroVector;
                vertexWeights[s] = 0.0f;
            }
        }
    } // curOffset == endOffset (no more offsets to process)

#ifdef _PERF_WIN
    p3d::pddi->DrawString("Time: ", 250, 50);
    updateTimer.stopTimer();
    p3d::pddi->DrawString("    ", 300, 50);
    p3d::pddi->DrawString(updateTimer.elapsedTime, 300, 50);
    char expBuff[32];
    sprintf(expBuff, "%d", expCount);
    p3d::pddi->DrawString(expBuff, 400, 50);
#endif

#ifdef _PERF_WIN
    updateTimer.startTimer();    
#endif
    //
    // Apply the new vertex positions
    //
    tPolySkin* pSkin = static_cast<tPolySkin*>(m_pDrawable);
    P3DASSERT( pSkin->GetNumPrimGroups() == m_nPrimGroups );
    for(i = 0; i < m_nPrimGroups; i++)
    {
        if(m_pVtxOffsets[i])
        {
            tPrimGroup* pPrimGroupSkinned = pSkin->GetPrimGroupSkinned(i);
            P3DASSERT( pPrimGroupSkinned->GetVertexCount() == m_nVertices[i] );

            //
            // Get sum of resultOffsets = Offsets + original Vtx position
            //
            for(unsigned j = 0; j < m_nVertices[i]; j++)
            {
                P3DASSERT(m_pResultOffsets[i]);
                m_pResultOffsets[i][j].Add(m_pVtxOffsets[i][j].position, m_pVtxOffsets[i][j].offset);
            }
            pPrimGroupSkinned->SetVertices(0, m_nVertices[i], m_pResultOffsets[i]);
        }
    }

    /*
#ifdef _PERF_WIN
    p3d::pddi->DrawString("Time: ", 250, 70);
    updateTimer.stopTimer();
    p3d::pddi->DrawString("    ", 300, 70);
    p3d::pddi->DrawString(updateTimer.elapsedTime, 300, 70);
#endif

    char offestLoopChar[124];
    sprintf(offestLoopChar,"offestLoop: %d", offestLoop);
    p3d::pddi->DrawString(offestLoopChar, 0, 50);

    char offsetVertLoopChar[124];
    sprintf(offsetVertLoopChar,"offsetVertLoop: %d", offsetVertLoop);
    p3d::pddi->DrawString(offsetVertLoopChar, 0, 70);

    char expressionLoopChar[124];
    sprintf(expressionLoopChar,"expressionLoop: %d", expressionLoop);
    p3d::pddi->DrawString(expressionLoopChar, 0, 90);
    */
    mixerReady = true;
}

#else

void tVertexOffsetExpressionMixer::Update()
{
    if (mixerReady)
    {
       // return;
    }

    ResetAccumulators();

#ifdef _PERF_WIN
    updateTimer.startTimer();
    unsigned int expCount = 0;
#endif
    
    int exprIndex;
    int i;
    rmt::Vector zeroVector(0.0f,0.0f,0.0f);

    //
    // Initialize arrays to store intermediate and final results
    //
    rmt::Vector vertexOffsets[P3D_VERTEXOFFSET_NUM_ACCUMULATE_STAGES];    
    float vertexWeights[P3D_VERTEXOFFSET_NUM_ACCUMULATE_STAGES] = {0.0f}; 
    memset(vertexOffsets, 0, sizeof(rmt::Vector)*P3D_VERTEXOFFSET_NUM_ACCUMULATE_STAGES);

    Offset* curOffset, *endOffset, *lowOffset, *highOffset;
    curOffset = m_pOffsetArray;
    endOffset = m_pOffsetArray + m_uTotalNumOffsets;

    //
    // Create this frame's weight mask
    //
    unsigned int curExpMask = 0;
    int numExpression = expressionGroup->GetNumExpression();
    for (i=0; i < numExpression; i++)
    {
        if(state[i] != 0.0f)
        {

#ifdef _PERF_WIN
    expCount++;
#endif
            curExpMask |= (1 << i);
        }
    }

    if (curExpMask == 0)
    {
        mixerReady = true;
        return;
    }

    p3dExpressionStage exprStage; 
    float absExprWeight, exprWeight, low_frac = 0.0f, high_frac= 0.0f, mirror = 1.0f;
    float recWeight,curKey, upperKey, lowerKey;
    vertexData* vertex;
    rmt::Vector interpVtx;
    tExpression* expr = NULL;
    bool negKeysExists = false, posKeysExists = false;

    //
    // For all offset data in offset_array
    //
    while (curOffset < endOffset) 
    {
        vertex = curOffset->vertex;
        
        //
        // Calculate the offsets to be applied to this vertex
        //
        do 
        {
            //
            // Compute & accumulate the offset for this vertex: 
            // each expression contributes a certain amount
            // to this vertex's offset. 
            //
            exprIndex  = curOffset->expressionIndex;
            //
            // quick test to see if this expression participates
            //
            if (curExpMask & (1 << exprIndex))  
            {
                expr = expressionGroup->GetExpression(exprIndex);
                negKeysExists = expr->NegKeysExist();
                posKeysExists = expr->PosKeysExist();
                exprWeight = state[exprIndex]; // this array is filled by tExpressionFrameController::Update()

                exprStage  = expressionGroup->GetExpressionStage(exprIndex);
                absExprWeight = rmt::Fabs(exprWeight);

                //
                // Determine the upper and lower bounding key given current 
                // weight of this expression
                // Note: if the current weight is equals of a key, 
                // or if current weight is less all keys in
                // expression or greater than all keys in expression, 
                // then upperKey will equal lowerKey
                //
                if(exprWeight < 0 && !negKeysExists)
                {
                    upperKey = expr->GetUpperBoundKey(fabsf(exprWeight));
                    lowerKey = expr->GetLowerBoundKey(fabsf(exprWeight));
                    mirror = -1.0f;
                }
                else if(exprWeight > 0 && !posKeysExists)
                {
                    upperKey = expr->GetUpperBoundKey(-1 * exprWeight);
                    lowerKey = expr->GetLowerBoundKey(-1 * exprWeight);
                    mirror = -1.0f;
                }
                else
                {
                    upperKey = expr->GetUpperBoundKey(exprWeight);
                    lowerKey = expr->GetLowerBoundKey(exprWeight);
                    mirror = 1.0f;
                }
                
                //
                // calculate the weighted average factor for this offset vector
                //
                if(upperKey == lowerKey) 
                {
                    high_frac = 1.0f;
                    low_frac = 0.0f;
                }
                else if ( keyEquals(upperKey, 0) ) 
                {
                    high_frac = 0.0f;
                    low_frac = rmt::Fabs(exprWeight / lowerKey);
                }
                else if ( keyEquals(lowerKey, 0) )
                {
                    low_frac = 0.0f;
                    high_frac = rmt::Fabs(exprWeight / upperKey);
                }
                else
                {
                    high_frac = rmt::Fabs( (exprWeight - lowerKey) / (upperKey - lowerKey) );
                    low_frac = 1 - high_frac;
                }

                P3DASSERT(upperKey != tExpression::INVALID_EXPRESSION_KEYVAL || 
                          lowerKey != tExpression::INVALID_EXPRESSION_KEYVAL); 

                highOffset = lowOffset = NULL;
                bool foundKeys = false;

                //
                // Accumulate this expression's contribution to the 
                // final offset for this vertex
                //
                do 
                {
                    //
                    // Iterate through each offset in expr until you find 
                    // upperKey/lowerKey in m_poffsetArray
                    // then get their offsets to calculate the weighted average 
                    //
                    curKey = curOffset->keyVal;

                    if(curKey == lowerKey)
                    {
                        lowOffset = curOffset;
                    }
                    else if(curKey == upperKey)
                    {
                        highOffset = curOffset;
                    }

                    //
                    // Found offset values for bounding keys - 
                    // calculate the offset and accumulate it
                    //
                    if( keyEquals(high_frac, 0) && lowOffset != NULL)
                    {
                        interpVtx = VecWght(lowOffset->offset, low_frac, mirror);
                        vertexOffsets[exprStage].ScaleAdd(absExprWeight, interpVtx);
                        vertexWeights[exprStage]+=absExprWeight;
                        foundKeys = true;
                        break;
                    }
                    else if( keyEquals(low_frac, 0) && highOffset != NULL)
                    {
                        interpVtx = VecWght(highOffset->offset, high_frac, mirror);
                        vertexOffsets[exprStage].ScaleAdd(absExprWeight, interpVtx);
                        vertexWeights[exprStage]+=absExprWeight;
                        foundKeys = true;
                        break;
                    }
                    else if(lowOffset != NULL && highOffset != NULL)
                    {
                        if(highOffset == lowOffset)
                        {
                            vertexOffsets[exprStage].ScaleAdd(1, highOffset->offset);
                            vertexWeights[exprStage]+=absExprWeight;
                        }
                        else
                        {
                            interpVtx = VecWghtAv(lowOffset->offset, highOffset->offset, low_frac, high_frac, mirror);
                            vertexOffsets[exprStage].ScaleAdd(absExprWeight, interpVtx);
                            vertexWeights[exprStage]+=absExprWeight;
                        }
                        foundKeys = true;
                        break;
                    }
                    else 
                    {
                        //
                        // Advance to the next element in m_pOffsetArray
                        //
                        curOffset++;     
                    }

                } while ( (exprIndex == curOffset->expressionIndex) && (curOffset->vertex == vertex) );


                if(m_diagnose)
                {
                    //
                    // Expression group assigned to this mixer and the mixer's 
                    // target are mismatched
                    // Currently disabled

           //         P3DASSERTMSG(foundKeys, "Detected expression group and mixer target mismatch", "tVertexExpressionMixer::Update");
                }

                //
                // Found high and low bounding keys so advance to the next expression
                //
                while( (exprIndex == curOffset->expressionIndex) && (curOffset->vertex == vertex) )
                {
                    curOffset++;
                }
            } //end if
            else 
            {
                curOffset++;
            }

        } while(curOffset->vertex == vertex);

        //
        // Average the offsets from expressions to get the contribution of a blendstage
        // sum contributions of all blendstage
        //
        for (int s = 0; s < m_nMaxNumStages; s++)
        {
            if (vertexWeights[s] > 0.0f)
            {
                recWeight = 1.0f / vertexWeights[s];
                vertex->offset.ScaleAdd(recWeight, vertexOffsets[s]);
                vertexOffsets[s] = zeroVector;
                vertexWeights[s] = 0.0f;
            }
        }
    } // curOffset == endOffset (no more offsets to process)

#ifdef _PERF_WIN
    p3d::pddi->DrawString("Time: ", 250, 50);
    updateTimer.stopTimer();
    p3d::pddi->DrawString("    ", 300, 50);
    p3d::pddi->DrawString(updateTimer.elapsedTime, 300, 50);
    char expBuff[32];
    sprintf(expBuff, "%d", expCount);
    p3d::pddi->DrawString(expBuff, 400, 50);
#endif

#ifdef _PERF_WIN
    updateTimer.startTimer();    
#endif
    //
    // Apply the new vertex positions
    //
    tPolySkin* pSkin = static_cast<tPolySkin*>(m_pDrawable);
    P3DASSERT( pSkin->GetNumPrimGroups() == m_nPrimGroups );
    for(i = 0; i < m_nPrimGroups; i++)
    {
        if(m_pVtxOffsets[i])
        {
            tPrimGroup* pPrimGroupSkinned = pSkin->GetPrimGroupSkinned(i);
            P3DASSERT( pPrimGroupSkinned->GetVertexCount() == m_nVertices[i] );

            //
            // Get sum of resultOffsets = Offsets + original Vtx position
            //
            for(unsigned j = 0; j < m_nVertices[i]; j++)
            {
                P3DASSERT(m_pResultOffsets[i]);
                m_pResultOffsets[i][j].Add(m_pVtxOffsets[i][j].position, m_pVtxOffsets[i][j].offset);
            }
            pPrimGroupSkinned->SetVertices(0, m_nVertices[i], m_pResultOffsets[i]);
        }
    }

#ifdef _PERF_WIN
    p3d::pddi->DrawString("Time: ", 250, 70);
    updateTimer.stopTimer();
    p3d::pddi->DrawString("    ", 300, 70);
    p3d::pddi->DrawString(updateTimer.elapsedTime, 300, 70);
#endif

    mixerReady = true;
}
#endif
//----------------------------------------------------------------------------
//  Description: Returns the offsets that should be applied to vtx positions of 
//  primgroup after the mixer update method is called. 
//
//  Parameters:primgroupIdx - the index of the primgroup in the target skin
//
//  Returns:pointer to the resultOffsets array that contains offset vectors.
//  if the primgroup does not have expressions, then return NULL.
//
//----------------------------------------------------------------------------/
const rmt::Vector* tVertexOffsetExpressionMixer::GetVtxOffsets(int primgroupIdx)
{
    if(!mixerReady)
    {
        Update();
    }
    return m_pResultOffsets[primgroupIdx];
}

//----------------------------------------------------------------------------
// Description: Populates and sorts offset data in m_pOffsetArray. m_pOffset
// Array is sorted by vertex pointer, then by expressionIndex, then by keyVal.
//
// Parameters:none
//
// Returns:none
//
//----------------------------------------------------------------------------/
void tVertexOffsetExpressionMixer::InitOffsetArray()
{
    tPolySkin* pSkin = static_cast<tPolySkin*>(m_pDrawable);
    P3DASSERT( pSkin->GetNumPrimGroups() == m_nPrimGroups );
    
    int numExpressions = expressionGroup->GetNumExpression();
    tExpressionOffsets* expressionOffsets = pSkin->GetExpressionOffsets();
    expressionOffsets->AddRef();
    
    // this is a preliminary pass to determine the total number of vertex offsets in this group
    // go through all the expressions in the current group / mesh
    int expIter;
    unsigned t;
    for(expIter = 0; expIter < numExpressions; expIter++)
    {
        // this has already been validated, otherwise it wouldn't have been loaded
        tExpression* exp = static_cast<tExpression*>(expressionGroup->GetExpression(expIter));

        // iterate through all the keys / offset lists
        for (t = 0; t < exp->GetNumKeys(); t++)
        {
            unsigned keyIndex = exp->GetIndex(t);  // the offset list that this key relates to
            m_uTotalNumOffsets += expressionOffsets->GetNumVerticesForKey(keyIndex);
        }
    }

    m_pOffsetArray = new Offset[m_uTotalNumOffsets];
    int curOffset = 0;
    tExpressionOffsets::OffsetList** offsetLists = expressionOffsets->GetOffsetLists();
    
    for(expIter = 0; expIter < numExpressions; expIter++)
    {
        tExpression* exp = static_cast<tExpression*>(expressionGroup->GetExpression(expIter));
        P3DASSERT(exp);

        //iterate through all the keys / offset lists
        for (t = 0; t < exp->GetNumKeys(); t++)
        {
            unsigned keyIndex = exp->GetIndex(t);  
            // All keys belonging to a expression are given
            // a zero-based index.
            // the offset lists are associated with the posekeys by this index.

            float keyValue = exp->GetKey(t);

            bool foundOffsetList = false;
            unsigned i;
            for(i = 0; i < expressionOffsets->GetNumOffsetLists(); i++ )
            {
                tExpressionOffsets::OffsetList* curList = offsetLists[i];
                if(curList->keyIndex == keyIndex)
                {
                    unsigned j;
                    for(j = 0; j < curList->offsetCount; j++ ) //for each vtx in the offset list
                    {
                        //add the vertex and its index to the 'offsetToAdd' struct, and shove that into the holder object
                        tVertexOffsetExpressionMixer::Offset offsetToAdd; 

                        offsetToAdd.expressionIndex = expIter;
                        offsetToAdd.keyVal = keyValue;
                        offsetToAdd.offset = curList->offsets[j].offset;

                        //check memory correctly allocated for m_pVtxOffsets
                        P3DASSERT(m_pVtxOffsets[curList->primGroupIdx]);
                        int vertexIndex = curList->offsets[j].vtxIndex;

                        offsetToAdd.vertex = &(m_pVtxOffsets[curList->primGroupIdx][vertexIndex]);
                        m_pOffsetArray[curOffset++] = offsetToAdd;
                    }
                            foundOffsetList = true;
                }
            }
            if(m_diagnose)
            {
                P3DASSERTMSG(foundOffsetList, "Missing offset list for expression group", "tVertexOffsetExpressionMixer::InitOffsetArray()");
            }
        }
    }

    qsort(m_pOffsetArray, m_uTotalNumOffsets, sizeof(tVertexOffsetExpressionMixer::Offset), compareOffsets);
    tRefCounted::Release(expressionOffsets);
}

// this function compares two elements of type tVertexOffsetExpressionMixer::Offset
// it orders by vertex pointer, then by expression index, then by key value.
// that gets the offset array sorted as we want it
int compareOffsets(const void* el1, const void* el2)
{
    tVertexOffsetExpressionMixer::Offset* off1 = (tVertexOffsetExpressionMixer::Offset*)el1;
    tVertexOffsetExpressionMixer::Offset* off2 = (tVertexOffsetExpressionMixer::Offset*)el2;
    
    if (off1->vertex < off2->vertex) // first we compare the vertex pointers
        return -1;
    else if (off1->vertex > off2->vertex)
        return 1;
    else
    {
        if (off1->expressionIndex < off2->expressionIndex) // if they're the same then we compare the expression indices
            return -1;
        else if (off1->expressionIndex > off2->expressionIndex)
            return 1;
        else
        {
            if (off1->keyVal < off2->keyVal) // if *those* are the same, we compare by key value.           
                return -1;
            else if (off1->keyVal > off2->keyVal)
                return 1;
            else return 0;
        }

    }
}

#ifdef _PERF_WIN
void winTimer::resetTimer()
{
    startTime = 0;
    stopTime = 0;
}

void winTimer::startTimer()
{
    resetTimer();
    QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
    QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
}


void winTimer::stopTimer()
{
    QueryPerformanceCounter((LARGE_INTEGER*)&stopTime);
    //calc elapse time

    double freqMillisec = (double)(frequency/1000);
    double elapseTime = (double)(stopTime - startTime)/freqMillisec;
    sprintf(elapsedTime, "%f", elapseTime);
}

#endif
