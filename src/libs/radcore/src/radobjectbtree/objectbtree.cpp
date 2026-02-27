//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        objectbtree.cpp
//
// Subsystem:   Foundation Technologies - radObjectBTree implementation
//
// Description: This file contains the implementation of a balanced binary
//              search tree.
//
// Revisions:   June 26, 2001       Creation
//
//=============================================================================

//============================================================================
// Include Files
//============================================================================

//============================================================================
// Include Files
//============================================================================

#include "pch.hpp"
#include <string.h>
#include <radmemory.hpp>
#include <radmemorymonitor.hpp>
#include <radobject.hpp>
#include <radobjectlist.hpp>
#include <raddebug.hpp>

#include <radobjectbtree.hpp>

#include "objectbtree.hpp"



//============================================================================
// Local variables
//============================================================================

//
// This is the state list for controlling traversals
//
static const int TraversalStateList[ 3 ][ 4 ] =
{
    { 2, 3, 1, 4 },     // Pre order
    { 1, 2, 3, 4 },     // In order
    { 1, 3, 4, 2 }      // Post order
};


//============================================================================
// Class implementation
//============================================================================

#undef NULL
#define NULLNODEINDEX ((short)-1)
#define NULLNODEPTR ((radObjectBTreeNode*)0)
#define NULLNODEINDEXPTR ((short*)0)
#define NULLREFCOUNTPTR ((IRefCount*)0)
#define NULLKEYPTR ((radKey32*)0)

static radObjectBTreeNode* gpNodeTable;
static radObjectBTreeNode*  gpNextFreeNode;
static unsigned int gUsedNodes;
static unsigned int gMaxNodes;

struct FreeNode
{
    radObjectBTreeNode* pNextFree;
};

static inline radObjectBTreeNode* AllocBTreeNode( void )
{
    rTuneAssert( NULLNODEPTR != gpNextFreeNode );
    
    radObjectBTreeNode* pRet = gpNextFreeNode;
    
    gpNextFreeNode = ((FreeNode*)gpNextFreeNode)->pNextFree;
    
    gUsedNodes++;
    
    return pRet;
}

static inline void FreeBTreeNode( radObjectBTreeNode* pNode )
{
    FreeNode * pFn = (FreeNode*) pNode;
    
    ((FreeNode*)pNode)->pNextFree = gpNextFreeNode;
    gpNextFreeNode = pNode;
    
    gUsedNodes--;
}

static inline short Ref( radObjectBTreeNode * pNode )
{
    short index = ( pNode - gpNodeTable );
    
    return index;
}

static inline radObjectBTreeNode* DeRef( short index )
{
    if ( NULLNODEINDEX == index )
    {
        return NULLNODEPTR;
    }
    
    return gpNodeTable + index;
}

void radObjectBTree::Initialize( radObjectBTreeNode* pNodePool, unsigned int maxNodes )
{
    gpNodeTable = pNodePool;
    gMaxNodes = maxNodes;
    gUsedNodes = 0;
    gpNextFreeNode = gpNodeTable;
    
    for( unsigned int i = 0; i < maxNodes - 1; i ++ )
    {
        ((FreeNode*)(gpNodeTable + i))->pNextFree = gpNodeTable + i + 1;
    }
    
    ((FreeNode*)(gpNodeTable + maxNodes - 1))->pNextFree = 0;
}

short radObjectBTree::GetNumAllocatedNodes( void )
{
    return gUsedNodes;
}
    
void radObjectBTree::Terminate( void )
{
    gpNodeTable = NULLNODEPTR;
    gMaxNodes = 0;
}

//=============================================================================
// Function:    radObjectBTree::radObjectBTree
//=============================================================================
// Description: Constructor
//
// Parameters:  allocator - the memory allocator to use
//
//------------------------------------------------------------------------------

radObjectBTree::radObjectBTree( )
    :
    mRootNode( NULLNODEINDEX ),
    m_Size( 0 ),
    m_TraversalOrder( PreOrder ),
    m_pLastTraverseNode( NULLNODEPTR ),
    m_TraversalState( 0 )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radObjectBTree" );
    //
}

//=============================================================================
// Function:    radObjectBTree::~radObjectBTree
//=============================================================================
// Description: Destructor
//
//------------------------------------------------------------------------------
    
radObjectBTree::~radObjectBTree( )
{
    //
    // Clean up the tree
    //
    RemoveAll( );
}

//=============================================================================
// Function:    radObjectBTree::AddObject
//=============================================================================
// Description: Add an object to the tree.  Must not already be there.
//
// Parameters:  key - a key for the object
//              pIObject - the object to add
//
// Return:      Returns true if the object was successfully added, and
//              false otherwise.
//
// Notes:       The object is reference counted and stored in the tree.
//              Two objects with the same key are not presently allowed.
//------------------------------------------------------------------------------

bool radObjectBTree::AddObject
(
    const radKey32& key,
    IRefCount* pIObject
)
{
    //
    // Stop any traversals
    //
    m_pLastTraverseNode = NULLNODEPTR;
    
    //
    // Find a suitable location for this node
    //
    radObjectBTreeNode* pNode = NULLNODEPTR;
    radObjectBTreeNode* pParentNode = NULLNODEPTR;
    int side = 0;
    find
    (
        key,
        &pNode,
        &pParentNode,
        &side
    );

    //
    // If the node does not already exist, add it
    //
    bool nodeAdded = false;
    if( pNode == NULLNODEPTR )
    {
        //
        // Create the new node for this object
        //
        radObjectBTreeNode* pNewNode = AllocBTreeNode( );
        pNewNode->m_key = key;
        pNewNode->m_xIObject = pIObject;
        pNewNode->mLeftNode = NULLNODEINDEX;
        pNewNode->mRightNode = NULLNODEINDEX;

        //
        // If there is are objects so far, use the root node
        //
        if( pParentNode == NULLNODEPTR )
        {
            rAssert( mRootNode == NULLNODEINDEX );
            mRootNode = Ref( pNewNode );
        }
        else
        {
            //
            // Add it to the tree
            //
            if( side > 0 )
            {
                pParentNode->mRightNode = Ref( pNewNode );
            }
            else
            {
                pParentNode->mLeftNode = Ref( pNewNode );
            }
        }

        //
        // Increment the size
        //
        m_Size++;

        nodeAdded = true;
    }

    //
    // Return whether or not a node was added
    //
    return nodeAdded;
}

//=============================================================================
// Function:    radObjectBTree::deleteNode
//=============================================================================
// Description: Copy one node to an other
//
// Returns:     n/a
//
//------------------------------------------------------------------------------

void radObjectBTree::deleteNode
(
    radObjectBTreeNode* pNode
)
{
    rAssert( pNode != NULLNODEPTR );
    rAssert( pNode->mLeftNode == NULLNODEINDEX );
    rAssert( pNode->mRightNode == NULLNODEINDEX );

    pNode->m_xIObject = NULLREFCOUNTPTR;
    FreeBTreeNode( pNode );
}

//=============================================================================
// Function:    radObjectBTree::RemoveObject
//=============================================================================
// Description: Remove an object from the tree based on a key.
//
// Parameters:  key - the key of the object to remove
//
// Return:      Returns true if the object was successfully removed, and
//              false otherwise.
//
//------------------------------------------------------------------------------

bool radObjectBTree::RemoveObject
(
    const radKey32& key
)
{
    //
    // Stop any traversals
    //
    m_pLastTraverseNode = NULLNODEPTR;
    
    //
    // Find the node with the given key
    //
    radObjectBTreeNode* pNode = NULLNODEPTR;
    radObjectBTreeNode* pParentNode = NULLNODEPTR;
    int side = 0;
    find
    (
        key,
        &pNode,
        &pParentNode,
        &side
    );

    //
    // If the node is found, remove it
    //
    bool noderemoved = false;
    if( pNode != NULLNODEPTR )
    {
        //
        // Find out what is pointing to this node
        //
        short * pPointToMe = NULLNODEINDEXPTR;
        
        if( pParentNode == NULLNODEPTR )
        {
            pPointToMe = & mRootNode;
        }
        else if( pParentNode->mLeftNode == Ref( pNode ) )
        {
            pPointToMe = & pParentNode->mLeftNode;
        }
        else
        {
            pPointToMe = & pParentNode->mRightNode;
        }

        //
        // What kind of node are we
        //
        if( pNode->mLeftNode == NULLNODEINDEX )
        {
            if( pNode->mRightNode == NULLNODEINDEX )
            {
                //
                // We are a leaf, just remove ourselves from the tree
                //
                (*pPointToMe) = NULLNODEINDEX;
            }
            else
            {
                //
                // Left subtree empty, replace with right
                //
                (*pPointToMe) = pNode->mRightNode;
                pNode->mRightNode = NULLNODEINDEX;
            }
        }
        else
        {
            if( pNode->mRightNode ==  NULLNODEINDEX )
            {
                //
                // Right subtree empty, replace with left
                //
                (*pPointToMe) = pNode->mLeftNode;
                pNode->mLeftNode = NULLNODEINDEX;
            }
            else
            {
                //
                // In middle of tree.  We could replace the node with
                // the predessor or the successor.
                // The sucessor is just the leftmost node in the
                // right subtree.
                //
                pPointToMe = &pNode->mRightNode;
                radObjectBTreeNode* pSuccessor = DeRef( pNode->mRightNode );
                while( pSuccessor->mLeftNode != NULLNODEINDEX )
                {
                    pPointToMe = &pSuccessor->mLeftNode;
                    pSuccessor = DeRef( pSuccessor->mLeftNode );
                }

                // We don't actually move it up here, we just copy
                // its information and then remove it
                copyNode( pSuccessor, pNode );
                (*pPointToMe) = pSuccessor->mRightNode;

                pNode = pSuccessor;
                pNode->mLeftNode = NULLNODEINDEX;
                pNode->mRightNode = NULLNODEINDEX;
            }
        }

        //
        // Delete the node
        //
        deleteNode( pNode );

        //
        // Adjust the size of the tree
        //
        m_Size--;

        noderemoved = true;
    }

    //
    // Return whether or not the node was removed
    //
    return noderemoved;
}

//=============================================================================
// Function:    radObjectBTree::RemoveObject
//=============================================================================
// Description: Remove an object from the tree based on the object itself.
//
// Parameters:  pIObject - the object to remove
//
// Note:        Since all elements are stored, by definition, based on there key
//              value, finding the actual object is not an optimized procedure.
//
// Return:      Returns true if the object was successfully removed, and
//              false otherwise.
//
//------------------------------------------------------------------------------

bool radObjectBTree::RemoveObject( IRefCount* pIObject )
{
    // Fast return for simple case
    if( pIObject == NULLREFCOUNTPTR )
    {
        return false;
    }

    //
    // Travese the BTree nodes until the suitable one is found, then remove it.
    //
    radKey32 mykey = 0;
    IRefCount* pMyObject = GetFirst( PreOrder, &mykey );
    while( ( pMyObject != NULLREFCOUNTPTR ) && ( pMyObject != pIObject ) )
    {
        pMyObject = GetNext( &mykey );
    }

    //
    // Did we find it?
    //
    bool noderemoved = false;
    if( pMyObject != NULLREFCOUNTPTR )
    {
        //
        // Remove the node
        //
        noderemoved = RemoveObject( mykey );
        pMyObject = NULLREFCOUNTPTR;
    }

    //
    // Stop any traversals
    //
    m_pLastTraverseNode = NULLNODEPTR;

    return noderemoved;
}

//=============================================================================
// Function:    radObjectBTree::RemoveAll
//=============================================================================
// Description: Remove all objects from the tree
//
// Parameters:  none
//
//------------------------------------------------------------------------------

void radObjectBTree::RemoveAll( void )
{
    //
    // Stop any pending traversals
    //
    m_pLastTraverseNode = NULLNODEPTR;

    //
    // The remove all is a recursive function.  This may cause a stack
    // explosion, but will avoid unnecessary dynamic allocation for
    // our stack.
    // Use a helper function do destroy the tree.
    //
    if( mRootNode != NULLNODEINDEX )
    {
        RemoveAll_Helper( DeRef( mRootNode ) );
    }

    //
    // Clean up
    //
    mRootNode = NULLNODEINDEX;
    m_pLastTraverseNode = NULLNODEPTR;
    m_Size = 0;
}

//=============================================================================
// Function:    radObjectBTree::RemoveAll_Helper
//=============================================================================
// Description: Helper function to remove all nodes from the tree
//
// Pre Condion: The node must not be NULL
//
// Parameters:  none
//
//------------------------------------------------------------------------------

void radObjectBTree::RemoveAll_Helper
(
    radObjectBTreeNode* pNode
)
{
    rAssert( pNode != NULLNODEPTR );

    // Destroy the left subtree
    if( pNode->mLeftNode != NULLNODEINDEX )
    {
        RemoveAll_Helper( DeRef( pNode->mLeftNode ) );
        pNode->mLeftNode = NULLNODEINDEX;
    }

    // Destroy the right subtree
    if( pNode->mRightNode != NULLNODEINDEX )
    {
        RemoveAll_Helper( DeRef( pNode->mRightNode ) );
        pNode->mRightNode = NULLNODEINDEX;
    }

    //
    // Destroy this node
    //
    deleteNode( pNode );
}

//=============================================================================
// Function:    radObjectBTree::FindObject
//=============================================================================
// Description: Find an object in the tree based on its key
//
// Parameters:  the key of the object to find
//
// Return:      Returns a pointer to the object with a matching key,
//              or NULL if none are found.
//
//------------------------------------------------------------------------------

IRefCount* radObjectBTree::FindObject( const radKey32& key )
{
    //
    // Find the node
    //
    radObjectBTreeNode* pNode = NULLNODEPTR;
    radObjectBTreeNode* pParentNode = NULLNODEPTR;
    int side = 0;
    find
    (
        key,
        &pNode,
        &pParentNode,
        &side
    );

    //
    // Get the object
    //
    IRefCount* pObject = NULLREFCOUNTPTR;
    if( pNode != NULLNODEPTR )
    {
        //
        // The key was found, get the object
        //
        pObject = pNode->m_xIObject;
    }
    
    //
    // Return the object
    //
    return pObject;
}

//=============================================================================
// Function:    radObjectBTree::GetFirst
//=============================================================================
// Description: Initialize a traversal of the tree
//
// Parameters:  traversalOrder - specify the traversal order
//              pKey - (out) the key of the first node (ignored if NULL)
//
// Returns:     Returns a pointer to the first node found.
//
// Note:        If ppObject returns a pointer to NULL, there
//              is no objects in the tree to traverse.
//
//------------------------------------------------------------------------------

IRefCount* radObjectBTree::GetFirst
(
    TraversalOrder traversalOrder,
    radKey32* pKey
)
{
    //
    // Set the traversal order
    //
    m_TraversalOrder = traversalOrder;

    //
    // Verify the traversal order
    //
    if( m_TraversalOrder >= radObjectBTree::MaxTraversalOrder )
    {
        rWarningMsg( 0, "Using an unknown traversal order!" );
        return NULLREFCOUNTPTR;
    }

    //
    // Stop pending traversals and set the first node to the root
    //
    m_pLastTraverseNode = NULLNODEPTR;
    if( mRootNode != NULLNODEINDEX )
    {
        m_pLastTraverseNode = DeRef( mRootNode );
    }

    //
    // Set the traversal state
    //
    m_TraversalState = TraversalStateList[ m_TraversalOrder ][ 0 ];

    //
    // Get the first node
    //
    return GetNext( pKey );
}

//=============================================================================
// Function:    radObjectBTree::GetNext
//=============================================================================
// Description: Get the next object in a tree.  This is based on the traversal
//              type specified in GetFirst.
//
// Parameters:  pKey - (out) the key of the first node (ignored if NULL)
//
// Returns:     Returns a pointer to the next object in the tree.  If we
//              return a NULL, we are finished traversing the tree.  Also,
//              if something disrupts the traversal such as adding or removing
//              a node, then NULL is returned.
//
//------------------------------------------------------------------------------

IRefCount* radObjectBTree::GetNext
(
    radKey32* pKey
)
{
    //
    // Use the GetNextNode helper
    //
    radObjectBTreeNode* pNode = NULLNODEPTR;
    IRefCount* pNext = NULLREFCOUNTPTR;
    pNode = GetNextNode( );

    //
    // If it is the last
    //
    if( pNode == NULLNODEPTR )
    {
        if( pKey != NULLKEYPTR )
        {
            (*pKey) = 0;
        }
    }
    else
    {
        //
        // Return the info
        //
        if( pKey != NULLKEYPTR )
        {
            (*pKey) = pNode->m_key;
        }
        pNext = pNode->m_xIObject;
    }

    return pNext;
}

//=============================================================================
// Function:    radObjectBTree::GetSize
//=============================================================================
// Description: Gets the number of objects stored in the tree
//
// Parameters:  none
//
// Return:      Returns the number of objects in the tree
//
//------------------------------------------------------------------------------

unsigned int radObjectBTree::GetSize( void )
{
    return m_Size;
}

#ifdef RAD_DEBUG
//=============================================================================
// Function:    radObjectBTree::DisplayTree
//=============================================================================
// Description: Debug output of the tree
//
// Parameters:  none
//
// Return:      n/a
//
//------------------------------------------------------------------------------

void radObjectBTree::DisplayTree
(
    radObjectBTreeNode* pNode,
    int indent
)
{
    // Do spaces
    for( int i = 0; i < indent; i++ )
    {
        rDebugString( "  " );
    }

    // Print node
    if( pNode == NULLNODEPTR )
    {
        rDebugString( "( --, -- )\n" );
    }
    else
    {
        char buf[ 256 ];
        sprintf( buf, "( %i, %p )\n", pNode->m_key, static_cast< IRefCount* >( pNode->m_xIObject ) );
        rDebugString( buf );

        // Print subtrees
        DisplayTree( DeRef( pNode->mLeftNode ), indent + 1 );
        DisplayTree( DeRef( pNode->mRightNode ), indent + 1 );
    }
}
#endif

//=============================================================================
// Function:    radObjectBTree::GetNextNode
//=============================================================================
// Description: Gets the next node according to the current traversal order
//
// Returns:     Returns the next node in the traversal
//
//------------------------------------------------------------------------------

radObjectBTreeNode* radObjectBTree::GetNextNode( )
{
    //
    // Store the next node
    //
    radObjectBTreeNode* pNext = m_pLastTraverseNode;

    //
    // If there is no active traversal, just return NULL
    //
    if( m_pLastTraverseNode == NULLNODEPTR )
    {
        pNext = NULLNODEPTR;
    }
    else
    {
        //
        // Find the next node
        //
        while( true )
        {
            //
            // We use a state based traversal using our traversal state list.
            //
            if( m_TraversalState == 1 )
            {
                if( pNext->mLeftNode != NULLNODEINDEX )
                {
                    pNext = DeRef( pNext->mLeftNode );
                    m_TraversalState = TraversalStateList[ m_TraversalOrder ][ 0 ];
                }
                else
                {
                    m_TraversalState = TraversalStateList[ m_TraversalOrder ][ 1 ];
                }
            }

            if( m_TraversalState == 2 )
            {
                m_TraversalState = TraversalStateList[ m_TraversalOrder ][ 2 ];
                break;
            }

            if( m_TraversalState == 3 )
            {
                if( pNext->mRightNode != NULLNODEINDEX )
                {
                    pNext = DeRef( pNext->mRightNode );
                    m_TraversalState = TraversalStateList[ m_TraversalOrder ][ 0 ];
                }
                else
                {
                    m_TraversalState = TraversalStateList[ m_TraversalOrder ][ 3 ];
                }
            }

            if( m_TraversalState == 4 )
            {
                //
                // Get the parent node (equivalent to popping off the stack like
                // as we used to do).
                //
                radObjectBTreeNode* pCurrentNode = NULLNODEPTR;
                int side = 0;
                find
                (
                    m_pLastTraverseNode->m_key,
                    &pCurrentNode,
                    &pNext,
                    &side
                );
                rAssert( pCurrentNode == m_pLastTraverseNode );
                m_pLastTraverseNode = pNext;

                //
                // If there is no parent, we are done
                //
                if( pNext == NULLNODEPTR )
                {
                    break;
                }
                else
                {
                    rAssert( side != 0 );
                    
                    if( side < 0 )
                    {
                        m_TraversalState = TraversalStateList[ m_TraversalOrder ][ 1 ];
                    }
                    else
                    {
                        m_TraversalState = TraversalStateList[ m_TraversalOrder ][ 3 ];
                    }
                }
            }
        }

        //
        // Update the last traversed node.  If this is the
        // last node of the tree, shut down the traversal.
        //
        m_pLastTraverseNode = pNext;
    }

    //
    // Return the next node
    //
    return pNext;
}


//=============================================================================
// Inline function definitions
//=============================================================================

//=============================================================================
// Function:    radObjectBTree::find
//=============================================================================
// Description: Finds the node closest to the given key.
//
// Parameters:  key - a key to match nodes with
//              ppNode - (out) the closest node to the given key
//              ppParent - (out) the parent of the closest node to the
//                         given key
//              pLocation - (out) the location where the key would reside
//
// Note:        The location where the key would reside is one of three values:
//              < 0   - The given key would reside to the left of the node
//                      returned in pNode.  Also, the left subtree of pNode is
//                      NULL.
//              ==0   - the parent must be NULL.
//              > 0   - The given key would reside to the right of the node
//                      returned in pNode.  Also, the right subtree of pNode is
//                      NULL.
//
// Returns:     n/a
//
//------------------------------------------------------------------------------

void radObjectBTree::find
(
    const radKey32& key,
    radObjectBTreeNode** ppNode,
    radObjectBTreeNode** ppParent,
    int* pSide
)
{
    (*pSide) = 0;
    (*ppNode) = NULLNODEPTR;
    (*ppParent) = NULLNODEPTR;

    //
    // Start off at the root node
    //
    (*ppNode) = DeRef( mRootNode );

    //
    // Iterate the nodes
    //
    while( (*ppNode) != NULLNODEPTR )
    {
        //
        // Do the keys match?
        //
        if( key == (*ppNode)->m_key )
        {
            break;
        }

        //
        // Is the the node to the left or to the right?
        // (We don't just subtract them because pLocation
        // is only an int and this might lead to an overflow).
        //
        (*ppParent) = (*ppNode);
        if( key < (*ppNode)->m_key )
        {
            (*ppNode) = DeRef( (*ppNode)->mLeftNode );
            (*pSide) = -1;
        }
        else
        {
            (*ppNode) = DeRef( (*ppNode)->mRightNode );
            (*pSide) = 1;
        }
    }
}

//=============================================================================
// Function:    radObjectBTree::copyNode
//=============================================================================
// Description: Copy a node's information (not its subnodes)
//
//-----------------------------------------------------------------------------

void radObjectBTree::copyNode
(
    radObjectBTreeNode* pSrc,
    radObjectBTreeNode* pDest
)
{
    pDest->m_key = pSrc->m_key;
    pDest->m_xIObject = pSrc->m_xIObject;
}


