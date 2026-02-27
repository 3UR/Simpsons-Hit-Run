/*=============================================================================
  tExpressionOffsets.cpp
  11-Dec-2001
  Created by: Pamela Chow

  Description: Implements a new data chunk for storing offset lists used
  in expression animation

  Copyright (c) 2001 Radical Entertainment, Inc.
  All Rights Reserved
=============================================================================*/

#include <string.h>

#include <p3d/error.hpp>
#include <p3d/utility.hpp>
#include <constants/chunkids.hpp>
#include <p3d/chunkfile.hpp>


#include <p3d/anim/expressionoffsets.hpp>

tExpressionOffsets::tExpressionOffsets():   m_nExprPrimGroups(0),
                                            m_nOffsetLists(0),
                                            m_pPrimGroupIndices(NULL),
                                            m_pOffsetListsGroup(NULL)
{
    
}

unsigned long tExpressionOffsets::GetNumVerticesForKey(int keyIdx)
{
    unsigned long vtxCount = 0;
    unsigned long i;
    for(i = 0; i < m_nOffsetLists; i++)
    {
        OffsetList* thisList= m_pOffsetListsGroup[i];
        if(thisList->keyIndex == (unsigned) keyIdx)
            vtxCount += thisList->offsetCount;
    }
    return vtxCount;
}

void tExpressionOffsets::InsertList(tExpressionOffsets::OffsetList* pOffsetList, int idx)
{
    if(m_pOffsetListsGroup == NULL)
    {
        m_pOffsetListsGroup = new OffsetList*[m_nOffsetLists];
        P3DASSERT(m_pOffsetListsGroup);
    }
    //P3DASSERT(mOffsetListCount < MAX_OFFSETLISTS && "tPrimGroup : too many offset lists");
    m_pOffsetListsGroup[idx] = pOffsetList;
}

bool tExpressionOffsets:: hasExpression(int primgrpIdx)
{
    unsigned long i;
    for(i = 0; i < m_nExprPrimGroups; i++)
    {
        if( m_pPrimGroupIndices[i] == (unsigned) primgrpIdx )
            return true;
    }
    return false;
}
tExpressionOffsets::~tExpressionOffsets()
{
    delete[] m_pPrimGroupIndices;
    if(m_pOffsetListsGroup)
    {
        for(unsigned long i = 0;i < m_nOffsetLists; i++)
        {
            delete[] m_pOffsetListsGroup[i]->offsets;
            delete[] m_pOffsetListsGroup[i];
            m_pOffsetListsGroup[i] = NULL;
        }
    }
    delete[] m_pOffsetListsGroup;
    
}
//-----------------------------------------------------------------------------
tExpressionOffsetsLoader::tExpressionOffsetsLoader()
{

}

tRefCounted* tExpressionOffsetsLoader::LoadObject(tChunkFile* f, tEntityStore*)
{

    unsigned long numPG = f->GetLong();
    unsigned long numOL = f->GetLong();

    tExpressionOffsets* exprOffsets = new tExpressionOffsets();

    exprOffsets->SetNumPrimGroups(numPG);
    exprOffsets->SetNumOffsetLists(numOL);

    exprOffsets->m_pPrimGroupIndices = new unsigned long[numPG];
    for(unsigned long i = 0; i < numPG; i++)
    {
        exprOffsets->m_pPrimGroupIndices[i] = (unsigned)f->GetLong();
    }

    int listIdx = 0;
    while (f->ChunksRemaining())
    {
        switch(f->BeginChunk())
        {
            
            case Pure3D::Mesh::OFFSETLIST:
            {
                tExpressionOffsets::OffsetList* pOffsetList = new tExpressionOffsets::OffsetList;
                P3DASSERT(pOffsetList);

                int numOffsets = f->GetLong();
                pOffsetList->offsetCount = numOffsets;
                pOffsetList->keyIndex = (unsigned)f->GetLong();

                pOffsetList->offsets = new tExpressionOffsets::Offset[numOffsets];
                P3DASSERT(pOffsetList->offsets);
                memset(pOffsetList->offsets, 0, sizeof(tExpressionOffsets::Offset) * numOffsets);

                for (int a = 0; a < numOffsets; ++a)
                {
                    pOffsetList->offsets[a].vtxIndex = (unsigned)f->GetLong();
                    f->GetData(&(pOffsetList->offsets[a].offset), 3, tFile::DWORD);
                }

                pOffsetList->primGroupIdx = (unsigned) f->GetLong();
                exprOffsets->InsertList(pOffsetList, listIdx);
                listIdx++;
                break;
            }

            default:    break;
        }// switch ID

        f->EndChunk();
    } // while !EndOfChunk
    return exprOffsets;
}
