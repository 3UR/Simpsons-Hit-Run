//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   FE Builder Data Structure
//
// Description: This contains the XMLTree class which represents XML tree.
//
// Authors:     Eric Tong
//
//===========================================================================

// Recompilation protection flag.
#ifndef _XMLTREE_H_
#define _XMLTREE_H_

//===========================================================================
// Nested Includes
//===========================================================================
#include "strings/pcstring.h"

//===========================================================================
// Forward References
//===========================================================================

class XMLNode;

//===========================================================================
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//===========================================================================

//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: This class is used to represent a XMLTree structure
//
// Constraints: None
//
//===========================================================================

class XMLTree
{
    public:
        XMLTree();
        XMLTree( XMLNode* node );
        virtual ~XMLTree();

        bool LoadTreeFromFile( const char* filename, const char* rootSectionName );
        bool LoadTreeFromBuffer( char* buffer, 
                                 const unsigned int bufferSize, 
                                 const char* rootSectionName );

        bool SetCurrentElementByName( const char* name );
        bool SetCurrentElementByIndex( const int index );

        void SetFilename( const char* filename );
        bool GetFilename( PascalCString& returnString );
        int GetName( PascalCString &returnString );
        int GetAttribute( const char* name, PascalCString &returnString );
        bool GetAttribute( const char* name, int* returnInt );
        bool GetAttribute( const char* name, double* returnDouble );
        bool GetAttribute( const char* name, bool* returnBool );

        XMLTree* GetSubTreeByName( const char* elementName );
        XMLTree* GetSubTreeByIndex( const int index );

        // Win Debug functions
        void DumpTree();

    private:
        XMLNode* m_fileRootNode;
        XMLNode* m_rootNode;
        XMLNode* m_currentNode;
        PascalCString m_filename;
        bool m_loadedFromFile;

        bool LoadTreeWorker( const char* rootSectionName );
        // Win Debug functions
        void PrintNode( XMLNode* node, PascalCString spaces );
};

//===========================================================================
// Inlines
//===========================================================================


#endif                                  // End conditional inclusion

