//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        objectavltree.hpp
//
// Subsystem:   Foundation Technologies - Binary Tree Descriptions
//
// Description: This file contains a description of the various binary
//              trees and their associated structures.
//
// Revisions:   V1.00   August 21, 2001     Creation        BJR
//
//=============================================================================

#ifndef OBJECTAVLTREE_HPP
#define OBJECTAVLTREE_HPP

//============================================================================
// Include Files
//============================================================================

#include <string.h>
#include <radmemory.hpp>
#include <radobject.hpp>
#include <radobjectlist.hpp>
#include <raddebug.hpp>

#include <radobjectbtree.hpp>
#include "objectbtree.hpp"


//============================================================================
// Classes
//============================================================================

//
// This class implements the balenced (AVL) binary tree.
//
class radObjectAVLTree : public radObjectBTree
{
public:
    radObjectAVLTree( radMemoryAllocator allocator );
    virtual ~radObjectAVLTree( );

    // IRadObjectBTree
    virtual bool AddObject
    (
        const radKey32& key,
        IRefCount* pIObject
    );

    virtual bool RemoveObject( const radKey32& key );
    virtual bool RemoveObject( IRefCount* pIObject );

protected:

    //
    // Calculate the depth of the various subtrees
    //
    int GetMaxDepth( radObjectBTreeNode* pNode );
    int GetLeftDepth( radObjectBTreeNode* pNode );
    int GetRightDepth( radObjectBTreeNode* pNode );

    //
    // Rotate a node in the given direction
    //
    inline void RotateLeft( radObjectBTreeNode* pNode );
    inline void RotateRight( radObjectBTreeNode* pNode );
    inline void RotateDoubleLeft( radObjectBTreeNode* pNode );
    inline void RotateDoubleRight( radObjectBTreeNode* pNode );

    //
    // Generic function to rebalence a node
    //
    void BalanceNode( radObjectBTreeNode* pNode );
    void DoBalance
    (
        radObjectBTreeNode* pNode,
        int lefttDepth,
        int rightDepth,
        int maxDepth
    );

private:
};


#endif //OBJECTAVLTREE_HPP
