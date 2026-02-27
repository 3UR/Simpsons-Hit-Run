//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   FE Builder Data Structure
//
// Description: This contains the XMLTree class.
//
// Authors:     Eric Tong
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================

#include "xml/XMLTree.h"
#include "xml/XMLParser.h"
#include <string.h>
#include <stdlib.h>
#include <utility/memory.h>

//===========================================================================
// Local Constants, Typedefs, and Macros
//===========================================================================
static void OutputDebugString( PascalCString ){};

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================

//===========================================================================
// Member Functions
//===========================================================================

//===========================================================================
// XMLTree::XMLTree
//===========================================================================
// Description: XMLTree's constructor
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      Nothing
//
//===========================================================================
XMLTree::XMLTree( )
{
    m_fileRootNode = NULL;
    m_rootNode = m_fileRootNode;
    m_currentNode = m_rootNode;
}

//===========================================================================
// XMLTree::XMLTree
//===========================================================================
// Description: XMLTree's constructor
//
// Constraints:    None
//
// Parameters:    node - the root node
//
// Return:      Nothing
//
//===========================================================================
XMLTree::XMLTree( XMLNode* node )
{
    m_fileRootNode = NULL;
    m_rootNode = node;
    m_currentNode = m_rootNode;
}

//===========================================================================
// XMLTree::~XMLTree
//===========================================================================
// Description: XMLTree's destructor
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      Nothing
//
//===========================================================================
XMLTree::~XMLTree()
{
    p3d::AllocType type = p3d::GetCurrentAlloc();
    p3d::SetCurrentAlloc( p3d::ALLOC_TEMPORARY );

    m_rootNode = NULL;
    m_currentNode = NULL;

    // If root node is loaded from file, that's the master copy, so we delete it
    if( m_fileRootNode != NULL )
    {
        delete m_fileRootNode;
    }
    m_fileRootNode = NULL;

    p3d::SetCurrentAlloc( type );

}


//===========================================================================
// XMLTree::LoadTreeFromBuffer
//===========================================================================
// Description: This recreates a tree from memory buffer.
//
// Constraints:    None
//
// Parameters:    buffer - the memory buffer to read from.
//              bufferSize - the size of the buffer.
//
// Return:      bool - whether the retrieve was sucessful
//
//===========================================================================
bool XMLTree::LoadTreeFromBuffer( char* buffer, 
                                  const unsigned int bufferSize, 
                                  const char* rootSectionName )
{
    p3d::AllocType type = p3d::GetCurrentAlloc();
    p3d::SetCurrentAlloc( p3d::ALLOC_TEMPORARY );

    // Delete the previous root node if it's loaded from file
    if( m_fileRootNode != NULL )
    {
        delete m_fileRootNode;
    }

    XMLParser parser;
    m_fileRootNode = parser.ParseFromBuffer( buffer, bufferSize );

    m_filename = "";

    p3d::SetCurrentAlloc( type );

    return( this->LoadTreeWorker( rootSectionName ) );
}


//===========================================================================
// XMLTree::LoadTreeFromFile
//===========================================================================
// Description: This recreates a tree from file
//
// Constraints:    None
//
// Parameters:    filename - the file name
//
// Return:      bool - whether the retrieve was sucessful
//
//===========================================================================
bool XMLTree::LoadTreeFromFile( const char* filename, const char* rootSectionName )
{
    p3d::AllocType type = p3d::GetCurrentAlloc();
    p3d::SetCurrentAlloc( p3d::ALLOC_TEMPORARY );

    // Delete the previous root node if it's loaded from file
    if( m_fileRootNode != NULL )
    {
        delete m_fileRootNode;
    }

    XMLParser parser;
    m_fileRootNode = parser.ParseFromFile( filename );

    m_filename = filename;

    p3d::SetCurrentAlloc( type );


    return( this->LoadTreeWorker( rootSectionName ) ); 
    
/*
    // Default node to not found
    m_rootNode = NULL;

    

    if( m_fileRootNode != NULL )
    {
        XMLNodeList* childNodes = m_fileRootNode->GetChildNodes();
        int numChildNodes = childNodes->GetLength();
        for( int i=0; i<numChildNodes; i++ )
        {
            // Match section name
            if( childNodes->GetItem(i)->GetName().EqualsInsensitive( rootSectionName ) )
            {
                // Found the section, set it as the root node
                m_rootNode = childNodes->GetItem(i);
                m_currentNode = m_rootNode;
                return true;
            }
        }
    }

    m_rootNode = NULL;
    m_currentNode = m_rootNode;
    return false;
*/
}

//===========================================================================
// XMLTree::LoadTreeWorker
//===========================================================================
// Description: Does the grunt work for LoadTreeFromFile and 
//              LoadTreeFromBuffer.
//
// Constraints:    None
//
// Parameters:    
//
// Return:      bool - whether the retrieve was sucessful
//
//===========================================================================
bool XMLTree::LoadTreeWorker( const char* rootSectionName )
{
    // Default node to not found
    m_rootNode = NULL;

    if( m_fileRootNode != NULL )
    {
        XMLNodeList* childNodes = m_fileRootNode->GetChildNodes();
        int numChildNodes = childNodes->GetLength();
        for( int i=0; i<numChildNodes; i++ )
        {
            // Match section name
            if( childNodes->GetItem(i)->GetName().EqualsInsensitive( rootSectionName ) )
            {
                // Found the section, set it as the root node
                m_rootNode = childNodes->GetItem(i);
                m_currentNode = m_rootNode;
                return true;
            }
        }
    }

    m_rootNode = NULL;
    m_currentNode = m_rootNode;
    return false;
}


//===========================================================================
// XMLTree::SetCurrentElementByName
//===========================================================================
// Description: Looks up the sub element by name, and set as current element
//
// Constraints:    None
//
// Parameters:    name - the name of the sub element
//
// Return:      bool - whether the action was sucessful
//
//===========================================================================
bool XMLTree::SetCurrentElementByName( const char* name )
{
    if( m_rootNode == NULL )
    {
        m_currentNode = NULL;
        return false;
    }

    XMLNodeList* childNodes = m_rootNode->GetChildNodes();
    int numChildNodes = childNodes->GetLength();
    for( int i=0; i<numChildNodes; i++ )
    {
        // Is the name equal? (Case insensitive)
        if( childNodes->GetItem(i)->GetName().EqualsInsensitive( name ) )
        {
            m_currentNode = childNodes->GetItem(i);
            return true;
        }
    }

    m_currentNode = NULL;
    return false;
}

//===========================================================================
// XMLTree::SetCurrentElementByIndex
//===========================================================================
// Description: Finds the sub element by index, and set as current element
//
// Constraints:    None
//
// Parameters:    index - the index
//
// Return:      bool - whether the action was sucessful
//
//===========================================================================
bool XMLTree::SetCurrentElementByIndex( const int index )
{
    if( m_rootNode == NULL )
    {
        m_currentNode = NULL;
        return false;
    }

    XMLNodeList* childNodes = m_rootNode->GetChildNodes();
    int numChildNodes = childNodes->GetLength();
    
    if( index < numChildNodes )
    {
        m_currentNode = childNodes->GetItem( index );
        return true;
    }
    else
    {
        m_currentNode = NULL;
        return false;
    }
}

//===========================================================================
// XMLTree::GetFilename
//===========================================================================
// Description: Retrieve the name of the file attached
//
// Constraints:    None
//
// Parameters:    returnString - the return string buffer
//
// Return:      int - length of string
//
//===========================================================================
bool XMLTree::GetFilename( PascalCString &returnString )
{
    returnString = m_filename;
    return true;
}

//===========================================================================
// XMLTree::SetFilename
//===========================================================================
// Description: Set the name of the file this tree is based on
//
// Constraints:    None
//
// Parameters:    filename - the new filename value
//
//===========================================================================
void XMLTree::SetFilename( const char* filename )
{
    m_filename = filename;
}


//===========================================================================
// XMLTree::GetName
//===========================================================================
// Description: Retrieve the name of the current element
//
// Constraints:    None
//
// Parameters:    returnString - the return string buffer
//
// Return:      int - length of string
//
//===========================================================================
int XMLTree::GetName( PascalCString &returnString )
{
    returnString = m_currentNode->GetName();
    return returnString.Length();
}

//===========================================================================
// XMLTree::GetAttribute
//===========================================================================
// Description: Retrieve the attribute of the current element
//
// Constraints:    None
//
// Parameters:    name - the name of the attribute
//              returnString - the return PascalCString buffer
//
// Return:      int - the number of characters of retrieved string
//
//===========================================================================
int XMLTree::GetAttribute( const char* name, PascalCString &returnString )
{
    if( m_currentNode == NULL )
    {
        returnString = "";
        return 0;
    }

    XMLAttributeList* attributes = m_currentNode->GetAttributes();
    if( attributes == NULL )
    {
        returnString = "";
        return 0;
    }

    int attrCount = attributes->GetLength();
    for (int i = 0; i < attrCount; i++)
    {
        // See if there is the attribute specified
        XMLAttribute* attribute = attributes->GetItem(i);
        if( attribute->GetName().EqualsInsensitive(name) )
        {
            returnString = attribute->GetValue();
            return returnString.Length();
        }
    }
    
    // No attribute of that name found
    returnString = "";
    return 0;
}


//===========================================================================
// XMLTree::GetAttribute
//===========================================================================
// Description: Retrieve the attribute of the current element
//
// Constraints:    None
//
// Parameters:    name - the name of the attribute
//              returnInt - the value returned as integer
//
// Return:      bool - whether the retrieve was sucessful
//
//===========================================================================
bool XMLTree::GetAttribute( const char* name, int* returnInt )
{
    PascalCString string;
    
    int stringLength = GetAttribute( name, string );
    if( stringLength == 0 )
    {
        *returnInt = 0;
        return false;
    }
    else
    {
        *returnInt = atoi( string );
        return true;
    }
}

//===========================================================================
// XMLTree::GetAttribute
//===========================================================================
// Description: Retrieve the attribute of the current element
//
// Constraints:    None
//
// Parameters:    name - the name of the attribute
//              returnDouble - the value returned as double
//
// Return:      bool - whether the retrieve was sucessful
//
//===========================================================================
bool XMLTree::GetAttribute( const char* name, double* returnDouble )
{
    PascalCString string;
    
    int stringLength = GetAttribute( name, string );
    if( stringLength == 0 )
    {
        *returnDouble = 0.0;
        return false;
    }
    else
    {
        *returnDouble = atof( string );
        return true;
    }
}

//===========================================================================
// XMLTree::GetAttribute
//===========================================================================
// Description: Retrieve the attribute of the current element
//
// Constraints:    None
//
// Parameters:    name - the name of the attribute
//              returnBool - the value returned as boolean
//
// Return:      bool - whether the retrieve was sucessful
//
//===========================================================================
bool XMLTree::GetAttribute( const char* name, bool* returnBool )
{
    PascalCString string;
    
    int stringLength = GetAttribute( name, string );
    if( stringLength == 0 )
    {
        return false;
    }
    else
    {
        // We treat anything other than "true" (case insensitive) is false
        string.ToUpper();
        if( strcmp( string, "TRUE" ) == 0 )
        {
            *returnBool = true;
        }
        else
        {
            *returnBool = false;
        }
        return true;
    }
}

//===========================================================================
// XMLTree::GetSubTreeByName
//===========================================================================
// Description: Find the subtree by name and return that subtree
//
// Constraints:    You have to make sure to free the memory if it returns a subtree!
//
// Parameters:    name - the name of the element to lookup
//
// Return:      XMLTree* - the pointer to the subtree, or NULL if nothing found
//
//===========================================================================
XMLTree* XMLTree::GetSubTreeByName( const char* name )
{
    if( m_rootNode == NULL )
    {
        return NULL;
    }

    XMLNodeList* childNodes = m_rootNode->GetChildNodes();
    int numChildNodes = childNodes->GetLength();
    for( int i=0; i<numChildNodes; i++ )
    {
        // First lookup the element
        if( childNodes->GetItem(i)->GetName().EqualsInsensitive( name ) )
        {
            p3d::AllocType type = p3d::GetCurrentAlloc();
            p3d::SetCurrentAlloc( p3d::ALLOC_TEMPORARY );
            return new XMLTree( childNodes->GetItem(i) );
            p3d::SetCurrentAlloc( type );

        }
    }
    return NULL;
}

//===========================================================================
// XMLTree::GetSubTreeByIndex
//===========================================================================
// Description: This returns the subtree by index
//
// Constraints:    You have to make sure to free the memory if it returns a subtree!
//
// Parameters:    index - the index of the subtree
//
// Return:      XMLTree* - the pointer to the subtree, or NULL if nothing found
//
//===========================================================================
XMLTree* XMLTree::GetSubTreeByIndex( const int index )
{
    if( m_rootNode == NULL )
    {
        return NULL;
    }

    XMLNodeList* childNodes = m_rootNode->GetChildNodes();
    int numChildNodes = childNodes->GetLength();
    
    if( index < numChildNodes )
    {
        p3d::AllocType type = p3d::GetCurrentAlloc();
        p3d::SetCurrentAlloc( p3d::ALLOC_TEMPORARY );

        return new XMLTree( childNodes->GetItem( index ) );
        p3d::SetCurrentAlloc( type );

    }
    else
    {
        return NULL;
    }
}

//===========================================================================
// XMLTree::PrintNode
//===========================================================================
// Description: Prints the current node, and nested to its child node
//
// Constraints:    None
//
// Parameters:    node - the current node
//              spaces - the spaces to prepend
//
// Return:      None
//
//===========================================================================
void XMLTree::PrintNode( XMLNode* node, PascalCString spaces )
{

#ifndef SCROOBY_RUNTIME

    PascalCString thisLine;

    thisLine += spaces;
    thisLine += "(";
    thisLine += node->GetName();
    thisLine += ")";

    thisLine += "(";
    XMLAttributeList* attributes= node->GetAttributes();
    int numItems = attributes->GetLength();
    int i;
    for( i=0; i<numItems; i++ )
    {
        XMLAttribute* attribute = attributes->GetItem( i );
        thisLine += "(";
        thisLine += attribute->GetName();
        thisLine += ",";
        thisLine += attribute->GetValue();
        thisLine += ")";
    }
    thisLine += ")\n";

    OutputDebugString( thisLine );

    XMLNodeList* childNodes= node->GetChildNodes();
    numItems = childNodes->GetLength();
    for( i=0; i<numItems; i++ )
    {
        XMLNode* childNode = childNodes->GetItem( i );
        PascalCString tempSpaces = spaces;
        tempSpaces += "  ";
        PrintNode( childNode, tempSpaces );
    }

    thisLine = spaces;
    thisLine += "(/";
    thisLine += node->GetName();
    thisLine += ")\n";
    OutputDebugString( thisLine );

#endif // SCROOBY_RUNTIME

}

//===========================================================================
// XMLTree::DumpTree
//===========================================================================
// Description: Walk the tree and print it out
//
// Constraints:    None
//
// Parameters:    rootNode - the root node
//
// Return:      None
//
//===========================================================================
void XMLTree::DumpTree()
{

#ifndef SCROOBY_RUNTIME

    OutputDebugString( "====================" );
    PrintNode( this->m_rootNode, "" );
    OutputDebugString( "====================" );

#endif // SCROOBY_RUNTIME

}
