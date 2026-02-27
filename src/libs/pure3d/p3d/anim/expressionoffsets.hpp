/*=============================================================================
  expressionoffsets.hpp
  11-December-2001
  Created by: Pamela Chow

  Description: Class for storing vertex offset data for Expression animation
               
  Copyright (c) 2001-2002 Radical Entertainment, Inc.
  All Rights Reserved
=============================================================================*/

#ifndef _EXPRESSIONANIMOFFSETS_HPP
#define _EXPRESSIONANIMOFFSETS_HPP

#include <p3d/refcounted.hpp>
#include <radmath/radmath.hpp>


class tExpressionOffsets : public tRefCounted
{
public:
    tExpressionOffsets();
    ~tExpressionOffsets();

    struct Offset
    {
        unsigned int vtxIndex;
        rmt::Vector  offset;
    };

    struct OffsetList
    {
        unsigned int offsetCount;
        unsigned int keyIndex;
        unsigned int primGroupIdx;
        Offset *offsets;
    };
    
    typedef OffsetList** OffsetListGroup;

    void SetNumPrimGroups(unsigned long n_primgroups) { m_nExprPrimGroups =  n_primgroups;} 
    void SetNumOffsetLists(unsigned long n_offsetlists) { m_nOffsetLists =  n_offsetlists;} 

    bool hasExpression(int primgrpIdx);
    unsigned long GetNumPrimGroups() { return m_nExprPrimGroups; }
    unsigned long GetNumOffsetLists() { return m_nOffsetLists; }
    unsigned long GetNumVerticesForKey(int keyIdx);
    OffsetList** GetOffsetLists() { return m_pOffsetListsGroup; }
 


protected:
    friend class tExpressionOffsetsLoader;
    void InsertList(tExpressionOffsets::OffsetList* pOffsetList, int idx);

    

    unsigned long    m_nExprPrimGroups;    //number of primgroups which have vtx offset data
    unsigned long    m_nOffsetLists;        //number of offset lists
    unsigned long*    m_pPrimGroupIndices;//primgroups which have offset data
    OffsetListGroup    m_pOffsetListsGroup;


};

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

class tExpressionOffsetsLoader
{
public:
    tExpressionOffsetsLoader();
    ~tExpressionOffsetsLoader() {};
    tRefCounted* LoadObject(tChunkFile*, tEntityStore* store);
protected:  
    
};

#endif
