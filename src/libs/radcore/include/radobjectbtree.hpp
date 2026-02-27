//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radobjectbtree.hpp
//
// Subsystem:   Foundation Technologies - Radcore
//
// Description: Functions and data structyures for storing a balanced binary
//              tree of reference counted nodes.
//
//=============================================================================

#ifndef RADOBJECTBTREE_HPP
#define RADOBJECTBTREE_HPP

//=============================================================================
// Included files
//=============================================================================

#include <radkey.hpp>
#include <radmemory.hpp>
#include <radobject.hpp>
#include <radobjectlist.hpp>
#include <raddebug.hpp>

//=============================================================================
// Factory functions
//=============================================================================

struct radObjectBTreeNode
{
    radKey32 m_key;
    radRef< IRefCount > m_xIObject;

    short mLeftNode;
    short mRightNode;
};

//============================================================================
// Classes
//============================================================================

//
// This class actually implements the binary tree.
//
class radObjectBTree : public radRefCount
{
public:
    
    static void Initialize( radObjectBTreeNode* pNodePool, unsigned int maxNodex );
    static short GetNumAllocatedNodes( void );
    static void Terminate( void );
    
    radObjectBTree( );
    virtual ~radObjectBTree( );

    // IRefCount
    IMPLEMENT_REFCOUNTED( "radObjectBTree" )

    // IRadObjectBTree
    bool AddObject
    (
        const radKey32& key,
        IRefCount* pIObject
    );

    bool RemoveObject( const radKey32& key );
    bool RemoveObject( IRefCount* pIObject );
    void RemoveAll( void );

    IRefCount* FindObject( const radKey32& key );

    enum TraversalOrder
    {
        PreOrder,
        InOrder,
        PostOrder,
        MaxTraversalOrder

    };
    
    IRefCount* GetFirst
    (
        TraversalOrder traversalOrder,
        radKey32* pKey
    );
    IRefCount* GetNext
    (
        radKey32* pKey
    );

    unsigned int GetSize( void );

protected:

#ifdef RAD_DEBUG
    //
    // Debug output
    //
    void DisplayTree
    (
        radObjectBTreeNode* pNode,
        int indent = 0
    );
#endif

    // Find a node
    void find
    (
        const radKey32& key,
        radObjectBTreeNode** ppNode,
        radObjectBTreeNode** ppParent,
        int* pSide
    );

    // Copy a node
    /*TODO inline*/ void copyNode
    (
        radObjectBTreeNode* pSrc,
        radObjectBTreeNode* pDest
    );

    // Helper function for remove nodes
    void RemoveAll_Helper( radObjectBTreeNode* pNode );

    // DATA MEMBERS ///////////////////////////////////////////////////////////

    //
    // A pointer to the root node
    //
    
    short mRootNode;

    
private:

    // Delete a node
    inline void deleteNode( radObjectBTreeNode* pNode );

    // Perform the actual traversal to the next node
    radObjectBTreeNode* GetNextNode( );

    // DATA MEMBERS ///////////////////////////////////////////////////////////

    //
    // The number of elements in the tree
    //
    unsigned int m_Size;


    //
    // Store information about the current traversal.
    // My convention is that if m_pLastTraverseNode is
    // NULL, there is no traversal active.
    //
    TraversalOrder m_TraversalOrder;
    radObjectBTreeNode* m_pLastTraverseNode;
    int m_TraversalState;
};

#endif //RADOBJECTBTREE_HPP

